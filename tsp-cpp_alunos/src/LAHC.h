#ifndef LAHC_H
#define LAHC_H

#include <vector>
#include "Instancia.h"

/*
 * ============================================================================
 *  Módulo: LAHC (Late Acceptance Hill-Climbing)
 * ============================================================================
 *  ESQUELETO -- método ainda a ser implementado (cronograma: aula sobre
 *  Busca Local Iterativa (ILS) e Late Acceptance Hill-Climbing).
 *
 *  Ideia: mantém um histórico circular com as fo's das últimas L
 *  soluções aceitas. A cada iteração, gera um vizinho e o aceita se ele
 *  for melhor que a solução aceita há exatamente L iterações atrás (não
 *  apenas melhor que a solução corrente, como na descida). Isso permite
 *  aceitar pequenas pioras temporárias, ajudando a escapar de ótimos
 *  locais, sem o custo de calibrar uma temperatura como no Simulated
 *  Annealing.
 * ============================================================================
 */

// [EXERCÍCIO - método futuro do cronograma]
// tamanhoLista: tamanho L do histórico circular de fo's.
// lahcMax: número de iterações sem melhora até parar.
double lahc(const Instancia &inst, std::vector<int> &s, int tamanhoLista, int lahcMax);

#endif // LAHC_H
