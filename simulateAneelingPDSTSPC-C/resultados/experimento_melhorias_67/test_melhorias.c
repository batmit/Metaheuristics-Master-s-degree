#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "SA.h"
#include "lerInstancias.h"
int main(void) {
    Instancia inst={0};Solucao a={0},b={0};
    assert(lerInstancia("instances/pdstsp-c/large/67-c-c.txt",&inst));
    alocarInstancias(&inst);construirMatrizCaminhao(&inst);
    assert(lerTemposDrone("instances/pdstsp-c/large/tau_kj_67-c-c.txt",&inst));
    alocarSolucao(&inst,&a);alocarSolucao(&inst,&b);srand(7123);
    for(int s=0;s<9;s++) {
        assert(construirSolucaoInicialDiversa(&inst,&a));
        for(int i=0;i<500;i++) {
            if(!gerarVizinhoAvaliadoConfigurado(&inst,&a,&b,NULL,NULL,true)) {
                fprintf(stderr,"s=%d i=%d a=%d/%d flags=%d/%d b=%d/%d tempos=%g,%g\n",s,i,
                    a.numMissoesCaminhao,a.numMissoesDrone,a.avaliacaoValida,a.cronogramaValido,
                    b.numMissoesCaminhao,b.numMissoesDrone,b.tempoCaminhao,b.tempoDrones);
                fprintf(stderr,"reescala=%d ",escalonarMissoesDrone(&inst,&b));
                fprintf(stderr,"reavalia=%d\n",calcularMakespan(&inst,&b));
                return 1;
            }
            double antes=b.makespan;
            assert(calcularMakespan(&inst,&b) && fabs(antes-b.makespan)<1e-9);
            if(i%13==0) {
                double drones=b.tempoDrones,cam=b.tempoCaminhao;
                assert(repararCargaDrone(&inst,&b));
                assert(b.tempoDrones<=drones && b.tempoCaminhao==cam);
            }
            Solucao aux=a;a=b;b=aux;
        }
    }
    for(unsigned mask=0;mask<64;mask++) {
        assert(construirSolucaoInicial(&inst,&a));
        ParametrosSA p=parametrosSAPadrao();p.melhorias=mask;p.maxAvaliacoes=1000;
        assert(simulatedAnnealing(&inst,&a,&p));
        assert(calcularMakespan(&inst,&a));
    }
    liberarSolucao(&a);liberarSolucao(&b);liberarInstancia(&inst);return 0;
}
