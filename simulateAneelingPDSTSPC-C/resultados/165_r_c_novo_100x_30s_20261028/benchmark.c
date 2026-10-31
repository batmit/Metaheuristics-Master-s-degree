#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "SA.h"
#include "lerInstancias.h"
int main(int argc,char **argv){
    assert(argc==5);unsigned seed=strtoul(argv[1],NULL,10);double budget=strtod(argv[2],NULL);
    Instancia in={0};Solucao s={0};assert(lerInstancia("instances/pdstsp-c/large/165-r-c.txt",&in));
    alocarInstancias(&in);construirMatrizCaminhao(&in);assert(lerTemposDrone("instances/pdstsp-c/large/tau_kj_165-r-c.txt",&in));alocarSolucao(&in,&s);
    srand(seed);
    if(atoi(argv[3])){
        FILE *f=fopen("resultados/analise_165_20261028/165-r-c_best.txt","r");assert(f);assert(fscanf(f,"%d%d",&s.numMissoesCaminhao,&s.numMissoesDrone)==2);
        for(int i=0;i<s.numMissoesCaminhao;i++)assert(fscanf(f,"%d",&s.missaoCaminhao[i].n)==1);
        for(int i=0;i<s.numMissoesDrone;i++)assert(fscanf(f,"%d%d",&s.missaoDrone[i].cliente,&s.missaoDrone[i].qtdDrones)==2);
        fclose(f);assert(escalonarMissoesDrone(&in,&s)&&calcularMakespan(&in,&s));
    }else assert(construirSolucaoInicial(&in,&s));
    double initial=s.makespan;
#ifdef CONTROLE
    ParametrosSA p=parametrosSAExploracao();
#else
    ParametrosSA p=parametrosSAFoco165();
#endif
    p.limiteSegundos=budget;EstatisticasSA stats={0};p.estatisticas=&stats;double temp,start=relogioSA();assert(resolverComCalibracaoSA(&in,&s,&p,&temp));
    FILE *f=fopen(argv[4],"w");assert(f);
    fprintf(f,"{\"instance\":\"165-r-c\",\"seed\":%u,\"initial\":%.17g,\"seconds\":%.9g,\"budget\":%.9g,\"claimed\":%.17g,\"truck\":%.17g,\"drone\":%.17g,\"evaluations\":%llu",seed,initial,relogioSA()-start,budget,s.makespan,s.tempoCaminhao,s.tempoDrones,stats.avaliacoes);
#ifndef CONTROLE
    fprintf(f,",\"repair\":{\"proposals\":%llu,\"load_filters\":%llu,\"route_filters\":%llu,\"improvements\":%llu,\"cp_calls\":%llu,\"cp_success\":%llu,\"cp_infeasible\":%llu,\"cp_timeout\":%llu,\"cp_errors\":%llu,\"native_seconds\":%.9g,\"cp_seconds\":%.9g}",stats.reparo.propostas,stats.reparo.podasCarga,stats.reparo.podasRota,stats.reparo.melhorias,stats.reparo.chamadasCP,stats.reparo.sucessosCP,stats.reparo.inviaveisCP,stats.reparo.limitesCP,stats.reparo.errosCP,stats.reparo.segundosNativo,stats.reparo.segundosCP);
#endif
    fputs(",\"route\":[",f);for(int i=0;i<s.numMissoesCaminhao;i++)fprintf(f,"%s%d",i?",":"",s.missaoCaminhao[i].n);
    fputs("],\"missions\":[",f);for(int i=0;i<s.numMissoesDrone;i++){MissaoDrone m=s.missaoDrone[i];fprintf(f,"%s{\"client\":%d,\"k\":%d,\"start\":%.17g,\"end\":%.17g}",i?",":"",m.cliente,m.qtdDrones,m.tempoInicio,m.tempoFim);}fputs("]}\n",f);fclose(f);liberarSolucao(&s);liberarInstancia(&in);
}
