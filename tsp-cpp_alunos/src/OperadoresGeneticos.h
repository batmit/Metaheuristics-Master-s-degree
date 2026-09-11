#ifndef OPERADORES_GENETICOS_H
#define OPERADORES_GENETICOS_H

#include <vector>

/*
 * ============================================================================
 *  Módulo: OperadoresGeneticos
 * ============================================================================
 *  Operadores de seleção, cruzamento (crossover) e mutação usados tanto
 *  pelo Algoritmo Genético (AlgoritmoGenetico.cpp) quanto pelo Algoritmo
 *  Memético (Memetico.cpp) -- que nada mais é do que um AG com uma etapa
 *  extra de busca local aplicada aos descendentes. Como os operadores
 *  genéticos em si são idênticos nos dois métodos, eles ficam
 *  centralizados aqui em vez de duplicados (no código original em C,
 *  AG.cpp e Memeticos.cpp tinham cada um sua própria cópia de roleta,
 *  crossover_OX, crossover_ERX e mutacao -- duas cópias do mesmo código
 *  para manter sincronizadas).
 * ============================================================================
 */

// [EXERCÍCIO]
// Seleciona um índice da população pelo mecanismo da roleta (roulette
// wheel selection): cada indivíduo recebe uma fatia da roleta
// proporcional à sua aptidão (indivíduos com menor fo, isto é, melhores
// para minimização, recebem fatias maiores).
int roleta(const std::vector<double> &foPop);

// [EXERCÍCIO]
// Variante da roleta com "scaling": indivíduos com fo muito acima da
// média (mais de "alfa" desvios-padrão) recebem aptidão zero, o que
// reduz a dominância de indivíduos muito bons no início da busca e evita
// convergência prematura.
int roletaScaling(const std::vector<double> &foPop);

// [EXERCÍCIO]
// Aplica mutação em s: troca duas cidades de posição, escolhidas ao
// acaso.
void mutacao(std::vector<int> &s);

// [EXERCÍCIO]
// Operador de cruzamento OX (Order Crossover): copia o segmento entre
// dois pontos de corte de cada pai diretamente para o filho
// correspondente, e completa as posições restantes com as cidades do
// outro pai, na ordem em que aparecem, pulando as que já foram copiadas.
void crossoverOX(const std::vector<int> &pai1, const std::vector<int> &pai2,
                  std::vector<int> &filho1, std::vector<int> &filho2);

// [EXERCÍCIO]
// Operador de cruzamento ERX (Edge Recombination Crossover): constrói os
// filhos priorizando as arestas (ligações entre cidades vizinhas) que já
// apareciam nos pais, escolhendo a cada passo a cidade adjacente com
// menos arestas restantes (heurística de "menor conectividade").
void crossoverERX(const std::vector<int> &pai1, const std::vector<int> &pai2,
                   std::vector<int> &filho1, std::vector<int> &filho2);

#endif // OPERADORES_GENETICOS_H
