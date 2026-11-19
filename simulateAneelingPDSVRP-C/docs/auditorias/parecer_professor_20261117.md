# Auditoria para apresentacao ao professor — 17/11/2026

## Parecer

O programa implementa uma heuristica SA com refinamento BB aplicavel ao PDSVRP-C
com os dados locais. A revisao e os testes nao identificaram violacao de viabilidade
nos casos verificados. Isso nao e prova formal de ausencia de defeitos nem de
otimalidade global. A contribuicao e uma heuristica implementada e validada para
um modelo existente; nao foi demonstrada a novidade de uma formulacao matematica.

## Revisao bibliografica e alcance

Foram usados o PDF local do fundador Nguyen e Ha (2023), as auditorias de leitura
das equacoes, o texto COR 2024 e a tabela MIC 2024 novamente localizada no indice
institucional. O acesso direto ao PDF MIC retornou 403. Buscas por titulo,
PDSVRP-c e collective drones com anos 2025/2026 localizaram tambem tese de Corsini
e trabalhos de variantes robustas/multideposito. Nao se afirma leitura integral
de todos os artigos existentes, nem revisao sistematica exaustiva ou inexistencia
de resultados posteriores. Variantes com outros objetivos, incerteza, varios
depositos ou drones nao coletivos nao fornecem automaticamente BKS comparaveis.

- Fundador: PDSTSP-C, um caminhao, modelo energetico, MILP e RnR.
  https://doi.org/10.1287/trsc.2022.1192
- COR: extensao PDSVRP-C, varios caminhoes, MILP e CP; dados sob solicitacao.
  https://doi.org/10.1016/j.cor.2023.106514
- MIC: heuristica min-max VRP com CP-SAT, tabela de 30 instancias grandes.
  https://doi.org/10.1007/978-3-031-62912-9_13
  https://iris.unimore.it/bitstream/11380/1342368/2/chapter.pdf
- Tese de Corsini: contexto dos modelos CP e benchmark, nao novo conjunto
  automaticamente independente de resultados.
  https://www.iris.unimore.it/retrieve/e3ed862d-71a6-453b-8374-582a09782343/TesiDefinitiva_CorsiniAndrea.pdf

## Conformidade do codigo

| Componente | Evidencia e alcance |
| --- | --- |
| Leitura | IDs consecutivos, parametros finitos, dimensoes; arquivos comparados ao ZIP oficial. |
| Caminhoes | Manhattan, velocidade do arquivo, minutos; rotas fechadas no deposito; sem carga maxima adicional. |
| Drones | Missoes deposito-cliente-deposito; k fixo durante toda a missao; duracao tau; -1 proibido. |
| Cobertura | Cada cliente exatamente uma vez, por caminhao ou missao. |
| Sincronizacao | Recurso cumulativo: soma dos k ativos <= frota; reutilizacao no termino. Para drones homogeneos, permite atribuir unidades fisicas. |
| Objetivo | Maximo entre retorno de todos os caminhoes e termino de todas as missoes. |
| Vizinhos | Troca dentro da rota, transferencia, alteracao de k, 2-opt e reinsercao; deltas conferidos contra recalculo integral. |
| SA | Calibracao, aceita pioras por exp(-delta/T), resfriamento, atual e melhor separados. |
| BB | Refina missoes com clientes/k fixos; teto de 100.000 nos e parada por meta do caminhao; nao prova otimo global. |
| Persistencia | Valida em C e Python, grava melhorias estritas com troca atomica e bloqueio; registros historicos nao sao novas execucoes. |

Os parametros energeticos sao lidos mas a producao consulta tau pronto. Alterar
bateria/peso sem regenerar tau nao atualiza a fisica. O modelo impresso tem
contradicoes conhecidas, explicitadas no relatorio equacoes_completas_20261117.md.
O SA usa uma proposta nao necessariamente simetrica sem correcao de Hastings:
e uma heuristica de otimizacao, sem alegacao de amostragem exata de Boltzmann
ou garantia assintotica derivada apenas da formula de aceitacao.

Limites de robustez: o leitor C aceita duracao zero, enquanto o validador Python
rejeita missoes de duracao zero; os arquivos oficiais possuem apenas positivos
ou -1. A validacao C confere matriz mas nao recalcula energia. Limites de massa
e tamanho de formacao dependem da autenticidade de tau. A busca nao implementa
warm start a partir do registro. Essas limitacoes nao invalidaram certificados
oficiais verificados, mas impedem garantir comportamento fisico para entradas
arbitrariamente modificadas. Nao foi refeita nesta rodada a instrumentacao ASan/UBSan.

## Verificacao executada nesta rodada

```json
{
  "instancias": 60,
  "configuracoes": 180,
  "vizinhos_validados": 90000,
  "testes_python": 29,
  "certificados_salvos_validados": 120,
  "certificados_integracao": 1,
  "compilacao": "C11, Wall, Wextra, Wpedantic, Werror, O2",
  "trajetorias_sa": [
    "15-rc-c",
    "50-r-e",
    "200-r-e"
  ],
  "saida_apenas_tabela": true,
  "validacao_python_otimizado": true,
  "limite": "Viabilidade relativa aos dados locais; sem prova de otimo global nem recalculo fisico dos voos."
}
```

Tambem foram revalidados os 14 certificados abaixo usando a implementacao
Python independente. A auditoria anterior validou 2.160 execucoes, 480 transferencias
de certificados entre matrizes e os 60 certificados originais de um caminhao.
Resultados de duas rodadas nao devem ser confundidos: a suite atual testa o
codigo atual; o experimento de 2 segundos tem seus proprios hashes e protocolo.

## Achado principal: as 14 solucoes usam somente caminhoes

Este achado corrige a justificativa excessivamente ampla dada anteriormente:
a matriz de drones desconhecida de MIC nao impede validar ESTAS 14 solucoes,
pois nenhuma delas utiliza drones. Elas sao solucoes min-max VRP admissiveis
tambem no PDSVRP-C, que permite deixar drones ociosos. Resta conferir identidade
de coordenadas, deposito, custos de caminhao, numero de caminhoes e atualidade
do BKS antes de anunciar recordes universais. Os certificados originais e as
convencoes declaradas sustentam a comparacao, mas nao substituem arquivos MIC.

O UB foi fortalecido por monotonicidade: mais caminhoes nao podem piorar a
melhor solucao, pois podem ficar ociosos. Assim, 144-rc-c/S=5 usa referencia
72 (S=4), em vez de 92; 187-rc-e/S=4 e S=5 usam 124 (S=3), em vez de 148/128.
As 14 melhorias persistem mesmo contra essas referencias mais fortes.
Essa normalizacao e analitica e nao altera o catalogo historico publicado.

| Instancia | S | UB publicado da linha | UB monotono | Local | Gap vs UB monotono % |
| --- | ---: | ---: | ---: | ---: | ---: |
| 103-rc-c | 4 | 76 | 76 | 72 | -5.263 |
| 114-rc-c | 5 | 64 | 64 | 60 | -6.250 |
| 126-rc-e | 2 | 152 | 152 | 148 | -2.632 |
| 144-rc-c | 3 | 116 | 116 | 112 | -3.448 |
| 144-rc-c | 5 | 92 | 72 | 68 | -5.556 |
| 165-r-c | 3 | 132 | 132 | 128 | -3.030 |
| 167-r-e | 5 | 116 | 116 | 112 | -3.448 |
| 173-r-c | 5 | 92 | 92 | 88 | -4.348 |
| 173-rc-c | 3 | 116 | 116 | 108 | -6.897 |
| 173-rc-c | 5 | 80 | 80 | 76 | -5.000 |
| 187-rc-e | 4 | 148 | 124 | 116 | -6.452 |
| 187-rc-e | 5 | 128 | 124 | 112 | -9.677 |
| 200-r-e | 2 | 216 | 216 | 208 | -3.704 |
| 200-r-e | 5 | 120 | 120 | 116 | -3.333 |

Nao atribuir essas melhorias especificamente ao SA ou ao BB sem ablacao:
a construcao inicial e o 2-opt tambem participam. Os testes anteriores tiveram
apenas 2 segundos por execucao; muitos foram interrompidos. Nao demonstram
superioridade estatistica, desempenho final do SA nem vantagem de tempo frente
aos solvers dos artigos. As variantes Euclidiana e de equacoes fisicas devem
ser apresentadas como sensibilidades, nunca misturadas aos 14 casos oficiais.

## Formulacao recomendada para a apresentacao

“Desenvolvemos uma heuristica SA+BB para o PDSVRP-C e verificamos sua viabilidade
por validadores independentes e testes sistematicos. No benchmark local oficial,
obtivemos certificados com valores inferiores aos UBs MIC 2024 consultados em
14 configuracoes, inclusive apos fortalecer as referencias por monotonicidade.
Esses certificados usam somente caminhoes e independem dos tempos de drones.
Sua caracterizacao como novos melhores resultados atuais requer confirmar
a correspondencia dos dados de caminhões e revisar resultados posteriores.”

## Resumo breve do algoritmo

Constroi rotas por insercao gulosa e 2-opt, explora cinco tipos de vizinhanca
com Simulated Annealing e escalona drones coletivos como recurso cumulativo.
Um BB limitado refina a ordem das missoes. Minimiza o ultimo retorno ao deposito
e conserva apenas melhorias validadas. E uma solucao heuristica valida para o
modelo local, com evidencia de valores menores que referencias publicadas,
sem prova de otimalidade global ou superioridade geral do metodo.

Reproducao: python scripts/verificar_projeto.py; python scripts/relatar_auditoria_professor.py.
Certificados, sementes e CSV: output/auditoria_professor/.
