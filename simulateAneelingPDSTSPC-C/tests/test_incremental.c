#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cv.h"
#include "lerInstancias.h"

static void conferir(const Instancia *inst, const Solucao *s, Solucao *completa){
    completa->numMissoesCaminhao = s->numMissoesCaminhao;
    completa->numMissoesDrone = s->numMissoesDrone;
    memcpy(completa->missaoCaminhao, s->missaoCaminhao, s->numMissoesCaminhao * sizeof(Missao));
    memcpy(completa->missaoDrone, s->missaoDrone, s->numMissoesDrone * sizeof(MissaoDrone));
    assert(escalonarMissoesDrone(inst, completa));
    assert(calcularMakespan(inst, completa));
    if (fabs(s->tempoCaminhao - completa->tempoCaminhao) > 1e-7) {
        fprintf(stderr, "caminhao incremental %.17g completo %.17g nc %d\n", s->tempoCaminhao, completa->tempoCaminhao, s->numMissoesCaminhao);
        abort();
    }
    assert(fabs(s->tempoDrones - completa->tempoDrones) < 1e-7);
    assert(fabs(s->makespan - completa->makespan) < 1e-7);
    for (int i = 0; i < s->numMissoesDrone; i++) {
        assert(s->missaoDrone[i].tempoInicio == completa->missaoDrone[i].tempoInicio);
        assert(s->missaoDrone[i].tempoFim == completa->missaoDrone[i].tempoFim);
        assert(s->missaoDrone[i].qtdDrones == completa->missaoDrone[i].qtdDrones);
    }
}
int main(void){
    Instancia inst = {0};
    assert(lerInstancia("18-r-e.txt", &inst));
    alocarInstancias(&inst); construirMatrizCaminhao(&inst);
    assert(lerTemposDrone("tau_kj_18-r-e.txt", &inst));
    Solucao atual = {0}, vizinho = {0}, completa = {0};
    alocarSolucao(&inst, &atual); alocarSolucao(&inst, &vizinho); alocarSolucao(&inst, &completa);
    assert(construirSolucaoInicial(&inst, &atual));
    assert(escalonarMissoesDrone(&inst, &atual));
    assert(calcularMakespan(&inst, &atual));
    srand(123);
    for (int i = 0; i < 20000; i++) {
        assert(gerarVizinhoAvaliado(&inst, &atual, &vizinho));
        conferir(&inst, &vizinho, &completa);
        if (i % 3 != 0) { Solucao aux = atual; atual = vizinho; vizinho = aux; }
    }
    liberarSolucao(&atual); liberarSolucao(&vizinho); liberarSolucao(&completa);
    liberarInstancia(&inst);
    /* Matriz assimetrica, diagonal nao nula, tempos fracionarios e lacunas
     * de viabilidade: cobre deposito, rota vazia e trocas adjacentes. */
    Cliente clientes[4] = {0};
    for (int i = 1; i < 4; i++) {
        clientes[i].elegivelDrone = true; clientes[i].minDrones = 1;
    }
    double t0[] = {7.1, 1.2, 8.3, 2.4}, t1[] = {4.5, 7.1, 2.6, 9.7};
    double t2[] = {3.8, 6.9, 7.1, 1.1}, t3[] = {8.2, 3.3, 5.4, 7.1};
    double *tc[] = {t0, t1, t2, t3};
    double d0[] = {INFINITY, INFINITY, INFINITY, INFINITY};
    double d1[] = {INFINITY, 9.1, INFINITY, 5.2};
    double d2[] = {INFINITY, 4.3, 3.4, 2.5};
    double d3[] = {INFINITY, 0.0, INFINITY, INFINITY};
    double *td[] = {d0, d1, d2, d3};
    Instancia pequena = {.n = 4, .numDrones = 3, .clientes = clientes,
                         .tempoCaminhao = tc, .tempoDrone = td};
    alocarSolucao(&pequena, &atual); alocarSolucao(&pequena, &vizinho);
    alocarSolucao(&pequena, &completa);
    assert(construirSolucaoInicial(&pequena, &atual));
    assert(!gerarVizinhoAvaliado(&pequena, &atual, &vizinho));
    assert(escalonarMissoesDrone(&pequena, &atual));
    assert(calcularMakespan(&pequena, &atual));
    int vazio = 0, cheio = 0, trocas = 0, transferencias = 0, quantidades = 0;
    for (int i = 0; i < 10000; i++) {
        assert(gerarVizinhoAvaliado(&pequena, &atual, &vizinho));
        conferir(&pequena, &vizinho, &completa);
        conferir(&pequena, &atual, &completa);
        vazio += vizinho.numMissoesCaminhao == 0;
        cheio += vizinho.numMissoesDrone == 0;
        transferencias += atual.numMissoesDrone != vizinho.numMissoesDrone;
        if (atual.numMissoesDrone == vizinho.numMissoesDrone) {
            trocas += memcmp(atual.missaoCaminhao, vizinho.missaoCaminhao,
                            atual.numMissoesCaminhao * sizeof(Missao)) != 0;
            for (int j = 0; j < atual.numMissoesDrone; j++)
                quantidades += atual.missaoDrone[j].cliente == vizinho.missaoDrone[j].cliente &&
                    atual.missaoDrone[j].qtdDrones != vizinho.missaoDrone[j].qtdDrones;
        }
        if (i % 3 != 0) { Solucao aux = atual; atual = vizinho; vizinho = aux; }
    }
    assert(vazio && cheio && trocas && transferencias && quantidades);
    assert(!gerarVizinhoAvaliado(&pequena, &atual, &atual));
    liberarSolucao(&atual); liberarSolucao(&vizinho); liberarSolucao(&completa);
    return 0;
}
