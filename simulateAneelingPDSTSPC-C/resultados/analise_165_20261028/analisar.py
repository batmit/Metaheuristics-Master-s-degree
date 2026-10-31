from pathlib import Path
from fractions import Fraction as F
from functools import lru_cache
from collections import Counter
import ast, json, math, hashlib, statistics

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
refs=json.loads((ROOT/'resultados/revisao_literatura_2025/testemunhas_validadas.json').read_text())
tree=ast.parse((ROOT/'resultados/grupos_quatro_100/analisar.py').read_text())
fn=next(n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='verify_instance')
scope={'ROOT':ROOT,'F':F}
exec(compile(ast.Module(body=[fn],type_ignores=[]),'validator','exec'),scope)
out={}
for name,target in [('165-r-c',159)]:
    lines=(ROOT/f'instances/pdstsp-c/large/{name}.txt').read_text().splitlines()
    pars={s.split(',')[0]:F(s.split(',')[1].strip()) for s in lines[:6]}
    xy={int(j):(F(x),F(y)) for j,x,y,w in (s.split() for s in lines[7:] if s.strip())}
    tau={}
    for line in (ROOT/f'instances/pdstsp-c/large/tau_kj_{name}.txt').read_text().splitlines()[1:]:
        v=line.split()
        if v:tau[int(v[0])]={k:F(t) for k,t in enumerate(v[1:],1) if F(t)>=0}
    fleet=int(pars['NUM_DRONES'])
    minarea={j:min(k*t for k,t in v.items()) if v else None for j,v in tau.items()}
    def area(ds):return sum((minarea[j] for j in ds),F(0))
    matrix=[[sum(abs(a-b) for a,b in zip(xy[i],xy[j]))*60/pars['TRUCK_SPEED'] for j in xy] for i in xy]
    def cost(route):return sum((matrix[a][b] for a,b in zip([0]+list(route),list(route)+[0])),F(0))
    def exact_route(route):
        points=sorted(set(xy[j] for j in route)-{xy[0]})
        reps=[next(j for j in route if xy[j]==p) for p in points]
        @lru_cache(None)
        def dp(bits,last):
            prev=bits^(1<<last)
            if not prev:return matrix[0][reps[last]]
            return min(dp(prev,k)+matrix[reps[k]][reps[last]] for k in range(len(reps)) if prev&(1<<k))
        full=(1<<len(reps))-1
        return min((dp(full,k)+matrix[reps[k]][0] for k in range(len(reps))),default=F(0))
    rows=[]; provenance=[]
    for folder in ['tres_maiores_10x_30s_20261026','tres_maiores_20x_30s_20261027','tres_maiores_50x_30s_20261027','tres_maiores_100x_30s_20261027','quatro_abertas_10x_30s_20261028']:
        batch=[json.loads(p.read_text()) for p in (ROOT/'resultados'/folder).glob(f'{name}_*.jsonl')]
        scope['verify_instance'](name,batch)
        rows.extend(batch)
        provenance.append({'folder':folder,'n':len(batch),'best':min((r['claimed'] for r in batch),default=None)})
    best=min(rows,key=lambda r:(r['claimed'],r['seed']))
    ds={m['client'] for m in best['missions']}; official=refs[name]
    ods={m['client'] for m in official['missions']}
    official_row={'seed':0,'route':[j for j in official['route'] if j], 'missions':official['missions'],
                  'truck':official['truck'],'drone':official['drone'],'claimed':official['makespan']}
    scope['verify_instance'](name,[official_row])
    detail={'target':target,'n':len(rows),'batches':provenance,'best':best,'distribution':dict(sorted(Counter(r['claimed'] for r in rows).items())),
            'official':official,'customers':len(xy)-1,'fleet':fleet,'locations':len(set(xy.values())),
            'eligible':sum(bool(v) for v in tau.values()),'cooperative_only':sum(bool(v) and min(v)>1 for v in tau.values()),
            'drone_count':len(ds),'reference_drone_count':len(ods),'different_resource':len(ds^ods),
            'to_drone_vs_best':sorted(ods-ds),'to_truck_vs_best':sorted(ds-ods),
            'min_area':float(area(ds)),'conditional_drone_LB':math.ceil(area(ds)/fleet),
            'actual_area':float(sum(m['k']*F(str(m['end']-m['start'])) for m in best['missions'])),
            'reference_min_area':float(area(ods)), 'truck_blocks_target':sum(r['truck']>target for r in rows),
            'drone_blocks_target':sum(r['drone']>target for r in rows),
            'assignment_LB_blocks_target':sum(math.ceil(area({m['client'] for m in r['missions']})/fleet)>target for r in rows),
            'normal_time_runs':sum(r['seconds']<=35 for r in rows),
            'normal_time_best':min(r['claimed'] for r in rows if r['seconds']<=35),
            'median_evaluations':statistics.median(r['stats']['evaluations'] for r in rows),
            'median_restarts':statistics.median(r['stats']['restarts'] for r in rows),
            'zero_saving_eligible_removals':sum(bool(tau[j]) and cost([x for x in best['route'] if x!=j])==best['truck'] for j in best['route']),
            'positive_saving_eligible_removals':sum(bool(tau[j]) and cost([x for x in best['route'] if x!=j])<best['truck'] for j in best['route'])}
    if name=='67-c-c':
        detail['exact_TSP_best_assignment']=float(exact_route(best['route']))
        probes=[]
        for point in sorted({xy[j] for j in best['route']}):
            removed=[j for j in best['route'] if xy[j]==point]
            if not all(tau[j] for j in removed):continue
            new_route=[j for j in best['route'] if j not in removed]
            probes.append({'point':list(map(float,point)),'clients':removed,'exact_truck_after':float(exact_route(new_route)),
                           'drone_LB_after':math.ceil(area(ds|set(removed))/fleet)})
        detail['whole_location_probes']=probes
        detail['exact_TSP_by_assignment']={}
        for r in rows:
            key=tuple(sorted({xy[j] for j in r['route']}))
            if str(key) not in detail['exact_TSP_by_assignment']:
                detail['exact_TSP_by_assignment'][str(key)]=float(exact_route(r['route']))
    out[name]=detail
out['hashes']={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in ['SA.c','cv.c','SA.h','cv.h','lerInstancias.c']}
(HERE/'diagnostico.json').write_text(json.dumps(out,indent=2),encoding='utf-8')
for name,d in out.items():
    if name=='hashes':continue
    print(name,json.dumps({k:v for k,v in d.items() if k not in ['best','official','exact_TSP_by_assignment']}))

