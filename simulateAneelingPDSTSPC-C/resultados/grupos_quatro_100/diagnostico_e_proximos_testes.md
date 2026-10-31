# Diagnóstico atualizado e próximos testes — PDSTSP-c

Análise em 26/10/2026, baseada no código de produção congelado em `fontes/`, nos 800 certificados do lote completo, em ensaios adicionais de rota e nas testemunhas oficiais ORLab. Esta etapa não introduziu outras alterações no algoritmo.

A evidência mais forte aponta para dificuldade em coordenar atribuição de clientes, rota, quantidades e prioridades dos drones. A aleatoriedade existe. Entretanto, o novo lote mostra que aumentar o alcance de uma transferência isolada não resolve essa coordenação e pode piorar a qualidade média.

## 1. Resultados que fundamentam a análise

Foram executadas 100 sementes por versão e por caso, com o resfriamento completo: 400 execuções com grupos e 400 controles sem grupos. Mesmas sementes dentro de cada comparação. Todos os 800 certificados passaram pela validação independente com frações exatas: cobertura, rota, retorno ao depósito, duração pela matriz e capacidade simultânea, incluindo uma atribuição física viável dos drones.

|Caso|Melhor sem grupos|Melhor com grupos|Média sem grupos|Média com grupos|UB publicado|Gap do melhor com grupos|
|---|---:|---:|---:|---:|---:|---:|
|67-c-c|76|76|80,32|76,12|74|2,70%|
|165-r-c|172|172|186,20|189,11|160|7,50%|
|167-r-e|200|196|216,97|217,67|180|8,89%|
|181-r-e|240|240|256,24|257,68|216|11,11%|

Na 67-c-c, grupos venceu em 95 sementes, empatou em cinco e não perdeu; chegou a 76 em 97/100 execuções, contra 2/100 no controle. Na 165-r-c, venceu 29, empatou 14 e perdeu 57. O intervalo bootstrap de 95% da diferença média nessa instância é [1,19; 4,65] minutos, indicando piora nesta comparação. Nas outras duas, os intervalos incluem zero: houve melhora do melhor da 167-r-e, mas não evidência consistente de melhora média. As análises estatísticas são exploratórias, sem correção por múltiplas comparações.

Nenhuma execução atingiu ou superou o UB publicado. UB é uma solução viável, não um ótimo provado. Os intervalos publicados são 73–74, 156–160, 179–180 e 215–216.

## 2. A barreira de 76 da 67-c-c foi localizada com mais precisão

Para o melhor certificado com grupos, semente 1793032030, calculei por programação dinâmica exata de Held–Karp a melhor rota do caminhão mantendo seus clientes. Clientes co-localizados podem ser agrupados neste cálculo porque o modelo não tem tempo de serviço. São dez coordenadas fora do depósito e 5.120 estados de programação dinâmica.

O resultado exato é **76**, igual à rota encontrada. Para um melhor certificado do controle, com nove coordenadas, o ótimo condicional também é 76. Isso prova uma limitação específica desses dois estados: nenhuma reordenação dos mesmos clientes do caminhão pode alcançar 74. Não é uma prova de ótimo da instância nem de todos os finais do lote.

Examinei ainda cada coordenada do caminhão cujos clientes podem ser todos atendidos por drones. No melhor com grupos:

- Tirar os 14 clientes de (0,6) reduz a rota de 76 para 72.
- Entretanto, a carga mínima dos drones passa a 830 drone-minutos. Com dez drones, qualquer escalonamento desses clientes exige pelo menos **83 minutos**, independentemente das quantidades e da ordem.
- As outras remoções de uma coordenada elegível inteira não reduzem o ótimo condicional da rota abaixo de 76.

Portanto, uma transferência unilateral desse local não alcança 74. É preciso liberar capacidade dos drones, mudar outras atribuições e/ou explorar outro conjunto de locais simultaneamente. A solução oficial possui 23 clientes nos drones, contra 14 neste certificado. Eles compartilham apenas nove: a atribuição publicada troca 14 clientes para drones e cinco no sentido inverso. Essa comparação é uma testemunha de viabilidade de outra região, não uma prescrição de que toda solução boa deva copiar esses clientes.

Essa é a razão para priorizar **trocas simultâneas nos dois sentidos**, com avaliação após a reconstrução completa. O operador atual move grupos em apenas um sentido por proposta.

## 3. Nas maiores, o diagnóstico antigo de excesso de carga ficou parcialmente superado

|Caso|Finais com LB condicional dos drones > UB: sem/com grupos|Clientes nos drones no melhor com grupos / referência|LB condicional inteiro desse melhor|Caminhão / drones desse melhor|
|---|---:|---:|---:|---:|
|165-r-c|97/100 → 24/100|79 / 71|156|172 / 172|
|167-r-e|86/100 → 13/100|53 / 57|166|196 / 196|
|181-r-e|81/100 → 14/100|50 / 49|206|240 / 239|

O limite é `sum_j min_k(k*tau[j][k]) / frota`, condicionado aos clientes escolhidos para drones. Para metas inteiras pode-se tomar o teto. É condição necessária, não suficiente: não captura toda a sincronização.

Os grupos reduziram muito a incidência de conjuntos comprovadamente incompatíveis com o UB. Porém, nos melhores novos, o limite já está abaixo ou igual à referência e o caminhão continua fixando o makespan. O problema deixou de poder ser descrito simplesmente como “clientes demais nos drones”. Na 181-r-e, por exemplo, as contagens 50 e 49 são próximas, mas a atribuição dos clientes difere em 29 posições e a rota continua em 240, frente a 216 na testemunha oficial.

Também tentei, nos oito melhores certificados (duas versões × quatro casos), descida 2-opt e dez reconstruções de vizinho mais próximo, seguidas de 2-opt, mantendo o conjunto do caminhão. Nenhuma melhorou a rota. Para os três casos grandes isso é evidência heurística, sem prova de ótimo do TSP condicionado. Na 67-c-c, o resultado exato acima elimina essa dúvida para os dois estados analisados.

## 4. O que o código e as fontes permitem concluir

**Representação e avaliação.** Importei novamente as decisões oficiais no escalonador C atual: ele reproduziu 74, 160, 180 e 216. Também reproduziu os controles 43, 112 e 68. Não foi encontrada uma incompatibilidade que impeça representar as referências; isso não prova ausência de qualquer defeito possível. O lote e os ensaios adicionais não evidenciaram um erro de cálculo do makespan.

**B&B.** Ele mantém os clientes e a prioridade das missões, varia quantidades e usa inserção no primeiro intervalo viável. O filtro periódico exige `tempoDrones > tempoCaminhao`. Nos quatro melhores novos, os drones estão empatados ou abaixo; portanto esses estados não são elegíveis para esse refinamento. Mesmo um B&B ilimitado dessas quantidades não reduziria o tempo do caminhão. “Ótimo” no retorno desse B&B é condicional ao seu subproblema, não à instância inteira.

**Grupos.** Para drones, o operador sorteia quantidades viáveis e insere o grupo como bloco numa posição aleatória. Para caminhão, usa melhor inserção dos clientes retornados. Ele não busca conjuntamente trocas inversas, a melhor composição de quantidades e novas prioridades antes de testar a aceitação. Grupos e movimentos individuais compartilham a pontuação do tipo de transferência. A piora da 165-r-c não identifica qual detalhe é responsável: demanda ablação específica.

**Temperatura.** A calibração usa `gerarVizinhoAvaliado`, sem a configuração de grupos ou pesos usada depois pelo SA, e parte da solução inicial só com caminhão. Busca 95% de aceitação contando também movimentos neutros e melhores. As temperaturas medianas observadas foram aproximadamente 498 na 67-c-c e 663 nas maiores. Isso é um motivo para testar calibração na distribuição real de movimentos e de deltas positivos; não demonstra, sozinho, temperatura inadequada.

**Elite.** A comparação principal é somente makespan; o arquivo cheio rejeita empates com a pior. Nas sete trajetórias instrumentadas de um diagnóstico anterior, os quatro slots finais repetiam a mesma atribuição de recursos. Isso não foi medido novamente nos 800 ensaios atuais e não deve ser generalizado a eles. Além disso, a elite não funciona como uma população de quatro trajetórias ativas: a busca volta à melhor quando o refinamento reduz o melhor makespan. Diversificar o arquivo sem usar seus estados para reconstruir a busca pode ter pouco efeito.

**Aleatoriedade.** A semente vem do relógio na main, e os testes registram sementes distintas. Os pesos têm piso positivo e há sorteios de operadores, clientes, posições e quantidades. Não encontrei um mecanismo que torne todas as execuções determinísticas entre sementes. Os mesmos valores finais são compatíveis com atração recorrente para mínimos locais. A pontuação adaptativa merece avaliação: aceitações piores recebem ponto e empates recebem zero; em sete trajetórias antigas, todos os pesos chegaram ao piso.

## 5. Diferença para as abordagens da literatura

Nguyen e Hà (2023) apresentam uma metaheurística de **ruin-and-recreate**, com remoção e inserção específicas e avaliação por ordenação topológica para a sincronização. Confirmei esses elementos no resumo oficial da editora. O PDF completo não ficou acessível nesta consulta; não atribuo a ele parâmetros ou operadores detalhados que não pude verificar. A proposta de reconstrução abaixo é uma adaptação a testar, não uma reprodução desse método.

Montemanni, Dell’Amico e Corsini (2024) formulam conjuntamente circuito do caminhão, atribuições, formações e sincronização, além da desigualdade de carga já incorporada ao projeto. O nosso B&B aplica essa desigualdade num subproblema muito mais restrito. Assim, usar o mesmo limite não oferece o mesmo poder de busca global.

As referências atuais vêm do pacote ORLab de 2025. Seus logs e planilha identificam CP-interval; os logs exibem `Circuit`, intervalos opcionais e `Cumulative`, CP-SAT 9.12, limite de 3.600 segundos e 16 workers. Isso documenta uma busca integrada com um orçamento muito diferente do nosso ciclo de poucos segundos. Não identifiquei nesta consulta um artigo completo que permita reproduzir exatamente esse modelo de 2025; não o confundo com o modelo de fluxo do artigo de 2024.

## 6. Testes prioritários

|Prioridade|Teste proposto|Hipótese e sinal de sucesso|
|---|---|---|
|1|Troca simultânea caminhão ↔ drones, incluindo local inteiro|Superar a barreira demonstrada da 67: liberar carga e remover uma visita na mesma proposta. Procurar rota ≤72 e drones ≤74, sem usar clientes oficiais como regra.|
|2|Reconstrução ruin-and-recreate com reinserção conjunta|Retirar uma fração variável, inicialmente 5–15%, alternando escolha aleatória, proximidade e contribuição ao gargalo. Reinserir em ambos os recursos, comparando posições e quantidades. Procurar novas atribuições que reduzam os dois tempos.|
|3|Reparo exato pequeno com decisões de atribuição livres|Liberar inicialmente 8–20 clientes e decisões locais da rota/cronograma, usando CP-SAT por 0,05–0,5 s como ponto inicial de teste. Medir se há melhorias onde apenas variar quantidades falha. Considerar grupos maiores quando uma coordenada inteira exigir.|
|4|Prioridade e quantidade dos drones ajustadas juntas|No SA, testar reinserção de missão, blocos e reconstrução parcial com várias quantidades antes da aceitação. Preservar o B&B de ordem fixa conforme a especificação atual. Medir ganho de transferência após reparo e custo por melhoria.|
|5|Frequência e crédito próprios para grupos|Comparar grupos desligados, probabilidade atual e probabilidades menores, com pesos separados para cada direção e tamanho. Recuperar a média da 165 sem perder a consistência da 67.|
|6|Calibração e esforço alinhados à busca real|Amostrar estados mistos e os operadores ativos; testar aceitação inicial de deltas positivos, por exemplo 0,6/0,8/0,9. Comparar execuções sustentadas e retomadas em 30/60/300 s, por tempo igual.|
|7|Diversidade útil da elite, conectada à reconstrução|Medir distância entre atribuições e reiniciar/reconstruir a partir de elites diferentes após estagnação. Manter o melhor makespan e tratar folga como secundária. Ganho deve aparecer na busca, não somente na aparência dos quatro vetores.|

Na prioridade 1, selecionar um local pela economia efetiva de rota; escolher retornos dos drones pelo alívio de carga frente ao custo de inserção no caminhão; reparar as quantidades e a prioridade; avaliar somente a solução completa. Manter uma parcela uniforme de sorteios para preservar exploração. O limite inferior serve para descartar uma tentativa incapaz de cumprir a meta do reparo; não deve eliminar permanentemente estados piores que o SA pode atravessar.

Na prioridade 2, testar melhor inserção e inserção por arrependimento (diferença entre melhores alternativas), com desempates e escolha parcialmente aleatórios. Isso difere do operador atual de transferir um bloco num único sentido e das retomadas genéricas já ensaiadas.

Na prioridade 3, separar inicialmente dois diagnósticos: escalonamento exato com atribuição fixa e reparo com atribuição livre. Se apenas o segundo melhorar, confirma a necessidade de ampliar decisões; se o primeiro melhorar muito, o escalonamento também merece esforço. Um submodelo com decisões externas fixadas continua sendo um reparo condicional e não uma prova global.

## 7. O que não repetir sem uma hipótese nova

Folga, diversidade simples, reparo mínimo de carga, inicializações e retomadas já foram ensaiados na 67 em cópias experimentais. As combinações confirmadas em 100 sementes novas não mostraram ganho adicional consistente sobre grupos. Isso não elimina as ideias, mas impede anunciá-las novamente como melhorias demonstradas. Recompensar folga sozinho não resolve a troca bidirecional localizada agora.

Aumentar só o B&B também tem prioridade baixa: em ensaios anteriores, 100 mil nós nos estados finais não reduziram o makespan. Mais sementes podem revelar soluções melhores, mas não corrigem por si o alcance da vizinhança. Não é possível inferir probabilidade zero de alcançar a literatura com base nas falhas observadas.

## 8. Protocolo de avaliação

1. Manter o controle atual congelado, com grupos ligados/desligados identificados. Testar uma mudança por vez nas quatro instâncias.
2. Fazer triagem em 30 sementes e confirmar somente propostas promissoras em 100 sementes novas; registrar o conjunto de testes para evitar escolher somente resultados favoráveis.
3. Usar o mesmo tempo por execução, contabilizando reconstrução, calibração, B&B e qualquer solver. Registrar também avaliações e nós; igual número de iterações não iguala custo entre operadores.
4. Medir melhor, média, mediana, percentis, frequência de atingir UB, tempo até cada alvo e diferenças pareadas. Melhor isolado e melhora média são resultados diferentes.
5. Registrar origem de cada melhoria, capacidade liberada, número de clientes que mudaram de recurso, diversidade real da elite e cronograma antes/depois.
6. Validar os certificados independentemente. Comparar combinações somente depois de verificar seus componentes, usando novas sementes.

O primeiro objetivo é reproduzir com regularidade os UBs 74/160/180/216. Só depois a busca por valores inferiores terá uma base experimental mais forte. Eles podem já ser ótimos, apesar da ausência de prova na fonte examinada.

## Fontes e arquivos reproduzíveis

- [Nguyen e Hà, 2023 — resumo oficial](https://pubsonline.informs.org/doi/10.1287/trsc.2022.1192).
- [Montemanni, Dell’Amico e Corsini — preprint do artigo publicado em 2024](https://arxiv.org/html/2307.03523v1).
- [ORLab — pacote de dados e resultados de 2025](https://orlab.com.vn/download/data-and-results-of-pdstsp-c-and-pdsop-c/).
- `relatorio.md`, `comparacao.json`, `manifesto.json`, `execucoes.csv`: lote completo e resultados.
- `diagnosticar.py`, `diagnostico_novo.json`: diagnósticos de atribuição, carga, rota e TSP exato da 67.
- `verificar_referencias.c`, `referencias_no_codigo_atual.txt`: reprodução das testemunhas no C atual.
- `../diagnostico_abertas/analise_causas.md`: diagnóstico histórico, cujos números não substituem os novos.
- `../experimento_melhorias_67/relatorio.md`: ablações e combinações anteriores.
