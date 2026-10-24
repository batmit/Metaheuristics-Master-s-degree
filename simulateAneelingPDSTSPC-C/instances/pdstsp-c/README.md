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

O `main.c` executa automaticamente 41 instancias: as 30 pequenas (ate 30
clientes) e as 11 medias disponiveis (31 a 100 clientes, convencao deste lote).
As medias continuam em `large/`, pois a fonte original nao possui a categoria
media. Instancias acima de 100 clientes ficam fora deste teste.

Cada instancia recebe uma calibracao e uma execucao de SA com os parametros
padrao. A semente 123 inicializa uma unica sequencia aleatoria para o lote.
A tabela mostra makespan inicial, SA, melhor literatura em minutos, gap,
melhoria inicial, temperatura e CPU. Falhas nao interrompem as demais
instancias e produzem codigo de saida de falha ao final.

A coluna de literatura usa o melhor limite superior viavel (UB) das Tabelas
1 e 2 de Montemanni et al. (2024), nunca o limite inferior (LB).
Os valores por instancia tambem estao em `literature_small_medium.csv`;
a lista usada na execucao esta em `casos` no `main.c`.
Sao referencias da publicacao consultada, nao uma consulta online a cada teste.
Alguns valores ainda nao possuem prova de otimalidade.

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
