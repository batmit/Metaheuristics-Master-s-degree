"""Certificados independentes e comparacoes pareadas, sem importar codigo C."""
from pathlib import Path
from fractions import Fraction as F
from collections import defaultdict,Counter
import csv
import json
import math
import statistics
import sys

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
stage=HERE/sys.argv[1]
partial='--parcial' in sys.argv
records=[]
for path in stage.glob('*.jsonl'):
    for line in path.read_text().splitlines():
        try: records.append(json.loads(line))
        except json.JSONDecodeError:
            if not partial:raise
groups=defaultdict(list)
for r in records:groups[r['instance'],r['mask']].append(r)
summary=[]
for (name,mask),rows in sorted(groups.items()):
    vals=[r['claimed'] for r in rows]
    summary.append({'instance':name,'mask':mask,'n':len(rows),'best':min(vals),
                    'mean':statistics.mean(vals),'median':statistics.median(vals),'worst':max(vals),
                    'at_most_74':sum(v<=74 for v in vals) if name=='67-c-c' else None,
                    'seconds_mean':statistics.mean(r['seconds'] for r in rows),
                    'distribution':dict(sorted(Counter(vals).items()))})
if partial:
    print(json.dumps(summary,ensure_ascii=False,indent=2));raise SystemExit()
manifest=json.loads((stage/'manifesto.json').read_text())
assert 'finished' in manifest
assert len(records)==len(manifest['masks'])*len(manifest['instances'])*manifest['count_per_instance_mask']
for key,rows in groups.items():
    assert len(rows)==manifest['count_per_instance_mask']
    assert {r['seed'] for r in rows}==set(range(manifest['seed'],manifest['seed']+len(rows)))

def verify_instance(name,rows):
    folder='small' if int(name.split('-')[0])<=30 else 'large'
    lines=(ROOT/f'instances/pdstsp-c/{folder}/{name}.txt').read_text().splitlines()
    params={s.split(',')[0]:F(s.split(',')[1].strip()) for s in lines[:6]}
    coords={}
    for line in lines[7:]:
        if line.strip():
            j,x,y,_=line.split();coords[int(j)]=(F(x),F(y))
    fleet=int(params['NUM_DRONES']); tau={}
    for line in (ROOT/f'instances/pdstsp-c/{folder}/tau_kj_{name}.txt').read_text().splitlines()[1:]:
        if line.strip():
            vals=line.split();tau[int(vals[0])]={k:F(t) for k,t in enumerate(vals[1:],1)}
    for r in rows:
        route=r['route'];ms=r['missions']
        clients=route+[m['client'] for m in ms]
        assert len(clients)==len(coords)-1 and set(clients)==set(coords)-{0},(name,r['seed'],'coverage')
        truck=sum((sum(abs(a-b) for a,b in zip(coords[i],coords[j]))*60/params['TRUCK_SPEED']
                   for i,j in zip([0]+route,route+[0])),F(0))
        exact=[]
        for m in ms:
            a,b=F(str(m['start'])),F(str(m['end']));k=m['k']
            assert 1<=k<=fleet and a>=0 and b>a
            assert tau[m['client']][k]>=0 and b-a==tau[m['client']][k]
            exact.append((a,b,k))
        events=sorted({t for a,b,_ in exact for t in (a,b)})
        assert all(sum(k for a,b,k in exact if a<=t<b)<=fleet for t in events)
        available=[F(0)]*fleet
        for a,b,k in sorted(exact):
            ids=[i for i,t in enumerate(available) if t<=a][:k]
            assert len(ids)==k
            for i in ids:available[i]=b
        drone=max((b for _,b,_ in exact),default=F(0))
        assert truck==F(str(r['truck'])) and drone==F(str(r['drone']))
        assert max(truck,drone)==F(str(r['claimed'])),(name,r['seed'],'makespan')

for name in manifest['instances']:
    verify_instance(name,[r for r in records if r['instance']==name])

def compare(anchor):
    paired=[]
    for (name,mask),rows in sorted(groups.items()):
        if mask==anchor or (name,anchor) not in groups:continue
        baseline={r['seed']:r['claimed'] for r in groups[name,anchor]}
        deltas=[r['claimed']-baseline[r['seed']] for r in rows]
        wins=sum(d<0 for d in deltas);losses=sum(d>0 for d in deltas);ties=len(deltas)-wins-losses
        nt=wins+losses
        p=min(1.,2*sum(math.comb(nt,j) for j in range(min(wins,losses)+1))/2**nt) if nt else 1.
        paired.append({'instance':name,'mask':mask,'n':len(rows),'mean_delta':statistics.mean(deltas),
                       'wins':wins,'ties':ties,'losses':losses,'sign_test_p_two_sided':p})
    return paired

result={'validated':len(records),'summary':summary,'paired_vs_baseline':compare(0),'paired_vs_groups':compare(1)}
(stage/'resumo.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
columns=['instance','mask','run','seed','budget','seconds','initial','temperature','claimed','truck','drone']
with (stage/'execucoes.csv').open('w',newline='',encoding='utf-8') as f:
    w=csv.DictWriter(f,fieldnames=columns,extrasaction='ignore');w.writeheader()
    w.writerows(sorted(records,key=lambda r:(r['instance'],r['mask'],r['seed'])))
for name,mask in groups:
    best=min(groups[name,mask],key=lambda r:(r['claimed'],r['seed']))
    (stage/f'melhor_{name}_m{mask}.json').write_text(json.dumps(best,indent=2))
print(json.dumps(result,ensure_ascii=False,indent=2))
