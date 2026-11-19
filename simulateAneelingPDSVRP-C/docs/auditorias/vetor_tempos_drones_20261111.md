# Correspondencia do vetor de tempos com o artigo

Data: 11/11/2026. Complementa `artigo_fisica126_20261111.md`.
Nenhum arquivo de producao foi alterado nesta conferencia.

## O que o programa armazena

O objeto usado nao e um unico tempo por cliente: e a matriz
`Instancia.tempoDrone[cliente][k]`, de tipo `double **` em `main.h`.
Na 126-rc-e sao alocadas 127 linhas e 13 colunas; linha zero corresponde
ao deposito e coluna zero nao representa uma formacao valida.
As 126*12 combinacoes reais vem de `tau_kj_126-rc-e.txt`.

Uma celula positiva e a duracao completa, em minutos, da missao
deposito-cliente-deposito com k unidades. -1 no arquivo vira INFINITY
na memoria. Nao significa distancia, energia ou tempo por drone.

Ja `Solucao.missaoDrone[]` armazena as decisoes da solucao: cliente,
quantidade de drones, inicio e fim. Esse vetor nao substitui a matriz;
consulta a celula correspondente para determinar a duracao.

## Conferencia de cada componente

| Aspecto do artigo | Onde aparece na implementacao | Resultado |
| --- | --- | --- |
| Dependencia do cliente e numero k | Uma celula distinta para cada par (j,k) | Representacao adequada. |
| Distancia euclidiana de ida e retorno | Preprocessamento fisico | Incorporada em tau; nao e recalculada durante o SA. |
| Ida com carga w/k e volta vazia | Modelo energetico distribuido | Incorporada na selecao das velocidades e na admissibilidade. |
| Decolagem/pouso nos dois trechos | Preprocessamento | 200 segundos verticais totais incorporados. |
| Velocidades variaveis e diferentes na ida/volta | Busca dos pares com passo 0,2 m/s | Incorporadas em tau. |
| Preparacao da formacao para k>1 | Preprocessamento | Um minuto incorporado; nao e somado novamente pelo escalonador. |
| Corrente maxima, Peukert e reserva de bateria | Preprocessamento | Combinacao rejeitada recebe -1. |
| Massa total de 25 kg e formacao de ate seis unidades | Preprocessamento | Celulas nao permitidas recebem -1, inclusive k=7..12. |
| Arredondamento para minutos inteiros | Preprocessamento | Incorporado uma vez na duracao. |
| Ocupacao simultanea de k drones | Perfil de recursos em drone_bb.c | k unidades reservadas em todo [inicio,fim), incluindo preparo. |
| Espera ate que k unidades estejam disponiveis | primeiroInicioPerfil | Incorporada no inicio da missao; nao deve ser um acrescimo fixo em tau. |
| Sincronizacao da formacao | Uma missao possui inicio/fim comuns e demanda k | Sem partidas individuais em horarios distintos. |
| Reutilizacao e bateria nova antes de cada partida | Liberacao no fim e tau validada por missao | Coerente com a hipotese de bateria carregada pronta; nao acumula descarga entre missoes. |
| Atendimento de cada cliente exatamente uma vez | calcularMakespan e validador Python | Conferidos entre caminhoes e missoes. |
| Objetivo de ultimo retorno | Maior fim das missoes e maior rota de caminhao | Nao soma missoes paralelas nem usa k*tau como duracao. |

O produto k*tau usado no limite inferior de carga do BB mede drone-minutos,
nao tempo de voo. Dividir essa carga pela frota produz um limite inferior;
isso nao substitui a duracao fisica nem o makespan do cronograma.

## Rastreio da leitura e do uso

`lerTemposDrone`, em lerInstancias.c, ignora o cabecalho, exige IDs
consecutivos e le exatamente uma celula por unidade da frota. Os IDs
do arquivo coincidem com os indices dos clientes; nao ha subtracao de um
no C. A elegibilidade e derivada das celulas finitas.

`quantidadeViavel` e `sortearQuantidade`, em cv.c, consultam cada celula.
O campo minDrones e informativo: o algoritmo nao transforma todas as
quantidades acima do minimo em alternativas automaticamente viaveis.
Isso e importante porque massa total pode proibir formacoes maiores.

`duracaoBB` consulta exatamente tempoDrone[cliente][k]. Tanto o guloso
quanto o BB fixam fim=inicio+duracao e ocupam k unidades nesse intervalo.
Quando clientes/k mudam, o gerador de vizinhos reescalona as missoes;
alterar somente as rotas dos caminhoes permite preservar o cronograma.

`calcularMakespan` exige fim-inicio=tau, confere capacidade simultanea e
retorna max(maior tempo dos caminhoes, maior fim dos drones). O validador
Python repete a conferencia usando o arquivo oficial, com indices
times[cliente-1][k-1], porque sua tupla nao inclui deposito/coluna zero.
Os dois sistemas de indices sao coerentes.

Nao e necessario armazenar os numeros fisicos de cada drone para calcular
um cronograma neste modelo: sao unidades homogeneas, com mesmo deposito e
sem historico de bateria entre missoes. O perfil cumulativo garante a
disponibilidade de k unidades. A representacao nao reproduz os operadores
RnR do artigo, mas isso nao e necessario para respeitar suas restricoes.

## Teste direto executado

Foi compilado um pequeno programa isolado usando os arquivos atuais
lerInstancias.c, cv.c, drone_bb.c e SA.c. Ele exportou as celulas da
matriz realmente carregada, e nao uma segunda leitura em Python.

- Todas as **1512 celulas carregadas** coincidiram com o arquivo original.
- Todas as colunas k=7..12 permaneceram proibidas.
- O cliente 46 permaneceu proibido em todas as colunas.
- Com uma missao para cliente 1/k=2 e outra para cliente 3/k=1, os tempos
  efetivos foram [0,40) e [0,22), respectivamente. As outras 124 entregas
  foram atribuidas a uma rota de caminhao para validar cobertura completa.
- O tempo dos drones foi **40**, nao 62, 80 nem 20. O makespan global
  desse exemplo de cobertura tambem considera a rota de caminhao; 40 aqui
  e especificamente o maior termino dos drones.

O teste anterior de fisica tambem continua valido: todas as 1512 entradas
coincidem com o recalculo do codigo energetico original mais as regras
textuais explicitadas no relatorio do artigo.

## Limites da afirmacao de correspondencia

Nao e correto dizer que o vetor corresponde literalmente a todas as
expressoes do PDF. Ele corresponde ao **modelo computacional distribuido**,
com as interpretacoes documentadas: sinal <= no consumo de capacidade,
preparo de um minuto, par ida/volta distinto e termo adicional da potencia
horizontal. O texto impresso contem contradicoes entre essas regras.

A matriz tampouco guarda os valores de velocidade, potencia, consumo,
angulo e parametros usados em sua geracao. Ela guarda apenas o resultado
final e a admissibilidade. Esses detalhes precisam de rastreabilidade
externa para serem auditados; nao precisam ser repetidos em cada iteracao.

Ha uma limitacao concreta: mudar bateria, peso, coordenadas ou regras em
outro arquivo **nao recalcula tau automaticamente**. O codigo de producao
le os parametros fisicos, mas nao reexecuta o modelo energetico ao carregar
a matriz. Uma matriz de outro experimento com o mesmo tamanho pode ser
estruturalmente aceita. A concordancia fisica aqui verificada vale para
os arquivos atuais e seus hashes, nao para qualquer combinacao futura.

O leitor C tambem aceita zero como duracao, enquanto o validador Python
do benchmark exige valores positivos. Nao existe zero nos arquivos
oficiais conferidos, portanto isso nao modifica os resultados atuais.

Logo: nao foi encontrada omissao nem contagem duplicada de preparo,
ida/volta ou fases verticais no uso atual da matriz da 126-rc-e. Isso nao
comprova equivalencia com os dados da MIC que produziram 76; o limite
inferior local de 112 continua valendo.

## Artefatos

Em `output/pdf/revisao126_20261111/`:

- `conferir_vetor.c`: programa de conferencia ligado ao codigo de producao.
- `vetor_carregado.txt`: celulas exportadas e missoes do teste.
- `conferencia_vetor.json`: comparacoes e duracoes verificadas.
- `resultado.json` e `fisica.csv`: recalculo fisico anterior, parametros e
  comparacao integral da matriz.
