# Sementes e mudancas recentes — 07/11/2026

A tabela agora mostra apenas instancia, literatura, encontrado, gap e tempo
de execucao. Estado e tempo da literatura foram retirados da apresentacao.

## Historico examinado

Comparacao com o ultimo commit disponivel, `adb0ac7` (31/10/2026), e com os
fontes atuais. As alteracoes locais posteriores nao possuem commits
intermediarios: nao e possivel datar cada uma apenas pelo Git.
As mudancas dos dois ultimos atendimentos concentraram-se no executor
Python, persistencia, tabela e validacao da comparabilidade, sem alterar
os operadores C, o resfriamento ou a calibracao.

- O executor antigo usava `--seed` com padrao 42; o atual usa o horario.
- A primeira repeticao preserva a formula `base + 10*n + S`. As repeticoes
  adicionais agora somam 1000003 por repeticao, com reducao para 32 bits.
- Alpha=0,99, beta=1,1, gamma=0,95, 500 tentativas, temperatura final 0,01,
  gatilho BB apos cinco tentativas e teto de 100000 nos continuam iguais.
- A construcao e os cinco operadores de vizinhanca continuam iguais ao
  commit. Foram retirados caches/flags redundantes de drones; o BB teve
  lacos com uma unica iteracao simplificados, preservando k fixo.
- `SA.c` passou a recalcular integralmente antes de publicar uma melhora.
  Isso pode alterar arredondamentos e custo de execucao; nao se pode
  prometer trajetorias identicas para todas as instancias apenas pelo diff.
- A leitura recebeu validacoes; os seis arquivos usados nos testes
  (tres instancias e tres matrizes) coincidem byte a byte com o commit.
- Antes, a tabela do lote era publicada apos a conclusao; agora tambem
  publica a construcao inicial e melhorias intermediarias. Comparar uma
  dessas primeiras linhas com um resultado final antigo produz impressao
  de piora. `Encontrado` continua sendo o melhor acumulado no registro.
- O reinicio do registro, documentado anteriormente no README, significa
  que melhores acumulados nao reaparecem automaticamente em um registro
  novo. Nao houve reinicio, importacao ou substituicao neste diagnostico.

## Experimentos com controle

Compilados separadamente o commit e os fontes atuais com GCC `-std=c11 -O2`
no WSL. Mesmas entradas locais, sementes efetivas, compilador e plataforma.
16 combinacoes de instancia/semente, executadas nas duas versoes: 32 buscas,
mais duas repeticoes exatas. Todas terminaram normalmente; todos os
checkpoints foram recalculados pelo validador independente.

Os 16 pares produziram o mesmo melhor objetivo nas duas versoes. As duas
repeticoes da mesma semente tambem reproduziram o melhor objetivo.
Isso nao e uma prova universal de ausencia de regressao; e evidencia
contra uma regressao nesses casos.

| Instancia/S | Semente efetiva | Melhor anterior | Melhor atual | Gap atual |
| --- | ---: | ---: | ---: | ---: |
| 88-c-e/2 | 924 (base antiga 42) | 84 | 84 | 0,00% |
| 88-c-e/2 | 1794064650 (salva na execucao recente) | 100 | 100 | 19,05% |
| 68-rc-c/5 | 727 (base 42) | 56 | 56 | 27,27% |
| 68-rc-c/5 | 728 (base 43) | 52 | 52 | 18,18% |
| 68-rc-c/5 | 729 (base 44) | 48 | 48 | 9,09% |
| 91-r-c/2 | 954 (base 42) | 120 | 120 | -3,23% |
| 91-r-c/2 | 1794065152 (execucao recente) | 104 | 104 | -16,13% |
| 53-r-e/5 | 577 (base 42) | 112 | 112 | 21,74% |

A variacao de semente explica a piora reproduzida da 88-c-e e pode tambem
melhorar resultados, como na 91-r-c. Nao ha uma semente universalmente
melhor. A media de poucas sementes nao constitui avaliacao estatistica
completa do SA.

## Comparacao de resultados salvos

Foi preservada uma fotografia de 110 configuracoes presentes tanto no
registro atual quanto em `results/bb5_all_unproven/comparison.csv` do commit.
Comparacao sempre no mesmo par instancia/S, usando os mesmos UBs:

| Medida | Anterior | Atual |
| --- | ---: | ---: |
| Media do gap bruto nas mesmas 110 configuracoes | 3,5098% | 3,2885% |
| Media excluindo as tres referencias incompativeis da 126-rc-e (107 casos) | 2,7044% | 2,4770% |

Houve 14 resultados piores, 79 iguais e 17 melhores. Nessa fotografia nao
ha aumento geral do gap medio; ha pioras locais importantes. O registro
pode continuar mudando durante as execucoes do usuario.

O relatorio antigo de 3,2461% corresponde a 120 configuracoes. Comparar
essa media com apenas as primeiras linhas de um novo lote nao controla
o conjunto de instancias. Tambem existem no Git resultados de um lote
`latest` cortado em 30 segundos, com 8 workers, media 729,19% e retomadas;
ele nao representa o mesmo experimento completo e nao foi usado como
baseline principal.

## Como reproduzir

Lote com a mesma semente base do experimento antigo, no mesmo ambiente:

```sh
python3 scripts/executar_benchmark.py --seed 42
```

Reproducao individual da semente efetiva antiga da 88-c-e/S=2:

```sh
python3 scripts/executar_benchmark.py instances/benchmark/large/88-c-e.txt instances/drone_times/tau_kj_88-c-e.txt 2 924
```

O padrao por horario foi mantido; nenhuma escolha de semente foi alterada
para favorecer resultados. Para investigar variabilidade, use varias
repeticoes e compare mesmas instancias, plataforma e criterio de parada.
Com limite externo de tempo, validacao e gravacao em disco tambem podem
afetar quantos passos terminam antes do corte.

Artefatos: `output/comparacao_sementes/experimentos.json`, `caso88.json`,
`historico.json` e diffs dos fontes. Script reproduzivel:
`scripts/comparar_sementes.py`. Os experimentos nao gravam em `results`.
