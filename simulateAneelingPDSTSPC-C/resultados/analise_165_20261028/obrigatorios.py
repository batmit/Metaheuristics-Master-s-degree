from pathlib import Path
from fractions import Fraction as F
from functools import lru_cache
import json
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
lines=(ROOT/'instances/pdstsp-c/large/165-r-c.txt').read_text().splitlines()
xy={int(j):(F(x),F(y)) for j,x,y,w in (s.split() for s in lines[7:] if s.strip())}
tau={int(v[0]):[F(t) for t in v[1:] if F(t)>=0] for s in (ROOT/'instances/pdstsp-c/large/tau_kj_165-r-c.txt').read_text().splitlines()[1:] if (v:=s.split())}
mandatory=[j for j in tau if not tau[j]]
points=sorted(set(xy[j] for j in mandatory)-{xy[0]})
reps=[next(j for j in mandatory if xy[j]==point) for point in points]
def dist(a,b):return 2*sum(abs(x-y) for x,y in zip(xy[a],xy[b]))
@lru_cache(None)
def dp(bits,last):
    prev=bits^(1<<last)
    if not prev:return dist(0,reps[last])
    return min(dp(prev,k)+dist(reps[k],reps[last]) for k in range(len(reps)) if prev&(1<<k))
full=(1<<len(reps))-1
value=min(dp(full,k)+dist(reps[k],0) for k in range(len(reps)))
result={'mandatory_customers':mandatory,'mandatory_locations':len(points),'exact_TSP_mandatory_only':float(value),'states':dp.cache_info().currsize}
(HERE/'obrigatorios.json').write_text(json.dumps(result,indent=2))
print(json.dumps(result,indent=2))
