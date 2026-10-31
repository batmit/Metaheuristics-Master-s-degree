# Diagnostico de desempenho — 30/10/2026

O algoritmo principal nao foi alterado durante esta analise. A instrumentacao
esta em `scripts/diagnosticar_desempenho.py`; gera fontes e executavel isolados
em `diagnostics/performance`. O snapshot tem 16 configuracoes abertas, lidas
do lote mais recente. Nao confundir esse snapshot com o resumo antigo de 120
configuracoes: comparison.csv foi reiniciado depois de summary.json/run.json.

## Evidencias

- Nos 16 certificados do snapshot, os caminhoes determinam o makespan.
- Reordenar cada rota por 2-opt, sem mudar atribuicoes, k ou cronograma de
  drones, reduz o makespan em mediana 51,905%. Este e um experimento offline,
  nao uma alteracao aplicada ao SA ou aos resultados publicados do lote.
- Exemplo 50-r-e/s=2: encontrado 384, drones terminam em 104; somente 2-opt
  nas rotas reduz a solucao para 180. O BKS MIC 2024 e 112.
- Perfil isolado de 50-r-e/s=2, semente 544, com prazo externo de 15 s:
  64 tentativas iniciadas do SA, um unico nivel de temperatura (T=34,522712144).
- Com alpha=0,99, saMax=500 e Tf=0,01, seriam 811 niveis/405.500 tentativas
  para esse T inicial. A execucao medida sequer concluiu o primeiro nivel.
- Chamadas completas do BB consumiram 13,394 s em 15,202 s de parede
  (aproximadamente 88,1%, sem incluir a ultima chamada interrompida).
- Uma chamada com 11 missoes levou 7,426 s, com 14.525.152 nos, LB=138,
  UB=142. Outra, tambem com 11 missoes, levou 4,515 s e 9.994.867 nos.
- Depois dessas chamadas, o melhor makespan publicado no perfil era 368,
  embora os drones terminassem em 142. A otimizacao exata dos drones nao
  corrige a rota de caminhao que limita a solucao.

Arquivos de evidencia: `route_analysis.json`, `profile_summary.json`,
`profile.log` e `comparison_snapshot.csv`. Os tempos sao desta medicao,
com sobrecarga de instrumentacao e carga do computador; nao sao uma medida
de tempo universal ou uma comparacao de CPU equivalente com o artigo.

## Causas identificadas

1. O BB enumera ordens, com crescimento fatorial, em cada mudanca de
   clientes/k. Busca provar o melhor makespan isolado dos drones mesmo
   quando qualquer cronograma suficientemente curto produziria o mesmo
   objetivo global. O perfil confirma o custo elevado dessa escolha.
2. A construcao inicial embaralha e distribui clientes aleatoriamente.
   Nao ha organizacao espacial, 2-opt ou insercao de menor custo. A grande
   reducao offline com a mesma atribuicao confirma a fragilidade das rotas.
3. O SA tem somente trocas de posicoes, transferencias com insercao aleatoria
   e sorteio de k. A referencia PDSTSP-C possui construcao por vizinho mais
   proximo/2-opt e operadores de reinsercao e 2-opt, entre outros recursos.
4. O resfriamento depende de 500 tentativas completas, sem relacao com o
   prazo. Como o BB bloqueia as tentativas, o programa permanece quente;
   aumentar tempo ou ajustar alpha isoladamente nao resolve essa causa.
5. Com k fixo, o limite de drone-tempo/frota e matematicamente valido,
   mas sua carga total e constante ao longo da permutacao. Portanto, nao
   discrimina muitas ordens equivalentes; LB perto de UB ainda pode exigir
   milhoes de nos. A desigualdade do artigo nao torna esse BB eficiente sozinha.
6. Oito processos simultaneos dividem CPU e memoria, mas cada um tem somente
   30 s de parede. A MIC usa CP-SAT com hint inicial otimizado e ate uma hora.
   Nao ha igualdade de metodo, solucao inicial nem orcamento computacional.

Os certificados foram validados independentemente quanto a cobertura,
distancia Manhattan, duracoes e capacidade. Nao surgiu evidencia de erro
de makespan capaz de explicar a diferenca. Isso nao prova equivalencia dos
tempos de drones de 2025 com os usados nos artigos de 2024; essa verificacao
continua pendente. Entretanto, os caminhoes dominam o snapshot, de modo que
a diferenca nas rotas ja explica uma parcela grande sem depender desse ponto.

## Ordem recomendada de correcao

1. Preservar o BB, mas dar como meta o maior tempo atual dos caminhoes.
   Assim que houver cronograma viavel de drones abaixo dessa meta, encerrar
   a busca da ordem: o objetivo global ja e exatamente o tempo dos caminhoes.
   Isso nao prova o otimo isolado dos drones, mas nao perde qualidade no
   makespan daquele vizinho. Se nenhum cronograma atingir a meta, continuar
   o BB para determinar o tempo dos drones.
2. Melhorar a construcao das rotas e acrescentar 2-opt/reinsercao nos
   caminhoes. O teste offline mostra potencial imediato, sem mudar o modelo.
3. Medir tentativas, nos/tempo de BB, movimentos aceitos e niveis atingidos;
   depois adequar o resfriamento ao prazo efetivo, com experimento controlado.
4. Comparar uma execucao por vez com sementes e prazo fixos, antes de
   escolher o grau de paralelismo do lote. Preservar versoes de resultados
   em diretorios diferentes para nao misturar CSV atual com resumos antigos.

## Fontes

- Codigo local: cv.c (construcao/movimentos), drone_bb.c (busca da ordem),
  SA.c (calibracao e resfriamento), scripts/executar_benchmark.py (prazo).
- MIC 2024, secoes 3 e 4: https://doi.org/10.1007/978-3-031-62912-9_13
  PDF: https://iris.unimore.it/bitstream/11380/1342368/2/chapter.pdf
- Cabecalhos e implementacao do projeto vizinho simulateAneelingPDSTSPC-C.
