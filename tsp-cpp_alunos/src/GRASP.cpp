#include "GRASP.h"
#include "Construcao.h"
#include "BuscaLocal.h"
#include "Utilitarios.h"

#include <limits>
#include <algorithm>
#include <random>
#include <stdexcept>

// ============================================================================
// [EXERCÍCIO]
// ============================================================================

double grasp(const Instancia &inst, std::vector<int> &s,
             double alpha, int graspMax, int tipoConstrucao)
{
    std::vector<int> sEstrela;
    double foEstrela = std::numeric_limits<double>::max();

    for (int i = 0; i < graspMax; i++) {

        std::vector<int> sAtual;

        // Construção gulosa randomizada
        if (tipoConstrucao == 1) {
            constroiSolucaoGulosaVizinhoMaisProximo(inst, sAtual);
        }
        else if (tipoConstrucao == 2) {
            constroiSolucaoGulosaInsercaoMaisBarata(inst, sAtual);
        }

        // Busca local
        double foAtual = descidaCompleta(inst, sAtual);

        // Atualização da melhor solução
        if (foAtual < foEstrela) {
            foEstrela = foAtual;
            sEstrela = sAtual;
        }
    }

    s = sEstrela;
    return foEstrela;
}