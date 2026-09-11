#ifndef CONSTRUCAO_H
#define CONSTRUCAO_H

#include <vector>
#include "Instancia.h"

/*
 * ============================================================================
 *  Módulo: Construcao
 * ============================================================================
 *  Heurísticas construtivas: constroem uma solução inicial "do zero",
 *  cidade a cidade, sem partir de nenhuma solução anterior.
 *
 *  Todas as funções seguem a mesma convenção:
 *    - recebem a instância do problema (const Instancia &inst);
 *    - devolve a rota construída no vetor de saída (std::vector<int> &s),
 *      passado por referência. Ao final da função, s deve conter uma
 *      permutação das cidades 0..n-1 (s.size() == inst.n).
 *    - retorna o custo da solução, ou seja, o valor da função objetivo
 *
 *  Dica de implementação: mantenha um vetor com as cidades ainda não
 *  visitadas (std::vector<int> naoVisitadas) e vá removendo cidades dele
 *  conforme forem inseridas na rota. Funções úteis da STL:
 *
 *      std::iota(...)            -- preenche 0,1,2,...
 *      std::min_element(...)     -- encontra o elemento mínimo (com um
 *                                    comparador, dá pra achar "a cidade
 *                                    mais próxima")
 *      std::erase / vector::erase -- remove um elemento do meio do vetor
 *      std::sort / std::partial_sort -- ordena por um critério (ex.: pela
 *                                        distância até a última cidade)
 * ============================================================================
 */

// [EXERCÍCIO]
// Constrói uma solução de forma gulosa pelo método do Vizinho Mais Próximo:
// parte da cidade 0 e, a cada passo, escolhe a cidade não visitada mais
// próxima da última cidade inserida na rota.
double constroiSolucaoGulosaVizinhoMaisProximo(const Instancia &inst,
                                              std::vector<int> &s);

// [EXERCÍCIO]
// Constrói uma solução totalmente aleatória: a cidade 0 é fixada como
// origem e as demais n-1 cidades são embaralhadas aleatoriamente.
double constroiSolucaoAleatoria(const Instancia &inst, std::vector<int> &s);

// [EXERCÍCIO]
// Constrói uma solução parcialmente gulosa (estilo GRASP) pelo método do
// Vizinho Mais Próximo: a cada passo, monta-se uma Lista Restrita de
// Candidatos (LRC) com as "alpha * |candidatos|" cidades mais próximas da
// última cidade inserida, e sorteia-se uma delas ao acaso.
double constroiSolucaoParcialmenteGulosaVizinhoMaisProximo(const Instancia &inst,
                                                           std::vector<int> &s,
                                                           double alpha);

// [EXERCÍCIO]
// Constrói uma solução de forma gulosa pelo método da Inserção Mais
// Barata: parte de uma subrota com 3 cidades (obtida pelo vizinho mais
// próximo) e, a cada passo, insere a cidade k (entre duas cidades i e j
// já presentes na rota) que resulta no menor custo de inserção
// d(i,k) + d(k,j) - d(i,j).
double constroiSolucaoGulosaInsercaoMaisBarata(const Instancia &inst,
                                              std::vector<int> &s);

// [EXERCÍCIO]
// Constrói uma solução parcialmente gulosa (estilo GRASP) pelo método da
// Inserção Mais Barata: a cada passo, calcula-se o custo de inserção de
// cada cidade candidata na sua melhor posição, forma-se uma LRC com as
// "alpha * |candidatos|" cidades de menor custo, e sorteia-se uma delas.
double constroiSolucaoParcialmenteGulosaInsercaoMaisBarata(const Instancia &inst,
                                                           std::vector<int> &s,
                                                           double alpha);

#endif // CONSTRUCAO_H
