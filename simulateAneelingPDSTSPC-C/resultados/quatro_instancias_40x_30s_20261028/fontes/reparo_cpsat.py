"""Reparo de viabilidade CP-SAT chamado pelo C, sem alterar arquivos de entrada.

Preserva atribuicoes fora da vizinhanca. Blocos consecutivos do caminhao
fixados externamente sao contraidos; sua ordem entre blocos continua livre.
INFEASIBLE certifica somente esta vizinhanca, nunca a instancia inteira.
"""
from __future__ import annotations

import json
import math
import sys
import time
from pathlib import Path


def validate(data: dict) -> None:
    n, fleet = data["n"], data["fleet"]
    if data.get("version") != 1 or not isinstance(n, int) or not 1 <= n <= 1000:
        raise ValueError("dimensao/protocolo invalido")
    if not isinstance(fleet, int) or not 0 <= fleet <= 100:
        raise ValueError("frota invalida")
    for field in ("horizon", "truck_limit"):
        if type(data[field]) is not int or not 0 <= data[field] <= 10**7:
            raise ValueError("horizonte deve ser inteiro nao negativo")
    if not math.isfinite(data["seconds"]) or not 0 < data["seconds"] <= 3600:
        raise ValueError("prazo invalido")
    truck, drone = data["truck_times"], data["drone_times"]
    if len(truck) != n or len(drone) != n:
        raise ValueError("matriz invalida")
    for row in truck:
        if len(row) != n or any(type(x) is not int or not 0 <= x <= 10**7 for x in row):
            raise ValueError("tempos do caminhao invalidos")
    for row in drone:
        if len(row) != fleet or any(x is not None and (type(x) is not int or not 0 <= x <= 10**7) for x in row):
            raise ValueError("tempos dos drones invalidos")
    clients = data["route"] + [m[0] for m in data["missions"]]
    if len(clients) != n - 1 or set(clients) != set(range(1, n)):
        raise ValueError("cobertura invalida")
    free = data["free"]
    if len(free) > 64 or len(set(free)) != len(free) or any(type(j) is not int or not 0 < j < n for j in free):
        raise ValueError("vizinhanca invalida")


def solve(data: dict, started: float | None = None) -> dict:
    started = time.monotonic() if started is None else started
    validate(data)
    from ortools.sat.python import cp_model

    model = cp_model.CpModel()
    n, fleet = data["n"], data["fleet"]
    horizon, limit = data["horizon"], data["truck_limit"]
    free = set(data["free"])
    route = data["route"]
    times = data["truck_times"]
    old_drone = {m[0] for m in data["missions"]}
    # Cada bloco mantem somente suas arestas internas. Blocos e clientes
    # livres podem ser reordenados; isso nao equivale a fixar toda a rota.
    blocks = [[0]]
    run: list[int] = []
    for j in route:
        if j in free:
            if run:
                blocks.append(run)
                run = []
        else:
            run.append(j)
    if run:
        blocks.append(run)
    mandatory_nodes = len(blocks)
    node = {}
    z = {}
    for j in sorted(free):
        node[j] = len(blocks)
        blocks.append([j])
        z[j] = model.new_bool_var(f"truck_{j}")
        model.add_hint(z[j], int(j not in old_drone))
    arcs = []
    arc_vars = {}
    for j, idx in node.items():
        arcs.append((idx, idx, z[j].Not()))
    idle = model.new_bool_var("truck_idle")
    arcs.append((0, 0, idle))
    if mandatory_nodes > 1:
        model.add(idle == 0)
    elif z:
        model.add(sum(z.values()) == 0).only_enforce_if(idle)
        model.add(sum(z.values()) >= 1).only_enforce_if(idle.Not())
    else:
        model.add(idle == 1)
    travel = []
    for a, source in enumerate(blocks):
        for b, dest in enumerate(blocks):
            if a == b:
                continue
            var = model.new_bool_var(f"arc_{a}_{b}")
            arc_vars[a, b] = var
            arcs.append((a, b, var))
            travel.append(times[source[-1]][dest[0]] * var)
    model.add_circuit(arcs)
    internal = sum(times[a][b] for block in blocks for a, b in zip(block, block[1:]))
    model.add(sum(travel) + internal <= limit)
    original_nodes = [0]
    block_for_client = {j: i for i, block in enumerate(blocks) for j in block}
    for j in route:
        idx = block_for_client[j]
        if idx != original_nodes[-1]:
            original_nodes.append(idx)
    for a, b in zip(original_nodes, original_nodes[1:] + [0]):
        if (a, b) in arc_vars:
            model.add_hint(arc_vars[a, b], 1)

    intervals, demands, workload = [], [], []
    choices = {}
    starts = {}
    ends = {}
    for j in range(1, n):
        if j not in free and j not in old_drone:
            continue
        start = model.new_int_var(0, horizon, f"start_{j}")
        end = model.new_int_var(0, horizon, f"end_{j}")
        starts[j], ends[j] = start, end
        options = []
        for k, duration in enumerate(data["drone_times"][j], 1):
            if duration is None or duration > horizon:
                continue
            present = model.new_bool_var(f"drone_{j}_{k}")
            choices[j, k] = present
            options.append(present)
            intervals.append(model.new_optional_interval_var(start, duration, end, present, f"mission_{j}_{k}"))
            demands.append(k)
            workload.append(k * duration * present)
        if j in free:
            model.add(sum(options) + z[j] == 1)
            model.add(start == 0).only_enforce_if(z[j])
            model.add(end == 0).only_enforce_if(z[j])
        else:
            model.add(sum(options) == 1)
    if intervals:
        model.add_cumulative(intervals, demands, fleet)
    model.add(sum(workload) <= fleet * horizon)
    remaining = data["seconds"] - (time.monotonic() - started) - 0.02
    if remaining <= 0:
        return {"status": "UNKNOWN"}
    solver = cp_model.CpSolver()
    solver.parameters.max_time_in_seconds = remaining
    solver.parameters.num_search_workers = 1
    solver.parameters.random_seed = int(data.get("seed", 0)) % (2**31 - 1)
    status = solver.solve(model)
    if status == cp_model.INFEASIBLE:
        return {"status": "INFEASIBLE"}
    if status not in (cp_model.FEASIBLE, cp_model.OPTIMAL):
        if status == cp_model.MODEL_INVALID:
            raise ValueError(solver.solution_info())
        return {"status": "UNKNOWN"}
    output_route = []
    if not solver.value(idle):
        current = 0
        seen = {0}
        while True:
            nxt = next(b for (a, b), var in arc_vars.items() if a == current and solver.value(var))
            if nxt == 0:
                break
            if nxt in seen:
                raise ValueError("circuito invalido")
            seen.add(nxt)
            output_route.extend(blocks[nxt])
            current = nxt
    missions = []
    for (j, k), present in choices.items():
        if solver.value(present):
            missions.append([j, k, solver.value(starts[j]), solver.value(ends[j])])
    missions.sort(key=lambda m: (m[2], m[0]))
    return {"status": "FEASIBLE", "route": output_route, "missions": missions}


def write_result(path: Path, result: dict) -> None:
    lines = [result["status"]]
    if result["status"] == "FEASIBLE":
        lines.append(f"{len(result['route'])} {len(result['missions'])}")
        lines.append(" ".join(map(str, result["route"])))
        lines.extend(" ".join(map(str, mission)) for mission in result["missions"])
    path.write_text("\n".join(lines) + "\n", encoding="ascii")


def main() -> int:
    started = time.monotonic()
    if len(sys.argv) == 2 and sys.argv[1] == "--check":
        from ortools.sat.python import cp_model
        print("CP-SAT disponivel")
        return 0
    if len(sys.argv) != 3:
        return 2
    try:
        data = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
        result = solve(data, started)
        write_result(Path(sys.argv[2]), result)
        return 0
    except Exception as exc:
        write_result(Path(sys.argv[2]), {"status": "ERROR"})
        print(f"Reparo CP-SAT: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
