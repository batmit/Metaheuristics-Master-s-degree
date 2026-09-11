#ifndef VND_H
#define VND_H

#include <vector>
#include "Instancia.h"

/*
 * ============================================================================
 *  Módulo: VND (Variable Neighborhood Descent / Busca em Vizinhança
 *  Descendente)
 * ============================================================================
 *  ESQUELETO -- método ainda a ser implementado (cronograma: aula sobre
 *  Busca em Vizinhança Variável e Variantes: GVNS, RVNS, SVNS).
 *
 *  Diferente do VNS (que sorteia um vizinho aleatório em cada
 *  vizinhança), o VND é DETERMINÍSTICO: aplica busca local até o ótimo
 *  em uma primeira estrutura de vizinhança; ao ficar preso em um ótimo
 *  local, passa para a PRÓXIMA estrutura de vizinhança (por exemplo,
 *  trocar a vizinhança de troca (swap) pela vizinhança 2-opt); se essa
 *  segunda vizinhança encontrar uma melhora, volta para a primeira
 *  vizinhança. Para quando nenhuma das r vizinhanças encontra melhora.
 *
 *  Ideia de estrutura de vizinhanças possíveis para o PCV:
 *    k=1: troca de duas cidades (swap)      -- já implementado em BuscaLocal
 *    k=2: troca de dois segmentos (2-opt)   -- fica de exercício
 *    k=3: realocação de uma cidade (or-opt) -- fica de exercício
 * ============================================================================
 */

// [EXERCÍCIO - método futuro do cronograma]
// r: número de estruturas de vizinhança distintas a explorar.
double vnd(const Instancia &inst, std::vector<int> &s, int r);

#endif // VND_H
