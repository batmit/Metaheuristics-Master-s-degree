#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include "SA.h"

int main(void)
{
    double tc[5][5] = {{0}}, td[5][3];
    double *caminhao[5], *drones[5]; Cliente clientes[5] = {0};
    for (int i = 0; i < 5; i++) {
        caminhao[i] = tc[i]; drones[i] = td[i];
        for (int k = 0; k < 3; k++) td[i][k] = INFINITY;
        clientes[i].elegivelDrone = i > 0 && i < 3; clientes[i].minDrones = 1;
    }
    td[1][1] = td[2][1] = 6; td[1][2] = td[2][2] = 4;
    Instancia inst = {.n=5,.numDrones=2,.clientes=clientes,.tempoDrone=drones,.tempoCaminhao=caminhao};
    Solucao elite[NUM_MELHORES_SA] = {0}, vizinho = {0};
    for (int i = 0; i < NUM_MELHORES_SA; i++) alocarSolucao(&inst,&elite[i]);
    alocarSolucao(&inst,&vizinho);
    elite[0].numMissoesDrone = 2; elite[0].numMissoesCaminhao = 2;
    elite[0].missaoCaminhao[0].n = 3; elite[0].missaoCaminhao[1].n = 4;
    for (int i = 0; i < 2; i++) elite[0].missaoDrone[i] = (MissaoDrone){i+1,2,0,0};
    assert(escalonarMissoesDrone(&inst,&elite[0]));
    assert(calcularMakespan(&inst,&elite[0]) && elite[0].makespan == 8);
    MissaoDrone limitadas[] = {{1,2,0,4},{2,2,4,8}};
    bool provado = true;
    assert(escalonarMissoesDroneBBLimitado(&inst,limitadas,2,1,&provado) == 8);
    assert(!provado);
    int quantidade = 1;
    assert(refinarEliteAleatoriaSA(&inst,elite,&quantidade,10000) == 1);
    assert(elite[0].makespan == 6 && elite[0].tempoDrones == 6);
    assert(elite[0].branchBoundAplicado && elite[0].cronogramaValido);
    assert(refinarEliteAleatoriaSA(&inst,elite,&quantidade,10000) == 0);
    int trocas = 0, outras = 0;
    srand(123);
    for (int i = 0; i < 100; i++) {
        TipoMovimento tipo;
        assert(gerarVizinhoAvaliadoPonderado(&inst,&elite[0],&vizinho,NULL,&tipo));
        if (tipo == TROCAR_CAMINHAO) {
            trocas++;
            assert(vizinho.branchBoundAplicado && vizinho.cronogramaValido);
            assert(vizinho.tempoDrones == 6);
        } else { outras++; assert(vizinho.cronogramaValido && !vizinho.branchBoundAplicado); }
        double resultado = vizinho.makespan;
        assert(calcularMakespan(&inst,&vizinho) && vizinho.makespan == resultado);
    }
    assert(trocas && outras);
    assert(construirSolucaoInicial(&inst,&elite[0]));
    assert(refinarEliteAleatoriaSA(&inst,elite,&quantidade,10000) == 0);
    for (int i = 0; i < 4; i++) liberarSolucao(&elite[i]);
    liberarSolucao(&vizinho);

    /* Escolha aleatoria: quatro ordens diferentes, sem mudanca de makespan. */
    inst.n = 4; inst.numDrones = 1;
    clientes[3].elegivelDrone = true;
    td[1][1]=1; td[2][1]=2; td[3][1]=3;
    const int ordens[4][3] = {{1,2,3},{1,3,2},{2,1,3},{3,1,2}};
    for (int i = 0; i < 4; i++) {
        alocarSolucao(&inst,&elite[i]); elite[i].numMissoesDrone=3;
        for (int j=0;j<3;j++) elite[i].missaoDrone[j]=(MissaoDrone){ordens[i][j],1,0,0};
        assert(escalonarMissoesDrone(&inst,&elite[i]));
        assert(calcularMakespan(&inst,&elite[i]));
    }
    int escolhas[4]={0}; quantidade=4;
    for (int repeticao=0;repeticao<120;repeticao++) {
        for (int i=0;i<4;i++) elite[i].branchBoundAplicado=false;
        elite[0].branchBoundAplicado=true; /* Melhor indice nunca e elegivel. */
        assert(refinarEliteAleatoriaSA(&inst,elite,&quantidade,1000)==1);
        int aplicadas=0;
        for (int i=1;i<4;i++) if(elite[i].branchBoundAplicado) {escolhas[i]++; aplicadas++;}
        assert(aplicadas==1 && quantidade==4);
    }
    assert(escolhas[1]>20 && escolhas[2]>20 && escolhas[3]>20);
    for (int i=0;i<4;i++) liberarSolucao(&elite[i]);
    /* Disparo no SA: um nivel sem melhora, compara com BB desativado. */
    inst.n = 3; inst.numDrones = 2;
    td[1][1] = td[2][1] = 6; td[1][2] = td[2][2] = 4;
    for (int i=0;i<3;i++) for (int j=0;j<3;j++) tc[i][j] = i == j ? 0 : 100;
    ParametrosSA p = parametrosSAPadrao();
    p.tempInicial=0.02; p.tempFinal=0.01; p.alpha=0.5; p.saMax=1;
    p.niveisSemMelhoraBB=1; p.pesosAdaptativos=false;
    Solucao sol = {0}; alocarSolucao(&inst,&sol);
    for (int modo=0;modo<2;modo++) {
        sol.numMissoesCaminhao=0; sol.numMissoesDrone=2;
        for(int i=0;i<2;i++) sol.missaoDrone[i]=(MissaoDrone){i+1,2,0,0};
        p.bbPeriodico = modo != 0;
        srand(123);
        assert(simulatedAnnealing(&inst,&sol,&p));
        assert(sol.makespan == (modo ? 6 : 8));
        assert(calcularMakespan(&inst,&sol));
    }
    liberarSolucao(&sol);
    return 0;
}
