#ifndef SIMULATED_ANNEALING_H
#define SIMULATED_ANNEALING_H

#include <vector>
#include "Instancia.h"

/*
 * ============================================================================
 *  Módulo: SimulatedAnnealing (Recozimento Simulado)
 * ============================================================================
 *  Metaheurística de trajetória inspirada no processo físico de recozimento
 *  de metais: aceita movimentos de piora com uma probabilidade que
 *  depende da "temperatura" corrente, a qual vai diminuindo ao longo da
 *  execução (o que reduz gradualmente a chance de aceitar pioras).
 * ============================================================================
 */

// [EXERCÍCIO]
// Executa o Simulated Annealing a partir da solução corrente s. A cada
// nível de temperatura, tenta SAmax movimentos de troca; aceita
// movimentos de melhora sempre, e movimentos de piora com probabilidade
// exp(-delta/temperatura). A temperatura é resfriada geometricamente
// (temperatura *= alpha) até atingir tempFinal. Devolve o custo da melhor
// solução encontrada, e deixa essa solução em s.
double simulatedAnnealing(const Instancia &inst, std::vector<int> &s,
                           double alpha, int saMax,
                           double tempInicial, double tempFinal);

// [EXERCÍCIO]
// Calcula uma temperatura inicial adequada para a instância: parte de uma
// temperatura "chute" e vai multiplicando por beta (aquecendo) até que a
// taxa de aceitação de movimentos de piora atinja pelo menos "gamma"
// (por exemplo, gamma = 0.95 significa aceitar pelo menos 95% dos
// movimentos testados).
double calculaTemperaturaInicial(const Instancia &inst, std::vector<int> &s,
                                  double beta, double gamma, int saMax);

#endif // SIMULATED_ANNEALING_H
