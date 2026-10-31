# Instâncias para o PDSVRP-C

Coleta em 30/10/2026. Foram localizadas e baixadas as 60 instâncias do
benchmark original: 30 pequenas (15–30 clientes) e 30 grandes (50–200 clientes).
Também foram obtidas 60 tabelas oficiais de tempos de drones já calculados.
Não foram geradas instâncias sintéticas nem recalculados tempos de voo.
Esta é a coleção pública localizada nas fontes abaixo; não é uma garantia
de que inexistam outros conjuntos privados ou publicados em outros locais.

## Arquivos para usar na implementação

- `benchmark/small/`: 30 instâncias pequenas, originais.
- `benchmark/large/`: 30 instâncias grandes, originais.
- `drone_times/tau_kj_<instância>.txt`: tabela de tempos correspondente.
- `inventory.csv`: associação entre instâncias e tabelas, número de clientes,
  drones, combinações viáveis e hashes SHA-256.
- `original/Upload-SuperDrone/other-params.txt`: parâmetros físicos e preparação.
- `original/Upload-SuperDrone/power-consumption-calculation/`: código C++
  original do modelo de energia (`energymodel.cpp` e `energymodel.h`).
  O código referencia `gnuplot-iostream.h`, que não acompanha o pacote;
  não é necessário compilá-lo para usar as tabelas prontas.
- `original/Upload-SuperDrone/bks.txt`: soluções do PDSTSP-C (um caminhão).
  Não interpretar esses valores como BKS do PDSVRP-C com vários caminhões.
- `archives/`: pacotes ZIP oficiais preservados e página HTML da coleta adicional.
- `additional_2025/`: extração integral do pacote mais recente, incluindo dados
  de PDSOP-C, logs e resultados. Dados de lucro/orienteering não fazem parte
  do benchmark PDSVRP-C usado aqui.

## Como ler os dados

Cada instância começa com seis parâmetros: quantidade de drones, velocidade
do caminhão, peso da bateria, capacidade, corrente máxima e tensão.
Após o cabeçalho vêm as colunas `node x y weight`. O nó 0 é o depósito.

Cada tabela de drones tem a coluna `request id` e as colunas `1-unit`,
`2-unit`, ..., até a quantidade de drones da instância. A linha identifica
o cliente; a coluna identifica quantos drones participam juntos da missão.
`-1` representa uma combinação inviável: tratar como indisponível/infinito,
nunca como tempo negativo utilizável no objetivo. Os valores positivos são
os tempos inteiros fornecidos no pacote, preservados sem transformações.
Por exemplo, em `15-rc-c`, o cliente 1 tem valores 13, 12, 12, 12, 12;
o cliente 4 não pode ser atendido por um drone (`-1`).

O artigo de Montemanni et al., seção 4.1, descreve tempos de missões de ida
e volta, pré-calculados e arredondados para cima, distância Manhattan para
caminhões e euclidiana para drones. Na implementação, é necessário conferir
a convenção de unidade e a inclusão do tempo de preparação antes de somar
qualquer parcela adicional: os arquivos de tabelas não explicitam isso no
cabeçalho. Não acrescentar preparação nem aplicar novo arredondamento por
suposição. Os parâmetros originais incluem `drone_prepare_time, 1`.

O PDSVRP-C reutiliza as instâncias do PDSTSP-C acrescentando o número de
caminhões: 2 e 3 nas pequenas; 2, 3, 4 e 5 nas grandes, conforme o artigo.
Esse parâmetro não está nos arquivos originais e deve ser definido na
configuração experimental. São 180 configurações de benchmark ao combinar
as 60 instâncias com essas quantidades de caminhões.

## Fontes e limites de correspondência

1. Pacote original de Nguyen e Hà:
   https://orlab.com.vn/download/instances-for-the-parallel-drone-scheduling-traveling-salesman-problem-with-collective-drones/
   Download: https://orlab.com.vn/download/instances-for-the-parallel-drone-scheduling-traveling-salesman-problem-with-collective-drones/?wpdmdl=9555
2. Pacote de 2025 com as tabelas `data/instances/tau_jk`:
   https://orlab.com.vn/download/data-and-results-of-pdstsp-c-and-pdsop-c/
   Download: https://orlab.com.vn/download/data-and-results-of-pdstsp-c-and-pdsop-c/?wpdmdl=9713
   As 60 instâncias `tsp` deste pacote são idênticas byte a byte às originais.
   Seus logs confirmam a leitura das tabelas `tau_kj` correspondentes.
   Isso comprova a associação com os mesmos dados de entrada, mas não prova
   que as tabelas de 2025 sejam idênticas às usadas nos experimentos de 2024.
3. Montemanni, Dell'Amico e Corsini (2024), *Parallel drone scheduling vehicle
   routing problems with collective drones*, DOI 10.1016/j.cor.2023.106514.
   Preprint aberto: https://arxiv.org/html/2307.03523
   Seção 4.1 descreve a reutilização do benchmark e os números de caminhões.
   Os agradecimentos registram que os autores receberam os tempos otimizados
   de Nguyen e Hà. A página do editor informa dados disponíveis sob solicitação:
   https://www.sciencedirect.com/science/article/pii/S0305054823003787
4. Artigo original do PDSTSP-C: Nguyen e Hà (2023), *The Parallel Drone
   Scheduling Traveling Salesman Problem with Collective Drones*,
   https://doi.org/10.1287/trsc.2022.1192
   Não foi necessário obter o texto fechado para coletar as tabelas oficiais.

## Validação

Execute `python scripts/validate_instances.py` a partir da raiz do projeto.
O script verifica contagens, IDs, dimensões das tabelas, valores permitidos,
identidade das instâncias dos dois pacotes e gera `inventory.csv`.
Resultado da coleta: 60 instâncias, 60 tabelas e 4.316 clientes no total.
Isso valida integridade estrutural; não reproduz o cálculo físico dos tempos.
