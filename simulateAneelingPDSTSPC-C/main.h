#ifndef MAIN_H
#define MAIN_H
#include <stdbool.h>

/* Parametros originais do cliente; elegibilidade deriva da matriz de drones. */
typedef struct Cliente {
    int id;
    double x, y, peso;
    bool elegivelDrone;
    int minDrones;
} Cliente;

/* Dados imutaveis durante uma busca; todos os tempos estao em minutos. */
typedef struct Instancia {
    int n, numDrones;
    double truckSpeed;
    double batteryWeight, batteryCapacity, maxDischargeCurrent, voltage;
    Cliente *clientes;
    double **tempoCaminhao, **tempoDrone;
} Instancia;

/* Uma missao coletiva ocupa qtdDrones durante todo o intervalo [inicio, fim). */
typedef struct MissaoDrone {
    int cliente, qtdDrones;
    double tempoInicio, tempoFim;
} MissaoDrone;

/* Indice interno do cliente na rota; deposito 0 implicito nas extremidades. */
typedef struct Missao { int n; } Missao;

/* Vetores independentes e seus prefixos usados; caches invalidam quando mudam. */
typedef struct Solucao {
    Missao *missaoCaminhao;
    int numMissoesCaminhao;
    MissaoDrone *missaoDrone;
    int numMissoesDrone;
    double makespan, tempoCaminhao, tempoDrones;
    double *prefixosDrone;
    bool cronogramaValido, avaliacaoValida;
} Solucao;
#endif
