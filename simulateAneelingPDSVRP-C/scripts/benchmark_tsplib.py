"""Test the unmodified routing/SA core on TSPLIB EUC_2D optima."""
from concurrent.futures import ThreadPoolExecutor, as_completed
from decimal import Decimal, localcontext, ROUND_FLOOR
import csv
import hashlib
import json
import math
import platform
import statistics
import subprocess
import time
from executar_benchmark import ROOT
from baixar_tsplib import OPTIMA

OUT=ROOT/'output/benchmark_tsplib'
DATA=ROOT/'instances/tsplib'
SEEDS=list(range(1,11))


def read(name):
    text=(DATA/f'{name}.tsp').read_text()
    header,coords=text.split('NODE_COORD_SECTION')
    fields=dict(line.split(':',1) for line in header.splitlines() if ':' in line)
    fields={k.strip():v.strip() for k,v in fields.items()}
    if fields['TYPE']!='TSP' or fields['EDGE_WEIGHT_TYPE']!='EUC_2D':
        raise ValueError('Unsupported metric')
    nodes=[]
    for line in coords.splitlines():
        values=line.split()
        if not values or values[0]=='EOF': continue
        if int(values[0])!=len(nodes)+1: raise ValueError('Nonsequential IDs')
        nodes.append((Decimal(values[1]),Decimal(values[2])))
    if len(nodes)!=int(fields['DIMENSION']): raise ValueError('Dimension mismatch')
    matrix=[[math.floor(math.hypot(float(x-a),float(y-b))+0.5) for a,b in nodes] for x,y in nodes]
    raw=(DATA/f'{name}.opt.tour').read_text().split('TOUR_SECTION')[1].split()
    tour=[]
    for item in raw:
        if item in ('-1','EOF'): break
        tour.append(int(item)-1)
    if validate(nodes,tour)!=OPTIMA[name]: raise ValueError('Official optimum tour mismatch')
    # Compare the complete matrix to a separate Decimal implementation.
    with localcontext() as ctx:
        ctx.prec=50
        for i,(x,y) in enumerate(nodes):
            for j,(a,b) in enumerate(nodes):
                d=int((((x-a)**2+(y-b)**2).sqrt()+Decimal('0.5')).to_integral_value(rounding=ROUND_FLOOR))
                if matrix[i][j]!=d: raise ValueError('Distance rounding mismatch')
    return nodes,matrix


def validate(nodes,tour):
    if sorted(tour)!=list(range(len(nodes))): raise ValueError('Invalid Hamiltonian cycle')
    total=0
    with localcontext() as ctx:
        ctx.prec=50
        for i,j in zip(tour,tour[1:]+tour[:1]):
            x,y=nodes[i];a,b=nodes[j]
            d=((x-a)**2+(y-b)**2).sqrt()
            total+=int((d+Decimal('0.5')).to_integral_value(rounding=ROUND_FLOOR))
    return total


def run(name,seed,nodes,matrix):
    payload=str(len(nodes))+'\n'+'\n'.join(' '.join(map(str,row)) for row in matrix)+'\n'
    start=time.perf_counter()
    result=subprocess.run([str(OUT/'worker_tsp.exe'),str(seed)],input=payload,
                          text=True,capture_output=True,check=True,timeout=120)
    r=json.loads(result.stdout)
    r.update(instance=name,seed=seed,wall_seconds=time.perf_counter()-start)
    value=validate(nodes,r['tour'])
    if value!=r['cost'] or value<OPTIMA[name] or r['bb_calls']!=0:
        raise ValueError('Invalid result')
    r['gap_pct']=100*(value-OPTIMA[name])/OPTIMA[name]
    return r


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    sources=['SA.c','cv.c','drone_bb.c','lerInstancias.c','scripts/worker_tsp.c']
    hashes={s:hashlib.sha256((ROOT/s).read_bytes()).hexdigest() for s in sources}
    subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2',
                    *sources,'-o',str(OUT/'worker_tsp.exe'),'-lm'],cwd=ROOT,check=True)
    instances={name:read(name) for name in OPTIMA}
    print('Eight official optimum tours and complete EUC_2D matrices verified.',flush=True)
    start=time.perf_counter();runs=[]
    with (OUT/'runs.jsonl').open('w') as f, ThreadPoolExecutor(max_workers=6) as pool:
        futures=[pool.submit(run,name,seed,*instances[name]) for name in OPTIMA for seed in SEEDS]
        for future in as_completed(futures):
            r=future.result();runs.append(r);f.write(json.dumps(r)+'\n');f.flush()
            if len(runs)%10==0: print(f'{len(runs)}/80 runs completed',flush=True)
    rows=[]
    for name,optimum in OPTIMA.items():
        group=[r for r in runs if r['instance']==name]
        values=[r['cost'] for r in group];best=min(group,key=lambda r:r['cost'])
        row=dict(instance=name,n=len(instances[name][0]),optimum=optimum,
            initial=group[0]['initial'],best=min(values),mean=statistics.mean(values),
            worst=max(values),stdev=statistics.stdev(values),
            best_gap_pct=100*(min(values)-optimum)/optimum,
            mean_gap_pct=100*(statistics.mean(values)-optimum)/optimum,
            optimal_runs=sum(v==optimum for v in values),
            improved_initial_runs=sum(r['cost']<r['initial'] for r in group),
            mean_seconds=statistics.mean(r['wall_seconds'] for r in group))
        rows.append(row)
        (OUT/f'{name}_best.json').write_text(json.dumps(best,indent=2)+'\n')
    with (OUT/'comparison.csv').open('w',newline='') as f:
        w=csv.DictWriter(f,fieldnames=list(rows[0]));w.writeheader();w.writerows(rows)
    if hashes!={s:hashlib.sha256((ROOT/s).read_bytes()).hexdigest() for s in sources}:
        raise ValueError('Sources changed during run')
    meta=dict(seeds=SEEDS,workers=6,wall_seconds=time.perf_counter()-start,
        platform=platform.platform(),python=platform.python_version(),source_sha256=hashes,
        parameters=dict(alpha=.99,beta=1.1,gamma=.95,saMax=500,tempFinal=.01,tempInitial='existing calibration'),
        completed_runs=len(runs),optimal_runs=sum(r['optimal_runs'] for r in rows),
        instances_with_optimum=sum(r['best']==r['optimum'] for r in rows),
        macro_best_gap=statistics.mean(r['best_gap_pct'] for r in rows),
        macro_mean_gap=statistics.mean(r['mean_gap_pct'] for r in rows))
    (OUT/'summary.json').write_text(json.dumps(meta,indent=2)+'\n')
    report='''# Avaliacao do SA em outro problema: TSPLIB — 17/11/2026

## Protocolo

Oito instancias EUC_2D escolhidas antes das execucoes, de 52 a 280 cidades.
Dez sementes fixas (1 a 10) por instancia, uma execucao completa do resfriamento
original por semente, sem ajuste de parametros por instancia e sem limite de
tempo truncando trajetorias. Limite externo de seguranca de 120 s por processo:
qualquer estouro interromperia o experimento com erro, sem resultado parcial.
Seis processos C concorrentes. Valores medidos sao custos TSPLIB, nao minutos.

O adaptador worker_tsp.c recebe matriz simetrica de custos, configura um unico
veiculo e torna toda atribuicao a drones inviavel. Liga SA.c, cv.c e drone_bb.c
originais sem alterar seu conteudo. Preserva construcao inicial, 2-opt inicial,
movimentos, calibracao, resfriamento e criterio de aceitacao. O BB nao e chamado.
Nao e uma validacao do escalonamento de drones ou da frota multipla.

Distancias oficiais EUC_2D: floor(sqrt(dx^2+dy^2)+0.5) em cada aresta.
Nao se usa Manhattan nem distancia euclidiana continua. Todas as matrizes foram
conferidas com Decimal de 50 digitos. As oito rotas otimas baixadas da TSPLIB
foram reconstruidas e coincidem com os valores publicados. Cada solucao do SA
foi validada em C e novamente em Python/Decimal, incluindo cobertura e custo.

## Resultados

| Instancia | Otimo | Inicial | Melhor SA | Media SA | Pior SA | Gap melhor (%) | Gap medio (%) | Otimos/10 | Tempo medio (s) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
'''
    for r in rows:
        report+=f"| {r['instance']} | {r['optimum']} | {r['initial']} | {r['best']} | {r['mean']:.1f} | {r['worst']} | {r['best_gap_pct']:.2f} | {r['mean_gap_pct']:.2f} | {r['optimal_runs']} | {r['mean_seconds']:.3f} |\n"
    report+=f'''
Gap = 100 * (custo - otimo) / otimo. Media nao ponderada dos melhores gaps:
{meta['macro_best_gap']:.3f}%; media nao ponderada dos gaps medios: {meta['macro_mean_gap']:.3f}%.
Otimo encontrado em {meta['instances_with_optimum']}/8 instancias e {meta['optimal_runs']}/80 execucoes.

## Interpretacao

Neste painel, o SA encontra o otimo em berlin52 (5/10 sementes) e lin105
(2/10). Melhora a construcao inicial em 71 das 80 execucoes. Os melhores gaps
de st70, kroA100 e ch130 ficam abaixo de 1%, mas ch150 chega a 2,74% e a280
a 9,38%. O comportamento e promissor nas instancias menores e intermediarias,
mas nao consistentemente competitivo em todo o painel. A diferenca entre melhor
e media tambem evidencia dependencia da semente. O painel pequeno nao permite
atribuir a degradacao apenas ao tamanho, pois a estrutura das instancias varia.

Encontrar um otimo conhecido confirma a qualidade daquela solucao; nao demonstra
capacidade geral de provar otimalidade. O painel mede qualidade e variabilidade
do roteamento sem a incerteza do benchmark PDSVRP-C. Nao demonstra superioridade
sobre heuristicas especializadas como Lin-Kernighan/LKH ou sobre o estado da arte.
A comparacao com a coluna Inicial isola o ganho do SA alem da construcao e 2-opt
ja presentes. Resultados TSP nao substituem a validacao do problema original.

## Reproducao e arquivos

```powershell
python scripts/baixar_tsplib.py
python scripts/benchmark_tsplib.py
```

Dados e rotas otimas: instances/tsplib; hashes/URLs: manifest.json nesse diretorio.
Resultados completos: output/benchmark_tsplib/runs.jsonl e comparison.csv.
Rotas melhores: *_best.json (indices internos base zero; some 1 para IDs TSPLIB).
Fontes/parametros/ambiente: output/benchmark_tsplib/summary.json.

Fonte oficial dos otimos:
https://comopt.ifi.uni-heidelberg.de/software/TSPLIB95/tsp/TSP-BEST.html

Fonte oficial dos arquivos:
https://comopt.ifi.uni-heidelberg.de/software/TSPLIB95/tsp/tspindex.html
'''
    (ROOT/'docs/auditorias/benchmark_tsplib_20261117.md').write_text(report,encoding='utf-8')
    print(json.dumps(dict(summary=meta,results=rows),indent=2),flush=True)

if __name__=='__main__': main()
