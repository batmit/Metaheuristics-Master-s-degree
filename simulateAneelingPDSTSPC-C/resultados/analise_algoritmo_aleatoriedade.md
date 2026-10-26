# Revisao do SA, diversidade e escalonamento

Atualizacao de 26/10/2026: a auditoria detalhada em `105-rc-e/` confirmou a
viabilidade de 116, mas encontrou no pacote oficial ORLab de 2025 uma solucao
de 112, com status OPTIMAL e Makespan Bound=112. A solucao publicada de 112
tambem foi validada independentemente. Portanto, 116 melhora a referencia de
2024 (120), mas fica 3.57% acima do otimo reportado em 2025. Esta instancia
nao deve ser chamada de "sem otimo conhecido" com base apenas na tabela antiga.

Data: 24/10/2026. Revisao de main.c, SA.c/SA.h, cv.c/cv.h,
main.h e lerInstancias.c. Nenhum parametro ou algoritmo de producao foi
alterado nesta revisao. O lote de 2300 execucoes segue separado.

## Conclusao e evidencias

O algoritmo preserva escolhas pseudoaleatorias e encontra solucoes completas
diferentes. Sua principal limitacao de diversidade nao e a ausencia de sorteios:
e a combinacao de inicio deterministico, arquivo pequeno que rejeita empates,
recompensa dos operadores baseada apenas no makespan e ausencia de reinicios
ou reaquecimento dentro da mesma execucao.

Experimento registrado em auditoria_aleatoriedade.txt, com os parametros padrao:

- 20-c-c: sementes 123 a 132 produziram dez solucoes completas distintas,
  todas com makespan 43. As dez rotas iniciais foram identicas.
- Repetir a semente 123 reproduziu exatamente a temperatura calibrada,
  rota, atribuicao dos clientes, quantidades e horarios finais.
- O compilador/runtime usado apresenta RAND_MAX=32767.
- Os 13 testes existentes passaram, incluindo deltas incrementais, operadores,
  pesos, arquivo de elite, B&B, historico e consistencia do escalonamento.

Este experimento confirma diversidade nesta amostra e reproducibilidade no
mesmo ambiente; nao constitui um teste estatistico completo do gerador.
Mesmo makespan nao implica mesma solucao, e semente diferente nao garante
um makespan diferente nem uma sequencia estatisticamente independente.

## Onde existe aleatoriedade

1. main.c: semente base time(NULL), acrescida de c*100+i. Cada repeticao
   inicializa srand uma vez; nao ha reinicializacao nos lacos de vizinhanca.
2. cv.c: escolha ponderada do tipo entre os disponiveis, posicoes de troca,
   clientes transferidos, posicao de insercao e quantidade viavel de drones.
3. SA.c: aceitacao de pioras por exp(-delta/T), com sorteio aleatorio.
4. SA.c: escolha aleatoria entre as elites elegiveis para o B&B.
5. A calibracao tambem consome numeros aleatorios e varia entre sementes.

O piso positivo dos pesos conserva a possibilidade de selecionar todo tipo
disponivel. Transferencias nos dois sentidos, trocas e alteracoes de quantidade
permitem navegar entre atribuicoes, ordens e quantidades nas instancias atuais.
Nao foi encontrado um bloqueio estrutural que force todas as sementes a mesma
solucao. Na pratica, temperatura baixa e orcamento finito dificultam transicoes.

## Prioridades propostas

### 1. Diversificar o arquivo das quatro solucoes

SA.c: atualizarMelhoresSolucoesSA rejeita candidata >= pior quando o arquivo
esta cheio. Portanto, quatro solucoes com o mesmo makespan podem permanecer
sem renovacao mesmo se forem muito parecidas. As distintas sao definidas por
igualdade exata dos vetores, nao por distancia estrutural. Rotas reversas podem
ser equivalentes nas matrizes simetricas atuais, mas contam como distintas.

Proposta: preservar sempre a melhor e usar diversidade no desempate das demais.
Medir diferencas na atribuicao caminhao/drone, nas arestas do caminhao, na ordem
de prioridade dos drones e nas quantidades. Aceitar uma candidata empatada
quando aumentar a diversidade minima do arquivo. A ordem dos drones deve
continuar sendo tratada como significativa. Permitir solucoes ligeiramente
piores em slots de diversidade seria um experimento separado, pois altera a
regra atual de guardar estritamente as quatro melhores.

### 2. Melhorar a recompensa dos movimentos em plato

SA.c aceita delta=0, mas concede zero ponto ao operador. Como o objetivo e
max(tempoCaminhao,tempoDrones), melhorar um recurso que nao domina pode nao
alterar o makespan. Exemplo: (100,120) -> (90,120) e aceito sem recompensa.
Essa folga pode ser util para futuras transferencias, mas nao e reconhecida.
Ao mesmo tempo, uma piora aceita recebe um ponto: pode favorecer operadores
que pioram frequentemente durante temperaturas altas.

Proposta: manter makespan como objetivo principal e atribuir uma recompensa
pequena a empates com ganho secundario ou novidade estrutural. Nao recompensar
indefinidamente ciclos entre estados ja vistos. Avaliar separadamente a retirada
ou reducao do premio por piora aceita.

Adicionar mistura explicita de exploracao:

P(tipo) = (1-epsilon)*peso(tipo)/somaPesos + epsilon/M,

onde M e o numero de tipos disponiveis. epsilon=0.20 e um ponto inicial de
experimento, nao um valor comprovadamente ideal. Com sete tipos, garante ao
menos 2.86% para cada um. O piso atual de peso 0.1 nao e um piso de probabilidade:
com seis pesos 5 e um 0.1, a menor probabilidade e aproximadamente 0.33%.

### 3. Criar solucoes iniciais variadas e viaveis

cv.c: construirSolucaoInicial sempre coloca todos no caminhao, aplica vizinho
mais proximo com desempate deterministico e 2-opt em ordem fixa.

Proposta: misturar essa construcao com escolha aleatoria entre alguns bons
candidatos e construcoes que ja atribuam parte dos clientes elegiveis aos
drones. Sortear quantidades viaveis e validar o cronograma. Manter a construcao
atual como uma das alternativas, comparando qualidade e diversidade iniciais.

### 4. Adicionar movimentos compostos

Os sete tipos atuais sao troca no caminhao, troca nos drones, transferencia
para drones, transferencia para caminhao, mudanca de quantidade, reinsercao
no caminhao e 2-opt no caminhao.

Propostas: troca simultanea de um cliente do caminhao por um dos drones;
reinsercao de uma missao em outra posicao no vetor de drones; alteracao conjunta
das quantidades de duas missoes; remocao e reinsercao de um pequeno conjunto
de clientes. Essas operacoes reduzem a necessidade de atravessar estados
intermediarios ruins. A ordem permanece fixa DENTRO do B&B; as novas ordens
seriam produzidas pela vizinhanca do SA.

### 5. Tratar estagnacao com perturbacao e reaquecimento

O SA atual apenas esfria. Depois de cinco niveis sem melhora, tenta refinar
uma elite, mas nao possui reinicio ou reaquecimento. Se o B&B melhora o melhor
global, a busca volta para ele; essa intensificacao e util, mas concentra a busca.

Proposta: apos estagnacao prolongada, sortear uma elite diversa, perturba-la e
reaquecer moderadamente. Preservar o melhor global e limitar numero/custo desses
reinicios. Medir estagnacao por avaliacoes ou tempo, alem dos niveis, para nao
confundir ausencia de melhora com poucas oportunidades em instancias maiores.

### 6. Modernizar o gerador e a gestao das sementes

rand() neste ambiente oferece 32768 valores por chamada. rand()%n introduz
pequeno vies quando n nao divide 32768. O sorteio real tem resolucao 1/32768:
probabilidades muito pequenas de aceitar piora ficam grosseiramente quantizadas.
Isso nao torna a busca deterministica, nem demonstra ser a principal causa
dos gaps observados. A parte fracionaria do sorteio e um problema diferente
do numero de sementes ou do periodo total do gerador.

Proposta: gerador conhecido com estado explicito por execucao, inteiros por
amostragem com rejeicao e uniforme real de maior resolucao. Registrar a semente
mestre e derivar sementes por instancia/repeticao, com mistura de bits. Permitir
semente manual. Manter estados separados para calibracao e busca facilita
comparacoes. Para paralelismo, cada trabalhador deve ter seu proprio estado.

time(NULL) tem resolucao de segundos. Dois lotes iniciados no mesmo segundo
repetem sementes. Faixas consecutivas de sementes tambem podem se sobrepor
entre lotes proximos; as sementes do lote atual sao distintas entre si.
Alterar a semente nao substitui diversificar operadores ou estados iniciais.

### 7. Ajustar calibracao e orcamento de busca

A calibracao gera 500 vizinhos da MESMA solucao inicial a cada temperatura,
inclui melhorias/empates na taxa de aceitacao e sobe por beta=1.1 ate 95%.
Na amostra de 20-c-c, T0 variou de aproximadamente 309 a 548. Ela nao representa
necessariamente as variacoes encontradas depois que os drones entram na busca.

Proposta: coletar deltas positivos de estados representativos, resolver uma
temperatura para uma taxa-alvo de aceitacao dessas pioras e reutilizar a amostra
durante a calibracao. Testar a taxa-alvo; nao assumir que 95% e sempre adequado.
Com T0 perto de 600, alpha=0.99 e Tf=0.01, o SA faz cerca de 1100 niveis,
ou 550 mil propostas, independentemente de haver progresso. Comparar as
variantes por mesmo tempo ou mesmo numero de avaliacoes; somente 100 repeticoes
nao equalizam o esforco quando o custo por repeticao muda.

### 8. Gerenciar o custo e o alcance do B&B

O B&B mantem a ordem recebida e varia quantidades. O cronograma e sempre
construido por insercao no primeiro intervalo viavel. A prova BB_OTIMO refere-se
somente a essas combinacoes sob a regra fixa. O SA usa 10000 nos por chamada
e pode parar ao atingir o tempo do caminhao. A escolha da elite e aleatoria;
a exploracao interna dos ramos e deterministica, ordenada por termino/consumo.
Essa parte deterministica nao remove a aleatoriedade global e nao precisa ser
substituida por escolhas aleatorias em toda poda.

O historico evita repetir o mesmo subproblema com o mesmo orcamento. Com o
orcamento fixo da configuracao atual, uma tentativa encerrada por limite nao
ganha mais esforco apenas por reaparecer. Isso economiza tempo, mas pode deixar
subproblemas promissores incompletos.

Proposta: registrar tempo, nos, motivo de parada e ganho; aumentar o orcamento
somente para elites promissoras/diversas e impor um orcamento global de B&B.
Usar o limite inferior para evitar refinar quantidades quando o gargalo
comprovadamente exige mudar a atribuicao ou a rota. A carga por sufixo ja foi
otimizada para O(1); nao e uma melhoria pendente. O limite por missao restante
ainda percorre missoes, quantidades e eventos em muitos nos: medir seu custo
e sua taxa de poda antes de ampliar o orcamento.

## Desempenho e robustez

- A avaliacao incremental ja evita recalcular a rota completa e preserva
  prefixos do cronograma; deve ser mantida e validada pelos testes diferenciais.
- Cada vizinho ainda verifica todos os clientes, copia os vetores e aloca
  visitado. Reescalonamentos alocam visitado, cronograma e eventos novamente.
  Reutilizar buffers e listas de quantidades viaveis reduz trabalho repetido.
- deltaDoisOpt soma os arcos internos para suportar matrizes assimetricas.
  Nas matrizes Manhattan simetricas atuais, o delta pode usar apenas as quatro
  arestas externas, depois de confirmar a simetria uma vez. A geracao inteira
  do vizinho continua tendo outros custos lineares.
- O historico do B&B faz busca linear por sequencia. Uma tabela hash com
  verificacao de igualdade pode ajudar se a instrumentacao mostrar muitas entradas.
- main.c recarrega arquivos e matrizes a cada repeticao. Carregar uma vez por
  instancia simplifica o lote; medir o ganho antes de considera-lo prioridade.
- calcularMakespan verifica duracoes, cobertura e capacidade, mas nao consulta
  diretamente elegivelDrone/minDrones. O carregador e os geradores atuais
  mantem essas restricoes; um validador independente deve checa-las explicitamente.
- O leitor de instancia poderia validar velocidade positiva/finita, frota,
  coordenadas, deposito e correspondencia entre IDs e indices. Esses pontos
  sao robustez de entrada, nao evidencia de erro nos benchmarks atuais.
- Salvar a melhor solucao completa, parametros, semente e versao do codigo
  automaticamente. Hoje main.c guarda valores e sementes, mas libera a solucao.
  Um resultado abaixo da referencia precisa de uma testemunha viavel verificavel.

## Resultado adicional reproduzido

O lote em andamento registrou 116 em 105-rc-e, contra 120 da referencia de 2024.
A semente 1792860329 reproduziu o valor. A solucao completa foi preservada em
auditoria_aleatoriedade.txt. Uma verificacao separada em PowerShell, lendo os
arquivos originais e sem chamar calcularMakespan, confirmou:

- os 105 clientes cobertos exatamente uma vez;
- rota fechada do caminhao com tempo 116;
- duracoes das missoes compativeis com as quantidades e a matriz tau;
- horarios nao negativos e uso simultaneo maximo de 12 drones (frota 12);
- tempo dos drones 115 e makespan max(116,115)=116.

A melhora e de 3.33% sobre a referencia cadastrada. Nao foi feita nesta revisao
uma busca de novos resultados publicados, nem uma prova de otimalidade.

## Plano de comparacao

Prioridade pratica: (1) instrumentar e salvar solucoes, (2) diversidade da elite
e recompensa de empates uteis, (3) inicio variado e movimentos compostos,
(4) reinicios/temperatura, (5) custo do B&B e gerador com estado explicito.
Melhorias de geracao aleatoria podem ser implementadas cedo como infraestrutura,
mas nao devem ser confundidas com garantia de melhora da qualidade.

Alterar um componente por vez, usando as mesmas sementes mestre e o mesmo
orcamento. Registrar melhor, media, mediana, desvio, taxa de atingir a referencia,
tempo ate o melhor, solucoes/atribuicoes distintas, frequencia e aceitacao por
operador e custo/ganho do B&B. Repetir os testes de viabilidade e de reproducao.
