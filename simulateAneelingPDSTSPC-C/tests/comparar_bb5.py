"""Experimento pareado e validacao independente dos certificados finais."""
import csv
import hashlib
import json
import platform
import statistics
import subprocess
from fractions import Fraction as F
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'resultados/experimento_bb5_20261119'
EXE = ROOT / 'tmp/benchmark_bb5.exe'


def validar(grupo, nome, r):
    base = ROOT / 'instances/pdstsp-c' / grupo
    linhas = (base / (nome+'.txt')).read_text().splitlines()
    frota = int(linhas[0].split(',')[1])
    velocidade = F(linhas[1].split(',')[1].strip())
    pontos = [list(map(F, line.split())) for line in linhas[7:] if line.strip()]
    tau = [list(map(F, line.split())) for line in (base / ('tau_kj_'+nome+'.txt')).read_text().splitlines()[1:] if line.strip()]
    rota = [0] + r['rota'] + [0]
    t = sum((abs(pontos[a][1]-pontos[b][1])+abs(pontos[a][2]-pontos[b][2]))*60/velocidade
            for a, b in zip(rota, rota[1:]))
    clientes = r['rota'] + [m[0] for m in r['missoes']]
    assert sorted(clientes) == list(range(1, len(pontos)))
    for cliente, k, ini, fim in r['missoes']:
        assert 1 <= k <= frota and ini >= 0 and fim >= ini
        duracao = tau[cliente-1][k]
        assert duracao >= 0 and abs(float(duracao)-(fim-ini)) < 1e-7
    for _, _, ini, _ in r['missoes']:
        assert sum(k for _, k, a, b in r['missoes'] if a <= ini < b) <= frota
    d = max((m[3] for m in r['missoes']), default=0)
    assert abs(float(t)-r['caminhao']) < 1e-7
    assert abs(d-r['drone']) < 1e-7 and abs(max(float(t),d)-r['makespan']) < 1e-7


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    refs = list(csv.DictReader((ROOT/'instances/pdstsp-c/referencias.csv').open(encoding='utf-8-sig')))
    selecionadas = {'18-r-e', '24-rc-c', '30-c-c', '67-c-c', '99-rc-c', '105-rc-e',
                    '126-r-c', '144-rc-c', '154-c-c', '165-r-c', '181-r-e', '200-r-e'}
    jobs = [('tempo_1s', x, 314159, 1, 0) for x in refs]
    jobs += [(modo, x, seed, segundos, n) for modo, segundos, n in
             [('tempo_2s', 2, 0), ('1000_propostas', 0, 1000)]
             for x in refs if x['instance'] in selecionadas for seed in [11, 29, 47]]
    rows = []
    with (OUT/'execucoes.jsonl').open('w', encoding='utf-8') as stream:
        for indice, (modo, ref, seed, segundos, n) in enumerate(jobs):
            nome = ref['instance']; grupo = 'small' if int(ref['customers']) <= 30 else 'large'
            # Alterna a ordem dos tratamentos para reduzir vies temporal.
            for variante in ([0, 1] if indice % 2 == 0 else [1, 0]):
                proc = subprocess.run([str(EXE), grupo, nome, str(seed), str(variante), str(segundos), str(n)],
                                      cwd=ROOT, capture_output=True, text=True, check=True, timeout=90)
                r = json.loads(proc.stdout); validar(grupo, nome, r)
                r.update(modo=modo, instancia=nome, seed=seed, variante=variante, validada=True,
                         referencia=float(ref['ub_2025']))
                rows.append(r); stream.write(json.dumps(r)+'\n'); stream.flush()
            print(f'{indice+1}/{len(jobs)} {modo} {nome} seed={seed}', flush=True)
    campos = [k for k in rows[0] if k not in ('rota', 'missoes')]
    with (OUT/'execucoes.csv').open('w', newline='', encoding='utf-8-sig') as f:
        w = csv.DictWriter(f, fieldnames=campos, extrasaction='ignore'); w.writeheader(); w.writerows(rows)
    resumo = {}
    for modo in dict.fromkeys(r['modo'] for r in rows):
        a = {(r['instancia'], r['seed']): r for r in rows if r['modo']==modo and r['variante']==0}
        b = {(r['instancia'], r['seed']): r for r in rows if r['modo']==modo and r['variante']==1}
        resumo[modo] = dict(pares=len(a), melhores=sum(b[k]['makespan']<a[k]['makespan']-1e-7 for k in a),
                           iguais=sum(abs(b[k]['makespan']-a[k]['makespan'])<=1e-7 for k in a),
                           piores=sum(b[k]['makespan']>a[k]['makespan']+1e-7 for k in a),
                           razao_tempo_mediana=statistics.median(b[k]['segundos']/a[k]['segundos'] for k in a))
        for rotulo, tratamento in [('base', a), ('bb5', b)]:
            rs = list(tratamento.values())
            resumo[modo][rotulo] = dict(tempo_medio=statistics.mean(r['segundos'] for r in rs),
                avaliacoes_medias=statistics.mean(r['avaliacoes'] for r in rs),
                gap_medio=statistics.mean(100*(r['makespan']/r['referencia']-1) for r in rs),
                chamadas_c=sum(r['chamadas_c'] for r in rs), chamadas_d=sum(r['chamadas_d'] for r in rs),
                nos_c=sum(r['nos_c'] for r in rs), nos_d=sum(r['nos_d'] for r in rs),
                melhorias_c=sum(r['melhorias_c'] for r in rs), melhorias_d=sum(r['melhorias_d'] for r in rs),
                fracao_bb=sum(r['segundos_c']+r['segundos_d'] for r in rs)/sum(r['segundos'] for r in rs))
    (OUT/'resumo.json').write_text(json.dumps(resumo, indent=2), encoding='utf-8')
    arquivos = ['SA.c', 'SA.h', 'cv.c', 'cv.h', 'tests/benchmark_bb5.c', 'tests/comparar_bb5.py']
    meta = dict(plataforma=platform.platform(), processador=platform.processor(), python=platform.python_version(),
                gcc=subprocess.check_output(['gcc', '--version'], text=True).splitlines()[0],
                hashes={p: hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in arquivos},
                parametros=dict(gatilho=5, max_nos=10000, segundos_por_bb=.02, bound_passos=8),
                protocolo='sequencial; ordem alternada; sementes pareadas; orcamentos 1s/2s incluem calibracao; '
                          '1000 propostas com temperatura inicial 10 e sem calibracao, macros, retomadas ou BB final; '
                          'sem sobrescrever recordes de producao')
    (OUT/'proveniencia.json').write_text(json.dumps(meta, indent=2), encoding='utf-8')
    print(json.dumps(resumo, indent=2), flush=True)


if __name__ == '__main__':
    main()
