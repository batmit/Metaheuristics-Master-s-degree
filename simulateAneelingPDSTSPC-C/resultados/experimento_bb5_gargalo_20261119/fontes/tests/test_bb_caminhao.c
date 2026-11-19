#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include "SA.h"
#include "lerInstancias.h"

static double enumerar(const Instancia *in,int v,unsigned mask) {
    if(!mask) return in->tempoCaminhao[v][0];
    double best=INFINITY;
    for(int j=1;j<in->n;j++) if(mask&(1u<<j))
        best=fmin(best,in->tempoCaminhao[v][j]+enumerar(in,j,mask^(1u<<j)));
    return best;
}
int main(void) {
    srand(42);
    for(int caso=0;caso<40;caso++) {
        Instancia in={.n=2+caso%7,.numDrones=0,.truckSpeed=30};
        in.clientes=calloc((size_t)in.n,sizeof(Cliente));assert(in.clientes);
        for(int j=0;j<in.n;j++) {
            in.clientes[j].x=(rand()%20)/3.0;
            in.clientes[j].y=(rand()%20)/3.0;
        }
        alocarInstancias(&in);construirMatrizCaminhao(&in);
        Solucao s={0};alocarSolucao(&in,&s);s.numMissoesCaminhao=in.n-1;
        for(int j=1;j<in.n;j++) s.missaoCaminhao[j-1].n=j;
        assert(calcularMakespan(&in,&s));double antes=s.makespan;
        ResultadoBB r;
        assert(otimizarCaminhaoBB(&in,&s,1,0,&r));
        assert(s.makespan<=antes+1e-8);
        assert(otimizarCaminhaoBB(&in,&s,0,0,&r));
        double exato=enumerar(&in,0,((1u<<in.n)-1)^1u);
        assert(r.motivo==BB_OTIMO && fabs(s.tempoCaminhao-exato)<1e-7);
        assert(r.limiteInferior<=exato+1e-7);
        assert(otimizarCaminhaoBB(&in,&s,0,relogioSA()-1,&r));
        if(in.n>2) assert(r.motivo==BB_LIMITE_TEMPO);
        liberarSolucao(&s);liberarInstancia(&in);
    }
    /* Dez propostas sem alteracao dos tempos -> exatamente duas chamadas/recurso. */
    Instancia in={.n=4,.numDrones=0,.truckSpeed=30};
    in.clientes=calloc(4,sizeof(Cliente));alocarInstancias(&in);construirMatrizCaminhao(&in);
    Solucao s={0};alocarSolucao(&in,&s);assert(construirSolucaoInicial(&in,&s));
    EstatisticasSA e;ParametrosSA p=parametrosSARapido();
    p.estatisticas=&e;p.estagnacaoRecursoBB=5;p.saMax=10;p.alpha=.5;
    p.tempInicial=10;p.tempFinal=6;p.refinoFinalBB=false;
    assert(simulatedAnnealing(&in,&s,&p));
    assert(e.avaliacoes==10 && e.chamadasBBCaminhao==2 && e.chamadasBBDrone==2);
    /* Empate nao e gargalo estrito: bloqueia apenas caminhao, nao drones. */
    p.apenasGargaloBBCaminhao=true;
    assert(simulatedAnnealing(&in,&s,&p));
    assert(e.chamadasBBCaminhao==0 && e.bloqueiosBBCaminhao==2 && e.chamadasBBDrone==2);
    /* Com drones zerados e rota positiva, o filtro permite o B&B. */
    in.clientes[1].x=2;construirMatrizCaminhao(&in);
    assert(construirSolucaoInicial(&in,&s));
    assert(simulatedAnnealing(&in,&s,&p));
    assert(e.chamadasBBCaminhao==2 && e.bloqueiosBBCaminhao==0 && e.chamadasBBDrone==2);
    liberarSolucao(&s);liberarInstancia(&in);
    /* Drones estritamente dominantes; transferir seu cliente para o caminhao
     * custa 400, pior que 100, e as dez propostas sao rejeitadas nesta semente. */
    in=(Instancia){.n=3,.numDrones=1,.truckSpeed=30};
    in.clientes=calloc(3,sizeof(Cliente));alocarInstancias(&in);
    in.clientes[2]=(Cliente){.x=100,.elegivelDrone=true,.minDrones=1};
    construirMatrizCaminhao(&in);in.tempoDrone[2][1]=100;
    s=(Solucao){0};alocarSolucao(&in,&s);
    s.numMissoesCaminhao=1;s.missaoCaminhao[0].n=1;
    s.numMissoesDrone=1;s.missaoDrone[0]=(MissaoDrone){2,1,0,100};
    srand(123);assert(simulatedAnnealing(&in,&s,&p));
    assert(e.chamadasBBCaminhao==0 && e.bloqueiosBBCaminhao==2 && e.chamadasBBDrone==2);
    assert(s.tempoDrones>s.tempoCaminhao);
    liberarSolucao(&s);liberarInstancia(&in);
    return 0;
}
