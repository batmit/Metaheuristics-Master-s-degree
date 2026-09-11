#include "VNS.h"
#include "BuscaLocal.h" // calculaDelta, descida
#include "Utilitarios.h"
#include "Aleatorio.h"

#include <algorithm> // std::swap
#include <iostream>

// ============================================================================
// [EXERCÍCIO]
// ============================================================================
double geraUmVizinhoAleatorio(const Instancia &inst, std::vector<int> &s, double fo)
{
    int n = inst.n;
    int i = inteiroAleatorio(0, n - 1);
    int j;
    do {
        j = inteiroAleatorio(0, n - 1);
    } while (i == j);

    double delta1 = calculaDelta(inst, s, i, j);
    std::swap(s[i], s[j]);
    double delta2 = calculaDelta(inst, s, i, j);

    return fo - delta1 + delta2;
}

// ============================================================================
// [EXERCÍCIO]
// ============================================================================
double geraKVizinhosAleatorios(const Instancia &inst, std::vector<int> &s, double fo, int k)
{
    double foVizinho = fo;
    for (int i = 0; i < k; i++) {
        foVizinho = geraUmVizinhoAleatorio(inst, s, foVizinho);
    }
    return foVizinho;
}

// ============================================================================
// [EXERCÍCIO]
// ============================================================================
double vns(const Instancia &inst, std::vector<int> &s, int vnsMax, int r)
{
    double fo = custo(inst, s);
    int iter = 0, melhorIter = 0;

    std::cout << "Iter VNS = " << iter << "\t fo_star = " << fo << '\n';

    //TODO

    return fo;
}
