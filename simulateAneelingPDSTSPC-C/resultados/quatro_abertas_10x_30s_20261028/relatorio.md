# Quatro instâncias sem ótimo comprovado — dez execuções de 30 segundos

40 soluções verificadas com frações exatas. Configuração de exploração atual (máscara 15), três processos concorrentes e sementes independentes. Os valores de referência são os limites superiores do pacote ORLab de 2025 preservados no projeto.

| Instância | Execuções | Melhor | Média | Pior | Referência | Empates | Abaixo da referência |
|---|---:|---:|---:|---:|---:|---:|---:|
| 67-c-c | 10 | 76 | 76.00 | 76 | 74 | 0 | 0 |
| 165-r-c | 10 | 162 | 166.80 | 170 | 160 | 0 | 0 |
| 167-r-e | 10 | 184 | 192.40 | 204 | 180 | 0 | 0 |
| 181-r-e | 10 | 220 | 227.60 | 236 | 216 | 0 | 0 |

Cada execução começa com nova solução inicial. O limite configurado de 30 segundos inclui calibração e SA; a construção inicial fica fora. Fontes, hashes, sementes, rotas e cronogramas preservados neste diretório.

As soluções foram verificadas independentemente: cobertura única dos clientes, rota e retorno ao depósito, tempos das missões, capacidade simultânea e atribuição física dos drones.

Execuções com tempo decorrido acima de 35 s: 0. Maior tempo registrado: 30.037 s.

## Comparação com os melhores anteriores usados neste lote

| Instância | Melhor anterior | Melhor atual |
|---|---:|---:|
| 67-c-c | 76 | 76 |
| 165-r-c | 160 | 162 |
| 167-r-e | 180 | 184 |
| 181-r-e | 220 | 220 |

Resultados abaixo das referências: 0.
