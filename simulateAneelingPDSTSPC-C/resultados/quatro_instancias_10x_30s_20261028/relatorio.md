# Quatro instâncias — 10/10 execuções por instância

Orçamento de 30 segundos incluindo calibração, SA e reparos; três processos concorrentes. A 165-r-c usa parametrosSAFoco165 (reparo dirigido, cadeias, filtros e CP-SAT); as outras usam parametrosSAExploracao, conforme o comportamento atual da main. Cada execução começa com nova construção inicial. Os lotes são separados em blocos de dez por instância e interrompidos entre blocos para apresentar os resultados.

| Instância | Execuções | Melhor acumulado | Melhor último bloco | Média | Pior | Referência | Empates | Abaixo da referência |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 67-c-c | 10 | 76 | 76 | 76.00 | 76 | 74 | 0 | 0 |
| 165-r-c | 10 | 164 | 164 | 165.40 | 172 | 160 | 0 | 0 |
| 167-r-e | 10 | 184 | 184 | 196.00 | 236 | 180 | 0 | 0 |
| 181-r-e | 10 | 220 | 220 | 236.50 | 280 | 216 | 0 | 0 |

## Parciais acumuladas

| Repetições por instância | 67-c-c | 165-r-c | 167-r-e | 181-r-e |
|---:|---:|---:|---:|---:|
| 10 | 76 | 164 | 184 | 220 |

40 certificados verificados independentemente com frações exatas: cobertura, percurso e retorno, tempos da matriz, capacidade simultânea e atribuição física dos drones. Sementes, fontes, hashes, rotas e cronogramas preservados nesta pasta.

CP-SAT na 165-r-c: 20 chamadas, 0 reparos bem-sucedidos, 0 vizinhanças inviáveis, 20 encerramentos pelo prazo e 0 erros. Inviabilidade é local à vizinhança; prazo não prova inviabilidade.

Execuções com tempo decorrido acima de 35 s: 4; maior tempo registrado: 1156.373 s. A construção inicial fica fora do orçamento e operações em andamento podem causar pequenos excessos.

## Ocorrências de tempo

Quatro execuções registraram mais de 35 segundos decorridos durante intervalos prolongados no andamento do ambiente. A causa não foi determinada. Seus certificados foram mantidos e validados, mas o lote não deve ser tratado como comparação rigorosa com orçamento efetivo uniforme de 30 segundos. As sementes e os tempos estão em anomalias_tempo.json.
