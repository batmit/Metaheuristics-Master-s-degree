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
    int n; /* Numero de nos, incluindo o deposito no indice 0. */
    int numDrones;
    int numCaminhoes; /* Configurado externamente; nao consta no arquivo. */

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
    /* Indices de clientes, na ordem de visita; deposito 0 implicito. */
    int *rota;

    int tamanho; 
    int capacidade;

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

    /* Vetor dinamico com uma rota por caminhao, inclusive rotas vazias. */
    RotaCaminhao *rotasCaminhao;
    int numCaminhoes; /* Numero de rotas alocadas, para liberar com seguranca. */

    MissaoDrone *missaoDrone;
    /* Prefixo utilizado do vetor, na ordem de processamento das missoes. */
    int numMissoesDrone;

    double makespan;
    double tempoCaminhoes; /* Maior tempo entre todas as rotas. */
    double tempoDrones;
    /* Maior termino apos cada prefixo; ocupacao e dada pelas proprias missoes. */
    double *prefixosDrone;
    bool cronogramaValido;
    bool cronogramaExterno; /* Certificado validado (CP), sem cache first-fit. */
    bool avaliacaoValida;
    bool branchBoundAplicado; /* Refinamento ja tentado para estas missoes. */

}Solucao;



#endif
