from pathlib import Path
import concurrent.futures as cf
import subprocess,json,hashlib
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
assert not (HERE/'experimentos_manifesto.json').exists()
sources=['SA.c','SA.h','cv.c','cv.h','lerInstancias.c','lerInstancias.h','main.h']
manifest={'purpose':'Exploracao curta de temperatura/retomadas; 3 sementes por variante, nao confirmatoria',
          'source_sha256':{n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in sources},'runs':[]}
(HERE/'fontes').mkdir(exist_ok=True)
for n in sources:(HERE/'fontes'/n).write_bytes((ROOT/n).read_bytes())
s=(ROOT/'SA.c').read_text()
s='#include <stdio.h>\n'+s
anchor='            if (parametros->pesosAdaptativos && !macro) {'
assert s.count(anchor)==1
s=s.replace(anchor,'''            if(estat->avaliacoes%1000==0) printf("THERM %.9g %.9g %.9g %.9g %llu\\n",relogioSA()-inicioBusca,temperatura,melhores[0].makespan,atual.makespan,estat->retomadas);
'''+anchor)
(HERE/'SA_trace.c').write_text(s)
b=(ROOT/'resultados/tres_maiores_100x_30s_20261027/benchmark.c').read_text()
b=b.replace('p.limiteSegundos=limite;','if(mask&256) p.maxRetomadas=0; p.limiteSegundos=limite;')
(HERE/'benchmark_trace.c').write_text(b)
subprocess.run(['gcc','-O2','-std=c11','-I',str(ROOT),str(HERE/'benchmark_trace.c'),str(HERE/'SA_trace.c'),str(ROOT/'cv.c'),str(ROOT/'lerInstancias.c'),'-lm','-o',str(HERE/'trace.exe')],check=True)
tasks=[(name,seed,mask) for seed in [202610281,202610282,202610283] for name in ['67-c-c','181-r-e'] for mask in [15,271]]
def run(task):
    name,seed,mask=task;stem=f'trace_{name}_{seed}_{mask}'
    p=subprocess.run([str(HERE/'trace.exe'),name,str(seed),str(mask),'10',str(HERE/(stem+'.jsonl'))],cwd=ROOT,capture_output=True,text=True)
    (HERE/(stem+'.stdout')).write_text(p.stdout)
    (HERE/(stem+'.stderr')).write_text(p.stderr)
    return {'instance':name,'seed':seed,'mask':mask,'budget':10,'code':p.returncode,'stem':stem}
with cf.ThreadPoolExecutor(max_workers=3) as pool:
    for r in pool.map(run,tasks):
        manifest['runs'].append(r)
        print(json.dumps(r),flush=True)
manifest['sources_unchanged']=all(hashlib.sha256((ROOT/n).read_bytes()).hexdigest()==h for n,h in manifest['source_sha256'].items())
(HERE/'experimentos_manifesto.json').write_text(json.dumps(manifest,indent=2))
assert manifest['sources_unchanged'] and all(r['code']==0 for r in manifest['runs'])
