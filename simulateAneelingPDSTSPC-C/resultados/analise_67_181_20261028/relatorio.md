# Por que o SA não alcançou 74 na 67-c-c e 216 na 181-r-e

Análise de 28/10/2026. Código de produção preservado; os ensaios desta pasta usam executáveis e cópias experimentais separados. Tempos das soluções em minutos; orçamentos dos programas em segundos.

## Resultado principal

Os estados finais examinados estão presos principalmente na escolha de quais clientes ficam no caminhão e na necessidade de coordenar essa escolha com a capacidade dos drones. Na 67 há uma barreira exata e um problema específico na meta usada pela troca bidirecional. Na 181 há evidência consistente de dificuldade de atribuição e rota, mas não uma prova de ótimo do TSP condicionado aos clientes do caminhão.

Não encontrei uma incompatibilidade que torne as referências irrepresentáveis: as testemunhas oficiais foram reproduzidas pelo escalonador C atual e verificadas independentemente. Isso não prova que o programa esteja livre de qualquer defeito.

| Instância | Referência | Melhor SA recente | Caminhão/drones do certificado representativo | Referência: caminhão/drones |
|---|---:|---:|---:|---:|
| 67-c-c | 74 | 76 | 76 / 71 | 72 / 74 |
| 181-r-e | 216 | 220 | 220 / 220 | 216 / 216 |

O certificado da 67 é a semente 1793192902, do lote de dez de 28/10. O da 181 é a semente 1793050747, do primeiro lote de dez execuções de 30 s: entre os melhores de valor 220, foi escolhido pelo desempate por menor semente. Outros certificados empatados podem ter tempos dos drones diferentes. As conclusões condicionais abaixo dizem respeito ao estado explicitamente indicado.

## 1. O que foi efetivamente examinado

- Dez certificados recentes da 67, todos com makespan 76, e os sete conjuntos distintos de coordenadas do caminhão presentes nesses certificados.
- Todos os 190 certificados recentes da 181, dos lotes de 10, 20, 50, 100 e dez execuções. Os hashes de SA.c, cv.c e respectivos cabeçalhos/entrada coincidem com o código atual nos cinco lotes; ver compatibilidade_lotes.json.
- As duas testemunhas oficiais de 2025, incluindo cobertura, rota Manhattan, retorno ao depósito, duração pela matriz e uso simultâneo dos drones.
- Para cada melhor representativo: reparo de quantidades por menor carga, B&B de até 100 mil nós, descida 2-opt/reinserção com 20 reconstruções de rota, 500 propostas de troca bidirecional e 500 reconstruções parciais. Cada proposta parte do mesmo estado-base; não são trajetórias completas de SA.
- Mais 500 trocas por instância em uma cópia que altera apenas a meta de carga; ver seção 3. Não incorporada à produção.
- Doze execuções instrumentadas de dez segundos: duas instâncias, três sementes, configuração atual versus mesma configuração com retomadas desligadas. Ensaio exploratório, sem poder para concluir eficácia estatística.

Foram validados 200 certificados históricos recentes e 42 certificados adicionais publicados pelos ensaios desta pasta. As milhares de propostas intermediárias não foram todas exportadas para validação externa: seus contadores vêm do C; as melhores soluções exportadas passaram pelo verificador independente com frações exatas.

## 2. A 67-c-c tem uma barreira de atribuição demonstrada

### 2.1 Reordenar os mesmos clientes não basta

Calculei a rota ótima por Held–Karp, agrupando clientes na mesma coordenada. Isso é válido porque não há tempo de serviço por cliente neste modelo. Nos sete conjuntos distintos de locais dos dez finais recentes, o ótimo condicionado do caminhão é **76**.

Portanto, para esses dez estados, nem um TSP exato que apenas reorganize os clientes atuais do caminhão conseguiria alcançar a referência 74. É necessário mudar a atribuição entre os recursos.

Há também uma discretização importante: as coordenadas são inteiras, a distância é Manhattan e a velocidade é 30 km/h. Um circuito fechado tem comprimento Manhattan par; multiplicar por dois para converter em minutos produz tempos do caminhão múltiplos de quatro. Para makespan ≤74, o caminhão precisa terminar em **72 ou menos**. O passo estrutural exigido é 76→72, não uma simples economia de dois minutos na rota.

### 2.2 O movimento necessário retira um endereço inteiro

No certificado representativo, todos os 35 clientes elegíveis que ainda estão no caminhão têm economia individual de remoção igual a zero. Clientes no mesmo local escondem o benefício da transferência até sair o último cliente que obriga a visita.

Examinei cada endereço cujos clientes atuais do caminhão podem ser todos transferidos para drones:

| Local | Clientes a retirar do caminhão | Ótimo da rota após retirada | LB dos drones após transferência unilateral |
|---|---:|---:|---:|
| (0,6) | 12 | 72 | 84 |
| (1,6) | 8 | 76 | 70 |
| (4,9) | 1 | 76 | 53 |
| (6,9) | 6 | 76 | 57 |

O único desses movimentos que reduz a rota ao patamar necessário transfere 12 clientes de uma vez, mas seu conjunto resultante nos drones exige pelo menos 84 minutos. Isso é impossível para uma meta 74, independentemente da ordem e das quantidades escolhidas.

O limite usado é:

`LB(D) = ceil(sum(j em D) min(k viável) [k * tau(j,k)] / número_de_drones)`.

Ele mede trabalho mínimo em drone-minutos dividido pela frota. É uma condição necessária; não garante que o escalonamento caiba quando o limite é baixo.

### 2.3 A solução oficial mostra uma troca coordenada viável

O certificado atual usa 24 clientes nos drones; o oficial usa 23. Portanto, a diferença não é simplesmente colocar mais clientes nos drones. Para passar da atribuição atual à testemunha oficial, 12 clientes vão para drones e 13 voltam ao caminhão: **25 decisões de recurso diferentes**.

Os retornos podem ser inseridos nos locais já visitados sem desfazer a rota de 72. Reproduzi essa inserção por menor acréscimo, que continuou em 72; o cronograma oficial tem 74. Essa é uma testemunha de outra região viável, não uma exigência de que toda solução boa copie exatamente a atribuição oficial.

## 3. Um detalhe do reparo atual dificulta justamente essa transição

O algoritmo já contém transferência de grupos e troca bidirecional. Não é correto atribuir o problema atual à ausência desses operadores. A dificuldade está em como a troca escolhe o reparo.

Em cv.c, trocarRecursosCV calcula:

`alvo = min(makespan_atual, custo_da_rota_após_retirada)`

e devolve clientes dos drones ao caminhão enquanto:

`carga_mínima > frota * alvo`.

Após retirar o endereço (0,6), a rota vale 72. Isso faz o reparo buscar carga ≤720 drone-minutos. Entretanto, o conjunto de drones da testemunha oficial tem carga mínima **726** e é viável em 74 minutos: dispõe de 740 drone-minutos.

| Decisão | Capacidade de carga usada |
|---|---:|
| Meta do reparo quando a rota vale 72 | 720 |
| Carga mínima da atribuição oficial | 726 |
| Capacidade da frota no horizonte 74 | 740 |

Assim, no caminho concreto que mantém a rota 72 durante os retornos, o reparo continua retirando clientes dos drones mesmo depois de chegar a uma atribuição com a carga da referência. Ele tenta equilibrar os drones com o caminhão, embora o objetivo permita que os drones terminem depois do caminhão.

Isso é uma limitação da construção heurística, não uma restrição do modelo nem uma prova de que todos os caminhos do SA estão bloqueados. Outros retornos podem aumentar a rota antes do polimento, e outras sequências de movimentos podem chegar à referência.

Também há alcance insuficiente da reconstrução genérica para essa mudança em uma única chamada: 5–15% de 67, arredondados para cima, removem **4 a 11 clientes**. O grupo crítico atual tem 12. A troca especializada consegue retirar o local inteiro, mas enfrenta o reparo descrito acima.

Testei uma cópia com alvo de carga igual a makespan_atual−1, coerente com a integralidade dos objetivos e sem usar a referência como regra. Em 500 propostas novas partindo desse estado, ela não reduziu o makespan. Portanto, corrigir somente o alvo **não demonstrou ser suficiente**: escolha dos retornos, prioridade das missões e reinserção gulosa continuam relevantes.

## 4. Na 181-r-e, o gargalo combina rota e atribuição

Dos 190 finais recentes:

- **190/190** têm tempo do caminhão acima de 216.
- **181/190** também têm tempo dos drones acima de 216.
- **120/190** têm LB de carga mínima dos drones acima de 216. Nesses estados, nenhuma combinação de quantidades ou prioridades pode atingir a referência sem alterar os clientes atribuídos.
- Há 180 execuções com duração registrada ≤35 s, cujo melhor também é 220. As pausas longas não explicam sozinhas a barreira observada.

No melhor representativo, os drones têm 50 clientes, contra 49 na testemunha. Porém, os conjuntos diferem em **33 clientes**: 16 mudariam para drones e 17 para caminhão. As contagens próximas escondem atribuições muito diferentes.

A carga mínima desse estado é 1922 drone-minutos. O teto `ceil(1922/9)` é 214; portanto o limite não prova impossibilidade de horizonte 216 nesse melhor específico. Mas a margem de carga é de apenas **22 drone-minutos**: `9*216−1922`. A testemunha oficial tem carga mínima 1820, liberando 102 drone-minutos em relação ao conjunto atual. Mesmo uma troca aparentemente pequena pode consumir toda a margem se os retornos não compensarem os novos clientes.

Existem 160 clientes elegíveis, dos quais 107 só aceitam grupos de pelo menos dois drones. O problema exige coordenar cliente, quantidade e sincronização, além de reconstruir a rota. Isso distingue uma alteração de atribuição de uma simples permutação de missões.

No representante, remover individualmente 108 clientes elegíveis do caminhão não economiza tempo; apenas dois têm economia positiva. A rota também não melhorou com a descida 2-opt/reinserção nem com as 20 reconstruções mantendo os mesmos clientes. Esse resultado é **heurístico**: não provei que 220 seja o ótimo do TSP condicionado da 181.

## 5. Por que mais B&B dos drones não resolveu

O B&B mantém conjunto de clientes e prioridade do vetor, varia quantidades e agenda pela regra de primeiro intervalo viável. Mesmo uma solução ótima desse subproblema não altera a rota do caminhão.

| Caso e intervenção | Caminhão | Drones | Makespan |
|---|---:|---:|---:|
| 67, estado recebido | 76 | 71 | 76 |
| 67, quantidades de menor carga | 76 | 57 | 76 |
| 67, B&B de 100 mil nós | 76 | 67 | 76 |
| 181, estado recebido | 220 | 220 | 220 |
| 181, quantidades de menor carga | 220 | 218 | 220 |
| 181, B&B, 30.433 nós e término ótimo do subproblema | 220 | 218 | 220 |
| 181, melhor das 500 reconstruções propostas | 220 | 214 | 220 |

O B&B da 67 atingiu o limite de nós e seu resultado foi pior para os drones que o reparo barato por carga mínima. Ele não usa esse reparo entre seus incumbentes iniciais. Isso justifica testar um incumbente adicional, mas não promete melhora do objetivo quando o caminhão permanece em 76.

No SA, o refinamento periódico nem seleciona elites com `tempoDrones <= tempoCaminhao`, e só copia o refinamento de volta se o makespan cair. Isso limita a preservação de folga que poderia ser útil em uma transferência seguinte. A elite diversa já usa soma de tempos como desempate dentro de uma mesma atribuição; essa melhoria atual não elimina a limitação específica do refinamento B&B.

## 6. Os operadores existem, mas o reparo ainda é limitado

Os macromovimentos têm probabilidade nominal de 1% cada. Não faltam tentativas em termos absolutos: nos certificados recentes, a mediana de avaliações é 1.284.057 na 67 e 243.491,5 na 181. O custo maior por avaliação na 181 reduz as oportunidades no mesmo orçamento de parede.

A inserção da reconstrução testa a melhor posição de caminhão, mas apenas três prioridades de inserção nos drones: início, fim e uma posição aleatória. Ela testa as quantidades viáveis para essas posições e decide cliente a cliente pelo makespan parcial, seguido da soma dos tempos. Não resolve conjuntamente a composição final de clientes, quantidades e prioridades. Uma escolha boa para uma solução parcial pode prejudicar a combinação final.

As 500 trocas e 500 reconstruções independentes a partir de cada melhor não reduziram o caminhão em nenhum dos dois casos. Algumas liberaram bastante capacidade dos drones, mas não produziram a transição conjunta necessária. Isso apoia a hipótese de reparo pouco eficaz nesse entorno; não demonstra ótimo local em toda a vizinhança nem inutilidade desses operadores em outros estados.

## 7. Temperatura e retomadas: mecanismo relevante, mas causa não isolada

Com limite de tempo, maxRetomadas=3 não limita o número de retomadas: o cabeçalho documenta que o limite numérico vale quando não há prazo. A mediana dos certificados recentes foi 127,5 retomadas na 67 e 19 na 181. Não se trata de uma violação escondida do parâmetro, mas de uma escolha de funcionamento que merece revisão experimental.

Depois de 20 blocos sem melhora global, há mudança de elite/reconstrução e reaquecimento para pelo menos 15% de T0. Em uma instância rápida, isso pode ocorrer muitas vezes antes de acabar o prazo.

Ensaio curto em três sementes por variante, dez segundos cada, com três processos concorrentes:

| Caso | Configuração | Resultados | Mediana de retomadas | Temperatura mediana amostrada no último quinto |
|---|---|---|---:|---:|
| 67 | Atual | 76,76,76 | 52 | 7,98 |
| 67 | Retomadas desligadas | 76,76,76 | 0 | 0,024 |
| 181 | Atual | 232,232,224 | 2 | 0,053 |
| 181 | Retomadas desligadas | 232,232,224 | 0 | 0,024 |

Na 67 atual, a média de `exp(-4/T)` nas amostras do último quinto foi aproximadamente 45%. Isso é a probabilidade teórica de aceitar uma piora de quatro minutos nas temperaturas amostradas; não é a taxa observada de aceitação de todos os movimentos. Sem retomadas, foi praticamente zero.

O resfriamento efetivo é claramente diferente, mas os resultados empataram nas três sementes. Não há evidência aqui para anunciar que desligar retomadas resolve o problema, muito menos para atribuir a falha apenas à temperatura.

## 8. Comparação com o método original e com a referência posterior

O artigo de Nguyen e Hà (2023) descreve uma metaheurística de **ruin-and-recreate**, com operadores específicos de remoção/inserção e avaliação por ordenação topológica para sincronizar drones. A página oficial permite confirmar esses elementos. O PDF integral não ficou acessível nesta consulta: não alego ter auditado seus parâmetros internos, código-fonte ou todos os operadores. Fonte: [Transportation Science, DOI 10.1287/trsc.2022.1192](https://pubsonline.informs.org/doi/10.1287/trsc.2022.1192).

A Tabela 2 de Montemanni, Dell'Amico e Corsini, publicada em 2024, reproduz resultados desse método: **76 na 67** e **224 na 181**, melhores de 30 execuções. Logo, nosso 76 empata aquele resultado histórico da 67; nosso 220 supera o histórico 224 da 181. Os alvos 74 e 216 pertencem ao pacote posterior. Fonte: [preprint, Tabela 2](https://arxiv.org/html/2307.03523v1#S4.T2). A tabela também foi conferida na imagem local do PDF.

Os logs oficiais de 2025 examinados nesta pasta identificam CP-SAT 9.12, um Circuit e um Cumulative com intervalos opcionais, prazo de 3.600 s e 16 workers. Terminam FEASIBLE, com LB/UB 73/74 e 215/216. São decisões integradas de atribuição, percurso e uso dos drones, com orçamento diferente do SA de 30 s. Não obtive código-fonte desse solver; o ZIP de resultados contém resultados e logs, não os fontes. Fonte do pacote: [ORLab, dados e resultados PDSTSP-c/PDSOP-c](https://orlab.com.vn/download/data-and-results-of-pdstsp-c-and-pdsop-c/).

A diferença central para o nosso B&B não é apenas ter mais nós: o solver oficial decide conjuntamente variáveis que nosso B&B deixa fixas. A descrição resumida de ruin-and-recreate também não torna a reconstrução atual uma reprodução fiel do algoritmo original.

## 9. O que eu priorizaria agora

1. **Reparo dirigido por makespan-alvo, com decisões de atribuição livres.** Preservar o melhor global, liberar um local inteiro e selecionar conjuntamente retornos, quantidades e prioridades. Usar horizonte de melhoria do objetivo, sem exigir igualdade entre tempos do caminhão e dos drones. A troca simples de alvo testada nesta pasta não bastou.
2. **Para a 67, tratar endereços como unidades de decisão.** Retirar todos os clientes que obrigam uma visita, mesmo que exceda 15%, e reinserir com avaliação do conjunto completo. O estado representativo exige remover 12, além de retornos compensatórios.
3. **Para a 181, separar dois subproblemas diagnósticos.** Procurar TSP ≤216 mantendo a atribuição para verificar se a rota sozinha é melhorável; em paralelo, reparar um subconjunto de clientes com atribuição livre. Um fracasso com tempo limitado não prova inviabilidade. O ideal é um submodelo CP-SAT com certificado de status e limites, não apenas aumentar o B&B atual.
4. **Melhorar o reparo guloso.** Testar inserção por arrependimento, mais prioridades de drones e recolocação conjunta de pequenos grupos. Medir ganho por segundo, pois mais posições e quantidades também encarecem cada proposta.
5. **Preservar folga útil no refinamento.** Incluir menor carga como incumbente adicional do B&B e admitir cronogramas melhores mesmo com makespan empatado, respeitando critérios de elite e evitando ciclos. Isso ajuda uma transferência futura; não reduz sozinho uma rota que já fixa o objetivo.
6. **Revisar retomadas somente com ablação controlada.** Testar um período final sem reaquecimento, limite efetivo de retomadas e esforço em função de tempo/custo. O ensaio curto atual não mostrou ganho ao desligá-las.

Não recomendo outro lote grande idêntico como próximo passo. Ele acrescenta amostras, mas não testa a razão estrutural da estagnação. Também não há base para afirmar probabilidade zero de atingir as referências apenas porque os lotes anteriores falharam.

## 10. Limitações e reprodução

- Não foi provada a otimalidade global de 74 ou 216; continuam sendo UBs nas fontes examinadas.
- A impossibilidade de apenas reordenar o caminhão foi provada nos dez finais recentes da 67. Na 181, o teste de rota foi heurístico.
- As contagens de 190 execuções não constituem um experimento controlado de parâmetros; existem diferentes sementes, lotes e pausas. Foram usadas para descrever gargalos, e os tempos prolongados foram identificados separadamente.
- Os ensaios de 500 propostas partem de um estado escolhido; os de temperatura têm três sementes e dez segundos, com concorrência. Servem para localizar mecanismos, não para estimar ganho médio definitivo.
- Nenhuma alteração desta pasta foi aplicada a SA.c ou cv.c de produção; hashes conferidos após os ensaios.

Arquivos principais:

- analisar.py e diagnostico.json: agregação dos 200 certificados, limites de carga e Held–Karp da 67.
- probe.c, *_best.txt, *_official.txt e *_probe.jsonl: reparos, rotas e testemunhas no C atual. As missões oficiais são fornecidas em ordem crescente de início; usar uma ordem arbitrária de clientes muda a prioridade do escalonador.
- cv_alvo_experimental.c: cópia que muda apenas o alvo da troca, não o modelo.
- experimentos.py, SA_trace.c, benchmark_trace.c e experimentos_manifesto.json: instrumentação e comparação exploratória de retomadas.
- validar_ensaios.py e validacao_ensaios.json: validação independente dos 42 certificados adicionais e resumo térmico.
- official_67-c-c.log e official_181-r-e.log: logs primários extraídos do results.zip interno ao pacote de 2025.
- compatibilidade_lotes.json e fontes/: hashes e fontes preservados.

Para recalcular a análise e validar os certificados salvos, a partir da raiz do projeto:

```powershell
python resultados/analise_67_181_20261028/analisar.py
python resultados/analise_67_181_20261028/validar_ensaios.py
```

O script de experimentos impede sobrescrever o manifesto; para repetir a campanha, usar outra pasta e manter o lote atual preservado.
