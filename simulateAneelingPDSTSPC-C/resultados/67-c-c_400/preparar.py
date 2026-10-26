"""Prepara um novo lote sem sobrescrever os resultados das 200 execucoes."""
from pathlib import Path

here = Path(__file__).resolve().parent
previous = here.parent / '67-c-c_200'
assert not (here/'manifesto.json').exists(), 'Este lote ja foi iniciado.'
source = (previous/'executar.c').read_text().replace('/200;', '/400;')
(here/'executar.c').write_text(source)
runner = (previous/'rodar.py').read_text().replace('200', '400')
runner = runner.replace('str(w*50)', 'str(w*100)').replace("'50'", "'100'")
runner = runner.replace("manifest = {'instance'", "previous_manifest = json.loads((HERE.parent/'67-c-c_200'/'manifesto.json').read_text())\nmanifest = {'instance'")
runner = runner.replace("'base_seed': int(time.time())", "'base_seed': max(int(time.time()), previous_manifest['base_seed'] + previous_manifest['runs'])")
(here/'rodar.py').write_text(runner)
analysis = (previous/'analisar.py').read_text().replace('200', '400').replace('201', '401')
analysis = analysis.replace('50 execucoes por processo', '100 execucoes por processo')
analysis = analysis.replace("assert len({r['seed'] for r in records}) == 400", "assert len({r['seed'] for r in records}) == 400\nprevious_manifest = json.loads((HERE.parent/'67-c-c_200'/'manifesto.json').read_text())\nassert manifest['base_seed'] >= previous_manifest['base_seed'] + previous_manifest['runs']")
(here/'analisar.py').write_text(analysis)
print('Executor e validador preparados para 400 novas execucoes.')
