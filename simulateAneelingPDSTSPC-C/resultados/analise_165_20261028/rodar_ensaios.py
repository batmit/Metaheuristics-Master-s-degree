from pathlib import Path
import concurrent.futures as cf
import subprocess,json,hashlib
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
assert not (HERE/'manifesto_ensaios.json').exists()
sources=['SA.c','SA.h','cv.c','cv.h','lerInstancias.c','lerInstancias.h','main.h']
hashes={n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in sources}
(HERE/'fontes').mkdir(exist_ok=True)
for n in sources:(HERE/'fontes'/n).write_bytes((ROOT/n).read_bytes())
s=(ROOT/'cv.c').read_text()
s=s.replace('double alvo=fmin(atual->makespan,custoRotaMelhoria(inst,s));','double alvo=atual->makespan-1.0;').replace('alvo=fmin(atual->makespan,custoRotaMelhoria(inst,s));','alvo=atual->makespan-1.0;')
(HERE/'cv_alvo.c').write_text(s)
for variant,cv in [('controle','cv.c'),('alvo',str(HERE/'cv_alvo.c'))]:
    subprocess.run(['gcc','-O2','-std=c11','-I',str(ROOT),str(HERE/'busca.c'),'SA.c',cv,'lerInstancias.c','-lm','-o',str(HERE/f'busca_{variant}.exe')],cwd=ROOT,check=True)
def run(task):
    seed,mode=task;stem=f'ensaio_{seed}_{mode}'
    exe='alvo' if mode==3 else 'controle'
    p=subprocess.run([str(HERE/f'busca_{exe}.exe'),str(seed),str(mode),str(HERE/'165-r-c_best.txt'),str(HERE/(stem+'.jsonl'))],cwd=ROOT,capture_output=True,text=True)
    (HERE/(stem+'.stderr')).write_text(p.stderr)
    if p.stdout:(HERE/(stem+'.stdout')).write_text(p.stdout)
    return {'seed':seed,'mode':mode,'code':p.returncode}
tasks=[(seed,mode) for seed in [202610281,202610282,202610283] for mode in range(4)]
results=[]
with cf.ThreadPoolExecutor(max_workers=3) as pool:
    for r in pool.map(run,tasks):results.append(r);print(json.dumps(r),flush=True)
manifest={'warm_start_seed':1793136331,'seconds':15,'workers':3,'seeds':[202610281,202610282,202610283],
          'modes':{'0':'controle atual','1':'probabilidades macro 5%+5%','2':'destruicao 15%-30%','3':'alvo makespan atual -1'},
          'runs':results,'source_sha256':hashes,
          'sources_unchanged':all(hashlib.sha256((ROOT/n).read_bytes()).hexdigest()==h for n,h in hashes.items())}
(HERE/'manifesto_ensaios.json').write_text(json.dumps(manifest,indent=2))
assert manifest['sources_unchanged'] and all(r['code']==0 for r in results)
