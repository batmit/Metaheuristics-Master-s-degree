#ifndef ILS_H
#define ILS_H

#include <vector>
#include "Instancia.h"

/*
 * ============================================================================
 *  Módulo: ILS (Iterated Local Search / Busca Local Iterada)
 * ============================================================================
 *  A cada iteração, perturba a solução corrente (aplicando "nivel + 1"
 *  trocas aleatórias) e refina o resultado com busca local. Se a
 *  perturbação encontrar uma solução melhor, o nível de perturbação volta
 *  ao mínimo; caso contrário, após "vezesNivel" tentativas sem melhora no
 *  mesmo nível, o nível de perturbação aumenta (perturbações maiores).
 * ============================================================================
 */

// [EXERCÍCIO]
// vezesNivel: número de perturbações tentadas em cada nível antes de
//             aumentar o nível.
// ilsMax: número de iterações totais sem melhora até parar.
double ils(const Instancia &inst, std::vector<int> &s, int vezesNivel, int ilsMax);

#endif // ILS_H
