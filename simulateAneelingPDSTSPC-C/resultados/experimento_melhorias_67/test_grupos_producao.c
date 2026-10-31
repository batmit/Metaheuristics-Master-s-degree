#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include "SA.h"
#include "lerInstancias.h"

static void conferir(const Instancia *inst, Solucao *s)
{
    double valor=s->makespan;
    assert(calcularMakespan(inst,s) && fabs(valor-s->makespan)<1e-9);
    double drones=s->tempoDrones;
    assert(escalonarMissoesDrone(inst,s));
    assert(calcularMakespan(inst,s) && fabs(drones-s->tempoDrones)<1e-9);
}

int main(void)
{
    /* Transferencias completas: nenhum cliente no caminhao ou nos drones. */
    double tc[4][4],td[4][3];double *ct[4],*dt[4];Cliente clientes[4]={0};
    for(int i=0;i<4;i++) {
        ct[i]=tc[i];dt[i]=td[i];
        for(int j=0;j<4;j++) tc[i][j]=i==j?0:2;
        td[i][0]=INFINITY;td[i][1]=3;td[i][2]=2;
        if(i) {clientes[i].elegivelDrone=true;clientes[i].minDrones=1;clientes[i].x=1;}
    }
    Instancia pequena={.n=4,.numDrones=2,.clientes=clientes,.tempoCaminhao=ct,.tempoDrone=dt};
    Solucao a={0},b={0};alocarSolucao(&pequena,&a);alocarSolucao(&pequena,&b);
    assert(construirSolucaoInicial(&pequena,&a));assert(escalonarMissoesDrone(&pequena,&a));
    assert(calcularMakespan(&pequena,&a));
    double pesos[NUM_MOVIMENTOS];for(int i=0;i<NUM_MOVIMENTOS;i++) pesos[i]=1e-20;
    pesos[PARA_DRONES]=1;srand(9841);int completos=0,quantidades[3]={0};
    for(int i=0;i<200;i++) {
        assert(gerarVizinhoAvaliadoConfigurado(&pequena,&a,&b,pesos,NULL,true));conferir(&pequena,&b);
        if(b.numMissoesCaminhao==0) {
            completos++;
            for(int j=0;j<b.numMissoesDrone;j++) quantidades[b.missaoDrone[j].qtdDrones]++;
        }
        assert(a.numMissoesCaminhao==3 && a.numMissoesDrone==0 && a.makespan==8);
    }
    assert(completos>0 && quantidades[1]>0 && quantidades[2]>0);
    a.numMissoesCaminhao=0;a.numMissoesDrone=3;
    for(int i=0;i<3;i++) a.missaoDrone[i]=(MissaoDrone){i+1,1,0,0};
    assert(escalonarMissoesDrone(&pequena,&a));assert(calcularMakespan(&pequena,&a));
    pesos[PARA_DRONES]=1e-20;pesos[PARA_CAMINHAO]=1;completos=0;
    for(int i=0;i<200;i++) {
        assert(gerarVizinhoAvaliadoConfigurado(&pequena,&a,&b,pesos,NULL,true));conferir(&pequena,&b);
        completos+=b.numMissoesDrone==0;
    }
    assert(completos>0);liberarSolucao(&a);liberarSolucao(&b);

    /* Uma caminhada longa verifica deltas, cobertura, cronograma e prefixos. */
    Instancia inst={0};assert(lerInstancia("instances/pdstsp-c/large/67-c-c.txt",&inst));
    alocarInstancias(&inst);construirMatrizCaminhao(&inst);
    assert(lerTemposDrone("instances/pdstsp-c/large/tau_kj_67-c-c.txt",&inst));
    alocarSolucao(&inst,&a);alocarSolucao(&inst,&b);
    assert(construirSolucaoInicial(&inst,&a));assert(escalonarMissoesDrone(&inst,&a));
    assert(calcularMakespan(&inst,&a));srand(731);
    for(int i=0;i<3000;i++) {
        assert(gerarVizinhoAvaliadoConfigurado(&inst,&a,&b,NULL,NULL,true));conferir(&inst,&b);
        Solucao aux=a;a=b;b=aux;
    }
    liberarSolucao(&a);liberarSolucao(&b);liberarInstancia(&inst);return 0;
}
