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
assert len(manifest['returncodes'])==30 and all(r['code']==0 for r in manifest['returncodes'])
for n,h in manifest['source_sha256'].items():
    assert hashlib.sha256((ROOT/n).read_bytes()).hexdigest()==h
    assert hashlib.sha256((HERE/'fontes'/n).read_bytes()).hexdigest()==h
for n,h in manifest['input_sha256'].items(): assert hashlib.sha256((ROOT/n).read_bytes()).hexdigest()==h
rows=[json.loads(line) for p in HERE.glob('*.jsonl') for line in p.read_text().splitlines()]
assert len(rows)==30
source=ast.parse((ROOT/'resultados/grupos_quatro_100/analisar.py').read_text())
validator=next(n for n in source.body if isinstance(n,ast.FunctionDef) and n.name=='verify_instance')
scope={'ROOT':ROOT,'F':F}
exec(compile(ast.Module(body=[validator],type_ignores=[]),'<independent-validator>','exec'),scope)
all_runs=[];summary=[]
for c,(name,ub) in enumerate(refs.items()):
    data=sorted((r for r in rows if r['instance']==name),key=lambda r:r['seed'])
    assert len(data)==10 and {r['seed'] for r in data}==set(range(manifest['seed']+c*10,manifest['seed']+(c+1)*10))
    assert all(r['mask']==15 and r['budget']==30 for r in data)
    scope['verify_instance'](name,data)
    for i,r in enumerate(data,1):
        all_runs.append({'instancia':name,'execucao':i,'semente':r['seed'],'makespan':r['claimed'],
                         'caminhao':r['truck'],'drones':r['drone'],'literatura':ub,
                         'gap_percentual':100*(r['claimed']-ub)/ub,'segundos':r['seconds']})
    best=min(data,key=lambda r:(r['claimed'],r['seed']));values=[r['claimed'] for r in data]
    item={'instance':name,'n':10,'best':min(values),'mean':st.mean(values),'median':st.median(values),
          'worst':max(values),'literature':ub,'gap_best_percent':100*(min(values)-ub)/ub,
          'gap_mean_percent':100*(st.mean(values)-ub)/ub,'equal_literature':sum(v==ub for v in values),
          'below_literature':sum(v<ub for v in values),'best_seed':best['seed'],
          'best_run':best['seed']-(manifest['seed']+c*10)+1,'previous_10s_best':previous[name],
          'mean_seconds':st.mean(r['seconds'] for r in data),'max_seconds':max(r['seconds'] for r in data),
          'results':values}
    summary.append(item)
    (HERE/f'melhor_{name}.json').write_text(json.dumps(best,indent=2),encoding='utf-8')
with (HERE/'execucoes.csv').open('w',newline='',encoding='utf-8') as out:
    w=csv.DictWriter(out,fieldnames=list(all_runs[0]));w.writeheader();w.writerows(all_runs)
with (HERE/'resumo.csv').open('w',newline='',encoding='utf-8') as out:
    columns=[k for k in summary[0] if k!='results'];w=csv.DictWriter(out,fieldnames=columns,extrasaction='ignore');w.writeheader();w.writerows(summary)
(HERE/'resumo.json').write_text(json.dumps({'validated':30,'summary':summary},indent=2),encoding='utf-8')
text='''# Dez execucoes das tres instancias maiores — 30 segundos

Configuracao atual, com trocas simultaneas, reconstrucao parcial, calibracao mista, resfriamento por tempo e elites diversas. Foram realizadas dez execucoes independentes em cada instancia, sempre a partir da construcao inicial. As sementes originadas do relogio, os fontes e os hashes dos dados estao preservados no manifesto e em fontes/.

Cada execucao recebeu 30 segundos de parede para calibracao e SA. A construcao inicial fica fora desse limite. Foram usados tres processos concorrentes, intercalando as instancias. Operacoes em andamento sao concluidas antes do encerramento, podendo exceder ligeiramente o prazo. Nao houve alteracao dos parametros ou fontes durante o lote.

Os 30 certificados passaram pela verificacao independente com fracoes exatas: cobertura unica, rota Manhattan e retorno ao deposito, tempos da matriz, capacidade simultanea e atribuicao fisica dos drones. O validador nao chama o avaliador C.

## Resultados individuais em minutos

| Execucao | 165-r-c | 167-r-e | 181-r-e |
|---|---:|---:|---:|
'''
for i in range(10):text+='| '+str(i+1)+' | '+' | '.join(f"{s['results'][i]:g}" for s in summary)+' |\n'
text+='''
## Comparacao com as referencias

| Instancia | Melhor | Media | Mediana | Pior | Melhor literatura | Gap do melhor |
|---|---:|---:|---:|---:|---:|---:|
'''
for r in summary:
    text+=f"| {r['instance']} | {r['best']:g} | {r['mean']:.2f} | {r['median']:g} | {r['worst']:g} | {r['literature']} | {r['gap_best_percent']:.2f}% |\n"
text+='\nGap = 100 × (resultado − referencia) / referencia. As referencias sao os limites superiores 160, 180 e 216 usados no projeto a partir do pacote oficial ORLab de 2025. Nao sao otimos comprovados nesses tres casos.\n\n'
for r in summary:
    text+=f"- {r['instance']}: iguais a referencia {r['equal_literature']}/10; abaixo {r['below_literature']}/10. Melhor na execucao {r['best_run']}, semente {r['best_seed']}. Melhor do lote anterior de 10 s: {r['previous_10s_best']}; melhor deste lote: {r['best']:g}.\n"
text+='''
A comparacao com o lote anterior de 10 segundos e descritiva: o tempo, as sementes e a concorrencia diferem; ela nao isola causalmente o efeito de aumentar o orcamento. Dez execucoes tambem nao demonstram que repetir mais vezes alcancara a literatura.

Arquivos: execucoes.csv contem as 30 execucoes e sementes; resumo.csv e resumo.json apresentam estatisticas; melhor_*.json guarda cada melhor solucao completa; *.jsonl contem todos os vetores e cronogramas. Nenhuma fonte de producao foi alterada para executar este lote.

Fonte das referencias: [ORLab — dados e resultados PDSTSP-c/PDSOP-c](https://orlab.com.vn/download/data-and-results-of-pdstsp-c-and-pdsop-c/).
'''
(HERE/'relatorio.md').write_text(text,encoding='utf-8')
print(json.dumps(summary,indent=2))
