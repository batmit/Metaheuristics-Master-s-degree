"""Compara decisoes completas do controle com os fontes congelados anteriores."""
from pathlib import Path
import json
import subprocess

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
gcc='C:/TDM-GCC-64/bin/gcc.exe'
driver=ROOT/'resultados/grupos_quatro_100/benchmark.c'
for mode in ('antes','atual'):
    src=HERE/'fontes_antes' if mode=='antes' else ROOT
    subprocess.run([gcc,'-std=c11','-O2','-Wall','-Wextra','-pedantic','-I',str(src),'-I',str(ROOT),
                    str(driver),str(src/'SA.c'),str(src/'cv.c'),str(ROOT/'lerInstancias.c'),'-lm',
                    '-o',str(HERE/f'controle_{mode}.exe')],cwd=ROOT,check=True)
results=[]
for name in ('67-c-c','165-r-c','167-r-e','181-r-e'):
    rows={}
    for mode in ('antes','atual'):
        out=HERE/f'compatibilidade_{name}_{mode}.jsonl'
        subprocess.run([str(HERE/f'controle_{mode}.exe'),name,'731','0','1','1','0','0',str(out)],cwd=ROOT,check=True)
        rows[mode]=json.loads(out.read_text())
    fields=['initial','temperature','claimed','truck','drone','route','missions']
    assert all(rows['antes'][f]==rows['atual'][f] for f in fields),name
    results.append({'instance':name,'seed':731,'makespan':rows['atual']['claimed'],'same_decisions':True})
    print(f'COMPATIVEL {name}',flush=True)
(HERE/'compatibilidade.json').write_text(json.dumps(results,indent=2))
