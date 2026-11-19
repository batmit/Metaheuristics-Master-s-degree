"""Confere dados, vizinhos e certificados sem analisar a 126-rc-e ou gravar em results."""
from collections import Counter
from io import BytesIO
import json
import math
import os
from pathlib import Path
import subprocess
from zipfile import ZipFile

from executar_benchmark import ROOT, dados_instancia, validar

OUT = ROOT/'output/auditoria_59_20261111'
EXCLUDED = '126-rc-e'


def run(command, **kwargs):
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, **kwargs)
    if result.returncode:
        raise RuntimeError(f'{command}: {result.stdout}\n{result.stderr}')
    return result.stdout


def reconstruct(block, instance):
    speed, _, nodes, tau = dados_instancia(instance)
    lines = block.strip().splitlines()
    declared = float(lines[1])
    truck = list(map(int, lines[2].split()[3:]))
    chains = [list(map(int, line.split()[3:])) for line in lines[3:] if line.strip()]
    counts = Counter(j for chain in chains for j in chain)
    parents = {j: set() for j in counts}
    for chain in chains:
        for a, b in zip(chain, chain[1:]):
            parents[b].add(a)
    ends, missions = {}, []
    while len(ends) < len(counts):
        ready = [j for j in counts if j not in ends and parents[j] <= ends.keys()]
        if not ready:
            raise ValueError('Ciclo no cronograma original')
        for j in ready:
            k = counts[j]
            duration = tau[j-1][k-1]
            if duration <= 0:
                raise ValueError(f'Celula inviavel: cliente {j}, k={k}')
            start = max((ends[p] for p in parents[j]), default=0)
            ends[j] = start+duration
            missions.append(dict(client=j, k=k, start=start, end=ends[j]))
    truck_time = sum((abs(nodes[a][1]-nodes[b][1])+abs(nodes[a][2]-nodes[b][2]))/speed*60
                     for a, b in zip(truck, truck[1:]))
    certificate = dict(routes=[[j for j in truck if j]], missions=missions,
                       makespan=max(truck_time, max(ends.values(), default=0)))
    value = validar(certificate, instance, 1)
    return dict(instance=instance, declared=declared, reconstructed=value,
                matches=math.isclose(value, declared), missions=len(missions))


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    suffix = '.exe' if os.name == 'nt' else ''
    common = ['gcc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2',
              'lerInstancias.c', 'cv.c', 'drone_bb.c', 'SA.c']
    binaries = {}
    for name in ('verificar_leitura', 'verificar_vizinhos', 'verificar_bb',
                 'verificar_bb5', 'verificar_sa', 'auditar_certificados'):
        binary = OUT/(name+suffix)
        run([*common, f'scripts/{name}.c', '-o', str(binary), '-lm'])
        binaries[name] = str(binary)
    print('Compilacao estrita: OK', flush=True)
    for name in ('verificar_bb', 'verificar_bb5'):
        run([binaries[name]])
    paths = sorted(p for p in (ROOT/'instances/benchmark').glob('*/*.txt') if p.stem != EXCLUDED)
    with ZipFile(ROOT/'instances/archives/pdstsp-c-pdsop-c-2025.zip') as outer:
        official_bytes = outer.read('PDSOP-c/data.zip')
    configurations = fractional = 0
    with ZipFile(BytesIO(official_bytes)) as official:
        for path in paths:
            tau = ROOT/f'instances/drone_times/tau_kj_{path.name}'
            if path.read_bytes() != official.read('data/instances/tsp/'+path.name):
                raise ValueError(f'Instancia diferente do ZIP: {path.stem}')
            if tau.read_bytes() != official.read('data/instances/tau_jk/'+tau.name):
                raise ValueError(f'Matriz diferente do ZIP: {path.stem}')
            speed, _, nodes, _ = dados_instancia(path.stem)
            fractional += sum(abs(t-round(t)) > 1e-9
                for a in nodes for b in nodes
                for t in [(abs(a[1]-b[1])+abs(a[2]-b[2]))/speed*60])
            run([binaries['verificar_leitura'], str(path), str(tau)])
            configurations += int(run([binaries['verificar_vizinhos'], str(path), str(tau)]))
    print(f'59 entradas/matrizes coincidem com o ZIP; {configurations*500} vizinhos validados.', flush=True)
    for name, size in [('15-rc-c', 'small'), ('50-r-e', 'large'), ('200-r-e', 'large')]:
        run([binaries['verificar_sa'], str(ROOT/f'instances/benchmark/{size}/{name}.txt'),
             str(ROOT/f'instances/drone_times/tau_kj_{name}.txt')])
    frozen = json.loads((ROOT/'results/best.json').read_text(encoding='utf-8'))
    (OUT/'certificados_snapshot.json').write_text(json.dumps(
        {key: e for key, e in frozen.items() if e['instance'] != EXCLUDED}, indent=2)+'\n')
    certificates = []
    for entry in frozen.values():
        name, trucks, cert = entry['instance'], entry['trucks'], entry.get('best')
        if name == EXCLUDED or not cert:
            continue
        value = validar(cert, name, trucks)
        size = 'small' if int(name.split('-')[0]) <= 30 else 'large'
        tokens = []
        for route in cert['routes']:
            tokens.extend([str(len(route)), *map(str, route)])
        tokens.append(str(len(cert['missions'])))
        for m in cert['missions']:
            tokens.extend(str(m[k]) for k in ('client', 'k', 'start', 'end'))
        tokens.append(str(value))
        c_value = float(run([binaries['auditar_certificados'],
                            f'instances/benchmark/{size}/{name}.txt',
                            f'instances/drone_times/tau_kj_{name}.txt', str(trucks)], input=' '.join(tokens)))
        if not math.isclose(c_value, value):
            raise ValueError('Certificado diverge entre C e Python')
        certificates.append(dict(instance=name, trucks=trucks, value=value, missions=len(cert['missions'])))
    original = []
    failures = []
    for block in (ROOT/'instances/original/Upload-SuperDrone/bks.txt').read_text().split('---'):
        if not block.strip():
            continue
        instance = block.strip().splitlines()[0].removesuffix('.txt')
        if instance == EXCLUDED:
            continue
        try:
            original.append(reconstruct(block, instance))
        except (ValueError, IndexError) as error:
            failures.append(dict(instance=instance, error=str(error)))
    summary = dict(excluded=EXCLUDED, instances=len(paths), configurations=configurations,
                   neighbors=configurations*500, original_zip_match=True, fractional_truck_edges=fractional,
                   saved_certificates_validated_in_C_and_Python=len(certificates), certificates=certificates,
                   original_bks_reconstructed=len(original), original_bks_matches=sum(r['matches'] for r in original),
                   original_bks=original, original_bks_failures=failures,
                   SA_trajectories=['15-rc-c', '50-r-e', '200-r-e'], bb_oracle=True)
    (OUT/'conformidade.json').write_text(json.dumps(summary, indent=2)+'\n')
    print(json.dumps({key: value for key, value in summary.items()
                      if key not in ('certificates', 'original_bks')}, indent=2), flush=True)


if __name__ == '__main__':
    main()
