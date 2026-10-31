# Cinquenta execuções por instância — 30 segundos

150 soluções verificadas independentemente com frações exatas. Referências locais: melhores resultados conhecidos, sem ótimo comprovado. Três processos concorrentes, sementes independentes e configuração de exploração atual.

| Instância | Execuções | Melhor | Média | Pior | Referência | Empates | Melhorias |
|---|---:|---:|---:|---:|---:|---:|---:|
| 165-r-c | 50 | 160 | 167.54 | 194 | 160 | 2 | 0 |
| 167-r-e | 50 | 180 | 192.70 | 212 | 180 | 1 | 0 |
| 181-r-e | 50 | 220 | 230.62 | 274 | 216 | 0 | 0 |

Cada execução começa com nova solução inicial. O limite de 30 segundos inclui calibração e SA; a construção inicial fica fora. Fontes, hashes, sementes, rotas e cronogramas preservados neste diretório.

Execuções com tempo decorrido acima de 35 s: 6. Maior tempo registrado: 10144.400 s.

## Comparação com o lote anterior de 20 execuções

| Instância | Melhor anterior | Melhor atual |
|---|---:|---:|
| 165-r-c | 160 | 160 |
| 167-r-e | 184 | 180 |
| 181-r-e | 224 | 220 |

Nenhuma solução superou as referências locais. As 150 soluções passaram pelo validador independente com frações exatas, que verifica cobertura única, rota e retorno ao depósito, tempos das missões, capacidade simultânea e atribuição física dos drones.

Houve pausas no avanço do lote e seis execuções registraram tempos acima de 35 segundos. A causa das pausas não foi determinada. Esses resultados foram mantidos e validados, mas o lote não deve ser tratado como comparação rigorosa de desempenho com orçamento efetivo de 30 segundos. Os tempos individuais estão em execucoes.csv e anomalias_tempo.json.
