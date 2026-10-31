from pathlib import Path
from fractions import Fraction as F
import ast,json,hashlib,statistics
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
tree=ast.parse((ROOT/'resultados/grupos_quatro_100/analisar.py').read_text())
fn=next(n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='verify_instance')
scope={'ROOT':ROOT,'F':F}
exec(compile(ast.Module(body=[fn],type_ignores=[]),'validator','exec'),scope)
probes=[json.loads(line) for p in HERE.glob('*_probe.jsonl') for line in p.read_text().splitlines()]
rows=[json.loads(p.read_text()) for p in HERE.glob('ensaio_*.jsonl')]
scope['verify_instance']('165-r-c',probes+rows)
manifest=json.loads((HERE/'manifesto_ensaios.json').read_text())
assert len(rows)==12 and len(manifest['runs'])==12 and all(r['code']==0 for r in manifest['runs'])
assert manifest['sources_unchanged'] and all(hashlib.sha256((ROOT/n).read_bytes()).hexdigest()==h for n,h in manifest['source_sha256'].items())
summary=[]
for mode in range(4):
    data=sorted((r for r in rows if r['mode']==mode),key=lambda r:r['seed'])
    assert len(data)==3
    summary.append({'mode':mode,'description':manifest['modes'][str(mode)],'n':len(data),'results':[r['claimed'] for r in data],
                    'best':min(r['claimed'] for r in data),'mean':statistics.mean(r['claimed'] for r in data),
                    'median_evaluations':statistics.median(r['evaluations'] for r in data),
                    'assignments_changed':sum(set(r['route'])!=set(json.loads((HERE/'diagnostico.json').read_text())['165-r-c']['best']['route']) for r in data),
                    'below160':sum(r['claimed']<160 for r in data)})
result={'validated_probes':len(probes),'validated_experiments':len(rows),'sources_unchanged':True,'summary':summary}
(HERE/'validacao.json').write_text(json.dumps(result,indent=2))
print(json.dumps(result,indent=2))
