from pathlib import Path
from fractions import Fraction as F
import ast,concurrent.futures as cf,subprocess,json,hashlib,statistics,datetime
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
assert not (HERE/'comparacao_manifesto.json').exists()
for n in ['lerInstancias.c','lerInstancias.h','main.h']:
    (HERE/'antes'/n).write_bytes((ROOT/'resultados/analise_165_20261028/fontes'/n).read_bytes())
common=['gcc','-O2','-std=c11','-Wall','-Wextra','-Werror','-pedantic']
subprocess.run(common+['-DCONTROLE','-I',str(HERE/'antes'),str(HERE/'benchmark.c')]+[str(HERE/'antes'/n) for n in ['SA.c','cv.c','lerInstancias.c']]+['-lm','-o',str(HERE/'controle.exe')],cwd=ROOT,check=True)
subprocess.run(common+['-I',str(ROOT),str(HERE/'benchmark.c'),'SA.c','cv.c','lerInstancias.c','-lm','-o',str(HERE/'novo.exe')],cwd=ROOT,check=True)
sources=['SA.c','SA.h','cv.c','cv.h','main.c','main.h','reparo_alvo.inc','reparo_cpsat.py']
hashes={n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in sources}
tasks=[(mode,seed,warm) for seed,warm in [(202610281,0),(202610282,0),(202610283,0),(202610284,1)] for mode in ['controle','novo']]
def run(task):
    mode,seed,warm=task;stem=f'{mode}_{seed}_{warm}'
    p=subprocess.run([str(HERE/(mode+'.exe')),str(seed),'30',str(warm),str(HERE/(stem+'.jsonl'))],cwd=ROOT,capture_output=True,text=True)
    (HERE/(stem+'.stderr')).write_text(p.stderr)
    if p.stdout:(HERE/(stem+'.stdout')).write_text(p.stdout)
    r={'mode':mode,'seed':seed,'warm':warm,'code':p.returncode,'stem':stem}
    print(json.dumps(r),flush=True);return r
with cf.ThreadPoolExecutor(max_workers=3) as pool:results=list(pool.map(run,tasks))
manifest={'started_from':'3 cold starts + 1 warm start at 160 per version','budget':30,'workers':3,'runs':results,'source_sha256':hashes,
          'sources_unchanged':all(hashlib.sha256((ROOT/n).read_bytes()).hexdigest()==h for n,h in hashes.items())}
(HERE/'comparacao_manifesto.json').write_text(json.dumps(manifest,indent=2))
assert manifest['sources_unchanged'] and all(r['code']==0 for r in results)
tree=ast.parse((ROOT/'resultados/grupos_quatro_100/analisar.py').read_text());fn=next(n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='verify_instance')
scope={'ROOT':ROOT,'F':F};exec(compile(ast.Module(body=[fn],type_ignores=[]),'validator','exec'),scope)
rows=[dict(json.loads((HERE/(r['stem']+'.jsonl')).read_text()),mode=r['mode'],warm=r['warm']) for r in results]
scope['verify_instance']('165-r-c',rows)
summary=[]
for warm in [0,1]:
    for mode in ['controle','novo']:
        data=[r for r in rows if r['warm']==warm and r['mode']==mode]
        summary.append({'mode':mode,'warm':warm,'n':len(data),'results':[r['claimed'] for r in data],'best':min(r['claimed'] for r in data),'mean':statistics.mean(r['claimed'] for r in data),
                        'cp_calls':sum(r.get('repair',{}).get('cp_calls',0) for r in data),'cp_success':sum(r.get('repair',{}).get('cp_success',0) for r in data),'cp_errors':sum(r.get('repair',{}).get('cp_errors',0) for r in data)})
(HERE/'comparacao.json').write_text(json.dumps({'validated':len(rows),'summary':summary},indent=2))
print(json.dumps(summary,indent=2))
