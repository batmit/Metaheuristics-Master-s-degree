#ifndef VNS_H
#define VNS_H

#include <vector>
#include "Instancia.h"

/*
 * ============================================================================
 *  Módulo: VNS (Variable Neighborhood Search / Busca em Vizinhança Variável)
 * ============================================================================
 *  A cada iteração, percorre vizinhanças cada vez "mais distantes" (k =
 *  1, 2, ..., r): sorteia k trocas aleatórias (shaking/agitação) e aplica
 *  busca local. Se o resultado melhorar a solução corrente, volta para a
 *  primeira vizinhança (k = 1); caso contrário, avança para a vizinhança
 *  seguinte (k + 1).
 * ============================================================================
 */

// [EXERCÍCIO]
double vns(const Instancia &inst, std::vector<int> &s, int vnsMax, int r);

// [EXERCÍCIO]
// Gera um vizinho aleatório de s (uma única troca de duas cidades
// escolhidas ao acaso), aplicando o movimento diretamente em s. Devolve o
// custo estimado do vizinho gerado.
double geraUmVizinhoAleatorio(const Instancia &inst, std::vector<int> &s, double fo);

// [EXERCÍCIO]
// Aplica k perturbações (chamadas sucessivas a geraUmVizinhoAleatorio)
// sobre s. Usado como "shaking" da k-ésima vizinhança do VNS.
double geraKVizinhosAleatorios(const Instancia &inst, std::vector<int> &s, double fo, int k);

#endif // VNS_H
