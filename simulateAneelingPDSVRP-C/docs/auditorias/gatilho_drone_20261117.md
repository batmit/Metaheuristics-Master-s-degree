# Experimento: BB apos estagnacao dos drones — 17/11/2026

## Decisao

Nao promover a candidata; manter o gatilho anterior.
SA.c identico byte a byte ao anterior: True.
Nenhuma versao experimental foi instalada no executavel principal.

## Alteracao testada

Original: apos cinco tentativas sem melhora do makespan global, tenta refinar
a melhor solucao com BB, preservando as verificacoes de missoes existentes e
branchBoundAplicado.

Candidata: conta apenas tentativas que alteram a quantidade de drones de uma
missao ou transferem cliente entre caminhao e drones. Compara o tempo dos drones
do candidato ao da solucao corrente, antes da aceitacao. Uma reducao zera o
contador; uma tentativa sem reducao o incrementa; movimentos somente dos
caminhoes nao contam. Ao atingir cinco, aplica o mesmo refinamento BB na melhor
solucao e zera a contagem. Conta tambem candidatos rejeitados pelo SA. Nao usa
um recorde historico de tempo dos drones, pois o estado sem missoes tem tempo
zero. Isso mede estagnacao local observada, nao prova de otimalidade.

O escalonamento guloso continua necessario para avaliar cada candidato; somente
o gatilho do BB foi modificado. Permaneceram identicos: dados, matriz Manhattan,
tempos oficiais, movimentos, calibracao, resfriamento, meta dos caminhoes,
limite de 100.000 nos e alvo do BB (melhor solucao).

## Protocolo

180 configuracoes, tres sementes pareadas por configuracao, duas versoes:
1.080 execucoes. Limite de 2 segundos por execucao, seis processos
concorrentes. As duas versoes de cada par rodam sequencialmente, alternando
a ordem entre configuracoes/repeticoes. Semente = 42 + 10*n + S + 1000003*rep,
rep = 0,1,2. Nenhuma solucao anterior e fornecida. Resultados nao atualizam
results/best.json. Fontes isolados e executaveis preservados no diretorio do teste.

Criterio registrado antes dos resultados: adotar somente se a variacao media
percentual do makespan for negativa, o tempo medio dos drones nao aumentar e o
teste de sinais unilateral sobre medias por configuracao resultar p < 0,05.
Configuracoes da mesma instancia compartilham dados; o teste e uma triagem
exploratoria, nao prova estatistica de superioridade independente.

## Resultados

| Indicador | Original | Candidata |
| --- | ---: | ---: |
| Makespan medio, minutos | 87.742593 | 87.757407 |
| Tempo medio dos drones, minutos | 16.250000 | 16.051852 |
| Execucoes completas | 218 | 217 |

- Medias por configuracao: 4 melhoras, 5 pioras,
  171 empates no makespan.
- Por semente pareada: 9 melhoras, 10 pioras,
  521 empates.
- Variacao media percentual do makespan (candidata-original): 0.023003%.
- Variacao media do tempo dos drones: -0.198148 minutos.
- Teste de sinais unilateral, p: 0.746094.
- Pares com ambas as execucoes completas: 214.
- Tempo de parede da campanha: 285.1 segundos.
- Certificados/checkpoints validados independentemente: 3176.

## Validacao e limites

Verificacao final da versao preservada: scripts/verificar_projeto.py passou
com 29 testes Python, 90.000 vizinhos em 180 configuracoes, 63 conjuntos
conferidos por oraculo exaustivo e 120 certificados salvos validados.
Nos 214 pares em que ambas as execucoes terminaram, houve 8 melhoras, 5 pioras
e 201 empates. Esse recorte nao elimina vies de selecao por tempo e nao reverte
a decisao: o conjunto completo nao demonstrou beneficio global consistente.

Compilacao C11 estrita com avisos como erros nas duas versoes. Testes do contador
cobrem cinco tentativas sem melhora, reinicio por melhora, ignorar movimentos
de caminhao e transferencias caminhao-drone. Todos os checkpoints emitidos sao
validados pelo avaliador Python quanto a cobertura, tempos e capacidade.

O tempo dos drones isolado nao mede qualidade global: pode cair porque clientes
voltaram aos caminhoes. O makespan e a medida primaria. O limite curto, carga
do sistema e encerramentos por tempo introduzem variacao; os resultados nao
demonstram o comportamento com orcamentos longos. Tempos computacionais de
execucoes interrompidas nao representam tempo para completar o SA. Contadores
BB so existem nas execucoes completas; comparar somas brutas enviesaria a analise.

## Artefatos

output/experimento_gatilho_drone: protocol.json, summary.json,
additional_metrics.json, paired.csv (540 pares), comparison.csv (180 configuracoes),
runs.jsonl (certificados), SA_original.c e SA_drone.c.

Reproducao: python scripts/experimentar_gatilho_drone.py
Relatorio: python scripts/relatar_gatilho_drone.py

## Diferencas por configuracao

| Instancia | S | Media original | Media candidata | Variacao (%) |
| --- | ---: | ---: | ---: | ---: |
| 15-rc-c | 3 | 32.667 | 33.333 | 2.041 |
| 23-r-c | 3 | 53.333 | 52.000 | -2.500 |
| 25-r-e | 2 | 104.000 | 102.667 | -1.282 |
| 25-rc-e | 2 | 81.333 | 78.333 | -3.689 |
| 27-rc-c | 3 | 61.333 | 60.000 | -2.174 |
| 50-r-e | 2 | 133.333 | 136.000 | 2.000 |
| 50-r-e | 5 | 109.333 | 110.667 | 1.220 |
| 68-rc-c | 4 | 68.667 | 70.333 | 2.427 |
| 68-rc-c | 5 | 54.667 | 58.000 | 6.098 |
