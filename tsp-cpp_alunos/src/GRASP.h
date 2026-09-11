#ifndef GRASP_H
#define GRASP_H

#include <vector>
#include "Instancia.h"

/*
 * ============================================================================
 *  Módulo: GRASP (Greedy Randomized Adaptive Search Procedure)
 * ============================================================================
 *  A cada iteração: (1) constrói uma solução parcialmente gulosa/aleatória
 *  (fase de construção); (2) refina essa solução com busca local (fase de
 *  busca local). Mantém a melhor solução encontrada entre as iterações.
 * ============================================================================
 */

// tipoConstrucao: 1 = Vizinho Mais Próximo parcialmente guloso,
//                 2 = Inserção Mais Barata parcialmente gulosa.
// [EXERCÍCIO]
double grasp(const Instancia &inst, std::vector<int> &s,
             double alpha, int graspMax, int tipoConstrucao);

#endif // GRASP_H
