#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "SA.h"
#include "lerInstancias.h"
int main(int argc,char **argv){
    assert(argc==5);unsigned seed=strtoul(argv[1],NULL,10);int mode=atoi(argv[2]);
    Instancia in={0};Solucao s={0};assert(lerInstancia("instances/pdstsp-c/large/165-r-c.txt",&in));
    alocarInstancias(&in);construirMatrizCaminhao(&in);assert(lerTemposDrone("instances/pdstsp-c/large/tau_kj_165-r-c.txt",&in));alocarSolucao(&in,&s);
    FILE *f=fopen(argv[3],"r");assert(f);assert(fscanf(f,"%d%d",&s.numMissoesCaminhao,&s.numMissoesDrone)==2);
    for(int i=0;i<s.numMissoesCaminhao;i++)assert(fscanf(f,"%d",&s.missaoCaminhao[i].n)==1);
    for(int i=0;i<s.numMissoesDrone;i++)assert(fscanf(f,"%d%d",&s.missaoDrone[i].cliente,&s.missaoDrone[i].qtdDrones)==2);
    fclose(f);assert(escalonarMissoesDrone(&in,&s)&&calcularMakespan(&in,&s));
    srand(seed);ParametrosSA p=parametrosSAExploracao();p.limiteSegundos=15;
    if(mode==1){p.probTroca=.05;p.probReconstrucao=.05;}
    if(mode==2){p.fracaoMinima=.15;p.fracaoMaxima=.30;}
    EstatisticasSA stats={0};p.estatisticas=&stats;double temp,start=relogioSA();assert(resolverComCalibracaoSA(&in,&s,&p,&temp));
    f=fopen(argv[4],"w");assert(f);
    fprintf(f,"{\"instance\":\"165-r-c\",\"seed\":%u,\"mode\":%d,\"budget\":15,\"seconds\":%.9g,\"claimed\":%.17g,\"truck\":%.17g,\"drone\":%.17g,\"evaluations\":%llu,\"exchanges\":%llu,\"reconstructions\":%llu,\"restarts\":%llu,\"route\":[",seed,mode,relogioSA()-start,s.makespan,s.tempoCaminhao,s.tempoDrones,stats.avaliacoes,stats.trocas,stats.reconstrucoes,stats.retomadas);
    for(int i=0;i<s.numMissoesCaminhao;i++)fprintf(f,"%s%d",i?",":"",s.missaoCaminhao[i].n);
    fputs("],\"missions\":[",f);
    for(int i=0;i<s.numMissoesDrone;i++){MissaoDrone m=s.missaoDrone[i];fprintf(f,"%s{\"client\":%d,\"k\":%d,\"start\":%.17g,\"end\":%.17g}",i?",":"",m.cliente,m.qtdDrones,m.tempoInicio,m.tempoFim);}
    fputs("]}\n",f);fclose(f);liberarSolucao(&s);liberarInstancia(&in);
}
