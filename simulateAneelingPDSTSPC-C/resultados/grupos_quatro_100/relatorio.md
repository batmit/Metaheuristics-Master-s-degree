# Transferencias em grupos nas quatro instancias abertas

Foram concluidas **100 execucoes com grupos e 100 sem grupos para cada instancia**: 400 execucoes novas e 400 controles pareados. Todos os 800 certificados passaram pela validacao independente com fracoes exatas.

As quatro configuracoes da main agora usam `movimentosGrupo=true`. A API conserva a opcao para desligar grupos. O algoritmo e exatamente o integrado no ensaio anterior; nao foram adicionados folga, diversidade de elites, reparo de carga ou retomadas.

## Metodo

- Fontes de producao congelados em `fontes/`, compilados em C11 com GCC 10.3.0 e `-O2`.
- Controle e tratamento diferem somente em `ParametrosSA.movimentosGrupo`.
- Mesmas 100 sementes dentro de cada comparacao; faixas distintas entre instancias, originadas do relogio e ajustadas para nao repetir os lotes recentes. Sementes e hashes constam do manifesto e dos registros.
- Resfriamento completo, sem limite adicional de tempo, sem reduzir o numero de propostas ou o orcamento do B&B.
- Quatro processos concorrentes, blocos de cinco, alternando ordem de casos e versoes. Os tempos registrados incluem construcao inicial, calibracao e SA sob concorrencia; nao sao um benchmark de tempo isolado.
- O controle foi executado novamente; os resultados nao foram comparados apenas com melhores historicos obtidos com outras sementes.

## Resultados em minutos

| Instancia | Melhor sem grupos | Melhor com grupos | Media sem grupos | Media com grupos | Reducao da media | UB literatura | Gap melhor novo |
|---|---:|---:|---:|---:|---:|---:|---:|
| 67-c-c | 76 | 76 | 80.32 | 76.12 | +5.23% | 74 | +2.70% |
| 165-r-c | 172 | 172 | 186.20 | 189.11 | -1.56% | 160 | +7.50% |
| 167-r-e | 200 | 196 | 216.97 | 217.67 | -0.32% | 180 | +8.89% |
| 181-r-e | 240 | 240 | 256.24 | 257.68 | -0.56% | 216 | +11.11% |

Percentual positivo na reducao indica melhora; negativo indica piora. Melhorar a media e melhorar o melhor resultado sao criterios diferentes.

## Comparacao por semente

| Instancia | Vitorias grupos | Empates | Derrotas grupos | Diferenca media novo - controle | IC bootstrap 95% da diferenca |
|---|---:|---:|---:|---:|---|
| 67-c-c | 95 | 5 | 0 | -4.20 | [-4.52, -3.92] |
| 165-r-c | 29 | 14 | 57 | +2.91 | [+1.19, +4.65] |
| 167-r-e | 40 | 8 | 52 | +0.70 | [-1.45, +2.77] |
| 181-r-e | 39 | 10 | 51 | +1.44 | [-0.76, +3.64] |

O bootstrap reamostra os 100 pares de cada instancia (10.000 reamostragens), usando uma semente fixa apenas na analise. Os intervalos descrevem essa amostra e pressupõem pares representativos; nao demonstram superioridade universal. O teste de sinais descritivo esta em `resumo.json`; nao foi usado como garantia de otimalidade.

## Distribuicao final entre recursos

| Instancia | Clientes medios nos drones, antes/depois | Caminhao medio, antes/depois | Drones medio, antes/depois | Atribuicoes com limite de carga acima do UB, antes/depois |
|---|---:|---:|---:|---:|
| 67-c-c | 28.51 / 13.70 | 80.32 / 76.12 | 78.28 / 72.89 | 0 / 0 |
| 165-r-c | 89.77 / 74.03 | 186.08 / 189.08 | 186.05 / 188.68 | 97 / 24 |
| 167-r-e | 71.32 / 53.57 | 216.96 / 217.64 | 216.65 / 216.70 | 86 / 13 |
| 181-r-e | 65.73 / 50.05 | 256.24 / 257.64 | 255.72 / 256.28 | 81 / 14 |

O ultimo indicador conta, em cada lote de 100, os conjuntos de clientes dos drones cuja soma de min_k(k*tau_j(k))/m ja excede o UB publicado. Nesses estados finais, melhorar somente quantidades ou ordem dos drones nao basta para atingir a referencia; e preciso mudar a atribuicao de clientes. Os indicadores descrevem os estados finais, sem estabelecer a causa da diferenca de desempenho.

## Analise por instancia

- **67-c-c**: a media melhorou, de 80.32 para 76.12; o melhor empatou, de 76 para 76. Grupos ganhou 95, empatou 5 e perdeu 0 pares. Resultados iguais ao UB publicado: 0/100; abaixo: 0/100.
- **165-r-c**: a media piorou, de 186.20 para 189.11; o melhor empatou, de 172 para 172. Grupos ganhou 29, empatou 14 e perdeu 57 pares. Resultados iguais ao UB publicado: 0/100; abaixo: 0/100.
- **167-r-e**: a media piorou, de 216.97 para 217.67; o melhor melhorou, de 200 para 196. Grupos ganhou 40, empatou 8 e perdeu 52 pares. Resultados iguais ao UB publicado: 0/100; abaixo: 0/100.
- **181-r-e**: a media piorou, de 256.24 para 257.68; o melhor empatou, de 240 para 240. Grupos ganhou 39, empatou 10 e perdeu 51 pares. Resultados iguais ao UB publicado: 0/100; abaixo: 0/100.

Esses resultados avaliam o efeito da nova vizinhanca na qualidade final. Nao isolam a causa mecanica de eventuais regressoes: o sorteio de grupos modifica a trajetoria aleatoria e os pesos dos tipos de transferencia, e uma investigacao causal exigiria instrumentacao adicional. Ganhos observados na 67-c-c nao devem ser extrapolados automaticamente para as maiores.

## Referencias e validacao

Referencias do pacote oficial ORLab de 2025: 67-c-c [73,74], 165-r-c [156,160], 167-r-e [179,180] e 181-r-e [215,216]. O UB nao e um otimo comprovado. Fonte: https://orlab.com.vn/download/data-and-results-of-pdstsp-c-and-pdsop-c/

A validacao independente recalcula cobertura unica dos clientes, rota Manhattan e retorno ao deposito, duracoes da matriz tau, capacidade simultanea e uma atribuicao de drones fisicos sem interrupcao de missoes. Nao chama o avaliador C. Os fontes de producao e dados permaneceram com os hashes registrados durante o lote.

Arquivos principais: `comparacao.csv` e `comparacao.json` (resumo), `execucoes.csv` (800 execucoes), `*.jsonl` (todos os vetores), `melhor_*_grupos.json` (melhor certificado de cada instancia), `manifesto.json` e `fontes/` (reproducao). A tabela de 400 execucoes com grupos tambem foi salva em `resultados/resumo_quatro_abertas_grupos_100_execucoes.csv`.
