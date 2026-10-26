"""Executa exatamente 400 sementes distintas nos fontes atuais, sem altera-los."""
from pathlib import Path
import concurrent.futures
import datetime
import hashlib
import json
import subprocess
import time

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
GCC = r'C:\TDM-GCC-64\bin\gcc.exe'
sources = ['main.c', 'main.h', 'SA.c', 'SA.h', 'cv.c', 'cv.h', 'lerInstancias.c', 'lerInstancias.h',
           'instances/pdstsp-c/large/67-c-c.txt', 'instances/pdstsp-c/large/tau_kj_67-c-c.txt']
previous_manifest = json.loads((HERE.parent/'67-c-c_200'/'manifesto.json').read_text())
manifest = {'instance': '67-c-c', 'runs': 400, 'workers': 4, 'base_seed': max(int(time.time()), previous_manifest['base_seed'] + previous_manifest['runs']),
            'started': datetime.datetime.now().astimezone().isoformat(),
            'literature_UB': 74, 'literature_LB': 73,
            'hashes': {s: hashlib.sha256((ROOT/s).read_bytes()).hexdigest() for s in sources}}
command = [GCC, '-std=c11', '-O2', '-Wall', '-Wextra', '-pedantic', '-I', str(ROOT),
           str(HERE/'executar.c'), 'SA.c', 'cv.c', 'lerInstancias.c', '-lm', '-o', str(HERE/'executar.exe')]
manifest['compile_command'] = command
manifest['compiler'] = subprocess.check_output([GCC, '--version'], text=True).splitlines()[0]
subprocess.run(command, cwd=ROOT, check=True)
(HERE/'manifesto.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')

def worker(w):
    cmd = [str(HERE/'executar.exe'), str(manifest['base_seed']), str(w*100), '100',
           str(HERE/f'solucoes_{w}.jsonl'), str(HERE/f'execucoes_{w}.log')]
    with (HERE/f'stderr_{w}.txt').open('w') as errors:
        return subprocess.run(cmd, cwd=ROOT, stdout=errors, stderr=errors).returncode

print(f"Iniciando 400 execucoes; semente base {manifest['base_seed']}; 4 processos.", flush=True)
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
    codes = list(pool.map(worker, range(4)))
manifest['returncodes'] = codes
manifest['finished'] = datetime.datetime.now().astimezone().isoformat()
manifest['sources_unchanged'] = all(hashlib.sha256((ROOT/s).read_bytes()).hexdigest() == h
                                   for s,h in manifest['hashes'].items())
(HERE/'manifesto.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
print(json.dumps({'returncodes': codes, 'sources_unchanged': manifest['sources_unchanged']}), flush=True)
if any(codes):
    raise SystemExit(1)
