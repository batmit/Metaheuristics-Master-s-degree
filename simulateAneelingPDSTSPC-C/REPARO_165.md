# Reparo dirigido e CP-SAT na 165-r-c

As propostas 1, 2, 4 e 5 estao integradas. A `main` ativa o perfil novo
somente na instancia `165-r-c`; as outras tres conservam a exploracao anterior.
`parametrosSAPadrao()` e `parametrosSAExploracao()` continuam disponiveis.
`parametrosSAFoco165()` ativa o perfil completo para chamadas pela API C.

## Executar

O ambiente `.venv-cpsat` foi criado e testado com Python 3.13 e OR-Tools
9.14.6206. Para recria-lo:

```powershell
python -m venv .venv-cpsat
.venv-cpsat/Scripts/python.exe -m pip install -r requirements-cpsat-lock.txt
.venv-cpsat/Scripts/python.exe reparo_cpsat.py --check
gcc -O2 -std=c11 -Wall -Wextra -pedantic main.c SA.c cv.c lerInstancias.c -lm -o sa_foco165.exe
./sa_foco165.exe --instancia 165-r-c --execucoes 10 --segundos 30
```

Executar a partir da raiz do projeto. Para executar somente o reparo nativo,
usar `--sem-reparo-cp`. Os caminhos do Python e do script podem ser definidos
com `--python-cp caminho` e `--script-cp caminho`. `--controle` conserva o
controle legado, anterior ao perfil de exploracao.

O ambiente Python nao e versionado. `requirements-cpsat.txt` fixa o OR-Tools;
`requirements-cpsat-lock.txt` registra tambem as dependencias usadas nos testes.

## Comportamento

1. O reparo parte do melhor global armazenado. Para melhorar 160, procura
   horizonte 159 e rota <=156. A discretizacao de quatro minutos e usada
   somente depois de verificar coordenadas inteiras, velocidade 30 e a
   matriz Manhattan correspondente. Tempos fracionarios nao suportados sao
   deixados para os operadores preexistentes.
2. As cadeias usam busca em feixe e permitem transferencias nos dois sentidos,
   inclusive locais co-localizados. Estados intermediarios que excedem a
   meta continuam candidatos a compensacao na proxima transferencia.
3. Antes de avaliar uma tentativa completa, filtros verificam carga minima
   dos drones e limite inferior de rota por 1-tree. O custo da rota atual nao
   e usado como se fosse um limite inferior. Os filtros nao restringem as
   transicoes normais do simulated annealing.
4. O reparo CP-SAT libera atribuicoes de um subconjunto de clientes e decide
   quantidades, horarios e capacidade cumulativa. Segmentos externos do
   caminhao sao contraidos em blocos: suas arestas internas ficam fixas,
   mas a ordem entre blocos e os clientes livres pode mudar.

O objetivo do modelo CP e viabilidade na meta. `INFEASIBLE` significa apenas
que a vizinhanca e os segmentos fixados nao admitem aquela meta. `UNKNOWN`
ou encerramento pelo prazo nao provam inviabilidade. Nenhum desses resultados
e uma prova de otimalidade global.

## Orcamento e falhas

Valores iniciais configuraveis em `ParametrosSA`:

| Parametro | Valor |
|---|---:|
| Estagnacao para tentar reparo | 12 blocos |
| Largura/profundidade da cadeia | 6 / 4 |
| Prazo por tentativa nativa | 0,06 s |
| Clientes livres no CP-SAT | ate 24 |
| Prazo por processo CP | ate 2,5 s |
| Chamadas CP por execucao | ate 2 |
| Parcela maxima de parede para CP | 20% |
| Parcela maxima para reparos nativos | 15% |

Os reparos respeitam o tempo restante do SA. O prazo do processo CP inclui
inicializacao de Python, importacao, montagem, solver e validacao no C.
O processo e encerrado se ultrapassar sua janela; no Windows e iniciado
sem janela visivel e sem shell. No Linux/Unix a ponte usa fork/exec.

Certificados CP sao validados no C quanto a cobertura, duracoes, capacidade,
rota e meta antes de substituir uma solucao. O cronograma e preservado;
nao e reagendado por first-fit. `cronogramaExterno` distingue um certificado
valido sem cache de prefixos: mudancas nos drones reconstroem o cronograma,
enquanto mudancas apenas na rota preservam os horarios recebidos.

Uma falha de dependencia, processo ou certificado e reportada em stderr e
no contador `errosCP`; novas chamadas CP sao desativadas naquela execucao,
preservando a melhor solucao do SA. Limite de tempo e inviabilidade local
possuem contadores separados e nao sao tratados como erro.

## Validacao realizada

- Suite C anterior e teste novo de cadeia com duas transferencias necessarias.
- CP-SAT: atribuicao conjunta, escolhas de quantidades, capacidade cumulativa,
  clientes externos fixados, prazo vencido e entradas invalidas.
- Ponte C/Python: certificado real, continuidade dos movimentos incrementais,
  Python inexistente, certificado duplicando cliente e processo excedendo prazo.
- Compilacao do executavel com `-Wall -Wextra -Werror -pedantic`.
- Comparacao exploratoria de tres partidas do zero e uma partida de 160 por
  versao, sempre com 30 s; certificados validados independentemente.

Resultados dessa comparacao:

| Partida | Versao anterior | Versao nova |
|---|---|---|
| Do zero, tres sementes | 164, 161, 172 | 164, 164, 164 |
| Da solucao 160, uma semente | 160 | 160 |

As oito chamadas CP na instancia real terminaram pelo prazo, sem erros e sem
melhoria certificada. Os reparos nativos produziram melhorias durante as
trajetorias, mas nenhuma solucao final inferior a 160. Essa amostra nao
demonstra superioridade estatistica da nova versao nem que exista 159.

Arquivos completos em `resultados/implementacao_reparo_165_20261028/`:
`testes.json`, `comparacao.json`, `comparacao_manifesto.json`, certificados
`*.jsonl`, fontes anteriores em `antes/` e scripts de reproducao. As campanhas
nao sobrescrevem seus manifestos; repetir em outra pasta.

O arquivo privado `reparo_alvo.inc` e incluido por `cv.c`, preservando os
comandos existentes de compilacao com quatro arquivos C.
