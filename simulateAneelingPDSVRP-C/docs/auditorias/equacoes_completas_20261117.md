# Validacao das equacoes do artigo fundador: todas as instancias

Data: 17/11/2026. Objetivo em minutos; menor e melhor.

## Escopo e escolhas explicitas

Foram conferidas visualmente as pp.870-874, 880 e 883 de Nguyen e Ha, *The Parallel Drone Scheduling Traveling Salesman Problem with Collective Drones*, Transportation Science 57(4), 866-888 (2023), DOI 10.1287/trsc.2022.1192. O SHA-256 do PDF e os hashes dos fontes estao em environment.json.

Nao existe uma interpretacao literal unica e fisicamente coerente de todas as linhas impressas: a Eq.(4) imprime consumo >=90% da capacidade, mas o texto exige no maximo 90%; a Tabela 5 diz preparo de 1 segundo e o texto diz 1 minuto; a linha 14 do Algoritmo 2 repete vin onde o texto descreve vin e vout. A restricao de bateria usada neste experimento e consumo <=0,9*Cnom; os pares vin/vout sao independentes. Nao foi aplicado o sinal >= da Eq.(4), pois ele permitiria missoes acima da capacidade e excluiria algumas de baixo consumo. Assim, este experimento aplica as equacoes fisicas impressas com essas correcoes textuais explicitadas; nao afirma uma reproducao literal sem excecoes.

| Cenario | Definicao |
| --- | --- |
| Tabelas oficiais | Matrizes distribuidas, sem alteracao. |
| Equacoes + 1 min | Eq.(24) SEM o termo de arrasto quadratico adicional do C++; Eq.(25) resolvida por bissecao (80 iteracoes); preparo de 1 min para k>1. |
| Equacoes + 1 s | Mesma fisica anterior; preparo de 1 s para k>1, conforme Tabela 5. |

Em ambos os cenarios recalculados: Eq.(23) para decolagem/pouso; seis fases de voo; carga w/k na ida e zero na volta; corrente P/V limitada em todas as fases (Eq.2); Peukert (Eq.3), reserva de 10%, altitude 100 m, velocidades verticais 2 m/s; busca dos pares de velocidades entre 4 e 28 m/s com passo 0,2; Eq.(1), conversao para minutos e teto; maximo de seis drones por formacao e massa total <=25 kg. Coeficientes calibrados, geometria e aproximacao numerica de pi sao os do codigo original disponibilizado, pois o PDF nao fornece sozinho todos os detalhes executaveis da calibracao.

A potencia de cruzeiro foi pre-calculada por velocidade/carga para evitar repeti-la para cada par vin/vout. A ordem e os valores de ponto flutuante da malha original foram conservados. A reproducao do codigo distribuido continuou com exatamente as duas divergencias previamente conhecidas.

O otimizador permanece o SA+BB atual. As Eqs.(5)-(22) do MILP fundador sao representadas pelas restricoes de rotas, cobertura, duracoes e recurso cumulativo; nao foi implementado nem resolvido esse MILP ou substituido o SA pelo RnR. Com varios caminhoes, usa-se a extensao PDSVRP-C, e nao o modelo de um caminhao literal do fundador. Manhattan e velocidade dos caminhoes foram preservadas.

## Validacao fisica

Foram recalculadas 60 instancias, 38450 pares cliente/k. Todas as 19076 combinacoes viaveis da fisica impressa tiveram angulo, potencia, corrente, Peukert, reserva de bateria, massa e duracao reavaliados independentemente em Python. Esta conferencia valida a viabilidade das velocidades selecionadas; nao e uma segunda enumeracao independente de todos os pares de velocidades nem validacao experimental de drones reais.

| Matriz | Entradas diferentes da oficial |
| --- | ---: |
| Reproducao do C++ original | 2 |
| Equacoes + preparo 1 min | 4497 |
| Equacoes + preparo 1 s | 17825 |

| Cenario | Novos pares viaveis | Tempos menores em pares ja viaveis | Tempos maiores | Pares que ficaram inviaveis |
| --- | ---: | ---: | ---: | ---: |
| Equacoes + preparo 1 min | 91 | 4406 | 0 | 0 |
| Equacoes + preparo 1 s | 91 | 17734 | 0 | 0 |

As alteracoes fisicas observadas so reduzem duracoes ou acrescentam alternativas viaveis. Por isso o otimo teorico nao piora, mas uma execucao curta da heuristica pode encontrar solucao pior por seguir outra trajetoria.

## Protocolo de otimizacao

Foram executadas 240 configuracoes: as 180 do catalogo PDSVRP-C e as 60 instancias com S=1 do PDSTSP-C fundador. Para S=1, a referencia numerica e bks.txt do pacote original, sem assumir que todos os seus valores sejam otimos ou que representem resultados posteriores. Para S>1, usa-se o catalogo local COR2024/MIC2024, sem busca exaustiva de novos BKS.

Tres cenarios, 3 sementes por cenario e configuracao, 2 segundos por execucao, 4 processos simultaneos: 2160 execucoes, 8067 checkpoints validados. 788 execucoes encerraram antes do limite; as demais conservam o melhor checkpoint anterior ao corte. Semente efetiva: (42 + 10*n + S + 1000003*r) modulo 2^32, r=0,1,2.

As tabelas abaixo usam o melhor valor das tres repeticoes por configuracao. Medias percentuais sao aritmeticas dos percentuais individuais; valores negativos indicam reducao. O limite curto pode cortar a calibracao do SA, e as mudancas de tau alteram trajetorias. Esta e uma comparacao exploratoria, sem prova geral de otimalidade ou significancia estatistica. Tempos de execucao nao sao comparados diretamente com computadores e solvers dos artigos.

Certificados originais S=1 reconstruidos com sucesso e objetivo coincidente: 60/60. Detalhes e eventuais incompatibilidades estao em original_certificate_validation.json.

## Resultados agregados: PDSVRP-C

| Cenario | Gap medio vs referencia % | Abaixo / igual / acima da referencia | Melhor / igual / pior que oficial | Mudanca media vs oficial % |
| --- | ---: | ---: | ---: | ---: |
| Tabelas oficiais | 5.938 | 14 / 82 / 84 | 0 / 180 / 0 | 0.000 |
| Equacoes + preparo 1 min | 5.795 | 17 / 81 / 82 | 6 / 169 / 5 | -0.098 |
| Equacoes + preparo 1 s | 5.651 | 19 / 82 / 79 | 9 / 167 / 4 | -0.262 |

Compatibilidade necessaria: UB publicado >= limite inferior local por cliente. Passar esse teste nao demonstra identidade entre os dados dos artigos e os locais.

| Cenario | Referencias que passam o teste | Gap medio nessas referencias % |
| --- | ---: | ---: |
| Tabelas oficiais | 177/180 | 5.493 |
| Equacoes + preparo 1 min | 177/180 | 5.347 |
| Equacoes + preparo 1 s | 177/180 | 5.200 |

## Resultados agregados: PDSTSP-C

| Cenario | Gap medio vs referencia % | Abaixo / igual / acima da referencia | Melhor / igual / pior que oficial | Mudanca media vs oficial % |
| --- | ---: | ---: | ---: | ---: |
| Tabelas oficiais | 41.131 | 0 / 20 / 40 | 0 / 60 / 0 | 0.000 |
| Equacoes + preparo 1 min | 40.860 | 2 / 20 / 38 | 5 / 53 / 2 | -0.216 |
| Equacoes + preparo 1 s | 40.507 | 2 / 20 / 38 | 9 / 51 / 0 | -0.552 |

Compatibilidade necessaria: UB publicado >= limite inferior local por cliente. Passar esse teste nao demonstra identidade entre os dados dos artigos e os locais.

| Cenario | Referencias que passam o teste | Gap medio nessas referencias % |
| --- | ---: | ---: |
| Tabelas oficiais | 60/60 | 41.131 |
| Equacoes + preparo 1 min | 60/60 | 40.860 |
| Equacoes + preparo 1 s | 60/60 | 40.507 |

## Efeito fisico mantendo as mesmas decisoes

Para isolar a mudanca fisica, o melhor certificado oficial de cada configuracao foi transferido para cada nova matriz. Foram mantidos rotas, clientes, k e instantes de partida; somente os terminos de missoes foram atualizados. Todos os 480 certificados resultantes foram validados, incluindo capacidade. Isso nao acrescenta outra busca nem e o resultado de uma execucao SA com warm start.

| Grupo | Cenario | Melhor / igual / pior | Mudanca media % |
| --- | --- | ---: | ---: |
| PDSVRP-C | Equacoes + preparo 1 min | 0 / 180 / 0 | 0.000 |
| PDSVRP-C | Equacoes + preparo 1 s | 1 / 179 / 0 | -0.015 |
| PDSTSP-C | Equacoes + preparo 1 min | 1 / 59 / 0 | -0.016 |
| PDSTSP-C | Equacoes + preparo 1 s | 4 / 56 / 0 | -0.125 |

## Interpretacao

Melhorar contra a tabela oficial significa obter menor objetivo no respectivo cenario fisico. As novas equacoes alteram tempos e combinacoes permitidas, portanto um objetivo inferior ao publicado nao constitui automaticamente novo recorde para o conjunto original. A matriz exata usada em MIC 2024 continua indisponivel. Comparacoes S=1 usam o certificado distribuido do fundador; S>1 usa a literatura da extensao com varios caminhoes.

## 126-rc-e

O cliente 46 passa de inviavel por drones para 54 min com k=6 (53 min no preparo de 1 s). Entretanto, o cliente 97 permanece inviavel por drones e exige ida e volta de caminhao de 108 min. Logo, as mudancas nao explicam o UB MIC de 76 para S=5.

| S | Referencia | Oficial | Equacoes 1 min | Equacoes 1 s |
| --- | ---: | ---: | ---: | ---: |
| 1 | 144.000 | 252.000 | 252.000 | 252.000 |
| 2 | 152.000 | 148.000 | 148.000 | 148.000 |
| 3 | 100.000 | 116.000 | 116.000 | 116.000 |
| 4 | 84.000 | 112.000 | 112.000 | 112.000 |
| 5 | 76.000 | 112.000 | 112.000 | 112.000 |

## Todas as configuracoes

| Instancia | S | Referencia | Oficial | Equacoes 1 min | Equacoes 1 s | Gap Eq.1min % |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 15-r-e | 1 | 92.000 | 92.000 | 92.000 | 92.000 | 0.000 |
| 15-r-e | 2 | 92.000 | 92.000 | 92.000 | 92.000 | 0.000 |
| 15-r-e | 3 | 92.000 | 92.000 | 92.000 | 92.000 | 0.000 |
| 15-rc-c | 1 | 44.000 | 44.000 | 44.000 | 42.000 | 0.000 |
| 15-rc-c | 2 | 33.000 | 36.000 | 32.000 | 33.000 | -3.030 |
| 15-rc-c | 3 | 32.000 | 32.000 | 32.000 | 32.000 | 0.000 |
| 16-c-c | 1 | 60.000 | 61.000 | 60.000 | 60.000 | 0.000 |
| 16-c-c | 2 | 40.000 | 40.000 | 40.000 | 40.000 | 0.000 |
| 16-c-c | 3 | 36.000 | 36.000 | 36.000 | 36.000 | 0.000 |
| 16-r-e | 1 | 112.000 | 112.000 | 112.000 | 112.000 | 0.000 |
| 16-r-e | 2 | 108.000 | 108.000 | 108.000 | 108.000 | 0.000 |
| 16-r-e | 3 | 108.000 | 108.000 | 108.000 | 108.000 | 0.000 |
| 18-c-c | 1 | 56.000 | 59.000 | 59.000 | 57.000 | 5.357 |
| 18-c-c | 2 | 44.000 | 44.000 | 44.000 | 44.000 | 0.000 |
| 18-c-c | 3 | 44.000 | 44.000 | 44.000 | 44.000 | 0.000 |
| 18-r-e | 1 | 96.000 | 96.000 | 96.000 | 96.000 | 0.000 |
| 18-r-e | 2 | 92.000 | 92.000 | 92.000 | 92.000 | 0.000 |
| 18-r-e | 3 | 92.000 | 92.000 | 92.000 | 92.000 | 0.000 |
| 18-rc-c | 1 | 57.000 | 64.000 | 64.000 | 60.000 | 12.281 |
| 18-rc-c | 2 | 46.000 | 48.000 | 48.000 | 45.000 | 4.348 |
| 18-rc-c | 3 | 40.000 | 44.000 | 44.000 | 40.000 | 10.000 |
| 19-c-c | 1 | 44.000 | 44.000 | 44.000 | 44.000 | 0.000 |
| 19-c-c | 2 | 36.000 | 36.000 | 36.000 | 36.000 | 0.000 |
| 19-c-c | 3 | 36.000 | 36.000 | 36.000 | 36.000 | 0.000 |
| 20-c-c | 1 | 43.000 | 45.000 | 46.000 | 44.000 | 6.977 |
| 20-c-c | 2 | 40.000 | 40.000 | 40.000 | 40.000 | 0.000 |
| 20-c-c | 3 | 40.000 | 40.000 | 40.000 | 40.000 | 0.000 |
| 20-r-c | 1 | 64.000 | 64.000 | 64.000 | 64.000 | 0.000 |
| 20-r-c | 2 | 48.000 | 48.000 | 48.000 | 48.000 | 0.000 |
| 20-r-c | 3 | 37.000 | 40.000 | 36.000 | 36.000 | -2.703 |
| 20-r-e | 1 | 80.000 | 88.000 | 81.000 | 84.000 | 1.250 |
| 20-r-e | 2 | 72.000 | 76.000 | 76.000 | 74.000 | 5.556 |
| 20-r-e | 3 | 72.000 | 72.000 | 72.000 | 72.000 | 0.000 |
| 20-rc-c | 1 | 96.000 | 96.000 | 96.000 | 96.000 | 0.000 |
| 20-rc-c | 2 | 64.000 | 64.000 | 64.000 | 64.000 | 0.000 |
| 20-rc-c | 3 | 48.000 | 48.000 | 48.000 | 48.000 | 0.000 |
| 20-rc-e | 1 | 100.000 | 100.000 | 100.000 | 100.000 | 0.000 |
| 20-rc-e | 2 | 80.000 | 80.000 | 80.000 | 80.000 | 0.000 |
| 20-rc-e | 3 | 68.000 | 68.000 | 68.000 | 68.000 | 0.000 |
| 21-c-c | 1 | 62.000 | 64.000 | 64.000 | 64.000 | 3.226 |
| 21-c-c | 2 | 40.000 | 40.000 | 40.000 | 40.000 | 0.000 |
| 21-c-c | 3 | 36.000 | 40.000 | 36.000 | 40.000 | 0.000 |
| 21-r-e | 1 | 85.000 | 88.000 | 84.000 | 88.000 | -1.176 |
| 21-r-e | 2 | 76.000 | 76.000 | 76.000 | 76.000 | 0.000 |
| 21-r-e | 3 | 76.000 | 76.000 | 76.000 | 76.000 | 0.000 |
| 23-c-e | 1 | 80.000 | 80.000 | 80.000 | 80.000 | 0.000 |
| 23-c-e | 2 | 80.000 | 80.000 | 80.000 | 80.000 | 0.000 |
| 23-c-e | 3 | 80.000 | 80.000 | 80.000 | 80.000 | 0.000 |
| 23-r-c | 1 | 88.000 | 92.000 | 96.000 | 92.000 | 9.091 |
| 23-r-c | 2 | 60.000 | 60.000 | 60.000 | 60.000 | 0.000 |
| 23-r-c | 3 | 48.000 | 52.000 | 52.000 | 52.000 | 8.333 |
| 24-c-e | 1 | 84.000 | 84.000 | 84.000 | 84.000 | 0.000 |
| 24-c-e | 2 | 60.000 | 60.000 | 60.000 | 60.000 | 0.000 |
| 24-c-e | 3 | 60.000 | 60.000 | 60.000 | 60.000 | 0.000 |
| 24-r-e | 1 | 112.000 | 112.000 | 112.000 | 112.000 | 0.000 |
| 24-r-e | 2 | 100.000 | 100.000 | 100.000 | 100.000 | 0.000 |
| 24-r-e | 3 | 100.000 | 100.000 | 100.000 | 100.000 | 0.000 |
| 24-rc-c | 1 | 71.000 | 72.000 | 72.000 | 72.000 | 1.408 |
| 24-rc-c | 2 | 52.000 | 52.000 | 52.000 | 52.000 | 0.000 |
| 24-rc-c | 3 | 44.000 | 48.000 | 48.000 | 48.000 | 9.091 |
| 25-c-c | 1 | 56.000 | 56.000 | 56.000 | 56.000 | 0.000 |
| 25-c-c | 2 | 40.000 | 52.000 | 52.000 | 52.000 | 30.000 |
| 25-c-c | 3 | 37.000 | 40.000 | 40.000 | 40.000 | 8.108 |
| 25-r-e | 1 | 104.000 | 108.000 | 108.000 | 108.000 | 3.846 |
| 25-r-e | 2 | 88.000 | 104.000 | 104.000 | 104.000 | 18.182 |
| 25-r-e | 3 | 85.000 | 96.000 | 88.000 | 96.000 | 3.529 |
| 25-rc-e | 1 | 92.000 | 96.000 | 96.000 | 92.000 | 4.348 |
| 25-rc-e | 2 | 76.000 | 76.000 | 76.000 | 76.000 | 0.000 |
| 25-rc-e | 3 | 66.000 | 76.000 | 64.000 | 64.000 | -3.030 |
| 26-r-c | 1 | 103.000 | 103.000 | 101.000 | 100.000 | -1.942 |
| 26-r-c | 2 | 70.000 | 72.000 | 72.000 | 68.000 | 2.857 |
| 26-r-c | 3 | 56.000 | 56.000 | 60.000 | 60.000 | 7.143 |
| 27-c-c | 1 | 84.000 | 84.000 | 84.000 | 84.000 | 0.000 |
| 27-c-c | 2 | 52.000 | 52.000 | 52.000 | 52.000 | 0.000 |
| 27-c-c | 3 | 36.000 | 36.000 | 36.000 | 36.000 | 0.000 |
| 27-c-e | 1 | 68.000 | 68.000 | 68.000 | 68.000 | 0.000 |
| 27-c-e | 2 | 68.000 | 68.000 | 68.000 | 68.000 | 0.000 |
| 27-c-e | 3 | 68.000 | 68.000 | 68.000 | 68.000 | 0.000 |
| 27-rc-c | 1 | 100.000 | 100.000 | 100.000 | 100.000 | 0.000 |
| 27-rc-c | 2 | 72.000 | 72.000 | 72.000 | 72.000 | 0.000 |
| 27-rc-c | 3 | 60.000 | 60.000 | 60.000 | 60.000 | 0.000 |
| 27-rc-e | 1 | 84.000 | 88.000 | 88.000 | 88.000 | 4.762 |
| 27-rc-e | 2 | 76.000 | 76.000 | 76.000 | 76.000 | 0.000 |
| 27-rc-e | 3 | 76.000 | 76.000 | 76.000 | 76.000 | 0.000 |
| 29-rc-e | 1 | 116.000 | 116.000 | 116.000 | 116.000 | 0.000 |
| 29-rc-e | 2 | 100.000 | 100.000 | 100.000 | 100.000 | 0.000 |
| 29-rc-e | 3 | 100.000 | 100.000 | 100.000 | 100.000 | 0.000 |
| 30-c-c | 1 | 96.000 | 96.000 | 96.000 | 96.000 | 0.000 |
| 30-c-c | 2 | 64.000 | 64.000 | 64.000 | 64.000 | 0.000 |
| 30-c-c | 3 | 38.000 | 38.000 | 38.000 | 36.000 | 0.000 |
| 50-r-e | 1 | 116.000 | 224.000 | 224.000 | 224.000 | 93.103 |
| 50-r-e | 2 | 112.000 | 136.000 | 136.000 | 136.000 | 21.429 |
| 50-r-e | 3 | 104.000 | 112.000 | 112.000 | 112.000 | 7.692 |
| 50-r-e | 4 | 100.000 | 108.000 | 112.000 | 112.000 | 12.000 |
| 50-r-e | 5 | 100.000 | 104.000 | 112.000 | 108.000 | 12.000 |
| 53-r-e | 1 | 132.000 | 232.000 | 224.000 | 224.000 | 69.697 |
| 53-r-e | 2 | 112.000 | 136.000 | 136.000 | 136.000 | 21.429 |
| 53-r-e | 3 | 96.000 | 112.000 | 112.000 | 112.000 | 16.667 |
| 53-r-e | 4 | 96.000 | 112.000 | 96.000 | 112.000 | 0.000 |
| 53-r-e | 5 | 92.000 | 112.000 | 112.000 | 112.000 | 21.739 |
| 66-rc-e | 1 | 124.000 | 228.000 | 228.000 | 228.000 | 83.871 |
| 66-rc-e | 2 | 112.000 | 120.000 | 120.000 | 120.000 | 7.143 |
| 66-rc-e | 3 | 100.000 | 104.000 | 104.000 | 104.000 | 4.000 |
| 66-rc-e | 4 | 100.000 | 100.000 | 100.000 | 100.000 | 0.000 |
| 66-rc-e | 5 | 100.000 | 100.000 | 100.000 | 100.000 | 0.000 |
| 67-c-c | 1 | 76.000 | 92.000 | 92.000 | 92.000 | 21.053 |
| 67-c-c | 2 | 48.000 | 56.000 | 56.000 | 56.000 | 16.667 |
| 67-c-c | 3 | 52.000 | 52.000 | 52.000 | 52.000 | 0.000 |
| 67-c-c | 4 | 48.000 | 52.000 | 52.000 | 52.000 | 8.333 |
| 67-c-c | 5 | 52.000 | 52.000 | 52.000 | 52.000 | 0.000 |
| 68-rc-c | 1 | 76.000 | 212.000 | 212.000 | 212.000 | 178.947 |
| 68-rc-c | 2 | 56.000 | 108.000 | 108.000 | 108.000 | 92.857 |
| 68-rc-c | 3 | 56.000 | 84.000 | 84.000 | 84.000 | 50.000 |
| 68-rc-c | 4 | 52.000 | 60.000 | 60.000 | 56.000 | 15.385 |
| 68-rc-c | 5 | 44.000 | 52.000 | 52.000 | 56.000 | 18.182 |
| 76-c-c | 1 | 52.000 | 56.000 | 56.000 | 56.000 | 7.692 |
| 76-c-c | 2 | 36.000 | 40.000 | 40.000 | 40.000 | 11.111 |
| 76-c-c | 3 | 24.000 | 24.000 | 28.000 | 24.000 | 16.667 |
| 76-c-c | 4 | 24.000 | 24.000 | 28.000 | 24.000 | 16.667 |
| 76-c-c | 5 | 24.000 | 24.000 | 24.000 | 24.000 | 0.000 |
| 82-c-e | 1 | 64.000 | 64.000 | 64.000 | 64.000 | 0.000 |
| 82-c-e | 2 | 64.000 | 64.000 | 64.000 | 64.000 | 0.000 |
| 82-c-e | 3 | 64.000 | 64.000 | 64.000 | 64.000 | 0.000 |
| 82-c-e | 4 | 64.000 | 64.000 | 64.000 | 64.000 | 0.000 |
| 82-c-e | 5 | 64.000 | 64.000 | 64.000 | 64.000 | 0.000 |
| 82-rc-c | 1 | 104.000 | 196.000 | 196.000 | 196.000 | 88.462 |
| 82-rc-c | 2 | 100.000 | 128.000 | 128.000 | 128.000 | 28.000 |
| 82-rc-c | 3 | 80.000 | 92.000 | 92.000 | 92.000 | 15.000 |
| 82-rc-c | 4 | 60.000 | 76.000 | 76.000 | 76.000 | 26.667 |
| 82-rc-c | 5 | 60.000 | 64.000 | 64.000 | 64.000 | 6.667 |
| 88-c-e | 1 | 108.000 | 120.000 | 120.000 | 120.000 | 11.111 |
| 88-c-e | 2 | 84.000 | 100.000 | 100.000 | 100.000 | 19.048 |
| 88-c-e | 3 | 76.000 | 76.000 | 76.000 | 76.000 | 0.000 |
| 88-c-e | 4 | 72.000 | 76.000 | 76.000 | 76.000 | 5.556 |
| 88-c-e | 5 | 72.000 | 76.000 | 76.000 | 76.000 | 5.556 |
| 91-r-c | 1 | 124.000 | 276.000 | 276.000 | 276.000 | 122.581 |
| 91-r-c | 2 | 124.000 | 152.000 | 152.000 | 152.000 | 22.581 |
| 91-r-c | 3 | 100.000 | 108.000 | 108.000 | 108.000 | 8.000 |
| 91-r-c | 4 | 76.000 | 84.000 | 84.000 | 84.000 | 10.526 |
| 91-r-c | 5 | 68.000 | 76.000 | 76.000 | 76.000 | 11.765 |
| 99-rc-c | 1 | 100.000 | 232.000 | 232.000 | 232.000 | 132.000 |
| 99-rc-c | 2 | 96.000 | 116.000 | 116.000 | 116.000 | 20.833 |
| 99-rc-c | 3 | 64.000 | 92.000 | 92.000 | 92.000 | 43.750 |
| 99-rc-c | 4 | 64.000 | 68.000 | 68.000 | 68.000 | 6.250 |
| 99-rc-c | 5 | 60.000 | 64.000 | 64.000 | 64.000 | 6.667 |
| 101-r-c | 1 | 120.000 | 296.000 | 296.000 | 296.000 | 146.667 |
| 101-r-c | 2 | 152.000 | 160.000 | 160.000 | 160.000 | 5.263 |
| 101-r-c | 3 | 96.000 | 108.000 | 108.000 | 108.000 | 12.500 |
| 101-r-c | 4 | 76.000 | 88.000 | 88.000 | 88.000 | 15.789 |
| 101-r-c | 5 | 68.000 | 76.000 | 76.000 | 76.000 | 11.765 |
| 103-rc-c | 1 | 124.000 | 224.000 | 224.000 | 224.000 | 80.645 |
| 103-rc-c | 2 | 124.000 | 144.000 | 144.000 | 144.000 | 16.129 |
| 103-rc-c | 3 | 88.000 | 92.000 | 92.000 | 92.000 | 4.545 |
| 103-rc-c | 4 | 76.000 | 72.000 | 72.000 | 72.000 | -5.263 |
| 103-rc-c | 5 | 68.000 | 68.000 | 68.000 | 68.000 | 0.000 |
| 105-rc-e | 1 | 120.000 | 236.000 | 236.000 | 236.000 | 96.667 |
| 105-rc-e | 2 | 128.000 | 144.000 | 144.000 | 144.000 | 12.500 |
| 105-rc-e | 3 | 104.000 | 120.000 | 120.000 | 120.000 | 15.385 |
| 105-rc-e | 4 | 104.000 | 104.000 | 104.000 | 104.000 | 0.000 |
| 105-rc-e | 5 | 104.000 | 104.000 | 104.000 | 104.000 | 0.000 |
| 108-rc-e | 1 | 136.000 | 264.000 | 264.000 | 264.000 | 94.118 |
| 108-rc-e | 2 | 152.000 | 168.000 | 168.000 | 168.000 | 10.526 |
| 108-rc-e | 3 | 124.000 | 124.000 | 124.000 | 124.000 | 0.000 |
| 108-rc-e | 4 | 120.000 | 120.000 | 120.000 | 120.000 | 0.000 |
| 108-rc-e | 5 | 120.000 | 120.000 | 120.000 | 120.000 | 0.000 |
| 114-rc-c | 1 | 96.000 | 240.000 | 240.000 | 240.000 | 150.000 |
| 114-rc-c | 2 | 104.000 | 136.000 | 136.000 | 136.000 | 30.769 |
| 114-rc-c | 3 | 72.000 | 88.000 | 88.000 | 88.000 | 22.222 |
| 114-rc-c | 4 | 72.000 | 72.000 | 72.000 | 72.000 | 0.000 |
| 114-rc-c | 5 | 64.000 | 60.000 | 60.000 | 60.000 | -6.250 |
| 121-rc-e | 1 | 124.000 | 236.000 | 236.000 | 236.000 | 90.323 |
| 121-rc-e | 2 | 132.000 | 136.000 | 136.000 | 136.000 | 3.030 |
| 121-rc-e | 3 | 96.000 | 104.000 | 104.000 | 104.000 | 8.333 |
| 121-rc-e | 4 | 92.000 | 96.000 | 96.000 | 96.000 | 4.348 |
| 121-rc-e | 5 | 92.000 | 96.000 | 96.000 | 96.000 | 4.348 |
| 126-r-c | 1 | 160.000 | 296.000 | 296.000 | 296.000 | 85.000 |
| 126-r-c | 2 | 136.000 | 152.000 | 152.000 | 152.000 | 11.765 |
| 126-r-c | 3 | 108.000 | 120.000 | 120.000 | 120.000 | 11.111 |
| 126-r-c | 4 | 88.000 | 92.000 | 92.000 | 92.000 | 4.545 |
| 126-r-c | 5 | 80.000 | 80.000 | 80.000 | 80.000 | 0.000 |
| 126-rc-e | 1 | 144.000 | 252.000 | 252.000 | 252.000 | 75.000 |
| 126-rc-e | 2 | 152.000 | 148.000 | 148.000 | 148.000 | -2.632 |
| 126-rc-e | 3 | 100.000 | 116.000 | 116.000 | 116.000 | 16.000 |
| 126-rc-e | 4 | 84.000 | 112.000 | 112.000 | 112.000 | 33.333 |
| 126-rc-e | 5 | 76.000 | 112.000 | 112.000 | 112.000 | 47.368 |
| 144-rc-c | 1 | 128.000 | 288.000 | 288.000 | 288.000 | 125.000 |
| 144-rc-c | 2 | 116.000 | 148.000 | 148.000 | 148.000 | 27.586 |
| 144-rc-c | 3 | 116.000 | 112.000 | 112.000 | 112.000 | -3.448 |
| 144-rc-c | 4 | 72.000 | 76.000 | 76.000 | 76.000 | 5.556 |
| 144-rc-c | 5 | 92.000 | 68.000 | 68.000 | 68.000 | -26.087 |
| 154-c-c | 1 | 72.000 | 72.000 | 72.000 | 72.000 | 0.000 |
| 154-c-c | 2 | 40.000 | 40.000 | 40.000 | 40.000 | 0.000 |
| 154-c-c | 3 | 36.000 | 36.000 | 36.000 | 36.000 | 0.000 |
| 154-c-c | 4 | 36.000 | 36.000 | 36.000 | 36.000 | 0.000 |
| 154-c-c | 5 | 36.000 | 36.000 | 36.000 | 36.000 | 0.000 |
| 165-r-c | 1 | 164.000 | 360.000 | 360.000 | 360.000 | 119.512 |
| 165-r-c | 2 | 184.000 | 200.000 | 200.000 | 200.000 | 8.696 |
| 165-r-c | 3 | 132.000 | 128.000 | 128.000 | 128.000 | -3.030 |
| 165-r-c | 4 | 108.000 | 108.000 | 108.000 | 108.000 | 0.000 |
| 165-r-c | 5 | 84.000 | 88.000 | 88.000 | 88.000 | 4.762 |
| 167-r-e | 1 | 188.000 | 344.000 | 344.000 | 344.000 | 82.979 |
| 167-r-e | 2 | 180.000 | 180.000 | 180.000 | 180.000 | 0.000 |
| 167-r-e | 3 | 132.000 | 140.000 | 140.000 | 140.000 | 6.061 |
| 167-r-e | 4 | 120.000 | 120.000 | 120.000 | 120.000 | 0.000 |
| 167-r-e | 5 | 116.000 | 112.000 | 112.000 | 112.000 | -3.448 |
| 173-r-c | 1 | 164.000 | 376.000 | 376.000 | 376.000 | 129.268 |
| 173-r-c | 2 | 176.000 | 192.000 | 192.000 | 192.000 | 9.091 |
| 173-r-c | 3 | 128.000 | 132.000 | 132.000 | 132.000 | 3.125 |
| 173-r-c | 4 | 96.000 | 100.000 | 100.000 | 100.000 | 4.167 |
| 173-r-c | 5 | 92.000 | 88.000 | 88.000 | 88.000 | -4.348 |
| 173-rc-c | 1 | 133.000 | 264.000 | 264.000 | 264.000 | 98.496 |
| 173-rc-c | 2 | 144.000 | 148.000 | 148.000 | 148.000 | 2.778 |
| 173-rc-c | 3 | 116.000 | 108.000 | 108.000 | 108.000 | -6.897 |
| 173-rc-c | 4 | 84.000 | 84.000 | 84.000 | 84.000 | 0.000 |
| 173-rc-c | 5 | 80.000 | 76.000 | 76.000 | 76.000 | -5.000 |
| 181-r-e | 1 | 224.000 | 396.000 | 396.000 | 396.000 | 76.786 |
| 181-r-e | 2 | 180.000 | 208.000 | 208.000 | 208.000 | 15.556 |
| 181-r-e | 3 | 152.000 | 152.000 | 152.000 | 152.000 | 0.000 |
| 181-r-e | 4 | 124.000 | 128.000 | 128.000 | 128.000 | 3.226 |
| 181-r-e | 5 | 120.000 | 120.000 | 120.000 | 120.000 | 0.000 |
| 185-c-c | 1 | 96.000 | 100.000 | 100.000 | 100.000 | 4.167 |
| 185-c-c | 2 | 60.000 | 60.000 | 60.000 | 60.000 | 0.000 |
| 185-c-c | 3 | 48.000 | 48.000 | 48.000 | 48.000 | 0.000 |
| 185-c-c | 4 | 44.000 | 48.000 | 48.000 | 48.000 | 9.091 |
| 185-c-c | 5 | 44.000 | 48.000 | 48.000 | 48.000 | 9.091 |
| 187-rc-e | 1 | 196.000 | 312.000 | 312.000 | 312.000 | 59.184 |
| 187-rc-e | 2 | 160.000 | 176.000 | 176.000 | 176.000 | 10.000 |
| 187-rc-e | 3 | 124.000 | 136.000 | 136.000 | 136.000 | 9.677 |
| 187-rc-e | 4 | 148.000 | 116.000 | 116.000 | 116.000 | -21.622 |
| 187-rc-e | 5 | 128.000 | 112.000 | 112.000 | 112.000 | -12.500 |
| 198-c-c | 1 | 64.000 | 68.000 | 68.000 | 68.000 | 6.250 |
| 198-c-c | 2 | 36.000 | 36.000 | 36.000 | 36.000 | 0.000 |
| 198-c-c | 3 | 36.000 | 36.000 | 36.000 | 36.000 | 0.000 |
| 198-c-c | 4 | 36.000 | 36.000 | 36.000 | 36.000 | 0.000 |
| 198-c-c | 5 | 36.000 | 36.000 | 36.000 | 36.000 | 0.000 |
| 200-r-e | 1 | 212.000 | 368.000 | 368.000 | 368.000 | 73.585 |
| 200-r-e | 2 | 216.000 | 208.000 | 208.000 | 208.000 | -3.704 |
| 200-r-e | 3 | 148.000 | 156.000 | 156.000 | 156.000 | 5.405 |
| 200-r-e | 4 | 120.000 | 132.000 | 132.000 | 132.000 | 10.000 |
| 200-r-e | 5 | 120.000 | 116.000 | 116.000 | 116.000 | -3.333 |

## Reproducao e arquivos

`python scripts/experimentar_equacoes.py --seconds 2 --repetitions 3 --workers 4`

`python scripts/relatar_equacoes.py`

O executor retoma execucoes individuais ja registradas com mesmos parametros. Para um experimento novo, use outro --output. Os fontes de producao, executavel principal, instancias oficiais e results/best.json sao preservados.

Saidas: output/experimento_equacoes/physics.csv, physics_summary.json, comparison.csv, summary.json, paired_summary.json, environment.json, original_certificate_validation.json, fixed_assignment_comparison.csv, fixed_assignment_summary.json, transferred_certificates/*.json e runs/*.json com certificados.

Referencias: https://doi.org/10.1287/trsc.2022.1192 ; https://doi.org/10.1016/j.cor.2023.106514 ; https://doi.org/10.1007/978-3-031-62912-9_13 .
