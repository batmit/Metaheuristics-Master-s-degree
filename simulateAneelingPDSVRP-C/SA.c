#include "SA.h"
#include <math.h>
#include <stdlib.h>

static double uniforme(void)
{
    return (double)rand() / ((double)RAND_MAX + 1.0);
}

ParametrosSA parametrosSAPadrao(void)
{
    return (ParametrosSA){0.99, 1.1, 0.95, 500, 10.0, 0.01, NULL, NULL, NULL};
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
    int semMelhora = 0;
    EstatisticasSA estatisticas = {0};
    if (!alocarSolucao(inst, &atual) || !alocarSolucao(inst, &vizinho) ||
        !alocarSolucao(inst, &melhor) || !copiarSolucao(inst, solucao, &atual) ||
        !escalonarMissoesDrone(inst, &atual) || !calcularMakespan(inst, &atual) ||
        !copiarSolucao(inst, &atual, &melhor)) goto fim;
    for (double t = p->tempInicial; t > p->tempFinal;) {
        ++estatisticas.niveis;
        for (int w = 0; w < p->saMax; ++w) {
            ++estatisticas.tentativas;
            ++semMelhora;
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
                    semMelhora = 0;
                    if (p->registrarMelhor) p->registrarMelhor(&melhor, p->contextoRegistro);
                }
            }
            if (semMelhora >= 5) {
                semMelhora = 0;
                /* As mesmas missoes/k ja refinadas nao precisam de nova busca.
                 * Mudancas so nas rotas preservam o cronograma encontrado. */
                if (!melhor.branchBoundAplicado && melhor.numMissoesDrone > 0) {
                    ResultadoBB r;
                    double anterior = melhor.makespan;
                    ++estatisticas.chamadasBB;
                    if (!escalonarMissoesDroneBBLimitado(inst, &melhor, melhor.tempoCaminhoes, 100000, &r) ||
                        !calcularMakespan(inst, &melhor)) goto fim;
                    estatisticas.nosBB += r.nos;
                    if (r.limiteNosAtingido) ++estatisticas.bbLimitados;
                    if (melhor.makespan < anterior) {
                        ++estatisticas.melhoriasBB;
                        if (!copiarSolucao(inst, &melhor, &atual)) goto fim;
                        if (p->registrarMelhor) p->registrarMelhor(&melhor, p->contextoRegistro);
                    }
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
    if (p->estatisticas) *p->estatisticas = estatisticas;
    liberarSolucao(&atual);
    liberarSolucao(&vizinho);
    liberarSolucao(&melhor);
    return sucesso;
}
