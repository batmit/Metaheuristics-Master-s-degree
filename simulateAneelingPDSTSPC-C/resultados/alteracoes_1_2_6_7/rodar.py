from pathlib import Path
import concurrent.futures as cf
import datetime
import hashlib
import json
import subprocess
import time
import sys

ROOT=next(p for p in Path(__file__).resolve().parents if (p/'cv.c').is_file() and (p/'instances/pdstsp-c').is_dir())
HERE=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path(__file__).resolve().parent
HERE.mkdir(parents=True,exist_ok=True)
names=['67-c-c','165-r-c','167-r-e','181-r-e']
seed=int(time.time());count=10;budget=10
sources=['main.c','SA.c','SA.h','cv.c','cv.h','lerInstancias.c','lerInstancias.h','main.h']
hashes={n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in sources}
manifest={'started':datetime.datetime.now().astimezone().isoformat(),'seed':seed,'count':count,
          'budget':budget,'workers':4,'masks':[0,15],'source_sha256':hashes,'returncodes':[]}
(HERE/'fontes_depois').mkdir(exist_ok=True)
for n in sources:(HERE/'fontes_depois'/n).write_bytes((ROOT/n).read_bytes())
(HERE/'manifesto.json').write_text(json.dumps(manifest,indent=2))
tasks=[]
for i in range(count):
    for c,name in enumerate(names):
        for mask in ([0,15] if (i+c)%2==0 else [15,0]):tasks.append((name,seed+c*count+i,mask))
def run(task):
    name,s,m=task;stem=f'{name}_{s}_m{m}';out=HERE/(stem+'.jsonl')
    p=subprocess.run([str(HERE/'benchmark.exe'),name,str(s),str(m),str(budget),str(out)],cwd=ROOT,capture_output=True,text=True)
    (HERE/(stem+'.stderr')).write_text(p.stderr)
    if p.stdout:(HERE/(stem+'.stdout')).write_text(p.stdout)
    return {'instance':name,'seed':s,'mask':m,'code':p.returncode}
with cf.ThreadPoolExecutor(max_workers=4) as pool:
    for k,r in enumerate(pool.map(run,tasks),1):
        manifest['returncodes'].append(r)
        (HERE/'manifesto.json').write_text(json.dumps(manifest,indent=2))
        if k%8==0:print(f'{k}/{len(tasks)} concluidas',flush=True)
manifest['finished']=datetime.datetime.now().astimezone().isoformat()
manifest['sources_unchanged']=all(hashlib.sha256((ROOT/n).read_bytes()).hexdigest()==h for n,h in hashes.items())
(HERE/'manifesto.json').write_text(json.dumps(manifest,indent=2))
assert manifest['sources_unchanged'] and all(r['code']==0 for r in manifest['returncodes'])
print('LOTE CONCLUIDO',flush=True)
