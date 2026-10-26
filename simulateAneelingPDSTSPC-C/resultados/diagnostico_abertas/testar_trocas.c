
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "cv.h"
#include "lerInstancias.h"
static void copy(const Solucao *a,Solucao *b) {
    b->numMissoesCaminhao=a->numMissoesCaminhao;b->numMissoesDrone=a->numMissoesDrone;
    for(int i=0;i<a->numMissoesCaminhao;i++)b->missaoCaminhao[i]=a->missaoCaminhao[i];
    for(int i=0;i<a->numMissoesDrone;i++)b->missaoDrone[i]=a->missaoDrone[i];
}
static void run(const char *name,const char *dir,int nc,const int *route,int nd,const int (*mis)[2]) {
    char path[256];Instancia in={0};Solucao base={0},candidate={0},best={0};
    snprintf(path,sizeof path,"instances/pdstsp-c/%s/%s.txt",dir,name);
    assert(lerInstancia(path,&in));alocarInstancias(&in);construirMatrizCaminhao(&in);
    snprintf(path,sizeof path,"instances/pdstsp-c/%s/tau_kj_%s.txt",dir,name);
    assert(lerTemposDrone(path,&in));alocarSolucao(&in,&base);alocarSolucao(&in,&candidate);alocarSolucao(&in,&best);
    base.numMissoesCaminhao=nc;base.numMissoesDrone=nd;
    for(int i=0;i<nc;i++)base.missaoCaminhao[i].n=route[i];
    for(int i=0;i<nd;i++)base.missaoDrone[i]=(MissaoDrone){mis[i][0],mis[i][1],0,0};
    assert(escalonarMissoesDrone(&in,&base)&&calcularMakespan(&in,&base));
    copy(&base,&best);assert(escalonarMissoesDrone(&in,&best)&&calcularMakespan(&in,&best));
    int removed=-1,added=-1;unsigned long long evaluated=0;
    int *remaining=malloc((size_t)nc*sizeof *remaining);assert(remaining);
    for(int i=0;i<nc;i++) {
        int a=base.missaoCaminhao[i].n;
        if(!in.clientes[a].elegivelDrone)continue;
        int prev=i?base.missaoCaminhao[i-1].n:0, next=i+1<nc?base.missaoCaminhao[i+1].n:0;
        for(int q=0,t=0;q<nc;q++)if(q!=i)remaining[t++]=base.missaoCaminhao[q].n;
        for(int j=0;j<nd;j++) {
            int b=base.missaoDrone[j].cliente;
            double reduced=base.tempoCaminhao-in.tempoCaminhao[prev][a]-in.tempoCaminhao[a][next]+in.tempoCaminhao[prev][next];
            double truck=INFINITY;int position=-1;
            for(int q=0;q<nc;q++) {
                int u=q?remaining[q-1]:0,v=q<nc-1?remaining[q]:0;
                double value=reduced+in.tempoCaminhao[u][b]+in.tempoCaminhao[b][v]-in.tempoCaminhao[u][v];
                if(value<truck){truck=value;position=q;}
            }
            if(truck>=best.makespan-1e-9)continue;
            for(int k=in.clientes[a].minDrones;k<=in.numDrones;k++) {
                if(!isfinite(in.tempoDrone[a][k]))continue;
                copy(&base,&candidate);
                for(int q=0;q<nc;q++)candidate.missaoCaminhao[q].n=q<position?remaining[q]:q==position?b:remaining[q-1];
                candidate.missaoDrone[j]=(MissaoDrone){a,k,0,0};evaluated++;
                assert(escalonarMissoesDrone(&in,&candidate)&&calcularMakespan(&in,&candidate));
                if(candidate.makespan<best.makespan-1e-9) {
                    copy(&candidate,&best);assert(escalonarMissoesDrone(&in,&best)&&calcularMakespan(&in,&best));
                    removed=b;added=a;
                }
            }
        }
    }
    printf("CROSS %s %.9f %.9f %.9f %.9f %d %d %llu ROUTE",name,base.makespan,best.makespan,
           best.tempoCaminhao,best.tempoDrones,removed,added,evaluated);
    for(int i=0;i<nc;i++)printf(" %d",best.missaoCaminhao[i].n);
    printf(" MISSIONS");
    for(int i=0;i<nd;i++){MissaoDrone x=best.missaoDrone[i];printf(" %d:%d:%.9f:%.9f",x.cliente,x.qtdDrones,x.tempoInicio,x.tempoFim);}
    puts("");fflush(stdout);free(remaining);liberarSolucao(&base);liberarSolucao(&candidate);liberarSolucao(&best);liberarInstancia(&in);
}
int main(void){
    static const int r1[]={56,67,55,52,54,65,50,64,51,46,47,60,63,61,57,58,45,43,66,62,33,36,34,35,38,31,27,21,16,25,22,19,17,28,12,4,5,3,32,41,39};
    static const int d1[][2]={{29,1},{20,2},{40,3},{6,3},{2,2},{24,2},{48,2},{53,1},{15,4},{30,1},{13,5},{44,1},{37,6},{10,2},{1,2},{9,3},{18,3},{42,1},{26,1},{14,3},{11,1},{59,1},{23,2},{7,2},{8,1},{49,3}};
    run("67-c-c","large",41,r1,26,d1);
    static const int r2[]={97,99,96,103,102,101,14,41,12,58,59,61,55,53,64,63,60,23,4,44,38,21,91,84,89,37,51,82,93,88,94,92,83,90,1,31,6,43,67,22,45,69,78,76,74,32,72,70,71,77,80,75,68,79,73,85,86,81,87,26,25,34,54,62,56,11,65,66,57,28,5,48};
    static const int d2[][2]={{19,4},{16,1},{40,3},{49,1},{47,1},{7,2},{30,2},{15,1},{42,1},{18,2},{24,2},{95,1},{36,1},{29,1},{33,2},{27,1},{98,1},{9,1},{50,2},{52,2},{3,2},{46,2},{39,1},{13,1},{104,2},{100,2},{10,2},{20,2},{17,2},{35,4},{2,2},{8,1},{105,1}};
    run("105-rc-e","large",72,r2,33,d2);
    static const int r3[]={73,125,153,149,134,108,142,126,148,118,144,137,112,129,121,145,124,122,136,116,114,119,143,138,111,128,113,140,151,152,120,150,147,139,154,135,123,133,110,130,146,107,106,131,127,88,62,69,89,57,80,102,78,60,99,67,68,101,95,83,70,100,103,63,58,56,91,92,81,59,84,86,64,94,76,72,79,90,75,34,35,39,25,22,32,15,18,44,11,50,17,23,5,4,9,27,12,6,16,51,52,14,43,46,45,7,42,49,13,8,24,29,48,28,38,47,41,33,36,10,40,37,19};
    static const int d3[][2]={{31,1},{3,1},{105,1},{53,2},{30,1},{54,1},{109,4},{20,2},{77,2},{26,1},{21,2},{97,2},{66,3},{132,1},{82,3},{96,4},{1,1},{61,2},{85,1},{104,4},{93,2},{65,2},{117,1},{87,2},{55,3},{141,1},{2,2},{74,1},{98,3},{115,1},{71,3}};
    run("154-c-c","large",123,r3,31,d3);
    static const int r4[]={115,75,95,87,100,66,76,21,139,63,99,9,90,29,52,8,83,106,61,144,101,68,45,135,85,114,33,133,122,134,71,42,10,59,67,156,64,70,128,47,28,43,116,92,97,132,93,141,120,129,56,15,146,31,65,51,49,113,151,160,150,102,118,89,25,34,123,46,110,103,148,140,125,26,121,98,109};
    static const int d4[][2]={{78,1},{164,1},{18,1},{119,1},{55,1},{80,1},{50,1},{77,3},{163,1},{27,1},{17,1},{81,1},{154,1},{35,1},{7,1},{108,1},{153,1},{30,1},{58,1},{44,1},{138,1},{88,1},{157,4},{105,1},{73,1},{158,1},{126,1},{39,1},{127,2},{22,1},{131,1},{6,1},{11,3},{86,1},{1,1},{69,2},{165,1},{53,1},{161,1},{60,1},{79,1},{5,1},{104,1},{48,1},{130,1},{162,1},{4,1},{84,2},{142,1},{149,1},{20,1},{57,3},{137,1},{91,2},{54,3},{62,2},{82,1},{40,2},{74,1},{38,1},{3,1},{145,1},{96,1},{32,1},{147,1},{112,2},{24,1},{159,1},{19,1},{14,2},{107,4},{12,2},{152,1},{155,1},{36,1},{136,2},{13,1},{117,1},{16,1},{37,1},{111,1},{143,1},{124,2},{41,1},{2,2},{94,4},{72,2},{23,4}};
    run("165-r-c","large",77,r4,88,d4);
    static const int r5[]={20,35,23,17,99,140,158,95,88,50,164,113,26,157,132,110,79,119,167,139,91,86,107,16,51,19,4,155,65,84,78,52,145,152,18,80,127,5,105,37,71,49,162,12,53,118,73,76,69,130,47,68,34,123,125,3,144,55,2,58,56,108,126,25,82,98,43,101,90,22,137,154,33,40,63,93,161,66,39,6,77,57,117,141,112,46,30,29,59,97,136,36,21,149,81,13,10};
    static const int d5[][2]={{100,1},{72,2},{148,3},{24,2},{87,1},{31,1},{27,1},{64,1},{122,1},{94,1},{38,1},{9,1},{133,1},{128,2},{60,2},{15,1},{1,1},{61,3},{11,1},{135,1},{83,3},{120,1},{45,5},{129,2},{163,5},{147,1},{151,1},{103,2},{44,3},{146,2},{159,3},{138,1},{85,2},{67,2},{166,3},{121,2},{114,1},{42,1},{102,2},{48,2},{109,1},{124,2},{74,3},{106,2},{116,1},{92,1},{131,1},{115,1},{7,1},{160,1},{70,2},{32,1},{150,1},{54,1},{143,1},{153,5},{134,1},{89,1},{8,1},{156,2},{14,2},{41,1},{104,1},{28,3},{142,1},{96,2},{165,3},{75,1},{62,2},{111,1}};
    run("167-r-e","large",97,r5,70,d5);
    static const int r6[]={169,52,114,34,166,162,144,118,26,161,100,84,76,158,56,97,173,143,80,12,151,131,99,10,88,25,164,59,22,61,168,40,16,102,60,78,4,19,48,154,178,89,46,85,33,64,67,108,63,7,42,6,58,47,79,150,98,82,139,175,124,53,1,153,128,65,31,96,71,17,125,55,14,109,111,171,176,32,181,140,94,110,35,57,54,138,113,72,130,41,122,69,127,24,145,49,2,91,136,156,121,177,20,134,23,21,73,15,172,142,45,116,83,27,38,174,119};
    static const int d6[][2]={{95,2},{13,4},{133,1},{149,1},{179,1},{147,2},{11,1},{39,1},{129,3},{66,2},{86,1},{37,2},{5,3},{170,1},{29,2},{123,1},{141,3},{92,1},{3,4},{43,2},{126,1},{8,1},{146,1},{36,3},{87,2},{132,2},{106,1},{51,2},{112,1},{81,1},{90,1},{50,1},{107,1},{104,1},{68,2},{105,1},{117,3},{159,1},{163,1},{135,5},{155,1},{62,1},{120,1},{9,1},{148,6},{77,2},{180,1},{167,4},{70,1},{165,2},{18,1},{103,3},{160,2},{152,1},{44,2},{28,2},{75,1},{157,2},{115,2},{137,2},{93,1},{101,1},{74,1},{30,3}};
    run("181-r-e","large",117,r6,64,d6);
return 0;
}
