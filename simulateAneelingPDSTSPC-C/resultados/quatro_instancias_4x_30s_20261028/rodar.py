from pathlib import Path
from fractions import Fraction as F
import ast,concurrent.futures as cf,csv,datetime,hashlib,json,statistics,subprocess,time

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
refs={'67-c-c':74,'165-r-c':160,'167-r-e':180,'181-r-e':216}
assert not (HERE/'manifesto.json').exists(), 'Preservar lote existente.'
count=4;budget=30;seed=int(time.time())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def stamp():return datetime.datetime.now().astimezone().isoformat()
check=subprocess.run([str(ROOT/'.venv-cpsat/Scripts/python.exe'),'reparo_cpsat.py','--check'],cwd=ROOT,capture_output=True,text=True,check=True)
sources=['SA.c','SA.h','cv.c','cv.h','lerInstancias.c','lerInstancias.h','main.c','main.h','reparo_alvo.inc','reparo_cpsat.py','requirements-cpsat.txt','requirements-cpsat-lock.txt']
(HERE/'fontes').mkdir(exist_ok=True)
for n in sources:(HERE/'fontes'/n).write_bytes((ROOT/n).read_bytes())
subprocess.run(['gcc','-O2','-std=c11','-Wall','-Wextra','-Werror','-pedantic','-I',str(ROOT),str(HERE/'benchmark.c'),'SA.c','cv.c','lerInstancias.c','-lm','-o',str(HERE/'benchmark.exe')],cwd=ROOT,check=True)
inputs=[f'instances/pdstsp-c/large/{prefix}{name}.txt' for name in refs for prefix in ['','tau_kj_']]
manifest={'started':stamp(),'count_per_instance':count,'budget':budget,'workers':3,'seed':seed,'profiles':{n:'foco165' if n=='165-r-c' else 'exploracao' for n in refs},'source_sha256':{n:sha(ROOT/n) for n in sources},'input_sha256':{n:sha(ROOT/n) for n in inputs},'cp_preflight':check.stdout.strip(),'executable_sha256':sha(HERE/'benchmark.exe'),'runs':[]}
(HERE/'manifesto.json').write_text(json.dumps(manifest,indent=2))
tasks=[(name,seed+c*count+i,i+1) for i in range(count) for c,name in enumerate(refs)]
def run(task):
    name,s,i=task;stem=f'{name}_{i:02d}_{s}'
    p=subprocess.run([str(HERE/'benchmark.exe'),name,str(s),str(budget),str(HERE/(stem+'.jsonl'))],cwd=ROOT,capture_output=True,text=True)
    (HERE/(stem+'.stderr')).write_text(p.stderr)
    if p.stdout:(HERE/(stem+'.stdout')).write_text(p.stdout)
    return {'instance':name,'seed':s,'run':i,'code':p.returncode,'stem':stem}
with cf.ThreadPoolExecutor(max_workers=3) as pool:
    for k,r in enumerate(pool.map(run,tasks),1):
        manifest['runs'].append(r)
        (HERE/'manifesto.json').write_text(json.dumps(manifest,indent=2))
        assert r['code']==0,r
        print(f'{k}/16 concluidas',flush=True)
tree=ast.parse((ROOT/'resultados/grupos_quatro_100/analisar.py').read_text())
fn=next(n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='verify_instance')
scope={'ROOT':ROOT,'F':F};exec(compile(ast.Module(body=[fn],type_ignores=[]),'validator','exec'),scope)
rows=[json.loads((HERE/(r['stem']+'.jsonl')).read_text()) for r in manifest['runs']]
summary=[];table=[]
for c,(name,ref) in enumerate(refs.items()):
    data=sorted((r for r in rows if r['instance']==name),key=lambda r:r['seed'])
    assert len(data)==4 and {r['seed'] for r in data}==set(range(seed+c*count,seed+(c+1)*count))
    assert all(r['budget']==30 and r['profile']==manifest['profiles'][name] for r in data)
    scope['verify_instance'](name,data)
    values=[r['claimed'] for r in data]
    summary.append({'instance':name,'n':len(data),'best':min(values),'mean':statistics.mean(values),'worst':max(values),'reference':ref,'equal_reference':values.count(ref),'below_reference':sum(v<ref for v in values),'results':values,'cp_calls':sum(r['repair']['cp_calls'] for r in data),'cp_success':sum(r['repair']['cp_success'] for r in data),'cp_infeasible':sum(r['repair']['cp_infeasible'] for r in data),'cp_timeout':sum(r['repair']['cp_timeout'] for r in data),'cp_errors':sum(r['repair']['cp_errors'] for r in data),'runs_above_35s':sum(r['seconds']>35 for r in data),'max_seconds':max(r['seconds'] for r in data)})
    (HERE/f'melhor_{name}.json').write_text(json.dumps(min(data,key=lambda r:(r['claimed'],r['seed'])),indent=2))
    for i,r in enumerate(data,1):table.append({'instance':name,'run':i,'seed':r['seed'],'profile':r['profile'],'makespan':r['claimed'],'truck':r['truck'],'drone':r['drone'],'seconds':r['seconds']})
with (HERE/'execucoes.csv').open('w',newline='',encoding='utf-8') as f:
    w=csv.DictWriter(f,fieldnames=list(table[0]));w.writeheader();w.writerows(table)
manifest['finished']=stamp();manifest['sources_unchanged']=all(sha(ROOT/n)==h for n,h in manifest['source_sha256'].items());manifest['inputs_unchanged']=all(sha(ROOT/n)==h for n,h in manifest['input_sha256'].items())
assert manifest['sources_unchanged'] and manifest['inputs_unchanged']
(HERE/'manifesto.json').write_text(json.dumps(manifest,indent=2))
(HERE/'resumo.json').write_text(json.dumps({'validated':16,'summary':summary},indent=2))
report='# Quatro instâncias — quatro execuções de 30 segundos cada\n\n'
report+='Cada execução parte de nova construção inicial. Perfil novo com reparos e CP-SAT na 165-r-c; exploração atual nas outras três. Três processos concorrentes, fontes e parâmetros fixos, sementes e certificados preservados nesta pasta.\n\n'
report+='| Instância | Execução 1 | Execução 2 | Execução 3 | Execução 4 | Melhor | Média | Referência |\n|---|---:|---:|---:|---:|---:|---:|---:|\n'
for s in summary:report+=f"| {s['instance']} | "+' | '.join(f'{v:g}' for v in s['results'])+f" | {s['best']:g} | {s['mean']:.2f} | {s['reference']} |\n"
report+='\nOs 16 certificados passaram pela verificação independente com frações exatas: cobertura, percurso e retorno, durações da matriz, capacidade simultânea e atribuição física dos drones. A construção inicial fica fora do limite; operações em andamento podem causar pequenos excessos.\n'
for s in summary:
    if s['cp_calls']:report+=f"\nCP-SAT: {s['cp_calls']} chamadas, {s['cp_success']} reparos bem-sucedidos, {s['cp_infeasible']} vizinhanças inviáveis, {s['cp_timeout']} encerramentos pelo prazo e {s['cp_errors']} erros. Inviabilidade é local à vizinhança; prazo não prova inviabilidade.\n"
report+=f"\nExecuções acima de 35 segundos decorridos: {sum(s['runs_above_35s'] for s in summary)}. Maior tempo observado: {max(s['max_seconds'] for s in summary):.3f} s.\n"
(HERE/'relatorio.md').write_text(report,encoding='utf-8')
print(json.dumps(summary,indent=2),flush=True)
print('LOTE CONCLUIDO; 16 CERTIFICADOS VALIDADOS',flush=True)
