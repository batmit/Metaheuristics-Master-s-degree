# Experimento: 67-c-c, 400 execucoes

Foram concluidas e validadas 400 execucoes independentes com sementes distintas, derivadas de time(NULL). SA.c, cv.c e main.c permaneceram inalterados. Parametros padrao do codigo atual; quatro processos, 100 execucoes por processo.

- Melhor makespan: 76 min.
- Media: 80.230 min; mediana: 80 min.
- Pior: 84 min; desvio padrao populacional: 1.228 min.
- Melhor execucao: 1; semente: 1793028187.
- Melhor solucao: caminhao 76 min, drones 76 min.
- Referencia: UB=74 min, LB=73 min; gap do melhor: 2.703%.
- Abaixo de 74: 0; iguais a 74: 0.

Referencia: https://orlab.com.vn/download/data-and-results-of-pdstsp-c-and-pdsop-c/

Validacao independente com Fraction: cobertura unica dos 67 clientes, rota Manhattan e retorno ao deposito, duracoes tau por quantidade, capacidade simultanea e atribuicao de drones fisicos. As 400 solucoes completas ficam nos quatro arquivos solucoes_*.jsonl.

O campo seconds inclui calibracao e SA por execucao; os processos executaram em paralelo, portanto os tempos nao sao comparaveis diretamente a um experimento sem concorrencia.
