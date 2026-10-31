#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "SA.h"
#include "lerInstancias.h"

static void verificar(const Instancia *in,const Solucao *s)
{
    int *seen=calloc((size_t)in->n,sizeof *seen);assert(seen);
    double truck=0,drone=0;int last=0;
    for(int i=0;i<s->numMissoesCaminhao;i++){int j=s->missaoCaminhao[i].n;assert(j>0&&j<in->n&&!seen[j]++);truck+=in->tempoCaminhao[last][j];last=j;}
    truck+=in->tempoCaminhao[last][0];
    for(int i=0;i<s->numMissoesDrone;i++){
        MissaoDrone m=s->missaoDrone[i];assert(m.cliente>0&&m.cliente<in->n&&!seen[m.cliente]++);
        assert(m.qtdDrones>=1&&m.qtdDrones<=in->numDrones&&m.tempoInicio>=0);
        assert(fabs(m.tempoFim-m.tempoInicio-in->tempoDrone[m.cliente][m.qtdDrones])<1e-9);
        int usage=0;for(int j=0;j<s->numMissoesDrone;j++){MissaoDrone x=s->missaoDrone[j];if(x.tempoInicio<=m.tempoInicio&&x.tempoFim>m.tempoInicio)usage+=x.qtdDrones;}
        assert(usage<=in->numDrones);drone=fmax(drone,m.tempoFim);
    }
    for(int j=1;j<in->n;j++)assert(seen[j]==1);
    assert(s->avaliacaoValida&&fabs(s->makespan-fmax(truck,drone))<1e-9);free(seen);
}
int main(int argc,char **argv)
{
    Instancia in={.n=3,.numDrones=1,.truckSpeed=30};
    Cliente clients[3]={{.x=0,.y=0},{.x=5,.y=0,.elegivelDrone=true,.minDrones=1},{.x=4,.y=0,.elegivelDrone=true,.minDrones=1}};
    in.clientes=clients;alocarInstancias(&in);construirMatrizCaminhao(&in);
    for(int j=0;j<3;j++){in.tempoDrone[j][0]=INFINITY;in.tempoDrone[j][1]=j==1?8:20;}
    Solucao a={0},b={0};alocarSolucao(&in,&a);alocarSolucao(&in,&b);
    a.numMissoesCaminhao=1;a.missaoCaminhao[0].n=1;a.numMissoesDrone=1;a.missaoDrone[0]=(MissaoDrone){2,1,0,0};
    assert(escalonarMissoesDrone(&in,&a)&&calcularMakespan(&in,&a)&&a.makespan==20);verificar(&in,&a);
    double h,tr;assert(metasReparoCV(&in,20,&h,&tr)&&h==19&&tr==16);
    assert(limiteCargaReparoCV(&in,&a)==20&&limiteRotaReparoCV(&in,&a)==20);
    ParametrosSA p=parametrosSAFoco165();assert(validarParametrosSA(&p));
    ParametrosSA invalid=p;invalid.profundidadeCadeia=7;assert(!validarParametrosSA(&invalid));
    invalid=p;invalid.pythonCP=NULL;assert(!validarParametrosSA(&invalid));
    invalid=p;invalid.fracaoTempoCP=.9;assert(!validarParametrosSA(&invalid));
    MetricasReparoCV m={0};srand(123);
    /* Nem uma transferencia isolada melhora 20. A cadeia compensa as duas
     * violacoes intermediarias sem poda indevida e fecha em 16. */
    assert(repararComMetaCV(&in,&a,20,&b,true,true,4,2,1,&m)==1);
    verificar(&in,&b);assert(b.makespan==16&&m.podasCarga>0&&m.podasRota>0&&m.melhorias==1);
    verificar(&in,&a);assert(a.makespan==20&&a.missaoCaminhao[0].n==1);
    /* Fracasso preserva a saida ja encontrada. */
    assert(repararComMetaCV(&in,&b,1,&a,true,true,4,2,.1,&m)==0);assert(a.makespan==20);
    if(argc>1){
        MetricasReparoCV cp={0};
        int r=repararCPSATCV(&in,&a,20,&b,2,8,123,argv[1],"reparo_cpsat.py",&cp);
        assert(r==1&&cp.chamadasCP==1&&cp.sucessosCP==1&&cp.errosCP==0);
        verificar(&in,&b);assert(b.makespan<=19&&b.tempoCaminhao<=16);
        /* Solucao CP sem cache first-fit suporta vizinhos incrementais. */
        Solucao c={0};alocarSolucao(&in,&c);
        for(int k=0;k<20;k++){assert(gerarVizinhoAvaliado(&in,&b,&c));verificar(&in,&c);}
        liberarSolucao(&c);
        double old=b.makespan;
        assert(repararCPSATCV(&in,&a,20,&b,2,.2,123,"nao_existe_python","reparo_cpsat.py",&cp)==-1);
        assert(b.makespan==old&&cp.errosCP==1);
        assert(repararCPSATCV(&in,&a,20,&b,2,2,123,argv[1],"tests/cpsat_invalid_fixture.py",&cp)==-1);
        assert(b.makespan==old&&cp.errosCP==2);
        double start=relogioSA();
        assert(repararCPSATCV(&in,&a,20,&b,2,.05,123,argv[1],"tests/cpsat_timeout_fixture.py",&cp)==0);
        assert(b.makespan==old&&cp.limitesCP==1&&relogioSA()-start<1);
    }
    /* Clientes e matrizes foram alocados de formas diferentes neste teste. */
    liberarSolucao(&a);liberarSolucao(&b);
    for(int i=0;i<in.n;i++){free(in.tempoCaminhao[i]);free(in.tempoDrone[i]);}free(in.tempoCaminhao);free(in.tempoDrone);
    /* Um cronograma externo com espera intencional precisa ser preservado
     * em uma troca so da rota; apos alterar drones, o cache e reconstruido. */
    Instancia ext={.n=5,.numDrones=1,.truckSpeed=30};Cliente exclientes[5]={0};ext.clientes=exclientes;
    for(int j=1;j<5;j++){exclientes[j].x=j;exclientes[j].elegivelDrone=true;exclientes[j].minDrones=1;}
    alocarInstancias(&ext);construirMatrizCaminhao(&ext);
    for(int j=0;j<5;j++){ext.tempoDrone[j][0]=INFINITY;ext.tempoDrone[j][1]=2;}
    alocarSolucao(&ext,&a);alocarSolucao(&ext,&b);
    a.numMissoesCaminhao=3;for(int j=0;j<3;j++)a.missaoCaminhao[j].n=j+1;
    a.numMissoesDrone=1;a.missaoDrone[0]=(MissaoDrone){4,1,8,10};a.cronogramaExterno=true;
    assert(calcularMakespan(&ext,&a));double weights[NUM_MOVIMENTOS];
    for(int j=0;j<NUM_MOVIMENTOS;j++)weights[j]=1e-30;
    weights[TROCAR_CAMINHAO]=1;
    TipoMovimento tipo;assert(gerarVizinhoAvaliadoPonderado(&ext,&a,&b,weights,&tipo));
    assert(tipo==TROCAR_CAMINHAO&&b.cronogramaExterno&&b.missaoDrone[0].tempoInicio==8&&b.missaoDrone[0].tempoFim==10);verificar(&ext,&b);
    for(int j=0;j<NUM_MOVIMENTOS;j++)weights[j]=1e-30;
    weights[PARA_CAMINHAO]=1;
    assert(gerarVizinhoAvaliadoPonderado(&ext,&a,&b,weights,&tipo));
    assert(tipo==PARA_CAMINHAO&&!b.cronogramaExterno&&b.cronogramaValido);verificar(&ext,&b);
    liberarSolucao(&a);liberarSolucao(&b);
    for(int j=0;j<5;j++){free(ext.tempoCaminhao[j]);free(ext.tempoDrone[j]);}free(ext.tempoCaminhao);free(ext.tempoDrone);
    puts("reparo alvo: OK");return 0;
}
