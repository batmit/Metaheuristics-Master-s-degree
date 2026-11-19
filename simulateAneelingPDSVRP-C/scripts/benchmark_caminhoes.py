"""Independent OR-Tools GLS baseline: mandatory customers, min-max truck routes.

No previous solution is supplied. Integer edge times are required explicitly;
the lexicographic objective is W*makespan + sum(route times), with a proven
upper bound on the secondary term. Outputs never update results/best.json.
"""
import argparse
from concurrent.futures import ProcessPoolExecutor, as_completed
import csv
from decimal import Decimal
import hashlib
import json
import platform
import statistics
import subprocess
import time

import ortools
from ortools.constraint_solver import pywrapcp, routing_enums_pb2
from executar_benchmark import ROOT, catalogo, validar

STRATEGIES = ['PARALLEL_CHEAPEST_INSERTION', 'GLOBAL_CHEAPEST_ARC', 'PATH_CHEAPEST_ARC']


def read_matrix(name):
    size = 'small' if int(name.split('-')[0]) <= 30 else 'large'
    path = ROOT / f'instances/benchmark/{size}/{name}.txt'
    lines = path.read_text().splitlines()
    speed = Decimal(lines[1].split(',')[1])
    nodes = [tuple(Decimal(v) for v in line.split()[1:3]) for line in lines[7:] if line.strip()]
    matrix = []
    for a in nodes:
        row = []
        for b in nodes:
            t = (abs(a[0]-b[0])+abs(a[1]-b[1]))*60/speed
            if t != t.to_integral_value():
                raise ValueError(f'Noninteger edge: {name}: {t}; do not round silently')
            row.append(int(t))
        matrix.append(row)
    return path, matrix


def solve(job):
    name, trucks, strategy, seconds = job
    path, matrix = read_matrix(name)
    n = len(matrix)
    manager = pywrapcp.RoutingIndexManager(n, trucks, 0)
    routing = pywrapcp.RoutingModel(manager)
    transit = routing.RegisterTransitMatrix(matrix)
    routing.SetArcCostEvaluatorOfAllVehicles(transit)
    total_bound = (n-1+trucks)*max(map(max, matrix))
    weight = total_bound+1
    routing.AddDimension(transit, 0, total_bound, True, 'Time')
    routing.GetDimensionOrDie('Time').SetGlobalSpanCostCoefficient(weight)
    params = pywrapcp.DefaultRoutingSearchParameters()
    params.first_solution_strategy = getattr(routing_enums_pb2.FirstSolutionStrategy, strategy)
    params.local_search_metaheuristic = routing_enums_pb2.LocalSearchMetaheuristic.GUIDED_LOCAL_SEARCH
    params.time_limit.FromMilliseconds(round(seconds*1000))
    start = time.perf_counter()
    solution = routing.SolveWithParameters(params)
    elapsed = time.perf_counter()-start
    if solution is None:
        return dict(instance=name, trucks=trucks, strategy=strategy, elapsed=elapsed,
                    status=routing.status(), certificate=None)
    routes = []
    for vehicle in range(trucks):
        route = []
        index = solution.Value(routing.NextVar(routing.Start(vehicle)))
        while not routing.IsEnd(index):
            route.append(manager.IndexToNode(index))
            index = solution.Value(routing.NextVar(index))
        routes.append(route)
    durations = [sum(matrix[a][b] for a,b in zip([0]+r, r+[0])) for r in routes]
    cert = dict(routes=routes, missions=[], makespan=max(durations))
    validar(cert, name, trucks)
    expected = weight*max(durations)+sum(durations)
    if solution.ObjectiveValue() != expected:
        raise ValueError('Objective mismatch')
    return dict(instance=name, trucks=trucks, strategy=strategy, elapsed=elapsed,
                status=routing.status(), certificate=cert, durations=durations,
                lexicographic_weight=weight, objective=expected,
                input_sha256=hashlib.sha256(path.read_bytes()).hexdigest())


def report(out, rows, runs, seconds, workers, wall):
    binary = out/'auditar.exe'
    subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2',
                    'lerInstancias.c','cv.c','drone_bb.c','SA.c','scripts/auditar_certificados.c',
                    '-o',str(binary),'-lm'], cwd=ROOT, check=True)
    old = json.loads((ROOT/'results/best.json').read_text())
    previous14 = {(r['instance'],int(r['trucks'])) for r in csv.DictReader(
        (ROOT/'output/auditoria_professor/melhorias.csv').open())}
    summaries=[]
    for row in rows:
        name, trucks = row['instance'], int(row['trucks'])
        group = [r for r in runs if r['instance']==name and r['trucks']==trucks and r['certificate']]
        if not group:
            raise ValueError(f'No solution for {name}/{trucks}')
        best = min(group, key=lambda r:(r['certificate']['makespan'],sum(r['durations'])))
        cert = best['certificate']
        path,matrix = read_matrix(name)
        flat=[j for route in cert['routes'] for j in route]
        if sorted(flat)!=list(range(1,len(matrix))) or cert['missions']:
            raise ValueError('Coverage mismatch')
        payload='\n'.join(str(len(r))+' '+' '.join(map(str,r)) for r in cert['routes'])
        payload+='\n0\n'+str(cert['makespan'])+'\n'
        checked=subprocess.run([str(binary),str(path),str(ROOT/f'instances/drone_times/tau_kj_{name}.txt'),str(trucks)],
                               input=payload,capture_output=True,text=True,check=True)
        if Decimal(checked.stdout.strip()) != cert['makespan']:
            raise ValueError('C validation mismatch')
        ub=float(row['ub'])
        mono=min(float(r['ub']) for r in rows if r['instance']==name and int(r['trucks'])<=trucks)
        value=cert['makespan']
        key=f'{name}_s{trucks}'
        oldbest=old.get(key,{}).get('best')
        previous=oldbest['makespan'] if oldbest else None
        summary=dict(instance=name,trucks=trucks,source=row['source'],published_ub=ub,
            monotone_ub=mono,best=value,mean=statistics.mean(r['certificate']['makespan'] for r in group),
            worst=max(r['certificate']['makespan'] for r in group),
            improvement_pct=100*(mono-value)/mono,
            outcome='better' if value<mono else 'equal' if value==mono else 'worse',
            previous_local=previous,previous_14=(name,trucks) in previous14,
            best_strategy=best['strategy'],successful_runs=len(group),c_valid=True,python_valid=True,
            decimal_valid=True,certificate=f'{key}.json')
        summaries.append(summary)
        (out/f'{key}.json').write_text(json.dumps(best,indent=2)+'\n')
    with (out/'comparison.csv').open('w',newline='') as f:
        writer=csv.DictWriter(f,fieldnames=list(summaries[0]));writer.writeheader();writer.writerows(summaries)
    counts={size:{outcome:sum(r['outcome']==outcome for r in summaries if
        (int(r['instance'].split('-')[0])<=30)==(size=='small')) for outcome in ['better','equal','worse']}
        for size in ['small','large']}
    meta=dict(algorithm='OR-Tools Routing Guided Local Search',version=ortools.__version__,
        python=platform.python_version(),platform=platform.platform(),seconds_per_run=seconds,
        strategies=STRATEGIES,workers=workers,wall_seconds=wall,runs=len(runs),counts=counts,
        starts='independent; no previous routes supplied',objective='lexicographic makespan, total duration',
        distance='Manhattan',units='minutes; exact integer edges; no rounding',
        previous14_beaten=sum(r['previous_14'] and r['outcome']=='better' for r in summaries),
        new_configurations=sum(not r['previous_14'] and r['outcome']=='better' for r in summaries))
    (out/'summary.json').write_text(json.dumps(meta,indent=2)+'\n')
    print(json.dumps(meta,indent=2),flush=True)


def main():
    p=argparse.ArgumentParser()
    p.add_argument('--seconds',type=float,default=5)
    p.add_argument('--workers',type=int,default=6)
    p.add_argument('--limit',type=int,default=0)
    p.add_argument('--out',default='output/benchmark_caminhoes_gls')
    args=p.parse_args()
    out=ROOT/args.out;out.mkdir(parents=True,exist_ok=True)
    rows=catalogo()
    if args.limit: rows=rows[:args.limit]
    jobs=[(r['instance'],int(r['trucks']),s,args.seconds) for r in rows for s in STRATEGIES]
    runs=[];start=time.perf_counter()
    with (out/'runs.jsonl').open('w') as stream, ProcessPoolExecutor(max_workers=args.workers) as pool:
        futures=[pool.submit(solve,j) for j in jobs]
        for i,future in enumerate(as_completed(futures),1):
            result=future.result();runs.append(result)
            stream.write(json.dumps(result)+'\n');stream.flush()
            if i%30==0 or i==len(jobs):
                print(f'{i}/{len(jobs)} runs; elapsed {time.perf_counter()-start:.1f}s',flush=True)
    report(out,rows,runs,args.seconds,args.workers,time.perf_counter()-start)


if __name__=='__main__':
    main()
