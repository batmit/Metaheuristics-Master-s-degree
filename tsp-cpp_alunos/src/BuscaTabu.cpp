#include "BuscaTabu.h"
#include "BuscaLocal.h"   // calculaDelta
#include "Utilitarios.h"
#include "Registro.h"

#include <algorithm> // std::swap
#include <limits>

// ============================================================================
// [FRAMEWORK]
// ============================================================================
bool estaNaListaTabu(int posicao1, int posicao2,
                      const std::deque<std::pair<int, int>> &listaTabu)
{
    for (const auto &movimento : listaTabu) {
        if (movimento.first == posicao1 && movimento.second == posicao2) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// [EXERCÍCIO]
// ============================================================================
double melhorVizinhoTabu(const Instancia &inst, std::vector<int> &s, double fo,
                          int &melhorI, int &melhorJ, double foEstrela,
                          const std::deque<std::pair<int, int>> &listaTabu)
{
    int n = inst.n;
    double foMelhorVizinho = std::numeric_limits<double>::max();

    
    return foMelhorVizinho;
}

// ============================================================================
// [EXERCÍCIO]
// ============================================================================
double buscaTabu(const Instancia &inst, std::vector<int> &s,
                  int tamanhoMaximoLista, int btMax)
{
    std::vector<int> sEstrela = s;
    double fo = custo(inst, s);
    double foEstrela = fo;

    std::deque<std::pair<int, int>> listaTabu;

    int iterBT = 0;
    int melhorIter = 0;

    const std::string arquivoLog = "BTsaida.txt";
    const std::string arquivoLogMelhorFo = "BTsaidaMelhorfo.txt";
    limpaArquivo(arquivoLog);
    limpaArquivo(arquivoLogMelhorFo);
    registraProgresso(arquivoLog, 0.0, iterBT, fo);
    registraProgresso(arquivoLogMelhorFo, 0.0, iterBT, foEstrela);

    // Atenção: com listas tabu curtas (ou instâncias específicas), a
    // busca pode entrar em CICLO -- revisitar sempre a mesma sequência de
    // soluções, sem nunca acumular "btMax" iterações seguidas sem
    // melhora, o que faria o critério de parada normal nunca ser
    // satisfeito. Um limite absoluto de iterações garante que o método
    // sempre termina.
    const int iterMaxAbsoluto = 100000;

    // TODO

    s = sEstrela;
    registraProgresso(arquivoLog, 0.0, iterBT, fo);
    registraProgresso(arquivoLogMelhorFo, 0.0, iterBT, foEstrela);

    return foEstrela;
}
