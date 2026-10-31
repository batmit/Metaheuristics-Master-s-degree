# Implementacao das propostas 1, 2, 4 e 5 — 165-r-c

Data: 28/10/2026. Implementacao de reparo com meta global, cadeias de
transferencias, filtros por limites inferiores e reparo CP-SAT integrado.

As fontes anteriores foram preservadas em `antes/`. A nova `main` usa o
perfil completo na 165-r-c; o comportamento das demais instancias conserva
o perfil anterior de exploracao. A fabrica C `parametrosSAExploracao()`
continua servindo para comparacoes com o algoritmo anterior.

## Comparacao de 30 segundos

Tres sementes para partidas do zero e uma partida da solucao de 160. Tres
processos concorrentes, sementes pareadas, mesmo orcamento de parede.
A comparacao e exploratoria: poucas sementes, concorrencia e custo variavel
por operador impedem uma conclusao estatistica geral.

| Inicio | Versao | Resultados | Media |
|---|---|---|---:|
| Construcao inicial | Anterior | 164,161,172 | 165,67 |
| Construcao inicial | Nova | 164,164,164 | 164,00 |
| Solucao 160 | Anterior | 160 | 160,00 |
| Solucao 160 | Nova | 160 | 160,00 |

Oito certificados passaram pelo validador independente com fracoes exatas.
Nao houve resultado abaixo de 160. O CP-SAT foi chamado oito vezes, sem
erro; essas chamadas terminaram pelo limite de tempo e nao produziram nova
solucao na instancia real. Os testes pequenos demonstraram que o modelo
e a ponte alteram atribuicoes/quantidades e devolvem certificados viaveis.

O codigo registra separadamente propostas nativas, podas de carga/rota,
melhorias, chamadas/sucessos/inviabilidades/limites/erros de CP e tempos.

## Arquivos

- `comparacao_manifesto.json`: sementes, fontes/hashes, retornos e orcamento.
- `comparacao.json`: estatisticas e quantidade de certificados validados.
- `controle_*.jsonl` e `novo_*.jsonl`: rotas e cronogramas completos.
- `verificar.py` e `testes.json`: compilacao e suite de testes.
- `comparar.py` e `benchmark.c`: campanha com fontes anteriores congeladas.
- `antes/`: fontes da versao anterior.

Uso, parametros, dependencias e limites de interpretacao estao em
`REPARO_165.md`, na raiz do projeto. Nenhuma melhora de qualidade foi
declarada como comprovada a partir desta amostra.
