#ifndef LERINSTANCIAS_H
#define LERINSTANCIAS_H

#include "main.h"

/*
 * Le uma instancia no formato:
 *
 * NUM_DRONES, ...
 * TRUCK_SPEED, ...
 * BATTERY_WEIGHT, ...
 * BATTERY_CAPACITY, ...
 * MAX_DISCHARGE_CURRENT, ...
 * VOLTAGE, ...
 * node  x  y  weight
 * ...
 *
 * Retorna 1 em caso de sucesso e 0 em caso de erro.
 */
/* Inicializar inst com {0}; liberar antes de outra leitura.
 * n inclui o deposito; IDs devem ser sequenciais desde 0.
 * Esta funcao le apenas parametros e clientes; numCaminhoes fica 0. */
int lerInstancia(const char *nomeArquivo, Instancia *inst);
/* Aloca tempoDrone se necessario. -1 vira INFINITY; preenche
 * elegivelDrone e minDrones. Na falha, dados podem estar parciais:
 * liberar a instancia ou repetir a leitura antes de utiliza-los. */
int lerTemposDrone(const char *nomeArquivo, Instancia *inst);

/* inst deve iniciar com {0}; liberar antes de reutilizar.
 * Le clientes e tempos, constroi matriz do caminhao e define a frota.
 * Na falha, libera os dados carregados e zera inst. */
int carregarInstancia(const char *nomeArquivo, const char *arquivoTempos,
                     int numCaminhoes, Instancia *inst);

/* Funcoes auxiliares para testar e liberar a instancia. */
void imprimirInstancia(const Instancia *inst);
void liberarInstancia(Instancia *inst);

#endif
