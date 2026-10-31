#include "../lerInstancias.h"
#include "../SA.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static void iguais(const Solucao *a, const Solucao *b)
{
    assert(fabs(a->makespan - b->makespan) < 1e-7);
    assert(fabs(a->tempoDrones - b->tempoDrones) < 1e-7);
    assert(a->numMissoesDrone == b->numMissoesDrone);
    for (int c = 0; c < a->numCaminhoes; ++c) {
        assert(a->rotasCaminhao[c].tamanho == b->rotasCaminhao[c].tamanho);
        assert(fabs(a->rotasCaminhao[c].tempo - b->rotasCaminhao[c].tempo) < 1e-7);
        for (int p = 0; p < a->rotasCaminhao[c].tamanho; ++p)
            assert(a->rotasCaminhao[c].rota[p] == b->rotasCaminhao[c].rota[p]);
    }
    for (int p = 0; p < a->numMissoesDrone; ++p) {
        assert(a->missaoDrone[p].cliente == b->missaoDrone[p].cliente);
        assert(a->missaoDrone[p].qtdDrones == b->missaoDrone[p].qtdDrones);
        assert(fabs(a->missaoDrone[p].tempoInicio - b->missaoDrone[p].tempoInicio) < 1e-7);
        assert(fabs(a->missaoDrone[p].tempoFim - b->missaoDrone[p].tempoFim) < 1e-7);
    }
}

/* Transcricao independente dos dois lacos do cpp para comparar trajetorias. */
static void referenciaSA(const Instancia *inst, Solucao *s, ParametrosSA p)
{
    Solucao atual = {0}, vizinho = {0}, melhor = {0};
    assert(alocarSolucao(inst, &atual) && alocarSolucao(inst, &vizinho) && alocarSolucao(inst, &melhor));
    assert(copiarSolucao(inst, s, &atual));
    assert(escalonarMissoesDrone(inst, &atual) && calcularMakespan(inst, &atual));
    assert(copiarSolucao(inst, &atual, &melhor));
    double t = p.tempInicial;
    int semMelhora = 0;
    while (t > p.tempFinal) {
        for (int w = 0; w < p.saMax; ++w) {
            ++semMelhora;
            assert(gerarVizinhoAvaliado(inst, &atual, &vizinho, NULL) == 1);
            double delta = vizinho.makespan - atual.makespan;
            int aceitar = delta < 0;
            if (!aceitar) aceitar = (double)rand() / ((double)RAND_MAX + 1) < exp(-delta / t);
            if (aceitar) {
                assert(copiarSolucao(inst, &vizinho, &atual));
                if (atual.makespan < melhor.makespan) {
                    assert(copiarSolucao(inst, &atual, &melhor));
                    semMelhora = 0;
                }
            }
            if (semMelhora >= 5) {
                semMelhora = 0;
                if (!melhor.branchBoundAplicado && melhor.numMissoesDrone) {
                    double antes = melhor.makespan;
                    assert(escalonarMissoesDroneBBLimitado(inst, &melhor, melhor.tempoCaminhoes, 100000, NULL));
                    assert(calcularMakespan(inst, &melhor));
                    if (melhor.makespan < antes) assert(copiarSolucao(inst, &melhor, &atual));
                }
            }
        }
        t *= p.alpha;
    }
    assert(copiarSolucao(inst, &melhor, s));
    liberarSolucao(&atual); liberarSolucao(&vizinho); liberarSolucao(&melhor);
}

static double referenciaTemperatura(const Instancia *inst, const Solucao *s)
{
    Solucao vizinho = {0};
    assert(alocarSolucao(inst, &vizinho));
    double t = 10;
    for (;;) {
        int aceitos = 0;
        for (int i = 0; i < 30; ++i) {
            assert(gerarVizinhoAvaliado(inst, s, &vizinho, NULL) == 1);
            double delta = vizinho.makespan - s->makespan;
            if (delta < 0 || (double)rand() / ((double)RAND_MAX + 1) < exp(-delta / t)) ++aceitos;
        }
        if (aceitos >= 0.8 * 30) break;
        t *= 1.5;
    }
    liberarSolucao(&vizinho);
    return t;
}

static void casosLimite(void)
{
    Cliente clientes[4] = {{0}};
    double tc[4][4] = {{0}}, td[4][3] = {
        {INFINITY, INFINITY, INFINITY}, {INFINITY, 10, INFINITY},
        {INFINITY, 4, INFINITY}, {INFINITY, INFINITY, 3}};
    double *c[4] = {tc[0], tc[1], tc[2], tc[3]};
    double *d[4] = {td[0], td[1], td[2], td[3]};
    Instancia inst = {0};
    inst.n = 4; inst.numCaminhoes = 2; inst.numDrones = 2;
    inst.clientes = clientes; inst.tempoCaminhao = c; inst.tempoDrone = d;
    for (int i = 1; i < 4; ++i) {
        clientes[i].elegivelDrone = true;
        clientes[i].minDrones = i == 3 ? 2 : 1;
    }
    Solucao s = {0}, v = {0};
    assert(alocarSolucao(&inst, &s) && alocarSolucao(&inst, &v));
    s.numMissoesDrone = 3;
    for (int i = 0; i < 3; ++i) s.missaoDrone[i] = (MissaoDrone){i+1, i == 2 ? 2 : 1, 0, 0};
    assert(escalonarMissoesDrone(&inst, &s) && calcularMakespan(&inst, &s));
    assert(s.makespan == 13);
    for (int i = 0; i < 3; ++i) {
        s.missaoDrone[i].tempoInicio = 0;
        s.missaoDrone[i].tempoFim = td[s.missaoDrone[i].cliente][s.missaoDrone[i].qtdDrones];
    }
    assert(!calcularMakespan(&inst, &s)); /* Excede dois drones entre 4 e 7. */
    assert(escalonarMissoesDrone(&inst, &s) && calcularMakespan(&inst, &s));
    s.missaoDrone[2].cliente = s.missaoDrone[0].cliente;
    assert(!escalonarMissoesDrone(&inst, &s)); /* Cliente repetido. */
    liberarSolucao(&s); liberarSolucao(&v);
    inst.n = 1;
    assert(alocarSolucao(&inst, &s) && alocarSolucao(&inst, &v));
    assert(construirSolucaoInicial(&inst, &s) && s.makespan == 0);
    assert(gerarVizinhoAvaliado(&inst, &s, &v, NULL) == 0);
    double t = -1;
    assert(!calculaTemperaturaInicial(&inst, &s, 1.1, 0.95, 20, &t) && t == -1);
    ParametrosSA p = parametrosSAPadrao();
    assert(simulatedAnnealing(&inst, &s, &p) && s.makespan == 0);
    p.alpha = 1;
    assert(!simulatedAnnealing(&inst, &s, &p));
    liberarSolucao(&s); liberarSolucao(&v);
}

int main(int argc, char **argv)
{
    assert(argc == 3);
    casosLimite();
    for (int frota = 2; frota <= 5; ++frota) {
        Instancia inst = {0};
        Solucao atual = {0}, vizinho = {0}, integral = {0}, copia = {0};
        assert(carregarInstancia(argv[1], argv[2], frota, &inst));
        assert(alocarSolucao(&inst, &atual) && alocarSolucao(&inst, &vizinho) &&
               alocarSolucao(&inst, &integral) && alocarSolucao(&inst, &copia));
        srand((unsigned)(1234 + frota));
        assert(construirSolucaoInicial(&inst, &atual));
        int trocas = 0, transferencias = 0, alteracoesDrone = 0;
        for (int iter = 0; iter < 50; ++iter) {
            assert(copiarSolucao(&inst, &atual, &copia));
            TipoMovimento tipo;
            assert(gerarVizinhoAvaliado(&inst, &atual, &vizinho, &tipo) == 1);
            iguais(&atual, &copia);
            if (tipo == TROCAR_POSICOES) ++trocas; else ++transferencias;
            if (vizinho.numMissoesDrone) ++alteracoesDrone;
            assert(copiarSolucao(&inst, &vizinho, &integral));
            assert(escalonarMissoesDrone(&inst, &integral));
            assert(calcularMakespan(&inst, &integral));
            iguais(&vizinho, &integral);
            Solucao aux = atual; atual = vizinho; vizinho = aux;
        }
        assert(trocas && transferencias && alteracoesDrone);
        assert(copiarSolucao(&inst, &atual, &copia));
        double temperatura = -1;
        srand(4567);
        assert(calculaTemperaturaInicial(&inst, &atual, 1.5, 0.8, 30, &temperatura));
        srand(4567);
        assert(temperatura == referenciaTemperatura(&inst, &copia));
        assert(temperatura >= 10 && isfinite(temperatura));
        iguais(&atual, &copia);
        ParametrosSA p = parametrosSAPadrao();
        p.tempInicial = temperatura; p.alpha = 0.5; p.tempFinal = 1; p.saMax = 30;
        assert(copiarSolucao(&inst, &atual, &integral));
        srand(9876);
        assert(simulatedAnnealing(&inst, &atual, &p));
        srand(9876);
        referenciaSA(&inst, &integral, p);
        iguais(&atual, &integral);
        assert(atual.makespan <= copia.makespan + 1e-7);
        assert(calcularMakespan(&inst, &atual));
        liberarSolucao(&atual); liberarSolucao(&vizinho);
        liberarSolucao(&integral); liberarSolucao(&copia); liberarInstancia(&inst);
    }
    puts("OK: deltas, BB, cobertura, capacidade, calibracao e trajetoria SA.");
    return 0;
}
