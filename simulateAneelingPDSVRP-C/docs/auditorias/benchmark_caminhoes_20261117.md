# Referencia independente somente com caminhoes — 17/11/2026

## Metodo e escopo

OR-Tools Routing 9.15.6755, Guided Local Search (GLS). Todas as 180 configuracoes
do catalogo: 60 pequenas e 120 grandes. Tres inicializacoes por configuracao
(PARALLEL_CHEAPEST_INSERTION, GLOBAL_CHEAPEST_ARC, PATH_CHEAPEST_ARC), cada uma com
limite de 5 segundos. Total de 540 execucoes;
6 processos concorrentes; 453.6 segundos de parede
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

- Referencia publicada diretamente: 5 configuracoes com valor menor.
- Referencia fortalecida por frota: 5 melhores, 46 iguais,
  129 piores. Usa-se o menor UB catalogado para a mesma instancia
  com no maximo S caminhoes; caminhoes adicionais podem ficar ociosos.
- Pequenas: {'better': 0, 'equal': 16, 'worse': 44}.
- Grandes: {'better': 5, 'equal': 30, 'worse': 85}.
- Das 14 configuracoes destacadas antes, 2 tambem melhoram
  a referencia fortalecida usando esta abordagem independente.
- 3 melhorias ocorrem fora daquelas 14 configuracoes.
- Contra os valores dos 14 certificados anteriores: 1 melhores,
  1 iguais e 12 piores.

Todos os 540 certificados retornados foram submetidos ao validador Python.
Os 180 melhores passaram adicionalmente pelo avaliador C recompilado e pela
reconstrucao dos custos inteiros a partir de Decimal, com cobertura exata.
Os resultados sao heuristicas viaveis; nao certificados de otimalidade.

## Melhorias contra a referencia fortalecida

| Instancia | Caminhoes | UB publicado | UB fortalecido | GLS | Reducao (%) |
| --- | ---: | ---: | ---: | ---: | ---: |
| 101-r-c | 2 | 152 | 152 | 144 | 5.26 |
| 103-rc-c | 2 | 124 | 124 | 120 | 3.23 |
| 103-rc-c | 4 | 76 | 76 | 72 | 5.26 |
| 108-rc-e | 3 | 124 | 124 | 120 | 3.23 |
| 126-rc-e | 2 | 152 | 152 | 144 | 5.26 |

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
