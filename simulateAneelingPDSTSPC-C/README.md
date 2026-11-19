# PDSTSP-C: Simulated Annealing em C

Uma execução para cada uma das 60 instâncias, com tabela progressiva mostrando
os resultados e gaps da execução atual. O CSV preserva os melhores resultados
entre todas as execuções. As referências da literatura são preservadas.

## Compilar e executar

Na raiz do projeto, com GCC no PATH:

```powershell
gcc -O2 -std=c11 -Wall -Wextra -pedantic main.c cv.c lerInstancias.c SA.c -lm -o benchmark.exe
.\benchmark.exe
```

Também é possível abrir `benchmark.exe` com duplo clique. No Git Bash,
use `./benchmark.exe`. Não é preciso passar argumentos. Mantenha o executável
junto à pasta `instances`. A execução do algoritmo não precisa de Python.

Para consultar os recordes salvos, abra `melhores_resultados.exe`. Ele apenas
lê `resultados/relatorio.csv` e imprime a tabela, sem executar a busca ou alterar
o arquivo. O tempo exibido é o da última execução, não necessariamente o da obtenção do recorde.
Para recompilar esse visualizador:

```powershell
gcc -O2 -std=c11 -Wall -Wextra -pedantic -DMOSTRAR_RECORDES=1 main.c cv.c lerInstancias.c SA.c -lm -o melhores_resultados.exe
```

## Documentação

- [Guia simplificado do código](docs/GUIA_CODIGO.md): lógica, dados, fórmulas, exemplos e catálogo das funções.
- [Verificação da limpeza](docs/verificacao_limpeza.json): comparação de rotas, quantidades, horários, temperaturas e makespans antes/depois.
- [Experimento de B&B após cinco propostas sem melhora](docs/experimento_bb5_20261119.md): variante opcional, protocolo pareado e resultados isolados.
- [Experimento com B&B do caminhão somente no gargalo](docs/experimento_bb5_gargalo_20261119.md): comparação entre base, BB5 e BB5 com filtro.

## Arquivos ativos

| Caminho | Conteúdo |
|---|---|
| `main.c`, `main.h` | Lote, tabela, CSV e estruturas de dados. |
| `lerInstancias.c`, `lerInstancias.h` | Leitura e liberação das entradas. |
| `cv.c`, `cv.h` | Construção, vizinhanças, avaliação e escalonamento. |
| `SA.c`, `SA.h` | Calibração, aceitação, diversidade e encerramento. |
| `instances/pdstsp-c/` | Entradas oficiais, tempos, catálogo, referências e proveniência. |
| `resultados/relatorio.csv` | Único relatório ativo. |
| `tests/` | Testes do código mantido. |
| `docs/` | Guia do código e registro da comparação histórica. |
| `historico/pesquisas_e_versoes_anteriores.zip` | Pesquisa anterior e código retirado, com manifesto e integridade verificada. |

O diretório `tmp/` é criado somente para compilações e testes temporários.

## Configuração e resultado

A configuração fica em `parametrosSARapido()` (`SA.c`). O teto de 30 segundos
fica no início de `main.c`; ele é compartilhado pela calibração e busca e não
precisa ser esgotado. A busca termina pela temperatura final ou por estagnação
(400 níveis sem melhora e pelo menos 200 mil avaliações). Pode fazer duas
retomadas e uma chamada final de B&B com até 10 mil nós.

O CSV contém `instancia`, `melhor_algoritmo`, `literatura_2024`,
`literatura_2025` e `gap_percentual`. Os tempos são minutos e o gap usa a
referência de 2025. Após cada instância, um valor menor atualiza o CSV e o gap; valores iguais ou
piores preservam o recorde. O arquivo é recarregado a cada execução e não é
zerado automaticamente. Na tela, a coluna `Resultado atual` e o gap usam sempre
o resultado da rodada atual, mesmo quando ele é pior que o recorde salvo.
Em caso de falha, o resultado atual aparece como `--`. As referências
são soluções viáveis publicadas; não provam otimalidade em todos os casos.

A leitura aceita UTF-8 com ou sem BOM e quebras de linha LF/CRLF. O Windows
resolve os caminhos a partir da pasta do executável. Execute um processo por
vez para evitar disputa pelo mesmo CSV.

## Verificar

```powershell
python tests/executar.py
```

Esse comando compila e executa os 13 testes C e o teste de integração do CSV.
Os testes usam as instâncias canônicas e pastas temporárias próprias.
O histórico real de resultados é preservado.
