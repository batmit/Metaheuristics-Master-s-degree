#ifndef MULTISTART_H
#define MULTISTART_H

#include <vector>
#include "Instancia.h"

/*
 * ============================================================================
 *  Módulo: MultiStart
 * ============================================================================
 *  Procedimento de Múltiplos Reinícios (Multi-Start): gera diversas
 *  soluções iniciais aleatórias, aplica busca local (descida) em cada
 *  uma, e mantém a melhor solução encontrada entre todas as tentativas.
 * ============================================================================
 */

// [EXERCÍCIO]
// Executa itermax reinícios: em cada um, constrói uma solução aleatória e
// aplica o método da descida; ao final, s contém a melhor solução
// encontrada entre todos os reinícios. Devolve o custo dessa solução.
double multiStart(const Instancia &inst, std::vector<int> &s, int itermax);

#endif // MULTISTART_H
