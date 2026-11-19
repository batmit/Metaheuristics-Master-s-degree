# Referencias da comparacao PDSVRP-C

## Tempos de computacao (07/11/2026)

`literature_times.json` registra tempos totais das pequenas extraidos das tabelas
3 e 4 de `cor2024.html` (preprint). Para cada configuracao, seleciona o menor
`Sec_tot` entre modelos cujo UB coincide com o BKS do catalogo. O tempo inclui
a prova quando o modelo encerra; nao e o tempo ate encontrar a melhor solucao.
Modelo e tabela ficam registrados em `time_source`. Reproduzir com
`python scripts/extrair_tempos_literatura.py`. Nao atribuir estes tempos a versao
publicada sem nova conferencia. Plataforma informada no preprint: Intel Core
i7 12700F, 32 GB RAM, Gurobi 10.0 ou OR-Tools CP-SAT 9.6, limite de 3600 s.

Para as grandes, os tempos correspondentes aos BKS MIC nao foram confirmados.
O repositorio retornou HTTP 403 durante esta consulta, inclusive no endereco
`https://iris.unimore.it/retrieve/0929c99b-71a5-4f38-ba73-8c5a6d1e2eb0/chapter.pdf`.
Esses tempos permanecem nulos (`N/D` na tabela); nao se reutiliza o tempo de
outro artigo para uma solucao diferente nem se converte limite em tempo medido.

Atualizacao em 06/11/2026: o PDF original de Nguyen e Ha (2023), fornecido pelo
usuario, permitiu recalcular a 126-rc-e. As 1.512 entradas da matriz coincidem
com o codigo fisico original e as regras de massa total, cooperacao e preparo.
O certificado RnR=144 de UM caminhao tambem foi reconstruido e validado.
O certificado MIC 2024 de CINCO caminhoes com valor 76 continua indisponivel
nas fontes consultadas. Detalhes e ambiguidades do PDF:
`docs/auditorias/recalculo126.md` na raiz do projeto.

Pesquisa em 30/10/2026. A publicacao mais recente localizada com resultados
por instancia do PDSVRP-C foi MIC 2024. Nao confundir data de indexacao/upload
do PDF com ano da publicacao. Os dados ORLab de 2025 de PDSTSP-C/PDSOP-C nao
foram usados como BKS do PDSVRP-C.

## Grandes: MIC 2024

Montemanni, Dell'Amico e Corsini, *Combining Heuristics and Constraint
Programming for the Parallel Drone Scheduling Vehicle Routing Problem
with Collective Drones*, LNCS 14753, pp.136-142, 2024.

- DOI: https://doi.org/10.1007/978-3-031-62912-9_13
- PDF: https://iris.unimore.it/bitstream/11380/1342368/2/chapter.pdf
- Registro: https://iris.unimo.it/handle/11380/1342368
- Tabela 1, pagina impressa 140 (pagina 5 do PDF), 30 instancias e s=2,3,4,5.

Transcrevemos os quatro pares de New bounds. Ha tres casos em que o UB
da coluna VRP e menor que o UB de New bounds: 126-r-c com s=3 (108),
s=4 (88) e s=5 (80). Esses valores VRP foram usados como BKS, pois uma
solucao sem drones e viavel para o PDSVRP-C. Os LB de New bounds foram
mantidos. Nenhum dos 120 casos grandes tem LB igual a UB.

## Pequenas: COR 2024, versao publicada

Montemanni, Dell'Amico e Corsini, *Parallel drone scheduling vehicle routing
problems with collective drones*, Computers & Operations Research 163, 106514.

- DOI: https://doi.org/10.1016/j.cor.2023.106514
- PDF: https://iris.unimore.it/retrieve/66a28803-9167-4680-95a3-e00aa5929f68/1-s2.0-S0305054823003787-main.pdf
- Tabelas 3 e 4, paginas impressas 7 e 8: 30 instancias com s=2 e s=3.

As 60 configuracoes pequenas possuem otimo comprovado por pelo menos um
modelo. Em particular, 26-r-c/s=3 e comprovada pelo MILP, embora os dois
modelos CP nao fechem o gap. O filtro considera a melhor prova publicada,
nao apenas o status de um dos modelos.

## Reproducao e limites

`scripts/preparar_literatura.py` contem a transcricao numerica e gera
`instances/literature_pdsvrp_c.csv`: 180 configuracoes, 60 comprovadas e 120
abertas. Os nomes correspondem exatamente aos arquivos locais; 101-r-c e
a grafia usada na MIC (o preprint antigo omite um hifen em algumas tabelas).
`cor2024.html` e uma copia do preprint para consulta, mas os valores de
pequenas foram conferidos nas tabelas da versao publicada.

Os PDFs foram consultados pelo extrator web; o download direto local e a
captura de tela encontraram bloqueio do repositorio. A transcricao numerica
esta preservada no script, com links para as paginas de origem.

A comparacao e com as referencias localizadas, nao uma garantia universal
de que nao existam resultados posteriores nao encontrados. Alem disso, os
tempos locais de drones vieram do pacote oficial de 2025; nao foi comprovada
a identidade dessas tabelas com os tempos usados nos artigos de 2024. Um
valor menor que um LB publicado e destacado para verificar dados, em vez
de ser declarado automaticamente como nova solucao recorde.
