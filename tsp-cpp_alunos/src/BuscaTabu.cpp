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
    melhorI = -1;
    melhorJ = -1;
    for(int i = 0; i < n; i++){

        
        for(int j = i+1; j < n; j++){


            //No delta eu apenas calculo o custo das arestas

            double deltaAntigo = calculaDelta(inst, s, i, j);
            std::swap(s[i], s[j]);
            double deltaNovo = calculaDelta(inst, s, i, j);
            std::swap(s[i], s[j]);


            double novoValor = fo + deltaNovo - deltaAntigo;

            if(novoValor < foMelhorVizinho && (!estaNaListaTabu(i, j, listaTabu) || novoValor < foEstrela)){
                melhorI = i;
                melhorJ = j;

                foMelhorVizinho = novoValor;

            }





        }

    }
    
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
    
    int i, j;

    while((iterBT - melhorIter < btMax) && iterBT < iterMaxAbsoluto){

        iterBT++;

        fo = melhorVizinhoTabu(inst, s, fo, i, j, foEstrela, listaTabu);
        if(i == -1 || j == -1){
            break;
        }
        std::swap(s[i], s[j]);

        std::pair<int, int> par;
        par.first = i;
        par.second = j;

        if(tamanhoMaximoLista > 0){
            if(listaTabu.size() >= static_cast<std::size_t>(tamanhoMaximoLista)){

                listaTabu.pop_front();

            }
            listaTabu.push_back(par);

        }



        if(fo < foEstrela){

            sEstrela = s;
            melhorIter = iterBT;
            foEstrela = fo;

        }


    }


    s = sEstrela;
    registraProgresso(arquivoLog, 0.0, iterBT, fo);
    registraProgresso(arquivoLogMelhorFo, 0.0, iterBT, foEstrela);

    return foEstrela;
}
