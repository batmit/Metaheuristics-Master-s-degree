from pathlib import Path
import concurrent.futures as cf
import datetime
import hashlib
import json
import subprocess
import time

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
names=['165-r-c','167-r-e','181-r-e']
seed=int(time.time());count=100;budget=30
assert not (HERE/'manifesto.json').exists(), 'Preservar o lote anterior: usar outra pasta.'
sources=['main.c','SA.c','SA.h','cv.c','cv.h','lerInstancias.c','lerInstancias.h','main.h']
hashes={n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in sources}
inputs={f'instances/pdstsp-c/large/{prefix}{name}.txt':hashlib.sha256((ROOT/f'instances/pdstsp-c/large/{prefix}{name}.txt').read_bytes()).hexdigest()
        for name in names for prefix in ('','tau_kj_')}
manifest={'started':datetime.datetime.now().astimezone().isoformat(),'seed':seed,'count':count,
          'budget':budget,'workers':3,'mask':15,'instances':names,'source_sha256':hashes,
          'input_sha256':inputs,'returncodes':[]}
(HERE/'fontes').mkdir(exist_ok=True)
for n in sources:(HERE/'fontes'/n).write_bytes((ROOT/n).read_bytes())
(HERE/'manifesto.json').write_text(json.dumps(manifest,indent=2))
tasks=[(name,seed+c*count+i,i+1) for i in range(count) for c,name in enumerate(names)]
def run(task):
    name,s,i=task;stem=f'{name}_{i:02d}_{s}'
    p=subprocess.run([str(HERE/'benchmark.exe'),name,str(s),'15',str(budget),str(HERE/(stem+'.jsonl'))],cwd=ROOT,capture_output=True,text=True)
    (HERE/(stem+'.stderr')).write_text(p.stderr)
    if p.stdout:(HERE/(stem+'.stdout')).write_text(p.stdout)
    return {'instance':name,'seed':s,'run':i,'code':p.returncode}
with cf.ThreadPoolExecutor(max_workers=3) as pool:
    for k,r in enumerate(pool.map(run,tasks),1):
        manifest['returncodes'].append(r)
        (HERE/'manifesto.json').write_text(json.dumps(manifest,indent=2))
        if k%3==0: print(f'{k}/300 concluidas ({k//3}/100 por instancia)',flush=True)
        if k%30==0:
            milestone=k//3
            rows=[json.loads((HERE/f'{name}_{i:02d}_{seed+c*count+i-1}.jsonl').read_text())
                  for c,name in enumerate(names) for i in range(1,milestone+1)]
            import ast
            from fractions import Fraction as F
            tree=ast.parse((ROOT/'resultados/grupos_quatro_100/analisar.py').read_text())
            fn=next(n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='verify_instance')
            scope={'ROOT':ROOT,'F':F}
            exec(compile(ast.Module(body=[fn],type_ignores=[]),'<independent-validator>','exec'),scope)
            snapshot={'repetitions_per_instance':milestone,'validated':len(rows),'summary':[]}
            for name,ref in zip(names,[160,180,216]):
                data=[r for r in rows if r['instance']==name]
                scope['verify_instance'](name,data)
                values=[r['claimed'] for r in data]
                snapshot['summary'].append({'instance':name,'best':min(values),'reference':ref,
                    'ties':values.count(ref),'improvements':sum(v<ref for v in values)})
            (HERE/f'parcial_{milestone:03d}.json').write_text(json.dumps(snapshot,indent=2))
            print('MARCO_VALIDADO '+json.dumps(snapshot),flush=True)
manifest['finished']=datetime.datetime.now().astimezone().isoformat()
manifest['sources_unchanged']=all(hashlib.sha256((ROOT/n).read_bytes()).hexdigest()==h for n,h in hashes.items())
(HERE/'manifesto.json').write_text(json.dumps(manifest,indent=2))
assert manifest['sources_unchanged'] and all(r['code']==0 for r in manifest['returncodes'])
print('LOTE CONCLUIDO',flush=True)



