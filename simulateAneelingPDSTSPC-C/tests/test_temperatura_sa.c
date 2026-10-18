#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include "SA.h"

int main(void)
{
    /* Toda troca piora de 3 para 30: exige aquecimento para aceitar 95%. */
    double a[] = {0, 1, 10}, b[] = {10, 0, 1}, c[] = {1, 10, 0};
    double *tempos[] = {a, b, c};
    Instancia inst = {0}; inst.n = 3; inst.tempoCaminhao = tempos;
    Missao rota[] = {{1}, {2}};
    Solucao s = {0}; s.missaoCaminhao = rota; s.numMissoesCaminhao = 2;
    s.makespan = 123; /* A calibracao deve recalcular numa copia. */
    ParametrosSA p = parametrosSAPadrao();
    double temperatura = -1, repetida = -1;
    srand(123);
    assert(calculaTemperaturaInicial(&inst, &s, &p, &temperatura));
    assert(isfinite(temperatura) && temperatura > p.tempInicial);
    assert(s.makespan == 123 && rota[0].n == 1 && rota[1].n == 2);
    srand(123);
    assert(calculaTemperaturaInicial(&inst, &s, &p, &repetida));
    assert(temperatura == repetida);

    /* Todas as trocas melhoram: aceita a temperatura de partida. */
    rota[0].n = 2; rota[1].n = 1;
    assert(calculaTemperaturaInicial(&inst, &s, &p, &temperatura));
    assert(temperatura == p.tempInicial);
    assert(s.makespan == 123 && rota[0].n == 2);

    temperatura = -1;
    p.beta = 1;
    assert(!calculaTemperaturaInicial(&inst, &s, &p, &temperatura));
    assert(temperatura == -1);
    p = parametrosSAPadrao();
    rota[0].n = 1; rota[1].n = 1;
    assert(!calculaTemperaturaInicial(&inst, &s, &p, &temperatura));
    assert(temperatura == -1);
    inst.n = 2; s.numMissoesCaminhao = 1;
    assert(!calculaTemperaturaInicial(&inst, &s, &p, &temperatura));
    assert(temperatura == -1);
    assert(!calculaTemperaturaInicial(NULL, &s, &p, &temperatura));
    assert(!calculaTemperaturaInicial(&inst, NULL, &p, &temperatura));
    assert(!calculaTemperaturaInicial(&inst, &s, &p, NULL));
    return 0;
}
