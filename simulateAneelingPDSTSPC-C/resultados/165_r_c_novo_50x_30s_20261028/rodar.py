from pathlib import Path
from fractions import Fraction as F
import ast
import concurrent.futures as cf
import csv
import datetime
import hashlib
import json
import statistics
import subprocess
import time

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
assert not (HERE/'manifesto.json').exists(), 'Preservar o lote anterior.'
backend=subprocess.run([str(ROOT/'.venv-cpsat/Scripts/python.exe'),str(ROOT/'reparo_cpsat.py'),'--check'],cwd=ROOT,capture_output=True,text=True,check=True)
count=50;budget=30;workers=3;seed=int(time.time())
sources=['SA.c','SA.h','cv.c','cv.h','lerInstancias.c','lerInstancias.h','main.c','main.h','reparo_alvo.inc','reparo_cpsat.py','requirements-cpsat.txt','requirements-cpsat-lock.txt']
hashes={n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in sources}
inputs={n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in ['instances/pdstsp-c/large/165-r-c.txt','instances/pdstsp-c/large/tau_kj_165-r-c.txt']}
(HERE/'fontes').mkdir(exist_ok=True)
for n in sources:(HERE/'fontes'/n).write_bytes((ROOT/n).read_bytes())
subprocess.run(['gcc','-O2','-std=c11','-Wall','-Wextra','-Werror','-pedantic','-I',str(ROOT),str(HERE/'benchmark.c'),'SA.c','cv.c','lerInstancias.c','-lm','-o',str(HERE/'benchmark.exe')],cwd=ROOT,check=True)
manifest={'started':datetime.datetime.now().astimezone().isoformat(),'instance':'165-r-c','profile':'parametrosSAFoco165','count':count,'budget':budget,'workers':workers,'seed':seed,'warm_start':False,'cp_preflight':backend.stdout.strip(),'source_sha256':hashes,'input_sha256':inputs,'executable_sha256':hashlib.sha256((HERE/'benchmark.exe').read_bytes()).hexdigest(),'runs':[]}
(HERE/'manifesto.json').write_text(json.dumps(manifest,indent=2))
tree=ast.parse((ROOT/'resultados/grupos_quatro_100/analisar.py').read_text())
fn=next(n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='verify_instance')
scope={'ROOT':ROOT,'F':F}
exec(compile(ast.Module(body=[fn],type_ignores=[]),'validator','exec'),scope)
rows=[]

def checkpoint(final=False):
    scope['verify_instance']('165-r-c',rows)
    values=[r['claimed'] for r in rows]
    summary={'validated':len(rows),'best':min(values),'mean':statistics.mean(values),'median':statistics.median(values),'worst':max(values),'reference':160,'equal_reference':values.count(160),'below_reference':sum(v<160 for v in values),
             'cp_calls':sum(r['repair']['cp_calls'] for r in rows),'cp_success':sum(r['repair']['cp_success'] for r in rows),'cp_infeasible':sum(r['repair']['cp_infeasible'] for r in rows),'cp_timeout':sum(r['repair']['cp_timeout'] for r in rows),'cp_errors':sum(r['repair']['cp_errors'] for r in rows),'runs_above_35s':sum(r['seconds']>35 for r in rows),'max_seconds':max(r['seconds'] for r in rows),'results':values}
    (HERE/f'parcial_{len(rows):02d}.json').write_text(json.dumps(summary,indent=2))
    (HERE/'resumo.json').write_text(json.dumps(summary,indent=2))
    best=min(rows,key=lambda r:(r['claimed'],r['seed']))
    (HERE/'melhor_solucao.json').write_text(json.dumps(best,indent=2))
    with (HERE/'execucoes.csv').open('w',newline='',encoding='utf-8') as f:
        writer=csv.DictWriter(f,fieldnames=['run','seed','initial','claimed','truck','drone','seconds','evaluations','cp_calls','cp_success','cp_timeout','cp_errors'])
        writer.writeheader()
        for i,r in enumerate(rows,1):writer.writerow(dict(run=i,**{k:r[k] for k in ['seed','initial','claimed','truck','drone','seconds','evaluations']},**{k:r['repair'][k] for k in ['cp_calls','cp_success','cp_timeout','cp_errors']}))
    print('PARCIAL_VALIDADA '+json.dumps({k:v for k,v in summary.items() if k!='results'}),flush=True)
    if final:
        text='# 165-r-c — algoritmo novo, 50 execuções de 30 segundos\n\n'
        text+='Perfil parametrosSAFoco165, com reparo dirigido, cadeias de transferências, filtros e CP-SAT. Todas as execuções partiram de nova construção inicial, sem usar a solução conhecida de 160 como partida. Três processos concorrentes e sementes independentes preservadas no manifesto.\n\n'
        text+='| Execuções validadas | Melhor | Média | Mediana | Pior | Iguais a 160 | Abaixo de 160 |\n|---:|---:|---:|---:|---:|---:|---:|\n'
        text+=f"| {len(rows)} | {summary['best']:g} | {summary['mean']:.2f} | {summary['median']:g} | {summary['worst']:g} | {summary['equal_reference']} | {summary['below_reference']} |\n"
        text+=f"\nCP-SAT: {summary['cp_calls']} chamadas, {summary['cp_success']} sucessos, {summary['cp_infeasible']} vizinhanças inviáveis, {summary['cp_timeout']} encerramentos pelo prazo e {summary['cp_errors']} erros. Inviabilidade é local à vizinhança, e prazo não prova inviabilidade.\n"
        text+='\nO prazo configurado inclui calibração, busca e reparos. A construção inicial fica fora. As operações em andamento podem provocar pequeno excesso de tempo.\n'
        text+=f"\nExecuções acima de 35 segundos decorridos: {summary['runs_above_35s']}; maior tempo observado: {summary['max_seconds']:.3f} s.\n"
        text+='\n## Melhores acumulados a cada dez repetições\n\n| Repetições | Melhor | Média | Empates em 160 | Abaixo de 160 |\n|---:|---:|---:|---:|---:|\n'
        for n in range(10,count+1,10):
            s=json.loads((HERE/f'parcial_{n:02d}.json').read_text())
            text+=f"| {n} | {s['best']:g} | {s['mean']:.2f} | {s['equal_reference']} | {s['below_reference']} |\n"
        text+='\nTodos os certificados foram verificados independentemente com frações exatas: cobertura, percurso Manhattan e retorno, tempos da matriz, capacidade simultânea e atribuição física dos drones. Fontes, hashes, sementes, rotas e cronogramas estão preservados nesta pasta. Não houve ajuste de parâmetros durante o lote.\n'
        (HERE/'relatorio.md').write_text(text,encoding='utf-8')

def run(i):
    s=seed+i;stem=f'165-r-c_{i+1:02d}_{s}'
    p=subprocess.run([str(HERE/'benchmark.exe'),str(s),str(budget),'0',str(HERE/(stem+'.jsonl'))],cwd=ROOT,capture_output=True,text=True)
    (HERE/(stem+'.stderr')).write_text(p.stderr)
    if p.stdout:(HERE/(stem+'.stdout')).write_text(p.stdout)
    return {'run':i+1,'seed':s,'code':p.returncode,'stem':stem}

with cf.ThreadPoolExecutor(max_workers=workers) as pool:
    for result in pool.map(run,range(count)):
        manifest['runs'].append(result)
        (HERE/'manifesto.json').write_text(json.dumps(manifest,indent=2))
        assert result['code']==0,result
        row=json.loads((HERE/(result['stem']+'.jsonl')).read_text())
        assert row['seed']==result['seed'] and row['budget']==30
        rows.append(row)
        print(f"{len(rows)}/{count} concluídas",flush=True)
        if len(rows)%10==0:checkpoint()
manifest['finished']=datetime.datetime.now().astimezone().isoformat()
manifest['sources_unchanged']=all(hashlib.sha256((ROOT/n).read_bytes()).hexdigest()==h for n,h in hashes.items())
manifest['inputs_unchanged']=all(hashlib.sha256((ROOT/n).read_bytes()).hexdigest()==h for n,h in inputs.items())
(HERE/'manifesto.json').write_text(json.dumps(manifest,indent=2))
assert manifest['sources_unchanged'] and manifest['inputs_unchanged'] and len(rows)==count
checkpoint(final=True)
print('LOTE CONCLUIDO',flush=True)
