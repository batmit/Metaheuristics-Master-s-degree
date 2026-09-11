#ifndef UTILITARIOS_H
#define UTILITARIOS_H

#include <vector>
#include "Instancia.h"

/*
 * ============================================================================
 *  Módulo: Utilitarios
 * ============================================================================
 *  Funções pequenas e genéricas, usadas por praticamente todos os métodos
 *  de otimização implementados no curso (construtivos, busca local,
 *  metaheurísticas de trajetória e populacionais).
 *
 *  Note que, comparado à versão em C, várias funções que existiam aqui
 *  (cria_vetor, libera_vetor, atualiza_vetor, inicializa_vetor, ...)
 *  DESAPARECERAM. Isso não é acidente: com std::vector, essas operações
 *  já vêm prontas:
 *
 *      std::vector<int> s(n);      // "cria_vetor" + "inicializa_vetor"
 *      std::vector<int> sEstrela = s;   // "atualiza_vetor" (cópia)
 *      // ao sair de escopo, o vetor libera sua própria memória sozinho
 *      // ("libera_vetor" nunca mais precisa ser chamado manualmente)
 *
 *  Use sempre as facilidades da STL (std::vector, <algorithm>, <numeric>)
 *  em vez de reescrever essa lógica na mão.
 * ============================================================================
 */

// [FRAMEWORK]
// Calcula o custo (comprimento total) da rota s: soma as distâncias entre
// cidades consecutivas da rota e fecha o ciclo voltando da última cidade
// para a primeira.
double custo(const Instancia &inst, const std::vector<int> &s);

// [FRAMEWORK]
// Imprime a rota no formato "c0 -> c1 -> c2 -> ... -> c0".
void imprimeRota(const std::vector<int> &s);

// [FRAMEWORK]
// Calcula o desvio-padrão (amostral) de um conjunto de valores de função
// objetivo -- usado como critério de parada em Algoritmos Genéticos e
// Meméticos (parar quando a população convergir, isto é, quando o
// desvio-padrão das fo's ficar pequeno).
double desvioPadrao(const std::vector<double> &valores);

#endif // UTILITARIOS_H
