from pathlib import Path
from fractions import Fraction as F
import json
HERE=Path(__file__).resolve().parent
data=json.loads((HERE/'limites_rota.json').read_text())
for name,r in data.items():
    points=[tuple(map(F,p)) for p in r['points']]
    assert len(set(points))==len(points) and all(x.denominator==1 for p in points for x in p)
    pi=list(map(F,r['potentials']));n=len(points)
    def weight(i,j):return 2*sum(abs(x-y) for x,y in zip(points[i],points[j]))+pi[i]+pi[j]
    edges=sorted((weight(i,j),i,j) for i in range(1,n) for j in range(i+1,n))
    parent=list(range(n))
    def find(i):
        while parent[i]!=i:parent[i]=parent[parent[i]];i=parent[i]
        return i
    total=F(0);accepted=0
    for w,i,j in edges:
        a,b=find(i),find(j)
        if a!=b:parent[a]=b;total+=w;accepted+=1
        if accepted==n-2:break
    assert accepted==n-2
    total+=sum(sorted(weight(0,j) for j in range(1,n))[:2])-2*sum(pi)
    assert total==F(r['exact_LB_numerator'],r['exact_LB_denominator']) and total>156
    print(name,'LB=',float(total),'-> rota multipla de quatro >= 160')
