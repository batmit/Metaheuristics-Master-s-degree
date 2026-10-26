# Auditoria detalhada da instância 105-rc-e

**Data:** 26/10/2026.

**Parecer:** 116 é viável; a fonte oficial de 2025 reporta ótimo 112. Gap atualizado: +3,57%.

## Conclusão executiva

A solução de 116 minutos é viável. O resultado melhora o UB de 120 do artigo de 2024 em 3,33%.

O pacote oficial ORLab de 2025 registra OPTIMAL, Makespan=112 e Makespan Bound=112; a testemunha de 112 foi validada.

116 fica 3,57% acima de 112. Esta auditoria não reexecutou a prova de otimalidade do solver.

## Modelo e dados

Um caminhão faz um circuito fechado no depósito; drones idênticos realizam missões deposito-cliente-deposito.

Uma missão coletiva ocupa k drones simultaneamente durante toda a sua duração. Quantidades inviaveis da matriz são proibidas.

Não ha recuperacao de drones pelo caminhão, precedência entre o vetor completo de missões, nem recarga acumulativa entre missões no benchmark.

A validação usa os tempos completos oficiais, sem dividir tau por dois ou somar tempos extras sem respaldo nos dados.

## Verificações

Cobertura unica, retorno ao depósito, unidades, matriz viável, durações, capacidade em eventos e por minuto: aprovados.

Identidades D1..D12 e sincronização das missões coletivas: aprovadas.

Regra first-fit independente e reprodução C: exatas. Seis corrupcoes artificiais foram rejeitadas.

## Rota

0 -> 97 -> 99 -> 96 -> 103 -> 102 -> 101 -> 14 -> 41 -> 12 -> 58 -> 59 -> 61 -> 55 -> 53 -> 64 -> 63 -> 60 -> 23 -> 4 -> 44 -> 38 -> 21 -> 91 -> 84 -> 89 -> 37 -> 51 -> 82 -> 93 -> 88 -> 94 -> 92 -> 83 -> 90 -> 1 -> 31 -> 6 -> 43 -> 67 -> 22 -> 45 -> 69 -> 78 -> 76 -> 74 -> 32 -> 72 -> 70 -> 71 -> 77 -> 80 -> 75 -> 68 -> 79 -> 73 -> 85 -> 86 -> 81 -> 87 -> 26 -> 25 -> 34 -> 54 -> 62 -> 56 -> 11 -> 65 -> 66 -> 57 -> 28 -> 5 -> 48 -> 0

Tempo exato: 116 min; distancia Manhattan total: 58. Nenhum 2-opt ou reinsercao individual reduz a rota atual.

## Drones

33 missões, com ocupação simultânea máxima de 12. Último retorno: cliente 20 em t=115.

1326 drone-minutos ocupados. Todas as missões receberam identidades fisicas D1..D12 sem sobreposicoes.

## Carga e limites

Carga realizada 1326, limite de carga fixo 111. Carga mínima para os mesmos clientes 1271, limite 106.

Utilizacao global 95,26%; folga total 66 drone-minutos. Limites condicionais não provam otimalidade global.

O caminhão e o gargalo da testemunha de 116; refinar somente os drones não reduz esse valor.

## Referência atualizada

Pacote ORLab de 2025, log results/logs/tsp/105-rc-e.log: status OPTIMAL, Makespan=112, Makespan Bound=112.

A solução publicada foi reconstruida como DAG de sincronização e validada em 112. Usa 79 clientes no caminhão e 26 nos drones.

A carga 1344=12x112 é uma igualdade para aquela solução, não uma prova global independente. O objetivo codificado 1266496 não deve ser confundido com minutos.

## Estatísticas

100 execucoes: melhor 116, pior 140, média 124,67, mediana 124 e desvio amostral 4,7611.

2 resultados abaixo de 120, 34 iguais e 64 acima; nenhum igual a 112. CPU total registrada 723,88 s.

Melhores nas execucoes 82 e 92. Tempo isolado não fundamenta comparação de desempenho com o solver oficial.

## Parecer final

Viabilidade de 116 aprovada. Comparação com 120 e histórica; frente ao ótimo reportado 112 o gap e 3,57%.

Main e logs do lote foram preservados para reproducibilidade. A lista de instâncias sem ótimo baseada em 2024 precisa ser revista com fontes atualizadas.

O script verificar_105.py e independente do avaliador C e utiliza aritmética racional exata.

## Todas as missões de 116

|Prioridade|Cliente|k|Inicio|Fim|Duração|Drones identificados|
|---:|---:|---:|---:|---:|---:|---|
|1|19|4|0|46|46|1,2,3,4|
|2|16|1|0|26|26|5|
|3|40|3|0|14|14|6,7,8|
|4|49|1|0|18|18|9|
|5|47|1|0|23|23|10|
|6|7|2|0|9|9|11,12|
|7|30|2|9|41|32|11,12|
|8|15|1|14|26|12|6|
|9|42|1|14|33|19|7|
|10|18|2|18|29|11|8,9|
|11|24|2|26|57|31|5,6|
|12|95|1|23|29|6|10|
|13|36|1|29|55|26|8|
|14|29|1|29|50|21|9|
|15|33|2|33|65|32|7,10|
|16|27|1|41|62|21|11|
|17|98|1|41|47|6|12|
|18|9|1|46|70|24|1|
|19|50|2|46|78|32|2,3|
|20|52|2|47|77|30|4,12|
|21|3|2|55|81|26|8,9|
|22|46|2|57|84|27|5,6|
|23|39|1|62|82|20|11|
|24|13|1|65|86|21|7|
|25|104|2|70|77|7|1,10|
|26|100|2|77|84|7|1,4|
|27|10|2|77|111|34|10,12|
|28|20|2|78|115|37|2,3|
|29|17|2|81|102|21|8,9|
|30|35|4|84|112|28|1,4,5,6|
|31|2|2|86|114|28|7,11|
|32|8|1|102|113|11|8|
|33|105|1|102|108|6|9|

## Todos os arcos do caminhão

|Arco|Origem|Destino|Tempo|Saida|Chegada|
|---:|---:|---:|---:|---:|---:|
|1|0|97|2|0|2|
|2|97|99|0|2|2|
|3|99|96|0|2|2|
|4|96|103|0|2|2|
|5|103|102|0|2|2|
|6|102|101|0|2|2|
|7|101|14|8|2|10|
|8|14|41|6|10|16|
|9|41|12|6|16|22|
|10|12|58|2|22|24|
|11|58|59|0|24|24|
|12|59|61|2|24|26|
|13|61|55|0|26|26|
|14|55|53|0|26|26|
|15|53|64|0|26|26|
|16|64|63|0|26|26|
|17|63|60|0|26|26|
|18|60|23|2|26|28|
|19|23|4|6|28|34|
|20|4|44|0|34|34|
|21|44|38|2|34|36|
|22|38|21|4|36|40|
|23|21|91|2|40|42|
|24|91|84|2|42|44|
|25|84|89|0|44|44|
|26|89|37|0|44|44|
|27|37|51|0|44|44|
|28|51|82|0|44|44|
|29|82|93|0|44|44|
|30|93|88|0|44|44|
|31|88|94|0|44|44|
|32|94|92|0|44|44|
|33|92|83|0|44|44|
|34|83|90|2|44|46|
|35|90|1|2|46|48|
|36|1|31|4|48|52|
|37|31|6|4|52|56|
|38|6|43|4|56|60|
|39|43|67|2|60|62|
|40|67|22|2|62|64|
|41|22|45|0|64|64|
|42|45|69|2|64|66|
|43|69|78|0|66|66|
|44|78|76|2|66|68|
|45|76|74|0|68|68|
|46|74|32|0|68|68|
|47|32|72|0|68|68|
|48|72|70|0|68|68|
|49|70|71|0|68|68|
|50|71|77|0|68|68|
|51|77|80|0|68|68|
|52|80|75|0|68|68|
|53|75|68|0|68|68|
|54|68|79|0|68|68|
|55|79|73|2|68|70|
|56|73|85|6|70|76|
|57|85|86|0|76|76|
|58|86|81|2|76|78|
|59|81|87|0|78|78|
|60|87|26|4|78|82|
|61|26|25|6|82|88|
|62|25|34|2|88|90|
|63|34|54|2|90|92|
|64|54|62|0|92|92|
|65|62|56|0|92|92|
|66|56|11|0|92|92|
|67|11|65|0|92|92|
|68|65|66|0|92|92|
|69|66|57|2|92|94|
|70|57|28|2|94|96|
|71|28|5|2|96|98|
|72|5|48|12|98|110|
|73|48|0|6|110|116|

## Fontes e rastreabilidade

[Artigo, 2024](https://doi.org/10.1016/j.cor.2023.106514). Tabela 2: LB=109, UB=120.
[Pacote ORLab 2025](https://orlab.com.vn/download/data-and-results-of-pdstsp-c-and-pdsop-c/). Log results/logs/tsp/105-rc-e.log.

O log registra execucao em 17/04/2025, CP-SAT 9.12.4550, 16 trabalhadores, tempo 125,257 s, OPTIMAL, Makespan=112 e Makespan Bound=112.
O campo objective=1266496 é uma codificação do objetivo; não é o makespan.

### Hashes SHA-256

- `105-rc-e.txt`: `1829bb9a0298dcbe4fe4c5d0637362edbe0458bf78e9744fd5a44ff7c75e833d`
- `catalog_105.csv`: `54f7df5e6b264556c38761dfe3799b44fcde9e608f05d2f37c5474feab405d71`
- `execucoes_105_originais.txt`: `a1f9f0a5acd06573d6fa406b46a25ba3f1c6f3edcc515ba90a3b6083dbbacf55`
- `original_bks_105.txt`: `5fb2b9dd400dcfcf6a85eefc4fbae9d9c83c3c30149e35fa9fe0d5488ea0d927`
- `orlab_2025_105.log`: `4ef295b53d88d8e20b057802c297d3f90734e301a6927c81c91fedb5156ae971`
- `solucao_registrada.txt`: `ab0e3bd95480d56057cb2d418a5c0dd66a1bfb2e2dca0db9a3c7ee03c0a7c344`
- `tau_kj_105-rc-e.txt`: `f2c4fa3046614246e32cb0c78272fb8062d0b5bb5716804ab911dbba1a6857f4`

### Reprodução

```powershell
python resultados/105-rc-e/verificar_105.py
```

As restrições energeticas são verificadas pela admissibilidade na matriz oficial, sem recalculo físico independente. A prova de otimalidade reportada pelo solver de 2025 não foi reexecutada; a viabilidade da sua testemunha foi.
