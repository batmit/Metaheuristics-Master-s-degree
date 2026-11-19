# Avaliacao do SA em outro problema: TSPLIB — 17/11/2026

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
| berlin52 | 7542 | 8219 | 7542 | 7673.5 | 8070 | 0.00 | 1.74 | 5 | 0.390 |
| st70 | 675 | 743 | 680 | 685.7 | 695 | 0.74 | 1.59 | 0 | 0.295 |
| kroA100 | 21282 | 23192 | 21381 | 21847.5 | 23166 | 0.47 | 2.66 | 0 | 0.479 |
| eil101 | 629 | 699 | 641 | 651.4 | 660 | 1.91 | 3.56 | 0 | 0.345 |
| lin105 | 14379 | 15468 | 14379 | 14885.2 | 15452 | 0.00 | 3.52 | 2 | 0.481 |
| ch130 | 6110 | 6376 | 6129 | 6311.4 | 6376 | 0.31 | 3.30 | 0 | 0.509 |
| ch150 | 6528 | 7103 | 6707 | 6893.7 | 7052 | 2.74 | 5.60 | 0 | 0.539 |
| a280 | 2579 | 2883 | 2821 | 2861.9 | 2883 | 9.38 | 10.97 | 0 | 0.773 |

Gap = 100 * (custo - otimo) / otimo. Media nao ponderada dos melhores gaps:
1.944%; media nao ponderada dos gaps medios: 4.117%.
Otimo encontrado em 2/8 instancias e 7/80 execucoes.

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
