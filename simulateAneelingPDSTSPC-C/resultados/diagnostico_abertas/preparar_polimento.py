"""Ensaio separado: atribuicoes fixas; descida VND na rota + BB limitado."""
from pathlib import Path
import json

HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[1]
measurements=json.loads((HERE/'medicoes.json').read_text())
polished={}
c=r'''
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "cv.h"
#include "lerInstancias.h"
static void run(const char *name,const char *folder,int nc,const int *route,
                int nd,const int (*mis)[2],double old_truck,double old_drone) {
    Instancia in={0};Solucao s={0};char path[256];
    snprintf(path,sizeof path,"instances/pdstsp-c/%s/%s.txt",folder,name);
    assert(lerInstancia(path,&in));alocarInstancias(&in);construirMatrizCaminhao(&in);
    snprintf(path,sizeof path,"instances/pdstsp-c/%s/tau_kj_%s.txt",folder,name);
    assert(lerTemposDrone(path,&in));alocarSolucao(&in,&s);
    s.numMissoesCaminhao=nc;s.numMissoesDrone=nd;
    for(int i=0;i<nc;i++)s.missaoCaminhao[i].n=route[i];
    for(int i=0;i<nd;i++)s.missaoDrone[i]=(MissaoDrone){mis[i][0],mis[i][1],0,0};
    assert(escalonarMissoesDrone(&in,&s)&&calcularMakespan(&in,&s));
    Solucao low={0};alocarSolucao(&in,&low);low.numMissoesCaminhao=nc;low.numMissoesDrone=nd;
    for(int i=0;i<nc;i++)low.missaoCaminhao[i]=s.missaoCaminhao[i];
    for(int i=0;i<nd;i++) {
        int j=s.missaoDrone[i].cliente,best_k=0;double area=INFINITY;
        for(int k=in.clientes[j].minDrones;k<=in.numDrones;k++) {
            double value=k*in.tempoDrone[j][k];
            if(isfinite(value)&&value<area){area=value;best_k=k;}
        }
        assert(best_k);low.missaoDrone[i]=(MissaoDrone){j,best_k,0,0};
    }
    assert(escalonarMissoesDrone(&in,&low)&&calcularMakespan(&in,&low));
    printf("MINAREA %s %.9f %.9f %.9f ROUTE",name,low.makespan,low.tempoCaminhao,low.tempoDrones);
    for(int i=0;i<nc;i++)printf(" %d",low.missaoCaminhao[i].n);
    printf(" MISSIONS");
    for(int i=0;i<nd;i++){MissaoDrone x=low.missaoDrone[i];printf(" %d:%d:%.9f:%.9f",x.cliente,x.qtdDrones,x.tempoInicio,x.tempoFim);}
    puts("");fflush(stdout);liberarSolucao(&low);
    double before=s.tempoDrones;ResultadoBB bb;
    double value=escalonarMissoesDroneBBMeta(&in,s.missaoDrone,nd,100000,-1,&bb);
    assert(value<=before);
    assert(escalonarMissoesDrone(&in,&s)&&calcularMakespan(&in,&s));
    printf("POLISH %s %.9f %.9f %.9f %.9f %.9f %d %llu ROUTE",name,
           old_truck,old_drone,s.makespan,s.tempoCaminhao,s.tempoDrones,bb.motivo,bb.nos);
    for(int i=0;i<nc;i++)printf(" %d",s.missaoCaminhao[i].n);
    printf(" MISSIONS");
    for(int i=0;i<nd;i++){MissaoDrone m=s.missaoDrone[i];printf(" %d:%d:%.9f:%.9f",m.cliente,m.qtdDrones,m.tempoInicio,m.tempoFim);}
    puts("");fflush(stdout);liberarSolucao(&s);liberarInstancia(&in);
}
int main(void){
'''
for i,(name,r) in enumerate(measurements.items()):
    folder='small' if int(name.split('-')[0])<=30 else 'large'
    coords={int(v[0]):tuple(map(float,v[1:3])) for line in (ROOT/f'instances/pdstsp-c/{folder}/{name}.txt').read_text().splitlines()[7:] if (v:=line.split())}
    def cost(a,b):return 2*(abs(coords[a][0]-coords[b][0])+abs(coords[a][1]-coords[b][1]))
    route=r['final']['route'][:]
    original=route[:];passes=0
    while True:
        changed=False
        for a in range(len(route)-1):
            for b in range(a+1,len(route)):
                u=route[a-1] if a else 0;v=route[b+1] if b+1<len(route) else 0
                delta=cost(u,route[b])+cost(route[a],v)-cost(u,route[a])-cost(route[b],v)
                if delta<-1e-8:
                    route[a:b+1]=reversed(route[a:b+1]);changed=True;break
            if changed:break
        if changed:passes+=1;continue
        for a in range(len(route)):
            x=route[a];u=route[a-1] if a else 0;v=route[a+1] if a+1<len(route) else 0
            base=cost(u,v)-cost(u,x)-cost(x,v);rest=route[:a]+route[a+1:]
            for b in range(len(route)):
                p=rest[b-1] if b else 0;q=rest[b] if b<len(rest) else 0
                delta=base+cost(p,x)+cost(x,q)-cost(p,q)
                if delta<-1e-8:
                    route=rest[:b]+[x]+rest[b:];changed=True;break
            if changed:break
        if not changed:break
        passes+=1
    time=sum(cost(a,b) for a,b in zip([0]+route,route+[0]))
    polished[name]={'old_truck':r['final']['truck'],'new_truck':time,'descent_steps':passes,'route':route}
    c+=f'    static const int r{i}[]={{'+','.join(map(str,route))+'};\n'
    c+=f'    static const int d{i}[][2]={{'+','.join('{'+f'{m["client"]},{m["k"]}'+'}' for m in r['final']['missions'])+'};\n'
    c+=f'    run("{name}","{folder}",{len(route)},r{i},{len(r["final"]["missions"])},d{i},{r["final"]["truck"]},{r["final"]["drone"]});\n'
c+='return 0;\n}\n'
(HERE/'testar_polimento.c').write_text(c,encoding='utf-8')
(HERE/'rotas_polidas.json').write_text(json.dumps(polished,indent=2),encoding='utf-8')
print([(n,r['old_truck'],r['new_truck'],r['descent_steps']) for n,r in polished.items()])
