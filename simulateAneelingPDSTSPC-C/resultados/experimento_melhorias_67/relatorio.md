# Avaliacao das melhorias do SA na 67-c-c

Foram realizadas **1090 execucoes comparativas**, todas com certificados validados independentemente do avaliador C. O codigo foi integrado na branch `bab` apenas apos os ensaios. Nao houve commit nem alteracao de outra branch.

## Decisao incorporada

Foi incorporada somente a transferencia em grupos. A opcao `ParametrosSA.movimentosGrupo` permite liga-la ou desliga-la; o padrao da API continua `false`, e a `main` ativa `true` apenas para 67-c-c. As outras tres instancias, o numero de repeticoes e a inicializacao da semente da main permanecem com a configuracao anterior.

O movimento compartilha os tipos PARA_DRONES/PARA_CAMINHAO existentes: em 1/4 das transferencias, tenta mover um grupo. Prefere todos os clientes do mesmo endereco quando a transferencia e viavel; alternativamente sorteia uma ancora e seleciona 2 a 4 clientes proximos, com desempates aleatorios. Grupos retornando ao caminhao usam melhor insercao com desempates aleatorios. As quantidades de drones continuam sorteadas entre as viaveis. A ordem relativa das missoes que permanecem nos drones e preservada.

A avaliacao dos movimentos individuais continua incremental. Grupos recalculam o custo da rota alterada e o cronograma dos drones. O B&B e seus filtros, o arquivo de quatro elites, os pesos, o criterio de aceitacao e a calibracao nao foram alterados em producao. As propostas que nao tiveram ganho adicional consistente ficam apenas nas copias experimentais.

## Metodo

- Mesmas sementes dentro de cada comparacao; blocos de cinco execucoes intercalados, com ordem de variantes alternada.
- Quatro processos simultaneos, mesma compilacao GCC 10.3.0, C11 e `-O2`.
- Triagem e combinacoes: 30 sementes por variante, limite de 3 segundos incluindo construcao e calibracao. Se o resfriamento chega ao fim antes do prazo, a versao experimental continua com temperatura baixa ate esgotar o prazo; isso vale igualmente para o controle.
- Confirmacao: 100 sementes novas por variante, com o mesmo limite de 3 segundos. Essas sementes nao foram usadas para selecionar as combinacoes.
- Ciclo completo: mais 100 sementes novas por versao, usando o resfriamento original, sem limite adicional de tempo ou avaliacoes. Essa comparacao verifica o resultado final do algoritmo utilizado na main.
- Controles externos: 10 sementes por versao em cinco outras instancias, limite de 6 segundos. Sao diagnosticos curtos, especialmente nas instancias grandes.
- O controle com melhorias desligadas reproduziu exatamente uma solucao do lote anterior de 400, incluindo rota, missoes e quantidades.

Os limites de tempo sao verificados periodicamente; pequenos excessos de milissegundos sao registrados. Tempos foram medidos com processos concorrentes na mesma maquina e nao constituem uma garantia de desempenho em outro computador.

## 1. Sugestoes isoladas

| Versao | n | Melhor | Media | Mediana | Pior |
|---|---:|---:|---:|---:|---:|
| Controle anterior | 30 | 76 | 80.667 | 80 | 92 |
| Grupos | 30 | 76 | 76.800 | 76 | 92 |
| Folga | 30 | 76 | 80.667 | 80 | 92 |
| Diversidade das elites | 30 | 80 | 82.000 | 80 | 92 |
| Reparo de carga | 30 | 76 | 83.133 | 80 | 92 |
| Inicializacao + retomadas | 30 | 76 | 83.600 | 80 | 104 |


Somente grupos apresentou ganho claro nesta triagem. Folga empatou na media; diversidade, reparo e inicializacao/retomadas nao melhoraram esta amostra sob o mesmo tempo. Isso nao demonstra que essas ideias sejam inuteis em outras configuracoes.

## 2. Combinacoes

| Versao | n | Melhor | Media | Mediana | Pior |
|---|---:|---:|---:|---:|---:|
| Controle anterior | 30 | 80 | 82.933 | 80 | 92 |
| Grupos | 30 | 76 | 77.867 | 76 | 92 |
| Grupos + folga | 30 | 76 | 76.667 | 76 | 92 |
| Grupos + diversidade | 30 | 76 | 76.800 | 76 | 92 |
| Grupos + reparo | 30 | 76 | 78.667 | 78 | 84 |
| Grupos + inicializacao + retomadas | 30 | 76 | 76.267 | 76 | 84 |
| Todas as opcoes | 30 | 76 | 80.467 | 80 | 88 |


Foram testados grupos com cada outra proposta e a combinacao de todas. O reparo combinado avalia a transferencia de grupo e o ajuste das quantidades conjuntamente, antes da aceitacao. As combinacoes mais promissoras foram levadas a sementes novas. Nao foram examinados exaustivamente todos os subconjuntos possiveis de opcoes.

## 3. Confirmacao em sementes novas, mesmo tempo

| Versao | n | Melhor | Media | Mediana | Pior |
|---|---:|---:|---:|---:|---:|
| Controle anterior | 100 | 80 | 83.360 | 80 | 92 |
| Grupos | 100 | 76 | 77.200 | 76 | 92 |
| Grupos + folga | 100 | 76 | 77.160 | 76 | 92 |
| Grupos + inicializacao + retomadas | 100 | 76 | 77.200 | 76 | 100 |


Grupos venceu o controle em 95 sementes, empatou em cinco e nao perdeu em nenhuma. Frente a grupos, grupos+folga venceu quatro, perdeu tres e empatou 93; a diferenca media foi somente -0,04 minuto. Grupos+inicializacao+retomadas teve os mesmos totais de vitorias/empates/derrotas e media igual a grupos. Portanto a confirmacao nao sustentou ganho adicional consistente das combinacoes. Os testes de sinais e todas as diferencas pareadas estao em `confirmacao/resumo.json`; as comparacoes da triagem sao exploratorias e nao receberam correcao por multiplas comparacoes.

## 4. Resfriamento completo, sem corte por tempo

| Versao | n | Melhor | Media | Mediana | Pior |
|---|---:|---:|---:|---:|---:|
| Controle anterior | 100 | 76 | 80.080 | 80 | 84 |
| Grupos | 100 | 76 | 76.080 | 76 | 80 |


- Media: **80.08 -> 76.08 minutos**, reducao de 5.00%.
- Frequencia de 76 minutos: **4/100 -> 98/100**.
- Comparacao pareada: **94 vitorias, 6 empates, nenhuma derrota** para grupos.
- Tempo medio registrado: 2.989 -> 2.352 segundos, sob concorrencia.
- Melhor resultado: **76 minutos em ambas**. Nenhuma das variantes atingiu 74 ou menos na 67-c-c.

O ganho demonstrado foi em media, consistencia e convergencia, sem melhorar o melhor makespan conhecido pelo projeto. A referencia ORLab de 2025 e UB=74, LB=73; o gap do melhor continua em 2,70% frente ao UB, sem prova de otimalidade.

Fonte da referencia: https://orlab.com.vn/download/data-and-results-of-pdstsp-c-and-pdsop-c/

## 5. Controles externos exploratorios

| Instancia | Media controle | Media grupos | Melhor controle | Melhor grupos |
|---|---:|---:|---:|---:|
| 105-rc-e | 134.40 | 126.80 | 120 | 116 |
| 165-r-c | 368.00 | 341.60 | 368 | 208 |
| 167-r-e | 332.40 | 303.60 | 224 | 204 |
| 181-r-e | 344.80 | 337.20 | 244 | 240 |
| 20-c-c | 43.00 | 43.00 | 43 | 43 |

Cada celula usa dez execucoes, com seis segundos por execucao. Algumas instancias grandes ficaram proximas da solucao inicial, logo esse orcamento curto nao substitui uma comparacao de ciclos completos. A amostra nao justifica ativar a mudanca globalmente; por isso a main a habilita apenas na 67-c-c. Ha perdas em sementes individuais mesmo onde a media melhora, conforme `controles/resumo.json`.

## Verificacao e reproducao

Todos os 1090 certificados foram conferidos com `Fraction`, sem importar o avaliador ou escalonador C: cobertura unica dos clientes, percurso Manhattan com retorno ao deposito, duracao da matriz tau para cada quantidade, capacidade simultanea e testemunha com drones identificados sem interrupcao de missoes.

A integracao passou nos **14 testes C**, incluindo grupos completos que esvaziam um recurso e uma caminhada de 3.000 movimentos com verificacao de deltas e reconstrucoes completas. O teste experimental anterior verificou 4.500 movimentos e as 64 combinacoes de opcoes. A main integrada compilou sem avisos; dez solucoes integradas (cinco com grupos e cinco sem) reproduziram exatamente os certificados experimentais, incluindo todos os vetores.

Arquivos:

- `execucoes_1090.csv`: todos os resultados escalares.
- Pastas de cada etapa: JSONL com todas as solucoes, CSV, resumo pareado, melhor solucao por variante e copia dos fontes.
- `original/` e `hashes_originais.json`: codigo anterior preservado.
- `candidato/`: todas as propostas, independentes por mascara.
- `integracao/validacao.json`: testes e reproducoes da versao incorporada.
- `integracao/main_integrado.exe`: programa compilado a partir dos fontes finais.

Para repetir os ensaios, use `rodar.py` com um nome de pasta novo e sementes novas. As mascaras sao: 0 controle, 1 grupos, 2 folga, 4 diversidade, 8 reparo, 16 inicializacao diversa e 32 retomadas. Os fontes experimentais partem do snapshot anterior, mesmo depois da integracao em producao.
