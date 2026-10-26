"""Ensaio de troca simultanea entre modais, fora do algoritmo de producao."""
from pathlib import Path
import json
HERE=Path(__file__).resolve().parent
data=json.loads((HERE/'medicoes.json').read_text())
c=r'''
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
'''
for i,(name,r) in enumerate(data.items()):
    if name=='20-c-c':continue
    route=r['final']['route'];ms=r['final']['missions']
    c+=f'    static const int r{i}[]={{'+','.join(map(str,route))+'};\n'
    c+=f'    static const int d{i}[][2]={{'+','.join('{'+f'{m["client"]},{m["k"]}'+'}' for m in ms)+'};\n'
    c+=f'    run("{name}","large",{len(route)},r{i},{len(ms)},d{i});\n'
c+='return 0;\n}\n'
(HERE/'testar_trocas.c').write_text(c,encoding='utf-8')
