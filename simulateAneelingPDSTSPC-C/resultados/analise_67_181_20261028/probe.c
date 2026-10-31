#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "SA.h"
#include "lerInstancias.h"
static void copy(const Instancia *in,const Solucao *a,Solucao *b){
    b->numMissoesCaminhao=a->numMissoesCaminhao;b->numMissoesDrone=a->numMissoesDrone;
    for(int i=0;i<a->numMissoesCaminhao;i++)b->missaoCaminhao[i]=a->missaoCaminhao[i];
    for(int i=0;i<a->numMissoesDrone;i++)b->missaoDrone[i]=a->missaoDrone[i];
    assert(escalonarMissoesDrone(in,b)&&calcularMakespan(in,b));
}
static void dump(FILE *f,const char *name,const char *mode,const Solucao *s){
    fprintf(f,"{\"instance\":\"%s\",\"mode\":\"%s\",\"seed\":0,\"claimed\":%.17g,\"truck\":%.17g,\"drone\":%.17g,\"route\":[",name,mode,s->makespan,s->tempoCaminhao,s->tempoDrones);
    for(int i=0;i<s->numMissoesCaminhao;i++)fprintf(f,"%s%d",i?",":"",s->missaoCaminhao[i].n);
    fputs("],\"missions\":[",f);
    for(int i=0;i<s->numMissoesDrone;i++){MissaoDrone m=s->missaoDrone[i];fprintf(f,"%s{\"client\":%d,\"k\":%d,\"start\":%.17g,\"end\":%.17g}",i?",":"",m.cliente,m.qtdDrones,m.tempoInicio,m.tempoFim);}
    fputs("]}\n",f);fflush(f);
}
static int min_k(const Instancia *in,int j){int best=0;double area=INFINITY;
    for(int k=1;k<=in->numDrones;k++)if(isfinite(in->tempoDrone[j][k])&&in->tempoDrone[j][k]>=0&&k*in->tempoDrone[j][k]<area){area=k*in->tempoDrone[j][k];best=k;}
    assert(best);return best;
}
static double routecost(const Instancia *in,const Solucao *s){double v=0;int prev=0;for(int i=0;i<s->numMissoesCaminhao;i++){int j=s->missaoCaminhao[i].n;v+=in->tempoCaminhao[prev][j];prev=j;}return v+in->tempoCaminhao[prev][0];}
static void routepolish(const Instancia *in,Solucao *s){
    int n=s->numMissoesCaminhao;
    for(int pass=0;pass<100;pass++){
        double gain=0;int bi=-1,bj=-1;
        for(int i=0;i<n-1;i++)for(int j=i+1;j<n;j++){
            int a=i?s->missaoCaminhao[i-1].n:0,b=s->missaoCaminhao[i].n,c=s->missaoCaminhao[j].n,d=j+1<n?s->missaoCaminhao[j+1].n:0;
            double delta=in->tempoCaminhao[a][c]+in->tempoCaminhao[b][d]-in->tempoCaminhao[a][b]-in->tempoCaminhao[c][d];
            if(delta<gain-1e-9){gain=delta;bi=i;bj=j;}
        }
        if(bi>=0){while(bi<bj){Missao x=s->missaoCaminhao[bi];s->missaoCaminhao[bi++]=s->missaoCaminhao[bj];s->missaoCaminhao[bj--]=x;}continue;}
        for(int i=0;i<n;i++)for(int pos=0;pos<=n;pos++){
            if(pos==i||pos==i+1)continue;
            int x=s->missaoCaminhao[i].n,a=i?s->missaoCaminhao[i-1].n:0,b=i+1<n?s->missaoCaminhao[i+1].n:0;
            int c=pos?s->missaoCaminhao[pos-1].n:0,d=pos<n?s->missaoCaminhao[pos].n:0;
            double delta=in->tempoCaminhao[a][b]-in->tempoCaminhao[a][x]-in->tempoCaminhao[x][b]+in->tempoCaminhao[c][x]+in->tempoCaminhao[x][d]-in->tempoCaminhao[c][d];
            if(delta<gain-1e-9){gain=delta;bi=i;bj=pos;}
        }
        if(bi<0)break;
        Missao x=s->missaoCaminhao[bi];
        if(bj<bi){for(int k=bi;k>bj;k--)s->missaoCaminhao[k]=s->missaoCaminhao[k-1];s->missaoCaminhao[bj]=x;}
        else {for(int k=bi;k<bj-1;k++)s->missaoCaminhao[k]=s->missaoCaminhao[k+1];s->missaoCaminhao[bj-1]=x;}
    }
    assert(calcularMakespan(in,s));
}
int main(int argc,char **argv){
    assert(argc==5);const char *name=argv[1];Instancia in={0};Solucao a={0},b={0},best={0};char path[256];
    snprintf(path,sizeof path,"instances/pdstsp-c/large/%s.txt",name);assert(lerInstancia(path,&in));alocarInstancias(&in);construirMatrizCaminhao(&in);
    snprintf(path,sizeof path,"instances/pdstsp-c/large/tau_kj_%s.txt",name);assert(lerTemposDrone(path,&in));
    alocarSolucao(&in,&a);alocarSolucao(&in,&b);alocarSolucao(&in,&best);
    FILE *f=fopen(argv[2],"r");assert(f);assert(fscanf(f,"%d%d",&a.numMissoesCaminhao,&a.numMissoesDrone)==2);
    for(int i=0;i<a.numMissoesCaminhao;i++)assert(fscanf(f,"%d",&a.missaoCaminhao[i].n)==1);
    for(int i=0;i<a.numMissoesDrone;i++)assert(fscanf(f,"%d%d",&a.missaoDrone[i].cliente,&a.missaoDrone[i].qtdDrones)==2);
    fclose(f);assert(escalonarMissoesDrone(&in,&a)&&calcularMakespan(&in,&a));f=fopen(argv[3],"w");assert(f);dump(f,name,"input",&a);
    copy(&in,&a,&b);for(int i=0;i<b.numMissoesDrone;i++)b.missaoDrone[i].qtdDrones=min_k(&in,b.missaoDrone[i].cliente);
    assert(escalonarMissoesDrone(&in,&b)&&calcularMakespan(&in,&b));dump(f,name,"min_area",&b);
    ResultadoBB r={0};copy(&in,&a,&b);escalonarMissoesDroneBBMeta(&in,b.missaoDrone,b.numMissoesDrone,100000,-1,&r);
    assert(escalonarMissoesDrone(&in,&b)&&calcularMakespan(&in,&b));dump(f,name,"bb100k",&b);
    fprintf(stderr,"BB %s status=%d nodes=%llu\n",name,r.motivo,r.nos);
    if(atoi(argv[4])){
      copy(&in,&a,&best);routepolish(&in,&best);
      for(int start=0;start<20&&start<a.numMissoesCaminhao;start++){
        copy(&in,&a,&b);int used[in.n];for(int i=0;i<in.n;i++)used[i]=0;
        int prev=a.missaoCaminhao[start].n;b.missaoCaminhao[0].n=prev;used[prev]=1;
        for(int i=1;i<a.numMissoesCaminhao;i++){
          int chosen=-1;double nearest=INFINITY;
          for(int j=0;j<a.numMissoesCaminhao;j++){int x=a.missaoCaminhao[j].n;if(!used[x]&&in.tempoCaminhao[prev][x]<nearest){nearest=in.tempoCaminhao[prev][x];chosen=x;}}
          assert(chosen>=0);b.missaoCaminhao[i].n=chosen;used[chosen]=1;prev=chosen;
        }
        routepolish(&in,&b);if(routecost(&in,&b)<routecost(&in,&best))copy(&in,&b,&best);
      }
      dump(f,name,"route_only_20starts",&best);
    }
    if(atoi(argv[4]))for(int mode=0;mode<2;mode++){
        copy(&in,&a,&best);int improve=0,equal=0,routebetter=0,dronebetter=0;double start=relogioSA();srand(20261028+mode);
        for(int n=0;n<500;n++){
            int ok=mode?reconstruirParcialCV(&in,&a,&b,.05,.15):trocarRecursosCV(&in,&a,&b);assert(ok);
            improve+=b.makespan<a.makespan;equal+=b.makespan==a.makespan;routebetter+=b.tempoCaminhao<a.tempoCaminhao;dronebetter+=b.tempoDrones<a.tempoDrones;
            if(b.makespan<best.makespan||(b.makespan==best.makespan&&b.tempoCaminhao+b.tempoDrones<best.tempoCaminhao+best.tempoDrones))copy(&in,&b,&best);
        }
        dump(f,name,mode?"best_500_reconstruction":"best_500_exchange",&best);
        fprintf(stderr,"PROBES %s mode=%d n=500 improve=%d equal=%d truckbetter=%d dronebetter=%d seconds=%.6f\n",name,mode,improve,equal,routebetter,dronebetter,relogioSA()-start);
    }
    fclose(f);liberarSolucao(&a);liberarSolucao(&b);liberarSolucao(&best);liberarInstancia(&in);
}
