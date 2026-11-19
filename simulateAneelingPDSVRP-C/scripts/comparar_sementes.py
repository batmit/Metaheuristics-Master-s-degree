"""Compara HEAD e fontes atuais com entradas e sementes iguais, sem tocar resultados."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time

from executar_benchmark import ROOT, validar


def main(cases=None, filename='experimentos.json'):
    out = ROOT/'output/comparacao_sementes'
    previous = out/'anterior'
    out.mkdir(exist_ok=True)
    git = ['git', '--git-dir='+str(ROOT.parent/'.git')]
    revision = subprocess.check_output([*git, 'rev-parse', 'HEAD'], text=True).strip()
    sources = ['main.c', 'lerInstancias.c', 'cv.c', 'drone_bb.c', 'SA.c']
    for name in [*sources, 'main.h', 'cv.h', 'SA.h', 'lerInstancias.h']:
        previous.mkdir(exist_ok=True)
        (previous/name).write_bytes(subprocess.check_output(
            [*git, 'show', f'{revision}:simulateAneelingPDSVRP-C/{name}']))
    suffix = '.exe' if os.name == 'nt' else ''
    binaries = {}
    for label, folder in [('anterior', previous), ('atual', ROOT)]:
        binary = out/('worker_'+label+suffix)
        subprocess.run(['gcc', '-std=c11', '-O2', *[str(folder/s) for s in sources],
                        '-o', str(binary), '-lm'], check=True)
        binaries[label] = binary
    saved = json.loads((ROOT/'results/best.json').read_text(encoding='utf-8'))
    records, inputs = [], []
    report = dict(revision=revision, platform=os.name, runs=records, inputs=inputs,
                  note='Sementes efetivas do worker; mesma plataforma, entradas e compilador. Sem corte externo salvo timeout de seguranca de 30s.')
    for instance, trucks in (cases or [('53-r-e', 5), ('68-rc-c', 5), ('91-r-c', 2)]):
        key = f'{instance}_s{trucks}'
        historical_seed = saved.get(key, {}).get('best', {}).get('seed')
        seeds = list(dict.fromkeys([42+int(instance.split('-')[0])*10+trucks,
                                   43+int(instance.split('-')[0])*10+trucks,
                                   44+int(instance.split('-')[0])*10+trucks,
                                   historical_seed or 42]))
        for relative in [f'instances/benchmark/large/{instance}.txt',
                         f'instances/drone_times/tau_kj_{instance}.txt']:
            old = subprocess.check_output([*git, 'show', f'{revision}:simulateAneelingPDSVRP-C/{relative}'])
            inputs.append(dict(path=relative, same_bytes=old == (ROOT/relative).read_bytes(),
                               current_sha256=hashlib.sha256((ROOT/relative).read_bytes()).hexdigest()))
        for seed in seeds:
            for label, binary in binaries.items():
                started = time.perf_counter()
                command = [str(binary), f'instances/benchmark/large/{instance}.txt',
                           f'instances/drone_times/tau_kj_{instance}.txt', str(trucks), str(seed)]
                try:
                    result = subprocess.run(command, cwd=ROOT, env=dict(os.environ, PDSVRP_WORKER='1'),
                                            capture_output=True, text=True, timeout=30)
                    output, status = result.stdout, f'exit_{result.returncode}'
                    if result.returncode:
                        raise RuntimeError(result.stderr)
                except subprocess.TimeoutExpired as error:
                    output, status = error.stdout or b'', 'timeout_30s'
                    if isinstance(output, bytes):
                        output = output.decode('utf-8')
                values = []
                for line in output.splitlines():
                    if line.startswith('CHECKPOINT '):
                        try:
                            certificate = json.loads(line[11:])
                        except json.JSONDecodeError:
                            if status == 'timeout_30s':
                                continue
                            raise
                        values.append(validar(certificate, instance, trucks))
                record = dict(instance=instance, trucks=trucks, version=label, seed=seed,
                              initial=values[0] if values else None,
                              best=min(values) if values else None, checkpoints=len(values),
                              status=status, seconds=time.perf_counter()-started)
                records.append(record)
                (out/filename).write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
                print(json.dumps(record), flush=True)
    # Repeticao exata: testa determinismo no mesmo ambiente.
    repeated = records[1]
    binary = binaries['atual']
    instance, trucks, seed = repeated['instance'], repeated['trucks'], repeated['seed']
    result = subprocess.run([str(binary), f'instances/benchmark/large/{instance}.txt',
                             f'instances/drone_times/tau_kj_{instance}.txt', str(trucks), str(seed)],
                            cwd=ROOT, env=dict(os.environ, PDSVRP_WORKER='1'),
                            capture_output=True, text=True, check=True, timeout=30)
    values = [validar(json.loads(line[11:]), instance, trucks)
              for line in result.stdout.splitlines() if line.startswith('CHECKPOINT ')]
    report['repeat_same_seed'] = dict(instance=instance, seed=seed,
                                    first=repeated['best'], repeated=min(values),
                                    same_best=repeated['best'] == min(values))
    (out/filename).write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')


if __name__ == '__main__':
    main()
