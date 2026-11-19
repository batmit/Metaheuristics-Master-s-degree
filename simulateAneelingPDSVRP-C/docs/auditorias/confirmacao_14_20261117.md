# Confirmacao dos 14 certificados — 17/11/2026

## Uso de drones e opcional

O PDSVRP-C classico nao exige nenhuma missao de drone. Na atribuicao de cada
cliente elegivel, as alternativas caminhao e drones somam um; e permitido
escolher caminhao para todos. Nas formulacoes de fluxo, definir z=0, f=0 e y=0
satisfaz a parte dos drones. O limite de frota e superior, nao minimo de uso.
Os tempos dos clientes atendidos por caminhoes continuam sendo definidos pelas
rotas; nao se deve zerar indiscriminadamente todas as variaveis temporais CP2.

Evidencia textual direta: MIC 2024, p.137, apresenta o caso sem drones como
min-max VRP e utiliza solucoes so de caminhoes como hints. A secao 3, p.141,
explica a construcao dessas solucoes. Portanto, usar somente caminhoes e
admissivel no proprio problema publicado, nao uma dispensa inventada localmente.

Fontes primarias:
- https://iris.unimore.it/bitstream/11380/1342368/2/chapter.pdf
- https://iris.unimore.it/retrieve/0929c99b-71a5-4f38-ba73-8c5a6d1e2eb0/chapter.pdf
- https://arxiv.org/html/2307.03523v1 (secoes 2.2 e 3.2-3.4)
- https://doi.org/10.1016/j.cor.2023.106514 (versao publicada)

## Verificacao independente realizada

Os 14 certificados foram conferidos no avaliador C recompilado com avisos como
erros, no validador Python e num terceiro recalculo independente usando Decimal.
O terceiro verifica cobertura exata, S rotas, ausencia de missoes, deposito nas
duas extremidades, Manhattan, velocidade do arquivo e minutos. Todas as arestas
usadas sao inteiras em minutos, de modo que ceil nao altera os resultados.
As entradas coincidem byte a byte com a copia original e com o ZIP oficial de 2025.
Rotas, hashes e tempos individuais estao em output/confirmacao_14/verificacao.json;
certificados integrais em output/auditoria_professor/*.json.

| Instancia | S | UB publicado | Referencia fortalecida | Valor exato | Tempos das rotas |
| --- | ---: | ---: | ---: | ---: | --- |
| 103-rc-c | 4 | 76.0 | 76.0 | 72 | 72, 68, 64, 60 |
| 114-rc-c | 5 | 64.0 | 64.0 | 60 | 60, 60, 56, 60, 56 |
| 126-rc-e | 2 | 152.0 | 152.0 | 148 | 148, 144 |
| 144-rc-c | 3 | 116.0 | 116.0 | 112 | 112, 108, 104 |
| 144-rc-c | 5 | 92.0 | 72.0 | 68 | 68, 60, 68, 64, 52 |
| 165-r-c | 3 | 132.0 | 132.0 | 128 | 120, 128, 128 |
| 167-r-e | 5 | 116.0 | 116.0 | 112 | 112, 108, 112, 108, 96 |
| 173-r-c | 5 | 92.0 | 92.0 | 88 | 88, 88, 84, 76, 80 |
| 173-rc-c | 3 | 116.0 | 116.0 | 108 | 100, 104, 108 |
| 173-rc-c | 5 | 80.0 | 80.0 | 76 | 68, 72, 76, 72, 72 |
| 187-rc-e | 4 | 148.0 | 124.0 | 116 | 112, 112, 112, 116 |
| 187-rc-e | 5 | 128.0 | 124.0 | 112 | 112, 112, 112, 96, 56 |
| 200-r-e | 2 | 216.0 | 216.0 | 208 | 208, 208 |
| 200-r-e | 5 | 120.0 | 120.0 | 116 | 112, 116, 112, 112, 112 |

Referencia fortalecida = menor UB do catalogo para a mesma instancia com no
maximo S caminhoes. Os UBs de 144-rc-c/S=5 e 187-rc-e/S=4,5 foram reduzidos
analiticamente dessa forma; os 14 certificados continuam estritamente melhores.

## Conclusao precisamente delimitada

Confirmados integralmente: viabilidade local, valores dos certificados,
ausencia de obrigatoriedade de drones no PDSVRP-C classico e desigualdade estrita
contra as referencias do catalogo consultado. Nao e necessario conhecer tau dos
autores para validar estas solucoes, pois nao utilizam drones.

Nao confirmado integralmente: identidade com arquivos de caminhao efetivamente
executados em MIC 2024 e inexistencia de resultados posteriores melhores. A
tabela publicada foi consultada via indice institucional nas pesquisas desta
conversa; acesso direto ao PDF apresentou 403. Nao declarar “novos recordes
mundiais comprovados” ou superioridade estatistica do SA+BB. As solucoes validas
podem ser apresentadas como novos UBs locais inferiores aos UBs MIC consultados,
sob as convencoes publicadas e as instancias oficiais distribuidas.

Nenhum parametro foi alterado para obter a confirmacao; nenhuma nova otimizacao
foi necessaria. Reproduzir: python scripts/confirmar_14.py.
