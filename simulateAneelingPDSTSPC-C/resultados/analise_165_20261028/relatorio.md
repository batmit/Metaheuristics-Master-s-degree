# Melhorias prioritárias para a 165-r-c

28/10/2026. Análise e ensaios separados, sem alterar o algoritmo de produção. Objetivo: investigar como procurar uma solução inferior à referência 160, cujo limite inferior publicado é 156. Não foi encontrada solução abaixo de 160 nesta etapa.

## 1. Uma melhora exige mudar a atribuição, inclusive em relação à referência

Reavaliei 190 certificados recentes, com as mesmas fontes atuais: oito atingiram 160; 74 ficaram em 164 e 65 em 168. A comparação é descritiva, com diferentes sementes e lotes. Das 190 execuções, 180 registraram duração ≤35 s, e seu melhor também foi 160.

O tempo do caminhão é múltiplo de quatro: as coordenadas são inteiras, a distância é Manhattan e a velocidade é 30 km/h. Um circuito fechado tem comprimento Manhattan par; convertido em minutos, fica múltiplo de quatro.

Assim, qualquer solução de 157, 158 ou 159 precisa de **caminhão ≤156**. Se os drones também terminarem em 156, o resultado seria 156. Por isso, o primeiro problema de melhoria pode ser expresso como:

`T_caminhão <= 156 e T_drones <= 159`.

Não basta tentar tirar um minuto do cronograma mantendo a rota atual de 160.

### Limite matemático da rota condicionada

Escolhi como representante a melhor solução de menor semente, 1793136331, do lote de 20 execuções. Ela tem caminhão=160 e drones=160. A testemunha oficial tem caminhão=160 e drones=157.

Calculei limites inferiores por 1-tree com penalidades de Held–Karp, agrupando os clientes do caminhão por coordenada. Para cada conjunto fixo, tomei uma árvore geradora mínima sem o depósito e duas arestas incidentes ao depósito. Sob custos ajustados por potenciais dos vértices, subtraí duas vezes a soma dos potenciais. Todo circuito é uma 1-tree e tem grau dois em cada vértice; portanto esse valor limita inferiormente qualquer circuito desse conjunto.

O cálculo foi repetido com frações racionais, e um segundo programa conferiu a árvore por Kruskal, independentemente do Prim usado na construção.

| Conjunto de clientes fixado no caminhão | LB exato, mostrado em decimal | Menor duração possível, múltipla de quatro | Rota disponível |
|---|---:|---:|---:|
| Melhor SA representativo | 156,122058… | ≥160 | 160 |
| Testemunha oficial | 156,633781… | ≥160 | 160 |

**Isso prova que ambas as rotas de 160 já são ótimas para seus respectivos conjuntos de clientes.** Uma solução melhor que 160 exige outra atribuição, diferente tanto da melhor representativa quanto da oficial. Não é prova de ótimo global da instância.

Os 13 clientes obrigatórios do caminhão, isoladamente, admitem rota ótima de 120, calculada por Held–Karp exato com 53.248 estados. A exigência de 160 nesses dois conjuntos vem da inclusão e da combinação dos clientes opcionais, não apenas dos clientes obrigatórios.

## 2. A transferência aparentemente óbvia não fecha a capacidade

O melhor representante tem 76 clientes nos drones e 89 no caminhão. Entre os elegíveis no caminhão, 75 têm economia individual de remoção igual a zero. Apenas o cliente 52, em (0,12), economiza quatro minutos ao ser retirado da rota atual.

Retirar 52 produz rota de 156. Mas o trabalho mínimo dos drones muda assim:

| Quantidade | Drone-minutos |
|---|---:|
| Carga mínima do conjunto atual dos drones | 1564 |
| Carga mínima adicional do cliente 52 | 84 |
| Carga mínima após a transferência | 1648 |
| Capacidade de dez drones no horizonte 159 | 1590 |
| Trabalho mínimo que precisa retornar ao caminhão | ≥58 |

Na rota resultante, somente os clientes 64 e 71, atualmente nos drones, têm inserção individual de custo zero. Suas cargas mínimas somam apenas 24. Inserir ambos não cria novos segmentos geométricos; não libera os 58 necessários. Algum retorno adicional, outra transferência e/ou reconstrução mais profunda da rota é necessário.

Essa observação é sobre a rota atual e a melhor inserção direta, não uma prova de inviabilidade de todas as novas atribuições. Ela explica por que a troca mais imediata não produz 159.

As atribuições do representante e da testemunha oficial diferem em sete clientes: 52 iria para drones, enquanto 33, 64, 65, 71, 133 e 164 voltariam para caminhão. Entretanto, a rota da testemunha também não admite 156 com esses clientes fixados. Copiar a referência não bastaria para melhorá-la.

## 3. O quanto os gargalos se repetem

Com meta de melhoria 159:

| Obstáculo nos certificados recentes | Quantidade |
|---|---:|
| Caminhão >159 | 190/190 |
| Drones >159 | 188/190 |
| LB mínimo de carga dos drones >159 | 148/190 |

O limite de carga é `ceil(sum_j min_k[k*tau(j,k)] / 10)`, condicionado aos clientes escolhidos para drones. Nessas 148 soluções, não há quantidade nem prioridade de missões que resolva 159 sem mudar a atribuição.

No melhor representante, o LB é 157: a impossibilidade pelo trabalho mínimo não ocorre. Mas a margem em relação a 159 é só 26 drone-minutos, e a rota condicionada impede a melhora. A testemunha oficial usa carga mínima 1501, abaixo dos 1564 do representante; mesmo com maior folga dos drones, ela mantém o caminhão em 160.

## 4. O que já foi testado nesta etapa

### Refinamento do representante

- Escolher quantidades por menor carga: 160/160, makespan 160.
- B&B com até 100 mil nós: 160/160, makespan 160; atingiu limite de nós.
- 2-opt/reinserção e 20 reconstruções da rota, mantendo os mesmos clientes: 160/160. Aqui o limite da seção 1 já demonstra que uma rota de 156 é impossível.
- 500 propostas de troca bidirecional e 500 reconstruções independentes a partir do mesmo estado: nenhuma melhorou o makespan e nenhuma reduziu o caminhão.

### Alterações simples em busca continuada

Cada execução começou da solução já conhecida de 160, com três sementes por variante, 15 s incluindo calibração e três processos concorrentes. Foram comparadas variantes com uma alteração por vez:

| Variante | Resultados finais | Mediana de avaliações |
|---|---|---:|
| Algoritmo atual | 160,160,160 | 187403 |
| Troca/reconstrução com 5% de probabilidade cada | 160,160,160 | 65078 |
| Reconstrução removendo 15–30% dos clientes | 160,160,160 | 160000 |
| Meta do reparo igual a makespan atual−1 | 160,160,160 | 191962 |

Todas as variantes devolveram o mesmo conjunto de clientes da solução inicial de 160. Isso descreve o melhor final devolvido, não significa que o percurso da busca deixou de explorar outras atribuições.

O ensaio não demonstrou ganho de nenhuma alteração. A frequência maior de operadores caros reduziu as avaliações observadas para cerca de 35% do controle. Isso reforça a necessidade de medir melhorias por segundo, mas três sementes não sustentam uma conclusão estatística definitiva sobre qualidade.

Não alterei as fontes de produção. Os nove certificados dos reparos e os doze certificados da busca continuada passaram pela validação independente.

## 5. Primeira prioridade: reparo conjunto com meta explícita

O operador atual escolhe um grupo para drones e devolve clientes de acordo com alívio de carga/custo de inserção; depois reinserta sequencialmente. A reconstrução genérica também decide cliente a cliente, comparando o makespan parcial. Ela testa todas as quantidades viáveis, mas somente três prioridades de inserção nos drones: início, fim e uma posição aleatória.

Para a 165, eu implementaria um reparo que receba o melhor makespan global B e procure:

`H = B−1; limite_rota = 4*floor(H/4); limite_drones = H`.

Quando B=160, os limites são 156 e 159. O objetivo é escolher simultaneamente:

1. Quais clientes saem de cada recurso.
2. Quais clientes retornam ao caminhão e como a rota é reconstruída.
3. As quantidades e as prioridades dos drones no conjunto final.

As seleções devem usar economia de percurso, alívio de carga e proximidade, mas avaliar o resultado conjunto. Um retorno caro isoladamente pode fazer parte de uma troca que elimina outro local ou permite outra rota. A regra gulosa por uma razão individual não reconhece necessariamente essa complementaridade.

O reparo também deve usar o melhor global como alvo aspiracional, não perseguir apenas uma melhora do estado corrente do SA, que pode estar muito pior que 160.

## 6. Segunda prioridade: ejection chains e reinserção por arrependimento

Uma cadeia de ejeção permite transferir um cliente para drones, devolver outro para caminhão, retirar um terceiro para recuperar percurso e só então avaliar o conjunto. Isso generaliza a troca num único sentido seguida de retornos gulosos.

Proposta inicial de implementação experimental:

- Formar um conjunto candidato com clientes de maior economia de rota, clientes dos drones com maior carga e clientes próximos dos endereços afetados.
- Explorar cadeias de duas a quatro decisões, mantendo várias alternativas intermediárias, em vez de somente a melhor escolha local.
- Reinserir por arrependimento: priorizar quem tem maior diferença entre a melhor e a segunda melhor opção. Isso reduz a chance de consumir cedo uma posição escassa.
- Finalizar com otimização da rota e reparo de quantidades/prioridades; comparar o makespan completo.

Esses alcances e larguras são pontos iniciais de teste, não parâmetros validados. A comparação deve manter tempo total igual ao controle.

## 7. Terceira prioridade: filtrar atribuições incapazes de cumprir a meta

Uma atribuição pode ser descartada dentro de um reparo dirigido a 159 se:

- A carga mínima dos drones excede 1590; ou
- Um limite inferior válido da rota excede 156.

Isso evita gastar um B&B de quantidades em conjuntos que já não conseguem melhorar 160. Os limites devem ser usados no subproblema de melhoria, não como proibição geral de estados piores do SA, pois atravessar esses estados pode ser útil para exploração.

Não recomendo recalcular milhares de iterações de Held–Karp a cada vizinho. Usar primeiro limites baratos de carga e de árvore; reservar a otimização dos potenciais para atribuições promissoras ou repetidas, com cache. Qualquer atalho precisa preservar a validade matemática do limite.

O uso de limites sobre atribuições repetidas é especialmente relevante: a elite atual tem diversidade, mas o mecanismo de retorno do melhor pode permanecer nas mesmas bacias de 160.

## 8. Reparo por CP-SAT como opção mais forte

Uma implementação híbrida pode liberar um subconjunto de clientes e resolver um problema de viabilidade com alvo 159:

- Booleano z_j para atendimento pelo caminhão.
- Circuito do caminhão com laços para os clientes não visitados e custo ≤156.
- Escolha de exatamente uma quantidade viável de drones quando z_j=0.
- Intervalos opcionais das missões, duração tau(j,k), demanda k e capacidade cumulativa dez, todos contidos em [0,159].
- Desigualdade de carga total ≤1590, para fortalecer a busca.

Clientes fora do subconjunto têm suas atribuições fixadas. A liberdade da rota e dos cronogramas deve ser especificada explicitamente: fixar arcos demais pode excluir a cadeia que precisamos encontrar. A solução de 160 pode servir de indicação inicial, mas não satisfaz a meta 159.

Começaria por vizinhanças de aproximadamente 15–30 clientes ligados ao gargalo e aos retornos compensatórios, com limite curto adaptativo e orçamento contabilizado no total da heurística. Esses números ainda exigem calibração. O experimento desta pasta não implementou CP-SAT.

Encontrar uma solução com esse reparo demonstra melhoria; provar inviabilidade de um subconjunto só elimina aquela vizinhança. Uma prova global de ótimo 160 exigiria o problema completo, sem decisões externas indevidamente fixadas.

## 9. Ajustes complementares, com prioridade menor

- Guardar folga dos drones quando o makespan empata, para preparar trocas futuras. O refinamento B&B atual só copia de volta se o makespan cai e só seleciona estados com drones estritamente dominantes; a elite diversa já tem desempate por soma de tempos dentro da mesma atribuição.
- Incluir a solução de menor carga como incumbente adicional do B&B. Isso pode melhorar o cronograma ou a velocidade do refinamento, mas não altera o fato de que a rota condicionada vale 160.
- Rever retomadas/reaquecimento e reservar uma fase final de intensificação. A mediana recente foi 31,5 retomadas; não isolei nesta instância o efeito de alterá-las.
- Diversificar estados iniciais e atribuições, preservando o melhor. Apenas trocar a semente mantém o mesmo processo de construção inicial e o mesmo alcance dos reparos.

O primeiro investimento deve ser a qualidade do reparo conjunto. Ampliar apenas B&B, frequência de macromovimentos ou tamanho da destruição ainda não mostrou ganho nesta análise.

## 10. Critério para decidir se a mudança vale a pena

Separar duas metas: alcançar 160 com mais frequência a partir da construção inicial e encontrar valor <160 a partir de soluções boas. Um teste que começa de 160 não mede a primeira meta.

Para cada proposta, comparar com o controle em sementes previamente definidas, mesmo tempo total e validação independente. Medir frequência de 160, melhor/média, tempo até cada alvo, clientes que mudam de recurso, proporção de candidatos que passam pelos limites e origem das melhorias. Confirmar qualquer sinal em novas sementes; as três sementes desta pasta são apenas triagem exploratória.

## Arquivos e reprodução

- analisar.py / diagnostico.json: 190 certificados e características do representante.
- remocoes.json / retornos_apos_52.json: economias e cargas da troca imediata.
- limites_rota.py / limites_rota.json: limites de 1-tree com potenciais, depois recomputados em frações.
- verificar_limites.py: conferência independente por Kruskal e checagem de integralidade das coordenadas.
- obrigatorios.py / obrigatorios.json: TSP exato dos clientes obrigatórios.
- probe.c / *_probe.jsonl: refinamentos e testemunha oficial no C atual.
- busca.c / rodar_ensaios.py / manifesto_ensaios.json: busca continuada e variantes.
- validar_ensaios.py / validacao.json: verificação dos 21 certificados adicionais e resumos.
- fontes/: cópia preservada dos fontes usados.

Recalcular as análises a partir da raiz:

```powershell
python resultados/analise_165_20261028/analisar.py
python resultados/analise_165_20261028/limites_rota.py
python resultados/analise_165_20261028/verificar_limites.py
python resultados/analise_165_20261028/obrigatorios.py
python resultados/analise_165_20261028/validar_ensaios.py
```

O script de campanha impede sobrescrever seu manifesto. Para repetir os ensaios, usar outra pasta.
