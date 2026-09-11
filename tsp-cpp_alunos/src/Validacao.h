#ifndef VALIDACAO_H
#define VALIDACAO_H

#include <vector>
#include "Instancia.h"

/*
 * ============================================================================
 *  Módulo: Validacao
 * ============================================================================
 *  Função de depuração/conferência: verifica se uma rota s é uma solução
 *  VÁLIDA do PCV e se a função objetivo informada (por exemplo, a fo que
 *  um método devolveu) realmente corresponde a essa rota.
 *
 *  Uma solução válida do PCV precisa:
 *    1. ter exatamente n posições (uma para cada cidade);
 *    2. conter cada cidade de 0 a n-1 EXATAMENTE UMA VEZ (nem faltando,
 *       nem repetida).
 *
 *  Isso é útil porque um bug em um método (por exemplo, um índice
 *  incorreto em um movimento, ou um vetor não totalmente preenchido) pode
 *  produzir uma "solução" com cidades repetidas/faltando cuja fo, embora
 *  calculada corretamente pela fórmula do custo, não representa uma rota
 *  real -- e pode até parecer "boa demais para ser verdade" (menor que o
 *  ótimo conhecido da instância, por exemplo).
 * ============================================================================
 */

// [FRAMEWORK]
// Verifica se s é uma permutação válida das n cidades da instância, e se
// a fo informada bate com o custo recalculado a partir de s. Imprime um
// relatório no console (se imprimeDetalhes == true) e devolve true se a
// solução é válida (fo batendo, sem cidades faltando ou repetidas).
//
// foInformada: fo que o método devolveu, para conferir se corresponde a
//              s. Se for omitida (valor negativo), pula essa conferência
//              e apenas valida a integridade da rota.
bool validaSolucao(const Instancia &inst, const std::vector<int> &s,
                    double foInformada = -1.0, bool imprimeDetalhes = true);

#endif // VALIDACAO_H
