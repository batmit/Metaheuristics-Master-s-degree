# BCC466 — Código-base em C++ para o PCV (refatorado)

Código de aulas práticas do PCV (Problema do Caixeiro Viajante), originalmente
em C, convertido para C++ moderno (C++17) usando a STL. Este README documenta
o que mudou e por quê, para orientar o uso em sala pelos alunos.

## Estrutura de pastas

```
pcv-cpp/
├── Makefile
├── README.md
├── src/            <- todo o código-fonte (.cpp e .h)
├── instancias/      <- arquivos de dados das instâncias (.TXT)
├── obj/             <- arquivos objeto (.o) gerados pela compilação (não versionar)
└── pcv             <- executável final, gerado na raiz pelo `make`
```

## Como compilar e rodar

```bash
make            # compila (Linux: g++; Windows/MinGW: g++.exe) -> ./Upcv
make run        # compila (se preciso) e executa
make clean      # remove obj/*.o e o executável
```

**Rode sempre a partir da raiz do projeto** (onde está o `Makefile`), e não
de dentro de `src/` ou `obj/` — o programa lê as instâncias com caminhos
relativos como `"instancias/C50.TXT"` (veja `main.cpp`). O alvo `make run`
já faz isso corretamente.

No Windows, instale o MinGW-w64 (por exemplo, via MSYS2) e rode `make`
(ou `mingw32-make`) em um terminal com o `g++` no PATH. O Makefile detecta o
sistema operacional automaticamente (variável `$(OS)`) e ajusta o nome do
executável (`pcv` ou `pcv.exe`) e os comandos de limpeza/criação de pasta.

A instância padrão carregada em `main.cpp` é `C50` (51 cidades). Para trocar
de instância, basta editar as duas linhas no início do `main()`:

```cpp
const std::string arquivoInfo = "instancias/C50INFO.TXT";
const std::string arquivoCoordenadas = "instancias/C50.TXT";
```

e usar `instancias/a280INFO.TXT` / `instancias/A280.TXT` (280 cidades) como
alternativa já incluída.

**Atenção (maiúsculas/minúsculas):** no Linux, nomes de arquivo diferenciam
maiúsculas de minúsculas; `a280INFO.TXT` está em minúsculo no "a" de propósito,
igual ao arquivo original. No Windows isso não faz diferença, mas mantenha a
grafia exata para que o código funcione nos dois sistemas.

## O que mudou em relação ao código original em C

1. **STL em vez de estruturas manuais.** `struct lista` (lista encadeada
   usada só para a lista tabu) e as funções de alocação manual
   (`cria_vetor`, `cria_matriz`, `libera_*`, `atualiza_vetor`,
   `inicializa_vetor`, `embaralha_vetor`, `busca_pos_valor`,
   `insere_meio_vetor`...) desapareceram. `std::vector` cuida de alocação,
   cópia e liberação de memória sozinho; `std::find`, `std::min_element`,
   `std::sort`, `std::shuffle`, `vector::insert/erase` substituem o resto.
   A lista tabu agora é um `std::deque<std::pair<int,int>>`.

2. **`struct Instancia`** (`Instancia.h`) agrupa `n` e a matriz de
   distâncias, resolvendo o problema de `int n` e `float **d` sempre
   "andarem juntos" como parâmetros separados. Passada por
   `const Instancia &` — os dados continuam totalmente visíveis
   (`inst.n`, `inst.d[i][j]`), sem esconder nada em uma classe complexa.

3. **`<random>` (mt19937)** substitui `rand()/srand()`, centralizado em
   `Aleatorio.h/cpp`.

4. **Nomes de funções e variáveis em português**, mantendo consistência com
   o material do curso.

5. **Duplicação de código**: `AG.cpp` e `Memeticos.cpp` (originais) tinham
   cada um sua própria cópia de `roleta`, `crossover_OX`, `crossover_ERX` e
   `mutacao`. Agora esses operadores ficam centralizados em
   `OperadoresGeneticos.h/cpp` e são usados tanto por `AlgoritmoGenetico.cpp`
   quanto por `Memetico.cpp` (que só acrescenta a etapa de busca local).
   Da mesma forma, `SA.cpp`/`SA.h` (versão antiga, não usada no `pcv.cpp`
   original) foi descartada em favor de `SimulatedAnnealing.h/cpp` (a
   versão que de fato estava em uso).

6. **Bug conhecido corrigido**: a construção parcialmente gulosa por
   Inserção Mais Barata (`constroi_solucao_parcialmente_gulosa_insercao_mais_barata`
   no código original) tinha um bug documentado no próprio código-fonte
   ("HA ERRO NO METODO ABAIXO") e vinha desabilitada no menu do GRASP. A
   versão em `Construcao.cpp` foi reescrita corretamente e já está habilitada
   (opção 2 do menu GRASP, opção 4 da geração de solução inicial).

7. **Cronometragem com `std::chrono`** em vez de `clock()/CLOCKS_PER_SEC`
   (mais portável entre Linux e Windows).

8. **Validação automática da solução final** (`Validacao.h/cpp`): depois de
   cada método, o `main.cpp` confere se a rota devolvida é uma permutação
   válida das cidades e se a fo devolvida bate com o custo recalculado a
   partir da rota. Veja a seção "Validação da solução final" abaixo.

9. **Plotagem da rota em SVG e PNG** (`Plotagem.h/cpp`): a `struct Instancia`
   passou a guardar também as coordenadas (`x`, `y`) de cada cidade (antes
   descartadas depois de calcular a matriz de distâncias), usadas para
   desenhar a rota. Veja a seção "Plotagem da rota" abaixo.

10. **Organização em pastas** (`src/`, `instancias/`, `obj/`): o código
    original tinha tudo solto em uma única pasta (código, dados e
    executável misturados). Agora `src/` guarda só código-fonte,
    `instancias/` só os arquivos de dados, e `obj/` recebe os `.o` gerados
    pela compilação (pasta que pode ser apagada a qualquer momento com
    `make clean`). O executável final continua sendo gerado na raiz do
    projeto, para ficar fácil de rodar (`./pcv`).

## Estrutura dos arquivos

*(todos os arquivos abaixo estão dentro de `src/`)*

### Framework (dado pronto aos alunos)
| Arquivo | Conteúdo |
|---|---|
| `Instancia.h/cpp` | struct `Instancia`, leitura da instância |
| `Aleatorio.h/cpp` | geração de números aleatórios (`<random>`) |
| `Utilitarios.h/cpp` | `custo()`, `imprimeRota()`, `desvioPadrao()` |
| `Registro.h/cpp` | gravação do log de convergência em arquivo |
| `Validacao.h/cpp` | conferência de integridade da rota e da fo (veja abaixo) |
| `Plotagem.h/cpp` | gera imagens (SVG e PNG) da rota (veja abaixo) |
| `Menus.h/cpp` | menus de texto |
| `main.cpp` | laço principal (menu → chama o método escolhido) |

## Validação da solução final (`Validacao.h/cpp`)

`validaSolucao(inst, s, foInformada)` confere três coisas de uma vez:

1. **Tamanho**: `s.size() == inst.n`;
2. **Integridade**: cada cidade `0..n-1` aparece **exatamente uma vez** em
   `s` (nem faltando, nem repetida) — usa um vetor de contagem, sem
   reinventar nada da STL;
3. **Consistência da fo**: recalcula o custo do zero a partir da rota
   (`custo(inst, s)`) e compara com a `foInformada` (a fo que o método
   devolveu), com tolerância de `1e-3` para arredondamento de ponto
   flutuante.

Imprime um relatório no console e devolve `true`/`false`. Já está
conectada em `main.cpp`: depois de qualquer método de refinamento (opções
2 a 18 do menu), o programa chama `validaSolucao(inst, s, fo)`
automaticamente e mostra o resultado.

## Plotagem da rota (`Plotagem.h/cpp`)

`plotaRotaSVG(inst, s, "arquivo.svg")` e `plotaRotaPNG(inst, s, "arquivo.png")`
desenham a rota `s` (na mesma convenção do resto do projeto) usando as
coordenadas das cidades (`inst.x`, `inst.y`). A cidade 0 (origem) aparece
destacada em vermelho; as demais, em preto; as arestas da rota em azul.
Já conectada ao menu principal (opção 19), que gera os dois arquivos
(`rota.svg` e `rota.png`) de uma vez a partir da solução corrente.

Não existe biblioteca gráfica **nativa** do C++ (nem pré-instalada de
forma unificada em Linux/Windows/macOS) para gerar imagens — por isso a
implementação usa duas estratégias complementares:

- **SVG** (`plotaRotaSVG`): um arquivo de texto (XML), escrito só com
  `<fstream>` — **zero dependências externas**. Abre em qualquer
  navegador, é vetorial (não perde qualidade ao ampliar) e já vem com o
  número de cada cidade ao lado do seu marcador, útil para conferir a
  rota cidade a cidade.
- **PNG** (`plotaRotaPNG`): uma imagem raster de verdade, gerada com a
  [stb_image_write.h](https://github.com/nothings/stb) — uma biblioteca
  de **cabeçalho único, de domínio público**, sem necessidade de
  instalação em nenhum sistema operacional (basta o arquivo
  `stb_image_write.h`, já incluído no projeto). O desenho das linhas
  (algoritmo de Bresenham) e dos marcadores das cidades foi implementado
  do zero em `Plotagem.cpp`; a stb só entra no final, para salvar a
  matriz de pixels como `.png`. Diferente do SVG, o PNG não rotula as
  cidades com seus números (desenhar texto em uma imagem raster exige
  uma fonte bitmap, fora do escopo aqui) — para conferir a rota
  cidade a cidade, prefira o SVG.

O arquivo `stb_image_write_impl.cpp` existe só para conter a
implementação da biblioteca (por convenção da própria stb, o código real
só pode aparecer em um `.cpp` do projeto); os demais arquivos apenas
incluem `stb_image_write.h` normalmente.

### Métodos já implementados neste código de referência (exercícios do TP1/TP2)
Cada arquivo abaixo está marcado internamente com comentários
`// [EXERCÍCIO]` acima de cada função a ser implementada pelos alunos:

`Construcao.h/cpp`, `BuscaLocal.h/cpp`, `MultiStart.h/cpp`,
`SimulatedAnnealing.h/cpp`, `BuscaTabu.h/cpp`, `GRASP.h/cpp`, `ILS.h/cpp`,
`VNS.h/cpp`, `OperadoresGeneticos.h/cpp`, `AlgoritmoGenetico.h/cpp`,
`Memetico.h/cpp`.

