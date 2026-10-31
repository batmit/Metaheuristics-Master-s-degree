"""Modelo completo PDSTSP-c, sem fixacao de atribuicoes ou trechos de rota.

Coordenadas iguais sao agrupadas somente na rota do caminhao: com distancia
metrica e sem tempo de servico, visitas repetidas ao mesmo local podem ser
reunidas sem aumentar o custo. As decisoes de cada cliente continuam livres.
"""
from __future__ import annotations

import argparse
import ast
import hashlib
import json
import math
import os
import threading
import time
from fractions import Fraction as F
from pathlib import Path

from ortools.sat.python import cp_model


ROOT = Path(__file__).resolve().parent


def load_instance(name: str) -> dict:
    folder = "small" if int(name.split("-")[0]) <= 30 else "large"
    path = ROOT / f"instances/pdstsp-c/{folder}/{name}.txt"
    tau_path = path.with_name("tau_kj_" + path.name)
    lines = path.read_text().splitlines()
    params = {line.split(",")[0]: F(line.split(",")[1].strip()) for line in lines[:6]}
    coords = {int(j): (F(x), F(y)) for j, x, y, _ in (s.split() for s in lines[7:] if s.strip())}
    assert set(coords) == set(range(len(coords))) and params["TRUCK_SPEED"] > 0
    fleet = int(params["NUM_DRONES"])
    tau = {}
    for line in tau_path.read_text().splitlines()[1:]:
        v = line.split()
        if not v:
            continue
        assert len(v) == fleet + 1
        tau[int(v[0])] = {k: int(F(t)) for k, t in enumerate(v[1:], 1) if F(t) >= 0}
        assert all(F(t).denominator == 1 for t in v[1:] if F(t) >= 0)
    assert set(tau) == set(coords) - {0}
    dist = [[sum(abs(a-b) for a, b in zip(coords[i], coords[j])) * 60 / params["TRUCK_SPEED"]
             for j in coords] for i in coords]
    assert all(t.denominator == 1 for row in dist for t in row)
    quantum = 4 if params["TRUCK_SPEED"] == 30 and all(x.denominator == 1 for p in coords.values() for x in p) else 1
    return {"name": name, "coords": coords, "fleet": fleet, "tau": tau,
            "dist": [[int(t) for t in row] for row in dist], "quantum": quantum,
            "input_sha256": {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in [path, tau_path]}}


def nondominated(options: dict[int, int], horizon: int) -> dict[int, int]:
    return {k: t for k, t in options.items() if t <= horizon and not any(
        q <= k and duration <= t and (q < k or duration < t)
        for q, duration in options.items())}


def build_model(data: dict, horizon: int, hint: dict | None = None):
    assert type(horizon) is int and horizon >= 0
    coords, times, fleet = data["coords"], data["dist"], data["fleet"]
    clients = sorted(data["tau"])
    model = cp_model.CpModel()
    makespan = model.new_int_var(0, horizon, "makespan")
    truck_limit = data["quantum"] * (horizon // data["quantum"])
    z = {j: model.new_bool_var(f"truck_{j}") for j in clients}
    points = [coords[0]] + sorted(set(coords.values()) - {coords[0]})
    members = {p: [j for j in clients if coords[j] == p] for p in points}
    reps = [0] + [members[p][0] for p in points[1:]]
    visit = {i: model.new_bool_var(f"visit_{i}") for i in range(1, len(points))}
    for i in visit:
        model.add_max_equality(visit[i], [z[j] for j in members[points[i]]])
    idle = model.new_bool_var("truck_idle")
    model.add(sum(visit.values()) == 0).only_enforce_if(idle)
    model.add(sum(visit.values()) >= 1).only_enforce_if(idle.Not())
    arcs = [(0, 0, idle)] + [(i, i, v.Not()) for i, v in visit.items()]
    edges = {}
    route_cost = []
    arrival = {i: model.new_int_var(0, truck_limit, f"arrival_{i}") for i in range(1, len(points))}
    for i, var in arrival.items():
        model.add(var == 0).only_enforce_if(visit[i].Not())
        model.add(var >= times[0][reps[i]]).only_enforce_if(visit[i])
        model.add(var + times[reps[i]][0] <= truck_limit).only_enforce_if(visit[i])
    for i in range(len(points)):
        for j in range(len(points)):
            if i == j:
                continue
            distance = times[reps[i]][reps[j]]
            edge = model.new_bool_var(f"arc_{i}_{j}")
            edges[i, j] = edge
            arcs.append((i, j, edge))
            route_cost.append(distance * edge)
            if j:
                model.add(arrival[j] >= (arrival[i] if i else 0) + distance).only_enforce_if(edge)
            elif i:
                model.add(arrival[i] + distance <= truck_limit).only_enforce_if(edge)
    model.add_circuit(arcs)
    model.add(sum(route_cost) <= truck_limit)
    model.add(sum(route_cost) <= makespan)
    # Reversao da rota nao altera custo nem cronogramas dos drones.
    if all(times[i][j] == times[j][i] for i in range(len(times)) for j in range(len(times))):
        model.add(sum(j*v for (i, j), v in edges.items() if i == 0) <=
                  sum(i*v for (i, j), v in edges.items() if j == 0))
    starts, ends, quantity, duration_vars, areas = {}, {}, {}, {}, {}
    intervals, demands, min_area_terms = [], [], []
    option_count = 0
    for j in clients:
        options = nondominated(data["tau"][j], horizon)
        if not options:
            model.add(z[j] == 1)
            continue
        option_count += len(options)
        k = model.new_int_var(0, fleet, f"quantity_{j}")
        duration = model.new_int_var(0, max(options.values()), f"duration_{j}")
        area = model.new_int_var(0, max(q*t for q, t in options.items()), f"workload_{j}")
        model.add_allowed_assignments([k, duration, area], [(0, 0, 0)] + [(q, t, q*t) for q, t in options.items()])
        model.add(k == 0).only_enforce_if(z[j])
        model.add(k >= 1).only_enforce_if(z[j].Not())
        start = model.new_int_var(0, horizon, f"start_{j}")
        end = model.new_int_var(0, horizon, f"end_{j}")
        model.add(start == 0).only_enforce_if(z[j])
        model.add(end == 0).only_enforce_if(z[j])
        model.add(end <= makespan).only_enforce_if(z[j].Not())
        intervals.append(model.new_optional_interval_var(start, duration, end, z[j].Not(), f"mission_{j}"))
        demands.append(k)
        starts[j], ends[j], quantity[j], duration_vars[j], areas[j] = start, end, k, duration, area
        min_area_terms.append(min(q*t for q, t in options.items()) * (1-z[j]))
    if intervals:
        model.add_cumulative(intervals, demands, fleet)
    model.add(sum(areas.values()) <= fleet*makespan)
    model.add(sum(min_area_terms) <= fleet*makespan)
    model.minimize(makespan)
    variables = {"z": z, "visit": visit, "idle": idle, "edges": edges, "arrival": arrival,
                 "starts": starts, "ends": ends, "quantity": quantity, "duration": duration_vars,
                 "areas": areas, "makespan": makespan, "points": points, "members": members,
                 "reps": reps, "truck_limit": truck_limit, "option_count": option_count}
    if hint is not None:
        add_hint(model, variables, data, hint, horizon)
    assert not model.validate(), model.validate()
    return model, variables


def add_hint(model, variables, data, hint, horizon):
    z, points = variables["z"], variables["points"]
    route = [j for j in hint["route"] if j]
    old_missions = {m["client"]: m for m in hint["missions"]}
    point_idx = {p: i for i, p in enumerate(points)}
    seen = {0}
    cycle = [0]
    for j in route:
        idx = point_idx[data["coords"][j]]
        if idx not in seen:
            cycle.append(idx);seen.add(idx)
    # A rota reversa simetrica satisfaz a quebra de simetria.
    if len(cycle)>2 and cycle[1]>cycle[-1]:
        cycle = [0] + list(reversed(cycle[1:]))
    chosen = set(zip(cycle, cycle[1:]+[0])) if len(cycle)>1 else set()
    for key, var in variables["edges"].items():
        model.add_hint(var, int(key in chosen))
    model.add_hint(variables["idle"], int(len(cycle)==1))
    for i, var in variables["visit"].items():
        model.add_hint(var, int(i in seen))
    for j, var in z.items():
        model.add_hint(var, int(j not in old_missions))
    elapsed = 0
    previous = 0
    arrival = {i: 0 for i in variables["arrival"]}
    for idx in cycle[1:]:
        elapsed += data["dist"][variables["reps"][previous]][variables["reps"][idx]]
        arrival[idx] = elapsed
        previous = idx
    for i, var in variables["arrival"].items():
        if arrival[i] <= variables["truck_limit"]:
            model.add_hint(var, arrival[i])
    hinted_end = 0
    for j, k_var in variables["quantity"].items():
        mission = old_missions.get(j)
        if mission:
            old_k = mission["k"]
            # Dominancia permite substituir por demanda/duracao menores.
            options = nondominated(data["tau"][j], horizon)
            dominated_by = [(k,t) for k,t in options.items() if k<=old_k and t<=data["tau"][j][old_k]]
            if not dominated_by:
                continue
            k, duration = min(dominated_by,key=lambda pair:(pair[0]*pair[1],pair[0]))
            start = int(mission["start"]);end = start+duration
            if end>horizon:
                continue
        else:
            k=duration=start=end=0
        model.add_hint(k_var,k);model.add_hint(variables["duration"][j],duration)
        model.add_hint(variables["areas"][j],k*duration)
        model.add_hint(variables["starts"][j],start);model.add_hint(variables["ends"][j],end)
        hinted_end=max(hinted_end,end)
    truck_hint=elapsed+data["dist"][variables["reps"][previous]][0]
    objective=max(truck_hint,hinted_end)
    if objective<=horizon:
        model.add_hint(variables["makespan"],objective)


def extract(data, variables, value):
    selected = {j for j,z in variables["z"].items() if value(z)}
    route = sorted(j for j in selected if data["coords"][j]==variables["points"][0])
    if not value(variables["idle"]):
        current=0;seen={0}
        while True:
            nxt=next(j for (i,j),var in variables["edges"].items() if i==current and value(var))
            if nxt==0:break
            assert nxt not in seen;seen.add(nxt)
            route.extend(sorted(j for j in variables["members"][variables["points"][nxt]] if j in selected))
            current=nxt
    assert set(route)==selected
    missions=[]
    for j,k_var in variables["quantity"].items():
        k=value(k_var)
        if k:missions.append({'client':j,'k':k,'start':value(variables['starts'][j]),'end':value(variables['ends'][j])})
    missions.sort(key=lambda m:(m['start'],m['client']))
    truck=sum(data['dist'][a][b] for a,b in zip([0]+route,route+[0]))
    drone=max((m['end'] for m in missions),default=0)
    return {'instance':data['name'],'seed':0,'claimed':max(truck,drone),'truck':truck,'drone':drone,'route':route,'missions':missions}


def verifier():
    source=ast.parse((ROOT/'resultados/grupos_quatro_100/analisar.py').read_text())
    fn=next(n for n in source.body if isinstance(n,ast.FunctionDef) and n.name=='verify_instance')
    scope={'ROOT':ROOT,'F':F}
    exec(compile(ast.Module(body=[fn],type_ignores=[]),'verificador-independente','exec'),scope)
    return scope['verify_instance']


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--instance',default='165-r-c')
    parser.add_argument('--horizon',type=int,default=159)
    parser.add_argument('--seconds',type=float,default=3600)
    parser.add_argument('--workers',type=int,default=8)
    parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args()
    assert 0<args.seconds<=86400 and 1<=args.workers<=32
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    assert not (out/'manifesto.json').exists(), 'Usar outra pasta para preservar a busca anterior.'
    data=load_instance(args.instance)
    official=json.loads((ROOT/'resultados/revisao_literatura_2025/testemunhas_validadas.json').read_text())[args.instance]
    original={'seed':0,'route':[j for j in official['route'] if j],'missions':official['missions'],'truck':official['truck'],'drone':official['drone'],'claimed':official['makespan']}
    verify=verifier();verify(args.instance,[original])
    model,variables=build_model(data,args.horizon,official)
    # O exportador nativo falha silenciosamente com alguns caminhos Unicode
    # no Windows; a serializacao textual Python preserva o mesmo protobuf.
    (out/'modelo.pbtxt').write_text(str(model.proto),encoding='utf-8')
    (out/'cp_sat_completo.py').write_bytes(Path(__file__).read_bytes())
    import ortools
    manifest={'instance':args.instance,'horizon':args.horizon,'truck_limit':variables['truck_limit'],
              'seconds':args.seconds,'workers':args.workers,'ortools':ortools.__version__,
              'all_customer_assignments_free':True,'fixed_route_segments':False,'locations':len(variables['points']),
              'customers':len(data['tau']),'nondominated_drone_options':variables['option_count'],
              'source_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              'model_sha256':hashlib.sha256((out/'modelo.pbtxt').read_bytes()).hexdigest(),
              'input_sha256':data['input_sha256'],'official_witness_validated':True,
              'started':time.strftime('%Y-%m-%d %H:%M:%S %z')}
    (out/'manifesto.json').write_text(json.dumps(manifest,indent=2))
    solver=cp_model.CpSolver()
    solver.parameters.max_time_in_seconds=args.seconds
    solver.parameters.num_search_workers=args.workers
    solver.parameters.random_seed=20261028
    solver.parameters.log_search_progress=True
    solver.parameters.log_to_stdout=False
    lock=threading.Lock();state={'status':'SEARCHING','best_bound':None,'validated_solution':None};done=threading.Event();started=time.monotonic()
    logfile=(out/'solver.log').open('w',encoding='utf-8')
    def log(line):
        with lock:logfile.write(line+'\n');logfile.flush()
    solver.log_callback=log
    def bound(value):
        with lock:state['best_bound']=value
    solver.best_bound_callback=bound
    class Solution(cp_model.CpSolverSolutionCallback):
        def on_solution_callback(self):
            row=extract(data,variables,self.value)
            verify(args.instance,[row])
            assert row['claimed']<=args.horizon and row['truck']<=variables['truck_limit']
            (out/'solucao.json').write_text(json.dumps(row,indent=2))
            with lock:state['validated_solution']=row['claimed']
            print('SOLUCAO_VALIDADA '+json.dumps({'makespan':row['claimed'],'truck':row['truck'],'drone':row['drone'],'elapsed':time.monotonic()-started}),flush=True)
            self.stop_search()
    def heartbeat():
        while not done.wait(30):
            with lock:snapshot=dict(state,elapsed=time.monotonic()-started)
            (out/'progresso.json').write_text(json.dumps(snapshot,indent=2))
            print('PROGRESSO '+json.dumps(snapshot),flush=True)
    thread=threading.Thread(target=heartbeat,daemon=True);thread.start()
    print('MODELO_COMPLETO '+json.dumps(manifest),flush=True)
    try:
        status=solver.solve(model,Solution())
    finally:
        done.set();thread.join();logfile.close()
    solution=None
    if status in [cp_model.FEASIBLE,cp_model.OPTIMAL]:
        solution=extract(data,variables,solver.value);verify(args.instance,[solution])
        assert solution['claimed']<=args.horizon
        (out/'solucao.json').write_text(json.dumps(solution,indent=2))
    result={'status':solver.status_name(status),'seconds':solver.wall_time,'elapsed':time.monotonic()-started,
            'horizon':args.horizon,'truck_limit':variables['truck_limit'],'best_bound':solver.best_objective_bound,
            'solution':solution['claimed'] if solution else None,
            'global_infeasibility_under_horizon':status==cp_model.INFEASIBLE,
            'inputs_unchanged':all(hashlib.sha256((ROOT/p).read_bytes()).hexdigest()==h for p,h in data['input_sha256'].items()),
            'response_stats':solver.response_stats()}
    assert result['inputs_unchanged']
    (out/'resultado.json').write_text(json.dumps(result,indent=2))
    report=f"# CP-SAT completo — {args.instance}, alvo {args.horizon}\n\n"
    report+=f"Status: **{result['status']}**. Tempo de solver: {result['seconds']:.3f} s. Limite de busca: {args.seconds:g} s; {args.workers} workers.\n\n"
    report+='Todas as atribuições elegíveis, rota, quantidades e horários foram decisões do modelo. Não foram fixados clientes a partir de soluções anteriores nem trechos da rota. Clientes fisicamente inelegíveis permanecem obrigatórios no caminhão.\n\n'
    if solution:report+=f"Foi encontrada e validada independentemente uma solução com makespan **{solution['claimed']}**, caminhão {solution['truck']} e drones {solution['drone']}.\n"
    elif status==cp_model.INFEASIBLE:report+=f"O modelo completo não admite solução de até {args.horizon}. Para estes dados inteiros, uma testemunha válida de {args.horizon+1} demonstra otimalidade desse valor quando disponível. A testemunha oficial validada vale {official['makespan']}.\n"
    else:report+='Não foi encontrada uma solução dentro do alvo nem foi demonstrada sua impossibilidade. O encerramento pelo prazo mantém a questão aberta.\n'
    report+=f"\nLimite inferior reportado pelo solver: {result['best_bound']:g}. Interpretar junto ao status; não é uma solução.\n"
    report+='\nLocais iguais são agregados somente no circuito do caminhão; sem serviço e com distância métrica, reunir visitas não aumenta custo. Cada cliente mantém sua decisão de recurso. Quantidades dominadas por outra de demanda e duração menores são eliminadas por dominância válida.\n'
    report+='\nO modelo exportado, código congelado, hashes das entradas, log e resposta completa estão nesta pasta. A testemunha oficial foi validada antes da busca; novas soluções usam verificação independente com frações exatas.\n'
    (out/'relatorio.md').write_text(report,encoding='utf-8')
    print('RESULTADO_FINAL '+json.dumps(result),flush=True)


if __name__=='__main__':main()
