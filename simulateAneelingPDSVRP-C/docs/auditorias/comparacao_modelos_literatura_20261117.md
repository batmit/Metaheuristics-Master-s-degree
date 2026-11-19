# Comparacao do modelo local com a literatura

Consulta: 17/11/2026. Pesquisa por PDSVRP-c, titulo completo, collective drones,
vehicle routing e anos 2025/2026. Revisao direcionada, nao levantamento exaustivo
de todos os artigos da area. Ausencia em busca nao demonstra inexistencia.

## Resultado

Nao foi identificada uma restricao estrutural do PDSVRP-C classico que o modelo
local esteja omitindo e que invalide os certificados oficiais examinados.
Isso nao comprova identidade das matrizes experimentais nem ausencia de defeitos
para entradas arbitrarias. E preciso separar problema, dados e algoritmo.

## Hipoteses estruturais

Conferencia com as secoes 2.1 e 3.1-3.4 do COR 2024 e sua copia preprint local:

| Aspecto | PDSVRP-C classico / implementacao local |
| --- | --- |
| Deposito | Unico; saida e retorno de todos os recursos. |
| Atendimento | Cada cliente uma vez, por caminhao ou formacao de drones. |
| Caminhoes | Homogeneos, mesma velocidade, capacidade e autonomia ilimitadas. |
| Numero de rotas | No maximo S; rotas vazias sao permitidas. |
| Drones | Homogeneos; uma entrega por missao; podem realizar varias missoes. |
| Bateria | Nova/carga completa antes de cada missao; nao ha consumo acumulado entre missoes. |
| Colaboracao | k drones ocupados conjuntamente durante toda a missao. |
| Recursos | Frota compartilhada; drones liberados no retorno podem ser reutilizados. |
| Objetivo | Minimizar o ultimo retorno ao deposito, nao a soma de tempos. |
| Operacao paralela | Drones partem do deposito, nao do caminhao. |

O COR 2024 diz explicitamente que os tempos de missoes podem ser pre-calculados
e que combinacoes inviaveis recebem infinito. Usar tau pronto nao constitui
por si so uma simplificacao indevida. A representacao local por intervalos
cumulativos difere das variaveis de fluxo/precedencia de CP/MILP, mas sob drones
homogeneos e missoes no mesmo deposito ela permite atribuir unidades individuais
sem sobreposicao. Isso e um argumento de equivalencia de viabilidade, nao prova
de que o escalonador guloso explore todos os cronogramas possiveis.

## Diferencas e incertezas concretas

1. **Dados de drones:** usamos tabelas publicas ORLab de 2025; os arquivos exatos
   usados pelos autores em 2024 nao foram obtidos. O editor informa dados sob pedido.
2. **Frota:** a descricao do benchmark fala em 5-10 drones nas grandes; o arquivo
   126-rc-e contem 12. E uma discrepancia documental, nao prova de que os autores
   executaram essa instancia com 10. Nao corrigir o arquivo por suposicao.
3. **Fisica impressa versus codigo original:** diferem o termo da Eq.24, a
   aproximacao da Eq.25, o sinal da Eq.4 e a unidade de preparacao. Auditoria local
   detalhada em equacoes_completas_20261117.md. Duas entradas das tabelas distribuidas
   diferem da reproducao do codigo original em 38.450; nenhuma na 126-rc-e.
4. **Arredondamento:** o fundador arredonda tempos equivalentes em minutos para
   cima. O C local calcula tempos de caminhao sem ceil. A auditoria das 60 entradas
   encontrou zero arestas fracionarias, logo nao muda o benchmark atual. Com novas
   coordenadas fracionarias, a convencao deve ser definida antes de comparar.
5. **Versao geral versus benchmark:** o problema formal aceita matriz de viagem;
   nosso programa constroi matriz Manhattan simetrica a partir de coordenadas.
   Isso e adequado ao benchmark, mas nao cobre custos rodoviarios arbitrarios ou
   assimetricos. O delta de 2-opt tambem pressupoe simetria.
6. **Limites fisicos:** maximo de seis drones por formacao e massa de 25 kg estao
   embutidos em tau, nao rechecados a partir de massa/energia no worker. Modificar
   parametros mantendo tau antigo nao representa o novo problema fisico.
7. **Variantes locais:** Euclidiana nos caminhoes e matrizes recalculadas mudam os
   custos/viabilidade. Sao sensibilidades separadas; nao equivalem ao benchmark
   principal para declaracao de recordes.

SA+BB versus RnR, MILP ou CP-SAT e uma diferenca de metodo de solucao. O teto do
BB, os movimentos e o escalonamento guloso limitam qualidade/exploracao, nao
adicionam permissoes ao problema. Nao se deve apresentar o SA como novo modelo
matematico apenas por empregar outra representacao computacional.

## Trabalhos centrais e fontes

- Nguyen e Ha (2023), PDSTSP-C: um caminhao; modelo fisico e RnR. PDF local
  conferido anteriormente nas paginas de equacoes e parametros.
  https://doi.org/10.1287/trsc.2022.1192
- Montemanni, Dell'Amico e Corsini, COR 163 (2024): define o PDSVRP-C com varios
  caminhoes; fonte principal de comparacao estrutural. Texto publicado indexado
  e preprint local consultados. https://doi.org/10.1016/j.cor.2023.106514
  https://iris.unimore.it/bitstream/11380/1328966/2/1-s2.0-S0305054823003787-main.pdf
- Mesmos autores, MIC 2024: heuristica min-max VRP com CP, mesmo problema-alvo.
  Registro e tabela indexada consultados; PDF direto retornou 403, portanto nao
  foi possivel repetir nesta consulta a leitura integral pelo link institucional.
  https://doi.org/10.1007/978-3-031-62912-9_13
  https://iris.unimo.it/handle/11380/1342368

## Trabalhos proximos encontrados: nao confundir modelos

| Trabalho | Diferenca confirmada no texto/resumo disponivel |
| --- | --- |
| Multi-Depot Vehicle Routing Problem with Trucks and Collective Drones Collaboration (preprint, 2025) | Varios depositos; caminhoes equipados com drones e usados como depositos temporarios. |
| The multi-visit parallel drone scheduling pickup and delivery problem considering multiple trips and time windows (2025) | Coleta/entrega, varias visitas por voo, precedencias e janelas de tempo. |
| Multiple drone-vehicle parallel scheduling optimization in humanitarian logistics (2026) | Capacidade/autonomia de ambos os recursos, varias viagens e varias areas por voo. |
| Multi-visit truck-drone pickup and delivery problem with release dates and drone resupply (2026) | Datas de liberacao, janelas, coletas e reabastecimento; outra variante. |

Fontes primarias dos resumos/textos usados para essa triagem:

- https://papers.ssrn.com/sol3/Delivery.cfm/39986652-9993-4ca3-b522-714cb1e0d9fb-MECA.pdf?abstractid=5355869&mirid=1
- https://www.sciencedirect.com/science/article/pii/S1366554525003928
- https://www.sciencedirect.com/science/article/pii/S1366554526003182
- https://www.sciencedirect.com/science/article/pii/S0377221726008556

Tambem foi localizado o titulo Come fly with me: Scheduling point-to-point
transports with collective drones (2026), mas nao se obteve texto primario
suficiente nesta consulta para confrontar suas restricoes. Nao o tratamos como
artigo integralmente analisado nem como prova de identidade/diferenca de modelo.

## Consequencia para nossas 14 solucoes

Os 14 certificados do experimento oficial abaixo das referencias usam somente
caminhoes. Sua viabilidade nao depende da frota, preparacao, energia ou tau dos
drones. A comparacao relevante exige mesmos clientes, deposito, matriz de viagem,
numero de caminhoes e objetivo. Rotas fechadas com clientes unicos e caminhos
Manhattan nas entradas originais sustentam a comparacao local. Nenhuma restricao
de obrigatoriedade de usar drones foi encontrada no modelo classico.

Afirmacao recomendada: “Nosso modelo computacional segue as hipoteses estruturais
do PDSVRP-C classico para o benchmark local. As divergencias identificadas estao
nas convencoes/dados fisicos e em variantes experimentais. Encontramos 14
certificados locais inferiores aos UBs consultados, sem dependencia dos drones.”
Nao substituir isso por “todos os artigos comprovam que nosso modelo e identico”.
