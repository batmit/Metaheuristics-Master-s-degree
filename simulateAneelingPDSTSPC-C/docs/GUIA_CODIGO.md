# Como funciona o algoritmo PDSTSP-C

Guia simplificado do código C | Versão enxuta | 06 de novembro de 2026

## 1. O problema em uma ideia

Um caminhão e uma frota de drones precisam entregar todas as encomendas. Cada cliente é atendido uma única vez: pelo caminhão ou por uma missão de drones. Algumas encomendas exigem vários drones trabalhando juntos. As missões dos drones partem do depósito e retornam a ele.

O objetivo é terminar todas as entregas o mais cedo possível. O caminhão e os drones trabalham em paralelo. Por isso, o valor de uma solução é o maior tempo entre os dois recursos:

```
makespan = max(tempo do caminhão, término da última missão de drones)
```

Por exemplo: se o caminhão termina em 18 minutos e os drones em 22, o makespan é 22 minutos. Reduzir a rota do caminhão para 16 minutos não altera esse resultado; é preciso reduzir também o recurso que termina por último.

## Onde está cada parte

| Arquivo | Responsabilidade |
|---|---|
| main.c | Percorrer as 60 instâncias, executar a busca, exibir a tabela e atualizar o CSV. |
| main.h | Definir clientes, instâncias, missões e soluções. |
| lerInstancias.c / .h | Ler entradas e tempos dos drones, validar dados e liberar a instância. |
| cv.c / .h | Construir soluções, mover clientes, calcular tempos e escalonar drones. |
| SA.c / .h | Calibrar a temperatura e executar o Simulated Annealing com diversificação. |

A leitura recomendada é main.h, main.c, lerInstancias.c, SA.c e, por último, os detalhes de cv.c. Os cabeçalhos .h apresentam os contratos; os arquivos .c implementam as operações.

<!-- pagina -->

## 2. Do executável ao resultado

<!-- fluxo -->

O programa percorre uma lista de 60 casos, do menor para o maior. Para cada caso, carrega os dados, cria uma solução inicial, calibra a temperatura e executa uma busca. Depois salva o melhor conhecido e acrescenta uma linha à tabela. Os vetores da instância são liberados antes de avançar.

Há apenas uma execução por instância. Não é necessário informar parâmetros no terminal. A semente é baseada no relógio, acrescida do índice do caso; execuções em momentos diferentes podem seguir caminhos diferentes.

## A tabela e o histórico

| Coluna | O que significa |
|---|---|
| instancia | Nome da instância, sem a extensão .txt. |
| melhor_algoritmo | Menor makespan guardado no histórico, incluindo execuções anteriores. |
| literatura_2024 | Melhor solução viável de 2024 usada como referência. |
| literatura_2025 | Melhor solução viável do pacote de 2025 usado no projeto. |
| gap_percentual | Distância percentual do melhor histórico à referência de 2025. |

```
gap (%) = 100 * (melhor_algoritmo - literatura_2025) / literatura_2025
```

Tempos de solução são expressos em minutos. Gap zero significa empate com a referência; negativo significa uma solução menor. O CSV único é resultados/relatorio.csv. Ele é atualizado após cada instância, sem substituir um valor anterior por outro pior. Assim, a linha mostrada pode vir de uma execução anterior, não necessariamente da busca que acabou de terminar.

No Windows, os caminhos são resolvidos a partir da pasta do executável. A pasta de resultados é criada automaticamente. Ao abrir com duplo clique, a janela espera Enter somente no final.

<!-- pagina -->

## 3. Como os dados são representados

| Estrutura | Conteúdo principal |
|---|---|
| Cliente | ID do arquivo, coordenadas, peso e elegibilidade para drones. |
| Instancia | Clientes, quantidade de drones, parâmetros da entrada e matrizes de tempos. |
| Missao | Índice de um cliente na sequência de visitas do caminhão. O campo chama-se n. |
| MissaoDrone | Cliente, número de drones, início e fim da missão. |
| Solucao | Rota, missões, tempos, makespan e caches da avaliação incremental. |
| ParametrosSA | Valores ajustáveis do único perfil de busca mantido. |
| EstatisticasSA | Contadores e tempos opcionais usados nos testes e análises. |

O índice 0 representa o depósito. As rotas do caminhão armazenam apenas os clientes; a saída e o retorno ao depósito ficam implícitos. O índice interno do cliente deve ser usado nas matrizes, e não confundido com um ID externo.

## As duas matrizes de tempo

O caminhão usa distância Manhattan: soma dos deslocamentos horizontal e vertical. O código transforma essa distância em minutos usando a velocidade informada na instância.

```
tempoCaminhao[i][j] = (abs(xi - xj) + abs(yi - yj)) / velocidade * 60
```

Para os drones, a matriz já vem pronta nos arquivos tau_kj. Cada linha representa um cliente e cada coluna k representa a quantidade de drones cooperando na entrega. O código não recalcula esses tempos a partir das coordenadas.

O valor -1 na entrada significa uma combinação inviável e é convertido para infinito. O primeiro k viável determina minDrones. Mesmo acima desse mínimo, cada combinação precisa ser conferida na matriz; o mínimo não torna automaticamente viáveis todas as quantidades maiores.

## Memória e validade

Cada solução possui vetores independentes. A busca troca a propriedade dos buffers para aceitar um vizinho sem copiar tudo novamente. cronogramaValido informa se os horários e prefixos podem ser reutilizados; avaliacaoValida informa se os tempos e o makespan estão atualizados. As funções de liberação também atendem falhas de leitura ou de alocação parcial.

<!-- pagina -->

## 4. Construir e modificar uma solução

A construção inicial coloca todos os clientes no caminhão. Primeiro avalia a ordem dos índices; depois tenta uma rota de vizinho mais próximo, escolhendo a próxima visita de menor tempo. Se essa rota for pior, recupera a anterior. Em seguida aplica 2-opt: inverte trechos enquanto encontra redução do custo. A busca poderá transferir clientes aos drones depois.

## Os sete movimentos locais

| Movimento | Alteração proposta |
|---|---|
| TROCAR_CAMINHAO | Trocar duas posições da rota. |
| TROCAR_DRONES | Trocar a prioridade de processamento de duas missões. |
| PARA_DRONES | Transferir um cliente elegível do caminhão para drones. |
| PARA_CAMINHAO | Retirar uma missão de drones e inserir o cliente na rota. |
| ALTERAR_QUANTIDADE_DRONES | Escolher outro k viável para uma missão. |
| REINSERIR_CAMINHAO | Retirar uma visita e inseri-la em outra posição. |
| DOIS_OPT_CAMINHAO | Inverter um trecho da rota. |

O sorteio considera apenas movimentos disponíveis. Nas transferências, também pode tentar um grupo de clientes da mesma coordenada ou de locais próximos. A solução de origem permanece intacta; a candidata utiliza seus próprios vetores.

## Por que a avaliação é incremental

Uma troca no caminhão altera poucas arestas: o código calcula a diferença de custo em vez de somar toda a rota. Quando os drones não mudam, seus horários são copiados. Quando uma missão muda, o escalonamento refaz o sufixo a partir do primeiro ponto afetado.

Os prefixos guardam o maior término das missões já processadas. Eles ajudam a reaproveitar resultados anteriores. A avaliação completa continua existindo para construir a solução e conferir o resultado final.

Há ainda dois movimentos compostos: trocar clientes entre os recursos e reconstruir uma parte da solução. A reconstrução remove entre 5% e 15% dos clientes e compara reinserções no caminhão ou nos drones. Na busca principal, é tentado no máximo um movimento composto por nível após cinco níveis sem melhoria global.

<!-- pagina -->

## 5. A lógica do Simulated Annealing

O algoritmo mantém a solução atual e gera uma candidata. Se a candidata melhora ou empata, é aceita. Se piora, ainda pode ser aceita com probabilidade dependente da temperatura. Isso permite sair de uma região em que nenhuma mudança pequena melhora imediatamente a solução.

```
delta = makespan_candidata - makespan_atual
se delta <= 0: aceitar
se delta > 0: aceitar quando sorteio_uniforme < exp(-delta / temperatura)
```

Se delta é 5 e a temperatura é 10, a probabilidade é aproximadamente 0,607. Com temperatura 1, cai para 0,0067. A busca começa mais tolerante e fica mais seletiva conforme esfria. A melhor solução encontrada é preservada separadamente, mesmo quando a atual aceita uma piora.

## Como a temperatura inicial é escolhida

A calibração trabalha numa cópia, faz uma caminhada com movimentos locais e algumas propostas compostas, e coleta aumentos positivos do makespan. Uma busca binária encontra uma temperatura cuja aceitação média desses aumentos se aproxima de gamma = 0,8. Se não houver aumentos positivos na amostra, usa a temperatura inicial configurada. A solução fornecida pelo chamador não é alterada.

Essa mistura foi preservada na limpeza: mudar a calibração mudaria a escala da temperatura e a sequência de decisões, mesmo mantendo os mesmos operadores.

## Pesos dos operadores

Todos os sete operadores começam com peso 1. A pontuação distingue melhor global (5), melhoria da atual (3) e piora aceita (1); os casos são exclusivos. Empates e rejeições não recebem pontos. Ao final do nível, o peso combina o valor anterior com a pontuação média. O piso 0,1 evita eliminar um operador do sorteio.

```
novo_peso = max(piso, (1 - taxa) * peso_anterior + taxa * pontuacao_media)
temperatura_seguinte = temperatura * 0,99
```

Cada nível tenta 500 vizinhos locais no padrão. A avaliação, a aceitação e a atualização da elite são etapas distintas: aceitar uma transição não significa substituir o melhor histórico.

<!-- pagina -->

## 6. Como os drones são escalonados

Cada missão ocupa k drones simultaneamente durante toda a sua duração. O escalonador percorre as missões na ordem do vetor e coloca cada uma no primeiro intervalo com capacidade suficiente. Retornos e partidas no mesmo instante são permitidos: o intervalo de ocupação é [início, fim).

<!-- cronograma -->

No exemplo, a frota tem quatro drones. A ocupa dois entre 0 e 10; B precisa de três e só cabe a partir de 10; C precisa de um e ainda cabe entre 0 e 8. Mesmo sendo processada depois de B, C começa antes dela. Portanto, a ordem do vetor expressa prioridade de inserção, não uma obrigação de partidas em ordem crescente.

O perfil de eventos registra mudanças de ocupação nos inícios e términos. O escalonamento e o B&B usam essa mesma regra de inserção, evitando interpretações diferentes de viabilidade.

## O refinamento final de Branch and Bound

Após o SA, se os drones determinam o makespan e ainda existe tempo disponível, o B&B tenta outras quantidades de drones para as mesmas missões. Ele conserva o conjunto de clientes e a ordem de processamento. Um limite inferior baseado em carga de drone-tempo permite podar alternativas que não podem superar o resultado conhecido.

Há uma única chamada final, limitada a 10 mil nós. A meta é reduzir o término dos drones até o tempo do caminhão: abaixo disso, o makespan já não melhora sem alterar a rota. Se não encontrar melhoria, preserva a solução anterior.

Esse B&B não prova o ótimo global do PDSTSP-C. Mesmo quando esgota sua árvore, trata apenas das quantidades sob a ordem e a regra de inserção fixadas. A validação final confere cobertura exata, rota, durações e capacidade simultânea, além de recalcular o makespan.

<!-- pagina -->

## 7. Diversidade, retomadas e parada

A elite guarda o melhor resultado e até três outras atribuições de clientes aos recursos. O melhor fica protegido. As alternativas respeitam uma folga de 10% e ajudam a explorar caminhos diferentes. A distância conta clientes que mudam de recurso, sem tratar uma simples permutação de rota como nova atribuição.

A comparação usa bitsets: cada cliente corresponde a um bit. O XOR marca as diferenças, que são contadas. Para as instâncias do benchmark, quatro palavras de 64 bits cabem na pilha; não há malloc/free em cada comparação. Entradas maiores possuem um caminho com alocação proporcional.

Após 20 níveis sem melhoria, o algoritmo pode retomar de outra elite e reaquecer até uma fração de T0. São permitidas no máximo duas retomadas. Isso muda a solução atual, mas conserva o melhor global.

## Quando a execução termina

A busca encerra ao atingir a temperatura final, ao atingir o teto de tempo ou por estagnação. A parada por estagnação exige simultaneamente 400 níveis sem melhoria global e pelo menos 200 mil avaliações. O término pela temperatura pode acontecer antes desse mínimo. A falta de qualquer movimento disponível também encerra a busca.

| Parâmetro padrão | Efeito |
|---|---|
| alpha = 0,99; tempFinal = 0,01 | Resfriamento e temperatura de encerramento. |
| gamma = 0,8; saMax = 500 | Aceitação alvo da calibração e tentativas por nível. |
| limiteSegundos = 30 | Teto para calibração e busca, sem obrigação de consumi-lo. |
| niveisParada = 400; minimoAvaliacoes = 200000 | Condições conjuntas da parada por estagnação. |
| maxRetomadas = 2; reaquecimento = 0,15 | Limite e intensidade das retomadas. |
| folgaElite = 0,10 | Margem de qualidade para alternativas de diversidade. |
| refinoFinalBB = true; maxNosBB = 10000 | Refinamento final opcional e seu limite de nós. |

O teto é verificado entre operações. Leitura, construção inicial e uma operação em andamento podem acrescentar tempo ao lote. Os parâmetros ficam em parametrosSARapido(), em SA.c; o teto do executável fica no início de main.c.

<!-- pagina -->

## 8. Compilar, testar e manter

No PowerShell, compile na raiz do projeto, com GCC disponível no PATH:

```
gcc -O2 -std=c11 -Wall -Wextra -pedantic `
  main.c cv.c lerInstancias.c SA.c -lm -o benchmark.exe
```

Abra benchmark.exe com duplo clique ou use .\benchmark.exe no PowerShell. No Git Bash, use ./benchmark.exe. Mantenha o executável ao lado da pasta instances. A execução do algoritmo não depende de Python, CP-SAT ou bibliotecas externas de otimização.

## O que foi retirado

Foram removidos do núcleo os perfis antigos, a calibração legada, o resfriamento por tempo, a elite sem diversidade, o B&B periódico, o histórico desses refinamentos e os reparos dirigidos/CP-SAT. Também saíram os executáveis antigos, o C++ sem dependências e os dados duplicados da raiz. Os testes dos recursos retirados ficaram no arquivo histórico; os testes das operações mantidas continuam ativos.

Resultados, fontes e documentos anteriores foram preservados em historico/pesquisas_e_versoes_anteriores.zip, com manifesto de arquivos e verificação de integridade. A pasta ativa resultados contém somente relatorio.csv. As referências e a proveniência das entradas ficam em instances/pdstsp-c/.

## Como a limpeza foi conferida

Antes e depois da refatoração, a semente fixa 20261106 foi aplicada separadamente a cada uma das 60 instâncias. Foram comparados makespan, temperatura calibrada, sequência completa da rota, quantidades de drones e horários das missões. Todos coincidiram. O registro fica em docs/verificacao_limpeza.json. Essa checagem confirma preservação nessa amostra, não uma prova geral para todas as sementes.

Os testes C cobrem movimentos, avaliação incremental, pesos, escalonamento, B&B, calibração e parada. O teste Python do relatório cobre os 60 casos, persistência, pasta de trabalho, UTF-8 com BOM e finais de linha LF/CRLF. Eles não precisam alterar os resultados reais do usuário.

O restante deste guia é um catálogo das funções mantidas. Ele permite localizar cada detalhe da implementação sem precisar ler o código inteiro de uma vez.

<!-- pagina -->

## 9. Catálogo: entrada, saída e memória

As funções marcadas como internas são restritas ao arquivo C. As demais têm contratos nos cabeçalhos. Consulte as páginas anteriores para entender como elas se conectam.

| Função | Papel na implementação |
|---|---|
| carregarCaso (interna) | Carrega uma instancia e suas matrizes uma unica vez por lote. |
| resolverCaso (interna) | Constroi e resolve uma solucao, liberando seus vetores ao terminar. |
| lerRelatorio (interna) | Recupera os melhores, aceitando UTF-8 com BOM e finais de linha LF/CRLF. |
| escreverLinha (interna) | Grava uma linha no CSV; vazio indica ausencia de resultado. |
| salvarRelatorio (interna) | Consolida uma linha por instancia preservando os melhores anteriores. |
| prepararDiretorio (interna) | Localiza as entradas ao abrir por duplo clique e cria a pasta de saida. |
| mostrarResultado (interna) | Mostra somente os campos do relatorio em colunas alinhadas. |
| executarLote (interna) | Executa exatamente uma busca para cada uma das 60 instancias. |
| main | Ao abrir com duplo clique, conserva a tabela na janela ate pressionar Enter. |
| liberarMatriz (interna) | Libera uma matriz caso ela ja tenha sido criada em outra etapa do projeto. |
| liberarInstancia | Libera clientes e matrizes, inclusive apos leitura incompleta. |
| lerInstancia | Le e valida os parametros e os clientes do arquivo da instancia. |
| lerTemposDrone | Le a matriz oficial e converte combinacoes inviaveis para infinito. |

<!-- pagina -->

## 10. Catálogo: Simulated Annealing

As funções marcadas como internas são restritas ao arquivo C. As demais têm contratos nos cabeçalhos. Consulte as páginas anteriores para entender como elas se conectam.

| Função | Papel na implementação |
|---|---|
| relogioSA | Retorna o tempo de parede em segundos para limitar a busca. |
| proporCalibracaoSA (interna) | Sorteia propostas mistas apenas na calibracao, preservando a escala de T0. |
| parametrosSARapido | Conserva movimentos locais e diversidade com parada natural e macros raros. |
| validarParametrosSA | Verifica faixas e consistencia dos parametros antes da busca. |
| inicializarPesosMovimentosSA | Inicializa os pesos e contadores dos operadores. |
| atualizarPesosMovimentosSA | Atualiza os pesos segundo o desempenho dos operadores. |
| calculaTemperaturaInicial | Calibra a temperatura usando vizinhos de uma copia da solucao. |
| copiarSolucao (interna) | Os dois destinos sao previamente alocados para a mesma instancia. |
| ordenarEliteSA (interna) | Ordena a elite por makespan crescente. |
| distanciaAtribuicaoSA | Conta diferencas de atribuicao dos clientes aos recursos. |
| atualizarEliteDiversaSA | Preserva o melhor e atribuicoes distintas dentro da folga permitida. |
| refinarMelhorSA (interna) | Refina somente o melhor cronograma, sem historico nem escolha de outra elite. |
| existeMovimento (interna) | Verifica se a solucao admite algum operador de vizinhanca. |
| simulatedAnnealing | Executa a busca com aceitacao probabilistica e devolve o melhor viavel. |
| resolverComCalibracaoSA | Compartilha o prazo entre calibracao da temperatura e busca. |

<!-- pagina -->

## 11. Catálogo: cv.c (1/3)

As funções marcadas como internas são restritas ao arquivo C. As demais têm contratos nos cabeçalhos. Consulte as páginas anteriores para entender como elas se conectam.

| Função | Papel na implementação |
|---|---|
| alocarInstancias | Aloca as matrizes de tempos do caminhao e dos drones. |
| alocarSolucao | Aloca os vetores e inicializa os indicadores da solucao. |
| liberarSolucao | Libera os vetores e restaura uma solucao vazia. |
| construirMatrizCaminhao | Calcula os tempos de viagem pela distancia Manhattan em minutos. |
| alocarMatrizCaminhao (interna) | Aloca a matriz quadrada de tempos de viagem. |
| alocarTempoDrone (interna) | Aloca uma duracao por cliente e quantidade de drones. |
| deltaDoisOpt (interna) | Delta da reversao inclui arcos internos, inclusive em matrizes assimetricas. |
| inverterTrecho (interna) | Inverte um trecho da sequencia de clientes no proprio vetor. |
| construirSolucaoInicial | Constroi a rota por vizinho mais proximo e melhora com 2-opt. |
| clientePodeUsarDrone (interna) | Verifica se existe quantidade viavel de drones para o cliente. |
| quantidadeDroneAlternativa (interna) | Conta ou sorteia quantidades viaveis, excluindo a quantidade atual. |
| tempoTrecho (interna) | Consulta o tempo de viagem entre dois nos. |
| arestaRota (interna) | Aresta de indice p: deposito/cliente anterior -> cliente/deposito seguinte. |
| custoRotaMelhoria (interna) | Movimentos conjuntos: a ordem dos drones restantes e preservada. |
| moverGrupoMelhoria (interna) | Transfere um grupo entre caminhao e drones buscando melhoria. |

<!-- pagina -->

## 12. Catálogo: cv.c (2/3)

As funções marcadas como internas são restritas ao arquivo C. As demais têm contratos nos cabeçalhos. Consulte as páginas anteriores para entender como elas se conectam.

| Função | Papel na implementação |
|---|---|
| gerarVizinhoInterno (interna) | Aplica o operador sorteado e informa as partes alteradas. |
| inserirEventoDrone (interna) | Insere uma mudanca de ocupacao no perfil ordenado de drones. |
| ocuparPerfilDrone (interna) | Registra a ocupacao de drones durante o intervalo da missao. |
| primeiroInicioPerfil (interna) | Uma varredura do perfil encontra a primeira janela continua com capacidade. |
| escalonarDesde (interna) | Recalcula o cronograma a partir do primeiro prefixo alterado. |
| escalonarMissoesDrone | Escalona as missoes e atualiza os caches de prefixos. |
| duracaoConfiguracaoBB (interna) | Consulta a duracao de uma tarefa para a quantidade escolhida. |
| limiteCargaDroneBB (interna) | Desigualdade (16), com soma minima do sufixo pre-calculada: O(1) por no. |
| prepararOpcoesBB (interna) | Prepara as quantidades viaveis para o proximo nivel da busca. |
| explorarDroneBB (interna) | Explora configuracoes com podas e limite de nos. |
| construirIncumbenteBB (interna) | Reconstrucao de incumbentes com a regra compartilhada. |
| escalonarMissoesDroneBBMeta | Refina o cronograma com meta de tempo e orcamento de nos. |
| escalonarMissoesDroneBB | Refina o cronograma sem limite explicito de nos. |
| gerarVizinhoAvaliadoConfigurado | Gera e avalia uma vizinha com pesos e operadores configurados. |
| gerarVizinhoAvaliadoPonderado | Gera uma vizinha avaliada usando os pesos dos operadores locais. |

<!-- pagina -->

## 13. Catálogo: cv.c (3/3)

As funções marcadas como internas são restritas ao arquivo C. As demais têm contratos nos cabeçalhos. Consulte as páginas anteriores para entender como elas se conectam.

| Função | Papel na implementação |
|---|---|
| gerarVizinhoAvaliado | Gera e avalia uma vizinha com selecao uniforme dos operadores. |
| calcularMakespan | Valida a cobertura e calcula o maior tempo entre caminhao e drones. |
| copiarDecisoesCV (interna) | Reconstrucoes trabalham com cobertura parcial apenas nestas funcoes privadas. |
| avaliarParcialCV (interna) | Avalia uma solucao parcial durante a reconstrucao. |
| cargaMinimaClienteCV (interna) | Calcula a menor carga viavel de drones para um cliente. |
| melhorPosicaoCaminhaoCV (interna) | Encontra a insercao do cliente com menor acrescimo na rota. |
| inserirCaminhaoCV (interna) | Insere o cliente na posicao indicada da rota. |
| menorCargaKCV (interna) | Escolhe a quantidade de drones com menor carga para o cliente. |
| inserirReconstrucaoCV (interna) | Compara reinserções e escolhe o recurso para o cliente. |
| polirRotaReconstrucaoCV (interna) | Melhora a rota reconstruida por inversoes 2-opt. |
| prepararMacroCV (interna) | Prepara uma copia e os vetores auxiliares de um movimento composto. |
| trocarRecursosCV | Tenta trocar clientes entre os recursos preservando viabilidade. |
| reconstruirParcialCV | Remove uma fracao dos clientes e os reinsere nos recursos. |
