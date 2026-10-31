#include <stdio.h>
#include <stdlib.h>
#include "SA.h"
#include "lerInstancias.h"

int main(int argc,char **argv)
{
    if(argc!=6) return 2;
    const char *nome=argv[1];unsigned seed=(unsigned)strtoul(argv[2],NULL,10);
    unsigned mask=(unsigned)strtoul(argv[3],NULL,10);double limite=strtod(argv[4],NULL);
    Instancia inst={0};Solucao s={0};char path[256];
    srand(seed);snprintf(path,sizeof path,"instances/pdstsp-c/large/%s.txt",nome);
    if(!lerInstancia(path,&inst)) return 3;
    alocarInstancias(&inst);construirMatrizCaminhao(&inst);
    snprintf(path,sizeof path,"instances/pdstsp-c/large/tau_kj_%s.txt",nome);
    if(!lerTemposDrone(path,&inst)) return 4;
    alocarSolucao(&inst,&s);if(!construirSolucaoInicial(&inst,&s)) return 5;
    double initial=s.makespan,t;
    ParametrosSA p=parametrosSAPadrao();p.movimentosGrupo=true;
    p.trocasSimultaneas=(mask&1)!=0;p.reconstrucaoParcial=(mask&2)!=0;
    p.calibracaoMista=(mask&4)!=0;p.elitesDiversas=(mask&8)!=0;
    p.resfriamentoPorTempo=p.calibracaoMista;
    if(p.calibracaoMista) p.gamma=.8;
    if(mask&256) p.maxRetomadas=0; p.limiteSegundos=limite;EstatisticasSA e={0};p.estatisticas=&e;
    double inicio=relogioSA();
    if(!resolverComCalibracaoSA(&inst,&s,&p,&t)) return 6;
    double seconds=relogioSA()-inicio;
    FILE *out=fopen(argv[5],"w");if(!out) return 7;
    fprintf(out,"{\"instance\":\"%s\",\"seed\":%u,\"mask\":%u,\"budget\":%.9g,\"seconds\":%.9g,\"initial\":%.17g,\"temperature\":%.17g,\"claimed\":%.17g,\"truck\":%.17g,\"drone\":%.17g,\"stats\":{\"evaluations\":%llu,\"exchanges\":%llu,\"reconstructions\":%llu,\"restarts\":%llu,\"different_assignment_restarts\":%llu,\"elite_assignments\":%d,\"macro_improvements\":%llu,\"calibration_seconds\":%.9g},\"route\":[",
        nome,seed,mask,limite,seconds,initial,t,s.makespan,s.tempoCaminhao,s.tempoDrones,
        e.avaliacoes,e.trocas,e.reconstrucoes,e.retomadas,e.retomadasDeOutraAtribuicao,e.atribuicoesElite,e.melhoriasMacro,e.segundosCalibracao);
    for(int j=0;j<s.numMissoesCaminhao;j++) fprintf(out,"%s%d",j?",":"",s.missaoCaminhao[j].n);
    fputs("],\"missions\":[",out);
    for(int j=0;j<s.numMissoesDrone;j++) {
        MissaoDrone *m=&s.missaoDrone[j];
        fprintf(out,"%s{\"client\":%d,\"k\":%d,\"start\":%.17g,\"end\":%.17g}",j?",":"",m->cliente,m->qtdDrones,m->tempoInicio,m->tempoFim);
    }
    fprintf(out,"],\"final_temperature\":%.17g}\n",e.temperaturaFinal);int erro=ferror(out);if(fclose(out)) erro=1;
    liberarSolucao(&s);liberarInstancia(&inst);return erro?8:0;
}
