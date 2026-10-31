"""Compilacao, testes e lotes balanceados; cada tarefa usa as mesmas sementes."""
from pathlib import Path
import concurrent.futures
import datetime
import hashlib
import json
import subprocess
import sys

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
CANDIDATE=HERE/'candidato'
PYTHON=sys.executable
GCC=r'C:\TDM-GCC-64\bin\gcc.exe'

def compile_one(source,target):
    cmd=[GCC,'-std=c11','-O2','-Wall','-Wextra','-pedantic','-I',str(CANDIDATE),str(source),
         *(str(CANDIDATE/n) for n in ['SA.c','cv.c','lerInstancias.c']),'-lm','-o',str(target)]
    subprocess.run(cmd,cwd=ROOT,check=True)

if sys.argv[1] in ('build','compile'):
    subprocess.run([PYTHON,str(HERE/'preparar.py')],cwd=ROOT,check=True)
    compile_one(HERE/'benchmark.c',HERE/'benchmark.exe')
    if sys.argv[1]=='compile':raise SystemExit()
    tests=list((ROOT/'tests').glob('test_*.c'))+[HERE/'test_melhorias.c']
    for test in tests:
        target=HERE/(test.stem+'.exe')
        compile_one(test,target)
        subprocess.run([str(target)],cwd=ROOT,check=True,timeout=120)
        print(test.name+' OK',flush=True)
    raise SystemExit()

stage=sys.argv[1]; masks=list(map(int,sys.argv[2].split(',')))
count=int(sys.argv[3]);budget=float(sys.argv[4]);seed=int(sys.argv[5])
names=sys.argv[6].split(',') if len(sys.argv)>6 else ['67-c-c']
folder=HERE/stage;folder.mkdir(exist_ok=False)
frozen=folder/'fontes';frozen.mkdir()
for path in CANDIDATE.iterdir():
    if path.is_file(): (frozen/path.name).write_bytes(path.read_bytes())
tasks=[]
# Blocos curtos intercalados reduzem a confusao entre variante e momento de execucao.
for start in range(0,count,5):
    for name in names:
        for mask in (masks if (start//5)%2==0 else list(reversed(masks))):
            tasks.append((name,mask,start,min(5,count-start)))
meta={'stage':stage,'masks':masks,'count_per_instance_mask':count,'budget':budget,'seed':seed,
      'instances':names,'workers':4,'started':datetime.datetime.now().astimezone().isoformat()}
meta['source_sha256']={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in frozen.iterdir()}
(folder/'manifesto.json').write_text(json.dumps(meta,indent=2))
def worker(task):
    name,mask,start,n=task
    output=folder/f'{name}_m{mask}_{start:04}.jsonl'
    cmd=[str(HERE/'benchmark.exe'),name,str(seed),str(start),str(n),str(mask),str(budget),'0',str(output)]
    with output.with_suffix('.stderr').open('w') as err:
        code=subprocess.run(cmd,cwd=ROOT,stdout=err,stderr=err).returncode
    if code:raise RuntimeError((task,code))
    return task
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
    for task in pool.map(worker,tasks):print('DONE '+str(task),flush=True)
meta['finished']=datetime.datetime.now().astimezone().isoformat()
(folder/'manifesto.json').write_text(json.dumps(meta,indent=2))
print('LOTE CONCLUIDO',flush=True)
