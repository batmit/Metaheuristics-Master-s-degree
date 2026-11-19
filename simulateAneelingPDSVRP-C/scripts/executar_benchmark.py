"""Executa o SA, valida certificados e imprime somente a tabela de resultados."""
import argparse
from copy import deepcopy
import concurrent.futures
from collections import deque
from contextlib import contextmanager
from functools import lru_cache
import errno
import csv
import json
import math
import os
from pathlib import Path
import subprocess
import sys
import threading
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]


def localizar_executavel():
    """Localiza o worker compilado, respeitando o nome do sistema operacional."""
    # WSL pode executar arquivos PE do Windows, mas nao transfere automaticamente
    # PDSVRP_WORKER. Aceitar um .exe nesse caso reiniciava o executor Python.
    name = 'pdsvrp_sa.exe' if os.name == 'nt' else 'pdsvrp_sa'
    path = ROOT/name
    if path.is_file() and (os.name == 'nt' or os.access(path, os.X_OK)):
        with path.open('rb') as stream:
            magic = stream.read(4)
        native = magic[:2] == b'MZ' if os.name == 'nt' else magic == b'\x7fELF'
        if native:
            return path
        raise RuntimeError(f'{name} nao e um executavel nativo deste ambiente. '
                           'Recompile com python3 scripts/compilar.py no Linux/WSL '
                           'ou python scripts/compilar.py no Windows.')
    raise FileNotFoundError(f'Executavel nativo {name} nao encontrado. '
                            'Compile com python3 scripts/compilar.py no Linux/WSL '
                            'ou python scripts/compilar.py no Windows.')


@lru_cache(maxsize=60)
def dados_instancia(instance):
    """Carrega uma vez os dados oficiais; tuplas impedem alteracao do cache."""
    size = 'small' if int(instance.split('-')[0]) <= 30 else 'large'
    lines = (ROOT/f'instances/benchmark/{size}/{instance}.txt').read_text().splitlines()
    speed = float(lines[1].split(',')[1])
    fleet = int(lines[0].split(',')[1])
    nodes = tuple(tuple(float(v) for v in line.split()) for line in lines[7:] if line.strip())
    tau = tuple(tuple(int(v) for v in line.split()) for line in
           (ROOT/f'instances/drone_times/tau_kj_{instance}.txt').read_text().splitlines()[1:] if line.strip())
    if (not math.isfinite(speed) or speed <= 0 or fleet < 1 or
            len(nodes) != int(instance.split('-')[0]) + 1 or
            any(len(r) != 4 or r[0] != i or not all(map(math.isfinite, r)) or r[3] < 0
                for i, r in enumerate(nodes)) or len(tau) != len(nodes)-1 or
            any(len(r) != fleet+1 or r[0] != i or any(t != -1 and t <= 0 for t in r[1:])
                for i, r in enumerate(tau, 1))):
        raise ValueError(f'Dados oficiais inconsistentes: {instance}')
    return speed, fleet, nodes, tuple(r[1:] for r in tau)


def exigir(condicao, mensagem):
    """Ao contrario de assert, esta verificacao permanece ativa com python -O."""
    if not condicao:
        raise ValueError(f'Certificado invalido: {mensagem}')


def numero(valor):
    """Rejeita booleanos, strings, NaN e infinitos antes de fazer contas."""
    return type(valor) in (int, float) and math.isfinite(valor)


def validar(certificado, instance, trucks, metric='manhattan', data=None):
    """Recalcula objetivo, cobertura e capacidade sem usar caches ou deltas do C.

    As missoes sao viagens completas deposito-cliente-deposito. A viabilidade
    fisica de cada par cliente/k e definida pela tabela oficial de tempos.
    """
    if metric not in ('manhattan', 'euclidean'):
        raise ValueError(f'Metrica desconhecida: {metric}')
    speed, fleet, nodes, times = dados_instancia(instance) if data is None else data
    def distance(a, b):
        dx, dy = nodes[a][1]-nodes[b][1], nodes[a][2]-nodes[b][2]
        return math.hypot(dx, dy) if metric == 'euclidean' else abs(dx)+abs(dy)
    exigir(type(trucks) is int and trucks > 0, 'numero de caminhoes')
    exigir(isinstance(certificado, dict) and
           {'routes', 'missions', 'makespan'} <= certificado.keys(), 'estrutura')
    exigir(numero(certificado['makespan']) and certificado['makespan'] >= 0, 'makespan')
    routes = certificado['routes']
    exigir(isinstance(routes, list) and len(routes) == trucks, 'quantidade de rotas')
    seen = []
    value = 0.0
    for route in routes:
        exigir(isinstance(route, list) and
               all(type(j) is int and 1 <= j < len(nodes) for j in route), 'clientes da rota')
        seen.extend(route)
        path = [0, *route, 0]
        value = max(value, sum(distance(a, b) / speed * 60
                               for a, b in zip(path, path[1:])))
    missions = certificado['missions']
    exigir(isinstance(missions, list), 'lista de missoes')
    # Primeiro confere TODAS as missoes; depois verifica sobreposicoes.
    for m in missions:
        exigir(isinstance(m, dict) and {'client', 'k', 'start', 'end'} <= m.keys(), 'missao')
        j, k, start, end = m['client'], m['k'], m['start'], m['end']
        exigir(type(j) is int and 1 <= j < len(nodes), 'cliente do drone')
        exigir(type(k) is int and 1 <= k <= fleet, 'quantidade de drones')
        exigir(numero(start) and numero(end) and 0 <= start <= end, 'horarios')
        exigir(times[j-1][k-1] > 0 and
               math.isclose(end-start, times[j-1][k-1], rel_tol=1e-9, abs_tol=1e-7),
               'duracao ou combinacao cliente/k inviavel')
        seen.append(j)
        value = max(value, end)
    for m in missions:
        # Intervalos [inicio, fim): drones liberados podem partir no mesmo instante.
        exigir(sum(x['k'] for x in missions if x['start'] <= m['start'] < x['end']) <= fleet,
               'capacidade simultanea de drones excedida')
    exigir(sorted(seen) == list(range(1, len(nodes))), 'cliente ausente ou atendido mais de uma vez')
    exigir(math.isfinite(value) and
           math.isclose(value, certificado['makespan'], rel_tol=1e-9, abs_tol=1e-7),
           'objetivo difere do makespan recalculado')
    return value


CENTRAL = ROOT/'results/best.json'


@lru_cache(maxsize=60)
def limite_inferior_local(instance):
    """Cada cliente exige ao menos uma viagem direta, por caminhao ou drone.

    Pela desigualdade triangular Manhattan, uma rota contendo j nunca custa
    menos que deposito-j-deposito. Ignorar a competicao por recursos apenas
    enfraquece este limite, que vale para qualquer numero positivo de caminhoes.
    """
    speed, _, nodes, times = dados_instancia(instance)
    bounds = []
    for j, node in enumerate(nodes[1:], 1):
        truck = 2 * (abs(node[1]-nodes[0][1]) + abs(node[2]-nodes[0][2])) / speed * 60
        drone = min((t for t in times[j-1] if t > 0), default=math.inf)
        bounds.append((min(truck, drone), j))
    return max(bounds, default=(0.0, None))


def catalogo():
    """Le valores e status da literatura; esta e a unica fonte do filtro de configuracoes."""
    timings_path = ROOT/'references/literature_times.json'
    timings = json.loads(timings_path.read_text(encoding='utf-8')) if timings_path.exists() else {}
    with (ROOT/'instances/literature_pdsvrp_c.csv').open(encoding='utf-8') as stream:
        rows = list(csv.DictReader(stream))
    for row in rows:
        row.update(timings.get(chave(row['instance'], row['trucks']), {}))
    return rows


def chave(name, trucks):
    """Identifica unicamente o par instancia e numero de caminhoes no registro."""
    return f'{name}_s{trucks}'


class Registro:
    """Melhores certificados por configuracao, validados antes de ler ou gravar."""
    def __init__(self, path=CENTRAL):
        """Carrega o registro e confere todos os certificados existentes no catalogo."""
        self.path = path
        self.lock = threading.Lock()
        self.data = json.loads(path.read_text(encoding='utf-8')) if path.exists() else {}
        for row in catalogo():
            key = chave(row['instance'], row['trucks'])
            entry = self.data.setdefault(key, dict(instance=row['instance'], trucks=int(row['trucks']), best=None))
            entry['literature_lb'] = float(row['lb'])
            entry['literature_ub'] = float(row['ub'])
            entry['proven'] = bool(int(row['proven']))
            entry['literature_seconds'] = row.get('seconds', '')
            entry['literature_time_kind'] = row.get('time_kind', '')
            entry['literature_time_source'] = row.get('time_source', '')
            local_lb, witness = limite_inferior_local(row['instance'])
            entry['local_lb'] = local_lb
            entry['local_lb_client'] = witness
            entry['literature_comparable'] = entry['literature_ub'] + 1e-7 >= local_lb
            if entry['best'] is not None:
                validar(entry['best'], row['instance'], int(row['trucks']))

    def salvar(self):
        """Substituicao atomica com retentativas para bloqueios Windows/OneDrive."""
        self.path.parent.mkdir(parents=True, exist_ok=True)
        # Um arquivo unico evita colisao com temporarios de execucoes anteriores.
        with tempfile.NamedTemporaryFile(mode='w', encoding='utf-8', delete=False,
                                         dir=self.path.parent, prefix=self.path.stem+'.',
                                         suffix='.tmp') as stream:
            temporary = Path(stream.name)
            json.dump(self.data, stream, indent=2, allow_nan=False)
            stream.write('\n')
            stream.flush()
            os.fsync(stream.fileno())
        for attempt in range(8):
            try:
                temporary.replace(self.path)
                return
            except PermissionError as error:
                if attempt == 7:
                    # Nao apaga nem trunca o registro anterior. A nova fotografia
                    # completa fica disponivel para recuperacao manual.
                    raise PermissionError(
                        f'Nao foi possivel substituir {self.path} apos 8 tentativas. '
                        f'O registro anterior foi preservado; copia recuperavel em {temporary}. '
                        'Verifique permissoes e bloqueios do Windows/OneDrive. '
                        'No WSL, prefira executar em um diretorio do Linux.') from error
                time.sleep(min(0.05 * 2**attempt, 0.8))

    def atualizar(self, name, trucks, cert, seed, imprimir=True, elapsed=None):
        """Valida antes de adquirir o lock e persiste somente uma melhora estrita."""
        value = validar(cert, name, trucks)
        with self.lock:
            entry = self.data[chave(name, trucks)]
            old = entry['best']
            if old is not None and value >= old['makespan']:
                return False
            # Copia independente do objeto recebido; persiste antes de anunciar.
            entry['best'] = dict(deepcopy(cert), seed=seed)
            try:
                self.salvar()
            except BaseException:
                # Falha no disco nao pode deixar um recorde apenas na memoria.
                entry['best'] = old
                raise
            if imprimir:
                mostrar(entry, elapsed)
            return True


def cabecalho():
    """Imprime resultados e tempos de computacao em segundos."""
    print(f"{'Instancia':<24} {'Melhor literatura':>16} {'Encontrado':>16} {'Gap (%)':>10} {'Execucao (s)':>14}", flush=True)


def mostrar(entry, elapsed=None):
    """Formata valor e gap; -- indica que nao existe certificado do algoritmo."""
    best, ub = entry['best'], entry['literature_ub']
    value = f"{best['makespan']:.6f}" if best else '--'
    gap = f"{100 * (best['makespan'] - ub) / ub:.3f}" if best and ub else '--'
    if not entry.get('literature_comparable', True):
        gap = 'N/C'
    name = f"{entry['instance']} (S={entry['trucks']})"
    last_run = (entry.get('runs') or [{}])[-1]
    if elapsed is None:
        elapsed = last_run.get('seconds')
    duration = f'{elapsed:.3f}' if elapsed is not None else '--'
    print(f"{name:<24} {ub:>16.6f} {value:>16} {gap:>10} {duration:>14}", flush=True)


def executar(row, args, registro, repetition):
    """Executa uma configuracao e valida checkpoints enquanto o worker esta ativo."""
    name, trucks = row['instance'], int(row['trucks'])
    size = 'small' if int(name.split('-')[0]) <= 30 else 'large'
    seed = (args.seed + int(name.split('-')[0])*10 + trucks + repetition*1000003) % 4294967296
    command = [str(localizar_executavel()), f'instances/benchmark/{size}/{name}.txt',
               f'instances/drone_times/tau_kj_{name}.txt', str(trucks), str(seed)]
    env = dict(os.environ, PDSVRP_WORKER='1')
    # Leitura continua: cada CHECKPOINT e validado e salvo durante o SA.
    cancel = getattr(args, 'cancel_event', threading.Event())
    if cancel.is_set(): return
    started = time.perf_counter()
    with subprocess.Popen(command, cwd=ROOT, env=env, stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT, text=True, encoding='utf-8', errors='replace') as process:
        finished = threading.Event()
        def vigiar_cancelamento():
            """Interrompe o worker quando outra tarefa falha ou o usuario cancela."""
            while not finished.wait(0.1):
                if cancel.is_set():
                    if process.poll() is None: process.kill()
                    return
        watcher = threading.Thread(target=vigiar_cancelamento, daemon=True)
        watcher.start()
        diagnostics = deque(maxlen=12)
        timed_out = threading.Event()
        def encerrar_por_prazo():
            """Aplica somente o limite opcional, preservando checkpoints ja gravados."""
            if process.poll() is None:
                timed_out.set()
                process.kill()
        timer = threading.Timer(args.seconds, encerrar_por_prazo) if args.seconds else None
        if timer: timer.start()
        failed = False
        try:
            for line in process.stdout:
                if line.startswith('CHECKPOINT '):
                    cert = json.loads(line[len('CHECKPOINT '):])
                    # Persiste melhorias durante a busca; a tabela so recebe
                    # uma linha quando a configuracao termina, no main.
                    registro.atualizar(name, trucks, cert, seed, imprimir=False)
                elif line.strip():
                    diagnostics.append(line.rstrip())
            result = process.wait()
            if result and not timed_out.is_set() and not cancel.is_set():
                details = '\n'.join(diagnostics) or 'O processo nao informou detalhes.'
                raise RuntimeError(f'{name} S={trucks}: processo encerrou com codigo {result}; '
                                   f'melhores preservados.\n{details}')
        except BaseException:
            failed = True
            process.kill()
            process.wait()
            raise
        finally:
            finished.set()
            watcher.join()
            if timer:
                timer.cancel()
                timer.join()
            elapsed = time.perf_counter()-started
            status = ('error' if failed else 'cancelled' if cancel.is_set() else 'timeout' if timed_out.is_set()
                      else 'completed' if process.returncode == 0 else 'error')
            # Nao repete a gravacao que acabou de falhar nem mascara a causa
            # original com uma segunda excecao no finally.
            with registro.lock:
                entry = registro.data[chave(name, trucks)]
                runs = entry.setdefault('runs', [])
                runs.append(dict(seed=seed, repetition=repetition+1, seconds=elapsed, status=status))
                try:
                    if not failed:
                        registro.salvar()
                except BaseException:
                    runs.pop()
                    raise


def main():
    """Mostra uma linha por configuracao encerrada; --list consulta o historico."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seconds', type=float, default=0)
    parser.add_argument('--workers', type=int, default=min(2, os.cpu_count() or 1))
    parser.add_argument('--limit', type=int, default=0)
    parser.add_argument('--seed', type=int, default=None,
                        help='Semente fixa opcional; por padrao usa o horario atual, como time(NULL).')
    parser.add_argument('--repetitions', type=int, default=1, help='Repeticoes (padrao: 1).')
    parser.add_argument('--list', action='store_true')
    parser.add_argument('--results', type=Path, default=CENTRAL,
                        help='Arquivo JSON de resultados; no WSL pode ficar fora do OneDrive.')
    parser.add_argument('individual', nargs='*', help='instancia.txt tau_kj.txt caminhoes [semente]')
    args = parser.parse_args()
    # Equivalente a time(NULL) em C: segundos desde a epoca Unix.
    # Uma base nova por execucao; configuracoes/repeticoes recebem os offsets abaixo.
    if args.seed is None:
        args.seed = int(time.time())
    if args.seconds < 0 or not math.isfinite(args.seconds) or args.workers < 1 or args.limit < 0:
        parser.error('Prazo/workers/limite invalidos')
    rows = catalogo()
    if args.individual:
        if len(args.individual) not in (3, 4): parser.error('Informe instancia, tau, caminhoes e semente opcional.')
        name = Path(args.individual[0]).stem
        try:
            trucks = int(args.individual[2])
            if len(args.individual) == 4: args.seed = int(args.individual[3])
        except ValueError:
            parser.error('Caminhoes e semente devem ser inteiros.')
        size = 'small' if int(name.split('-')[0]) <= 30 else 'large'
        if (Path(args.individual[0]).resolve() != (ROOT/f'instances/benchmark/{size}/{name}.txt').resolve() or
            Path(args.individual[1]).resolve() != (ROOT/f'instances/drone_times/tau_kj_{name}.txt').resolve()):
            parser.error('Use os arquivos oficiais do catalogo para o registro central.')
        rows = [r for r in rows if r['instance'] == name and int(r['trucks']) == trucks]
        if not rows: parser.error('Configuracao ausente do catalogo.')
        # A semente individual e exatamente a informada.
        args.seed -= int(name.split('-')[0])*10 + trucks
    if args.repetitions < 1:
        parser.error('Repeticoes deve ser positivo.')
    registro = Registro(args.results)
    cabecalho()
    if args.list:
        for row in rows: mostrar(registro.data[chave(row['instance'], row['trucks'])])
        return
    pending = [r for r in rows if not int(r['proven'])]
    if args.limit: pending = pending[:args.limit]
    localizar_executavel()
    registro.salvar()
    args.cancel_event = threading.Event()
    for repetition in range(args.repetitions):
        with concurrent.futures.ThreadPoolExecutor(max_workers=args.workers) as pool:
            futures = {pool.submit(executar, row, args, registro, repetition): row for row in pending}
            try:
                for future in concurrent.futures.as_completed(futures):
                    future.result()
                    # Unica linha da execucao, mesmo quando nao houve recorde.
                    # O mesmo lock usado pelos workers impede linhas intercaladas.
                    row = futures[future]
                    with registro.lock:
                        mostrar(registro.data[chave(row['instance'], row['trucks'])])
            except BaseException:
                args.cancel_event.set()
                for future in futures: future.cancel()
                raise


@contextmanager
def bloquear_execucao(path):
    """Impede dois executores de substituir simultaneamente o registro central."""
    path.parent.mkdir(parents=True, exist_ok=True)
    # Sem modo append: os metadados sempre ficam depois do byte bloqueado.
    descriptor = os.open(path, os.O_RDWR | os.O_CREAT, 0o666)
    with os.fdopen(descriptor, 'r+b') as lock:
        if os.fstat(lock.fileno()).st_size == 0:
            lock.write(b'0')
            lock.flush()
        lock.seek(0)
        try:
            if os.name == 'nt':
                import msvcrt
                msvcrt.locking(lock.fileno(), msvcrt.LK_NBLCK, 1)
            else:
                import fcntl
                fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except OSError as error:
            if error.errno not in (errno.EACCES, errno.EAGAIN, errno.EDEADLK):
                raise OSError(f'Nao foi possivel bloquear {path}: {error}') from error
            lock.seek(1)
            owner = lock.read().decode('utf-8', errors='replace').strip()
            detail = f' ({owner})' if owner else ''
            raise SystemExit('Ja existe uma execucao usando o registro central' + detail +
                             '. Aguarde terminar ou interrompa a execucao anterior com Ctrl+C. '
                             'Os resultados salvos estao preservados.') from None
        try:
            lock.seek(1)
            lock.write(f'PID {os.getpid()}'.encode('utf-8'))
            lock.truncate()
            lock.flush()
            yield
        finally:
            lock.seek(0)
            if os.name == 'nt':
                msvcrt.locking(lock.fileno(), msvcrt.LK_UNLCK, 1)
            else:
                fcntl.flock(lock, fcntl.LOCK_UN)


def entrada():
    """Adquire o bloqueio do registro antes de iniciar a execucao publica."""
    try:
        # O bloqueio acompanha o arquivo escolhido, inclusive fora do projeto.
        options = argparse.ArgumentParser(add_help=False)
        options.add_argument('--results', type=Path, default=CENTRAL)
        selected, _ = options.parse_known_args()
        with bloquear_execucao(selected.results.parent/'.execution.lock'):
            main()
    except KeyboardInterrupt:
        # main ja sinalizou cancelamento e aguardou o encerramento dos workers;
        # o context manager acima libera o registro antes de sair sem traceback.
        print('Execucao interrompida. Melhores resultados ja salvos preservados.', file=sys.stderr)
        raise SystemExit(130) from None
    except OSError as error:
        print(f'Erro de execucao: {error}', file=sys.stderr)
        raise SystemExit(1) from None


if __name__ == '__main__':
    entrada()
