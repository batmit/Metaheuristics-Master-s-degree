"""Instrumentacao isolada; nao altera os fontes nem resultados do algoritmo principal."""
import csv
import json
from pathlib import Path
import re
import statistics
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT/'diagnostics/performance'


def analisar_rotas():
    rows = list(csv.DictReader((ROOT/'results/latest/comparison.csv').open()))
    (OUT/'comparison_snapshot.csv').write_text((ROOT/'results/latest/comparison.csv').read_text())
    results = []
    for row in rows:
        if row['proven'] != '0' or not row['found']: continue
        name, trucks = row['instance'], int(row['trucks'])
        cert = json.loads((ROOT/f'results/latest/solutions/{name}_s{trucks}.json').read_text())
        # O lote pode estar sendo atualizado enquanto diagnosticamos.
        if abs(cert['makespan']-float(row['found'])) > 1e-6: continue
        lines = (ROOT/f'instances/benchmark/large/{name}.txt').read_text().splitlines()
        nodes = [[float(v) for v in line.split()] for line in lines[7:] if line.strip()]
        speed = float(lines[1].split(',')[1])
        dist = [[(abs(a[1]-b[1])+abs(a[2]-b[2]))/speed*60 for b in nodes] for a in nodes]
        def custo(route):
            path = [0, *route, 0]
            return sum(dist[a][b] for a,b in zip(path, path[1:]))
        routes = cert['routes']
        before = [custo(r) for r in routes]
        improved = []
        for route in routes:
            r = list(route)
            # Somente diagnostico: melhor 2-opt ate minimo local, SEM mudar
            # atribuicao de clientes, drones, quantidades ou demais rotas.
            while True:
                delta, pair = 0, None
                for i in range(len(r)):
                    for j in range(i+1, len(r)):
                        a = r[i-1] if i else 0
                        b = r[j+1] if j+1 < len(r) else 0
                        d = dist[a][r[j]]+dist[r[i]][b]-dist[a][r[i]]-dist[r[j]][b]
                        if d < delta-1e-9: delta, pair = d, (i,j)
                if pair is None: break
                i,j = pair
                r[i:j+1] = reversed(r[i:j+1])
            improved.append(custo(r))
        td = max([m['end'] for m in cert['missions']]+[0])
        after = max(max(improved), td)
        results.append(dict(instance=name, trucks=trucks, found=cert['makespan'],
                            literature=float(row['ub']), trucks_time=max(before), drones_time=td,
                            drone_missions=len(cert['missions']),
                            makespan_after_truck_2opt=after,
                            reduction_percent=100*(cert['makespan']-after)/cert['makespan']))
    (OUT/'route_analysis.json').write_text(json.dumps(results, indent=2)+'\n')
    print(json.dumps(dict(snapshot_cases=len(results),
                          truck_dominant=sum(r['trucks_time']>r['drones_time']+1e-7 for r in results),
                          median_route_2opt_reduction_percent=statistics.median(r['reduction_percent'] for r in results),
                          examples=results[:4]), indent=2))


def instrumentar():
    timer = '''
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
static double diagAgora(void) { LARGE_INTEGER t,f; QueryPerformanceCounter(&t); QueryPerformanceFrequency(&f); return (double)t.QuadPart/f.QuadPart; }
#else
#include <sys/time.h>
static double diagAgora(void) { struct timeval t; gettimeofday(&t, NULL); return t.tv_sec+t.tv_usec/1e6; }
#endif
'''
    bb = (ROOT/'drone_bb.c').read_text(encoding='utf-8-sig')
    bb = timer + bb
    bb = bb.replace('int escalonarMissoesDroneBB(', 'static int escalonarInternoDiagnostico(')
    bb += '''
int escalonarMissoesDroneBB(const Instancia *inst, Solucao *s, ResultadoBB *resultado) {
    ResultadoBB r = {0}; double t = diagAgora();
    fprintf(stderr, "BB_BEGIN n=%d t=%.9f\\n", s->numMissoesDrone, t); fflush(stderr);
    int ok = escalonarInternoDiagnostico(inst,s,&r);
    fprintf(stderr, "BB_END n=%d sec=%.9f nodes=%llu lb=%.9f ub=%.9f\\n", s->numMissoesDrone, diagAgora()-t, r.nos, r.limiteInferior,s->tempoDrones); fflush(stderr);
    if(resultado) *resultado=r;
    return ok;
}
'''
    (OUT/'drone_bb_diag.c').write_text(bb, encoding='utf-8')
    sa = (ROOT/'SA.c').read_text()
    sa = timer + sa
    sa = sa.replace('for (double t = p->tempInicial; t > p->tempFinal;)',
                    'unsigned long long diagIter = 0; for (double t = p->tempInicial; t > p->tempFinal;)')
    sa = sa.replace('for (int w = 0; w < p->saMax; ++w) {',
                    'fprintf(stderr,"SA_LEVEL t=%.9f iter=%llu time=%.9f\\n",t,diagIter,diagAgora()); fflush(stderr); for (int w = 0; w < p->saMax; ++w) { ++diagIter; fprintf(stderr,"SA_ITER %llu\\n",diagIter); fflush(stderr);')
    (OUT/'SA_diag.c').write_text(sa)
    subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2',
                    '-I',str(ROOT),str(ROOT/'main.c'),str(ROOT/'lerInstancias.c'),str(ROOT/'cv.c'),
                    str(OUT/'drone_bb_diag.c'),str(OUT/'SA_diag.c'),'-o',str(OUT/'diagnostico.exe'),'-lm'], check=True)
    started = time.monotonic()
    with (OUT/'profile.log').open('w') as stream:
        try:
            subprocess.run([str(OUT/'diagnostico.exe'),'instances/benchmark/large/50-r-e.txt',
                            'instances/drone_times/tau_kj_50-r-e.txt','2','544'],cwd=ROOT,
                           stdout=stream,stderr=subprocess.STDOUT,timeout=15,check=True)
        except subprocess.TimeoutExpired:
            pass
    log = (OUT/'profile.log').read_text()
    ends = [dict(n=int(m[0]),seconds=float(m[1]),nodes=int(m[2]),lb=float(m[3]),ub=float(m[4]))
            for m in re.findall(r'BB_END n=(\d+) sec=([\d.]+) nodes=(\d+) lb=([\d.]+) ub=([\d.]+)',log)]
    levels = re.findall(r'SA_LEVEL t=([\d.]+) iter=(\d+)',log)
    iters = re.findall(r'SA_ITER (\d+)',log)
    last_begin = re.findall(r'BB_BEGIN n=(\d+)',log)
    result = dict(wall_seconds=time.monotonic()-started,bb_calls_started=len(last_begin),
                  bb_calls_completed=len(ends),bb_completed_seconds=sum(e['seconds'] for e in ends),
                  sa_iterations_started=int(iters[-1]) if iters else 0,
                  temperature_levels=len(levels),temperatures=levels,
                  last_bb_size=int(last_begin[-1]) if last_begin else 0,
                  slowest_completed=sorted(ends,key=lambda e:e['seconds'],reverse=True)[:5])
    (OUT/'profile_summary.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))


if __name__ == '__main__':
    OUT.mkdir(parents=True,exist_ok=True)
    analisar_rotas()
    instrumentar()
