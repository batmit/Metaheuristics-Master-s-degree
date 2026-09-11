#include "GRASP.h"
#include "Construcao.h"
#include "BuscaLocal.h"
#include "Utilitarios.h"

#include <limits>

// ============================================================================
// [EXERCÍCIO]
// ============================================================================
// Observação para o professor: no código original em C, a construção por
// Inserção Mais Barata (tipoConstrucao == 2) vinha DESABILITADA em
// GRASP.cpp por causa de um bug na heurística correspondente. Como esse
// bug foi corrigido em Construcao.cpp (veja o comentário lá), aqui as
// duas opções de construção já funcionam normalmente.
// ============================================================================
double grasp(const Instancia &inst, std::vector<int> &s,
             double alpha, int graspMax, int tipoConstrucao)
{
    std::vector<int> sEstrela;
    double foEstrela = std::numeric_limits<double>::max();

    //TODO

    s = sEstrela;
    return foEstrela;
}
