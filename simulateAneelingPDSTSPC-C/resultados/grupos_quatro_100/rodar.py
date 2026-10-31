"""100 ciclos completos por instancia e versao; sementes pareadas, sem corte."""
from pathlib import Path
import concurrent.futures
import datetime
import hashlib
import json
import subprocess
import time

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
GCC=r'C:\TDM-GCC-64\bin\gcc.exe'
NAMES=['67-c-c','165-r-c','167-r-e','181-r-e']
assert not (HERE/'manifesto.json').exists(), 'Nao sobrescrever um experimento iniciado.'
source=HERE/'fontes';source.mkdir()
files=['main.c','main.h','SA.c','SA.h','cv.c','cv.h','lerInstancias.c','lerInstancias.h']
for name in files:(source/name).write_bytes((ROOT/name).read_bytes())
(HERE/'benchmark.c').write_bytes((ROOT/'resultados/experimento_melhorias_67/integracao/reproduzir.c').read_bytes())
def compile_one(main,target):
    cmd=[GCC,'-std=c11','-O2','-Wall','-Wextra','-pedantic','-I',str(source),str(main),
         *(str(source/n) for n in ['SA.c','cv.c','lerInstancias.c']),'-lm','-o',str(target)]
    subprocess.run(cmd,cwd=ROOT,check=True)
    return cmd
command=compile_one(HERE/'benchmark.c',HERE/'benchmark.exe')
compile_one(source/'main.c',HERE/'main_grupos.exe')
base=int(time.time())
# Evitar sementes ja utilizadas nos lotes recentes, mantendo a origem no relogio.
used=set()
for folder in ['67-c-c_200','67-c-c_400','experimento_melhorias_67']:
    for file in (ROOT/'resultados'/folder).rglob('*.jsonl'):
        for line in file.read_text().splitlines():
            try:used.add(json.loads(line)['seed'])
            except (json.JSONDecodeError,KeyError):pass
while used.intersection(range(base,base+400)):
    base=max(used.intersection(range(base,base+400)))+1
data_files=[ROOT/f'instances/pdstsp-c/large/{prefix}{name}.txt' for name in NAMES for prefix in ['', 'tau_kj_']]
meta={'instances':NAMES,'masks':[0,1],'count_per_instance_mask':100,'workers':4,
      'base_seed':base,'seed_by_instance':{n:base+c*100 for c,n in enumerate(NAMES)},
      'seed_source':'relogio, avancando apenas para evitar sementes anteriores',
      'budget':0,'full_cooling':True,'started':datetime.datetime.now().astimezone().isoformat(),
      'source_sha256':{n:hashlib.sha256((source/n).read_bytes()).hexdigest() for n in files},
      'input_sha256':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in data_files},
      'compiler':subprocess.check_output([GCC,'--version'],text=True).splitlines()[0],
      'compile_command':command,'returncodes':[]}
(HERE/'manifesto.json').write_text(json.dumps(meta,indent=2))
tasks=[]
for start in range(0,100,5):
    order=NAMES if (start//5)%2==0 else list(reversed(NAMES))
    for name in order:
        for mask in ([0,1] if (start//5)%2==0 else [1,0]):tasks.append((name,mask,start))
def worker(task):
    name,mask,start=task
    output=HERE/f'{name}_m{mask}_{start:04}.jsonl'
    cmd=[str(HERE/'benchmark.exe'),name,str(meta['seed_by_instance'][name]),str(start),'5',str(mask),'0','0',str(output)]
    with output.with_suffix('.stderr').open('w') as errors:
        code=subprocess.run(cmd,cwd=ROOT,stdout=errors,stderr=errors).returncode
    if code:raise RuntimeError((task,code))
    return {'instance':name,'mask':mask,'start':start,'code':code}
print(f'800 ciclos completos; base {base}; 100 por instancia e versao.',flush=True)
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
    for result in pool.map(worker,tasks):
        meta['returncodes'].append(result)
        print('DONE '+json.dumps(result),flush=True)
meta['finished']=datetime.datetime.now().astimezone().isoformat()
meta['sources_unchanged']=all(hashlib.sha256((ROOT/n).read_bytes()).hexdigest()==h for n,h in meta['source_sha256'].items())
(HERE/'manifesto.json').write_text(json.dumps(meta,indent=2))
assert meta['sources_unchanged']
print('LOTE CONCLUIDO',flush=True)
