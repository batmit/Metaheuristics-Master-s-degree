#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "SA.h"
#include "lerInstancias.h"

int main(int argc, char **argv)
{
    if(argc!=9) return 2;
    const char *nome=argv[1], *pasta=atoi(nome)<=30?"small":"large";
    unsigned seed=(unsigned)strtoul(argv[2],NULL,10), mascara=(unsigned)strtoul(argv[5],NULL,10);
    int primeiro=atoi(argv[3]), total=atoi(argv[4]);
    double budget=strtod(argv[6],NULL);
    unsigned long long avaliacoes=strtoull(argv[7],NULL,10);
    FILE *out=fopen(argv[8],"w"); if(!out) return 3;
    for(int i=primeiro;i<primeiro+total;i++) {
        Instancia inst={0};Solucao s={0}; char caminho[256];
        srand(seed+(unsigned)i);
        snprintf(caminho,sizeof caminho,"instances/pdstsp-c/%s/%s.txt",pasta,nome);
        if(!lerInstancia(caminho,&inst)) return 4;
        alocarInstancias(&inst);construirMatrizCaminhao(&inst);
        snprintf(caminho,sizeof caminho,"instances/pdstsp-c/%s/tau_kj_%s.txt",pasta,nome);
        if(!lerTemposDrone(caminho,&inst)) return 5;
        alocarSolucao(&inst,&s);
        clock_t inicio=clock();
        int ok=construirSolucaoInicial(&inst,&s);
        if(!ok) return 6;
        double inicial=s.makespan,temperatura;
        ParametrosSA p=parametrosSAPadrao();p.movimentosGrupo=mascara!=0;
        if(!calculaTemperaturaInicial(&inst,&s,&p,&temperatura)) return 7;
        p.tempInicial=temperatura; (void)avaliacoes;
        if(budget!=0) return 11;
        if(!simulatedAnnealing(&inst,&s,&p)) return 8;
        double segundos=(double)(clock()-inicio)/CLOCKS_PER_SEC;
        fprintf(out,"{\"instance\":\"%s\",\"run\":%d,\"seed\":%u,\"mask\":%u,\"budget\":%.6f,\"seconds\":%.6f,\"initial\":%.17g,\"temperature\":%.17g,\"claimed\":%.17g,\"truck\":%.17g,\"drone\":%.17g,\"route\":[",
                nome,i+1,seed+(unsigned)i,mascara,budget,segundos,inicial,temperatura,s.makespan,s.tempoCaminhao,s.tempoDrones);
        for(int j=0;j<s.numMissoesCaminhao;j++) fprintf(out,"%s%d",j?",":"",s.missaoCaminhao[j].n);
        fputs("],\"missions\":[",out);
        for(int j=0;j<s.numMissoesDrone;j++) {
            MissaoDrone *m=&s.missaoDrone[j];
            fprintf(out,"%s{\"client\":%d,\"k\":%d,\"start\":%.17g,\"end\":%.17g}",j?",":"",m->cliente,m->qtdDrones,m->tempoInicio,m->tempoFim);
        }
        fputs("]}\n",out);fflush(out);if(ferror(out)) return 9;
        liberarSolucao(&s);liberarInstancia(&inst);
    }
    return fclose(out)?10:0;
}
