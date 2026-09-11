#ifndef COLONIA_FORMIGAS_H
#define COLONIA_FORMIGAS_H

#include <vector>
#include "Instancia.h"

/*
 * ============================================================================
 *  Módulo: ColoniaFormigas (Ant Colony Optimization)
 * ============================================================================
 *  ESQUELETO -- método ainda a ser implementado (cronograma: aula sobre
 *  Colônia de Formigas e Otimização Discreta por Nuvem de Partículas).
 *
 *  Ideia: método populacional inspirado no comportamento de formigas
 *  reais. Cada "formiga" constrói uma rota escolhendo a próxima cidade
 *  probabilisticamente, com base em duas informações: o feromônio
 *  acumulado na aresta (i,j) -- que reflete o quanto essa aresta apareceu
 *  em boas soluções anteriores -- e uma informação heurística (ex.:
 *  1/distancia). Ao final de cada iteração (todas as formigas
 *  construíram uma rota), o feromônio evapora um pouco em todas as
 *  arestas e é reforçado nas arestas usadas pelas melhores rotas.
 *
 *  Uma matriz Instancia::n x Instancia::n de feromônio (semelhante à
 *  matriz de distâncias) é uma estrutura de dados natural aqui --
 *  considere um std::vector<std::vector<double>> feromonio.
 * ============================================================================
 */

// [EXERCÍCIO - método futuro do cronograma]
// numFormigas: quantidade de formigas por iteração.
// alfa: peso dado ao feromônio na escolha probabilística.
// beta: peso dado à informação heurística (1/distância) na escolha.
// taxaEvaporacao: fração do feromônio que evapora a cada iteração (0..1).
// iterMax: número de iterações sem melhora até parar.
double coloniaDeFormigas(const Instancia &inst, std::vector<int> &s,
                          int numFormigas, double alfa, double beta,
                          double taxaEvaporacao, int iterMax);

#endif // COLONIA_FORMIGAS_H
