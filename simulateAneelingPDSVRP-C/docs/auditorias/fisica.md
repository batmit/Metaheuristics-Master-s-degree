# Auditoria de conformidade do PDSVRP-C

Data: 06/11/2026. Escopo: código de produção, 60 instâncias, 60 matrizes de
drones e fotografia congelada de 120 certificados. Esta auditoria não modifica
as entradas nem o registro central de resultados.

## Conclusão

O núcleo implementa uma heurística para o PDSVRP-C: vários caminhões
homogêneos e drones coletivos operam em paralelo a partir de um depósito,
minimizando o instante do último retorno. A representação e as verificações
de viabilidade são adequadas a esse modelo com as matrizes locais.

Não há base para afirmar que todas as tabelas reproduzem literalmente tudo
do PDF, nem que reproduzem o experimento de cinco caminhões que publicou 76.
Há duas diferenças no recálculo das 60 matrizes, ambiguidades entre o PDF e
o C++ original e ausência da matriz/certificado exatos daquele experimento.
Isso impede tratar o gap de 47,37% como uma medida confirmada da qualidade
do SA em condições idênticas às da literatura.

## Artigos e tabelas diferentes

O PDF fornecido, Nguyen e Hà, Transportation Science 2023,
DOI 10.1287/trsc.2022.1192, trata do **PDSTSP-C, com um caminhão**. É a
referência física e a origem das instâncias; sua heurística é Ruin-and-Recreate.
Não é a fonte das tabelas de resultados com dois a cinco caminhões.

O PDSVRP-C é a extensão com vários caminhões descrita por Montemanni,
Dell'Amico e Corsini, seção 3 do [artigo COR](https://arxiv.org/html/2307.03523).
O catálogo local registra a fonte de cada LB/UB em
`instances/literature_pdsvrp_c.csv`: há valores COR2024 e MIC2024.
Um UB publicado não deve ser apresentado como ótimo provado quando LB<UB.
O valor 76 da 126-rc-e/S=5 vem da
[Tabela 1 do trabalho MIC](https://iris.unimore.it/bitstream/11380/1342368/2/chapter.pdf),
e não do PDF de Nguyen e Hà.

As matrizes locais são cópias das tabelas públicas do pacote ORLab de 2025;
o verificador de integridade compara os bytes com o ZIP preservado.
Identidade com esse pacote não demonstra identidade com as matrizes privadas
recebidas pelos autores dos experimentos anteriores.

## Recálculo físico completo

Executado `scripts/auditar_conformidade_pdsvrpc.py`, usando o C++ energético
original, parâmetros próprios de cada arquivo e coordenadas relativas ao
depósito de cada instância. Foram preservadas suas equações e sua busca de
velocidades. Apenas a dependência/rotina de plotagem foi removida da cópia
experimental. Aplicaram-se também os limites textuais de seis unidades por
formação e massa total de 25 kg, incluindo drones, baterias e encomenda.
As seis fases de voo incluem 200 segundos verticais; acrescenta-se um minuto
de preparação para k>1 e arredonda-se a duração final para cima em minutos.

| Conferência | Resultado |
|---|---:|
| Instâncias | 60 |
| Clientes | 4.316 |
| Pares cliente/k, incluindo combinações proibidas | 38.450 |
| Entradas coincidentes | 38.448 |
| Entradas divergentes | 2 |
| Entradas divergentes da 126-rc-e | 0 de 1.512 |
| Arestas de caminhão com minutos fracionários nos dados atuais | 0 |

| Instância | Cliente | k | Tabela distribuída | Recálculo |
|---|---:|---:|---:|---:|
| 105-rc-e | 81 | 2 | 30 | 29 |
| 66-rc-e | 31 | 2 | -1 | 41 |

As diferenças estão na reprodução dos dados, não em uma equação implementada
pelo SA: o SA consulta a matriz pronta. A primeira entrada reserva um minuto
a mais; a segunda exclui uma alternativa que o recálculo considera viável.
As duas são conservadoras em relação à física reproduzida. A causa histórica
das duas entradas não foi identificada: não se presume erro dos autores,
nem se substituem silenciosamente os arquivos oficiais.

O PDF tem diferenças internas/de implementação já documentadas em
`output/verificacao_literatura_126/relatorio_recalculo.md`: unidade de preparação,
sinal da restrição de capacidade consumida, par de velocidades do Algoritmo 2
e termo de arrasto da Eq.24. Portanto, esta conferência reproduz o **código
original mais as regras textuais explicitadas**, e não uma leitura literal
de todas as expressões impressas. Também não constitui validação experimental
da física de um drone real.

## Conformidade do algoritmo

| Requisito | Implementação e avaliação |
|---|---|
| Vários caminhões | Rotas independentes; número configurável, inclusive rotas vazias. |
| Rotas fechadas no depósito | O depósito é implícito antes/depois de cada vetor; ambos os trechos entram no custo. |
| Atendimento único e completo | `calcularMakespan` e `validar` rejeitam repetição, omissão e IDs inválidos. |
| Drones somente a partir do depósito | Cada missão representa ida, entrega e retorno; não há lançamento pelo caminhão. |
| Colaboração sincronizada | k drones ficam ocupados durante todo o intervalo da missão. |
| Frota compartilhada limitada | Soma dos k ativos conferida em cada início, usando intervalos [início,fim). |
| Drones reutilizáveis | Uma missão pode iniciar quando outra termina. Como os drones são homogêneos, a capacidade cumulativa permite atribuir unidades individuais sem sobreposição. |
| Carga/energia e combinações permitidas | Codificadas em tau; -1 vira infinito. O gerador escolhe somente células finitas, inclusive quando há lacunas entre k permitidos. |
| Caminhões sem limite de carga/autonomia | Coerente com a variante estudada; não se acrescentam restrições de um CVRP convencional. |
| Objetivo | Máximo entre todos os retornos dos caminhões e términos das missões. |
| Busca | SA com cinco tipos de movimento, aceitação de Metropolis e preservação separada do melhor. |
| Refinamento de drones | BB reordena missões com clientes/k fixos; sua execução limitada não prova ótimo global. |
| Publicação de resultados | Revalidação integral em C e validação independente em Python antes de salvar. |

Arquivos examinados: `main.c`, `lerInstancias.c`, `cv.c`, `SA.c`, `drone_bb.c`
e `scripts/executar_benchmark.py`, além dos respectivos cabeçalhos e testes.
Não foi identificada violação das restrições acima nas soluções examinadas.
Essa evidência não é uma prova formal de ausência de qualquer defeito para
qualquer entrada possível.

## Evidências e limitações de implementação

A suíte já executada, registrada em `output/verificacao.json`, contém compilação
C11 com avisos como erros, 19 testes Python, 90.000 vizinhos em 180 configurações,
oráculo independente para o escalonador BB e trajetórias SA nas instâncias de
15, 50 e 200 clientes. Esses resultados anteriores não foram apresentados
como uma nova execução nesta etapa.

Nesta etapa, `scripts/resumir_conformidade_pdsvrpc.py` revalidou os **120
certificados** da fotografia congelada e confrontou **1.476 missões** com as
novas matrizes. Nenhuma missão exige combinação fisicamente rejeitada pelo
recálculo ou reserva tempo insuficiente. Apenas 105-rc-e/S=2 usa a célula de
30 em vez de 29 minutos: sua reserva é conservadora. A fotografia e os 120
pares de arquivos de entrada têm hashes em `impacto_certificados.json`.

Pontos que limitam generalização ou desempenho, sem invalidar esses certificados:

1. O C e o validador Python calculam tempo de caminhão sem `ceil` por aresta.
   Nos 60 arquivos todos os valores já são inteiros, portanto não há efeito
   atual. Para novos dados fracionários, ambos precisariam adotar a convenção
   de arredondamento do benchmark para reproduzi-la.
2. A física é pré-processada: os validadores confiam em tau. Uma matriz externa
   adulterada pode liberar combinações indevidas; a validação física desta
   auditoria vale para os arquivos identificados pelos hashes, não para qualquer
   tabela futura.
3. O BB tem orçamento de 100 mil nós, não muda atribuições cliente/veículo nem k
   e é aplicado ao melhor incumbente. Marcar um cronograma como já refinado pode
   impedir nova busca após uma redução do gargalo dos caminhões. Isso pode perder
   oportunidades de melhora, sem tornar o cronograma inválido.
4. O SA reescalona as missões ao inicializar: uma futura API de warm start com
   cronograma externo melhor pode perder esse benefício. A execução atual começa
   com todos os clientes nos caminhões, portanto não utiliza esse caminho.
5. A calibração considera ausência de vizinho uma falha; casos degenerados fora
   do benchmark podem ter uma solução válida sem haver movimento disponível.
6. Não existe parada por igualdade com um limite inferior demonstrado. O algoritmo
   pode continuar gastando tentativas após já atingir o ótimo dos dados locais.

São oportunidades de robustez/desempenho, distintas de afirmar que o algoritmo
implementa o problema errado. SA também não precisa reproduzir os movimentos
RnR ou os modelos CP-SAT dos artigos para ser uma heurística do mesmo problema.

## O caso 126 e a interpretação dos gaps

Todas as 1.512 entradas da 126-rc-e coincidem. O cliente 46 permanece inviável
para drones nas formações permitidas e exige ao menos 112 minutos de caminhão.
Há solução válida de 112 com cinco caminhões: logo 112 é ótimo **para esses
dados locais**. O UB publicado de 76 não pode ser alcançado nessas mesmas
condições. Isso demonstra uma incompatibilidade ainda não explicada entre
os conjuntos/premissas comparados; não demonstra que a solução publicada é
inválida, pois seu certificado e sua matriz exata não estão disponíveis.

Os certificados originais de um caminhão foram reconstruídos e conferidos:
RnR=144 e log público de 2025=136. Eles apoiam a interpretação dos IDs e dos
tempos, mas não validam a solução de cinco caminhões com 76. As duas execuções
completas isoladas do SA, com tau distribuída e recalculada, terminaram em 112.

Até obter a matriz exata e o certificado do experimento de referência, os gaps
para esse experimento devem ser tratados como **comparações provisórias entre
resultados cuja equivalência de dados não foi demonstrada**. O passo científico
prioritário é resolver essa rastreabilidade antes de atribuir a discrepância
inteiramente à qualidade da busca.

## Reproduzir

Na raiz do projeto, com Python 3 e G++:

```
python3 scripts/auditar_conformidade_pdsvrpc.py
python3 scripts/resumir_conformidade_pdsvrpc.py
```

No Windows, pode-se usar `python` se esse for o nome disponível. O segundo
comando depende da fotografia congelada já preservada no projeto. Matrizes
recalculadas e CSVs são experimentais e ficam nesta pasta; não substituem
`instances/drone_times/`.
