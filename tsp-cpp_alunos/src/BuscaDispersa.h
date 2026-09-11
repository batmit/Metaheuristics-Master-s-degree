#ifndef BUSCA_DISPERSA_H
#define BUSCA_DISPERSA_H

#include <vector>
#include "Instancia.h"

/*
 * ============================================================================
 *  Módulo: BuscaDispersa (Scatter Search e Reconexão por Caminhos)
 * ============================================================================
 *  ESQUELETO -- método ainda a ser implementado (cronograma: aula sobre
 *  Busca Dispersa, Reconexão por Caminhos e Princípio da Otimalidade
 *  Próxima).
 *
 *  Ideia da Busca Dispersa: mantém um "conjunto de referência" pequeno de
 *  soluções boas E diversas entre si (não apenas as de menor custo).
 *  Combina sistematicamente pares de soluções do conjunto de referência
 *  (não aleatoriamente, como em um AG) e refina os resultados com busca
 *  local, atualizando o conjunto de referência com as melhores/mais
 *  diversas soluções encontradas.
 *
 *  Ideia da Reconexão por Caminhos (Path Relinking): dadas duas soluções
 *  (uma "de partida" e uma "guia"), gera uma sequência de soluções
 *  intermediárias que caminha gradualmente da primeira até a segunda,
 *  desfazendo uma diferença por vez -- e explora essa trajetória em busca
 *  de soluções melhores que as duas originais.
 * ============================================================================
 */

// [EXERCÍCIO - método futuro do cronograma]
// tamanhoConjuntoReferencia: nº de soluções mantidas no conjunto de referência.
// maxIteracoes: critério de parada.
double buscaDispersa(const Instancia &inst, std::vector<int> &s,
                      int tamanhoConjuntoReferencia, int maxIteracoes);

// [EXERCÍCIO - método futuro do cronograma]
// Gera o caminho de soluções entre solucaoPartida e solucaoGuia, e
// devolve a melhor solução encontrada ao longo desse caminho.
double reconexaoPorCaminhos(const Instancia &inst,
                             const std::vector<int> &solucaoPartida,
                             const std::vector<int> &solucaoGuia,
                             std::vector<int> &melhorSolucaoDoCaminho);

#endif // BUSCA_DISPERSA_H
