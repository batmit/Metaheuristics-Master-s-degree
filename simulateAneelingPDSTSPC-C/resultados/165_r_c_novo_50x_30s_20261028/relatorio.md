# 165-r-c — algoritmo novo, 50 execuções de 30 segundos

Perfil parametrosSAFoco165, com reparo dirigido, cadeias de transferências, filtros e CP-SAT. Todas as execuções partiram de nova construção inicial, sem usar a solução conhecida de 160 como partida. Três processos concorrentes e sementes independentes preservadas no manifesto.

| Execuções validadas | Melhor | Média | Mediana | Pior | Iguais a 160 | Abaixo de 160 |
|---:|---:|---:|---:|---:|---:|---:|
| 50 | 160 | 165.06 | 164 | 172 | 4 | 0 |

CP-SAT: 100 chamadas, 7 sucessos, 12 vizinhanças inviáveis, 81 encerramentos pelo prazo e 0 erros. Inviabilidade é local à vizinhança, e prazo não prova inviabilidade.

O prazo configurado inclui calibração, busca e reparos. A construção inicial fica fora. As operações em andamento podem provocar pequeno excesso de tempo.

Execuções acima de 35 segundos decorridos: 0; maior tempo observado: 30.025 s.

## Melhores acumulados a cada dez repetições

| Repetições | Melhor | Média | Empates em 160 | Abaixo de 160 |
|---:|---:|---:|---:|---:|
| 10 | 160 | 164.80 | 1 | 0 |
| 20 | 160 | 165.20 | 1 | 0 |
| 30 | 160 | 165.20 | 2 | 0 |
| 40 | 160 | 164.82 | 3 | 0 |
| 50 | 160 | 165.06 | 4 | 0 |

Todos os certificados foram verificados independentemente com frações exatas: cobertura, percurso Manhattan e retorno, tempos da matriz, capacidade simultânea e atribuição física dos drones. Fontes, hashes, sementes, rotas e cronogramas estão preservados nesta pasta. Não houve ajuste de parâmetros durante o lote.
