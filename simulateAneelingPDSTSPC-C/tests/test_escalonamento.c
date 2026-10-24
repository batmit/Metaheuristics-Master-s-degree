#include <assert.h>
#include <math.h>
#include <stddef.h>
#include "cv.h"

int main(void)
{
    Cliente clientes[4] = {0};
    clientes[1].elegivelDrone = true; clientes[1].minDrones = 2;
    clientes[2].elegivelDrone = true; clientes[2].minDrones = 1;
    clientes[3].elegivelDrone = true; clientes[3].minDrones = 3;
    double t0[] = {INFINITY, INFINITY, INFINITY, INFINITY};
    double t1[] = {INFINITY, INFINITY, 10, 9};
    double t2[] = {INFINITY, 6, 5, 4};
    double t3[] = {INFINITY, INFINITY, INFINITY, 8};
    double *tempos[] = {t0, t1, t2, t3};
    Instancia inst = {0};
    inst.n = 4; inst.numDrones = 3;
    inst.clientes = clientes; inst.tempoDrone = tempos;
    MissaoDrone missoes[] = {{1, 0, 99, 99}, {2, 0, 99, 99}, {3, 0, 99, 99}};
    Solucao s = {0};
    s.missaoDrone = missoes; s.numMissoesDrone = 3;

    assert(escalonarMissoesDrone(&inst, &s));
    assert(missoes[0].qtdDrones == 2);
    assert(missoes[0].tempoInicio == 0 && missoes[0].tempoFim == 10);
    assert(missoes[1].tempoInicio == 0 && missoes[1].tempoFim == 6);
    assert(missoes[2].tempoInicio == 10 && missoes[2].tempoFim == 18);
    assert(calcularMakespan(&inst, &s) && s.makespan == 18);

    /* Reconstrucao ignora horarios anteriores. */
    missoes[0].tempoInicio = 100;
    assert(escalonarMissoesDrone(&inst, &s));
    assert(missoes[0].tempoInicio == 0);
    assert(isinf(s.makespan));

    /* Mudanca de sequencia muda o cronograma. */
    missoes[0].cliente = 3; missoes[2].cliente = 1;
    missoes[0].qtdDrones = 0; missoes[2].qtdDrones = 0;
    assert(escalonarMissoesDrone(&inst, &s));
    assert(missoes[0].tempoFim == 8);
    assert(missoes[1].tempoInicio == 8 && missoes[1].tempoFim == 14);
    assert(missoes[2].tempoInicio == 8 && missoes[2].tempoFim == 18);

    /* Preserva a quantidade escolhida e utiliza a duracao correspondente. */
    missoes[2].qtdDrones = 3;
    assert(escalonarMissoesDrone(&inst, &s));
    assert(missoes[2].qtdDrones == 3);
    assert(missoes[2].tempoInicio == 14 && missoes[2].tempoFim == 23);
    missoes[2].qtdDrones = 1;
    assert(!escalonarMissoesDrone(&inst, &s));
    missoes[2].qtdDrones = 2;

    /* Falha posterior nao publica um cronograma parcial. */
    clientes[1].elegivelDrone = false;
    missoes[0].tempoInicio = 123;
    assert(!escalonarMissoesDrone(&inst, &s));
    assert(missoes[0].tempoInicio == 123 && isinf(s.makespan));
    clientes[1].elegivelDrone = true;
    clientes[1].minDrones = 4;
    assert(!escalonarMissoesDrone(&inst, &s));
    clientes[1].minDrones = 2;
    t1[2] = INFINITY;
    assert(!escalonarMissoesDrone(&inst, &s));
    t1[2] = 10;
    missoes[2].cliente = 2;
    assert(!escalonarMissoesDrone(&inst, &s));
    missoes[2].cliente = 4;
    assert(!escalonarMissoesDrone(&inst, &s));
    inst.numDrones = 0;
    assert(!escalonarMissoesDrone(&inst, &s));
    s.numMissoesDrone = 0;
    assert(escalonarMissoesDrone(&inst, &s));
    assert(!escalonarMissoesDrone(NULL, &s));
    assert(!escalonarMissoesDrone(&inst, NULL));
    return 0;
}
