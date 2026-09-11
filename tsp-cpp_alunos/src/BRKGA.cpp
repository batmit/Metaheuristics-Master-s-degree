#include "BRKGA.h"
#include "Utilitarios.h"

#include <iostream>

// ============================================================================
// [EXERCÍCIO - método futuro do cronograma]
// ============================================================================
double brkga(const Instancia &inst, std::vector<int> &s,
             int tamanhoPop, double fracaoElite, double fracaoMutantes,
             double probEliteNoFilho, int maxGeracoes)
{
    (void)tamanhoPop;
    (void)fracaoElite;
    (void)fracaoMutantes;
    (void)probEliteNoFilho;
    (void)maxGeracoes;
    std::cerr << "BRKGA: metodo ainda nao implementado.\n";
    return custo(inst, s);
}
