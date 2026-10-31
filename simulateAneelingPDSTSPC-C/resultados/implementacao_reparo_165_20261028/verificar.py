from pathlib import Path
import concurrent.futures as cf
import subprocess,json
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
def run(path):
    exe=HERE/(path.stem+'.exe')
    p=subprocess.run(['gcc','-O2','-std=c11','-Wall','-Wextra','-pedantic','-I',str(ROOT),str(path),'SA.c','cv.c','lerInstancias.c','-lm','-o',str(exe)],cwd=ROOT,capture_output=True,text=True)
    result={'test':path.name,'compile':p.returncode,'compile_stderr':p.stderr}
    if p.returncode==0:
        args=[str(exe)]
        if path.stem=='test_reparo_alvo':args.append(str(ROOT/'.venv-cpsat/Scripts/python.exe'))
        q=subprocess.run(args,cwd=ROOT,capture_output=True,text=True,timeout=120)
        result.update(code=q.returncode,stdout=q.stdout,stderr=q.stderr)
    return result
results=[]
with cf.ThreadPoolExecutor(max_workers=3) as pool:
    for result in pool.map(run,sorted((ROOT/'tests').glob('test_*.c'))):
        results.append(result);print(result['test'],result.get('code','COMPILE FAILED'),flush=True)
q=subprocess.run([str(ROOT/'.venv-cpsat/Scripts/python.exe'),'-m','unittest','discover','-s','tests','-p','test_reparo_cpsat.py','-v'],cwd=ROOT,capture_output=True,text=True)
results.append({'test':'test_reparo_cpsat.py','compile':0,'code':q.returncode,'stdout':q.stdout,'stderr':q.stderr})
(HERE/'testes.json').write_text(json.dumps(results,indent=2))
assert all(r['compile']==0 and r.get('code')==0 for r in results)
print('TODOS OS TESTES PASSARAM',flush=True)
