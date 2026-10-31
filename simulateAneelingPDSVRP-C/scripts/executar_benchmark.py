"""Lote PDSVRP-C: imprime todos; executa somente os sem otimo comprovado."""
import argparse
import concurrent.futures
import csv
import json
import math
import os
from pathlib import Path
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]


def localizar_executavel():
    names = ('pdsvrp_sa.exe', 'pdsvrp_sa') if os.name == 'nt' else ('pdsvrp_sa', 'pdsvrp_sa.exe')
    for name in names:
        path = ROOT/name
        if path.is_file() and (os.name == 'nt' or os.access(path, os.X_OK)):
            return path
    raise FileNotFoundError('Compile o programa na raiz do projeto como pdsvrp_sa ou pdsvrp_sa.exe.')


def validar(certificado, instance, trucks):
    """Valida certificado em implementacao independente do codigo C."""
    size = 'small' if int(instance.split('-')[0]) <= 30 else 'large'
    lines = (ROOT/f'instances/benchmark/{size}/{instance}.txt').read_text().splitlines()
    speed = float(lines[1].split(',')[1])
    fleet = int(lines[0].split(',')[1])
    nodes = [[float(v) for v in line.split()] for line in lines[7:] if line.strip()]
    tau = [[int(v) for v in line.split()] for line in
           (ROOT/f'instances/drone_times/tau_kj_{instance}.txt').read_text().splitlines()[1:] if line.strip()]
    times = {r[0]: r[1:] for r in tau}
    routes = certificado['routes']
    assert len(routes) == trucks
    seen = []
    value = 0.0
    for route in routes:
        seen.extend(route)
        path = [0, *route, 0]
        value = max(value, sum((abs(nodes[a][1]-nodes[b][1]) +
                                abs(nodes[a][2]-nodes[b][2])) / speed * 60
                               for a, b in zip(path, path[1:])))
    missions = certificado['missions']
    for m in missions:
        j, k, start, end = m['client'], m['k'], m['start'], m['end']
        assert 1 <= j < len(nodes) and 1 <= k <= fleet
        assert math.isfinite(start) and math.isfinite(end) and 0 <= start <= end
        assert times[j][k-1] >= 0 and math.isclose(end-start, times[j][k-1], abs_tol=1e-7)
        seen.append(j)
        value = max(value, end)
        assert sum(x['k'] for x in missions if x['start'] <= start < x['end']) <= fleet
    assert sorted(seen) == list(range(1, len(nodes)))
    assert math.isclose(value, certificado['makespan'], rel_tol=1e-9, abs_tol=1e-7)
    return value


def executar(row, args, directory):
    name, trucks = row['instance'], int(row['trucks'])
    key = f'{name}_s{trucks}'
    size = 'small' if int(name.split('-')[0]) <= 30 else 'large'
    seed = args.seed + int(name.split('-')[0])*10 + trucks
    command = [str(localizar_executavel()), f'instances/benchmark/{size}/{name}.txt',
               f'instances/drone_times/tau_kj_{name}.txt', str(trucks), str(seed)]
    started = time.monotonic()
    status = 'concluido'
    logfile = directory/'logs'/f'{key}.log'
    with logfile.open('w', encoding='utf-8') as stream:
        try:
            result = subprocess.run(command, cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT,
                                    timeout=args.seconds if args.seconds > 0 else None)
            if result.returncode: status = 'erro'
        except subprocess.TimeoutExpired:
            status = 'limite_30s' if args.seconds == 30 else 'limite_tempo'
    elapsed = time.monotonic() - started
    best = None
    for line in logfile.read_text(encoding='utf-8', errors='replace').splitlines():
        if line.startswith('CHECKPOINT '):
            try:
                cert = json.loads(line[len('CHECKPOINT '):])
                value = validar(cert, name, trucks)
                if best is None or value < best['makespan']: best = cert
            except (ValueError, KeyError, AssertionError, IndexError, TypeError):
                continue
    row = dict(row, found='', gap_percent='', status=status, seconds=f'{elapsed:.3f}', seed=seed)
    if best is not None:
        value = best['makespan']
        row['found'] = f'{value:.6f}'
        row['gap_percent'] = f'{100*(value-float(row["ub"]))/float(row["ub"]):.3f}'
        if value < float(row['lb'])-1e-7: row['status'] += '_abaixo_LB_verificar_dados'
        best.update(instance=name, trucks=trucks, seed=seed, literature_lb=row['lb'],
                    literature_ub=row['ub'], run_status=row['status'])
        (directory/'solutions'/f'{key}.json').write_text(json.dumps(best, indent=2)+'\n', encoding='utf-8')
    else: row['status'] += '_sem_certificado'
    return row


def imprimir(row):
    print(f"{row['instance']:<12} {row['trucks']:>2} {row['lb']:>5} {row['ub']:>5} "
          f"{row['found'] or '--':>12} {row['gap_percent'] or '--':>9} {row['status']}", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seconds', type=float, default=0, help='0: sem limite, encerra pelo resfriamento.')
    parser.add_argument('--workers', type=int, default=min(2, os.cpu_count() or 1))
    parser.add_argument('--limit', type=int, default=0, help='0: todas as configuracoes abertas; positivo: limita na ordem do catalogo.')
    parser.add_argument('--seed', type=int, default=42)
    parser.add_argument('--list', action='store_true', help='Somente imprime o catalogo.')
    parser.add_argument('--resume', action='store_true', help='Retoma resultados e certificados ja validados.')
    parser.add_argument('--output', type=Path, default=ROOT/'results/bb5_all_unproven')
    args = parser.parse_args()
    if args.seconds < 0 or not math.isfinite(args.seconds) or args.workers < 1 or args.limit < 0: parser.error('Prazo/workers/limite invalidos')
    with (ROOT/'instances/literature_pdsvrp_c.csv').open(encoding='utf-8') as stream:
        rows = list(csv.DictReader(stream))
    assert len(rows) == 180 and len({(r['instance'], r['trucks']) for r in rows}) == 180
    assert all(int(r['proven']) == (float(r['lb']) == float(r['ub'])) for r in rows)
    if args.limit == 0:
        args.limit = sum(not int(row['proven']) for row in rows)
    print('Instancia     S    LB   BKS     Encontrado     Gap% Status', flush=True)
    pending = []
    completed = {}
    resumed = 0
    selecionadas = 0
    directory = args.output.resolve()
    previous = {}
    if args.resume and not args.list and (directory/'comparison.csv').exists():
        with (directory/'comparison.csv').open(encoding='utf-8') as stream:
            previous = {(r['instance'], r['trucks']): r for r in csv.DictReader(stream)}
    for row in rows:
        key = row['instance'], row['trucks']
        if int(row['proven']):
            r = dict(row, found='', gap_percent='', status='ignorada_otimo_comprovado', seconds='0', seed='')
            completed[key] = r
            imprimir(r)
        elif args.list:
            selecionadas += 1
            imprimir(dict(row, found='', gap_percent='', status='pendente_sem_otimo' if selecionadas <= args.limit else 'fora_do_lote'))
        else:
            selecionadas += 1
            if selecionadas > args.limit:
                completed[key] = dict(row, found='', gap_percent='', status='fora_do_lote', seconds='0', seed='')
                continue
            old = previous.get(key)
            certificate = directory/'solutions'/f'{row["instance"]}_s{row["trucks"]}.json'
            if old and old['found'] and certificate.exists():
                cert = json.loads(certificate.read_text(encoding='utf-8'))
                value = validar(cert, row['instance'], int(row['trucks']))
                expected_seed = args.seed + int(row['instance'].split('-')[0])*10 + int(row['trucks'])
                assert all(old[k] == row[k] for k in row), 'Referencia mudou; use outro diretorio de saida.'
                assert int(old['seed']) == expected_seed and math.isclose(value, float(old['found']), abs_tol=1e-6)
                completed[key] = old
                resumed += 1
                imprimir(old)
            else:
                pending.append(row)
    if args.list: return
    for sub in ('logs', 'solutions'): (directory/sub).mkdir(parents=True, exist_ok=True)
    fieldnames = list(next(iter(completed.values())))
    def salvar():
        temporary = directory/'comparison.tmp'
        with temporary.open('w', newline='', encoding='utf-8') as stream:
            writer = csv.DictWriter(stream, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(completed[key] for key in [(r['instance'], r['trucks']) for r in rows] if key in completed)
        temporary.replace(directory/'comparison.csv')
    salvar()
    prazo = f'{args.seconds:g}s/configuracao' if args.seconds else 'sem limite de tempo; encerramento por temperatura'
    print(f'Executando {len(pending)} configuracoes abertas; {prazo}; '
          f'{args.workers} processos simultaneos (tempo de parede).', flush=True)
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.workers) as pool:
        futures = [pool.submit(executar, row, args, directory) for row in pending]
        for future in concurrent.futures.as_completed(futures):
            row = future.result()
            completed[row['instance'], row['trucks']] = row
            imprimir(row)
            salvar()
    print('\nTabela completa (SA nao executado nos otimos comprovados):', flush=True)
    for row in rows: imprimir(completed[row['instance'], row['trucks']])
    metadata = dict(seconds=args.seconds, workers=args.workers, seed=args.seed,
                    executed=len(pending)+resumed, resumed=resumed,
                    executed_this_invocation=len(pending), skipped=sum(int(r['proven']) for r in rows),
                    limit=args.limit, not_selected=max(0, selecionadas-args.limit),
                    bb_node_limit=100000, bb_stagnation_iterations=5, bb_target='melhor_solucao_global',
                    algorithm='BB apos 5 tentativas sem melhora global; construcao por insercao + 2-opt; 5 movimentos',
                    note='seconds=0: sem corte externo; encerramento por resfriamento. Certificados validados independentemente.')
    (directory/'run.json').write_text(json.dumps(metadata, indent=2)+'\n', encoding='utf-8')
    print(f'Relatorio: {directory / "comparison.csv"}', flush=True)


if __name__ == '__main__':
    main()
