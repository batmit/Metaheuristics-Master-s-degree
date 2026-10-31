from pathlib import Path
from fractions import Fraction as F
import ast
import json
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
source=ast.parse((ROOT/'resultados/grupos_quatro_100/analisar.py').read_text())
validator=next(n for n in source.body if isinstance(n,ast.FunctionDef) and n.name=='verify_instance')
scope={'ROOT':ROOT,'F':F}
exec(compile(ast.Module(body=[validator],type_ignores=[]),'<independent-validator>','exec'),scope)
rows=[json.loads(p.read_text()) for p in HERE.glob('*.jsonl')]
for name,ref in [('165-r-c',160),('167-r-e',180),('181-r-e',216)]:
    data=[r for r in rows if r['instance']==name]
    scope['verify_instance'](name,data)
    values=[r['claimed'] for r in data]
    print(json.dumps({'instance':name,'validated':len(data),'best':min(values) if values else None,'ties':values.count(ref),'improvements':sum(v<ref for v in values),'slow_runs':sum(r['seconds']>35 for r in data)}))
