/* Adapter only. SA.c, cv.c and drone_bb.c are linked without modifications.
 * Matrix entries are TSPLIB costs, not PDSVRP minutes. */
#include "../SA.h"
#include "../lerInstancias.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char **argv)
{
    if (argc != 2) return 1;
    Instancia inst = {0};
    Solucao s = {0};
    int success = 0;
    if (scanf("%d", &inst.n) != 1 || inst.n < 3 || inst.n > 10000) return 1;
    inst.numCaminhoes = 1;
    inst.numDrones = 1; /* storage only; every drone assignment is infeasible */
    inst.clientes = calloc((size_t)inst.n, sizeof *inst.clientes);
    if (!inst.clientes || !alocarMatrizCaminhao(&inst) || !alocarTempoDrone(&inst)) goto done;
    for (int i=0; i<inst.n; ++i) {
        inst.clientes[i].id=i;
        for (int j=0; j<inst.n; ++j) {
            double *v=&inst.tempoCaminhao[i][j];
            if (scanf("%lf",v)!=1 || !isfinite(*v) || *v<0 || floor(*v)!=*v) goto done;
        }
    }
    for (int i=0; i<inst.n; ++i) {
        if (inst.tempoCaminhao[i][i]!=0) goto done;
        for (int j=0; j<inst.n; ++j)
            if (inst.tempoCaminhao[i][j]!=inst.tempoCaminhao[j][i]) goto done;
    }
    srand((unsigned)strtoul(argv[1],NULL,10));
    ParametrosSA p=parametrosSAPadrao();
    EstatisticasSA stats={0};
    p.estatisticas=&stats;
    clock_t start=clock();
    if (!alocarSolucao(&inst,&s) || !construirSolucaoInicial(&inst,&s)) goto done;
    double initial=s.makespan;
    if (!calculaTemperaturaInicial(&inst,&s,p.beta,p.gamma,p.saMax,&p.tempInicial) ||
        !simulatedAnnealing(&inst,&s,&p) || !calcularMakespan(&inst,&s) || s.numMissoesDrone) goto done;
    printf("{\"initial\":%.17g,\"cost\":%.17g,\"temperature\":%.17g,"
           "\"iterations\":%llu,\"bb_calls\":%llu,\"cpu_seconds\":%.6f,\"tour\":[0",
           initial,s.makespan,p.tempInicial,stats.tentativas,stats.chamadasBB,
           (double)(clock()-start)/CLOCKS_PER_SEC);
    for (int i=0;i<s.rotasCaminhao[0].tamanho;++i) printf(",%d",s.rotasCaminhao[0].rota[i]);
    puts("]}");
    success=1;
done:
    liberarSolucao(&s);
    liberarInstancia(&inst);
    return success ? 0 : 1;
}
