#include "AlgoritmoGenetico.h"
#include "OperadoresGeneticos.h"
#include "GRASP.h"
#include "Utilitarios.h"
#include "Aleatorio.h"

#include <iostream>
#include <limits>

// ============================================================================
// [EXERCÍCIO]
// ============================================================================
double algoritmoGenetico(const Instancia &inst, std::vector<int> &s,
                          int nind, double maxDesvio,
                          double probCrossover, double probMutacao,
                          int tipoOperador)
{

    // A população mantém "nind" sobreviventes + até "nind" descendentes
    // gerados a cada geração -- por isso o vetor de população tem 2*nind
    // posições (metade "pais/sobreviventes", metade "filhos").
    int tamanhoPop = nind * 2;

    std::vector<std::vector<int>> pop(tamanhoPop);
    std::vector<double> foPop(tamanhoPop, 0.0);

    std::vector<int> sEstrela;
    double foEstrela = std::numeric_limits<double>::max();

    int nGeracoes = 0;
    double desvio = std::numeric_limits<double>::max();

    //TODO

    s = sEstrela;
    std::cout << "\nNumero de geracoes avaliadas: " << nGeracoes << '\n';
    return foEstrela;
}
