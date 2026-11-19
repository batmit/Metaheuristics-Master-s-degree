"""Confirmacao com o teto de producao, quatro instancias, uma semente."""
import json
import subprocess
from comparar_bb5 import ROOT, OUT, EXE, validar

if __name__ == '__main__':
    with (OUT/'confirmacao_30s.jsonl').open('w', encoding='utf-8') as f:
        for indice, nome in enumerate(['24-rc-c', '105-rc-e', '144-rc-c', '200-r-e']):
            grupo = 'small' if indice == 0 else 'large'
            for variante in ([0, 1] if indice % 2 == 0 else [1, 0]):
                p = subprocess.run([str(EXE), grupo, nome, '11', str(variante), '30', '0'],
                                   cwd=ROOT, text=True, capture_output=True, check=True, timeout=90)
                r = json.loads(p.stdout); validar(grupo, nome, r)
                r.update(instancia=nome, seed=11, variante=variante, modo='tempo_30s', validada=True)
                f.write(json.dumps(r)+'\n'); f.flush()
                print(nome, variante, r['makespan'], r['segundos'], r['avaliacoes'], flush=True)
