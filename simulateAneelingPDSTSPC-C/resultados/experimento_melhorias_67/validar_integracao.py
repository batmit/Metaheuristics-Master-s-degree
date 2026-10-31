from pathlib import Path
import json
import subprocess
import shutil
import sys

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
GCC=r'C:\TDM-GCC-64\bin\gcc.exe'
shutil.copyfile(HERE/'test_grupos_producao.c',ROOT/'tests/test_grupos.c')
folder=HERE/'integracao';folder.mkdir(exist_ok=True)
def compile_one(source,target):
    subprocess.run([GCC,'-std=c11','-O2','-Wall','-Wextra','-pedantic','-I',str(ROOT),str(source),
                    *(str(ROOT/n) for n in ['SA.c','cv.c','lerInstancias.c']),'-lm','-o',str(target)],cwd=ROOT,check=True)
tests=[]
for path in sorted((ROOT/'tests').glob('test_*.c')):
    target=folder/(path.stem+'.exe');compile_one(path,target)
    subprocess.run([str(target)],cwd=ROOT,check=True,timeout=120)
    tests.append(path.name);print(path.name+' OK',flush=True)
compile_one(ROOT/'main.c',folder/'main_integrado.exe')
source=(HERE/'benchmark.c').read_text()
source=source.replace('(mascara&SA_INICIO_DIVERSO)?construirSolucaoInicialDiversa(&inst,&s):construirSolucaoInicial(&inst,&s)','construirSolucaoInicial(&inst,&s)')
source=source.replace('p.melhorias=mascara;','p.movimentosGrupo=mascara!=0;')
source=source.replace('p.tempInicial=temperatura;p.maxAvaliacoes=avaliacoes;','p.tempInicial=temperatura; (void)avaliacoes;')
source=source.replace('        if(budget>0) p.maxSegundos=fmax(0.001,budget-(double)(clock()-inicio)/CLOCKS_PER_SEC);','        if(budget!=0) return 11;')
(folder/'reproduzir.c').write_text(source)
compile_one(folder/'reproduzir.c',folder/'reproduzir.exe')
reproducoes=[]
for mask in [0,1]:
    out=folder/f'reproducao_m{mask}.jsonl'
    subprocess.run([str(folder/'reproduzir.exe'),'67-c-c','1793034000','0','5',str(mask),'0','0',str(out)],cwd=ROOT,check=True)
    expected={r['seed']:r for p in (HERE/'ciclo_completo').glob(f'*_m{mask}_*.jsonl') for r in map(json.loads,p.read_text().splitlines())}
    actual=list(map(json.loads,out.read_text().splitlines()))
    keys=['seed','initial','temperature','claimed','truck','drone','route','missions']
    assert len(actual)==5
    for a in actual:assert all(a[k]==expected[a['seed']][k] for k in keys),(mask,a['seed'])
    reproducoes.append({'mask':mask,'identical_solutions':5})
result={'tests_passed':tests,'main_compiled':True,'exact_reproductions':reproducoes}
(folder/'validacao.json').write_text(json.dumps(result,indent=2))
print(json.dumps(result,indent=2),flush=True)
