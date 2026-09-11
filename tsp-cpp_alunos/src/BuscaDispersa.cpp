#include "BuscaDispersa.h"
#include "Utilitarios.h"

#include <iostream>

// ============================================================================
// [EXERCÍCIO - método futuro do cronograma]
// ============================================================================
double buscaDispersa(const Instancia &inst, std::vector<int> &s,
                      int tamanhoConjuntoReferencia, int maxIteracoes)
{
    (void)tamanhoConjuntoReferencia;
    (void)maxIteracoes;
    std::cerr << "Busca Dispersa: metodo ainda nao implementado.\n";
    return custo(inst, s);
}

// ============================================================================
// [EXERCÍCIO - método futuro do cronograma]
// ============================================================================
double reconexaoPorCaminhos(const Instancia &inst,
                             const std::vector<int> &solucaoPartida,
                             const std::vector<int> &solucaoGuia,
                             std::vector<int> &melhorSolucaoDoCaminho)
{
    std::cerr << "Reconexao por Caminhos: metodo ainda nao implementado.\n";
    melhorSolucaoDoCaminho = solucaoPartida;
    return custo(inst, solucaoGuia);
}
