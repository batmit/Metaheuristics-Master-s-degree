#include <assert.h>
#include <stdlib.h>
#include "SA.h"
#include "cv.h"
#include "lerInstancias.h"

int main(void)
{
    Instancia inst = {0};
    Solucao s = {0};
    assert(lerInstancia("18-r-e.txt", &inst));
    alocarInstancias(&inst); construirMatrizCaminhao(&inst);
    assert(lerTemposDrone("tau_kj_18-r-e.txt", &inst));
    alocarSolucao(&inst, &s);
    ParametrosSA p = parametrosSAPadrao();
    double resultado = 0;
    for (int repeticao = 0; repeticao < 2; repeticao++) {
        assert(construirSolucaoInicial(&inst, &s));
        double inicial = s.makespan, temperatura;
        srand(123);
        assert(calculaTemperaturaInicial(&inst, &s, &p, &temperatura));
        ParametrosSA calibrados = p; calibrados.tempInicial = temperatura;
        assert(simulatedAnnealing(&inst, &s, &calibrados));
        assert(s.makespan <= inicial);
        double obtido = s.makespan;
        assert(calcularMakespan(&inst, &s) && s.makespan == obtido);
        if (repeticao == 0) resultado = obtido;
        else assert(resultado == obtido);
    }
    p.alpha = 1;
    assert(!simulatedAnnealing(&inst, &s, &p));
    assert(s.makespan == resultado);
    liberarSolucao(&s); liberarInstancia(&inst);

    /* Dois clientes, apenas troca: otimo conhecido 3, outra rota custa 30.
     * Temperatura alta aceita pioras, mas o retorno deve preservar o otimo. */
    double a[] = {0,1,10}, b[] = {10,0,1}, c[] = {1,10,0};
    double *tempos[] = {a,b,c};
    Instancia pequena = {0}; pequena.n = 3; pequena.tempoCaminhao = tempos;
    alocarSolucao(&pequena, &s);
    assert(construirSolucaoInicial(&pequena, &s));
    p = parametrosSAPadrao(); p.tempInicial = 1000; p.alpha = 0.5; p.saMax = 20;
    srand(123);
    assert(simulatedAnnealing(&pequena, &s, &p));
    assert(s.makespan == 3);
    assert(s.missaoCaminhao[0].n == 1 && s.missaoCaminhao[1].n == 2);
    /* Sem movimento: um cliente obrigatorio no caminhao. */
    pequena.n = 2;
    assert(construirSolucaoInicial(&pequena, &s));
    assert(simulatedAnnealing(&pequena, &s, &p) && s.makespan == 11);
    pequena.n = 1;
    assert(construirSolucaoInicial(&pequena, &s));
    assert(simulatedAnnealing(&pequena, &s, &p) && s.makespan == 0);
    liberarSolucao(&s);
    return 0;
}
