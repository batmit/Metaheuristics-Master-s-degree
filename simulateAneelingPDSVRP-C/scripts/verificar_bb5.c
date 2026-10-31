#include "../SA.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    /* Cliente 1 obrigatorio no caminhao. Qualquer atribuicao do cliente 2
     * mantem makespan=100: nao ha melhoria global em nenhuma tentativa. */
    Cliente clientes[3] = {{0}};
    clientes[2].elegivelDrone = true; clientes[2].minDrones = 1;
    double tc[3][3] = {{0,50,50},{50,0,0},{50,0,0}};
    double td[3][2] = {{INFINITY,INFINITY},{INFINITY,INFINITY},{INFINITY,10}};
    double *c[3] = {tc[0],tc[1],tc[2]}, *d[3] = {td[0],td[1],td[2]};
    Instancia inst = {0};
    inst.n=3; inst.numCaminhoes=1; inst.numDrones=1;
    inst.clientes=clientes; inst.tempoCaminhao=c; inst.tempoDrone=d;
    int tentativas[3] = {4,5,10};
    for (int caso=0; caso<3; ++caso) {
        Solucao s = {0};
        assert(alocarSolucao(&inst,&s));
        s.rotasCaminhao[0].rota[0]=1; s.rotasCaminhao[0].tamanho=1;
        s.missaoDrone[0]=(MissaoDrone){2,1,0,0}; s.numMissoesDrone=1;
        assert(escalonarMissoesDrone(&inst,&s) && calcularMakespan(&inst,&s));
        EstatisticasSA e = {0};
        ParametrosSA p = parametrosSAPadrao();
        p.saMax=tentativas[caso]; p.tempInicial=10; p.tempFinal=2; p.alpha=0.1;
        p.estatisticas=&e;
        srand(42);
        assert(simulatedAnnealing(&inst,&s,&p));
        assert(e.tentativas==(unsigned)tentativas[caso] && e.niveis==1);
        assert(e.chamadasBB==(caso ? 1u : 0u));
        assert(s.makespan==100 && calcularMakespan(&inst,&s));
        liberarSolucao(&s);
    }
    puts("OK: BB nao dispara antes de 5; dispara na quinta; nao repete o mesmo refinamento.");
    return 0;
}
