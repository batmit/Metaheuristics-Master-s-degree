# Instancias PDSTSP-C com tempos de drones pre-calculados

Conjunto de 60 instancias publicadas pelo ORLab: 30 pequenas (15 a 30 clientes)
e 30 grandes (50 a 200 clientes). As frotas dos arquivos variam de 3 a 12 drones.
Inclui a instancia `18-r-e` ja presente na raiz do projeto e outras 59 instancias.

## Organizacao

- `small/`: 30 pares de arquivos para instancias pequenas.
- `large/`: 30 pares de arquivos para instancias grandes.
- `catalog.csv`: nomes, numeros de clientes e drones, caminhos dos pares e hashes SHA-256.
- `source.json`: origem, data da obtencao, caminhos internos dos arquivos e hashes dos pacotes.
- `other-params.txt`: parametros complementares originais fornecidos pelo ORLab.

Cada instancia `N-tipo-tipo.txt` tem sua matriz `tau_kj_N-tipo-tipo.txt`
na mesma pasta. O numero no nome conta clientes, sem incluir o deposito.

Os arquivos foram extraidos sem modificar seus bytes. Os tempos dos drones
vieram prontos do pacote: nenhum tempo foi gerado ou recalculado neste projeto.
A matriz `tau_kj_18-r-e.txt` coincide com a matriz ja usada na raiz do projeto.

## Fontes

Obtencao em 23/10/2026:

- [Data and results of PDSTSP-c and PDSOP-c, ORLab (2025)](https://orlab.com.vn/download/data-and-results-of-pdstsp-c-and-pdsop-c/).
  Dentro do pacote, `PDSOP-c/data.zip` contem `data/instances/tsp/` (instancias PDSTSP-C)
  e `data/instances/tau_jk/` (matrizes pre-calculadas). Os conjuntos `profit/`,
  relativos ao problema de orienteering, nao foram incluidos.
- [Instancias originais do PDSTSP com drones coletivos, ORLab](https://orlab.com.vn/download/instances-for-the-parallel-drone-scheduling-traveling-salesman-problem-with-collective-drones/).
  As 60 instancias foram comparadas com esse pacote original: todos os campos
  e valores coincidem, desconsiderando apenas espacos e quebras de linha.
- [Montemanni, Dell'Amico e Corsini: Parallel drone scheduling vehicle routing problems with collective drones (2024)](https://iris.unimore.it/bitstream/11380/1328966/2/1-s2.0-S0305054823003787-main.pdf).
  A secao 4.1 descreve o benchmark e seus tempos pre-calculados.

Os pacotes ZIP e os resultados de outros problemas nao foram mantidos no
projeto; os hashes dos pacotes consultados estao em `source.json`.

## Uso no codigo existente

Exemplo de carregamento, executando a partir da raiz do projeto:

```c
Instancia inst = {0};
if (!lerInstancia("instances/pdstsp-c/small/15-r-e.txt", &inst))
    return 1;
alocarInstancias(&inst);
construirMatrizCaminhao(&inst);
if (!lerTemposDrone("instances/pdstsp-c/small/tau_kj_15-r-e.txt", &inst)) {
    liberarInstancia(&inst);
    return 1;
}
```

O `main.c` executa apenas as quatro instancias ainda abertas no pacote oficial
ORLab de 2025: `67-c-c` (LB=73, UB=74), `165-r-c` (156, 160),
`167-r-e` (179, 180) e `181-r-e` (215, 216). O criterio e status FEASIBLE e
LB < UB nos logs PDSTSP-c; a coluna de literatura usa o UB.
O cruzamento das 60 instancias esta em
`resultados/revisao_literatura_2025/relatorio_revisao_2024_2025.md`.
`literature_unproven.csv` preserva o conjunto historico de 23 casos de 2024.

Os arquivos `144-rc-c` e `173-rc-c` correspondem, respectivamente, aos rotulos
`144-rc-r` e `173-rc-r` da Tabela 2, que diferem dos nomes do conjunto ORLab.
Essa correspondencia de nomenclatura esta registrada no CSV historico.
Os arquivos e matrizes originais nao foram renomeados.

Cada instancia recebe 100 execucoes de SA com os parametros padrao (400 no
total). Cada repeticao reconstroi a rota inicial por vizinho mais proximo e
2-opt e recalibra a temperatura. A semente base usa `time(NULL)`; a semente
de cada repeticao e base + indice da instancia * 100 + indice da repeticao,
com indices iniciados em zero, e aparece na saida para reproducao.
A tabela mostra makespan inicial, SA, melhor literatura em minutos, gap,
melhoria inicial, temperatura e CPU. Falhas nao interrompem as demais
instancias e produzem codigo de saida de falha ao final.

O resumo `resultados/resumo_quatro_abertas_2025_100_execucoes.csv` e atualizado apos
cada instancia: quantidade concluida, melhor, media, referencia, gap do melhor,
execucao/semente do melhor e contagens de resultados abaixo/iguais a referencia.
O arquivo e recriado a cada lote. Para guardar tambem a saida detalhada, redirecione
a execucao para um novo arquivo. Os resultados do lote historico de 23 casos,
`resultados/resumo_sem_otimo_100_execucoes.csv` e
`resultados/benchmark_sem_otimo_100_execucoes.txt`, permanecem separados.

A coluna de literatura usa os limites superiores viaveis (UB) do pacote
ORLab de 2025, nunca os limites inferiores (LB). Os valores atualizados e
os logs/celulas da fonte estao em `resultados/revisao_literatura_2025/ainda_abertas.csv`;
a lista usada na execucao esta em `casos` no `main.c`.
Sao referencias da fonte consultada, sem consulta online durante cada teste.

Execute a partir da raiz do projeto:

```powershell
gcc -O2 -std=c11 -Wall -Wextra -pedantic main.c cv.c lerInstancias.c SA.c -lm -o benchmark.exe
./benchmark.exe
```

Nas matrizes, cada linha corresponde a um cliente e cada coluna `k-unit`
a quantidade `k` de drones cooperando na missao. O valor `-1` indica uma
combinacao inviavel; `lerTemposDrone` o converte para `INFINITY`.

## Validacao realizada

Os 60 pares passaram pela leitura com `lerInstancia` e `lerTemposDrone` do
projeto. Para cada par, foram construidas as matrizes do caminhao, avaliada a
solucao inicial e escalonada/avaliada uma solucao mista com todos os clientes
elegiveis nos drones e os demais no caminhao. Tambem foram conferidos os IDs,
as dimensoes das matrizes e os valores das celulas (nao negativos ou `-1`).

## Pesos adaptativos do SA

Os parametros padrao agora ativam `pesosAdaptativos`, com `taxaAdaptacao=0.2`
e `pesoMinimo=0.1`. A cada chamada do SA, os sete tipos de movimento iniciam
com peso 1. O sorteio e proporcional aos pesos apenas entre tipos disponiveis.
Ao final de cada temperatura, o peso combina 80% do peso anterior com 20%
da pontuacao media por tentativa, respeitando o piso. Tipos nao tentados
mantem o peso; contadores sao zerados a cada bloco.

Pontuacoes exclusivas: novo melhor global=5, melhora da atual=3, piora
aceita=1, empate ou rejeicao=0. A regra de aceitacao por temperatura e o
makespan como objetivo continuam os mesmos. A calibracao usa sorteio uniforme.
Para comparar com o comportamento anterior, defina
`parametros.pesosAdaptativos = false` antes de executar o SA.

O arquivo `resultados/benchmark_pequenas_medias.txt` foi produzido antes
da introducao desses pesos; seus resultados correspondem ao sorteio uniforme.

## Quatro melhores solucoes do SA

Durante cada execucao, `simulatedAnnealing` mantem `melhores[4]` na memoria,
com vetores independentes para rota, missoes dos drones e caches de prefixos.
A solucao inicial ocupa a primeira vaga. Toda candidata viavel avaliada pode
entrar no conjunto, mesmo se a transicao for rejeitada pela regra do SA.

As vagas sao preenchidas com solucoes distintas. Depois de completar quatro,
uma candidata entra somente se seu makespan for menor que o da pior armazenada.
O conjunto fica ordenado da melhor para a pior; empates distintos preenchem
vagas, mas nao substituem a pior quando o conjunto esta cheio. Repeticoes da
mesma rota e sequencia de missoes/quantidades sao ignoradas.

Os quatro slots sao internos a cada chamada, sem compartilhamento entre
instancias. Ao terminar, a funcao retorna a melhor em `solucao` e libera os
buffers internos. `atualizarMelhoresSolucoesSA` implementa essa atualizacao.

## Branch and Bound com ordem fixa

`escalonarMissoesDroneBB` escolhe somente a quantidade viavel de drones de
cada missao e recalcula os horarios. O nivel da arvore corresponde ao indice
no vetor recebido: nao sao exploradas permutacoes dos clientes. O limite
inferior de carga do artigo continua ativo.

Para cada combinacao de quantidades, as missoes sao processadas na ordem
recebida e inseridas no primeiro intervalo viavel. Essa ordem e de prioridade,
nao uma restricao de precedencia: uma missao posterior pode aproveitar uma
janela anterior. O resultado e o melhor entre as combinacoes sob essa regra,
sem pretensao de provar o otimo irrestrito de todas as ordens.

## Refinamento periodico pelo Branch and Bound

O SA agora chama o BB apos cinco niveis consecutivos de temperatura sem
melhora global (`niveisSemMelhoraBB=5`). A cada disparo, sorteia uniformemente
uma das elites preenchidas que satisfaz `tempoDrones > tempoCaminhao` e
`branchBoundAplicado == false`. Sem candidatas elegiveis, nenhuma busca e feita.
O contador reinicia apos o disparo, evitando chamadas consecutivas em todos os
niveis. Uma melhora global pelo SA tambem reinicia a contagem.

O orcamento padrao e `maxNosBB=10000` nos por chamada. A versao limitada retorna
o melhor cronograma encontrado e pode encerrar sem prova de otimalidade;
a versao original `escalonarMissoesDroneBB` continua sem limite. A tentativa
fica marcada mesmo quando nao melhora ou esgota o orcamento. Defina
`parametros.bbPeriodico=false` para desativar o refinamento.

O BB mantem a ordem das missoes e otimiza suas quantidades. O incumbente e
preservado se o resultado nao melhorar. O arquivo e reordenado e duplicatas
sao removidas depois do refinamento. Quando o melhor global melhora, ele
passa a ser a solucao atual do SA.

A marca acompanha copias e reordenacoes da elite. Trocas apenas no caminhao
preservam a marca, pois nao alteram o subproblema dos drones. Novas mudancas
nas missoes dos drones ou suas quantidades permitem um novo refinamento.
O controle usa tambem um historico por sequencia de clientes dos drones,
conservado durante a execucao mesmo quando a solucao sai das quatro elites.
Quantidades e rota do caminhao nao fazem parte da chave; a instancia deve
permanecer imutavel. Cronogramas em cache podem ser reutilizados sem nova busca.
Tentativas sem prova podem voltar com maior orcamento; se a meta anterior
foi atingida, uma nova rota mais curta pode justificar completar a busca.

SA e BB compartilham a regra de insercao no primeiro intervalo livre com
capacidade suficiente. Apos o BB, o escalonador reproduz esses mesmos horarios,
reconstroi os caches de prefixos e o calculo completo valida o makespan.
Vizinhos que mudam apenas o caminhao preservam todo o cronograma; mudancas
nos drones reconstroem somente o sufixo a partir da primeira missao alterada.
A calibracao da temperatura continua sem BB.

Os resultados salvos em `resultados/benchmark_pequenas_medias.txt` antecedem
tambem a integracao periodica do BB.

## Escalonamento compartilhado por intervalos livres

`primeiroInicioPerfil` e `ocuparPerfilDrone`, internos a `cv.c`, sao usados
tanto pelo SA quanto pelo BB. O historico de ocupacao e um perfil de eventos
ordenados, com partidas e retornos agregados em cada instante. Uma varredura
linear encontra a primeira janela livre, inclusive em intervalos anteriores.
`prefixosDrone[i]` armazena apenas o maior termino das primeiras `i` missoes;
nao armazena mais a ultima disponibilidade de cada drone. O vetor tem `n`
posicoes, incluindo o prefixo vazio, reduzindo memoria e custo das copias.

No exemplo A=(1 drone,10 min), B=(3 drones,5 min), C=(2 drones,8 min), com
frota 3, ambos produzem A=[0,10), B=[10,15), C=[0,8), makespan 15.
Reescalonar a mesma ordem e quantidades preserva esse resultado. A ordem de
processamento permanece A,B,C; a insercao em janelas nao impoe precedencia.

O teste `test_escalonamento_unificado.c` verifica esse exemplo, a reproducao
dos horarios do BB e 1000 vizinhos comparados a reconstrucoes completas.
Resultados de benchmarks anteriores precisam ser regenerados para avaliar
a qualidade e o tempo de execucao desta nova regra.

## Propostas 3, 4, 5 e 6 aplicadas

- B&B: carga minima acumulada em sufixos, com consulta O(1) por no; perfis
  de eventos reutilizados por profundidade, sem alocacao em cada ramo;
  quantidades exploradas por menor termino, desempate por consumo; incumbente
  inicial e o melhor entre as quantidades recebidas e a construcao gulosa.
- Meta: `escalonarMissoesDroneBBMeta` pode parar ao obter tempo dos drones
  menor ou igual ao tempo do caminhao. `ResultadoBB` distingue `BB_META_ATINGIDA`,
  `BB_OTIMO`, `BB_LIMITE_NOS` e `BB_ERRO`, incluindo nos visitados e limite de carga.
  `BB_OTIMO` refere-se somente a busca de quantidades sob ordem/regra fixas.
- Historico: `HistoricoBBSA`, inicializado com `{0}`, armazena cronogramas,
  orcamento e status por sequencia de clientes; `liberarHistoricoBBSA` libera
  toda a memoria ao final. A interface `refinarEliteComHistoricoSA` mantem
  o sorteio uniforme entre elites elegiveis e reaproveita resultados anteriores.
- Caminhao: novos movimentos `REINSERIR_CAMINHAO` e `DOIS_OPT_CAMINHAO`
  participam dos pesos adaptativos. Ambos estao disponiveis com pelo menos
  tres clientes; deltas incluem os arcos necessarios em matrizes assimetricas.
  A solucao inicial continua com todos no caminhao, mas utiliza vizinho mais
  proximo seguido de 2-opt. A rota por indice e mantida como alternativa para
  evitar iniciar pior quando o vizinho mais proximo for desfavoravel.

`resultados/benchmark_propostas_3_4_5_6.txt` registra a execucao deste conjunto
com a nova implementacao. Comparar uma unica semente com resultados antigos
nao isola o beneficio de cada componente nem garante melhoria em toda instancia.

`resultados/benchmark_sem_otimo_todos.txt` registra o lote das 23 instancias.
`resultados/benchmark_sem_otimo.txt` corresponde ao subconjunto anterior de
seis pequenas/medias e foi preservado.
