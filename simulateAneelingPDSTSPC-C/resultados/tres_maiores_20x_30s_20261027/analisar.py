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
previous={'165-r-c':164,'167-r-e':188,'181-r-e':220}
manifest=json.loads((HERE/'manifesto.json').read_text())
assert 'finished' in manifest and manifest['sources_unchanged']
assert len(manifest['returncodes'])==60 and all(r['code']==0 for r in manifest['returncodes'])
for n,h in manifest['source_sha256'].items():
    assert hashlib.sha256((ROOT/n).read_bytes()).hexdigest()==h
    assert hashlib.sha256((HERE/'fontes'/n).read_bytes()).hexdigest()==h
for n,h in manifest['input_sha256'].items(): assert hashlib.sha256((ROOT/n).read_bytes()).hexdigest()==h
rows=[json.loads(line) for p in HERE.glob('*.jsonl') for line in p.read_text().splitlines()]
assert len(rows)==60
source=ast.parse((ROOT/'resultados/grupos_quatro_100/analisar.py').read_text())
validator=next(n for n in source.body if isinstance(n,ast.FunctionDef) and n.name=='verify_instance')
scope={'ROOT':ROOT,'F':F}
exec(compile(ast.Module(body=[validator],type_ignores=[]),'<independent-validator>','exec'),scope)
all_runs=[];summary=[]
for c,(name,ub) in enumerate(refs.items()):
    data=sorted((r for r in rows if r['instance']==name),key=lambda r:r['seed'])
    assert len(data)==20 and {r['seed'] for r in data}==set(range(manifest['seed']+c*20,manifest['seed']+(c+1)*20))
    assert all(r['mask']==15 and r['budget']==30 for r in data)
    scope['verify_instance'](name,data)
    for i,r in enumerate(data,1):
        all_runs.append({'instancia':name,'execucao':i,'semente':r['seed'],'makespan':r['claimed'],
                         'caminhao':r['truck'],'drones':r['drone'],'literatura':ub,
                         'gap_percentual':100*(r['claimed']-ub)/ub,'segundos':r['seconds']})
    best=min(data,key=lambda r:(r['claimed'],r['seed']));values=[r['claimed'] for r in data]
    item={'instance':name,'n':20,'best':min(values),'mean':st.mean(values),'median':st.median(values),
          'worst':max(values),'literature':ub,'gap_best_percent':100*(min(values)-ub)/ub,
          'gap_mean_percent':100*(st.mean(values)-ub)/ub,'equal_literature':sum(v==ub for v in values),
          'below_literature':sum(v<ub for v in values),'best_seed':best['seed'],
          'best_run':best['seed']-(manifest['seed']+c*20)+1,'previous_10s_best':previous[name],
          'mean_seconds':st.mean(r['seconds'] for r in data),'max_seconds':max(r['seconds'] for r in data),
          'results':values}
    summary.append(item)
    (HERE/f'melhor_{name}.json').write_text(json.dumps(best,indent=2),encoding='utf-8')
with (HERE/'execucoes.csv').open('w',newline='',encoding='utf-8') as out:
    w=csv.DictWriter(out,fieldnames=list(all_runs[0]));w.writeheader();w.writerows(all_runs)
with (HERE/'resumo.csv').open('w',newline='',encoding='utf-8') as out:
    columns=[k for k in summary[0] if k!='results'];w=csv.DictWriter(out,fieldnames=columns,extrasaction='ignore');w.writeheader();w.writerows(summary)
(HERE/'resumo.json').write_text(json.dumps({'validated':60,'summary':summary},indent=2),encoding='utf-8')
report = '# Vinte execuções por instância — 30 segundos\n\n'
report += '60 soluções verificadas independentemente com frações exatas. Referências locais: melhores resultados conhecidos, sem ótimo comprovado. Três processos concorrentes, sementes independentes e configuração de exploração atual.\n\n'
report += '| Instância | Execuções | Melhor | Média | Pior | Referência | Empates | Melhorias |\n|---|---:|---:|---:|---:|---:|---:|---:|\n'
for r in summary:
    report += f"| {r['instance']} | {r['n']} | {r['best']:g} | {r['mean']:.2f} | {r['worst']:g} | {r['literature']} | {r['equal_literature']} | {r['below_literature']} |\n"
report += '\nCada execução começa com nova solução inicial. O limite de 30 segundos inclui calibração e SA; a construção inicial fica fora. Fontes, hashes, sementes, rotas e cronogramas preservados neste diretório.\n'
report += '\nObservação de tempo: apesar do limite configurado de 30 s, a execução 17 das três instâncias registrou cerca de 739 s decorridos. A causa não foi determinada; este lote não deve ser tratado como comparação rigorosa de desempenho por tempo. Os resultados dessas execuções foram mantidos e validados.\n'
(HERE/'relatorio.md').write_text(report,encoding='utf-8')
print(json.dumps(summary,indent=2))


