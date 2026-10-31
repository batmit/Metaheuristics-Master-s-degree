from pathlib import Path
from fractions import Fraction as F
import ast
import csv
import datetime
import hashlib
import json
import statistics as st

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
refs={'165-r-c':160,'167-r-e':180,'181-r-e':216}
previous={'165-r-c':160,'167-r-e':180,'181-r-e':220}
manifest=json.loads((HERE/'manifesto.json').read_text())
count=manifest['count']; total=count*len(refs)
assert 'finished' in manifest and manifest['sources_unchanged']
assert len(manifest['returncodes'])==total and all(r['code']==0 for r in manifest['returncodes'])
for n,h in manifest['source_sha256'].items():
    assert hashlib.sha256((ROOT/n).read_bytes()).hexdigest()==h
    assert hashlib.sha256((HERE/'fontes'/n).read_bytes()).hexdigest()==h
for n,h in manifest['input_sha256'].items(): assert hashlib.sha256((ROOT/n).read_bytes()).hexdigest()==h
rows=[json.loads(line) for p in HERE.glob('*.jsonl') for line in p.read_text().splitlines()]
assert len(rows)==total
source=ast.parse((ROOT/'resultados/grupos_quatro_100/analisar.py').read_text())
validator=next(n for n in source.body if isinstance(n,ast.FunctionDef) and n.name=='verify_instance')
scope={'ROOT':ROOT,'F':F}
exec(compile(ast.Module(body=[validator],type_ignores=[]),'<independent-validator>','exec'),scope)
all_runs=[];summary=[]
for c,(name,ub) in enumerate(refs.items()):
    data=sorted((r for r in rows if r['instance']==name),key=lambda r:r['seed'])
    assert len(data)==count and {r['seed'] for r in data}==set(range(manifest['seed']+c*count,manifest['seed']+(c+1)*count))
    assert all(r['mask']==15 and r['budget']==30 for r in data)
    scope['verify_instance'](name,data)
    for i,r in enumerate(data,1):
        all_runs.append({'instancia':name,'execucao':i,'semente':r['seed'],'makespan':r['claimed'],
                         'caminhao':r['truck'],'drones':r['drone'],'literatura':ub,
                         'gap_percentual':100*(r['claimed']-ub)/ub,'segundos':r['seconds']})
    best=min(data,key=lambda r:(r['claimed'],r['seed']));values=[r['claimed'] for r in data]
    item={'instance':name,'n':count,'best':min(values),'mean':st.mean(values),'median':st.median(values),
          'worst':max(values),'literature':ub,'gap_best_percent':100*(min(values)-ub)/ub,
          'gap_mean_percent':100*(st.mean(values)-ub)/ub,'equal_literature':sum(v==ub for v in values),
          'below_literature':sum(v<ub for v in values),'best_seed':best['seed'],
          'best_run':best['seed']-(manifest['seed']+c*count)+1,'previous_50x_best':previous[name],
          'mean_seconds':st.mean(r['seconds'] for r in data),'max_seconds':max(r['seconds'] for r in data),
          'results':values}
    summary.append(item)
    (HERE/f'melhor_{name}.json').write_text(json.dumps(best,indent=2),encoding='utf-8')
with (HERE/'execucoes.csv').open('w',newline='',encoding='utf-8') as out:
    w=csv.DictWriter(out,fieldnames=list(all_runs[0]));w.writeheader();w.writerows(all_runs)
with (HERE/'resumo.csv').open('w',newline='',encoding='utf-8') as out:
    columns=[k for k in summary[0] if k!='results'];w=csv.DictWriter(out,fieldnames=columns,extrasaction='ignore');w.writeheader();w.writerows(summary)
(HERE/'resumo.json').write_text(json.dumps({'validated':total,'summary':summary},indent=2),encoding='utf-8')
report = '# Cem execuções por instância — 30 segundos\n\n'
report += '300 soluções verificadas independentemente com frações exatas. Referências locais: melhores resultados conhecidos, sem ótimo comprovado. Três processos concorrentes, sementes independentes e configuração de exploração atual.\n\n'
report += '| Instância | Execuções | Melhor | Média | Pior | Referência | Empates | Melhorias |\n|---|---:|---:|---:|---:|---:|---:|---:|\n'
for r in summary:
    report += f"| {r['instance']} | {r['n']} | {r['best']:g} | {r['mean']:.2f} | {r['worst']:g} | {r['literature']} | {r['equal_literature']} | {r['below_literature']} |\n"
report += '\nCada execução começa com nova solução inicial. O limite de 30 segundos inclui calibração e SA; a construção inicial fica fora. Fontes, hashes, sementes, rotas e cronogramas preservados neste diretório.\n'
slow=[{'instance':r['instance'],'seed':r['seed'],'seconds':r['seconds']} for r in rows if r['seconds']>35]
report += f'\nExecuções com tempo decorrido acima de 35 s: {len(slow)}. Maior tempo registrado: {max(r["seconds"] for r in rows):.3f} s.\n'
(HERE/'anomalias_tempo.json').write_text(json.dumps(slow,indent=2),encoding='utf-8')
report += '\n## Comparação com o lote anterior de 50 execuções\n\n| Instância | Melhor anterior | Melhor atual |\n|---|---:|---:|\n'
for r in summary:
    report += f"| {r['instance']} | {r['previous_50x_best']:g} | {r['best']:g} |\n"
report += '\n## Melhores resultados acumulados a cada dez repetições\n\n| Repetições por instância | 165-r-c | 167-r-e | 181-r-e |\n|---|---:|---:|---:|\n'
for milestone in range(10,count+1,10):
    snap=json.loads((HERE/f'parcial_{milestone:03d}.json').read_text())
    assert snap['validated']==milestone*len(refs)
    for entry in snap['summary']:
        c=list(refs).index(entry['instance'])
        subset=[r['claimed'] for r in rows if r['instance']==entry['instance'] and r['seed']<manifest['seed']+c*count+milestone]
        assert len(subset)==milestone and min(subset)==entry['best']
    report += f"| {milestone} | " + ' | '.join(f"{entry['best']:g}" for entry in snap['summary']) + ' |\n'
report += '\nAs 300 soluções passaram pelo validador independente com frações exatas, verificando cobertura única, rota e retorno ao depósito, tempos das missões, capacidade simultânea e atribuição física dos drones.\n'
report += f"\nResultados abaixo das referências: {sum(r['below_literature'] for r in summary)}.\n"
if slow:
    report += f'\n{len(slow)} execuções excederam 35 segundos decorridos. A causa não foi determinada. Os resultados foram mantidos e validados; os tempos efetivos estão em execucoes.csv e anomalias_tempo.json.\n'
(HERE/'relatorio.md').write_text(report,encoding='utf-8')
print(json.dumps(summary,indent=2))
