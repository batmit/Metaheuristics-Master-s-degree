from pathlib import Path
from fractions import Fraction as F
import ast,json,statistics,math,hashlib
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
tree=ast.parse((ROOT/'resultados/grupos_quatro_100/analisar.py').read_text())
fn=next(n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='verify_instance')
scope={'ROOT':ROOT,'F':F}
exec(compile(ast.Module(body=[fn],type_ignores=[]),'validator','exec'),scope)
probes=[json.loads(line) for p in HERE.glob('*_probe.jsonl') for line in p.read_text().splitlines()]
trace=[json.loads(p.read_text()) for p in HERE.glob('trace_*.jsonl')]
for name in ['67-c-c','181-r-e']:
    scope['verify_instance'](name,[r for r in probes+trace if r['instance']==name])
manifest=json.loads((HERE/'experimentos_manifesto.json').read_text())
assert len(manifest['runs'])==12 and len(trace)==12
assert manifest['sources_unchanged'] and all(r['code']==0 for r in manifest['runs'])
assert all(hashlib.sha256((ROOT/n).read_bytes()).hexdigest()==h for n,h in manifest['source_sha256'].items())
rows=[]
for r in manifest['runs']:
    samples=[]
    for line in (HERE/(r['stem']+'.stdout')).read_text().splitlines():
        if line.startswith('THERM '):
            _,t,temp,best,now,restarts=line.split();samples.append({'seconds':float(t),'temperature':float(temp),'best':float(best),'current':float(now),'restarts':int(restarts)})
    final=json.loads((HERE/(r['stem']+'.jsonl')).read_text())
    late=[s for s in samples if 8<=s['seconds']<10]
    rows.append(dict(r,claimed=final['claimed'],truck=final['truck'],drone=final['drone'],restarts=final['stats']['restarts'],
                     late_samples=len(late),late_temperature_median=statistics.median(s['temperature'] for s in late) if late else None,
                     late_expected_acceptance_delta4=statistics.mean(math.exp(-4/s['temperature']) for s in late) if late else None,
                     evaluations=final['stats']['evaluations'],seconds=final['seconds']))
summary=[]
for name in ['67-c-c','181-r-e']:
    for mask in [15,271]:
        data=[r for r in rows if r['instance']==name and r['mask']==mask]
        summary.append({'instance':name,'mode':'current' if mask==15 else 'no_restarts','n':len(data),
                        'results':[r['claimed'] for r in data],'median_restarts':statistics.median(r['restarts'] for r in data),
                        'median_late_temperature':statistics.median(r['late_temperature_median'] for r in data),
                        'mean_late_expected_acceptance_delta4':statistics.mean(r['late_expected_acceptance_delta4'] for r in data)})
result={'validated_probe_certificates':len(probes),'validated_trace_certificates':len(trace),'sources_unchanged':True,'probes':[dict(instance=r['instance'],mode=r['mode'],truck=r['truck'],drone=r['drone'],makespan=r['claimed']) for r in probes], 'thermal_summary':summary,'thermal_runs':rows}
(HERE/'validacao_ensaios.json').write_text(json.dumps(result,indent=2))
print(json.dumps({k:v for k,v in result.items() if k!='thermal_runs'},indent=2))
