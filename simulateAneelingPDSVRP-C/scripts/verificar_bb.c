#include "../SA.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/* Oraculo independente: fixa k e enumera TODOS os inicios inteiros num horizonte.
 * Nao usa first-fit, permutacoes, nem as podas de carga do BB. */
static int viavel(int tarefa, int n, int frota, double **tempos, const int *fixos, int horizonte, int *uso)
{
    if (tarefa == n) return 1;
    for (int k = 1; k <= frota; ++k) {
        if (k != fixos[tarefa + 1]) continue;
        double t = tempos[tarefa + 1][k];
        if (!isfinite(t)) continue;
        int duracao = (int)t;
        for (int inicio = 0; inicio + duracao <= horizonte; ++inicio) {
            int cabe = 1;
            for (int s = inicio; s < inicio + duracao; ++s)
                if (uso[s] + k > frota) { cabe = 0; break; }
            if (!cabe) continue;
            for (int s = inicio; s < inicio + duracao; ++s) uso[s] += k;
            int ok = viavel(tarefa + 1, n, frota, tempos, fixos, horizonte, uso);
            for (int s = inicio; s < inicio + duracao; ++s) uso[s] -= k;
            if (ok) return 1;
        }
    }
    return 0;
}

static void conferir(int n, int frota, double dados[7][4])
{
    Cliente clientes[7] = {{0}};
    double tc[7][7] = {{0}};
    double *c[7], *d[7];
    for (int i = 0; i <= n; ++i) { c[i] = tc[i]; d[i] = dados[i]; }
    Instancia inst = {0};
    inst.n = n + 1; inst.numCaminhoes = 2; inst.numDrones = frota;
    inst.clientes = clientes; inst.tempoCaminhao = c; inst.tempoDrone = d;
    for (int i = 1; i <= n; ++i) {
        clientes[i].elegivelDrone = true;
        for (int k = 1; k <= frota; ++k)
            if (isfinite(d[i][k]) && !clientes[i].minDrones) clientes[i].minDrones = k;
    }
    Solucao s = {0}, invertida = {0}, vizinho = {0}, original = {0};
    assert(alocarSolucao(&inst, &s) && alocarSolucao(&inst, &invertida) &&
           alocarSolucao(&inst, &vizinho) && alocarSolucao(&inst, &original));
    s.numMissoesDrone = invertida.numMissoesDrone = n;
    int fixos[7] = {0};
    for (int j = 1; j <= n; ++j) {
        int total = 0;
        for (int k = 1; k <= frota; ++k)
            if (isfinite(d[j][k]) && rand() % ++total == 0) fixos[j] = k;
    }
    for (int i = 0; i < n; ++i) {
        s.missaoDrone[i] = (MissaoDrone){i + 1, fixos[i + 1], 0, 0};
        invertida.missaoDrone[i] = (MissaoDrone){n - i, fixos[n - i], 999, 999};
    }
    int uso[64] = {0}, otimo = 0;
    while (!viavel(0, n, frota, d, fixos, otimo, uso)) { ++otimo; assert(otimo < 64); }
    ResultadoBB resultado;
    srand(42);
    int proximo = rand();
    srand(42);
    assert(escalonarMissoesDroneBB(&inst, &s, &resultado));
    assert(rand() == proximo); /* O BB nao consome aleatoriedade. */
    assert(resultado.otimoProvado && resultado.limiteInferior <= otimo);
    assert(s.tempoDrones == otimo && s.branchBoundAplicado);
    for (int i = 0; i < n; ++i) assert(s.missaoDrone[i].qtdDrones == fixos[s.missaoDrone[i].cliente]);
    assert(calcularMakespan(&inst, &s));
    assert(escalonarMissoesDroneBB(&inst, &invertida, NULL));
    assert(invertida.tempoDrones == s.tempoDrones);
    for (int i = 0; i < n; ++i) {
        assert(s.missaoDrone[i].cliente == invertida.missaoDrone[i].cliente);
        assert(s.missaoDrone[i].qtdDrones == invertida.missaoDrone[i].qtdDrones);
        assert(s.missaoDrone[i].tempoInicio == invertida.missaoDrone[i].tempoInicio);
    }
    assert(copiarSolucao(&inst, &s, &original));
    for (int i = 0; i < 60; ++i) {
        TipoMovimento tipo;
        int r = gerarVizinhoAvaliado(&inst, &s, &vizinho, &tipo);
        if (!r) break;
        assert(r == 1);
        /* Trocas nunca alteram drones; somente caminhos sao permutados. */
        if (tipo == TROCAR_POSICOES) {
            assert(vizinho.numMissoesDrone == s.numMissoesDrone);
            for (int p = 0; p < s.numMissoesDrone; ++p) {
                assert(vizinho.missaoDrone[p].cliente == s.missaoDrone[p].cliente);
                assert(vizinho.missaoDrone[p].tempoInicio == s.missaoDrone[p].tempoInicio);
            }
        }
        double antes = vizinho.makespan;
        assert(calcularMakespan(&inst, &vizinho));
        assert(fabs(antes - vizinho.makespan) < 1e-9);
        Solucao aux = s; s = vizinho; vizinho = aux;
    }
    double temperatura;
    if (n) assert(calculaTemperaturaInicial(&inst, &s, 1.5, 0.8, 20, &temperatura));
    else {
        assert(!calculaTemperaturaInicial(&inst, &s, 1.5, 0.8, 20, &temperatura));
        temperatura = 10;
    }
    ParametrosSA p = parametrosSAPadrao();
    p.tempInicial = temperatura; p.alpha = 0.5; p.tempFinal = 1; p.saMax = 20;
    double antes = s.makespan;
    assert(simulatedAnnealing(&inst, &s, &p) && s.makespan <= antes);
    assert(calcularMakespan(&inst, &s));
    liberarSolucao(&s); liberarSolucao(&invertida);
    liberarSolucao(&vizinho); liberarSolucao(&original);
}

static void conferirArredondamento(void)
{
    double td[3][4] = {{INFINITY, INFINITY, INFINITY, INFINITY},
                      {INFINITY, INFINITY, 2, INFINITY},
                      {INFINITY, INFINITY, 2, INFINITY}};
    double *d[3] = {td[0], td[1], td[2]};
    Instancia inst = {0};
    inst.n = 3; inst.numDrones = 3; inst.numCaminhoes = 1; inst.tempoDrone = d;
    Solucao s = {0};
    assert(alocarSolucao(&inst, &s));
    s.numMissoesDrone = 2;
    s.missaoDrone[0].cliente = 1; s.missaoDrone[1].cliente = 2;
    s.missaoDrone[0].qtdDrones = s.missaoDrone[1].qtdDrones = 2;
    ResultadoBB r;
    assert(escalonarMissoesDroneBB(&inst, &s, &r));
    assert(r.limiteInferior == 3 && s.tempoDrones == 4); /* ceil(8/3). */
    td[1][2] = td[2][2] = 1.25;
    assert(escalonarMissoesDroneBB(&inst, &s, &r));
    assert(r.limiteInferior < 2 && s.tempoDrones == 2.5); /* Sem ceil. */
    double anterior = s.tempoDrones;
    s.missaoDrone[1].cliente = s.missaoDrone[0].cliente;
    assert(!escalonarMissoesDroneBB(&inst, &s, &r));
    assert(!r.otimoProvado && s.tempoDrones == anterior);
    liberarSolucao(&s);
}

static void conferirOitoMissoes(void)
{
    int duracoes[8]={8,8,8,7,7,7,6,6};
    double td[9][4], *d[9];
    for (int i=0; i<9; ++i) {
        d[i]=td[i];
        for (int k=0; k<4; ++k) td[i][k]=INFINITY;
        if(i) td[i][1]=duracoes[i-1];
    }
    Instancia inst={0};
    inst.n=9; inst.numDrones=3; inst.numCaminhoes=1; inst.tempoDrone=d;
    Solucao s={0};
    assert(alocarSolucao(&inst,&s));
    s.numMissoesDrone=8;
    for(int i=0; i<8; ++i) s.missaoDrone[i]=(MissaoDrone){i+1,1,0,0};
    int otimo=1000;
    /* Oraculo independente: todas as 3^8 atribuicoes a maquinas, k=1. */
    for(int codigo=0; codigo<6561; ++codigo) {
        int numero=codigo, carga[3]={0};
        for(int i=0; i<8; ++i) { carga[numero%3]+=duracoes[i]; numero/=3; }
        int maximo=carga[0];
        for(int i=1; i<3; ++i) if(carga[i]>maximo) maximo=carga[i];
        if(maximo<otimo) otimo=maximo;
    }
    ResultadoBB r;
    assert(escalonarMissoesDroneBB(&inst,&s,&r));
    assert(r.otimoProvado && s.tempoDrones==otimo);
    assert(escalonarMissoesDroneBBLimitado(&inst,&s,-1,1,&r));
    assert(r.limiteNosAtingido && !r.otimoProvado && r.nos==1);
    assert(s.tempoDrones>=otimo);
    assert(escalonarMissoesDroneBBMeta(&inst,&s,100,&r));
    assert(r.metaAtingida && r.nos==0 && !r.otimoProvado);
    assert(s.tempoDrones<=100);
    liberarSolucao(&s);
}

int main(void)
{
    conferirArredondamento();
    conferirOitoMissoes();
    double d[7][4];
    for (int i = 0; i < 7; ++i) for (int k = 0; k < 4; ++k) d[i][k] = INFINITY;
    /* Exige pesquisar outra ordem: greedy menor termino faz 7; otimo=6. */
    d[1][1] = d[2][1] = 3;
    d[3][1] = d[4][1] = d[5][1] = 2;
    conferir(5, 2, d);
    /* Configuracoes alternativas existem, mas o BB deve preservar o k recebido. */
    d[1][1] = d[2][1] = 5; d[1][2] = d[2][2] = 2;
    conferir(2, 2, d);
    for (int caso = 0; caso < 60; ++caso) {
        srand((unsigned)(1000 + caso));
        int n = 1 + rand() % 5, frota = 1 + rand() % 3;
        for (int i = 1; i <= n; ++i) {
            for (int k = 1; k <= frota; ++k)
                d[i][k] = rand() % 3 ? (double)(1 + rand() % 5) : INFINITY;
            d[i][1 + rand() % frota] = 1 + rand() % 5;
        }
        conferir(n, frota, d);
    }
    conferir(0, 2, d);
    puts("OK: 63 conjuntos, oraculo exaustivo independente, invariancia da ordem, SA e calibracao.");
    return 0;
}
