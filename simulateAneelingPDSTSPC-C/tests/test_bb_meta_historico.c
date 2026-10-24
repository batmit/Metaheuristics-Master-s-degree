#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "SA.h"

static void preparar(const Instancia *inst, Solucao *s, int reversa, int k)
{
    s->numMissoesCaminhao=2; s->numMissoesDrone=2;
    s->missaoCaminhao[0].n=reversa?4:3; s->missaoCaminhao[1].n=reversa?3:4;
    s->missaoDrone[0]=(MissaoDrone){1,k,0,0};
    s->missaoDrone[1]=(MissaoDrone){2,k,0,0};
    assert(escalonarMissoesDrone(inst,s));
    assert(calcularMakespan(inst,s));
}

int main(void)
{
    Cliente clientes[5]={0}; double tc[5][5]={{0}}, td[5][3];
    double *caminhao[5], *drones[5];
    for(int i=0;i<5;i++) {
        caminhao[i]=tc[i]; drones[i]=td[i];
        for(int k=0;k<3;k++) td[i][k]=INFINITY;
        clientes[i].elegivelDrone=i>0 && i<3; clientes[i].minDrones=1;
    }
    td[1][1]=td[2][1]=6; td[1][2]=td[2][2]=4;
    tc[0][3]=2; tc[3][4]=3; tc[4][0]=2;
    Instancia inst={.n=5,.numDrones=2,.clientes=clientes,.tempoDrone=drones,.tempoCaminhao=caminhao};
    ResultadoBB r;
    MissaoDrone missoes[]={{1,2,0,0},{2,2,0,0}};
    assert(escalonarMissoesDroneBBMeta(&inst,missoes,2,1,10,&r)==8);
    assert(r.motivo==BB_META_ATINGIDA && r.nos==0);
    assert(escalonarMissoesDroneBBMeta(&inst,missoes,2,1,-1,&r)==8);
    assert(r.motivo==BB_LIMITE_NOS && r.nos==1);
    missoes[0].qtdDrones=missoes[1].qtdDrones=1;
    assert(escalonarMissoesDroneBBMeta(&inst,missoes,2,1,-1,&r)==6);
    assert(r.motivo==BB_OTIMO && r.nos==1 && r.limiteInferior==6);
    missoes[0].qtdDrones=missoes[1].qtdDrones=2;
    assert(escalonarMissoesDroneBBMeta(&inst,missoes,2,1000,7,&r)==6);
    assert(r.motivo==BB_META_ATINGIDA);
    assert(escalonarMissoesDroneBBMeta(&inst,missoes,2,1000,5,&r)==6);
    assert(r.motivo==BB_OTIMO);
    MissaoDrone copia[2]; memcpy(copia,missoes,sizeof copia);
    assert(isinf(escalonarMissoesDroneBBMeta(&inst,missoes,2,1000,NAN,&r)));
    assert(r.motivo==BB_ERRO && memcmp(copia,missoes,sizeof copia)==0);

    Solucao elite[NUM_MELHORES_SA]={0};
    for(int i=0;i<NUM_MELHORES_SA;i++) alocarSolucao(&inst,&elite[i]);
    HistoricoBBSA h={0}; int quantidade=1;
    preparar(&inst,&elite[0],0,2); /* Caminhao=7: pode parar com drones=6. */
    assert(refinarEliteComHistoricoSA(&inst,elite,&quantidade,1000,&h)==1);
    assert(h.execucoes==1 && h.quantidade==1 && elite[0].tempoDrones==6);
    /* Nova rota exige melhor limite: termina a prova do subproblema em cache. */
    elite[0].missaoCaminhao[0].n=4; elite[0].missaoCaminhao[1].n=3;
    assert(calcularMakespan(&inst,&elite[0]) && elite[0].tempoCaminhao==0);
    assert(refinarEliteComHistoricoSA(&inst,elite,&quantidade,1000,&h)==1);
    assert(h.execucoes==2 && h.quantidade==1);
    assert(refinarEliteComHistoricoSA(&inst,elite,&quantidade,1000,&h)==0);
    /* Mesmo subproblema reaparece apos sair do arquivo, com outras quantidades. */
    for(int tentativa=0;tentativa<10;tentativa++) {
        preparar(&inst,&elite[0],1,2);
        assert(refinarEliteComHistoricoSA(&inst,elite,&quantidade,1000,&h)==1);
        assert(elite[0].tempoDrones==6 && elite[0].branchBoundAplicado);
        assert(h.execucoes==2 && h.quantidade==1);
    }
    assert(h.reutilizacoes>=11);
    preparar(&inst,&elite[0],1,2);
    elite[0].missaoDrone[0].cliente=2; elite[0].missaoDrone[1].cliente=1;
    assert(escalonarMissoesDrone(&inst,&elite[0])); assert(calcularMakespan(&inst,&elite[0]));
    assert(refinarEliteComHistoricoSA(&inst,elite,&quantidade,1000,&h)==1);
    assert(h.execucoes==3 && h.quantidade==2); /* Ordem diferente e outra chave. */
    Instancia outra=inst;
    assert(refinarEliteComHistoricoSA(&outra,elite,&quantidade,1000,&h)==-1);
    liberarHistoricoBBSA(&h);
    assert(h.entradas==NULL && h.quantidade==0);
    preparar(&inst,&elite[0],1,2);
    assert(refinarEliteComHistoricoSA(&inst,elite,&quantidade,1,&h)==1);
    assert(elite[0].tempoDrones==8 && h.execucoes==1);
    assert(refinarEliteComHistoricoSA(&inst,elite,&quantidade,1,&h)==0);
    assert(refinarEliteComHistoricoSA(&inst,elite,&quantidade,1000,&h)==1);
    assert(elite[0].tempoDrones==6 && h.execucoes==2);
    liberarHistoricoBBSA(&h);
    for(int i=0;i<NUM_MELHORES_SA;i++) liberarSolucao(&elite[i]);
    return 0;
}
