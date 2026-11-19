# 1. O problema em poucas palavras

PDSVRP-C | Guia da implementação | 6 de novembro de 2026

O objetivo é entregar todos os pedidos usando vários caminhões e uma frota compartilhada de drones. Alguns pedidos precisam de vários drones trabalhando juntos. Caminhões e drones partem do mesmo depósito e trabalham em paralelo.

**O que minimizamos:** o instante em que termina a última rota de caminhão ou a última missão de drones. Esse valor é o makespan, expresso em minutos.

Se os caminhões terminam em 40 e 52 minutos e os drones terminam em 45, o makespan é 52. Melhorar só os drones para 42 não altera o objetivo: o caminhão de 52 continua determinando o término.

## Restrições verificadas

- Cada cliente é atendido exatamente uma vez: por um caminhão ou por uma missão de drones.
- Há uma rota por caminhão disponível. Rotas vazias são permitidas.
- Toda rota começa e termina no depósito; essas duas visitas ficam implícitas no vetor da rota.
- Cada missão é uma viagem completa depósito-cliente-depósito, sem interrupção, com uma quantidade inteira k de drones.
- A combinação cliente/k precisa ser permitida pela tabela oficial; -1 significa inviável.
- Início e fim são finitos, o início não é negativo e a duração coincide com o tempo oficial.
- A soma dos drones ocupados ao mesmo tempo nunca supera a frota disponível.
- O objetivo informado precisa coincidir com o makespan recalculado.

**Modelo usado:** não há sincronização com caminhões para lançar drones, pois eles saem do depósito. Não foram acrescentadas janelas de tempo ou capacidades de carga dos caminhões que não constam deste modelo. A viabilidade física dos voos é recebida nas tabelas oficiais, sem refazer suas equações de energia.

# 2. Dados e solução inicial

## O que entra

Há 60 instâncias: 30 pequenas e 30 grandes. Cada uma fornece coordenadas, pesos e parâmetros físicos. As pequenas são combinadas com 2 e 3 caminhões; as grandes, com 2, 3, 4 e 5. Isso produz 180 configurações.

O catálogo de literatura informa limites inferiores (LB), melhores valores conhecidos (UB) e se há prova de ótimo. As 60 configurações pequenas já comprovadas são mostradas na tabela, mas não executadas pelo SA. Restam 120 configurações abertas.

O tempo do caminhão entre dois nós é a distância Manhattan: soma das diferenças absolutas das coordenadas. Divide-se pela velocidade em km/h e multiplica-se por 60 para obter minutos. Não se arredonda esse tempo.

As tabelas de drones fornecem uma duração por cliente e por quantidade k. O código transforma -1 em infinito, de modo que essa combinação não possa ser escolhida. Não acrescenta preparação nem arredondamento aos tempos já fornecidos.

## Como construímos uma solução viável

1. Começamos com caminhões vazios e nenhuma missão de drones.
2. Ordenamos os clientes do mais distante do depósito para o mais próximo.
3. Para cada cliente, testamos todas as posições em todos os caminhões.
4. Escolhemos a inserção que produz o menor makespan. No empate, preferimos o menor acréscimo de viagem.
5. Melhoramos cada rota com 2-opt: invertemos trechos enquanto houver redução de custo.
6. Recalculamos e validamos a solução inteira antes de publicar o primeiro certificado.

Todos os clientes começam nos caminhões. A busca poderá transferir alguns para os drones. Essa construção fornece uma solução de partida; não pretende demonstrar que a atribuição inicial é ótima.

**Representação:** cada caminhão possui seu próprio vetor ordenado de clientes. As missões guardam cliente, k, início e fim. Cópias de soluções usam vetores independentes para que uma mudança no vizinho não altere o estado atual.

# 3. Como são geradas alternativas

Um vizinho é uma pequena mudança na solução atual. O programa sorteia uniformemente um dos tipos de movimento que estão disponíveis naquela solução.

## Os cinco movimentos

- **Troca:** permuta dois clientes dentro de uma rota de caminhão.
- **Transferência:** move um cliente entre caminhões ou entre um caminhão e os drones. Uma transferência para drones exige pelo menos um k viável.
- **Quantidade de drones:** troca o k de uma missão por outro valor permitido para aquele cliente.
- **2-opt:** inverte um trecho de uma rota de caminhão, alterando suas conexões.
- **Reinserção:** retira um cliente e o recoloca em outra posição da mesma rota.

Dentro do tipo escolhido, o programa sorteia as posições, o cliente ou a quantidade necessários. Só quantidades com tempo finito são consideradas; não se presume que todo k maior que o mínimo seja viável.

## Como o custo é atualizado com pouco trabalho

Ao alterar uma rota, recalculamos apenas as arestas afetadas. Por exemplo, remover um cliente substitui as duas arestas que o ligavam aos vizinhos por uma ligação direta entre esses vizinhos.

Quando a mudança não afeta clientes e quantidades dos drones, o cronograma existente é preservado. Quando afeta as missões, o escalonador guloso produz um novo cronograma viável.

O objetivo do vizinho é o máximo entre a maior rota e o maior término de missão. A cobertura é preservada pelas operações de remoção e inserção. Antes de publicar uma melhora, o código C confere novamente a solução completa. O Python refaz essa conferência sem usar os custos incrementais do C.

**Conferência dos atalhos:** os testes percorrem 500 vizinhos em cada uma das 180 configurações. Cada custo incremental é comparado com uma avaliação integral, incluindo todas as rotas e as restrições de drones. Foram verificadas 90.000 transições, com ocorrência dos cinco movimentos.

# 4. Simulated Annealing passo a passo

O SA mantém duas soluções: a atual, usada para explorar, e a melhor encontrada. Aceitar uma solução pior pode ajudar a sair de uma região em que pequenas mudanças não melhoram o resultado. Essa aceitação não descarta a melhor solução já guardada.

## Calibração da temperatura inicial

Partimos de T=10. Geramos 500 vizinhos sobre uma referência fixa e contamos quantos seriam aceitos. Se a proporção for menor que 95%, multiplicamos T por 1,1 e repetimos. A referência original fica preservada durante essa calibração.

## Busca e resfriamento

```text
atual = solucao_inicial
melhor = atual
T = temperatura_calibrada
enquanto T > 0,01:
    repetir 500 vezes:
        vizinho = pequena_mudanca(atual)
        delta = custo(vizinho) - custo(atual)
        aceitar se delta < 0 ou sorteio < exp(-delta / T)
        se aceito: atual = vizinho
        se atual melhora melhor: validar e guardar
        se 5 tentativas sem melhora: tentar BB no melhor
    T = 0,99 * T
devolver melhor
```

Pioras pequenas e temperaturas altas aumentam a chance de aceitação. À medida que T cai, a busca se torna mais seletiva. Soluções de custo igual são aceitas, mas não substituem o melhor registro por empate.

O contador de estagnação mede tentativas sem melhora estrita do melhor global. Após cinco, ele é zerado e o BB pode refinar a melhor solução. Uma melhora global também zera o contador.

O algoritmo termina pelo resfriamento. Não há corte de tempo no padrão; o executor permite um limite opcional por configuração. A semente controla os sorteios. O SA é uma heurística: uma solução viável boa não constitui prova de ótimo global.

# 5. Como os drones são escalonados

Os drones são um recurso compartilhado. Uma missão que usa k drones ocupa essas k unidades durante todo o voo. Ao retornar, libera todas elas para outras missões.

## Escalonamento guloso nas avaliações

O escalonador mantém uma lista ordenada de eventos: um início aumenta o uso de drones e um término diminui. Para cada missão candidata, encontra a primeira janela contínua que comporta seu k durante toda a duração. Escolhe a missão com menor término; empata pela carga k vezes duração e pela ordem canônica dos clientes.

Exemplo com dois drones: uma missão com k=2 em [0,5) impede qualquer outra nesse intervalo. Outra pode começar exatamente em 5. O intervalo inclui o início e exclui o fim, evitando contar um drone retornado como ainda ocupado.

## Branch and Bound no refinamento

O BB mantém os clientes e os k recebidos. Ele pesquisa outras ordens de inserção e seus horários viáveis. Não decide quais clientes saem dos caminhões nem altera k; isso pertence aos movimentos do SA.

Para economizar busca, corta ramos que não podem melhorar o melhor cronograma. Usa o término parcial, a carga total de drone-tempo dividida pela frota, limites por demanda de recurso, tarefas simétricas e estados idênticos já visitados. Limites numéricos usam arredondamento conservador.

Sete prioridades determinísticas e trocas locais fornecem bons cronogramas iniciais para o BB. O cache é opcional e tem teto de memória: se encher, apenas deixa de guardar estados novos.

No SA, o BB recebe um teto de 100.000 nós por chamada. Pode parar antes quando os drones terminarem até o maior tempo dos caminhões, pois isso já resolve o gargalo daquele objetivo. Ao atingir o teto, devolve um cronograma viável sem afirmar ótimo. A API sem teto é mantida para os testes exatos.

O mesmo estado de missões já refinado não dispara outra busca enquanto não mudar. Se o refinamento melhora o makespan global, o SA continua a partir dessa solução.

# 6. Da execução à tabela ao vivo

## Divisão do trabalho

- **main.c:** chama o executor Python. No modo interno, carrega uma configuração e roda o algoritmo C.
- **lerInstancias.c e main.h:** leitura e representação dos dados, rotas e missões.
- **cv.c:** construção inicial, vizinhos, cópia de soluções e avaliação integral.
- **SA.c:** calibração, aceitação de vizinhos, melhor solução e resfriamento.
- **drone_bb.c:** construção de cronogramas e refinamento por BB.
- **executar_benchmark.py:** catálogo, processos, validação independente, persistência e tabela.

O C envia certificados internos contendo rotas, missões e makespan. O executor captura essas linhas; elas não aparecem na saída pública. Cada certificado é validado antes de qualquer gravação. Somente uma melhora estrita substitui o melhor da configuração.

O arquivo results/best.json reúne todos os melhores certificados. A gravação usa temporário, sincronização e substituição atômica. Um bloqueio do sistema impede dois executores de escrever ao mesmo tempo. Cancelamento e falhas encerram os workers restantes; checkpoints já gravados continuam disponíveis.

## O que aparece ao usuário

A tabela aparece desde o início, com instância e S, valor da literatura, melhor valor encontrado e gap. Cada melhora validada e salva acrescenta uma linha imediatamente. A conclusão de cada configuração também mostra sua linha, mesmo sem melhora. Com processos paralelos, as linhas seguem a ordem de chegada. Erros reais continuam no stderr.

```text
gap (%) = 100 * (encontrado - literatura) / literatura
literatura = UB do catalogo
-- = nenhum certificado do algoritmo para aquele caso
```

Por exemplo, literatura=100 e encontrado=95 produzem gap=-5%. O valor encontrado é o melhor acumulado, inclusive de execuções anteriores. Não se preenche essa coluna com o valor da literatura em casos ignorados.

O padrão executa uma repetição das configurações abertas. --list apenas confere e lista os salvos. --repetitions define repetições, --workers define paralelismo, --limit reduz o conjunto e --seconds aplica corte opcional. Pode haver intervalos sem linhas novas durante a calibração ou sem melhorias. Comandos de compilação estão no README.md.

# 7. O que foi conferido e simplificado

## Evidências da verificação

- Compilação C11 com -Wall, -Wextra, -Wpedantic e -Werror, sem avisos.
- 60 instâncias e 60 tabelas comparadas byte a byte com o ZIP oficial preservado; são 4.316 clientes.
- Certificados salvos, quando presentes, revalidados independentemente em Python.
- 90.000 vizinhos conferidos integralmente nas 180 configurações.
- BB comparado com enumeração independente de horários e atribuições em casos pequenos, incluindo orçamento de nós e parada por meta.
- Trajetórias e calibração do SA conferidas em 15-rc-c, 50-r-e e 200-r-e, com várias frotas.
- 19 testes Python: certificados inválidos, capacidade, cobertura, duração, objetivo, persistência, bloqueio, cancelamento, checkpoints ao vivo, tabela e seleção de binários nativos.
- Validação obrigatória mesmo com python -O; teste integrado de um segundo usa registro isolado e certificado novo validado.

Para reproduzir: execute scripts/verificar_projeto.py com Python 3 e GCC disponíveis. O resumo fica em output/verificacao.json. A verificação não inicia o lote completo nem altera os melhores resultados reais.

## Limpeza feita

Foram removidos diagnósticos antigos e suas cópias instrumentadas, executáveis obsoletos, a estrutura Missao sem uso, rotinas de impressão sem chamadas e campos de cache sem consumidores. Laços que só executavam uma vez no BB foram simplificados. Funções e etapas centrais receberam comentários explicativos.

A extração additional_2025 foi removida, economizando cerca de 104 MB e milhares de arquivos de outros modelos. O ZIP oficial continua preservado e agora é lido diretamente pelo validador. Entradas ativas, resultados, testes e transcrição da literatura continuam disponíveis.

## Alcance da confirmação

Os certificados conferidos são viáveis segundo o modelo e os dados locais, dentro das tolerâncias numéricas de comparação. A validação não refaz as equações físicas dos drones nem prova o ótimo global do problema.

O catálogo local transcreve COR 2024 e MIC 2024. A identidade das tabelas oficiais de 2025 com as usadas nesses artigos não foi demonstrada. Portanto, um gap negativo é uma comparação numérica com o catálogo, sem confirmação automática de recorde publicado. A origem e as ressalvas estão em instances/README.md e references/README.md.
