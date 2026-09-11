#ifndef PSO_DISCRETO_H
#define PSO_DISCRETO_H

#include <vector>
#include "Instancia.h"

/*
 * ============================================================================
 *  Módulo: PSODiscreto (Otimização Discreta por Nuvem de Partículas)
 * ============================================================================
 *  ESQUELETO -- método ainda a ser implementado (cronograma: aula sobre
 *  Colônia de Formigas e Otimização Discreta por Nuvem de Partículas).
 *
 *  Ideia: cada "partícula" representa uma rota candidata e se move pelo
 *  espaço de busca influenciada por três forças: sua própria inércia
 *  (continuar parecida com o que era), a atração pela melhor solução que
 *  ELA MESMA já encontrou (pbest) e a atração pela melhor solução
 *  encontrada por QUALQUER partícula da nuvem (gbest). Em problemas
 *  discretos como o PCV, "movimento" costuma ser reinterpretado como uma
 *  sequência de trocas (swaps) aplicada com uma certa probabilidade.
 * ============================================================================
 */

// [EXERCÍCIO - método futuro do cronograma]
// numParticulas: quantidade de partículas na nuvem.
// iterMax: número de iterações sem melhora até parar.
double psoDiscreto(const Instancia &inst, std::vector<int> &s,
                    int numParticulas, int iterMax);

#endif // PSO_DISCRETO_H
