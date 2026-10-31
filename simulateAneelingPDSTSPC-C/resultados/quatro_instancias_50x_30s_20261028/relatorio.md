# Quatro instâncias — 50/50 execuções por instância

Orçamento de 30 segundos incluindo calibração, SA e reparos; três processos concorrentes. A 165-r-c usa parametrosSAFoco165 (reparo dirigido, cadeias, filtros e CP-SAT); as outras usam parametrosSAExploracao, conforme o comportamento atual da main. Cada execução começa com nova construção inicial. Os lotes são separados em blocos de dez por instância e interrompidos entre blocos para apresentar os resultados.

| Instância | Execuções | Melhor acumulado | Melhor último bloco | Média | Pior | Referência | Empates | Abaixo da referência |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 67-c-c | 50 | 76 | 76 | 76.00 | 76 | 74 | 0 | 0 |
| 165-r-c | 50 | 160 | 160 | 165.92 | 176 | 160 | 4 | 0 |
| 167-r-e | 50 | 180 | 184 | 192.42 | 208 | 180 | 1 | 0 |
| 181-r-e | 50 | 220 | 224 | 229.36 | 236 | 216 | 0 | 0 |

## Parciais acumuladas

| Repetições por instância | 67-c-c | 165-r-c | 167-r-e | 181-r-e |
|---:|---:|---:|---:|---:|
| 10 | 76 | 160 | 188 | 220 |
| 20 | 76 | 160 | 184 | 220 |
| 30 | 76 | 160 | 184 | 220 |
| 40 | 76 | 160 | 180 | 220 |
| 50 | 76 | 160 | 180 | 220 |

200 certificados verificados independentemente com frações exatas: cobertura, percurso e retorno, tempos da matriz, capacidade simultânea e atribuição física dos drones. Sementes, fontes, hashes, rotas e cronogramas preservados nesta pasta.

CP-SAT na 165-r-c: 100 chamadas, 6 reparos bem-sucedidos, 12 vizinhanças inviáveis, 82 encerramentos pelo prazo e 0 erros. Inviabilidade é local à vizinhança; prazo não prova inviabilidade.

Execuções com tempo decorrido acima de 35 s: 0; maior tempo registrado: 30.273 s. A construção inicial fica fora do orçamento e operações em andamento podem causar pequenos excessos.
