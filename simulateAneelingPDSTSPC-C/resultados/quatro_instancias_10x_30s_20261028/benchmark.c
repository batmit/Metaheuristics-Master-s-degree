#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SA.h"
#include "lerInstancias.h"

int main(int argc,char **argv)
{
    if(argc!=5)return 2;
    const char *name=argv[1];unsigned seed=(unsigned)strtoul(argv[2],NULL,10);
    double budget=strtod(argv[3],NULL);char path[256];Instancia in={0};Solucao s={0};
    srand(seed);snprintf(path,sizeof path,"instances/pdstsp-c/large/%s.txt",name);
    if(!lerInstancia(path,&in))return 3;
    alocarInstancias(&in);construirMatrizCaminhao(&in);
    snprintf(path,sizeof path,"instances/pdstsp-c/large/tau_kj_%s.txt",name);
    if(!lerTemposDrone(path,&in))return 4;
    alocarSolucao(&in,&s);if(!construirSolucaoInicial(&in,&s))return 5;
    int focus=!strcmp(name,"165-r-c");
    ParametrosSA p=focus?parametrosSAFoco165():parametrosSAExploracao();
    p.limiteSegundos=budget;EstatisticasSA stats={0};p.estatisticas=&stats;
    double initial=s.makespan,temp,start=relogioSA();
    if(!resolverComCalibracaoSA(&in,&s,&p,&temp))return 6;
    double seconds=relogioSA()-start;FILE *f=fopen(argv[4],"w");if(!f)return 7;
    fprintf(f,"{\"instance\":\"%s\",\"profile\":\"%s\",\"seed\":%u,\"budget\":%.9g,\"seconds\":%.9g,\"initial\":%.17g,\"temperature\":%.17g,\"claimed\":%.17g,\"truck\":%.17g,\"drone\":%.17g,\"evaluations\":%llu,\"restarts\":%llu,\"repair\":{\"proposals\":%llu,\"load_filters\":%llu,\"route_filters\":%llu,\"improvements\":%llu,\"cp_calls\":%llu,\"cp_success\":%llu,\"cp_infeasible\":%llu,\"cp_timeout\":%llu,\"cp_errors\":%llu,\"native_seconds\":%.9g,\"cp_seconds\":%.9g},\"route\":[",
      name,focus?"foco165":"exploracao",seed,budget,seconds,initial,temp,s.makespan,s.tempoCaminhao,s.tempoDrones,stats.avaliacoes,stats.retomadas,
      stats.reparo.propostas,stats.reparo.podasCarga,stats.reparo.podasRota,stats.reparo.melhorias,stats.reparo.chamadasCP,stats.reparo.sucessosCP,stats.reparo.inviaveisCP,stats.reparo.limitesCP,stats.reparo.errosCP,stats.reparo.segundosNativo,stats.reparo.segundosCP);
    for(int i=0;i<s.numMissoesCaminhao;i++)fprintf(f,"%s%d",i?",":"",s.missaoCaminhao[i].n);
    fputs("],\"missions\":[",f);
    for(int i=0;i<s.numMissoesDrone;i++){MissaoDrone m=s.missaoDrone[i];fprintf(f,"%s{\"client\":%d,\"k\":%d,\"start\":%.17g,\"end\":%.17g}",i?",":"",m.cliente,m.qtdDrones,m.tempoInicio,m.tempoFim);}
    fputs("]}\n",f);int error=ferror(f);if(fclose(f))error=1;
    liberarSolucao(&s);liberarInstancia(&in);return error?8:0;
}
