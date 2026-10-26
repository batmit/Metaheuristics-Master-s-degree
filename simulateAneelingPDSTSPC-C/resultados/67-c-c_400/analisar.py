"""Valida as 400 solucoes independentemente do avaliador C, com aritmetica exata."""
from collections import Counter
import csv
from fractions import Fraction as F
import hashlib
import json
from pathlib import Path
import statistics
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
records = []
for path in sorted(HERE.glob('solucoes_*.jsonl')):
    for line in path.read_text().splitlines():
        try:
            records.append(json.loads(line))
        except json.JSONDecodeError:
            if '--parcial' not in sys.argv:
                raise
records.sort(key=lambda r: r['run'])
if '--parcial' in sys.argv:
    print(json.dumps({'completed':len(records), 'best':min((r['claimed'] for r in records), default=None),
                      'distribution':dict(sorted(Counter(r['claimed'] for r in records).items()))}))
    raise SystemExit()

manifest = json.loads((HERE/'manifesto.json').read_text())
assert manifest['returncodes'] == [0]*4
assert len(records) == 400 and [r['run'] for r in records] == list(range(1,401))
assert len({r['seed'] for r in records}) == 400
previous_manifest = json.loads((HERE.parent/'67-c-c_200'/'manifesto.json').read_text())
assert manifest['base_seed'] >= previous_manifest['base_seed'] + previous_manifest['runs']
assert all(r['seed'] == manifest['base_seed'] + r['run'] - 1 for r in records)
assert manifest['sources_unchanged']
assert all(hashlib.sha256((ROOT/p).read_bytes()).hexdigest() == h for p,h in manifest['hashes'].items())

lines = (ROOT/'instances/pdstsp-c/large/67-c-c.txt').read_text().splitlines()
params = {s.split(',')[0]:F(s.split(',')[1].strip()) for s in lines[:6]}
clients = [s.split() for s in lines[7:] if s.strip()]
assert [int(v[0]) for v in clients] == list(range(68))
xy = {int(v[0]):(F(v[1]),F(v[2])) for v in clients}
fleet = int(params['NUM_DRONES'])
tau = {}
for line in (ROOT/'instances/pdstsp-c/large/tau_kj_67-c-c.txt').read_text().splitlines()[1:]:
    values = line.split()
    if values:
        assert len(values) == fleet+1
        tau[int(values[0])] = {k:F(t) for k,t in enumerate(values[1:],1)}

def verify(r):
    route, missions = r['route'], r['missions']
    coverage = route + [m['client'] for m in missions]
    assert len(coverage) == 67 and set(coverage) == set(range(1,68))
    truck = sum((sum(abs(a-b) for a,b in zip(xy[i],xy[j])) * 60 / params['TRUCK_SPEED']
                 for i,j in zip([0]+route,route+[0])), F(0))
    exact = []
    for m in missions:
        a,b = F(str(m['start'])),F(str(m['end']))
        k = m['k']
        assert 1 <= k <= fleet and a >= 0 and b > a
        assert tau[m['client']][k] >= 0 and b-a == tau[m['client']][k]
        exact.append((a,b,k,m['client']))
    events = sorted({x for a,b,_,_ in exact for x in (a,b)})
    peak = max((sum(k for a,b,k,_ in exact if a <= t < b) for t in events), default=0)
    assert peak <= fleet
    # Testemunha com drones individuais e nenhuma interrupcao de missao.
    available = [F(0)]*fleet
    for a,b,k,_ in sorted(exact):
        ids = [i for i,t in enumerate(available) if t <= a][:k]
        assert len(ids) == k
        for i in ids:
            available[i] = b
    drone = max((b for _,b,_,_ in exact), default=F(0))
    assert truck == F(str(r['truck'])) and drone == F(str(r['drone']))
    assert max(truck,drone) == F(str(r['claimed']))
    return {'run':r['run'], 'valid':True, 'makespan':float(max(truck,drone)), 'peak_drones':peak}

validation = [verify(r) for r in records]
values = [r['claimed'] for r in records]
best = min(records, key=lambda r:(r['claimed'],r['run']))
summary = {'instance':'67-c-c','completed':len(records),'validated':len(validation),
           'base_seed':manifest['base_seed'], 'best':min(values),'mean':statistics.mean(values),
           'median':statistics.median(values),'worst':max(values),'std_population':statistics.pstdev(values),
           'best_run':best['run'],'best_seed':best['seed'], 'best_truck':best['truck'],'best_drone':best['drone'],
           'literature_UB':74,'literature_LB':73, 'gap_percent':100*(min(values)-74)/74,
           'below_reference':sum(v<74 for v in values),'equal_reference':sum(v==74 for v in values),
           'distribution':dict(sorted(Counter(values).items())),
           'started':manifest['started'],'finished':manifest['finished'],
           'source_unchanged':True}
for name,obj in [('resumo.json',summary),('melhor_solucao.json',best),('validacao.json',validation)]:
    (HERE/name).write_text(json.dumps(obj,ensure_ascii=False,indent=2),encoding='utf-8')
columns = ['run','seed','claimed','truck','drone','initial','temperature','seconds']
with (HERE/'execucoes_400.csv').open('w',newline='',encoding='utf-8') as f:
    writer = csv.DictWriter(f,fieldnames=columns,extrasaction='ignore')
    writer.writeheader()
    writer.writerows(records)
(HERE/'relatorio.md').write_text(
    '# Experimento: 67-c-c, 400 execucoes\n\n'
    f"Foram concluidas e validadas {len(records)} execucoes independentes com sementes distintas, "
    'derivadas de time(NULL). SA.c, cv.c e main.c permaneceram inalterados. '
    'Parametros padrao do codigo atual; quatro processos, 100 execucoes por processo.\n\n'
    f"- Melhor makespan: {summary['best']:g} min.\n"
    f"- Media: {summary['mean']:.3f} min; mediana: {summary['median']:g} min.\n"
    f"- Pior: {summary['worst']:g} min; desvio padrao populacional: {summary['std_population']:.3f} min.\n"
    f"- Melhor execucao: {best['run']}; semente: {best['seed']}.\n"
    f"- Melhor solucao: caminhao {best['truck']:g} min, drones {best['drone']:g} min.\n"
    f"- Referencia: UB=74 min, LB=73 min; gap do melhor: {summary['gap_percent']:.3f}%.\n"
    f"- Abaixo de 74: {summary['below_reference']}; iguais a 74: {summary['equal_reference']}.\n\n"
    'Referencia: https://orlab.com.vn/download/data-and-results-of-pdstsp-c-and-pdsop-c/\n\n'
    'Validacao independente com Fraction: cobertura unica dos 67 clientes, rota Manhattan e retorno '
    'ao deposito, duracoes tau por quantidade, capacidade simultanea e atribuicao de drones fisicos. '
    'As 400 solucoes completas ficam nos quatro arquivos solucoes_*.jsonl.\n\n'
    'O campo seconds inclui calibracao e SA por execucao; os processos executaram em paralelo, '
    'portanto os tempos nao sao comparaveis diretamente a um experimento sem concorrencia.\n',encoding='utf-8')
print(json.dumps(summary,ensure_ascii=False,indent=2))
