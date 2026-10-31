# Implementacao das propostas 1, 2, 6 e 7

## O que foi integrado

- Troca bidirecional: local inteiro ou grupo proximo do caminhao para drones, acompanhado de retornos para liberar carga. Metade das ancoras prioriza economia de percurso, a outra metade e uniforme. Retornos alternam alivio de carga/custo de insercao e sorteio uniforme. Avalia somente a cobertura completa.
- Reconstrucao parcial: fracao aleatoria de 5% a 15%, selecao aleatoria, por proximidade ou contribuicao ao gargalo; ordem de reinsercao embaralhada. Cada reinsercao compara a melhor posicao no caminhao com todas as quantidades viaveis em ate tres prioridades dos drones. Dois passes de 2-opt finalizam a rota.
- Calibracao mista: caminhada em copia, operadores configurados e alvo 0,8 da aceitacao media dos deltas positivos. O SA com tempo limitado continua apos o resfriamento, com retomadas e reaquecimento apos estagnacao; sem limite usa resfriamento e numero finito de retomadas.
- Elite diversa: melhor makespan absoluto preservado, ate tres outras atribuicoes distintas dentro de 10% do melhor; distancia de Hamming dos clientes por recurso. Empates da mesma atribuicao usam a soma dos tempos. Retomada sorteia preferencialmente uma atribuicao diferente e a reconstrui quando essa opcao esta ativa.

Nao foi adicionado CP-SAT, nem alterada a regra de ordem fixa do B&B. Os macromovimentos recebem 1% de probabilidade cada; os movimentos locais preservam pelo menos 75% pela validacao dos parametros. Probabilidades, fracoes, folga, estagnacao, reaquecimento e limite sao configuraveis. As outras propostas nao foram integradas.

## Uso

Compilar: `gcc -std=c11 -O2 -Wall -Wextra -pedantic main.c SA.c cv.c lerInstancias.c -lm -o sa_exploracao.exe`.

- Padrao da main: quatro casos, 100 execucoes por caso, semente do relogio, 30 segundos por execucao (calibracao + SA), propostas 1/2/6/7 ativas.
- Exemplo curto: `./sa_exploracao.exe --instancia 67-c-c --execucoes 10 --segundos 30`.
- Comparacao com sementes registradas: acrescentar `--semente 123`; repetir com `--controle` para desligar as quatro novidades, mantendo grupos anteriores. Por tempo, a quantidade de avaliacoes pode variar entre repeticoes; para reproducao deterministica do ciclo usar `--segundos 0`.
- `--segundos 0` encerra pelo resfriamento; `--controle` sem limite explicito conserva o ciclo anterior.
- A inicializacao fica fora do limite. Um movimento/reparo em andamento e concluido antes da verificacao seguinte, portanto o limite pode ser excedido pelo custo de uma operacao e de fechamento.
- Resumos da main: `resultados/resumo_quatro_abertas_exploracao.csv` e `resultados/resumo_quatro_abertas_controle.csv`, separados dos resultados historicos.

## Verificacao

Suíte de 15 testes C, incluindo `test_exploracao`: checagem direta de cobertura, duracoes e capacidade; remocao de 100% dos clientes; caminhada de macromovimentos; independencia de buffers; manutencao do melhor e diversidade; calibracao reproduzivel sem modificar a referencia; retomadas; limite total. Compilacao C11/O2 com avisos ativos.

## Comparacao exploratoria por tempo igual

Dez sementes novas por instancia e por versao, 10 segundos por execucao incluindo calibracao: 80 execucoes, quatro processos concorrentes, mesma compilacao. A ordem das versoes alterna entre sementes/casos. Todos os 80 certificados foram validados independentemente com fracoes exatas. A matriz de duracoes, cobertura, percurso e frota sao conferidos sem chamar o avaliador C.

O controle usa as mesmas funcoes locais e calibracao legada. Sob limite de tempo, ele tambem continua na temperatura final ate esgotar o prazo; isso permite comparar o mesmo esforco em tempo, sem obrigar uma versao a encerrar antes. O lote nao substitui uma confirmacao de 100 sementes nem uma avaliacao do padrao de 30 s. Tempos medidos sob concorrencia nao representam desempenho isolado.

| Instancia | Melhor controle | Melhor novo | Media controle | Media nova | Vitorias/empates/derrotas | UB |
|---|---:|---:|---:|---:|---|---:|
| 67-c-c | 76 | 76 | 76.00 | 76.00 | 0/10/0 | 74 |
| 165-r-c | 176 | 164 | 179.20 | 166.10 | 10/0/0 | 160 |
| 167-r-e | 192 | 188 | 198.80 | 189.20 | 9/1/0 | 180 |
| 181-r-e | 232 | 220 | 240.40 | 226.50 | 9/1/0 | 216 |

Uma alteracao implementada nao equivale a melhoria comprovada. Este lote avalia o conjunto das quatro propostas; nao isola os efeitos individuais nem demonstra que a melhor combinacao foi encontrada. As metricas adicionais estao em comparacao.json, incluindo avaliacoes, temperaturas, diversidade e melhorias originadas nos macromovimentos.

Fontes anteriores em ../fontes_antes/, fontes usadas no lote em fontes_depois/, sementes e hashes no manifesto.json. Para reproduzir outro lote, preservar os arquivos existentes ou usar uma pasta nova: rodar.py gera sementes do relogio e os resultados sao identificados por semente.

## Ajuste do resfriamento durante a verificacao

A primeira comparacao de 80 execucoes, preservada na pasta superior, mostrou que os macromovimentos reduziam o numero de propostas por segundo e, nas maiores, podiam consumir o prazo antes de concluir os niveis de resfriamento. Essa versao teve pioras grandes nas instancias 167 e 181. Por isso o padrao final usa `resfriamentoPorTempo=true`: a temperatura percorre geometricamente de T0 a tempFinal em funcao da fracao de tempo consumida. Um reaquecimento tambem decai ate tempFinal dentro do prazo restante. Sem limite de tempo, conserva o resfriamento por alpha e limita o numero de retomadas.

Esta segunda comparacao usa sementes novas, com um novo controle pareado; nao reutiliza somente os melhores resultados da primeira. Dez sementes ainda sao uma amostra exploratoria, nao uma confirmacao estatistica do ganho ou do efeito separado de cada proposta. A temperatura final por execucao esta nos certificados.

Os contadores de avaliacoes e macromovimentos descrevem o laco principal; nao incluem as tentativas da calibracao, os nos do B&B nem as reconstrucoes das retomadas. O tempo inclui todas essas etapas.

A suite final de 15 testes passou. O modo sem novidades e sem limite de tempo reproduziu exatamente, nas quatro instancias com semente 731, as temperaturas, makespans, rotas, quantidades e cronogramas obtidos pelos fontes anteriores congelados. A verificacao esta em ../verificar_compatibilidade.py e ../compatibilidade.json. A main compilada tambem passou no teste de uma execucao curta dos quatro casos e rejeitou argumentos invalidos antes de abrir o resumo.
