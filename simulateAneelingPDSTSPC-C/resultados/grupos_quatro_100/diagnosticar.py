"""Diagnostico dos certificados finais; nao altera o algoritmo de producao."""
from pathlib import Path
from fractions import Fraction as F
from functools import lru_cache
import json
import math
import statistics

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
refs=json.loads((ROOT/'resultados/revisao_literatura_2025/testemunhas_validadas.json').read_text())
names=['67-c-c','165-r-c','167-r-e','181-r-e']
records=[json.loads(line) for p in HERE.glob('*.jsonl') for line in p.read_text().splitlines()]
out=[]
for name in names:
    lines=(ROOT/f'instances/pdstsp-c/large/{name}.txt').read_text().splitlines()
    params={s.split(',')[0]:F(s.split(',')[1].strip()) for s in lines[:6]}
    coords={int(j):(F(x),F(y)) for j,x,y,w in (s.split() for s in lines[7:] if s.strip())}
    tau={}
    for line in (ROOT/f'instances/pdstsp-c/large/tau_kj_{name}.txt').read_text().splitlines()[1:]:
        v=line.split()
        if v:tau[int(v[0])]={k:F(t) for k,t in enumerate(v[1:],1) if F(t)>=0}
    fleet=int(params['NUM_DRONES'])
    def dist(i,j):return float(sum(abs(a-b) for a,b in zip(coords[i],coords[j]))*60/params['TRUCK_SPEED'])
    ids=list(coords)
    matrix=[[dist(i,j) for j in ids] for i in ids]
    def cost(route):return sum(matrix[i][j] for i,j in zip([0]+route,route+[0]))
    def two_opt(route):
        route=list(route)
        while True:
            best=0; move=None
            for i in range(len(route)-1):
                a=route[i-1] if i else 0;b=route[i]
                for j in range(i+1,len(route)):
                    c=route[j];d=route[j+1] if j+1<len(route) else 0
                    delta=matrix[a][c]+matrix[b][d]-matrix[a][b]-matrix[c][d]
                    if delta<best-1e-8:best=delta;move=(i,j)
            if move is None:return route
            i,j=move;route[i:j+1]=reversed(route[i:j+1])
    for mask in (0,1):
        rows=[r for r in records if r['instance']==name and r['mask']==mask]
        best=min(rows,key=lambda r:(r['claimed'],r['seed']))
        official=refs[name];ds={m['client'] for m in best['missions']};rs={m['client'] for m in official['missions']}
        area=sum((min(k*t for k,t in tau[j].items()) for j in ds),F(0))
        route=best['route'];polished=two_opt(route);best_cost=cost(polished)
        # Dez construcoes deterministicas com o MESMO conjunto de clientes.
        for first in route[:10]:
            rem=set(route)-{first};trial=[first]
            while rem:
                j=min(rem,key=lambda j:(matrix[trial[-1]][j],j));rem.remove(j);trial.append(j)
            best_cost=min(best_cost,cost(two_opt(trial)))
        row={'instance':name,'mask':mask,'seed':best['seed'],'makespan':best['claimed'],
             'truck':best['truck'],'drone':best['drone'],'drone_clients':len(ds),
             'reference_drone_clients':len(rs),'drone_clients_common':len(ds&rs),
             'clients_different_resource':len(ds^rs),'min_workload':float(area),
             'conditional_drone_LB':float(area/fleet),'conditional_integer_LB':math.ceil(area/fleet),
             'polished_fixed_assignment_truck':best_cost,
             'hypothetical_makespan_after_route_only':max(best_cost,best['drone']),
             'temperature_min':min(r['temperature'] for r in rows),
             'temperature_median':statistics.median(r['temperature'] for r in rows),
             'temperature_max':max(r['temperature'] for r in rows)}
        if name=='67-c-c':
            # Co-localizados podem ser agrupados porque nao ha tempo de servico.
            points=sorted({coords[j] for j in route}-{coords[0]})
            reps=[next(j for j in route if coords[j]==p) for p in points]
            @lru_cache(None)
            def dp(bits,last):
                prev=bits^(1<<last)
                if not prev:return matrix[0][reps[last]]
                return min(dp(prev,k)+matrix[reps[k]][reps[last]] for k in range(len(reps)) if prev&(1<<k))
            full=(1<<len(reps))-1
            exact=min(dp(full,k)+matrix[reps[k]][0] for k in range(len(reps)))
            row['exact_TSP_fixed_assignment']=exact
            row['truck_locations']=len(points)
            row['reference_truck_locations']=len({coords[j] for j in official['route'] if j})
            probes=[]
            for h,p in enumerate(points):
                removed=[j for j in route if coords[j]==p]
                if not all(tau[j] for j in removed):continue
                bits=full^(1<<h)
                t=min(dp(bits,k)+matrix[reps[k]][0] for k in range(len(reps)) if bits&(1<<k)) if bits else 0
                lb=(area+sum((min(k*v for k,v in tau[j].items()) for j in removed),F(0)))/fleet
                probes.append({'coordinate':[float(v) for v in p],'clients':removed,
                               'direct_truck':cost([j for j in route if j not in removed]),
                               'exact_truck_after':t,'conditional_drone_LB_after':float(lb),
                               'relaxed_makespan_LB':max(t,math.ceil(lb))})
            row['whole_location_probes']=probes
            row['exact_TSP_states']=dp.cache_info().currsize
        out.append(row)
(HERE/'diagnostico_novo.json').write_text(json.dumps(out,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(out,ensure_ascii=False,indent=2))

# Testemunhas oficiais no escalonador C atual, sem SA e sem novos sorteios.
source=(ROOT/'resultados/diagnostico_abertas/executar_diagnostico.c').read_text()
source=source.replace('assert(construirSolucaoInicial(&in,&s));double initial=s.makespan;',
                      'liberarSolucao(&s);liberarInstancia(&in);return;\n    assert(construirSolucaoInicial(&in,&s));double initial=s.makespan;')
(HERE/'verificar_referencias.c').write_text(source,encoding='utf-8')
