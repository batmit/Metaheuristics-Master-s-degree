"""Summarize independent truck-only baseline, without claiming global records."""
import csv
import hashlib
import json
from executar_benchmark import ROOT


def main():
    folder=ROOT/'output/benchmark_caminhoes_gls'
    rows=list(csv.DictReader((folder/'comparison.csv').open()))
    meta=json.loads((folder/'summary.json').read_text())
    old=list(csv.DictReader((ROOT/'output/auditoria_professor/melhorias.csv').open()))
    comparison=[]
    for previous in old:
        current=next(r for r in rows if r['instance']==previous['instance'] and r['trucks']==previous['trucks'])
        a,b=float(previous['local']),float(current['best'])
        comparison.append(dict(instance=current['instance'],trucks=current['trucks'],
            previous=a,gls=b,outcome='better' if b<a else 'equal' if b==a else 'worse'))
    with (folder/'comparison_previous14.csv').open('w',newline='') as f:
        w=csv.DictWriter(f,fieldnames=list(comparison[0]));w.writeheader();w.writerows(comparison)
    counts={k:sum(r['outcome']==k for r in rows) for k in ['better','equal','worse']}
    old_counts={k:sum(r['outcome']==k for r in comparison) for k in counts}
    wins=[r for r in rows if r['outcome']=='better']
    raw_wins=sum(float(r['best'])<float(r['published_ub']) for r in rows)
    text=f'''# Referencia independente somente com caminhoes — 17/11/2026

## Metodo e escopo

OR-Tools Routing {meta['version']}, Guided Local Search (GLS). Todas as 180 configuracoes
do catalogo: 60 pequenas e 120 grandes. Tres inicializacoes por configuracao
(PARALLEL_CHEAPEST_INSERTION, GLOBAL_CHEAPEST_ARC, PATH_CHEAPEST_ARC), cada uma com
limite de {meta['seconds_per_run']:g} segundos. Total de {meta['runs']} execucoes;
{meta['workers']} processos concorrentes; {meta['wall_seconds']:.1f} segundos de parede
para a campanha de otimizacao, excluindo instalacao e validacao final.
Sao tres estrategias, nao tres sementes aleatorias independentes.

Nenhuma rota anterior foi fornecida ao solver. Nao se modificou o SA+BB nem o
registro central. Clientes obrigatorios, deposito unico, no maximo S caminhoes,
capacidade ilimitada, zero drones. Distancia Manhattan, velocidade do arquivo,
minutos. Todas as arestas sao inteiras verificadas com Decimal; nenhum
arredondamento ou troca de metrica foi introduzido.

Objetivo lexicografico W * makespan + soma dos tempos. Com N clientes e S
caminhoes, a soma tem no maximo N+S arestas, cada uma limitada por max_arc.
W=(N+S)*max_arc+1 assegura prioridade absoluta ao makespan inteiro. A soma
apenas desempata; nao se compara o objetivo ponderado com os valores publicados.

## Resultados

- Referencia publicada diretamente: {raw_wins} configuracoes com valor menor.
- Referencia fortalecida por frota: {counts['better']} melhores, {counts['equal']} iguais,
  {counts['worse']} piores. Usa-se o menor UB catalogado para a mesma instancia
  com no maximo S caminhoes; caminhoes adicionais podem ficar ociosos.
- Pequenas: {meta['counts']['small']}.
- Grandes: {meta['counts']['large']}.
- Das 14 configuracoes destacadas antes, {meta['previous14_beaten']} tambem melhoram
  a referencia fortalecida usando esta abordagem independente.
- {meta['new_configurations']} melhorias ocorrem fora daquelas 14 configuracoes.
- Contra os valores dos 14 certificados anteriores: {old_counts['better']} melhores,
  {old_counts['equal']} iguais e {old_counts['worse']} piores.

Todos os 540 certificados retornados foram submetidos ao validador Python.
Os 180 melhores passaram adicionalmente pelo avaliador C recompilado e pela
reconstrucao dos custos inteiros a partir de Decimal, com cobertura exata.
Os resultados sao heuristicas viaveis; nao certificados de otimalidade.

## Melhorias contra a referencia fortalecida

| Instancia | Caminhoes | UB publicado | UB fortalecido | GLS | Reducao (%) |
| --- | ---: | ---: | ---: | ---: | ---: |
'''
    for r in wins:
        text+=f"| {r['instance']} | {r['trucks']} | {float(r['published_ub']):g} | {float(r['monotone_ub']):g} | {r['best']} | {float(r['improvement_pct']):.2f} |\n"
    text+='''
## Interpretacao e limites

Uma melhoria por uma referencia independente reforca a existencia de margem nos
UBs consultados, mas nao estabelece uma contribuicao exclusiva do SA+BB. O teste
nao permite declarar superioridade estatistica entre os algoritmos: orcamentos,
inicializacoes e trajetorias nao foram equiparados ao experimento anterior.
O melhor de tres inicializacoes representa um orcamento de 15 segundos por
configuracao, nao uma execucao unica de cinco segundos.

Os benchmarks locais sao os oficiais distribuidos, mas os arquivos exatos
executados em COR/MIC 2024 continuam pendentes. Nao se afirma recorde mundial
atual. As matrizes tau nao afetam a viabilidade destas rotas sem drones.
Solucoes piores nao mostram inviabilidade de melhorar; refletem o limite de
busca e, em parte, a ausencia de drones. As discrepancias conhecidas da
126-rc-e continuam merecendo esclarecimento com os autores.

## Reproducao

```powershell
python -m venv output/routing_env
output/routing_env/Scripts/python.exe -m pip install -r scripts/requirements-routing.txt
output/routing_env/Scripts/python.exe scripts/benchmark_caminhoes.py --seconds 5 --workers 6
python scripts/relatar_caminhoes.py
```

Execucoes limitadas por tempo podem variar conforme carga/hardware. Versoes e
parametros estao em output/benchmark_caminhoes_gls/summary.json; todos os runs
em runs.jsonl; comparacao integral em comparison.csv; rotas em *_s*.json;
comparacao com os 14 certificados em comparison_previous14.csv.

## Fontes

- OR-Tools, GLS e estrategias: https://developers.google.com/optimization/routing/routing_options
- OR-Tools, min-max VRP e dimensao: https://developers.google.com/optimization/routing/vrp
- COR 2024: https://doi.org/10.1016/j.cor.2023.106514
- MIC 2024: https://doi.org/10.1007/978-3-031-62912-9_13
- Valores consultados: instances/literature_pdsvrp_c.csv.
'''
    (ROOT/'docs/auditorias/benchmark_caminhoes_20261117.md').write_text(text,encoding='utf-8')
    manifest={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [
        ROOT/'scripts/benchmark_caminhoes.py',ROOT/'scripts/requirements-routing.txt',
        ROOT/'instances/literature_pdsvrp_c.csv',folder/'runs.jsonl',folder/'comparison.csv']}
    (folder/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(json.dumps(dict(total=counts,versus_previous14=old_counts,wins=wins),indent=2))


if __name__=='__main__':
    main()
