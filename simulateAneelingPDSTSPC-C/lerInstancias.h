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
int lerInstancia(const char *nomeArquivo, Instancia *inst);
int lerTemposDrone(const char *nomeArquivo, Instancia *inst);

/* Libera clientes e matrizes, inclusive apos falhas de leitura. */
void liberarInstancia(Instancia *inst);

#endif
