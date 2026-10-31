# 165-r-c — algoritmo novo, 100 execuções de 30 segundos

Perfil parametrosSAFoco165, com reparo dirigido, cadeias de transferências, filtros e CP-SAT. Todas as execuções partiram de nova construção inicial, sem usar a solução conhecida de 160 como partida. Três processos concorrentes e sementes independentes preservadas no manifesto.

| Execuções validadas | Melhor | Média | Mediana | Pior | Iguais a 160 | Abaixo de 160 |
|---:|---:|---:|---:|---:|---:|---:|
| 100 | 160 | 166.14 | 165 | 178 | 10 | 0 |

CP-SAT: 200 chamadas, 4 sucessos, 14 vizinhanças inviáveis, 182 encerramentos pelo prazo e 0 erros. Inviabilidade é local à vizinhança, e prazo não prova inviabilidade.

O prazo configurado inclui calibração, busca e reparos. A construção inicial fica fora. As operações em andamento podem provocar pequeno excesso de tempo.

Execuções acima de 35 segundos decorridos: 0; maior tempo observado: 32.119 s.

## Melhores acumulados a cada dez repetições

| Repetições | Melhor | Média | Empates em 160 | Abaixo de 160 |
|---:|---:|---:|---:|---:|
| 10 | 164 | 168.90 | 0 | 0 |
| 20 | 164 | 167.50 | 0 | 0 |
| 30 | 160 | 166.77 | 2 | 0 |
| 40 | 160 | 166.65 | 2 | 0 |
| 50 | 160 | 166.28 | 4 | 0 |
| 60 | 160 | 166.23 | 5 | 0 |
| 70 | 160 | 166.40 | 5 | 0 |
| 80 | 160 | 166.21 | 6 | 0 |
| 90 | 160 | 166.24 | 7 | 0 |
| 100 | 160 | 166.14 | 10 | 0 |

Todos os certificados foram verificados independentemente com frações exatas: cobertura, percurso Manhattan e retorno, tempos da matriz, capacidade simultânea e atribuição física dos drones. Fontes, hashes, sementes, rotas e cronogramas estão preservados nesta pasta. Não houve ajuste de parâmetros durante o lote.
