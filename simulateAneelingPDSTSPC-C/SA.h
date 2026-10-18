#ifndef SA_H
#define SA_H

#include "main.h"

typedef struct ParametrosSA {
    double alpha;       /* Resfriamento: 0 < alpha < 1. */
    double beta;        /* Aquecimento da calibracao: beta > 1. */
    double gamma;       /* Fracao alvo de aceitacao: 0 < gamma < 1. */
    int saMax;          /* Tentativas por temperatura, inclusive na calibracao. */
    double tempInicial; /* Temperatura manual ou ponto de partida da calibracao. */
    double tempFinal;   /* 0 < tempFinal < tempInicial. */
} ParametrosSA;

/* beta=1.1, gamma=0.95, saMax=500.
 * Valores iniciais ajustaveis para os demais: alpha=0.99,
 * tempInicial=10.0 e tempFinal=0.01, na escala do makespan em minutos. */
ParametrosSA parametrosSAPadrao(void);

/* Retorna 1 somente se todos os parametros forem finitos e validos. */
int validarParametrosSA(const ParametrosSA *parametros);

/* A calibracao testa vizinhos sem modificar a solucao de referencia;
 * aumenta a temperatura por beta ate atingir gamma de aceitacao.
 * Reconstroi e avalia uma copia da referencia antes dos testes.
 * Retorna 1 e escreve temperatura somente no sucesso; 0 na falha, inclusive
 * ausencia de movimentos, erro de avaliacao, overflow ou limite de 10000 niveis.
 * Inicializar srand uma unica vez no chamador. */
int calculaTemperaturaInicial(const Instancia *inst, const Solucao *referencia,
                             const ParametrosSA *parametros, double *temperatura);

/* solucao alocada por alocarSolucao para esta instancia:
 * entrada inicial, saida melhor encontrada, incluindo seu cronograma.
 * Esta interface utiliza tempInicial; a calibracao sera uma chamada separada.
 * A semente aleatoria deve ser inicializada uma unica vez pelo chamador.
 * Retorna 1 no sucesso (inclusive sem movimentos); 0 na falha, preservando
 * a solucao do chamador. Reconstroi a avaliacao inicial numa copia. */
int simulatedAnnealing(const Instancia *inst, Solucao *solucao,
                      const ParametrosSA *parametros);

#endif
