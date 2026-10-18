#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "cv.h"
#include "lerInstancias.h"

int main(void)
{
    Instancia inst = {0};
    Solucao atual = {0}, vizinho = {0};
    assert(lerInstancia("18-r-e.txt", &inst));
    alocarInstancias(&inst);
    construirMatrizCaminhao(&inst);
    assert(lerTemposDrone("tau_kj_18-r-e.txt", &inst));
    alocarSolucao(&inst, &atual);
    alocarSolucao(&inst, &vizinho);
    assert(construirSolucaoInicial(&inst, &atual));
    Missao *copiaC = malloc(inst.n * sizeof *copiaC);
    MissaoDrone *copiaD = malloc(inst.n * sizeof *copiaD);
    assert(copiaC && copiaD);
    int movimentos[4] = {0};
    srand(123);
    for (int iteracao = 0; iteracao < 2000; iteracao++) {
        memcpy(copiaC, atual.missaoCaminhao, inst.n * sizeof *copiaC);
        memcpy(copiaD, atual.missaoDrone, inst.n * sizeof *copiaD);
        Solucao antes = atual;
        assert(gerarVizinho(&inst, &atual, &vizinho));
        assert(atual.numMissoesCaminhao == antes.numMissoesCaminhao);
        assert(atual.numMissoesDrone == antes.numMissoesDrone);
        assert(atual.makespan == antes.makespan);
        assert(memcmp(copiaC, atual.missaoCaminhao, inst.n * sizeof *copiaC) == 0);
        assert(memcmp(copiaD, atual.missaoDrone, inst.n * sizeof *copiaD) == 0);
        assert(isinf(vizinho.makespan));
        if (vizinho.numMissoesDrone > atual.numMissoesDrone) movimentos[2]++;
        else if (vizinho.numMissoesDrone < atual.numMissoesDrone) movimentos[3]++;
        else if (memcmp(copiaC, vizinho.missaoCaminhao,
                        atual.numMissoesCaminhao * sizeof *copiaC) != 0) movimentos[0]++;
        else movimentos[1]++;
        assert(escalonarMissoesDrone(&inst, &vizinho));
        /* Tambem verifica cobertura unica e ausencia de conflitos da frota. */
        assert(calcularMakespan(&inst, &vizinho));
        Solucao auxiliar = atual; atual = vizinho; vizinho = auxiliar;
    }
    for (int i = 0; i < 4; i++) assert(movimentos[i] > 0);
    assert(!gerarVizinho(&inst, &atual, &atual));
    assert(!gerarVizinho(NULL, &atual, &vizinho));

    /* Um cliente nao elegivel: nao ha movimento possivel. */
    Instancia pequena = {0}; pequena.n = 2;
    Missao unico[] = {{1}};
    Solucao semMovimento = {0};
    semMovimento.missaoCaminhao = unico; semMovimento.numMissoesCaminhao = 1;
    double makespanAnterior = vizinho.makespan;
    assert(!gerarVizinho(&pequena, &semMovimento, &vizinho));
    assert(vizinho.makespan == makespanAnterior);
    /* Todos no caminhao, sem elegiveis: apenas troca de posicoes. */
    assert(construirSolucaoInicial(&inst, &atual));
    for (int i = 1; i < inst.n; i++) inst.clientes[i].elegivelDrone = false;
    assert(gerarVizinho(&inst, &atual, &vizinho));
    assert(vizinho.numMissoesDrone == 0);
    assert(memcmp(atual.missaoCaminhao, vizinho.missaoCaminhao,
                  atual.numMissoesCaminhao * sizeof *copiaC) != 0);
    free(copiaC); free(copiaD);
    liberarSolucao(&atual); liberarSolucao(&vizinho); liberarInstancia(&inst);
    return 0;
}
