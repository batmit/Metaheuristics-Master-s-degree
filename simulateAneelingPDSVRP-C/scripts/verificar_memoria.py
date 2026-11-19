"""Executa casos de busca e oráculos com ASan/UBSan, em binários isolados."""
import json
import subprocess
from executar_benchmark import ROOT


def main():
    out=ROOT/'output/verificacao_memoria'
    out.mkdir(exist_ok=True)
    results=[]
    common=['gcc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-g','-O1',
            '-fsanitize=address,undefined','-fno-omit-frame-pointer',
            'lerInstancias.c','cv.c','drone_bb.c','SA.c']
    for name in ('verificar_bb','verificar_bb5','verificar_sa'):
        binary=out/name
        subprocess.run([*common,f'scripts/{name}.c','-o',str(binary),'-lm'],cwd=ROOT,check=True)
        cases=[[]] if name!='verificar_sa' else [
            [f'instances/benchmark/{size}/{inst}.txt',f'instances/drone_times/tau_kj_{inst}.txt']
            for inst,size in [('15-rc-c','small'),('50-r-e','large'),('200-r-e','large')]]
        for args in cases:
            result=subprocess.run([str(binary),*args],cwd=ROOT,capture_output=True,text=True)
            record=dict(test=name,arguments=args,returncode=result.returncode,
                        stdout=result.stdout,stderr=result.stderr)
            results.append(record)
            (out/'resultado.json').write_text(json.dumps(results,indent=2)+'\n')
            if result.returncode:
                raise RuntimeError(f'{name}: {result.stderr}')
            print(name,args,'ASan/UBSan OK',flush=True)


if __name__=='__main__':
    main()
