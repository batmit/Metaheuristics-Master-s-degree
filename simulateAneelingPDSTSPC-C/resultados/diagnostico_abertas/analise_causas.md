# Por que o SA perde qualidade nos casos ainda abertos

Data: **26/10/2026**. Diagnóstico do código atual, dos dados e de ensaios separados. Nenhum algoritmo de produção foi alterado.

## Conclusão

A perda de qualidade não decorre da ausência de prova de ótimo nem da perda de aleatoriedade. Os mecanismos medidos apontam principalmente para atribuições caminhão/drones presas em regiões locais, falta de valorização da folga em soluções empatadas, pouca diversidade de atribuição nas elites e um orçamento que praticamente não cresce com o tamanho da instância. O B&B apenas refina quantidades sob ordem fixa, portanto não corrige sozinho uma atribuição inadequada.

Há evidência direta de uma limitação: nas soluções reproduzidas de 165-r-c e 167-r-e, a carga mínima dos clientes selecionados para drones já torna impossível alcançar as referências 160 e 180 sem mudar a atribuição.

## 1. Comparação correta com as referências de 2025

|Instância|UB atual|SA|Gap frente ao UB|Base da comparação|
|---|---|---|---|---|
|67-c-c|74|76|2.70%|Melhor de 100, lote histórico|
|165-r-c|160|172|7.50%|Melhor de 100, lote histórico|
|167-r-e|180|196|8.89%|Melhor de 100, lote histórico|
|181-r-e|216|252|16.67%|Um ensaio, semente 123|

O UB de um caso aberto não é um ótimo provado. O gap acima é relativo à melhor solução publicada, não a um ótimo conhecido. Não misturar o ensaio único da 181-r-e com melhores de 100 execuções. O lote novo de quatro casos estava incompleto quando este diagnóstico foi produzido; não foi interrompido.

A percepção de bom desempenho anterior também precisa da referência atualizada: a 154-c-c alcança 72 em 92 das 100 execuções históricas, mas o ótimo atual é 68. A 126-r-c, já fechada, tem melhor SA 168 frente ao ótimo 152 (+10,53%); a 103-rc-c tem 132 frente a 120 (+10%). Portanto, casos já provados também são difíceis para a heurística. Os mesmos quatro casos abertos consumiram aproximadamente 3600 s nos logs oficiais sem fechar a prova, mas dificuldade de prova e qualidade da heurística são questões distintas.

## 2. Características dos dados

|Instância|Clientes|Drones|Elegíveis|Obrigatórios no caminhão|Elegíveis que exigem grupo k>1|Elegíveis por drone|
|---|---|---|---|---|---|---|
|67-c-c|67|10|59|8|26|5.90|
|165-r-c|165|10|152|13|58|15.20|
|167-r-e|167|11|158|9|104|14.36|
|181-r-e|181|9|160|21|107|17.78|

Na 20-c-c, usada como controle fácil, são 19 elegíveis para seis drones (3,17 por drone). Nas três maiores abertas, esse indicador vai de 14,36 a 17,78, com cerca de cinco quantidades viáveis por cliente. A quantidade de decisões de atribuição, ordem e formação cresce muito mais que o número de drones. Esse indicador descreve as alternativas da busca; não significa que todos os elegíveis devam ir para drones.

A 67-c-c tem uma dificuldade diferente: 67 clientes em apenas 15 coordenadas distintas; 59 clientes compartilham coordenadas com outros, e há um local com 14 clientes. Na solução reproduzida, retirar isoladamente qualquer um dos 33 clientes elegíveis ainda no caminhão economiza zero no percurso atual. Para eliminar uma visita pode ser necessário transferir um grupo completo, não apenas um cliente.

## 3. O modelo consegue representar as soluções publicadas

As testemunhas oficiais foram importadas com as quantidades publicadas e prioridades em ordem de início. O escalonador C atual reproduziu os makespans 74, 160, 180 e 216 nos quatro casos. Também reproduziu 43 na 20-c-c, 112 na 105-rc-e e 68 na 154-c-c. Isso afasta, nestes ensaios, uma incompatibilidade de modelo ou da regra first-fit que impedisse representar as referências. Não prova ausência de qualquer defeito possível.

## 4. O B&B não consegue consertar uma atribuição errada

|Instância|Clientes nos drones: SA / fonte|Carga mínima dos clientes do SA|LB dos drones, quaisquer quantidades|Referência|
|---|---|---|---|---|
|67-c-c|26 / 23|566|57|74|
|165-r-c|88 / 71|1624|163|160|
|167-r-e|70 / 57|1998|182|180|
|181-r-e|64 / 49|1842|205|216|

O limite usado é ceil(sum_j min_k(k × tau_j(k)) / m), condicionado ao conjunto atual de clientes dos drones. Ele vale independentemente da ordem ou do escalonamento escolhido. Na 165-r-c, 1624/10 exige pelo menos 163; na 167-r-e, 1998/11 exige pelo menos 182. Logo, mesmo um escalonamento exato de todas as quantidades e ordens não alcança 160 e 180 sem mudar os clientes atribuídos. A contagem de missões, isoladamente, não basta: é a carga mínima que fundamenta essa impossibilidade.

A 181-r-e não tem essa impossibilidade pelo limite de carga (LB condicional 205 < 216), mas a rota atual exige 252. Na 67-c-c, a rota exige 76. Nos quatro estados examinados, o caminhão fixa o makespan ou está empatado com os drones; reduzir somente tempoDrones não reduz o objetivo naquele instante.

## 5. Soluções empatadas com folga útil são subvalorizadas

O objetivo é max(tempoCaminhao, tempoDrones). O SA aceita empates, mas lhes dá zero ponto no sistema de pesos. O arquivo cheio exige makespan estritamente menor que o da pior solução; logo, uma configuração com o mesmo makespan e muito mais folga pode não entrar. O refinamento da elite também copia o B&B de volta somente quando reduz o makespan. São escolhas de intensificação, não violações de viabilidade.

Um reparo barato escolheu, para cada missão, a quantidade viável que minimiza k × tau_j(k), mantendo clientes e ordem, e reconstruiu o cronograma:

|Instância|Caminhão|Drones antes|Drones após reparo mínimo de carga|Makespan antes / depois|
|---|---|---|---|---|
|67-c-c|76|75|62|76 / 76|
|165-r-c|172|172|166|172 / 172|
|167-r-e|196|196|187|196 / 196|
|181-r-e|252|252|217|252 / 252|

Na 181-r-e, os drones caem de 252 para 217, mas a solução continua valendo 252 porque o caminhão não mudou. Essa folga poderia receber novos clientes e permitir reconstruir uma rota menor. O critério atual não garante que ela seja preservada ou recompensada. Na 67-c-c, o mesmo reparo libera 13 minutos nos drones; esse espaço pode ajudar a transferir um grupo que retire uma visita inteira do caminhão. Isso demonstra folga disponível, não garante que a sequência seguinte encontre uma melhoria global.

## 6. Pouca diversidade de atribuição dentro da elite

Nas sete trajetórias instrumentadas, os quatro slots finais tinham apenas uma atribuição distinta caminhão/drones. Os vetores eram diferentes por ordem e/ou quantidades, mas compartilhavam o mesmo conjunto de clientes por recurso. Isso limita as regiões que o B&B recebe. A observação inclui o controle fácil: portanto é uma propriedade do mecanismo, não uma causa suficiente da dificuldade dos quatro abertos.

## 7. Mais nós de B&B não foram a primeira solução

|Instância|Gatilhos de B&B no SA|Buscas efetivas|Ganhos globais via B&B|Makespan após B&B de 100 mil nós|
|---|---|---|---|---|
|67-c-c|215|5|1|76|
|165-r-c|196|6|2|172|
|167-r-e|204|8|0|196|
|181-r-e|202|1|0|252|

Os filtros escolhidos (drones dominantes, elite elegível e histórico) fazem muitos gatilhos não gerarem nova busca. Na 167-r-e, as oito buscas efetivas bateram o limite de 10 mil nós. Ainda assim, subir para 100 mil nós em cada estado final não melhorou nenhum makespan, pois o caminhão continuou sendo barreira. Em vários casos, o reparo mínimo de carga foi melhor para tempoDrones do que o resultado limitado do B&B. Isso recomenda acrescentar tal reparo como incumbente inicial barato, em vez de apenas ampliar o orçamento.

O B&B continua restrito às quantidades e à regra de inserção sob ordem fixa. Seu status de ótimo não equivale ao ótimo de toda a instância. As regras de elegibilidade do B&B foram mantidas nestes ensaios.

## 8. Orçamento e resfriamento quase iguais para tamanhos muito diferentes

|Instância|Propostas do SA|Última melhora global na fração da busca|Empates úteis sem recompensa|
|---|---|---|---|
|20-c-c|538500|62.1%|30230|
|67-c-c|543000|61.9%|44760|
|165-r-c|548000|95.7%|39222|
|167-r-e|562000|98.4%|42413|
|181-r-e|548000|99.0%|41854|

A 20-c-c recebeu 538500 propostas; as maiores receberam aproximadamente 548000 a 562000. O orçamento varia muito pouco, enquanto o espaço de decisões cresce fortemente. Nas trajetórias de 165-r-c, 167-r-e e 181-r-e, a última melhora ocorreu perto do final. Isso sugere que um orçamento adaptativo ou ciclos de perturbação e reaquecimento merecem teste, sem concluir que bastará executar por mais tempo.

Ao final das sete trajetórias, todos os pesos estavam no piso 0,1. Portanto a seleção dos tipos disponíveis voltava a ser uniforme; não havia um operador desabilitado nem desaparecimento da aleatoriedade. Por outro lado, a adaptação deixou de distinguir os tipos nessa fase. Foram observados dezenas de milhares de empates com redução de tempoCaminhao+tempoDrones que recebem zero ponto pelo critério atual.

## 9. Testes que não deram melhora imediata

Uma descida exaustiva com 2-opt e reinserção individual, mantendo os clientes no caminhão, não reduziu nenhuma das sete rotas finais. Uma troca de um cliente entre modais, com reinserção na melhor posição do caminhão e teste de todas as quantidades viáveis da missão substituta, também não melhorou os estados testados sob a ordem restante fixa. Isso é evidência de barreiras que exigem coordenação maior; não prova ótimo local em toda vizinhança possível, nem torna inúteis essas operações em outras etapas da busca.

## 10. Melhorias propostas em ordem prática

1. **Guardar e premiar folga útil nos empates.** Manter makespan como objetivo primário; desempatar as elites por folga/tempo secundário e diversidade, preservando a melhor. Evitar premiar ciclos de estados repetidos.
2. **Usar reparo por menor carga como mais um incumbente do B&B.** Comparar o recebido, o guloso de menor término já existente e o de menor k × tau. É uma heurística de construção de limite superior, não uma regra para eliminar quantidades da busca. Manter possibilidade de explorar todas as quantidades viáveis.
3. **Mover grupos e reconstruir conjuntamente.** Na 67-c-c, considerar clientes da mesma coordenada. Nas maiores, retirar/reinserir pequenos grupos próximos, rever a atribuição e reotimizar a rota. Uma troca simples de um cliente isolado não foi suficiente no ensaio feito.
4. **Diversificar atribuições da elite e estados iniciais.** Distinguir diversidade de rota de diversidade dos conjuntos atribuídos. Sortear entre construções e elites estruturalmente diferentes.
5. **Ajustar esforço por tamanho/estagnação e reaquecer após perturbações.** Comparar por mesmo tempo ou número de avaliações, com múltiplas sementes e registro de melhor, média e tempo até a referência.
6. **Não priorizar apenas mais B&B ou uma nova semente.** Ambos preservam o conjunto errado de clientes se não houver diversificação. A aleatoriedade atual existe; a vizinhança e os critérios determinam quais regiões e folgas são efetivamente aproveitadas.

## Alcance e validação

Esta é uma análise de mecanismos com sete trajetórias escolhidas, não um estudo estatístico de ablação. As sementes de 67-c-c, 165-r-c e 167-r-e reproduzem melhores históricos de 100; a 181-r-e usa só a semente 123. Os controles são 20-c-c, 105-rc-e e 154-c-c. Os experimentos extras não foram incorporados ao lote.
As cópias instrumentadas não acrescentam sorteios. Os certificados completos de 20-c-c e 105-rc-e reproduziram exatamente os arquivos anteriores. Os hashes de SA.c, cv.c, SA.h, cv.h e lerInstancias.c permaneceram iguais aos da auditoria anterior. As 34 soluções de ensaios foram conferidas independentemente com frações exatas: cobertura, percurso, matriz de tempos, duração e uso simultâneo da frota.
Para afirmar o ganho de cada proposta, o próximo passo é comparar versões em várias sementes com orçamento igual, alterando um componente por vez. Este diagnóstico não garante encontrar valores abaixo dos UBs publicados; eles podem já ser ótimos mesmo sem prova disponível.

## Arquivos de apoio

- medicoes.json e resumo_diagnostico.json: todas as métricas e limites condicionais.
- execucoes_instrumentadas.txt: trajetórias de referência e do SA, com contadores.
- polimento.txt: reparo mínimo de carga e B&B de 100 mil nós.
- trocas.txt: ensaio de troca entre modais.
- caracteristicas_60.json: estatísticas estruturais de todo o conjunto.
- A referência oficial está na revisão anterior: resultados/revisao_literatura_2025/.
