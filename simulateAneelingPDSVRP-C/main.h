#ifndef MAIN_H
#define MAIN_H
#include <stdbool.h>

/* Dados de entrada: no 0 e o deposito; demais indices sao clientes. */
typedef struct Cliente {
    int id;
    double x;
    double y;
    double peso;
    bool elegivelDrone;
    int minDrones; /* Menor k viavel na tabela de tempos; zero se inelegivel. */
}Cliente;
/* Tempos de drones vem das tabelas oficiais; nenhuma equacao de energia e refeita. */
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
/* Viagem completa deposito-cliente-deposito usando qtdDrones simultaneamente. */
typedef struct MissaoDrone {
    int cliente;
    int qtdDrones;
    double tempoInicio;
    double tempoFim;
}MissaoDrone;
/* Certificado completo, custos e indicadores de validade dos caches. */
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
    bool cronogramaValido;
    bool avaliacaoValida;
    bool branchBoundAplicado; /* Refinamento ja tentado para estas missoes. */
}Solucao;
#endif
