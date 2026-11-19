# Simulated Annealing para o PDSVRP-C

O programa minimiza o instante em que todos os caminhões e drones terminam suas entregas. A saída normal contém somente uma tabela: instância (com a quantidade S de caminhões), valor da literatura, melhor valor validado do algoritmo, gap percentual e tempo de execução. No início aparece apenas o cabeçalho, sem listar resultados salvos; cada configuração acrescenta uma única linha ao encerrar, mesmo quando não houve melhora. As melhorias intermediárias são validadas e salvas sem aparecer na tabela. Com processos paralelos, as linhas seguem a ordem de encerramento. Para consultar os resultados salvos, use `--list`.

## Compilar e executar

Compilação automática, usando o GCC do ambiente atual e todos os módulos C:

```sh
# Linux / WSL
python3 scripts/compilar.py
./pdsvrp_sa --list
```

No Windows, use `python scripts/compilar.py` e `.\pdsvrp_sa.exe --list`.
Compilar somente `main.c` deixa referências sem definição para as funções dos
outros módulos. O script inclui todos os fontes e `-lm`, e funciona mesmo
quando chamado de outro diretório.

Windows / PowerShell (GCC e Python 3 no PATH):

```powershell
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 main.c lerInstancias.c cv.c drone_bb.c SA.c -o pdsvrp_sa.exe -lm
.\pdsvrp_sa.exe
```

Linux / WSL (GCC e Python 3 instalados):

```sh
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 main.c lerInstancias.c cv.c drone_bb.c SA.c -o pdsvrp_sa -lm
./pdsvrp_sa
```

Execute o algoritmo na raiz do projeto. O lançador procura `python3` primeiro no Linux. No Windows, procura `python`, `python3` e `py -3`. Recompile sempre que alterar os fontes C. O executor exige um binário nativo: ELF `pdsvrp_sa` no Linux/WSL ou PE `pdsvrp_sa.exe` no Windows. Ele não usa o executável Windows como alternativa no WSL.

Sem argumentos, executa uma repetição das 120 configurações sem ótimo comprovado. As 60 configurações já comprovadas são ignoradas pelo SA. O padrão não tem limite de tempo: as linhas da tabela aparecem somente ao encerrar cada configuração, sem listar o histórico no início. Erros continuam no stderr, com código de saída diferente de zero.

```powershell
.\pdsvrp_sa.exe --list
.\pdsvrp_sa.exe --repetitions 3
.\pdsvrp_sa.exe --limit 1 --seconds 10 --workers 1
```

No Linux, substitua `.\pdsvrp_sa.exe` por `./pdsvrp_sa`. `--list` apenas valida os resultados salvos e mostra a tabela. `--limit` limita configurações abertas; `--seconds` limita o tempo por configuração; `--workers` define os processos paralelos. Um corte de tempo conserva o melhor checkpoint já validado. Também é possível chamar `python3 scripts/executar_benchmark.py` no Linux ou `python scripts/executar_benchmark.py` no Windows.

Execução individual, com semente opcional, usando os arquivos oficiais do catálogo:

```sh
./pdsvrp_sa instances/benchmark/large/50-r-e.txt instances/drone_times/tau_kj_50-r-e.txt 2 42
```

O gap é `100 * (encontrado - literatura) / literatura`. `--` indica ausência de solução do algoritmo, inclusive em casos ignorados. A coluna Encontrado mostra o melhor certificado acumulado, que pode ter sido obtido em uma execução anterior. Não se copia o valor da literatura para essa coluna.

`N/C` na coluna Gap indica uma referência comprovadamente incompatível com os dados locais: seu valor é menor que um limite inferior calculado diretamente das entradas. Na `126-rc-e`, o cliente 46 está em (14,14), não admite drones e exige no mínimo `2*(14+14)/30*60 = 112` minutos de caminhão. Assim, os UBs 100, 84 e 76 (S=3,4,5) não são alcançáveis com essas entradas. O registro conserva o valor publicado, o limite local e o cliente que o comprova. Os gaps das demais configurações continuam numéricos; passar essa verificação necessária não prova que todos os dados da literatura sejam idênticos.

Em diretórios Windows/OneDrive, bloqueios temporários podem impedir a substituição do JSON. O executor tenta novamente até oito vezes, mantendo a gravação atômica. Se o bloqueio persistir, conserva o arquivo anterior, deixa uma cópia completa `best.<identificador>.tmp` para recuperação e informa seu caminho no erro. Não apague o arquivo anterior para contornar o erro. No WSL, é possível manter o registro no sistema de arquivos Linux:

```sh
mkdir -p "$HOME/pdsvrp-results"
cp -n results/best.json "$HOME/pdsvrp-results/best.json"
python3 scripts/executar_benchmark.py --results "$HOME/pdsvrp-results/best.json"
```

Use o mesmo `--results` ao consultar com `--list`. O bloqueio acompanha o diretório do registro escolhido. Evite executar simultaneamente Windows e WSL sobre o mesmo registro: seus mecanismos de bloqueio não são garantidamente interoperáveis.

`Execucao (s)` mede tempo real com relógio monotônico, desde o lançamento do worker até seu encerramento, incluindo leitura, comunicação e validação dos checkpoints. A linha de encerramento mostra a duração total. `--list` mostra o tempo da última execução registrada, que pode ser diferente da execução que obteve o melhor acumulado. Todas as repetições ficam em `runs` no JSON, com semente, número da repetição e status (concluída, prazo, cancelamento ou erro). Resultados antigos sem medição mostram `--`.

A tabela contém somente instância, melhor literatura, encontrado, gap e tempo de execução. O histórico no JSON continua guardando o status de cada execução. Os metadados de tempos da literatura ficam disponíveis em `references/literature_times.json`, sem coluna na tabela.

Sem `--seed`, a semente base vem do horário atual. Para repetir o padrão antigo do lote, use `--seed 42` no mesmo ambiente. A semente efetiva de cada configuração é `(base + 10*clientes + S + 1000003*repeticao) % 2**32`, com repetição começando em zero. Assim, `--seed 42` no lote não significa que todos os workers recebem 42: a `88-c-e/S=2` recebe 924. Na chamada individual com semente explícita, o worker recebe exatamente o valor informado. Windows e Linux podem produzir sequências diferentes de `rand()` com o mesmo número; para comparar versões, mantenha plataforma, compilador, dados e sementes iguais.

## Algoritmo e validação

A construção inicial usa inserção gulosa de clientes distantes e 2-opt. O SA combina troca, transferência entre recursos, alteração de k drones, 2-opt e reinserção. A cada temperatura faz 500 tentativas e aceita pioras com probabilidade `exp(-delta/T)`. Padrões: alpha=0,99, beta=1,1, gamma=0,95 e temperatura final=0,01. A temperatura inicial é calibrada a partir de 10.

Os drones são escalonados pelo primeiro intervalo viável. Após cinco tentativas sem melhora global, o Branch and Bound refina a ordem e os horários da melhor solução, com k fixo e teto de 100.000 nós. Ele para também quando os drones terminam até o tempo dos caminhões. Atingir esses limites não prova ótimo global.

Cada melhor solução publicada é recalculada integralmente em C e conferida por uma implementação independente em Python antes de ser salva em `results/best.json`. São exigidos cobertura exata dos clientes, retorno implícito ao depósito, número correto de rotas, pares cliente/k permitidos, durações corretas, horários finitos e não negativos, capacidade simultânea da frota e makespan consistente. A validação permanece ativa com `python -O`.

O registro usa substituição atômica, sincronização e bloqueio entre executores. Resultados iguais ou piores não substituem o melhor salvo. O arquivo `.execution.lock` pode permanecer no disco: o bloqueio é liberado pelo sistema quando o processo termina.

`Ctrl+C` interrompe a execução, encerra os workers e preserva os melhores já
salvos. A saída tem código 130 e uma mensagem curta no stderr, sem traceback.
Pode haver intervalos sem novas linhas durante a calibração ou quando não há
melhoras. Para uma execução curta, use `--limit 1 --seconds 10`.

```sh
python3 scripts/verificar_projeto.py
```

No Windows, use `python`. Esse comando compila os testes em diretório temporário, confere os dados contra o ZIP oficial, verifica 90.000 vizinhos nas 180 configurações, compara BB com enumeração exaustiva, testa trajetórias do SA, persistência, cancelamento e saída somente em tabela. Uma execução integrada de um segundo usa registro isolado; o lote completo não é iniciado e os melhores reais não são alterados. O resumo fica em `output/verificacao.json`.

O registro foi reiniciado: `results/best.json` começa vazio. Na próxima execução, cada configuração recebe seu primeiro certificado validado; depois, somente valores estritamente menores substituem o melhor salvo. Configurações comprovadas continuam ignoradas pelo SA. Os valores da literatura permanecem como referência, e resultados ausentes aparecem como `--`.

## Estrutura e documentação

### Experimento com distância euclidiana

`python scripts/experimentar_metricas.py --seconds 2 --repetitions 3 --workers 4`
compara Manhattan e Euclidiana nos caminhões nas 120 configurações grandes,
com sementes pareadas e validação independente. O script compila cópias em
`output/experimento_metricas`, mantém os tempos oficiais dos drones e grava
CSV, certificados JSONL, resumo e relatório. O executável principal e o registro
`results/best.json` são preservados. O padrão do algoritmo continua Manhattan.

O [relatório de 17/11/2026](docs/auditorias/experimento_metricas_20261117.md)
registra os resultados. A literatura usa Manhattan nos caminhões: diferenças
euclidianas contra os valores publicados são comparações entre variantes,
sem constituir novos recordes. O orçamento de dois segundos é exploratório.

### Experimento com as equações físicas do fundador

`python scripts/experimentar_equacoes.py --seconds 2 --repetitions 3 --workers 4`
recalcula as 60 tabelas físicas e executa as 180 configurações PDSVRP-C mais
as 60 configurações com um caminhão. Compara tabelas oficiais, equações
impressas com preparo de um minuto e a sensibilidade com preparo de um segundo.
As contradições do artigo são explicitadas; a restrição de consumo segue o
texto de no máximo 90% da capacidade. As saídas ficam isoladas em
`output/experimento_equacoes`. O executor retoma registros existentes com
os mesmos parâmetros; use outro `--output` para iniciar um experimento novo.

`python scripts/relatar_equacoes.py` revalida os resultados e gera o
[relatório completo](docs/auditorias/equacoes_completas_20261117.md), incluindo
as diferenças físicas, resultados por configuração e recálculo das mesmas
soluções para separar o efeito das equações do efeito da busca.

| Arquivo | Responsabilidade |
| --- | --- |
| `main.c` | Lançador e worker; emite certificados internos capturados pelo executor |
| `lerInstancias.c` | Leitura, conferência e liberação das entradas |
| `cv.c` | Soluções, custos, validação integral e vizinhanças |
| `drone_bb.c` | Escalonamento guloso e Branch and Bound |
| `SA.c` | Calibração, aceitação e resfriamento |
| `scripts/executar_benchmark.py` | Catálogo, validação independente, execução, registro e tabela |
| `scripts/verificar_*.c`, `test_*.py` | Verificações necessárias para manter a correção |
| `scripts/preparar_literatura.py` | Transcrição auditável dos valores publicados |
| `scripts/validate_instances.py` | Integridade e comparação byte a byte com o ZIP oficial |

Explicação do algoritmo: [docs/algoritmo.md](docs/algoritmo.md). Para gerar uma versão PDF, execute `python scripts/gerar_pdf.py` com ReportLab instalado. A execução do algoritmo usa apenas a biblioteca padrão do Python.

Os relatórios de rastreabilidade física estão em `docs/auditorias/`. Logs, certificados antigos, binários obsoletos e scripts de experimentos pontuais foram removidos. Entradas oficiais, referências, auditoria física reproduzível e testes de correção foram preservados.
