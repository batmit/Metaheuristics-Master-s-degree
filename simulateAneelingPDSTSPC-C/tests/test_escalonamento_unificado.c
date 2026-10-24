#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "cv.h"

int main(void)
{
    Cliente clientes[4] = {0};
    double t0[] = {INFINITY,INFINITY,INFINITY,INFINITY};
    double t1[] = {INFINITY,10,INFINITY,INFINITY};
    double t2[] = {INFINITY,INFINITY,INFINITY,5};
    double t3[] = {INFINITY,INFINITY,8,INFINITY};
    double *td[] = {t0,t1,t2,t3};
    double tc[4][4] = {{0}}; double *caminhao[] = {tc[0],tc[1],tc[2],tc[3]};
    for (int i=1;i<4;i++) clientes[i].elegivelDrone=true;
    clientes[1].minDrones=1; clientes[2].minDrones=3; clientes[3].minDrones=2;
    Instancia inst={.n=4,.numDrones=3,.clientes=clientes,.tempoDrone=td,.tempoCaminhao=caminhao};
    Solucao atual={0}, vizinho={0}, completa={0};
    alocarSolucao(&inst,&atual); alocarSolucao(&inst,&vizinho); alocarSolucao(&inst,&completa);
    atual.numMissoesDrone=3;
    atual.missaoDrone[0]=(MissaoDrone){1,1,0,0};
    atual.missaoDrone[1]=(MissaoDrone){2,3,0,0};
    atual.missaoDrone[2]=(MissaoDrone){3,2,0,0};
    assert(escalonarMissoesDrone(&inst,&atual));
    assert(calcularMakespan(&inst,&atual) && atual.makespan==15);
    assert(atual.missaoDrone[2].tempoInicio==0 && atual.missaoDrone[2].tempoFim==8);
    assert(atual.prefixosDrone[0]==0 && atual.prefixosDrone[1]==10 &&
           atual.prefixosDrone[2]==15 && atual.prefixosDrone[3]==15);
    assert(escalonarMissoesDroneBB(&inst,atual.missaoDrone,3)==15);
    MissaoDrone horarios[3]; memcpy(horarios,atual.missaoDrone,sizeof horarios);
    assert(escalonarMissoesDrone(&inst,&atual));
    assert(atual.tempoDrones==15);
    for(int i=0;i<3;i++) {
        assert(atual.missaoDrone[i].tempoInicio==horarios[i].tempoInicio);
        assert(atual.missaoDrone[i].tempoFim==horarios[i].tempoFim);
    }
    assert(calcularMakespan(&inst,&atual));
    srand(321);
    for(int passo=0;passo<1000;passo++) {
        assert(gerarVizinhoAvaliado(&inst,&atual,&vizinho));
        completa.numMissoesCaminhao=vizinho.numMissoesCaminhao;
        completa.numMissoesDrone=vizinho.numMissoesDrone;
        memcpy(completa.missaoCaminhao,vizinho.missaoCaminhao,vizinho.numMissoesCaminhao*sizeof(Missao));
        memcpy(completa.missaoDrone,vizinho.missaoDrone,vizinho.numMissoesDrone*sizeof(MissaoDrone));
        assert(escalonarMissoesDrone(&inst,&completa));
        assert(calcularMakespan(&inst,&completa));
        assert(completa.makespan==vizinho.makespan);
        for(int i=0;i<vizinho.numMissoesDrone;i++) {
            assert(vizinho.missaoDrone[i].tempoInicio==completa.missaoDrone[i].tempoInicio);
            assert(vizinho.missaoDrone[i].tempoFim==completa.missaoDrone[i].tempoFim);
        }
        Solucao aux=atual; atual=vizinho; vizinho=aux;
    }
    liberarSolucao(&atual); liberarSolucao(&vizinho); liberarSolucao(&completa);
    return 0;
}
