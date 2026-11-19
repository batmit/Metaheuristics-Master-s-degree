# Instancias PDSTSP-C com tempos de drones pre-calculados

Conjunto de 60 instancias publicadas pelo ORLab: 30 pequenas (15 a 30 clientes)
e 30 grandes (50 a 200 clientes). As frotas dos arquivos variam de 3 a 12 drones.
Inclui todas as 60 instancias, sem copias duplicadas na raiz do projeto.

## Organizacao

- `small/`: 30 pares de arquivos para instancias pequenas.
- `large/`: 30 pares de arquivos para instancias grandes.
- `catalog.csv`: nomes, numeros de clientes e drones, caminhos dos pares e hashes SHA-256.
- `referencias.csv`: valores de 2024/2025 e localizacao das evidencias.
- `source.json`: origem, data da obtencao, caminhos internos dos arquivos e hashes dos pacotes.
- `other-params.txt`: parametros complementares originais fornecidos pelo ORLab.

Cada instancia `N-tipo-tipo.txt` tem sua matriz `tau_kj_N-tipo-tipo.txt`
na mesma pasta. O numero no nome conta clientes, sem incluir o deposito.

Os arquivos foram extraidos sem modificar seus bytes. Os tempos dos drones
vieram prontos do pacote: nenhum tempo foi gerado ou recalculado neste projeto.
A matriz `tau_kj_18-r-e.txt` coincide com a copia historica originalmente usada na raiz.

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

## Uso no programa

O executável percorre uma vez cada uma das 60 instâncias. Consulte o
[README principal](../../README.md) e o [guia do código](../../docs/GUIA_CODIGO.md).
`referencias.csv` conserva as referências de 2024/2025, limites, status e
localização das evidências. Os resultados e fontes detalhados da revisão foram
preservados no ZIP de `historico/`, na raiz do projeto.

Os nomes `144-rc-c` e `173-rc-c` correspondem aos rótulos `144-rc-r` e
`173-rc-r` da tabela de 2024; as entradas não foram renomeadas.

Cada linha de `tau_kj` corresponde a um cliente; a coluna k informa a duração
da missão com k drones cooperando. `-1` indica inviabilidade. As missões incluem
saída e retorno ao depósito. A leitura usa os arquivos canônicos desta pasta;
não é necessário manter cópias na raiz do projeto.
