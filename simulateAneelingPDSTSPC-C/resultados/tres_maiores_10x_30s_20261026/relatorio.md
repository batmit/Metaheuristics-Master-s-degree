# Dez execucoes das tres instancias maiores — 30 segundos

Configuracao atual, com trocas simultaneas, reconstrucao parcial, calibracao mista, resfriamento por tempo e elites diversas. Foram realizadas dez execucoes independentes em cada instancia, sempre a partir da construcao inicial. As sementes originadas do relogio, os fontes e os hashes dos dados estao preservados no manifesto e em fontes/.

Cada execucao recebeu 30 segundos de parede para calibracao e SA. A construcao inicial fica fora desse limite. Foram usados tres processos concorrentes, intercalando as instancias. Operacoes em andamento sao concluidas antes do encerramento, podendo exceder ligeiramente o prazo. Nao houve alteracao dos parametros ou fontes durante o lote.

Os 30 certificados passaram pela verificacao independente com fracoes exatas: cobertura unica, rota Manhattan e retorno ao deposito, tempos da matriz, capacidade simultanea e atribuicao fisica dos drones. O validador nao chama o avaliador C.

## Resultados individuais em minutos

| Execucao | 165-r-c | 167-r-e | 181-r-e |
|---|---:|---:|---:|
| 1 | 164 | 188 | 232 |
| 2 | 164 | 203 | 224 |
| 3 | 166 | 180 | 224 |
| 4 | 165 | 188 | 228 |
| 5 | 168 | 196 | 232 |
| 6 | 164 | 188 | 228 |
| 7 | 161 | 196 | 232 |
| 8 | 168 | 200 | 232 |
| 9 | 168 | 196 | 220 |
| 10 | 168 | 196 | 228 |

## Comparacao com as referencias

| Instancia | Melhor | Media | Mediana | Pior | Melhor literatura | Gap do melhor |
|---|---:|---:|---:|---:|---:|---:|
| 165-r-c | 161 | 165.60 | 165.5 | 168 | 160 | 0.62% |
| 167-r-e | 180 | 193.10 | 196 | 203 | 180 | 0.00% |
| 181-r-e | 220 | 228.00 | 228 | 232 | 216 | 1.85% |

Gap = 100 × (resultado − referencia) / referencia. As referencias sao os limites superiores 160, 180 e 216 usados no projeto a partir do pacote oficial ORLab de 2025. Nao sao otimos comprovados nesses tres casos.

- 165-r-c: iguais a referencia 0/10; abaixo 0/10. Melhor na execucao 7, semente 1793050725. Melhor do lote anterior de 10 s: 164; melhor deste lote: 161.
- 167-r-e: iguais a referencia 1/10; abaixo 0/10. Melhor na execucao 3, semente 1793050731. Melhor do lote anterior de 10 s: 188; melhor deste lote: 180.
- 181-r-e: iguais a referencia 0/10; abaixo 0/10. Melhor na execucao 9, semente 1793050747. Melhor do lote anterior de 10 s: 220; melhor deste lote: 220.

A comparacao com o lote anterior de 10 segundos e descritiva: o tempo, as sementes e a concorrencia diferem; ela nao isola causalmente o efeito de aumentar o orcamento. Dez execucoes tambem nao demonstram que repetir mais vezes alcancara a literatura.

Arquivos: execucoes.csv contem as 30 execucoes e sementes; resumo.csv e resumo.json apresentam estatisticas; melhor_*.json guarda cada melhor solucao completa; *.jsonl contem todos os vetores e cronogramas. Nenhuma fonte de producao foi alterada para executar este lote.

Fonte das referencias: [ORLab — dados e resultados PDSTSP-c/PDSOP-c](https://orlab.com.vn/download/data-and-results-of-pdstsp-c-and-pdsop-c/).
