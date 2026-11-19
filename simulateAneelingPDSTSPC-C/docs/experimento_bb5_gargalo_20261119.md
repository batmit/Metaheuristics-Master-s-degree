# B&B do caminhao somente quando ele e o gargalo

## Mudanca e tratamentos

`ParametrosSA.apenasGargaloBBCaminhao=true` habilita o filtro experimental.
Requer `estagnacaoRecursoBB=5` para o acionamento periodico. O perfil padrao
continua com o mecanismo periodico desativado. No runner:

- 0: base, sem B&B periodico, com refinamento final original;
- 1: BB5 anterior, ambos os recursos apos cinco propostas sem melhora;
- 2: BB5 com filtro: chama o B&B do caminhao apenas se
  `tempoCaminhao > tempoDrones + 1e-9` no momento de acionamento.

Empates bloqueiam a chamada do caminhao: reduzir apenas sua rota nao reduziria
o makespan. O contador reinicia ao atingir cinco, mesmo quando a chamada e
bloqueada; assim o filtro nao acumula acionamentos pendentes. O contador mede
propostas sem reducao do tempo da solucao corrente apos aceitacao, como no
experimento anterior. Continua contando tambem quando o recurso nao e gargalo.

O B&B dos drones permanece igual ao da variante 1: pode atuar inclusive quando
os drones nao sao o gargalo. O filtro e apenas de entrada: uma vez chamado,
o B&B do caminhao continua minimizando a rota sem parada por meta igual ao
tempo dos drones. Nao foram adicionados outros filtros, cache ou mudancas
nas vizinhancas, temperatura ou limites de busca.

Nos dois tratamentos com refinamento periodico: 10000 nos e prazo de 20 ms
por chamada, reduzido pelo prazo restante do SA. Bound do caminhao: oito passos
de penalidades lagrangianas sobre MST residual com conexoes, 1-tree na raiz.
B&B dos drones: otimiza quantidades com ordem/regra de insercao fixas.

## Experimentos

228 execucoes realizadas, 76 triplas, todas executadas novamente no mesmo binario:

- 12 instancias x 3 sementes x 3 tratamentos com teto de 2 s (108 execucoes);
- os mesmos pares com exatamente 1000 propostas (108 execucoes);
- quatro instancias x uma semente x 3 tratamentos com teto de 30 s (12 execucoes).

As instancias sao `18-r-e`, `24-rc-c`, `30-c-c`, `67-c-c`, `99-rc-c`,
`105-rc-e`, `126-r-c`, `144-rc-c`, `154-c-c`, `165-r-c`, `181-r-e` e `200-r-e`.
Sementes: 11, 29 e 47. Confirmacao de 30 s: `24-rc-c`, `105-rc-e`,
`144-rc-c` e `200-r-e`, semente 11.

Execucao sequencial, alternando as seis permutacoes dos tratamentos. Tempos
de parede medidos dentro do runner, excluindo carga e construcao inicial.
Nos tetos de 2/30 s, a calibracao esta incluida e a parada natural pode ocorrer
antes do teto. Em 1000 propostas: temperatura inicial fixa 10, um unico nivel,
sem calibracao, macros, retomadas ou B&B final; e um teste de custo por trabalho
de SA, nao uma busca completa. As propostas divergem entre tratamentos mesmo
com a mesma semente quando os refinamentos alteram o estado.

Cada solucao final e validada independentemente em Python: cobertura unica,
distancias Manhattan, duracoes oficiais, makespan e capacidade simultanea.
Chamadas, nos e tempo de B&B medem os refinamentos periodicos, excluindo a
chamada final preexistente; esta continua incluida no tempo total.

Os dados anteriores e os recordes de producao nao sao atualizados. Resultados,
certificados, parametros, versoes e hashes estao em
`resultados/experimento_bb5_gargalo_20261119/`.

## Reproducao

```powershell
gcc -O2 -std=c11 -Wall -Wextra -pedantic -I. tests/benchmark_bb5.c SA.c cv.c lerInstancias.c -lm -o tmp/benchmark_bb5_gargalo.exe
python tests/comparar_bb5_gargalo.py
python tests/executar.py
```

Os testes do caminhao conferem a otimizacao contra enumeracao exaustiva e,
para o novo filtro, verificam bloqueio em empate, liberacao com gargalo estrito
e preservacao das chamadas periodicas dos drones.

## Resultados: 12 instancias e tres sementes

| Tratamento | Tempo medio, teto 2 s | Propostas medias, teto 2 s | Gap medio a UB2025, teto 2 s | Tempo medio, 1000 propostas |
|---|---:|---:|---:|---:|
| Base | 1.497954 s | 221102 | 27.0868% | 0.003371 s |
| BB5 anterior | 2.000153 s | 2955 | 59.8931% | 3.090928 s |
| BB5 com gargalo | 2.000160 s | 2983 | 60.1347% | 2.813381 s |

Cada comparacao abaixo tem 36 pares. Melhor/pior significa menor/maior makespan.

| Teste e comparador | Nova variante melhor | Empate | Nova variante pior |
|---|---:|---:|---:|
| Teto 2 s: nova vs base | 0 | 3 | 33 |
| Teto 2 s: nova vs BB5 anterior | 8 | 20 | 8 |
| 1000 propostas: nova vs base | 20 | 16 | 0 |
| 1000 propostas: nova vs BB5 anterior | 8 | 18 | 10 |

Com 1000 propostas, a reducao de tempo medio em relacao ao BB5 anterior foi
8.98%; a mediana das razoes pareadas de tempo nova/anterior foi 0.9160
(8.40% de reducao). A mediana nova/base ainda foi 745.66. O gap medio do teste
de 1000 propostas foi 72.6576% na base, 46.7996% no BB5 e 47.2796% no novo.
Este teste truncado mede custo por proposta e nao qualidade final de producao.

### Chamadas e tempo dos refinamentos periodicos

Totais somados das 36 execucoes de cada tratamento:

| Teste | Tratamento | Chamadas caminhao | Bloqueios pelo filtro | Tempo caminhao | Tempo drones | Fracao do tempo total nos B&B |
|---|---|---:|---:|---:|---:|---:|
| Teto 2 s | BB5 | 12249 | 0 | 51.881 s | 18.541 s | 97.80% |
| Teto 2 s | Nova | 10147 | 1990 | 51.044 s | 19.149 s | 97.48% |
| 1000 propostas | BB5 | 6629 | 0 | 80.402 s | 30.516 s | 99.68% |
| 1000 propostas | Nova | 5621 | 1015 | 71.255 s | 29.682 s | 99.66% |

No teto de 2 s, as chamadas do caminhao cairam 17.16%, mas seu tempo acumulado
caiu apenas 1.61%. Como as trajetorias e os tamanhos dos subproblemas mudam,
uma chamada evitada nao equivale a economizar o tempo medio de outra chamada.
Os bloqueios nao devem ser subtraidos das chamadas antigas: sao oportunidades
registradas na trajetoria da nova variante.

Os limites de 20 ms por chamada podem encerrar os B&B em nos diferentes entre
execucoes, mesmo com a mesma semente. Por isso estes resultados usam uma nova
execucao de todos os tratamentos, em vez de reaproveitar os tempos do estudo
anterior. As comparacoes sao exploratorias, em uma unica maquina, sem afirmacao
de significancia estatistica ou competitividade com a literatura.

## Confirmacao com teto de 30 segundos

Makespans em minutos; uma semente (11) por instancia. Todos os tratamentos com
B&B periodico usaram aproximadamente 30 s em cada caso. A base encerrou
naturalmente antes desse teto.

| Instancia | Base | BB5 anterior | BB5 com gargalo | Tempo base | Tempo nova |
|---|---:|---:|---:|---:|---:|
| 24-rc-c | 72 | 72 | 72 | 0.354 s | 30.000 s |
| 105-rc-e | 116 | 196 | 196 | 6.163 s | 30.002 s |
| 144-rc-c | 144 | 197 | 197 | 7.491 s | 30.000 s |
| 200-r-e | 289 | 291 | 332 | 11.237 s | 30.000 s |

A nova variante empatou com a anterior em tres casos e perdeu em um; contra
a base, empatou em um e perdeu em tres. Os B&B periodicos consumiram 98.89%
do tempo da nova variante nesse lote (anterior: 99.31%). Uma unica semente
por instancia nao estabelece uma diferenca estatisticamente significativa.

## Conclusao pratica

Restringir a entrada do B&B do caminhao ao gargalo economizou aproximadamente
9% do tempo medio do teste de 1000 propostas, mas nao produziu melhora
consistente de qualidade em relacao ao BB5 anterior. Com tempo limitado, a base
continuou superior nesta amostra. O filtro sozinho nao compensou a frequencia
de cinco propostas, e o mecanismo periodico permanece desativado no perfil padrao.

Dois mecanismos que ainda podem ser investigados, mas nao foram testados aqui:
encerrar a busca do caminhao assim que sua rota ficar abaixo do tempo dos drones;
e diminuir a frequencia ou limitar a fracao do tempo reservada aos B&B.

Todas as 228 solucoes finais passaram pelo verificador independente. Fontes
exatos do experimento foram preservados em `fontes/` na pasta dos resultados,
com hashes em `proveniencia.json`. Os resultados anteriores foram preservados.
Os 14 testes C e o teste de integracao passaram apos a modificacao, sem avisos
de compilacao. Os testes especificos cobrem empate, drones estritamente
dominantes e caminhao estritamente dominante.
