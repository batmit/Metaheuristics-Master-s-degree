#ifndef BUSCA_TABU_H
#define BUSCA_TABU_H

#include <deque>
#include <utility>
#include <vector>
#include "Instancia.h"

/*
 * ============================================================================
 *  Módulo: BuscaTabu
 * ============================================================================
 *  Metaheurística de trajetória que, assim como a descida, explora a
 *  vizinhança de troca -- mas aceita o melhor movimento encontrado mesmo
 *  que ele piore a solução, para escapar de ótimos locais. Para evitar
 *  ficar "andando em círculos", os últimos movimentos realizados ficam
 *  proibidos (tabu) por um tempo.
 *
 *  A lista tabu, que no código original era uma lista duplamente
 *  encadeada implementada na mão (struct lista), agora é simplesmente:
 *
 *      std::deque<std::pair<int,int>> listaTabu;
 *
 *  Cada elemento é o par de posições (i, j) que foi trocado. Um
 *  std::deque (fila de duas pontas) é a estrutura certa aqui, pois
 *  precisamos inserir no final (push_back) e remover do início
 *  (pop_front) com eficiência -- exatamente o padrão "primeiro a entrar,
 *  primeiro a sair" (FIFO) de uma lista tabu clássica.
 * ============================================================================
 */

// [EXERCÍCIO]
// Executa a Busca Tabu a partir da solução corrente s. A cada iteração,
// aplica o melhor movimento de troca que não seja tabu (ou que, mesmo
// sendo tabu, resulte em uma solução melhor que a melhor já encontrada --
// critério de aspiração). tamanhoMaximoLista é o tamanho da lista tabu;
// btMax é o número de iterações sem melhora até parar. Devolve o custo
// da melhor solução encontrada, e deixa essa solução em s.
double buscaTabu(const Instancia &inst, std::vector<int> &s,
                  int tamanhoMaximoLista, int btMax);

// [EXERCÍCIO]
// Explora toda a vizinhança de troca a partir de s (de custo fo) e
// devolve o custo do melhor vizinho ADMISSÍVEL (não tabu, ou tabu mas que
// melhora foEstrela), indicando as posições trocadas em melhorI/melhorJ.
double melhorVizinhoTabu(const Instancia &inst, std::vector<int> &s, double fo,
                          int &melhorI, int &melhorJ, double foEstrela,
                          const std::deque<std::pair<int, int>> &listaTabu);

// [EXERCÍCIO]
// Verifica se o movimento (posicao1, posicao2) está na lista tabu.
bool estaNaListaTabu(int posicao1, int posicao2,
                      const std::deque<std::pair<int, int>> &listaTabu);

#endif // BUSCA_TABU_H
