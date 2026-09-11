#ifndef MEMETICO_H
#define MEMETICO_H

#include <vector>
#include "Instancia.h"

/*
 * ============================================================================
 *  Módulo: Memetico
 * ============================================================================
 *  Algoritmo Memético = Algoritmo Genético + Busca Local: idêntico ao
 *  AlgoritmoGenetico, mas, sempre que um indivíduo bate o recorde de
 *  melhor fo da população, aplica-se busca local (descida) sobre ele
 *  antes de seguir -- um refinamento "lamarckiano" da busca evolutiva.
 *  Usa os mesmos operadores de OperadoresGeneticos.h.
 * ============================================================================
 */

// maxGeracoes: critério de parada adicional (além do desvio-padrão): o
//              algoritmo para após esse número de gerações mesmo que a
//              população ainda não tenha convergido.
// [EXERCÍCIO]
double memetico(const Instancia &inst, std::vector<int> &s,
                 int nind, double maxDesvio, int maxGeracoes,
                 double probCrossover, double probMutacao,
                 int tipoOperador);

#endif // MEMETICO_H
