#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include "SA.h"
#include "lerInstancias.h"
/* Confere parada natural, estagnacao, macros limitados e bitsets acima de 256 nos. */
int main(void) {
    Instancia in={0};Solucao s={0};
    assert(lerInstancia("instances/pdstsp-c/small/18-r-e.txt",&in));alocarInstancias(&in);construirMatrizCaminhao(&in);
    assert(lerTemposDrone("instances/pdstsp-c/small/tau_kj_18-r-e.txt",&in));alocarSolucao(&in,&s);
    assert(construirSolucaoInicial(&in,&s));double inicial=s.makespan;
    ParametrosSA p=parametrosSARapido();EstatisticasSA e={0};p.estatisticas=&e;
    assert(p.refinoFinalBB && p.niveisParada==400 && p.minimoAvaliacoes==200000);
    p.tempInicial=.02;p.tempFinal=.01;p.alpha=.5;p.saMax=20;p.maxRetomadas=0;
    p.niveisParada=0;p.limiteSegundos=5;
    srand(123);assert(simulatedAnnealing(&in,&s,&p));
    assert(e.avaliacoes==20&&!e.terminouPorTempo&&!e.terminouPorEstagnacao);
    assert(e.temperaturaFinal<=p.tempFinal&&s.makespan<=inicial);
    assert(calcularMakespan(&in,&s));
    assert(construirSolucaoInicial(&in,&s));
    p.tempInicial=10;p.alpha=.99999;p.saMax=100;p.niveisParada=12;p.minimoAvaliacoes=1000;
    srand(123);assert(simulatedAnnealing(&in,&s,&p));
    assert(e.terminouPorEstagnacao&&!e.terminouPorTempo&&e.avaliacoes>=1000);
    assert(e.trocas+e.reconstrucoes<=e.avaliacoes/100);
    assert(calcularMakespan(&in,&s)&&s.makespan<=inicial);
    p.niveisParada=-1;assert(!validarParametrosSA(&p));
    liberarSolucao(&s);liberarInstancia(&in);
    Instancia grande={.n=301};
    MissaoDrone da[]={{.cliente=1},{.cliente=257}},db[]={{.cliente=257},{.cliente=299}};
    Solucao a={.missaoDrone=da,.numMissoesDrone=2},b={.missaoDrone=db,.numMissoesDrone=2};
    assert(distanciaAtribuicaoSA(&grande,&a,&b)==2);
    assert(distanciaAtribuicaoSA(&grande,&a,&a)==0);
    db[0].cliente=301;assert(distanciaAtribuicaoSA(&grande,&a,&b)==-1);
    return 0;
}
