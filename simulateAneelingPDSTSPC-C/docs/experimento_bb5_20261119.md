# Experimento: B&B apos cinco propostas sem melhora por recurso

## Variante implementada

A variante e opcional: `ParametrosSA.estagnacaoRecursoBB=5`. O valor padrao zero
preserva o algoritmo usado pelo executavel de producao. Cada iteracao e uma
proposta do laco interno do SA, incluindo propostas rejeitadas. Os contadores
comparam os tempos da solucao corrente antes/depois da aceitacao da proposta.
Uma reducao estrita maior que 1e-9 reinicia o contador do recurso; empate ou
piora incrementam. Ao chegar a cinco, aplica-se o B&B nesse recurso corrente e
reinicia-se seu contador, mesmo se a chamada nao melhorar. Ambos podem disparar
na mesma proposta, primeiro caminhao e depois drones. Movimentos compostos entre
niveis e retomadas nao sao propostas contadas por esse mecanismo.

O refinamento procura melhorar o tempo de cada recurso, inclusive quando ele
nao e o gargalo do makespan. Nao ha cache nem supressao de chamadas repetidas
sobre a mesma configuracao. O melhor global continua preservado pela elite.

Cada chamada tem no maximo 10000 nos e prazo de 20 ms, reduzido ao prazo restante
do SA. Os prazos sao cooperativos: sao verificados ao entrar nos nos, e preparacao,
um bound ou uma avaliacao final ainda podem provocar pequeno excesso de tempo.

## B&B do caminhao

Mantem fixo o conjunto de clientes e enumera prefixos de rota. Para restantes U,
ultimo cliente v e custo fixado g, usa uma MST em U e conexoes distintas v->U e
U->deposito. Na raiz essa relaxacao e uma 1-tree. Fortalece o bound com oito
iteracoes de penalidades lagrangianas: soma pi_i a cada extremidade em U e
subtrai 2*sum(pi_i). Todo caminho residual completo tem grau dois em U, tornando
cada iteracao um limite inferior valido; retém-se o maior limite encontrado.
Nao afirma obter o melhor limite possivel de Held-Karp com apenas oito passos.
Os filhos sao ordenados por proximidade ao ultimo vertice. O incumbente e a rota
recebida. A tolerancia de poda e relativa de 1e-10.

Referencias: [Held e Karp (1970)](https://doi.org/10.1287/opre.18.6.1138) e
[Held e Karp (1971)](https://research.ibm.com/publications/the-traveling-salesman-problem-and-minimum-spanning-trees-part-ii).

## B&B dos drones

Reutiliza a implementacao existente, acrescentando prazo. Otimiza quantidades
de drones mantendo a ordem de processamento e a regra de primeira janela
viavel. Nao otimiza todas as ordens nem o conjunto de clientes atribuidos.
As chamadas periodicas desativam a meta de igualar o outro recurso, pois o
experimento pede a melhora separada de cada tempo. A chamada final original
permanece habilitada nos testes por tempo, nos dois tratamentos.

## Protocolo

- Compilacao: GCC, `-O2 -std=c11 -Wall -Wextra -pedantic`, mesmos fontes e binario.
- Tratamentos: base (`estagnacaoRecursoBB=0`) e variante (`=5`).
- 60 instancias, uma semente pareada, teto de 1 s incluindo calibracao.
- 12 instancias estratificadas, tres sementes pareadas, teto de 2 s.
- As mesmas 12 instancias e tres sementes, exatamente 1000 propostas, sem teto
  global, um nivel de temperatura, sem calibracao, macros, retomadas ou B&B final.
  Este ultimo desenho mede o custo por trabalho de SA, nao reproduz uma busca
  completa de producao.
- Confirmacao adicional em quatro instancias, uma semente, teto de 30 s.
- Execucao sequencial com ordem dos tratamentos alternada; sementes iguais
  nao implicam propostas iguais depois que os refinamentos mudam o estado.
- Tempos sao de parede; a medicao principal exclui carregamento e construcao
  inicial, iguais nos tratamentos. Inclui calibracao nos testes por tempo.
- As estatisticas de chamadas/nos/tempo de B&B contabilizam o mecanismo
  periodico. A chamada final preexistente esta no tempo total, nao nesses contadores.
- Cada solucao final e validada em Python, separadamente do avaliador C:
  cobertura unica, rota Manhattan, duracoes oficiais, intervalos e uso da frota.
- Os recordes de `resultados/relatorio.csv` nao sao usados nem atualizados.

Resultados brutos, certificados e resumo ficam em
`resultados/experimento_bb5_20261119/`. Hardware, versoes e hashes ficam em
`proveniencia.json`. Sao experimentos exploratorios locais, sem comparacao
direta de velocidade com hardware da literatura.

## Reproducao

```powershell
gcc -O2 -std=c11 -Wall -Wextra -pedantic -I. tests/benchmark_bb5.c SA.c cv.c lerInstancias.c -lm -o tmp/benchmark_bb5.exe
python tests/comparar_bb5.py
python tests/comparar_bb5_30s.py
python tests/executar.py
```

O teste `test_bb_caminhao.c` compara 40 problemas pequenos com enumeracao
exaustiva independente (incluindo custos fracionarios), verifica limites de
nos e prazo, e verifica duas chamadas por recurso em dez propostas sem melhora.

## Resultados medidos em 19/11/2026

Foram realizadas 272 execucoes (136 pares). Todos os certificados finais foram
aprovados pelo verificador independente. A base e a variante foram executadas
do zero; estes valores nao sao os recordes historicos acumulados.
Ao final, os 14 testes C e o teste de integracao passaram, sem avisos de compilacao.

| Protocolo | Pares | Variante melhor | Empate | Variante pior | Tempo medio base (s) | Tempo medio BB5 (s) | Propostas medias base | Propostas medias BB5 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Teto 1 s, todas as instancias | 60 | 0 | 14 | 46 | 0.646020 | 1.000993 | 231547 | 2275 |
| Teto 2 s, 12 instancias x 3 sementes | 36 | 0 | 1 | 35 | 1.528848 | 2.000150 | 223496 | 2083 |
| 1000 propostas, 12 instancias x 3 sementes | 36 | 24 | 12 | 0 | 0.002925 | 2.664120 | 1000 | 1000 |

Nos testes de 1000 propostas, a mediana das razoes pareadas de tempo BB5/base
foi 885.09. Sao execucoes curtas com temperatura inicial fixa: mostram custo
por proposta e efeito do refinamento, nao qualidade final de uma busca completa.
Nos testes com teto de tempo, a base pode encerrar naturalmente antes do teto.

| Protocolo | Gap medio base a UB 2025 (%) | Gap medio BB5 (%) | Fracao do tempo BB5 nos B&B periodicos |
|---|---:|---:|---:|
| Teto 1 s | 17.8863 | 41.5328 | 96.92% |
| Teto 2 s | 26.3840 | 59.2320 | 97.86% |
| 1000 propostas | 72.6576 | 45.1953 | 99.79% |

Os gaps sao medias aritmeticas por execucao de 100*(makespan/UB2025-1).
As amostras de 1 s e 2 s contem conjuntos/sementes diferentes e nao devem ser
comparadas entre si como uma curva de desempenho no tempo.

### Confirmacao com o teto de producao de 30 s

Uma semente (11) por instancia; evidencia descritiva, sem inferencia estatistica.

| Instancia | Makespan base | Makespan BB5 | Tempo base (s) | Tempo BB5 (s) | Propostas base | Propostas BB5 |
|---|---:|---:|---:|---:|---:|---:|
| 24-rc-c | 72 | 72 | 0.297943 | 30.000025 | 338139 | 51948 |
| 105-rc-e | 116 | 196 | 2.011917 | 30.000069 | 480883 | 7121 |
| 144-rc-c | 144 | 196 | 5.036275 | 30.000202 | 476383 | 6982 |
| 200-r-e | 289 | 376 | 4.277611 | 30.000442 | 203396 | 7566 |

Os B&B periodicos ocuparam 99.35% do tempo total da variante nesses quatro
casos. Os pequenos excessos ao teto refletem verificacao cooperativa de prazo.
Estes resultados nao sao novos recordes nem comparacoes diretas com a literatura.

## Interpretacao e decisao

Os refinamentos podem melhorar a qualidade para uma quantidade fixa de propostas,
mas o gatilho de cinco propostas reduz drasticamente a exploracao por segundo.
O SA passa muito tempo refinando conjuntos correntes, inclusive recursos que
nao sao o gargalo e configuracoes que voltara a alterar, e avanca menos no
resfriamento. A frequencia literal solicitada nao se mostrou vantajosa sob
os orcamentos de tempo examinados. A variante permanece opcional/desativada
no perfil padrao.

Alternativas para um proximo experimento: contar niveis de temperatura em vez
de propostas; reservar uma fracao do tempo para B&B; refinar apenas a elite ou
o recurso gargalo; e evitar novas chamadas sobre configuracoes ja refinadas.
Essas alternativas nao foram avaliadas neste experimento.
