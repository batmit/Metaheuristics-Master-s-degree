/* Runner isolado: nao le nem atualiza resultados/relatorio.csv. */
#include <stdio.h>
#include <stdlib.h>
#include "SA.h"
#include "lerInstancias.h"
int main(int argc,char **argv) {
    if(argc!=7) return 2;
    const char *grupo=argv[1],*nome=argv[2];
    unsigned seed=(unsigned)strtoul(argv[3],NULL,10);
    int variante=atoi(argv[4]),avaliacoes=atoi(argv[6]);double segundos=atof(argv[5]);
    char path[512];Instancia in={0};Solucao s={0};
    snprintf(path,sizeof path,"instances/pdstsp-c/%s/%s.txt",grupo,nome);
    if(!lerInstancia(path,&in)) return 3;
    alocarInstancias(&in);construirMatrizCaminhao(&in);
    snprintf(path,sizeof path,"instances/pdstsp-c/%s/tau_kj_%s.txt",grupo,nome);
    if(!lerTemposDrone(path,&in)) return 4;
    alocarSolucao(&in,&s);if(!construirSolucaoInicial(&in,&s)) return 5;
    ParametrosSA p=parametrosSARapido();EstatisticasSA e;
    p.estatisticas=&e;p.estagnacaoRecursoBB=variante?5:0;p.limiteSegundos=segundos;
    p.apenasGargaloBBCaminhao=variante==2;
    if(avaliacoes>0) {
        /* Um unico nivel: mesma quantidade de propostas, sem macros/retomadas. */
        p.saMax=avaliacoes;p.alpha=.5;p.tempFinal=6;p.tempInicial=10;
        p.refinoFinalBB=false;
    }
    srand(seed);double inicio=relogioSA(),temperatura=0;
    int ok=avaliacoes>0?simulatedAnnealing(&in,&s,&p):resolverComCalibracaoSA(&in,&s,&p,&temperatura);
    double total=relogioSA()-inicio;
    if(!ok || !calcularMakespan(&in,&s)) return 6;
    printf("{\"makespan\":%.12g,\"caminhao\":%.12g,\"drone\":%.12g,\"segundos\":%.9g,"
        "\"avaliacoes\":%llu,\"chamadas_c\":%llu,\"chamadas_d\":%llu,"
        "\"nos_c\":%llu,\"nos_d\":%llu,\"melhorias_c\":%llu,\"melhorias_d\":%llu,"
        "\"segundos_c\":%.9g,\"segundos_d\":%.9g,\"bloqueios_c\":%llu,\"rota\":[",
        s.makespan,s.tempoCaminhao,s.tempoDrones,total,e.avaliacoes,
        e.chamadasBBCaminhao,e.chamadasBBDrone,e.nosBBCaminhao,e.nosBBDrone,
        e.melhoriasBBCaminhao,e.melhoriasBBDrone,e.segundosBBCaminhao,e.segundosBBDrone,e.bloqueiosBBCaminhao);
    for(int j=0;j<s.numMissoesCaminhao;j++) printf("%s%d",j?",":"",s.missaoCaminhao[j].n);
    printf("],\"missoes\":[");
    for(int j=0;j<s.numMissoesDrone;j++) {
        MissaoDrone m=s.missaoDrone[j];
        printf("%s[%d,%d,%.12g,%.12g]",j?",":"",m.cliente,m.qtdDrones,m.tempoInicio,m.tempoFim);
    }
    printf("]}\n");liberarSolucao(&s);liberarInstancia(&in);return 0;
}
