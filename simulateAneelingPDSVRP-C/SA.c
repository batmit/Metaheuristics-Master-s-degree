/* Simulated Annealing com calibracao de temperatura e refinamento periodico por BB. */
#include "SA.h"
#include <math.h>
#include <stdlib.h>

/* Sorteia em [0,1), usado no criterio de aceitacao de Metropolis. */
static double uniforme(void)
{
    return (double)rand() / ((double)RAND_MAX + 1.0);
}

/* Antes de emitir um resultado, recalcula rotas, cobertura e capacidade.
 * O executor Python faz uma segunda verificacao independente ao recebe-lo. */
static int registrarValidado(const Instancia *inst, Solucao *s, const ParametrosSA *p)
{
    if (!calcularMakespan(inst, s)) return 0;
    if (p->registrarMelhor) p->registrarMelhor(s, p->contextoRegistro);
    return 1;
}

/* Parametros do resfriamento, aquecimento e numero de tentativas por nivel. */
ParametrosSA parametrosSAPadrao(void)
{
    return (ParametrosSA){0.99, 1.1, 0.95, 500, 10.0, 0.01, NULL, NULL, NULL};
}

/* Aquece sobre uma referencia fixa ate aceitar a proporcao gamma de vizinhos. */
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

/* Mantem estado atual e melhor separado; resfria e refina o melhor por BB. */
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
                    if (!registrarValidado(inst, &melhor, p)) goto fim;
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
                        if (!registrarValidado(inst, &melhor, p)) goto fim;
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
