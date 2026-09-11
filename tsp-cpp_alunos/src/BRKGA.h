#ifndef BRKGA_H
#define BRKGA_H

#include <vector>
#include "Instancia.h"

/*
 * ============================================================================
 *  Módulo: BRKGA (Algoritmo Genético com Chaves Aleatórias Viciadas)
 * ============================================================================
 *  ESQUELETO -- método ainda a ser implementado (cronograma: aula sobre
 *  BRKGA e Algoritmos Meméticos).
 *
 *  Ideia: cada indivíduo é representado por um vetor de n números reais
 *  aleatórios em [0,1) (as "chaves"). Uma solução (rota) é obtida
 *  ordenando as cidades pelo valor de suas chaves (um "decodificador").
 *  A população é dividida em elite, não-elite e mutantes; a cada
 *  geração, a elite passa direto para a próxima geração, mutantes
 *  totalmente novos são inseridos, e o restante é gerado por cruzamento
 *  "viciado" (biased) entre um pai elite e um pai não-elite, dando mais
 *  peso às chaves do pai elite.
 *
 *  std::vector<double> é a representação natural para as chaves de um
 *  indivíduo; std::sort com um comparador que compara pelas chaves (por
 *  exemplo, ordenando um vetor de índices) implementa o decodificador.
 * ============================================================================
 */

// [EXERCÍCIO - método futuro do cronograma]
// tamanhoPop: tamanho da população de vetores de chaves aleatórias.
// fracaoElite: fração da população considerada "elite" (0..1).
// fracaoMutantes: fração da população recriada como mutantes (0..1).
// probEliteNoFilho: probabilidade de cada chave do filho vir do pai elite.
// maxGeracoes: critério de parada.
double brkga(const Instancia &inst, std::vector<int> &s,
             int tamanhoPop, double fracaoElite, double fracaoMutantes,
             double probEliteNoFilho, int maxGeracoes);

#endif // BRKGA_H
