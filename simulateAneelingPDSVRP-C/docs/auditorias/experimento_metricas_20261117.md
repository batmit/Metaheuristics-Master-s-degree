# Experimento: Manhattan versus Euclidiana

Data: 17/11/2026. Unidade do objetivo: minutos. Menor makespan e melhor.

Foram executadas 120 configuracoes grandes (30 instancias, S=2,3,4,5), 3 sementes por metrica, 2 segundos por execucao, 4 processos simultaneos. Semente base 42; semente efetiva: (42 + 10*n + S + 1000003*repeticao) modulo 2^32, repeticao iniciando em zero.

As duas versoes foram recompiladas com GCC -O2 a partir dos mesmos fontes. A unica mudanca na copia experimental de cv.c e substituir dx+dy por hypot(dx,dy). Os tempos oficiais de drones foram mantidos. Os certificados de cada checkpoint foram recalculados independentemente em Python, incluindo cobertura e capacidade. O executavel principal e results/best.json nao foram substituidos.

Cada linha usa o melhor valor das repeticoes. As medias percentuais sao aritmeticas por configuracao; nao sao a razao das medias dos makespans.

| Indicador | Resultado |
| --- | ---: |
| Execucoes / checkpoints validados | 720 / 71705 |
| Execucoes concluidas antes do limite | 75 |
| Reducao media Euclidiana versus Manhattan | 14.478% |
| Gap medio Manhattan versus UB publicado | 7.396% |
| Diferenca media Euclidiana versus UB publicado | -8.138% |
| Euclidiana menor / igual a Manhattan | 113 / 0 |
| Manhattan abaixo do UB publicado | 14 |
| Euclidiana abaixo do UB / LB publicado | 95 / 1 |

## Interpretacao e limites

Os agregados contra a literatura incluem todas as 120 configuracoes como diferencas numericas. Os UBs de 126-rc-e/S=3,4,5 sao incompativeis com os dados Manhattan locais. Excluindo esses tres casos, o gap medio Manhattan e 6,759% (117 configuracoes). A Euclidiana foi melhor em 113 casos e pior em sete; a heuristica com corte curto pode obter resultado pior embora o custo de uma mesma rota nunca aumente.

COR 2024 explicita Manhattan para caminhoes a 30 km/h e Euclidiana para drones. Portanto, o resultado euclidiano resolve uma variante com deslocamentos de caminhao mais curtos: diferencas negativas contra os UBs publicados nao sao recordes do problema original. Os LBs publicados tambem nao sao limites validos dessa variante. As referencias das grandes sao os valores MIC 2024 transcritos no catalogo local; nao foi feita uma busca exaustiva de novos BKS.

O limite curto pode encerrar o SA durante a calibracao. O melhor checkpoint valido e conservado. A variacao do escalonamento do sistema e os quatro processos simultaneos afetam o numero de iteracoes; sementes iguais nao garantem trajetorias iguais entre metricas nem reproducao exata com corte por tempo. Este teste e exploratorio, sem prova de otimalidade ou significancia estatistica. A identidade dos tempos locais de drones com os usados nos artigos continua nao comprovada.

## Resultados por configuracao

| Instancia | S | UB literatura | Manhattan | Euclidiana | Reducao % | Dif. Eucl./lit. % |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 50-r-e | 2 | 112.000 | 112.000 | 88.262 | 21.195 | -21.195 |
| 50-r-e | 3 | 104.000 | 112.000 | 81.893 | 26.881 | -21.256 |
| 50-r-e | 4 | 100.000 | 108.000 | 79.808 | 26.104 | -20.192 |
| 50-r-e | 5 | 100.000 | 104.000 | 79.196 | 23.850 | -20.804 |
| 53-r-e | 2 | 112.000 | 128.000 | 103.000 | 19.531 | -8.036 |
| 53-r-e | 3 | 96.000 | 112.000 | 85.340 | 23.804 | -11.105 |
| 53-r-e | 4 | 96.000 | 112.000 | 79.783 | 28.765 | -16.892 |
| 53-r-e | 5 | 92.000 | 112.000 | 79.196 | 29.289 | -13.917 |
| 66-rc-e | 2 | 112.000 | 120.000 | 110.193 | 8.172 | -1.613 |
| 66-rc-e | 3 | 100.000 | 104.000 | 88.528 | 14.876 | -11.472 |
| 66-rc-e | 4 | 100.000 | 100.000 | 73.147 | 26.853 | -26.853 |
| 66-rc-e | 5 | 100.000 | 100.000 | 72.000 | 28.000 | -28.000 |
| 67-c-c | 2 | 48.000 | 56.000 | 66.687 | -19.083 | 38.931 |
| 67-c-c | 3 | 52.000 | 52.000 | 43.081 | 17.151 | -17.151 |
| 67-c-c | 4 | 48.000 | 52.000 | 41.854 | 19.511 | -12.804 |
| 67-c-c | 5 | 52.000 | 52.000 | 41.761 | 19.690 | -19.690 |
| 68-rc-c | 2 | 56.000 | 108.000 | 106.030 | 1.824 | 89.339 |
| 68-rc-c | 3 | 56.000 | 84.000 | 70.766 | 15.755 | 26.367 |
| 68-rc-c | 4 | 52.000 | 60.000 | 49.000 | 18.333 | -5.769 |
| 68-rc-c | 5 | 44.000 | 52.000 | 36.129 | 30.521 | -17.889 |
| 76-c-c | 2 | 36.000 | 40.000 | 30.583 | 23.544 | -15.049 |
| 76-c-c | 3 | 24.000 | 24.000 | 19.190 | 20.040 | -20.040 |
| 76-c-c | 4 | 24.000 | 24.000 | 19.190 | 20.040 | -20.040 |
| 76-c-c | 5 | 24.000 | 24.000 | 19.190 | 20.040 | -20.040 |
| 82-c-e | 2 | 64.000 | 64.000 | 49.305 | 22.961 | -22.961 |
| 82-c-e | 3 | 64.000 | 64.000 | 48.332 | 24.481 | -24.481 |
| 82-c-e | 4 | 64.000 | 64.000 | 48.332 | 24.481 | -24.481 |
| 82-c-e | 5 | 64.000 | 64.000 | 48.332 | 24.481 | -24.481 |
| 82-rc-c | 2 | 100.000 | 128.000 | 114.536 | 10.519 | 14.536 |
| 82-rc-c | 3 | 80.000 | 92.000 | 81.902 | 10.976 | 2.378 |
| 82-rc-c | 4 | 60.000 | 76.000 | 56.344 | 25.863 | -6.093 |
| 82-rc-c | 5 | 60.000 | 64.000 | 54.071 | 15.514 | -9.881 |
| 88-c-e | 2 | 84.000 | 100.000 | 92.944 | 7.056 | 10.648 |
| 88-c-e | 3 | 76.000 | 76.000 | 61.196 | 19.479 | -19.479 |
| 88-c-e | 4 | 72.000 | 76.000 | 60.191 | 20.801 | -16.401 |
| 88-c-e | 5 | 72.000 | 76.000 | 57.353 | 24.536 | -20.344 |
| 91-r-c | 2 | 124.000 | 152.000 | 128.608 | 15.390 | 3.716 |
| 91-r-c | 3 | 100.000 | 108.000 | 93.354 | 13.562 | -6.646 |
| 91-r-c | 4 | 76.000 | 84.000 | 66.859 | 20.406 | -12.027 |
| 91-r-c | 5 | 68.000 | 76.000 | 59.585 | 21.599 | -12.375 |
| 99-rc-c | 2 | 96.000 | 116.000 | 107.627 | 7.218 | 12.112 |
| 99-rc-c | 3 | 64.000 | 92.000 | 76.725 | 16.604 | 19.882 |
| 99-rc-c | 4 | 64.000 | 68.000 | 58.960 | 13.294 | -7.875 |
| 99-rc-c | 5 | 60.000 | 64.000 | 57.134 | 10.728 | -4.776 |
| 101-r-c | 2 | 152.000 | 160.000 | 134.897 | 15.689 | -11.252 |
| 101-r-c | 3 | 96.000 | 108.000 | 95.617 | 11.466 | -0.399 |
| 101-r-c | 4 | 76.000 | 88.000 | 78.758 | 10.502 | 3.629 |
| 101-r-c | 5 | 68.000 | 76.000 | 61.305 | 19.336 | -9.846 |
| 103-rc-c | 2 | 124.000 | 144.000 | 123.237 | 14.419 | -0.615 |
| 103-rc-c | 3 | 88.000 | 92.000 | 92.370 | -0.402 | 4.966 |
| 103-rc-c | 4 | 76.000 | 72.000 | 76.569 | -6.346 | 0.749 |
| 103-rc-c | 5 | 68.000 | 68.000 | 64.266 | 5.491 | -5.491 |
| 105-rc-e | 2 | 128.000 | 144.000 | 123.694 | 14.102 | -3.364 |
| 105-rc-e | 3 | 104.000 | 120.000 | 100.130 | 16.559 | -3.721 |
| 105-rc-e | 4 | 104.000 | 104.000 | 91.799 | 11.732 | -11.732 |
| 105-rc-e | 5 | 104.000 | 104.000 | 84.885 | 18.380 | -18.380 |
| 108-rc-e | 2 | 152.000 | 168.000 | 132.373 | 21.206 | -12.912 |
| 108-rc-e | 3 | 124.000 | 124.000 | 109.411 | 11.765 | -11.765 |
| 108-rc-e | 4 | 120.000 | 120.000 | 96.332 | 19.724 | -19.724 |
| 108-rc-e | 5 | 120.000 | 120.000 | 84.853 | 29.289 | -29.289 |
| 114-rc-c | 2 | 104.000 | 136.000 | 104.181 | 23.397 | 0.174 |
| 114-rc-c | 3 | 72.000 | 88.000 | 80.132 | 8.941 | 11.294 |
| 114-rc-c | 4 | 72.000 | 72.000 | 60.952 | 15.344 | -15.344 |
| 114-rc-c | 5 | 64.000 | 60.000 | 55.319 | 7.801 | -13.563 |
| 121-rc-e | 2 | 132.000 | 136.000 | 114.085 | 16.114 | -13.572 |
| 121-rc-e | 3 | 96.000 | 104.000 | 88.591 | 14.816 | -7.718 |
| 121-rc-e | 4 | 92.000 | 96.000 | 79.225 | 17.474 | -13.886 |
| 121-rc-e | 5 | 92.000 | 96.000 | 69.774 | 27.318 | -24.158 |
| 126-rc-e | 2 | 152.000 | 148.000 | 124.729 | 15.724 | -17.942 |
| 126-rc-e | 3 | 100.000 | 116.000 | 102.751 | 11.422 | 2.751 |
| 126-rc-e | 4 | 84.000 | 112.000 | 92.373 | 17.524 | 9.967 |
| 126-rc-e | 5 | 76.000 | 112.000 | 83.252 | 25.668 | 9.542 |
| 126-r-c | 2 | 136.000 | 152.000 | 132.522 | 12.815 | -2.558 |
| 126-r-c | 3 | 108.000 | 120.000 | 103.909 | 13.409 | -3.788 |
| 126-r-c | 4 | 88.000 | 92.000 | 81.936 | 10.939 | -6.891 |
| 126-r-c | 5 | 80.000 | 80.000 | 70.085 | 12.394 | -12.394 |
| 144-rc-c | 2 | 116.000 | 148.000 | 131.925 | 10.861 | 13.729 |
| 144-rc-c | 3 | 116.000 | 112.000 | 93.115 | 16.861 | -19.728 |
| 144-rc-c | 4 | 72.000 | 76.000 | 75.456 | 0.716 | 4.800 |
| 144-rc-c | 5 | 92.000 | 68.000 | 59.068 | 13.136 | -35.796 |
| 154-c-c | 2 | 40.000 | 40.000 | 30.490 | 23.774 | -23.774 |
| 154-c-c | 3 | 36.000 | 36.000 | 29.448 | 18.201 | -18.201 |
| 154-c-c | 4 | 36.000 | 36.000 | 26.912 | 25.243 | -25.243 |
| 154-c-c | 5 | 36.000 | 36.000 | 26.556 | 26.233 | -26.233 |
| 165-r-c | 2 | 184.000 | 200.000 | 179.892 | 10.054 | -2.232 |
| 165-r-c | 3 | 132.000 | 128.000 | 117.001 | 8.593 | -11.363 |
| 165-r-c | 4 | 108.000 | 108.000 | 93.184 | 13.719 | -13.719 |
| 165-r-c | 5 | 84.000 | 88.000 | 80.031 | 9.056 | -4.725 |
| 167-r-e | 2 | 180.000 | 180.000 | 165.130 | 8.261 | -8.261 |
| 167-r-e | 3 | 132.000 | 140.000 | 125.733 | 10.191 | -4.748 |
| 167-r-e | 4 | 120.000 | 120.000 | 103.568 | 13.694 | -13.694 |
| 167-r-e | 5 | 116.000 | 112.000 | 97.491 | 12.955 | -15.956 |
| 173-r-c | 2 | 176.000 | 192.000 | 173.259 | 9.761 | -1.557 |
| 173-r-c | 3 | 128.000 | 132.000 | 116.671 | 11.613 | -8.851 |
| 173-r-c | 4 | 96.000 | 100.000 | 95.702 | 4.298 | -0.310 |
| 173-r-c | 5 | 92.000 | 88.000 | 75.290 | 14.443 | -18.163 |
| 173-rc-c | 2 | 144.000 | 148.000 | 122.263 | 17.390 | -15.095 |
| 173-rc-c | 3 | 116.000 | 108.000 | 88.701 | 17.870 | -23.534 |
| 173-rc-c | 4 | 84.000 | 84.000 | 69.417 | 17.361 | -17.361 |
| 173-rc-c | 5 | 80.000 | 76.000 | 61.968 | 18.464 | -22.540 |
| 181-r-e | 2 | 180.000 | 208.000 | 183.792 | 11.638 | 2.107 |
| 181-r-e | 3 | 152.000 | 152.000 | 136.350 | 10.296 | -10.296 |
| 181-r-e | 4 | 124.000 | 128.000 | 108.297 | 15.393 | -12.664 |
| 181-r-e | 5 | 120.000 | 120.000 | 95.961 | 20.032 | -20.032 |
| 185-c-c | 2 | 60.000 | 60.000 | 78.307 | -30.511 | 30.511 |
| 185-c-c | 3 | 48.000 | 48.000 | 66.042 | -37.588 | 37.588 |
| 185-c-c | 4 | 44.000 | 48.000 | 64.958 | -35.328 | 47.631 |
| 185-c-c | 5 | 44.000 | 48.000 | 47.573 | 0.889 | 8.121 |
| 187-rc-e | 2 | 160.000 | 176.000 | 156.070 | 11.324 | -2.456 |
| 187-rc-e | 3 | 124.000 | 136.000 | 113.968 | 16.200 | -8.090 |
| 187-rc-e | 4 | 148.000 | 116.000 | 99.653 | 14.092 | -32.667 |
| 187-rc-e | 5 | 128.000 | 112.000 | 89.650 | 19.955 | -29.961 |
| 198-c-c | 2 | 36.000 | 36.000 | 44.699 | -24.163 | 24.163 |
| 198-c-c | 3 | 36.000 | 36.000 | 27.625 | 23.264 | -23.264 |
| 198-c-c | 4 | 36.000 | 36.000 | 25.612 | 28.854 | -28.854 |
| 198-c-c | 5 | 36.000 | 36.000 | 25.612 | 28.854 | -28.854 |
| 200-r-e | 2 | 216.000 | 208.000 | 183.530 | 11.764 | -15.032 |
| 200-r-e | 3 | 148.000 | 156.000 | 138.470 | 11.237 | -6.439 |
| 200-r-e | 4 | 120.000 | 132.000 | 113.052 | 14.354 | -5.790 |
| 200-r-e | 5 | 120.000 | 116.000 | 98.070 | 15.457 | -18.275 |

## Fontes

- COR 2024: https://doi.org/10.1016/j.cor.2023.106514
- MIC 2024: https://doi.org/10.1007/978-3-031-62912-9_13
- Catalogo local: instances/literature_pdsvrp_c.csv

Reproducao: `python scripts/experimentar_metricas.py --seconds 2 --repetitions 3 --workers 4`

Artefatos: comparison.csv (120 linhas), runs.jsonl (certificados por execucao), summary.json (agregados), cv_*.c e executaveis isolados.
