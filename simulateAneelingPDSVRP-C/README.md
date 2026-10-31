# Simulated Annealing para o PDSVRP-C

Implementacao em C com varios caminhoes, drones coletivos e tempos de voo oficiais pre-calculados.

## Compilar e executar

Windows / PowerShell:

```powershell
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 main.c lerInstancias.c cv.c drone_bb.c SA.c -o pdsvrp_sa.exe -lm
.\pdsvrp_sa.exe
```

Linux / WSL:

```sh
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 main.c lerInstancias.c cv.c drone_bb.c SA.c -o pdsvrp_sa -lm
./pdsvrp_sa
```

Execute na raiz do projeto. O lancador procura Python 3 (python3/python no Linux; python/py -3 no Windows). O executor reconhece o binario com ou sem .exe.

Sem argumentos, executa **todas as 120 configuracoes (instancia, numero de caminhoes) sem otimo comprovado**, na ordem de `instances/literature_pdsvrp_c.csv`. As 60 configuracoes com otimo comprovado continuam sendo ignoradas.

**Nao ha limite de tempo.** O SA encerra quando a temperatura chega ao valor final. Cada chamada periodica do BB tem o orcamento autorizado de **100.000 nos**, devolvendo o melhor cronograma viavel caso nao consiga provar o otimo.

```sh
python3 scripts/executar_benchmark.py
python3 scripts/executar_benchmark.py --resume
python3 scripts/executar_benchmark.py --list
python3 scripts/executar_benchmark.py --limit 10
```

No Windows, use python em vez de python3. `--seconds` permanece como opcao explicita para outros experimentos; o padrao e 0, sem corte. `--limit` conta configuracoes, nao arquivos distintos; 0 (padrao) seleciona todas as abertas.

A saida padrao fica em `results/bb5_all_unproven/`: CSV comparativo, logs e certificados JSON. Use `--output` para preservar experimentos distintos. O lote anterior de 10 casos esta em `results/bb5_first10_final/`.

```sh
python3 scripts/verificar_relatorio.py
```

O modo individual aceita arquivo, tabela de tempos, numero de caminhoes e semente opcional:

```sh
./pdsvrp_sa instances/benchmark/large/50-r-e.txt instances/drone_times/tau_kj_50-r-e.txt 2 42
```

Esse modo tambem consulta a referencia e nao executa configuracoes com otimo comprovado ou sem entrada no catalogo.

## Solucao inicial e movimentos

A construcao inicial processa clientes distantes primeiro, insere cada cliente na posicao/caminhao que minimiza o makespan resultante (desempate pelo menor acrescimo de distancia), e aplica 2-opt ate um minimo local em cada rota. Inicialmente todos os clientes estao nos caminhoes.

Durante o SA, sorteia uniformemente entre os tipos disponiveis:

1. Troca de duas posicoes em uma rota de caminhao.
2. Transferencia de um cliente entre recursos, com insercao aleatoria no caminhao de destino.
3. Alteracao aleatoria da quantidade de drones de uma missao.
4. 2-opt aleatorio em uma rota de caminhao.
5. Reinsercao aleatoria dentro de uma rota de caminhao.

Clientes e quantidades k das missoes de drones sao sorteados; somente valores com tempo finito sao admitidos. A ordem de missoes de drones nao e sorteada.

## Escalonamento e Branch and Bound periodico

Nas tentativas normais e na calibracao usa-se um cronograma guloso deterministico, com primeiro intervalo de capacidade viavel e prioridades por termino. Ele preserva k e nao faz ramificacao.

Depois de **5 tentativas consecutivas sem melhora estrita do melhor makespan global**, o BB refina a **melhor solucao encontrada**, nao um estado pior aceito na temperatura alta. O contador e zerado apos o disparo. Um mesmo conjunto de clientes/k ja refinado nao e processado novamente sem mudanca; uma melhora global reinicia a contagem.

O BB decide somente ordem e horarios; os k recebidos sao fixos. O SA registra chamadas que atingiram o limite de nos em `bbLimitados`, sem declara-las otimas. A API exata sem limite continua disponivel para testes. Usa o limite inferior da referencia:

```text
LB = soma(k * duracao) / frota
```

Com tempos inteiros, arredonda para cima de forma conservadora. Tambem usa makespan parcial, limites por demanda, eliminacao de tarefas simetricas e cache de estados identicos (clientes ja escalonados + perfil de ocupacao). O cache tem teto de memoria, mas nao limita a busca. Ordens gulosas deterministicas e trocas locais fornecem limites superiores iniciais.

A meta do BB e o maior tempo dos caminhoes: quando encontra cronograma de drones abaixo dessa meta, o makespan global ja esta determinado, entao pode parar sem provar o otimo isolado dos drones. `ResultadoBB` distingue `metaAtingida`, `limiteNosAtingido` e `otimoProvado`. Se o BB melhora o melhor makespan global, o SA retoma a partir dessa solucao refinada.

O SA mantem os dois lacos do cpp de referencia: saMax tentativas por temperatura, aceitacao por exp(-delta/T) e T *= alpha. Calibracao parte de T=10 e aquece por beta ate atingir gamma sobre a mesma referencia. Padroes: alpha=0.99, beta=1.1, gamma=0.95, saMax=500, Tf=0.01. As estatisticas registram tentativas, niveis, chamadas/nos de BB e melhorias obtidas por BB.

## Dados e avaliacao

`main.h` define Instancia, Solucao, RotaCaminhao e MissaoDrone. Cada caminhao tem vetor independente, tamanho, capacidade e tempo; deposito 0 implicito. Os drones compartilham um vetor de missoes. `n` inclui o deposito. Inicializar structs com {0}, usar copiarSolucao para copia profunda e liberar antes de realocar.

`lerInstancias.c` carrega parametros, clientes e tabelas. Tempos dos caminhoes usam distancia Manhattan, dividida pela velocidade em km/h e convertida para minutos, sem arredondamento. Tempos de drones -1 viram INFINITY. Ver `instances/README.md` e `references/README.md` para origem dos dados e limites de comparabilidade com os artigos.

O makespan e max(maior tempo dos caminhoes, maior termino dos drones). Os movimentos atualizam as arestas afetadas e preservam horarios de drones quando clientes/k nao mudam. `calcularMakespan` recalcula e valida a solucao integral. O executor valida independentemente cada certificado em Python antes de inclui-lo no relatorio.

## Verificacao

```sh
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 lerInstancias.c cv.c drone_bb.c SA.c scripts/verificar_bb5.c -o verificar_bb5 -lm
./verificar_bb5
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 lerInstancias.c cv.c drone_bb.c SA.c scripts/verificar_bb.c -o verificar_bb -lm
./verificar_bb
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 lerInstancias.c cv.c drone_bb.c SA.c scripts/verificar_sa.c -o verificar_sa -lm
./verificar_sa instances/benchmark/small/15-rc-c.txt instances/drone_times/tau_kj_15-rc-c.txt
```

Verificacoes incluem: disparo somente a partir da quinta tentativa sem melhora, nao repeticao de refinamento identico, preservacao de k, comparacao com enumeracao independente de horarios em conjuntos pequenos e consistencia dos deltas dos movimentos com recálculo integral. Testes em funcoes internas podem usar instancias pequenas, sem executar o lote de otimos comprovados.
