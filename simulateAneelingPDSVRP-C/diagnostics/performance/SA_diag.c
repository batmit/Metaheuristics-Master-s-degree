
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
static double diagAgora(void) { LARGE_INTEGER t,f; QueryPerformanceCounter(&t); QueryPerformanceFrequency(&f); return (double)t.QuadPart/f.QuadPart; }
#else
#include <sys/time.h>
static double diagAgora(void) { struct timeval t; gettimeofday(&t, NULL); return t.tv_sec+t.tv_usec/1e6; }
#endif
#include "SA.h"
#include <math.h>
#include <stdlib.h>

static double uniforme(void)
{
    return (double)rand() / ((double)RAND_MAX + 1.0);
}

ParametrosSA parametrosSAPadrao(void)
{
    return (ParametrosSA){0.99, 1.1, 0.95, 500, 10.0, 0.01, NULL, NULL};
}

int calculaTemperaturaInicial(const Instancia *inst, const Solucao *referencia,
                             double beta, double gamma, int saMax,
                             double *temperatura)
{
    if (!temperatura || !isfinite(beta) || beta <= 1 ||
        !isfinite(gamma) || gamma <= 0 || gamma >= 1 || saMax <= 0) return 0;
    Solucao base = {0}, vizinho = {0};
    int sucesso = 0;
    if (!alocarSolucao(inst, &base) || !alocarSolucao(inst, &vizinho) ||
        !copiarSolucao(inst, referencia, &base) ||
        !escalonarMissoesDrone(inst, &base) || !calcularMakespan(inst, &base)) goto fim;
    double t = 10.0;
    for (;;) {
        int aceitos = 0;
        for (int i = 0; i < saMax; ++i) {
            if (gerarVizinhoAvaliado(inst, &base, &vizinho, NULL) != 1) goto fim;
            double delta = vizinho.makespan - base.makespan;
            if (delta < 0 || uniforme() < exp(-delta / t)) ++aceitos;
        }
        if (aceitos >= gamma * saMax) {
            *temperatura = t;
            sucesso = 1;
            break;
        }
        double proxima = t * beta;
        if (!isfinite(proxima) || proxima <= t) goto fim;
        t = proxima;
    }
fim:
    liberarSolucao(&base);
    liberarSolucao(&vizinho);
    return sucesso;
}

int simulatedAnnealing(const Instancia *inst, Solucao *solucao,
                      const ParametrosSA *p)
{
    if (!p || !isfinite(p->alpha) || p->alpha <= 0 || p->alpha >= 1 ||
        p->saMax <= 0 || !isfinite(p->tempInicial) || !isfinite(p->tempFinal) ||
        p->tempFinal <= 0 || p->tempInicial <= p->tempFinal) return 0;
    Solucao atual = {0}, vizinho = {0}, melhor = {0};
    int sucesso = 0;
    if (!alocarSolucao(inst, &atual) || !alocarSolucao(inst, &vizinho) ||
        !alocarSolucao(inst, &melhor) || !copiarSolucao(inst, solucao, &atual) ||
        !escalonarMissoesDrone(inst, &atual) || !calcularMakespan(inst, &atual) ||
        !copiarSolucao(inst, &atual, &melhor)) goto fim;
    unsigned long long diagIter = 0; for (double t = p->tempInicial; t > p->tempFinal;) {
        fprintf(stderr,"SA_LEVEL t=%.9f iter=%llu time=%.9f\n",t,diagIter,diagAgora()); fflush(stderr); for (int w = 0; w < p->saMax; ++w) { ++diagIter; fprintf(stderr,"SA_ITER %llu\n",diagIter); fflush(stderr);
            int resultado = gerarVizinhoAvaliado(inst, &atual, &vizinho, NULL);
            if (resultado < 0) goto fim;
            if (!resultado) goto publicar;
            double delta = vizinho.makespan - atual.makespan;
            if (delta < 0 || uniforme() < exp(-delta / t)) {
                Solucao aux = atual;
                atual = vizinho;
                vizinho = aux;
                if (atual.makespan < melhor.makespan) {
                    if (!copiarSolucao(inst, &atual, &melhor)) goto fim;
                    if (p->registrarMelhor) p->registrarMelhor(&melhor, p->contextoRegistro);
                }
            }
        }
        double proxima = t * p->alpha;
        if (proxima >= t) goto fim;
        t = proxima;
    }
publicar:
    sucesso = copiarSolucao(inst, &melhor, solucao);
fim:
    liberarSolucao(&atual);
    liberarSolucao(&vizinho);
    liberarSolucao(&melhor);
    return sucesso;
}
