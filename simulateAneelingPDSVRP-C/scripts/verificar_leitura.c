#include "../lerInstancias.h"
#include "../cv.h"
#include <assert.h>
#include <math.h>

int main(int argc, char **argv)
{
    assert(argc == 3);
    for (int frota = 2; frota <= 5; ++frota) {
        Instancia inst = {0};
        Solucao solucao = {0};
        assert(carregarInstancia(argv[1], argv[2], frota, &inst));
        assert(inst.numCaminhoes == frota && inst.n > 1);
        assert(alocarSolucao(&inst, &solucao));
        assert(!solucao.avaliacaoValida && isinf(solucao.makespan));
        for (int c = 0; c < frota; ++c) {
            assert(solucao.rotasCaminhao[c].capacidade == inst.n - 1);
            assert(solucao.rotasCaminhao[c].tamanho == 0);
            for (int d = c + 1; d < frota; ++d)
                assert(solucao.rotasCaminhao[c].rota != solucao.rotasCaminhao[d].rota);
        }
        for (int i = 0; i < inst.n; ++i) {
            assert(inst.tempoCaminhao[i][i] == 0);
            int minimo = 0;
            for (int k = 1; k <= inst.numDrones; ++k)
                if (isfinite(inst.tempoDrone[i][k]) && !minimo) minimo = k;
            assert(inst.clientes[i].minDrones == minimo);
            assert(inst.clientes[i].elegivelDrone == (minimo > 0));
            for (int j = 0; j < inst.n; ++j) {
                double esperado = (fabs(inst.clientes[i].x-inst.clientes[j].x) +
                                   fabs(inst.clientes[i].y-inst.clientes[j].y)) /
                                   inst.truckSpeed * 60.0;
                assert(fabs(inst.tempoCaminhao[i][j] - esperado) < 1e-9);
            }
        }
        liberarSolucao(&solucao);
        liberarSolucao(&solucao);
        liberarInstancia(&inst);
        liberarInstancia(&inst);
        assert(!inst.clientes && !solucao.rotasCaminhao);
    }
    return 0;
}
