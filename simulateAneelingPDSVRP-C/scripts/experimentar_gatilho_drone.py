"""Paired isolated test of five unsuccessful drone-change attempts as BB trigger."""
import concurrent.futures
import csv
import hashlib
import json
import math
import os
from pathlib import Path
import statistics
import subprocess
import threading
import time
from executar_benchmark import ROOT, catalogo, dados_instancia, validar

OUT=ROOT/'output/experimento_gatilho_drone'
SECONDS=2
REPETITIONS=3
WORKERS=6

HELPER='''
/* Count attempted changes, including rejected candidates, against the current
 * state's drone completion time. Truck-only changes do not affect the count.
 * This is observed local stagnation, not a proof of scheduling optimality. */
static int atualizarEstagnacaoDrone(int contador, const Solucao *atual,
                                  const Solucao *vizinho, TipoMovimento tipo)
{
    bool mudou = tipo == ALTERAR_QUANTIDADE_DRONES ||
                 atual->numMissoesDrone != vizinho->numMissoesDrone;
    if (!mudou) return contador;
    return vizinho->tempoDrones < atual->tempoDrones ? 0 : contador + 1;
}

'''


def prepare():
    OUT.mkdir(parents=True,exist_ok=True)
    baseline=(ROOT/'SA.c').read_bytes()
    (OUT/'SA_original.c').write_bytes(baseline)
    code=baseline.decode('utf-8').replace('\r\n','\n')
    marker='/* Mantem estado atual e melhor separado; resfria e refina o melhor por BB. */'
    code=code.replace(marker,HELPER+marker)
    old='''            ++semMelhora;
            int resultado = gerarVizinhoAvaliado(inst, &atual, &vizinho, NULL);'''
    new='''            TipoMovimento tipo;
            int resultado = gerarVizinhoAvaliado(inst, &atual, &vizinho, &tipo);'''
    if old not in code: raise ValueError('Unexpected SA source')
    code=code.replace(old,new)
    code=code.replace('            if (!resultado) goto publicar;','''            if (!resultado) goto publicar;
            semMelhora = atualizarEstagnacaoDrone(semMelhora, &atual, &vizinho, tipo);''')
    code=code.replace('                    semMelhora = 0;\n','')
    (OUT/'SA_drone.c').write_text(code)
    main=(ROOT/'main.c').read_text()
    main=main.replace('    parametros.registrarMelhor = registrarMelhor;',
        '    EstatisticasSA stats = {0};\n    parametros.estatisticas = &stats;\n    parametros.registrarMelhor = registrarMelhor;')
    main=main.replace('    status = 0;','''    registrarMelhor(&solucao, NULL);
    printf("STATS {\\"bb_calls\\":%llu,\\"bb_nodes\\":%llu,\\"bb_improvements\\":%llu,\\"iterations\\":%llu}\\n",
           stats.chamadasBB, stats.nosBB, stats.melhoriasBB, stats.tentativas);
    status = 0;''')
    (OUT/'main_stats.c').write_text(main)
    for version in ('original','drone'):
        subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-I',str(ROOT),
            str(OUT/'main_stats.c'),'lerInstancias.c','cv.c','drone_bb.c',str(OUT/f'SA_{version}.c'),
            '-o',str(OUT/f'{version}.exe'),'-lm'],cwd=ROOT,check=True)
    test='''#include "SA_drone.c"
#include <assert.h>
#include <stdio.h>
int main(void) {
    Solucao a={0}, b={0};
    a.numMissoesDrone=b.numMissoesDrone=2;
    a.tempoDrones=b.tempoDrones=30;
    int count=0;
    for(int i=0;i<50;++i) count=atualizarEstagnacaoDrone(count,&a,&b,DOIS_OPT_CAMINHAO);
    assert(count==0);
    for(int i=0;i<5;++i) count=atualizarEstagnacaoDrone(count,&a,&b,ALTERAR_QUANTIDADE_DRONES);
    assert(count==5);
    b.tempoDrones=29;
    assert(atualizarEstagnacaoDrone(count,&a,&b,ALTERAR_QUANTIDADE_DRONES)==0);
    b.tempoDrones=31;
    assert(atualizarEstagnacaoDrone(2,&a,&b,TRANSFERIR_CLIENTE)==2);
    b.numMissoesDrone=3;
    assert(atualizarEstagnacaoDrone(2,&a,&b,TRANSFERIR_CLIENTE)==3);
    b.numMissoesDrone=0; b.tempoDrones=0;
    assert(atualizarEstagnacaoDrone(2,&a,&b,TRANSFERIR_CLIENTE)==0);
    puts("Trigger tests OK");
    return 0;
}
'''
    (OUT/'test_trigger.c').write_text(test)
    subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-I',str(ROOT),
        str(OUT/'test_trigger.c'),'lerInstancias.c','cv.c','drone_bb.c','-o',str(OUT/'test_trigger.exe'),'-lm'],
        cwd=ROOT,check=True)
    subprocess.run([str(OUT/'test_trigger.exe')],check=True)
    protocol=dict(seconds=SECONDS,repetitions=REPETITIONS,workers=WORKERS,
        trigger='5 attempted mission/formation changes without reduction versus current drone time; truck-only ignored',
        decision='Keep only if mean relative global change < 0, mean drone change <= 0, and one-sided paired sign test across configuration means p < .05',
        baseline_sha256=hashlib.sha256(baseline).hexdigest())
    (OUT/'protocol.json').write_text(json.dumps(protocol,indent=2)+'\n')
    return baseline


def run(row,rep,version):
    name,s=row['instance'],int(row['trucks'])
    seed=42+int(name.split('-')[0])*10+s+rep*1000003
    size='small' if int(name.split('-')[0])<=30 else 'large'
    command=[str(OUT/f'{version}.exe'),str(ROOT/f'instances/benchmark/{size}/{name}.txt'),
             str(ROOT/f'instances/drone_times/tau_kj_{name}.txt'),str(s),str(seed)]
    best=None;stats=None;count=0;timed=threading.Event();start=time.perf_counter()
    with subprocess.Popen(command,env=dict(os.environ,PDSVRP_WORKER='1'),stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE,text=True) as p:
        def stop():
            if p.poll() is None: timed.set();p.kill()
        timer=threading.Timer(SECONDS,stop);timer.start()
        try:
            for line in p.stdout:
                if line.startswith('CHECKPOINT '):
                    cert=json.loads(line[11:]);validar(cert,name,s);count+=1
                    if best is None or cert['makespan']<=best['makespan']: best=cert
                elif line.startswith('STATS '): stats=json.loads(line[6:])
            p.wait();error=p.stderr.read()
        finally:
            timer.cancel();timer.join()
            if p.poll() is None: p.kill()
            p.wait()
    if best is None or (p.returncode and not timed.is_set()): raise RuntimeError(error)
    return dict(instance=name,trucks=s,seed=seed,repetition=rep,version=version,best=best,
        drone_time=max((m['end'] for m in best['missions']),default=0),
        wall_seconds=time.perf_counter()-start,completed=not timed.is_set(),stats=stats,checkpoints=count)


def pair(task):
    row,rep=task
    order=('original','drone') if (rep+int(row['trucks']))%2==0 else ('drone','original')
    return [run(row,rep,v) for v in order]


def main():
    baseline=prepare();rows=catalogo()
    for r in rows: dados_instancia(r['instance'])
    runs=[];start=time.perf_counter()
    tasks=[(r,rep) for r in rows for rep in range(REPETITIONS)]
    with (OUT/'runs.jsonl').open('w') as f, concurrent.futures.ThreadPoolExecutor(max_workers=WORKERS) as pool:
        futures=[pool.submit(pair,t) for t in tasks]
        for future in concurrent.futures.as_completed(futures):
            for r in future.result():
                runs.append(r);f.write(json.dumps(r)+'\n')
            f.flush()
            if len(runs)%60==0: print(f'{len(runs)}/1080 runs; {time.perf_counter()-start:.1f}s',flush=True)
    comparisons=[];paired=[]
    for row in rows:
        name,s=row['instance'],int(row['trucks'])
        subset=[r for r in runs if r['instance']==name and r['trucks']==s]
        for rep in range(REPETITIONS):
            a=next(r for r in subset if r['repetition']==rep and r['version']=='original')
            b=next(r for r in subset if r['repetition']==rep and r['version']=='drone')
            paired.append(dict(instance=name,trucks=s,seed=a['seed'],original=a['best']['makespan'],
                drone=b['best']['makespan'],original_drone_time=a['drone_time'],new_drone_time=b['drone_time'],
                both_completed=a['completed'] and b['completed'],
                original_seconds=a['wall_seconds'],new_seconds=b['wall_seconds']))
        g=paired[-REPETITIONS:]
        a=statistics.mean(r['original'] for r in g);b=statistics.mean(r['drone'] for r in g)
        comparisons.append(dict(instance=name,trucks=s,original_mean=a,new_mean=b,
            change_pct=100*(b-a)/a,original_best=min(r['original'] for r in g),new_best=min(r['drone'] for r in g),
            original_drone_mean=statistics.mean(r['original_drone_time'] for r in g),
            new_drone_mean=statistics.mean(r['new_drone_time'] for r in g)))
    for filename,data in [('comparison.csv',comparisons),('paired.csv',paired)]:
        with (OUT/filename).open('w',newline='') as f:
            w=csv.DictWriter(f,fieldnames=list(data[0]));w.writeheader();w.writerows(data)
    wins=sum(r['new_mean']<r['original_mean']-1e-7 for r in comparisons)
    losses=sum(r['new_mean']>r['original_mean']+1e-7 for r in comparisons)
    n=wins+losses
    pvalue=sum(math.comb(n,k) for k in range(wins,n+1))/2**n if n else 1
    change=statistics.mean(r['change_pct'] for r in comparisons)
    drone_change=statistics.mean(r['new_drone_mean']-r['original_drone_mean'] for r in comparisons)
    keep=change<0 and drone_change<=0 and pvalue<.05
    summary=dict(configurations=len(rows),runs=len(runs),wins=wins,losses=losses,ties=len(rows)-n,
        mean_global_change_pct=change,mean_drone_change_minutes=drone_change,sign_test_one_sided=pvalue,
        keep_candidate=keep,completed={v:sum(r['completed'] for r in runs if r['version']==v) for v in ('original','drone')},
        paired_wins=sum(r['drone']<r['original']-1e-7 for r in paired),
        paired_losses=sum(r['drone']>r['original']+1e-7 for r in paired),
        checkpoints=sum(r['checkpoints'] for r in runs),wall_seconds=time.perf_counter()-start)
    (OUT/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
    if (ROOT/'SA.c').read_bytes()!=baseline: raise ValueError('Production SA changed during experiment')
    print(json.dumps(summary,indent=2),flush=True)

if __name__=='__main__': main()
