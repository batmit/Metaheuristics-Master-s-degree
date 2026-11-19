"""Tres tratamentos pareados: base, BB5 e BB5 com filtro do caminhao."""
import csv
import hashlib
import itertools
import json
import platform
import statistics
import subprocess
from comparar_bb5 import ROOT, validar

OUT = ROOT / 'resultados/experimento_bb5_gargalo_20261119'
EXE = ROOT / 'tmp/benchmark_bb5_gargalo.exe'
NOMES = ['base', 'bb5', 'bb5_gargalo']


def resumir(rows):
    resultado = {}
    for modo in dict.fromkeys(r['modo'] for r in rows):
        grupos = [{(r['instancia'], r['seed']): r for r in rows
                   if r['modo']==modo and r['variante']==v} for v in range(3)]
        assert grupos[0].keys() == grupos[1].keys() == grupos[2].keys()
        resumo = {'triplas': len(grupos[0])}
        for v, grupo in enumerate(grupos):
            rs = list(grupo.values()); tempo = sum(r['segundos'] for r in rs)
            resumo[NOMES[v]] = dict(tempo_medio=statistics.mean(r['segundos'] for r in rs),
                avaliacoes_medias=statistics.mean(r['avaliacoes'] for r in rs),
                gap_medio=statistics.mean(100*(r['makespan']/r['referencia']-1) for r in rs),
                fracao_bb=sum(r['segundos_c']+r['segundos_d'] for r in rs)/tempo,
                segundos_c=sum(r['segundos_c'] for r in rs), segundos_d=sum(r['segundos_d'] for r in rs),
                **{k:sum(r[k] for r in rs) for k in ['chamadas_c','chamadas_d','bloqueios_c',
                                                    'nos_c','nos_d','melhorias_c','melhorias_d']})
        for a, b in [(0, 2), (1, 2)]:
            ga, gb = grupos[a], grupos[b]
            resumo[NOMES[b]+'_vs_'+NOMES[a]] = dict(
                melhores=sum(gb[k]['makespan'] < ga[k]['makespan']-1e-7 for k in ga),
                iguais=sum(abs(gb[k]['makespan']-ga[k]['makespan']) <= 1e-7 for k in ga),
                piores=sum(gb[k]['makespan'] > ga[k]['makespan']+1e-7 for k in ga),
                razao_tempo_mediana=statistics.median(gb[k]['segundos']/ga[k]['segundos'] for k in ga))
        resultado[modo] = resumo
    return resultado


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    refs = {r['instance']:r for r in csv.DictReader(
        (ROOT/'instances/pdstsp-c/referencias.csv').open(encoding='utf-8-sig'))}
    nomes = ['18-r-e','24-rc-c','30-c-c','67-c-c','99-rc-c','105-rc-e',
             '126-r-c','144-rc-c','154-c-c','165-r-c','181-r-e','200-r-e']
    jobs = [(modo, nome, seed, segundos, n) for modo, segundos, n in
            [('tempo_2s', 2, 0), ('1000_propostas', 0, 1000)]
            for nome in nomes for seed in [11,29,47]]
    jobs += [('tempo_30s', nome, 11, 30, 0) for nome in ['24-rc-c','105-rc-e','144-rc-c','200-r-e']]
    arquivos = ['SA.c','SA.h','cv.c','cv.h','tests/benchmark_bb5.c',
                'tests/comparar_bb5.py','tests/comparar_bb5_gargalo.py','tmp/benchmark_bb5_gargalo.exe']
    meta = dict(plataforma=platform.platform(), cpu=platform.processor(),
                gcc=subprocess.check_output(['gcc','--version'], text=True).splitlines()[0],
                hashes={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in arquivos},
                tratamentos=NOMES, gatilho=5, max_nos=10000, segundos_por_bb=.02,
                filtro='variante 2: tempoCaminhao > tempoDrones + 1e-9; empate bloqueia; '
                       'contador reiniciado ao atingir cinco, inclusive se bloqueado; drones inalterados',
                protocolo='sequencial; seis permutacoes ciclicas dos tratamentos; sementes pareadas; '
                          '2s/30s incluem calibracao; 1000 propostas sem calibracao, macros, retomadas ou BB final',
                python=platform.python_version())
    (OUT/'proveniencia.json').write_text(json.dumps(meta, indent=2), encoding='utf-8')
    ordens = list(itertools.permutations(range(3)))
    rows = []
    with (OUT/'execucoes.jsonl').open('w', encoding='utf-8') as f:
        for i, (modo, nome, seed, segundos, n) in enumerate(jobs):
            grupo = 'small' if int(refs[nome]['customers']) <= 30 else 'large'
            for v in ordens[i % len(ordens)]:
                p = subprocess.run([str(EXE),grupo,nome,str(seed),str(v),str(segundos),str(n)],
                                   cwd=ROOT,text=True,capture_output=True,check=True,timeout=90)
                r = json.loads(p.stdout); validar(grupo,nome,r)
                r.update(modo=modo,instancia=nome,seed=seed,variante=v,validada=True,
                         referencia=float(refs[nome]['ub_2025']))
                rows.append(r); f.write(json.dumps(r)+'\n'); f.flush()
                if modo=='tempo_30s':
                    print(nome, NOMES[v], r['makespan'], r['segundos'], flush=True)
            print(f'{i+1}/{len(jobs)} {modo} {nome} seed={seed}', flush=True)
    campos = [k for k in rows[0] if k not in ('rota','missoes')]
    with (OUT/'execucoes.csv').open('w',newline='',encoding='utf-8-sig') as f:
        w=csv.DictWriter(f,fieldnames=campos,extrasaction='ignore');w.writeheader();w.writerows(rows)
    resumo=resumir(rows)
    (OUT/'resumo.json').write_text(json.dumps(resumo,indent=2),encoding='utf-8')
    print(json.dumps(resumo,indent=2),flush=True)


if __name__ == '__main__':
    main()
