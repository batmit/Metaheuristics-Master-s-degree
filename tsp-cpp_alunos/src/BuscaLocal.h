#ifndef BUSCA_LOCAL_H
#define BUSCA_LOCAL_H

#include <vector>
#include "Instancia.h"

/*
 * ============================================================================
 *  Módulo: BuscaLocal
 * ============================================================================
 *  Implementa o Método da Descida (busca local) sobre a vizinhança de
 *  TROCA: um movimento consiste em trocar de posição duas cidades i e j
 *  da rota (s[i] <-> s[j]). Duas estratégias de exploração da vizinhança
 *  são exploradas:
 *
 *    - Melhor Melhora (best improvement): avalia TODOS os vizinhos e
 *      aplica o melhor movimento encontrado;
 *    - Primeiro Melhora (first improvement): aplica o PRIMEIRO movimento
 *      de melhora encontrado, sem explorar toda a vizinhança.
 *
 *  Todas as funções recebem a instância (const Instancia &) e a solução
 *  corrente (std::vector<int> &s), que é alterada in-place à medida que
 *  a busca local evolui.
 * ============================================================================
 */

// [EXERCÍCIO]
// Calcula a soma das 4 distâncias afetadas pela troca das cidades nas
// posições i e j da rota (as arestas que ligam i e j aos seus vizinhos
// na rota). Usado para calcular o delta de custo de um movimento de troca
// sem precisar recalcular o custo da rota inteira.
double calculaDelta(const Instancia &inst, const std::vector<int> &s, int i, int j);

// [EXERCÍCIO]
// Explora TODA a vizinhança de troca a partir da solução corrente s (de
// custo fo) e devolve o custo do melhor vizinho encontrado, indicando em
// melhorI/melhorJ as posições trocadas para obtê-lo. Não modifica s.
double melhorVizinho(const Instancia &inst, std::vector<int> &s, double fo,
                      int &melhorI, int &melhorJ);

// [EXERCÍCIO]
// Método da Descida com estratégia de Melhor Melhora: aplica repetidamente
// o melhor movimento de troca até não haver mais melhora possível (ótimo
// local). Devolve o custo da solução final.
double descidaCompleta(const Instancia &inst, std::vector<int> &s);

// [EXERCÍCIO]
// Explora um vizinho aleatório indicando em
// melhorI/melhorJ as posições trocadas para obtê-lo. Não modifica s.
double VizinhoRamdomico(const Instancia &inst, std::vector<int> &s, double fo,
                      int &melhorI, int &melhorJ);
// [EXERCÍCIO]
// Método da Descida Randômica: em vez de explorar toda a vizinhança, sorteia
// pares de posições (i, j) e aplica a troca sempre que ela melhora a
// solução; para após iterMax tentativas consecutivas sem melhora.
double descidaRandomica(const Instancia &inst, std::vector<int> &s, int iterMax);

// [EXERCÍCIO]
// Como melhorVizinho, mas explora a vizinhança em ordem aleatória e para
// assim que encontra o PRIMEIRO movimento de melhora (Primeiro Melhora).
double vizinhoPrimeiroMelhora(const Instancia &inst, std::vector<int> &s, double fo,
                               int &melhorI, int &melhorJ);

// [EXERCÍCIO]
// Método da Descida com estratégia de Primeiro Melhora.
double descidaPrimeiroMelhora(const Instancia &inst, std::vector<int> &s);

// [FRAMEWORK]
// Método da Descida para ser usado nos outros métodos.
// Por padrão usa a descidaCompleta
// Basta trocar o método da descida por outro para testar na meta-heurísticas
double descida(const Instancia &inst, std::vector<int> &s);

#endif // BUSCA_LOCAL_H
