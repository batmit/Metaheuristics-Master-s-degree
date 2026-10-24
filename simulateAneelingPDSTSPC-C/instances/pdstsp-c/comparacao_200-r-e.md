# Execucao unica: 200-r-e

Data: 23/10/2026. Instancia: 200 clientes e 12 drones.
Compilacao: GCC, `-O2 -std=c11 -Wall -Wextra -pedantic`.
Semente: 123. Solucao inicial: todos os clientes no caminhao.
Parametros: alpha=0.99, beta=1.1, gamma=0.95, saMax=500,
tempFinal=0.01; temperatura inicial calibrada a partir de 10.
Uma chamada de SA, sem repeticoes para selecionar um resultado melhor.

| Medida | Valor |
| --- | ---: |
| Makespan inicial | 3920 min |
| Makespan obtido pelo SA | 320 min |
| Temperatura calibrada (arredondada) | 50.54 |
| Tempo reportado por clock(), calibracao + SA | 7.977 s |
| Melhor solucao conhecida na referencia consultada | 212 min |
| Diferenca absoluta | +108 min |
| Gap sobre a referencia | +50.94% |

Gap = 100 * (320 - 212) / 212. O SA reduziu o makespan inicial em 91.84%,
mas ficou 50.94% acima da melhor solucao da referencia consultada.
A solucao retornada pelo SA passa pela avaliacao completa de viabilidade
no encerramento do algoritmo.

## Referencia e limites da comparacao

Montemanni, R., Dell'Amico, M., Corsini, A. (2024).
*Parallel drone scheduling vehicle routing problems with collective drones*.
Computers & Operations Research 163, 106514, Tabela 2.
[DOI](https://doi.org/10.1016/j.cor.2023.106514).
[Texto completo](https://iris.unimore.it/bitstream/11380/1328966/2/1-s2.0-S0305054823003787-main.pdf).

Para `200-r-e`, a tabela reporta 212 minutos pelo RnR de Nguyen e Ha (2023),
com tempo de 368.97 s ate a melhor solucao, e limites conhecidos [162, 212].
O valor 212 e uma solucao viavel conhecida, nao um otimo provado.
A tabela informa o melhor resultado do RnR em 30 execucoes; aqui houve
apenas uma execucao de SA. Os computadores e criterios de medicao diferem,
portanto os tempos nao demonstram uma vantagem de velocidade entre metodos.
O valor e o melhor encontrado nas fontes consultadas, sem garantia de que
nao exista atualizacao posterior ainda nao localizada.
