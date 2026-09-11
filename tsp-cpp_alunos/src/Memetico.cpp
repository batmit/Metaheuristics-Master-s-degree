#include "Memetico.h"
#include "OperadoresGeneticos.h"
#include "GRASP.h"
#include "BuscaLocal.h" // descida
#include "Utilitarios.h"
#include "Aleatorio.h"

#include <iostream>
#include <limits>

// ============================================================================
// [EXERCÍCIO]
// ============================================================================
double memetico(const Instancia &inst, std::vector<int> &s,
                 int nind, double maxDesvio, int maxGeracoes,
                 double probCrossover, double probMutacao,
                 int tipoOperador)
{

    int tamanhoPop = nind * 2;
    int nGeracoes = 0;

    std::vector<std::vector<int>> pop(tamanhoPop);
    std::vector<double> foPop(tamanhoPop, 0.0);

    std::vector<int> sEstrela;
    double foEstrela = std::numeric_limits<double>::max();

    //TODO
    
    s = sEstrela;
    std::cout << "\nNumero de geracoes avaliadas: " << nGeracoes << '\n';
    return foEstrela;
}
