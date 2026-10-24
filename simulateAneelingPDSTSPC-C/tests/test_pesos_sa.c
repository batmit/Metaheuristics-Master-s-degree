#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include "SA.h"

int main(void)
{
    ParametrosSA p = parametrosSAPadrao();
    assert(p.pesosAdaptativos && p.taxaAdaptacao == 0.2 && p.pesoMinimo == 0.1);
    PesosMovimentosSA e = inicializarPesosMovimentosSA();
    e.tentativas[TROCAR_CAMINHAO] = 10;
    e.pontos[TROCAR_CAMINHAO] = 20;
    e.tentativas[PARA_DRONES] = 10;
    atualizarPesosMovimentosSA(&e, &p);
    assert(fabs(e.pesos[TROCAR_CAMINHAO] - 1.2) < 1e-12);
    assert(fabs(e.pesos[PARA_DRONES] - 0.8) < 1e-12);
    assert(e.pesos[TROCAR_DRONES] == 1);
    for (int i = 0; i < NUM_MOVIMENTOS; i++)
        assert(e.tentativas[i] == 0 && e.pontos[i] == 0);
    for (int j = 0; j < 100; j++) {
        e.tentativas[PARA_DRONES] = 1;
        atualizarPesosMovimentosSA(&e, &p);
    }
    assert(e.pesos[PARA_DRONES] == p.pesoMinimo);
    ParametrosSA invalido = p;
    invalido.taxaAdaptacao = NAN; assert(!validarParametrosSA(&invalido));
    invalido = p; invalido.pesoMinimo = 0; assert(!validarParametrosSA(&invalido));
    invalido = p; invalido.taxaAdaptacao = 1.1; assert(!validarParametrosSA(&invalido));

    double t0[] = {0,1,1}, t1[] = {1,0,1}, t2[] = {1,1,0};
    double *tc[] = {t0,t1,t2};
    double d0[] = {INFINITY,INFINITY}, d1[] = {INFINITY,1}, d2[] = {INFINITY,1};
    double *td[] = {d0,d1,d2};
    Cliente clientes[3] = {0};
    for (int i = 1; i < 3; i++) { clientes[i].elegivelDrone = true; clientes[i].minDrones = 1; }
    Instancia inst = {.n=3,.numDrones=1,.tempoCaminhao=tc,.tempoDrone=td,.clientes=clientes};
    Solucao base = {0}, vizinho = {0};
    alocarSolucao(&inst,&base); alocarSolucao(&inst,&vizinho);
    assert(construirSolucaoInicial(&inst,&base));
    assert(escalonarMissoesDrone(&inst,&base));
    assert(calcularMakespan(&inst,&base));
    /* Os tres tipos indisponiveis tem pesos enormes: nao podem participar. */
    double pesos[NUM_MOVIMENTOS] = {1, 1e300, 9, 1e300, 1e300, 1e300, 1e300};
    srand(123);
    int transferencias = 0;
    for (int i = 0; i < 6000; i++) {
        TipoMovimento tipo;
        assert(gerarVizinhoAvaliadoPonderado(&inst,&base,&vizinho,pesos,&tipo));
        assert(tipo == TROCAR_CAMINHAO || tipo == PARA_DRONES);
        transferencias += tipo == PARA_DRONES;
        double obtido = vizinho.makespan;
        assert(calcularMakespan(&inst,&vizinho) && obtido == vizinho.makespan);
    }
    assert(transferencias > 5100 && transferencias < 5700);
    pesos[0] = NAN;
    double antes = vizinho.makespan;
    assert(!gerarVizinhoAvaliadoPonderado(&inst,&base,&vizinho,pesos,NULL));
    assert(vizinho.makespan == antes);
    /* Ambas as formas de selecao continuam integradas ao SA. */
    p.alpha = 0.5; p.saMax = 20;
    srand(123); assert(simulatedAnnealing(&inst,&base,&p));
    assert(calcularMakespan(&inst,&base));
    assert(construirSolucaoInicial(&inst,&base));
    p.pesosAdaptativos = false;
    srand(123); assert(simulatedAnnealing(&inst,&base,&p));
    assert(calcularMakespan(&inst,&base));
    liberarSolucao(&base); liberarSolucao(&vizinho);
    return 0;
}
