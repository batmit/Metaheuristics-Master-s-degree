# Conformidade das demais instancias com o artigo

Data: 11/11/2026. Escopo: 59 instancias, excluida a 126-rc-e por solicitacao
do usuario. A 126-r-c e um arquivo diferente e permanece incluida.
Artigo-base: Nguyen e Ha, Transportation Science 57(4), pp.866-888, 2023,
DOI 10.1287/trsc.2022.1192, PDF fornecido pelo usuario e relido nesta conversa.

## Parecer

Nao foi encontrado erro de calculo, cobertura ou sincronizacao nos casos
verificados do nucleo atual. A implementacao e coerente com as regras
de atendimento do modelo para os dados locais, estendidas a varios caminhoes.
Entretanto, **nao e correto afirmar correspondencia integral/literal com o
artigo**: existem duas divergencias numericas no preprocessamento, diferencas
entre a descricao do gerador e os dados distribuidos, e um algoritmo de busca
diferente do publicado.

Nenhuma entrada, certificado ou algoritmo de producao foi modificado.
As unicas alteracoes foram ferramentas de auditoria e esta documentacao.

## Verificacoes novas realizadas

| Verificacao | Resultado |
| --- | --- |
| Instancias e matrizes contra ZIP oficial preservado | 59/59 pares identicos byte a byte |
| Clientes examinados | 4190 |
| Celulas de tempos recalculadas pelo C++ energetico original | 36938 |
| Celulas coincidentes | 36936 |
| Divergencias | 2, ambas conservadoras em relacao a esse recalculo |
| Configuracoes examinadas | 176 |
| Vizinhos conferidos por avaliacao integral | 88000 |
| Certificados salvos, verificados em C e Python | 116 validos |
| Missoes desses certificados comparadas com a fisica recalculada | 1510; nenhuma fisicamente rejeitada ou com tempo insuficiente |
| Solucoes originais de um caminhao em bks.txt | 59 reconstruidas e com valor igual ao declarado |
| Arestas de caminhao com minutos fracionarios | Zero |

Foram tambem executados o oraculo independente de BB, o teste de acionamento
apos cinco tentativas e testes de trajetoria do SA nas instancias 15-rc-c,
50-r-e e 200-r-e. As compilacoes usaram C11, -O2 e avisos tratados como erros.
Os resultados sao evidencias de corretude dos casos examinados, nao uma
prova formal de ausencia de qualquer defeito em qualquer entrada.

## Correspondencia das regras de atendimento

| Regra | Implementacao e parecer |
| --- | --- |
| Cada cliente atendido uma vez | C e Python conferem cobertura conjunta de caminhoes/drones. |
| Retorno ao deposito | Incluido nas rotas e em cada duracao de missao. |
| Tempo de caminhao Manhattan/velocidade | Correto para os parametros atuais, 30 km/h. |
| Distancia aerea euclidiana | Usada no preprocessamento com coordenadas relativas ao deposito de cada arquivo. |
| Carga compartilhada por k unidades | Incorporada no modelo energetico que gera tau. |
| Ida carregada e volta vazia | Incorporadas na busca de velocidades. |
| Quatro fases verticais | 200 segundos ao todo, incorporados uma vez. |
| Preparo coletivo | Um minuto para k>1, incorporado uma vez. |
| Corrente, Peukert e reserva de bateria | Incorporados na admissibilidade dos pares cliente/k. |
| Limites de massa e formacao | Preprocessamento com massa total<=25 kg e k<=6. |
| Arredondamento dos tempos de drones | Aplicado no preprocessamento, sem segundo ceil no escalonamento. |
| Espera e sincronizacao | k unidades reservadas no intervalo inteiro da missao, inclusive preparo. |
| Frota limitada e reutilizacao | Perfil cumulativo e validacao nos inicios; liberacao no instante de termino. |
| Bateria carregada antes de cada partida | Tau verificada por missao, sem consumo acumulado entre partidas; corresponde a hipotese do artigo. |
| Objetivo de ultimo retorno | max entre maior rota de caminhao e maior termino de missao; nao soma missoes paralelas. |

O artigo manda arredondar os tempos de caminhao para cima; o codigo nao
aplica ceil por aresta. Nas 59 instancias todas as arestas ja tem minutos
inteiros, de modo que os valores coincidem. Em dados futuros com arestas
fracionarias, isso exigiria ajuste para reproduzir a convencao publicada.

## As duas divergencias fisicas

| Instancia | Cliente | k | Matriz distribuida | Recalculo | Efeito |
| --- | ---: | ---: | ---: | ---: | --- |
| 105-rc-e | 81 | 2 | 30 min | 29 min | Reserva um minuto adicional; pode restringir a qualidade. |
| 66-rc-e | 31 | 2 | -1 | 41 min | Exclui uma alternativa que o codigo energetico permite. |

As duas celulas sao identicas as do ZIP preservado: a diferenca nao nasceu
na leitura feita pelo C nem em alteracao local do arquivo. O modelo
recalculado encontra velocidades aproximadamente (18,6;18,2) m/s e
consumo 10,799684 Ah na primeira, e (14,4;14,0) m/s e 10,798825 Ah na segunda.
Ambos estao perto do orcamento de 10,8 Ah. Nao foi determinada a causa
historica: pode envolver versao do gerador ou detalhes numericos.

Essas divergencias sao relativas ao codigo energetico distribuido e as
convencoes explicitadas; nao justificam chamar a matriz oficial de errada
sem confirmar como foi gerada. Nenhuma das 1510 missoes na fotografia dos
116 certificados usa as celulas divergentes. Portanto, nao foi encontrado
efeito direto delas nas duracoes dos certificados examinados. A exclusao
de uma alternativa ainda pode afetar solucoes que o algoritmo poderia buscar.

## Diferencas entre o texto e os arquivos oficiais

A Secao 5.1 descreve frotas grandes entre 5 e 10 unidades. Nove dos arquivos
examinados informam 11 ou 12: 103-rc-c, 105-rc-e, 108-rc-e, 121-rc-e,
144-rc-c, 167-r-e, 173-r-c, 198-c-c e 200-r-e.

A mesma secao descreve deposito central em (7,5;7,5), mas os 33 arquivos
com sufixo central usam outras coordenadas. Por exemplo, 68-rc-c usa (6,6),
154-c-c usa (4,8), e 21-c-c usa (12,5). O programa utiliza corretamente o
deposito gravado no arquivo, nao substitui por (7,5;7,5).

Essas observacoes impedem afirmar que cada frase da geracao de instancias
descreve literalmente os arquivos distribuidos. A coincidencia byte a byte
com o ZIP e a reproducao dos valores das 59 solucoes originais apoiam o uso
dos arquivos como fonte efetiva do benchmark; nao resolvem a discrepancia
editorial/historica entre texto e dados. Mudar frotas ou depositos para
adequar o texto criaria outros experimentos.

## Algoritmo e desenho experimental

O artigo usa um caminhao, RnR com seis operadores de remocao, insercao em
rotas de caminhoes/drones, avaliacao por ordenacao topologica, aceitacao por
limiar e busca local. O projeto atual e uma heuristica SA para a extensao
com varios caminhoes, cinco tipos de movimento e refinamento BB de
ordem/horarios com quantidades de drones fixas.

Essas escolhas nao tornam as solucoes invalidas, mas o codigo nao deve ser
apresentado como uma reproducao do algoritmo RnR. Tampouco se deve comparar
diretamente um resultado com dois a cinco caminhoes com as Tabelas 2/3 de
um caminhao do PDF anexado. A reconstrução dos certificados com um caminhao
nesta auditoria e um teste separado e nao usa os BKS multicaminhao.

O artigo repete as heuristicas 30 vezes. Uma repeticao do SA, ou melhores
acumulados de quantidade variavel de repeticoes, nao reproduz esse protocolo.
Os gaps do artigo tambem incluem medidas entre repeticoes e melhores do
proprio experimento; o gap exibido pelo projeto e desvio contra a referencia
multicaminhao cadastrada. Sao metricas que exigem contexto ao comparar.

O BB com teto de 100000 nos, aplicado ao melhor incumbente, nao prova
otimalidade global e nao altera atribuicoes ou k. Sua marca de refinamento
pode impedir nova tentativa apos uma mudanca no gargalo dos caminhoes;
isso e uma limitacao de busca, nao uma violacao de viabilidade.

## Interpretacao fisica adotada

Permanecem as ambiguidades gerais ja confirmadas no PDF: sinal da Equacao
(4), unidade de preparacao da Tabela 5, par de velocidades no Algoritmo 2 e
termo adicional da potencia horizontal existente no C++. Esta auditoria
segue o codigo original com as regras textuais coerentes; nao afirma
equivalencia literal de cada expressao impressa.

O C recebe tau pronta. Alterar bateria, coordenadas ou peso sem regenerar
a matriz nao atualiza automaticamente os tempos. O programa pode aceitar
uma matriz de dimensoes compativeis e origem diferente; a conferencia de
integridade e de fisica e que assegura a correspondencia dos arquivos atuais.

## Reproduzir sem alterar dados de producao

```sh
python3 scripts/auditar_conformidade_pdsvrpc.py --exclude 126-rc-e --output output/auditoria_59_20261111
python3 scripts/verificar_conformidade59.py
```

Resultados em `output/auditoria_59_20261111/`: `tabelas_fisicas.json`,
`pares_fisicos.csv`, `conformidade.json`, `certificados_snapshot.json` e
`impacto_e_texto.json`. Os comandos escrevem somente artefatos de auditoria.
