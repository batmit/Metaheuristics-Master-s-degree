#ifndef SA_H
#define SA_H

#include "cv.h"

typedef struct EstatisticasSA {
    unsigned long long tentativas, niveis, chamadasBB, nosBB, melhoriasBB;
    unsigned long long bbLimitados;
} EstatisticasSA;

typedef struct ParametrosSA {
    double alpha, beta, gamma;
    int saMax;
    double tempInicial, tempFinal;
    /* Opcional: recebe apenas incumbentes completos e viaveis. */
    void (*registrarMelhor)(const Solucao *, void *);
    void *contextoRegistro;
    EstatisticasSA *estatisticas;
} ParametrosSA;

ParametrosSA parametrosSAPadrao(void);
/* Mesmo procedimento do cpp: T=10, saMax testes sobre referencia fixa;
 * aquece por beta enquanto aceitos < gamma*saMax. Referencia preservada.
 * Retorna 1 sucesso, 0 erro/sem vizinhos; saida preservada na falha. */
int calculaTemperaturaInicial(const Instancia *inst, const Solucao *referencia,
                             double beta, double gamma, int saMax,
                             double *temperatura);
/* Dois lacos do cpp: saMax tentativas por nivel, T *= alpha, Metropolis.
 * Apos 5 tentativas sem melhora global, refina a melhor por BB, sem repetir
 * os mesmos clientes/k ja refinados. Meta = maior rota dos caminhoes.
 * Publica melhor solucao; 1 sucesso, 0 erro. Sem corte de tempo.
 * Inicializar srand uma unica vez no chamador. */
int simulatedAnnealing(const Instancia *inst, Solucao *solucao,
                      const ParametrosSA *parametros);

#endif
