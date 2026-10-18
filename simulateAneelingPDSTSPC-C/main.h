#ifndef MAIN_H
#define MAIN_H
#include<stdbool.h>
typedef struct Cliente {
    int id;

    double x;
    double y;

    double peso;

    bool elegivelDrone;

    // quantidade mínima de drones necessária
    // para transportar a encomenda
    int minDrones;
}Cliente;



typedef struct Instancia
{
    int n;
    int numDrones;

    double truckSpeed;

    double batteryWeight;
    double batteryCapacity;
    double maxDischargeCurrent;
    double voltage;

    Cliente *clientes;

    double **tempoCaminhao;




    double **tempoDrone;

} Instancia;

typedef struct RotaCaminhao {
    int *rota;

    int tamanho; 

    double tempo;
} RotaCaminhao;

typedef struct MissaoDrone {
    int cliente;

    int qtdDrones;

    double tempoInicio;
    double tempoFim;
}MissaoDrone;

typedef struct Missao{

    /* Indice em inst->clientes e nas matrizes de tempos; nao o ID externo. */
    int n;

} Missao;

typedef struct Solucao {

    Missao *missaoCaminhao;
    /* Prefixo utilizado do vetor, na ordem da rota; deposito 0 implicito. */
    int numMissoesCaminhao;

    MissaoDrone *missaoDrone;
    /* Prefixo utilizado do vetor, na ordem de processamento das missoes. */
    int numMissoesDrone;

    double makespan;

}Solucao;



#endif
