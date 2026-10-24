#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "cv.h"

int main(void)
{
    double tempos[6][6]; double *matriz[6];
    for(int i=0;i<6;i++) {
        matriz[i]=tempos[i];
        for(int j=0;j<6;j++) tempos[i][j]=i==j?7:20+(i*7+j*11)%13/10.0;
    }
    int ciclo[]={0,3,1,4,2,5,0};
    for(int i=0;i<6;i++) tempos[ciclo[i]][ciclo[i+1]]=1;
    Instancia inst={.n=6,.tempoCaminhao=matriz};
    Solucao atual={0},vizinho={0}; alocarSolucao(&inst,&atual); alocarSolucao(&inst,&vizinho);
    assert(construirSolucaoInicial(&inst,&atual));
    assert(atual.makespan==6);
    for(int i=0;i<5;i++) assert(atual.missaoCaminhao[i].n==ciclo[i+1]);
    assert(escalonarMissoesDrone(&inst,&atual)); assert(calcularMakespan(&inst,&atual));
    atual.branchBoundAplicado=true;
    int vistos[NUM_MOVIMENTOS]={0};
    srand(123);
    for(int passo=0;passo<3000;passo++) {
        TipoMovimento tipo;
        double pesos[NUM_MOVIMENTOS]; for(int i=0;i<NUM_MOVIMENTOS;i++) pesos[i]=1;
        pesos[REINSERIR_CAMINHAO]=pesos[DOIS_OPT_CAMINHAO]=4;
        Missao antes[5]; memcpy(antes,atual.missaoCaminhao,sizeof antes);
        assert(gerarVizinhoAvaliadoPonderado(&inst,&atual,&vizinho,pesos,&tipo));
        assert(tipo==TROCAR_CAMINHAO || tipo==REINSERIR_CAMINHAO || tipo==DOIS_OPT_CAMINHAO);
        vistos[tipo]++;
        assert(memcmp(antes,atual.missaoCaminhao,sizeof antes)==0);
        assert(vizinho.branchBoundAplicado && vizinho.cronogramaValido);
        double incremental=vizinho.makespan;
        assert(calcularMakespan(&inst,&vizinho));
        assert(fabs(incremental-vizinho.makespan)<1e-8);
        Solucao aux=atual; atual=vizinho; vizinho=aux;
    }
    assert(vistos[REINSERIR_CAMINHAO]>500 && vistos[DOIS_OPT_CAMINHAO]>500);
    liberarSolucao(&atual); liberarSolucao(&vizinho); return 0;
}
