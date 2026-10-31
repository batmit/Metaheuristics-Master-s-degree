# Cem execuções por instância — 30 segundos

300 soluções verificadas independentemente com frações exatas. Referências locais: melhores resultados conhecidos, sem ótimo comprovado. Três processos concorrentes, sementes independentes e configuração de exploração atual.

| Instância | Execuções | Melhor | Média | Pior | Referência | Empates | Melhorias |
|---|---:|---:|---:|---:|---:|---:|---:|
| 165-r-c | 100 | 160 | 167.73 | 188 | 160 | 4 | 0 |
| 167-r-e | 100 | 180 | 194.21 | 240 | 180 | 1 | 0 |
| 181-r-e | 100 | 220 | 231.18 | 288 | 216 | 0 | 0 |

Cada execução começa com nova solução inicial. O limite de 30 segundos inclui calibração e SA; a construção inicial fica fora. Fontes, hashes, sementes, rotas e cronogramas preservados neste diretório.

Execuções com tempo decorrido acima de 35 s: 21. Maior tempo registrado: 36723.575 s.

## Comparação com o lote anterior de 50 execuções

| Instância | Melhor anterior | Melhor atual |
|---|---:|---:|
| 165-r-c | 160 | 160 |
| 167-r-e | 180 | 180 |
| 181-r-e | 220 | 220 |

## Melhores resultados acumulados a cada dez repetições

| Repetições por instância | 165-r-c | 167-r-e | 181-r-e |
|---|---:|---:|---:|
| 10 | 164 | 180 | 220 |
| 20 | 164 | 180 | 220 |
| 30 | 160 | 180 | 220 |
| 40 | 160 | 180 | 220 |
| 50 | 160 | 180 | 220 |
| 60 | 160 | 180 | 220 |
| 70 | 160 | 180 | 220 |
| 80 | 160 | 180 | 220 |
| 90 | 160 | 180 | 220 |
| 100 | 160 | 180 | 220 |

As 300 soluções passaram pelo validador independente com frações exatas, verificando cobertura única, rota e retorno ao depósito, tempos das missões, capacidade simultânea e atribuição física dos drones.

Resultados abaixo das referências: 0.

21 execuções excederam 35 segundos decorridos. A causa não foi determinada. Os resultados foram mantidos e validados; os tempos efetivos estão em execucoes.csv e anomalias_tempo.json.
