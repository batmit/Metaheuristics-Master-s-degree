#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "SA.h"
#include "lerInstancias.h"
/* Calibracao mista reproduzivel em copia, com rejeicao de referencia invalida. */
int main(void) {
    Instancia in={0};Solucao s={0};
    assert(lerInstancia("instances/pdstsp-c/small/18-r-e.txt",&in));
    alocarInstancias(&in);construirMatrizCaminhao(&in);
    assert(lerTemposDrone("instances/pdstsp-c/small/tau_kj_18-r-e.txt",&in));
    alocarSolucao(&in,&s);assert(construirSolucaoInicial(&in,&s));
    Missao *copia=malloc((size_t)s.numMissoesCaminhao*sizeof *copia);assert(copia);
    memcpy(copia,s.missaoCaminhao,(size_t)s.numMissoesCaminhao*sizeof *copia);
    double valor=s.makespan,a=-1,b=-1;ParametrosSA p=parametrosSARapido();
    srand(123);assert(calculaTemperaturaInicial(&in,&s,&p,&a));
    srand(123);assert(calculaTemperaturaInicial(&in,&s,&p,&b));
    assert(a==b&&isfinite(a)&&a>p.tempFinal&&s.makespan==valor);
    assert(!memcmp(copia,s.missaoCaminhao,(size_t)s.numMissoesCaminhao*sizeof *copia));
    p.gamma=1;assert(!calculaTemperaturaInicial(&in,&s,&p,&b));
    p=parametrosSARapido();s.missaoCaminhao[0].n=s.missaoCaminhao[1].n;
    assert(!calculaTemperaturaInicial(&in,&s,&p,&b));
    assert(!calculaTemperaturaInicial(NULL,&s,&p,&b));
    assert(!calculaTemperaturaInicial(&in,NULL,&p,&b));
    assert(!calculaTemperaturaInicial(&in,&s,&p,NULL));
    free(copia);liberarSolucao(&s);liberarInstancia(&in);return 0;
}
