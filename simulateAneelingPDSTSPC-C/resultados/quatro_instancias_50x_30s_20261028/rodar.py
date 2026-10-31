from pathlib import Path
from fractions import Fraction as F
import ast,concurrent.futures as cf,csv,datetime,hashlib,json,statistics,subprocess,sys,time

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
refs={'67-c-c':74,'165-r-c':160,'167-r-e':180,'181-r-e':216}
block=int(sys.argv[1]);assert 1<=block<=5
def stamp():return datetime.datetime.now().astimezone().isoformat()
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
if block==1:
    assert not (HERE/'manifesto.json').exists(), 'Preservar lote existente.'
    check=subprocess.run([str(ROOT/'.venv-cpsat/Scripts/python.exe'),str(ROOT/'reparo_cpsat.py'),'--check'],cwd=ROOT,capture_output=True,text=True,check=True)
    sources=['SA.c','SA.h','cv.c','cv.h','lerInstancias.c','lerInstancias.h','main.c','main.h','reparo_alvo.inc','reparo_cpsat.py','requirements-cpsat.txt','requirements-cpsat-lock.txt']
    (HERE/'fontes').mkdir(exist_ok=True)
    for n in sources:(HERE/'fontes'/n).write_bytes((ROOT/n).read_bytes())
    subprocess.run(['gcc','-O2','-std=c11','-Wall','-Wextra','-Werror','-pedantic','-I',str(ROOT),str(HERE/'benchmark.c'),'SA.c','cv.c','lerInstancias.c','-lm','-o',str(HERE/'benchmark.exe')],cwd=ROOT,check=True)
    inputs=[f'instances/pdstsp-c/large/{prefix}{name}.txt' for name in refs for prefix in ['','tau_kj_']]
    manifest={'started':stamp(),'count_per_instance':50,'block_size':10,'budget':30,'workers':3,'seed':int(time.time()),'instances':list(refs),'profiles':{n:'foco165' if n=='165-r-c' else 'exploracao' for n in refs},'source_sha256':{n:sha(ROOT/n) for n in sources},'input_sha256':{n:sha(ROOT/n) for n in inputs},'executable_sha256':sha(HERE/'benchmark.exe'),'cp_preflight':check.stdout.strip(),'runs':[],'blocks':[]}
else:
    manifest=json.loads((HERE/'manifesto.json').read_text())
assert len(manifest['blocks'])==block-1 and len(manifest['runs'])==(block-1)*40
assert all(sha(ROOT/n)==h and sha(HERE/'fontes'/n)==h for n,h in manifest['source_sha256'].items())
assert all(sha(ROOT/n)==h for n,h in manifest['input_sha256'].items())
assert sha(HERE/'benchmark.exe')==manifest['executable_sha256']
(HERE/'manifesto.json').write_text(json.dumps(manifest,indent=2))
tasks=[(name,manifest['seed']+c*50+i,i+1) for i in range((block-1)*10,block*10) for c,name in enumerate(refs)]
def run(task):
    name,seed,i=task;stem=f'{name}_{i:02d}_{seed}'
    assert not (HERE/(stem+'.jsonl')).exists()
    p=subprocess.run([str(HERE/'benchmark.exe'),name,str(seed),'30',str(HERE/(stem+'.jsonl'))],cwd=ROOT,capture_output=True,text=True)
    (HERE/(stem+'.stderr')).write_text(p.stderr)
    if p.stdout:(HERE/(stem+'.stdout')).write_text(p.stdout)
    return {'instance':name,'seed':seed,'run':i,'code':p.returncode,'stem':stem}
with cf.ThreadPoolExecutor(max_workers=3) as pool:
    for k,r in enumerate(pool.map(run,tasks),1):
        manifest['runs'].append(r)
        (HERE/'manifesto.json').write_text(json.dumps(manifest,indent=2))
        assert r['code']==0,r
        print(f'Bloco {block}/5: {k}/40 execucoes concluidas',flush=True)

tree=ast.parse((ROOT/'resultados/grupos_quatro_100/analisar.py').read_text())
fn=next(n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='verify_instance')
scope={'ROOT':ROOT,'F':F};exec(compile(ast.Module(body=[fn],type_ignores=[]),'validator','exec'),scope)
rows=[json.loads((HERE/(r['stem']+'.jsonl')).read_text()) for r in manifest['runs']]
summary=[];table=[]
for c,(name,ref) in enumerate(refs.items()):
    data=sorted((r for r in rows if r['instance']==name),key=lambda r:r['seed'])
    assert len(data)==block*10 and {r['seed'] for r in data}==set(range(manifest['seed']+c*50,manifest['seed']+c*50+block*10))
    assert all(r['budget']==30 and r['profile']==manifest['profiles'][name] for r in data)
    scope['verify_instance'](name,data)
    values=[r['claimed'] for r in data];last=[r['claimed'] for r in data[-10:]]
    s={'instance':name,'n':len(data),'best':min(values),'last_block_best':min(last),'mean':statistics.mean(values),'median':statistics.median(values),'worst':max(values),'reference':ref,'equal_reference':values.count(ref),'below_reference':sum(v<ref for v in values),'cp_calls':sum(r['repair']['cp_calls'] for r in data),'cp_success':sum(r['repair']['cp_success'] for r in data),'cp_infeasible':sum(r['repair']['cp_infeasible'] for r in data),'cp_timeout':sum(r['repair']['cp_timeout'] for r in data),'cp_errors':sum(r['repair']['cp_errors'] for r in data),'runs_above_35s':sum(r['seconds']>35 for r in data),'max_seconds':max(r['seconds'] for r in data)}
    summary.append(s)
    (HERE/f'melhor_{name}.json').write_text(json.dumps(min(data,key=lambda r:(r['claimed'],r['seed'])),indent=2))
    for i,r in enumerate(data,1):table.append({'instance':name,'run':i,'seed':r['seed'],'profile':r['profile'],'initial':r['initial'],'makespan':r['claimed'],'truck':r['truck'],'drone':r['drone'],'seconds':r['seconds'],'cp_calls':r['repair']['cp_calls'],'cp_success':r['repair']['cp_success'],'cp_errors':r['repair']['cp_errors']})
checkpoint={'repetitions_per_instance':block*10,'validated':len(rows),'summary':summary,'timestamp':stamp()}
(HERE/f'parcial_{block*10:02d}.json').write_text(json.dumps(checkpoint,indent=2))
(HERE/'resumo.json').write_text(json.dumps(checkpoint,indent=2))
for file,items in [('execucoes.csv',table),('resumo.csv',summary)]:
    with (HERE/file).open('w',newline='',encoding='utf-8') as f:
        writer=csv.DictWriter(f,fieldnames=list(items[0]));writer.writeheader();writer.writerows(items)
manifest['blocks'].append({'block':block,'completed':stamp(),'validated':len(rows)})
manifest['sources_unchanged']=all(sha(ROOT/n)==h for n,h in manifest['source_sha256'].items())
manifest['inputs_unchanged']=all(sha(ROOT/n)==h for n,h in manifest['input_sha256'].items())
assert manifest['sources_unchanged'] and manifest['inputs_unchanged']
if block==5:manifest['finished']=stamp()
(HERE/'manifesto.json').write_text(json.dumps(manifest,indent=2))
report=f'# Quatro instâncias — {block*10}/50 execuções por instância\n\n'
report+='Orçamento de 30 segundos incluindo calibração, SA e reparos; três processos concorrentes. A 165-r-c usa parametrosSAFoco165 (reparo dirigido, cadeias, filtros e CP-SAT); as outras usam parametrosSAExploracao, conforme o comportamento atual da main. Cada execução começa com nova construção inicial. Os lotes são separados em blocos de dez por instância e interrompidos entre blocos para apresentar os resultados.\n\n'
report+='| Instância | Execuções | Melhor acumulado | Melhor último bloco | Média | Pior | Referência | Empates | Abaixo da referência |\n|---|---:|---:|---:|---:|---:|---:|---:|---:|\n'
for s in summary:report+=f"| {s['instance']} | {s['n']} | {s['best']:g} | {s['last_block_best']:g} | {s['mean']:.2f} | {s['worst']:g} | {s['reference']} | {s['equal_reference']} | {s['below_reference']} |\n"
report+='\n## Parciais acumuladas\n\n| Repetições por instância | 67-c-c | 165-r-c | 167-r-e | 181-r-e |\n|---:|---:|---:|---:|---:|\n'
for k in range(1,block+1):
    snap=json.loads((HERE/f'parcial_{k*10:02d}.json').read_text())
    report+=f'| {k*10} | '+' | '.join(f"{s['best']:g}" for s in snap['summary'])+' |\n'
report+=f'\n{len(rows)} certificados verificados independentemente com frações exatas: cobertura, percurso e retorno, tempos da matriz, capacidade simultânea e atribuição física dos drones. Sementes, fontes, hashes, rotas e cronogramas preservados nesta pasta.\n'
for s in summary:
    if s['cp_calls']:report+=f"\nCP-SAT na {s['instance']}: {s['cp_calls']} chamadas, {s['cp_success']} reparos bem-sucedidos, {s['cp_infeasible']} vizinhanças inviáveis, {s['cp_timeout']} encerramentos pelo prazo e {s['cp_errors']} erros. Inviabilidade é local à vizinhança; prazo não prova inviabilidade.\n"
report+=f"\nExecuções com tempo decorrido acima de 35 s: {sum(s['runs_above_35s'] for s in summary)}; maior tempo registrado: {max(s['max_seconds'] for s in summary):.3f} s. A construção inicial fica fora do orçamento e operações em andamento podem causar pequenos excessos.\n"
(HERE/'relatorio.md').write_text(report,encoding='utf-8')
print('BLOCO_VALIDADO '+json.dumps(checkpoint),flush=True)
print(f'BLOCO {block} CONCLUIDO; sem processos de benchmark ativos deste bloco.',flush=True)
