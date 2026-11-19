# Verificacao do artigo anexado e recálculo da 126-rc-e

Data: 06/11/2026. Todos os experimentos desta pasta sao isolados do registro
`results/best.json` e das tabelas de producao.

## 1. Identificacao do artigo

O PDF fornecido e **The Parallel Drone Scheduling Traveling Salesman Problem
with Collective Drones**, Minh Anh Nguyen e Minh Hoang Ha, Transportation
Science 57(4), 866-888, 2023, DOI 10.1287/trsc.2022.1192. Ele e a fonte do
modelo fisico, das instancias e da heuristica Ruin-and-Recreate (RnR).
Esse problema usa UM caminhao. Portanto, e o artigo-base correto para
recalcular os drones, mas nao e o artigo que publica 76 com CINCO caminhoes.

O valor 76 vem de Montemanni, Dell'Amico e Corsini, MIC 2024, DOI
10.1007/978-3-031-62912-9_13, Tabela 1, pagina impressa 140. Ele e um UB,
com LB=41. Ambos os modelos CP-SAT com hint-start atingem 76. A abordagem
combina uma solucao de caminhões com modelos de programacao por restricoes;
usa OR-Tools 9.6 e limite de uma hora. Nao e Simulated Annealing.
Fonte: https://iris.unimore.it/bitstream/11380/1342368/2/chapter.pdf

## 2. Regras usadas no recálculo

Foram lidas e conferidas visualmente as equacoes e tabelas do PDF anexado,
especialmente Sec.3.2, 3.3, 3.6, 5.1 e 6.2.1, pp.870-874,880,883 impressas.

- Viagem de drone em seis fases: decolagem, cruzeiro e pouso, na ida e na volta.
- Carga compartilhada: cada unidade transporta w/k na ida; volta sem encomenda.
- Altitude de 100m e velocidades verticais de 2m/s: 200s no total de fases verticais.
- Velocidades de cruzeiro testadas de 4 a 28m/s, passo de 0,2m/s.
- Corrente de cada fase limitada a 24A, tensao de 12,8V, capacidade nominal de 12Ah,
  Peukert p=1,1 e uso maximo de 90% da capacidade nominal.
- Frame de 1,2kg e bateria de 1,55kg por unidade; quatro rotores de raio 0,15m.
- No maximo seis unidades colaboram numa missao; a frota total do arquivo e 12.
- A **massa total** deve satisfazer `k*(1,2+1,55)+peso_encomenda <= 25kg`.
- Preparacao de um minuto somente para k>1. Sem essa parcela para k=1.
- Duracao final em minutos, arredondada para cima.

As equacoes de potencia, coeficientes e a busca de velocidades foram
reexecutados a partir do C++ original distribuido no pacote ORLab.
Removi da copia de experimento somente a dependencia e a rotina de plotagem
Gnuplot; o arquivo original foi preservado.

## 3. Resultado completo da matriz

Foram avaliadas fisicamente as 126*6=756 combinacoes com k permitido.
As colunas k=7,...,12 recebem -1 pelo limite de cooperacao.
**Todas as 1.512 entradas da matriz completa coincidem numericamente com
a tabela atualmente usada no projeto. Nenhuma entrada precisa ser trocada
para reproduzir o codigo original e as regras acima.**

Arquivo entregue: `tau_kj_126-rc-e_recalculado.txt`.
Conferencia automatica: `comparacao_fisica.json`, campo `complete_rules`:
`total_pairs=1512`, `matching_pairs=1512`, `differences=[]`.
A formatacao textual do arquivo difere do original; os valores sao iguais.

A suspeita anterior de cinco divergencias foi esclarecida. Ao verificar so
energia, ignorando massa total, k=6 parecia viavel para cinco clientes:

| Cliente | Encomenda (kg) | Massa total com 6 unidades (kg) | Decisao correta |
| --- | --- | --- | --- |
| 41 | 12,07 | 28,57 | Inviavel |
| 53 | 9,49 | 25,99 | Inviavel |
| 60 | 8,98 | 25,48 | Inviavel |
| 69 | 8,63 | 25,13 | Inviavel |
| 117 | 10,26 | 26,76 | Inviavel |

Ao aplicar a regra de 25kg do PDF, os cinco -1 da tabela ficam confirmados.
Essa foi uma omissao da primeira reproducao de diagnostico, nao do algoritmo
de producao, que sempre recebeu os -1 corretos da tabela.

## 4. O cliente 46 depois do recálculo

O cliente 46 esta em (14,14), peso 2,25kg; distancia euclidiana de ida
19.798,989873m. O recálculo confirma a inviabilidade energetica com k=1,...,6.
Com k=7 ou 8, o codigo energetico encontra velocidades viaveis, mas essas
formacoes excedem o limite de seis unidades declarado no artigo.
Logo, continuam proibidas. Nao se deve preencher essas colunas como se
fossem missoes permitidas.

O cliente continua obrigatorio no caminhao. Seu tempo minimo de ida e volta
e `2*(14+14)/30*60 = 112min`. Como ja ha certificado viavel com 112 para
S=4 e S=5, esses certificados sao otimos **para o conjunto local atual**.
Essa prova nao identifica nem valida a solucao MIC de 76.

## 5. Solucoes distribuídas que foram efetivamente reconstruidas

O arquivo original `Upload-SuperDrone/bks.txt` fornece um certificado da
126-rc-e com UM caminhao. Reconstrui os horarios de drones pela ordem de
precedencias das cadeias individuais, recalculando todas as duracoes:

- 93 clientes em caminhao, 33 missoes de drones;
- rota do caminhao: 144min;
- maior termino de drones: 138min;
- makespan: **144min**, igual ao certificado e ao RnR da Tabela 3 do PDF anexado.

Certificado reconstruido: `certificado_rnr_original_s1.json`.

Tambem reconstrui o certificado do log ORLab de 2025: UM caminhao, 12 drones,
96 clientes no caminhao e 30 missoes; makespan **136min**, igual ao log.
Certificado: `certificado_publico_2025_s1.json`.

Ambos passaram pelas checagens independentes de cobertura, rotas fechadas,
duracoes, capacidade simultanea e objetivo. Essa concordancia apoia a leitura
das tabelas e dos IDs no projeto. Os certificados de 144 e 136 NAO sao a
solucao de 76 com cinco caminhoes e nao a substituem.

## 6. Nova execucao do algoritmo

Recompilei uma instrumentacao que usa os mesmos arquivos `SA.c`, `cv.c`,
`drone_bb.c`, construcao inicial e parametros de producao. Somente acrescentei
contadores e resumo; o serializer de certificados e o do proprio `main.c`.
Execucao completa no Ubuntu/WSL, S=5, mesma semente 1307 nos dois casos:

| Matriz | Inicial | Final | Tentativas | Niveis | Chamadas BB | Tempo observado |
| --- | --- | --- | --- | --- | --- | --- |
| Distribuida | 112 | 112 | 538.500 | 1.077 | 0 | 2,139s |
| Recalculada | 112 | 112 | 538.500 | 1.077 | 0 | 2,061s |

As duas execucoes encerraram normalmente, codigo 0, sem atingir o teto
externo de seguranca. Os certificados foram validados em C e Python.
Temperatura calibrada: 497,8518112499371. Os resumos e certificados coincidem.
Esses tempos sao observacoes de duas execucoes controladas, nao uma estimativa
estatistica de desempenho geral.

O BB nao foi acionado porque o melhor ja era a solucao de 112 sem drones;
ela nao foi estritamente melhorada durante a busca. O SA preservou-a.
O gap numerico para 76 permanece `100*(112-76)/76 = 47,368421%`.
Portanto, recalcular os tempos desta maneira nao resolveu a discrepancia.

Logs: `sa_distribuida.log`, `sa_recalculada.log`.
Resumo: `experimentos_sa.json`.

## 7. Ambiguidades do PDF, explicitadas

O texto e o codigo disponibilizado nao sao identicos em todos os detalhes:

1. As Sec.5.1 e 6.2.1 dizem um MINUTO de preparacao; a Tabela 5 imprime
   "Seconds 1". Usei um minuto, consistente com as duas descricoes e com a
   matriz original. Preparacao so existe para k>1, como explica a Sec.3.2.
2. A Eq.4 impressa usa `Cd >= beta*Cnom`, embora Cd seja capacidade consumida.
   O codigo original exige reserva de pelo menos 10%, equivalente a
   `Cd <= 0,9*Cnom`. Para reproduzir os dados, segui essa condicao do codigo
   original, sem usar literalmente o sinal da equacao impressa.
3. A linha 14 do Algoritmo 2 repete vin no par de velocidades. O texto
   explicativo e o codigo original preservam o par vin,vout.
4. A Eq.24 impressa omite um termo quadratico de arrasto presente no C++
   distribuido. Registrei tambem uma analise de sensibilidade removendo
   somente esse termo, sem assumir que essa e a implementacao dos autores.

Essa variante da Eq.24 muda 329 entradas da matriz: 1.183/1.512 continuam
iguais. O cliente 46 passa a ter duracao 52 com k=6, mas o cliente 97 continua
inviavel e exige pelo menos 108min de caminhao. Portanto, mesmo essa troca
isolada nao permite atingir 76 com os demais dados locais.
Resultados: `equacao24_impressa_comparacao_fisica.json` e matriz correspondente.
Nao substitui a tabela de producao por essa variante ambigua.

O PDF e suficiente para identificar regras fundamentais, mas uma leitura
inteiramente literal contem contradicoes. A reproducao principal deste
experimento fica ancorada no codigo original e nas regras textuais coerentes,
e foi confirmada pela matriz e pelo certificado original de 144.

## 8. O que segue sem verificacao integral

O certificado de 76 (rotas, clientes/k, horarios), a matriz tau exata usada
em MIC 2024 e as configuracoes executaveis daquele experimento nao foram
encontrados nas fontes publicas consultadas. O artigo divulga o valor e os
modelos, mas nao essas rotas. A pagina do artigo COR informa dados sob pedido:
https://www.sciencedirect.com/science/article/pii/S0305054823003787

Sem esses arquivos, nao e correto concluir que o artigo possui uma solucao
invalida nem que a nossa matriz reproduz aquele experimento de cinco
caminhoes. O resultado verificado e mais restrito: a matriz atual reproduz
o codigo original disponivel e o certificado RnR=144 do artigo-base; o
recálculo e a nova execucao do SA nao explicaram nem eliminaram o gap para 76.

Para encerrar a investigacao, faltam os autores confirmarem a matriz tau,
frota total, limite de cooperacao, limites de massa, versao do modelo fisico
e o certificado da 126-rc-e/S=5 que originou UB=76. Nenhuma mensagem foi enviada.

## 9. Reproducao

```
python3 scripts/reproduzir_fisica126.py --all
python3 scripts/executar_recalculo126.py
python3 scripts/verificar_literatura126.py
```

Os dois primeiros comandos requerem G++/GCC. O terceiro tambem usa o CSV
de sensibilidade k12 ja preservado nesta pasta. Nenhum deles atualiza
`results/best.json` ou os arquivos de `instances/drone_times/`.
