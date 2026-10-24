#include "SA.h"
#include "cv.h"
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

ParametrosSA parametrosSAPadrao(void)
{
    return (ParametrosSA){
        .alpha = 0.99,
        .beta = 1.1,
        .gamma = 0.95,
        .saMax = 500,
        .tempInicial = 10.0,
        .tempFinal = 0.01
    };
}

int validarParametrosSA(const ParametrosSA *parametros)
{
    if (parametros == NULL)
        return 0;

    return isfinite(parametros->alpha) &&
        parametros->alpha > 0.0 && parametros->alpha < 1.0 &&
        isfinite(parametros->beta) && parametros->beta > 1.0 &&
        isfinite(parametros->gamma) &&
        parametros->gamma > 0.0 && parametros->gamma < 1.0 &&
        parametros->saMax > 0 &&
        isfinite(parametros->tempInicial) &&
        isfinite(parametros->tempFinal) &&
        parametros->tempFinal > 0.0 &&
        parametros->tempInicial > parametros->tempFinal;
}

int calculaTemperaturaInicial(const Instancia *inst, const Solucao *referencia, const ParametrosSA *parametros, double *temperatura)
{
    if (!validarParametrosSA(parametros) || inst == NULL || referencia == NULL ||
        temperatura == NULL || inst->n < 1 || inst->numDrones < 0)
        return 0;
    int nc = referencia->numMissoesCaminhao;
    int nd = referencia->numMissoesDrone;
    if (nc < 0 || nc > inst->n - 1 || nd != inst->n - 1 - nc ||
        (nc > 0 && referencia->missaoCaminhao == NULL) ||
        (nd > 0 && referencia->missaoDrone == NULL))
        return 0;

    Solucao base = {0}, candidata = {0};
    /* A alocacao usa apenas n; a instancia original permanece constante. */
    Instancia dimensoes = {.n = inst->n, .numDrones = inst->numDrones};
    alocarSolucao(&dimensoes, &base);
    alocarSolucao(&dimensoes, &candidata);
    int sucesso = 0;
    if (base.missaoCaminhao == NULL || base.missaoDrone == NULL ||
        candidata.missaoCaminhao == NULL || candidata.missaoDrone == NULL || base.prefixosDrone == NULL || candidata.prefixosDrone == NULL)
        goto finalizar;
    base.numMissoesCaminhao = nc;
    base.numMissoesDrone = nd;
    for (int i = 0; i < nc; i++)
        base.missaoCaminhao[i] = referencia->missaoCaminhao[i];
    for (int i = 0; i < nd; i++)
        base.missaoDrone[i] = referencia->missaoDrone[i];
    if (!escalonarMissoesDrone(inst, &base) || !calcularMakespan(inst, &base))
        goto finalizar;

    double atual = parametros->tempInicial;
    /* Limite defensivo para beta muito proximo de 1 e metas nao atingidas. */
    const int maxNiveis = 10000;
    for (int nivel = 0; nivel < maxNiveis; nivel++) {
        int aceitos = 0;
        for (int iteracao = 0; iteracao < parametros->saMax; iteracao++) {
            /* Cada teste parte da mesma base; nunca se aceita um movimento nela. */
            if (!gerarVizinhoAvaliado(inst, &base, &candidata))
                goto finalizar;
            double delta = candidata.makespan - base.makespan;
            if (delta <= 0.0) {
                aceitos++;
            } else {
                double sorteio = (double)rand() / ((double)RAND_MAX + 1.0);
                if (sorteio < exp(-delta / atual))
                    aceitos++;
            }
        }
        if ((double)aceitos >= parametros->gamma * parametros->saMax) {
            *temperatura = atual;
            sucesso = 1;
            break;
        }
        double proxima = atual * parametros->beta;
        if (!isfinite(proxima) || proxima <= atual)
            goto finalizar;
        atual = proxima;
    }

finalizar:
    liberarSolucao(&base);
    liberarSolucao(&candidata);
    return sucesso;
}

/* Os dois destinos sao previamente alocados para a mesma instancia. */
static void copiarSolucao(const Instancia *inst, const Solucao *origem, Solucao *destino)
{
    for (int i = 0; i < origem->numMissoesCaminhao; i++)
        destino->missaoCaminhao[i] = origem->missaoCaminhao[i];
    for (int i = 0; i < origem->numMissoesDrone; i++)
        destino->missaoDrone[i] = origem->missaoDrone[i];
    destino->numMissoesCaminhao = origem->numMissoesCaminhao;
    destino->numMissoesDrone = origem->numMissoesDrone;
    destino->makespan = origem->makespan;
    destino->tempoCaminhao = origem->tempoCaminhao;
    destino->tempoDrones = origem->tempoDrones;
    destino->avaliacaoValida = origem->avaliacaoValida;
    destino->cronogramaValido = origem->cronogramaValido;
    if (origem->prefixosDrone && destino->prefixosDrone)
        memcpy(destino->prefixosDrone, origem->prefixosDrone,
               ((size_t)origem->numMissoesDrone + 1) * ((size_t)inst->numDrones + 1) * sizeof(double));
}

static int existeMovimento(const Instancia *inst, const Solucao *solucao)
{
    if (solucao->numMissoesCaminhao >= 2 || solucao->numMissoesDrone > 0)
        return 1;
    if (solucao->numMissoesCaminhao == 0 || inst->clientes == NULL ||
        inst->tempoDrone == NULL)
        return 0;
    int cliente = solucao->missaoCaminhao[0].n;
    int k = inst->clientes[cliente].minDrones;
    return inst->clientes[cliente].elegivelDrone && k >= 1 &&
        k <= inst->numDrones && inst->tempoDrone[cliente] != NULL &&
        isfinite(inst->tempoDrone[cliente][k]) && inst->tempoDrone[cliente][k] >= 0.0;
}

int simulatedAnnealing(const Instancia *inst, Solucao *solucao, const ParametrosSA *parametros){

    if (!validarParametrosSA(parametros) || inst == NULL || solucao == NULL || inst->n < 1 || inst->numDrones < 0 || solucao->missaoCaminhao == NULL || solucao->missaoDrone == NULL)
        return 0;

    int nc = solucao->numMissoesCaminhao, nd = solucao->numMissoesDrone;
    
    if (nc < 0 || nc > inst->n - 1 || nd != inst->n - 1 - nc)
        return 0;

    //Criamos três soluções: candidata, atual e melhor
    Solucao atual = {0}, candidata = {0}, melhor = {0};
    
    Instancia dimensoes = {.n = inst->n, .numDrones = inst->numDrones};
    
    alocarSolucao(&dimensoes, &atual);

    alocarSolucao(&dimensoes, &candidata);
    
    alocarSolucao(&dimensoes, &melhor);
    
    int sucesso = 0;
    
    if (atual.missaoCaminhao == NULL || atual.missaoDrone == NULL || candidata.missaoCaminhao == NULL || candidata.missaoDrone == NULL || melhor.missaoCaminhao == NULL || melhor.missaoDrone == NULL || atual.prefixosDrone == NULL || candidata.prefixosDrone == NULL || melhor.prefixosDrone == NULL){
        goto finalizar;
    }
    copiarSolucao(inst, solucao, &atual);
    
    if (!escalonarMissoesDrone(inst, &atual) || !calcularMakespan(inst, &atual)){
        goto finalizar;
    }
    copiarSolucao(inst, &atual, &melhor);
    
    double temperatura = parametros->tempInicial;
    
    //enquanto a temperatura for maior do que a final e existir movimento
    while (temperatura > parametros->tempFinal && existeMovimento(inst, &atual)) {
        
        //Dentro do SAmax
        for (int iteracao = 0; iteracao < parametros->saMax; iteracao++) {
            
            //Aqui eu gero vizinho, escalono essa mudança e calculo o makespan
            if (!gerarVizinhoAvaliado(inst, &atual, &candidata))
                goto finalizar;

            double delta = candidata.makespan - atual.makespan;
            
            int aceitar = delta <= 0.0;
            
            if (!aceitar) {
                double sorteio = (double)rand() / ((double)RAND_MAX + 1.0);
                aceitar = sorteio < exp(-delta / temperatura);
            }
            if (aceitar) {
                /* Troca apenas a propriedade dos buffers internos, sem copia rasa
                 * entre a solucao do chamador e as solucoes de trabalho. */
                Solucao auxiliar = atual;
                atual = candidata;
                candidata = auxiliar;
                if (atual.makespan < melhor.makespan)
                    copiarSolucao(inst, &atual, &melhor);
            }
        }

        double proxima = temperatura * parametros->alpha;
        
        if (proxima >= temperatura)
            goto finalizar;
        
        temperatura = proxima;
    }
    /* Uma verificacao completa ao final tambem elimina deriva numerica dos deltas. */
    if (!calcularMakespan(inst, &melhor)) goto finalizar;
    copiarSolucao(inst, &melhor, solucao);
    sucesso = 1;

finalizar:
    liberarSolucao(&atual);
    liberarSolucao(&candidata);
    liberarSolucao(&melhor);
    return sucesso;
}
