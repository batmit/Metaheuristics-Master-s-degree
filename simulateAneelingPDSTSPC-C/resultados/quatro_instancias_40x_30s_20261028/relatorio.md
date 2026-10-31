# Quatro instâncias — 40/40 execuções por instância

Orçamento de 30 segundos incluindo calibração, SA e reparos; três processos concorrentes. A 165-r-c usa parametrosSAFoco165 (reparo dirigido, cadeias, filtros e CP-SAT); as outras usam parametrosSAExploracao, conforme o comportamento atual da main. Cada execução começa com nova construção inicial. Os lotes são separados em blocos de dez por instância e interrompidos entre blocos para apresentar os resultados.

| Instância | Execuções | Melhor acumulado | Melhor último bloco | Média | Pior | Referência | Empates | Abaixo da referência |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 67-c-c | 40 | 76 | 76 | 76.00 | 76 | 74 | 0 | 0 |
| 165-r-c | 40 | 160 | 160 | 166.35 | 176 | 160 | 3 | 0 |
| 167-r-e | 40 | 184 | 188 | 194.12 | 216 | 180 | 0 | 0 |
| 181-r-e | 40 | 220 | 220 | 227.90 | 236 | 216 | 0 | 0 |

## Parciais acumuladas

| Repetições por instância | 67-c-c | 165-r-c | 167-r-e | 181-r-e |
|---:|---:|---:|---:|---:|
| 10 | 76 | 160 | 184 | 224 |
| 20 | 76 | 160 | 184 | 224 |
| 30 | 76 | 160 | 184 | 220 |
| 40 | 76 | 160 | 184 | 220 |

160 certificados verificados independentemente com frações exatas: cobertura, percurso e retorno, tempos da matriz, capacidade simultânea e atribuição física dos drones. Sementes, fontes, hashes, rotas e cronogramas preservados nesta pasta.

CP-SAT na 165-r-c: 75 chamadas, 3 reparos bem-sucedidos, 4 vizinhanças inviáveis, 68 encerramentos pelo prazo e 0 erros. Inviabilidade é local à vizinhança; prazo não prova inviabilidade.

Execuções com tempo decorrido acima de 35 s: 0; maior tempo registrado: 30.304 s. A construção inicial fica fora do orçamento e operações em andamento podem causar pequenos excessos.
