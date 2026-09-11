#ifndef ALGORITMO_GENETICO_H
#define ALGORITMO_GENETICO_H

#include <vector>
#include "Instancia.h"

/*
 * ============================================================================
 *  Módulo: AlgoritmoGenetico
 * ============================================================================
 *  Metaheurística populacional: mantém uma população de soluções (rotas),
 *  que evolui por gerações sucessivas de seleção, cruzamento e mutação.
 *  Usa os operadores definidos em OperadoresGeneticos.h.
 * ============================================================================
 */

// nind: tamanho da população (ajustado para par internamente).
// maxDesvio: critério de parada -- para quando o desvio-padrão das fo's
//            da população cai abaixo desse valor (população convergiu).
// tipoOperador: 1 = crossover OX, 2 = crossover ERX.
// [EXERCÍCIO]
double algoritmoGenetico(const Instancia &inst, std::vector<int> &s,
                          int nind, double maxDesvio,
                          double probCrossover, double probMutacao,
                          int tipoOperador);

#endif // ALGORITMO_GENETICO_H
