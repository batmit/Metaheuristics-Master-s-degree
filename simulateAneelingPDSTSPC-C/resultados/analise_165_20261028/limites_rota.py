from pathlib import Path
from fractions import Fraction as F
import json,math,time
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
d=json.loads((HERE/'diagnostico.json').read_text())['165-r-c']
lines=(ROOT/'instances/pdstsp-c/large/165-r-c.txt').read_text().splitlines()
xy={int(j):(F(x),F(y)) for j,x,y,w in (s.split() for s in lines[7:] if s.strip())}
def bound(route,ub=160,iterations=2500):
    points=[xy[0]]+sorted(set(xy[j] for j in route)-{xy[0]})
    n=len(points);cost=[[2*sum(abs(x-y) for x,y in zip(a,b)) for b in points] for a in points]
    def tree(pi):
        used=[False]*n;used[0]=True
        cheapest=[math.inf]*n;parent=[-1]*n;cheapest[1]=0
        edges=[]
        for _ in range(n-1):
            j=min((j for j in range(1,n) if not used[j]),key=lambda j:cheapest[j])
            used[j]=True
            if parent[j]>=0:edges.append((j,parent[j]))
            for k in range(1,n):
                c=cost[j][k]+pi[j]+pi[k]
                if not used[k] and c<cheapest[k]:cheapest[k]=c;parent[k]=j
        edges.extend((0,j) for j in sorted(range(1,n),key=lambda j:cost[0][j]+pi[0]+pi[j])[:2])
        degrees=[0]*n
        for a,b in edges:degrees[a]+=1;degrees[b]+=1
        value=sum(cost[a][b]+pi[a]+pi[b] for a,b in edges)-2*sum(pi)
        return value,degrees,edges
    pi=[0.]*n;best=-math.inf;bestpi=None;factor=1.8;stale=0
    for iteration in range(iterations):
        lb,degrees,edges=tree(pi)
        if lb>best+1e-8:best=lb;bestpi=pi[:];stale=0
        else:stale+=1
        norm=sum((v-2)**2 for v in degrees)
        if not norm or best>156.00001:break
        if stale>=100:factor*=.7;stale=0
        step=factor*max(0,ub-lb)/norm
        pi=[p+step*(degree-2) for p,degree in zip(pi,degrees)]
    # A new exact rational MST makes the stored bound independently checkable.
    rational=[F(str(v)) for v in bestpi]
    exact,degrees,edges=tree(rational)
    result={'locations_including_depot':n,'iterations':iteration+1,'floating_LB':best,
            'exact_LB_numerator':exact.numerator,'exact_LB_denominator':exact.denominator,
            'exact_LB_float':float(exact),'rules_out_route_156':exact>156,
            'potentials':[str(v) for v in rational],'points':[[str(x),str(y)] for x,y in points],'edges':edges}
    return result
out={}
out['best_assignment']=bound(d['best']['route'])
out['official_assignment']=bound([j for j in d['official']['route'] if j])
(HERE/'limites_rota.json').write_text(json.dumps(out,indent=2))
print(json.dumps({k:{x:y for x,y in v.items() if x not in ['potentials','points','edges']} for k,v in out.items()},indent=2))
