#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "SA.h"
#include "lerInstancias.h"

/* Executar da raiz do projeto, com o mesmo runtime C da execucao original. */
int main(void)
{
    Instancia inst = {0};
    Solucao s = {0};
    const unsigned int seed = 1792860329u;
    assert(lerInstancia("instances/pdstsp-c/large/105-rc-e.txt", &inst));
    alocarInstancias(&inst);
    construirMatrizCaminhao(&inst);
    assert(lerTemposDrone("instances/pdstsp-c/large/tau_kj_105-rc-e.txt", &inst));
    alocarSolucao(&inst, &s);
    srand(seed);
    assert(construirSolucaoInicial(&inst, &s));
    ParametrosSA p = parametrosSAPadrao();
    double temperatura;
    assert(calculaTemperaturaInicial(&inst, &s, &p, &temperatura));
    p.tempInicial = temperatura;
    assert(simulatedAnnealing(&inst, &s, &p));
    assert(calcularMakespan(&inst, &s));
    printf("FINAL 105-rc-e %u T0=%.9f MAKESPAN=%.9f TRUCK=%.9f DRONES=%.9f ROUTE",
           seed, temperatura, s.makespan, s.tempoCaminhao, s.tempoDrones);
    for (int i = 0; i < s.numMissoesCaminhao; i++) printf(" %d", s.missaoCaminhao[i].n);
    printf(" MISSIONS");
    for (int i = 0; i < s.numMissoesDrone; i++) {
        MissaoDrone m = s.missaoDrone[i];
        printf(" %d:%d:%.9f:%.9f", m.cliente, m.qtdDrones, m.tempoInicio, m.tempoFim);
    }
    puts("");
    liberarSolucao(&s);
    liberarInstancia(&inst);
    return 0;
}
