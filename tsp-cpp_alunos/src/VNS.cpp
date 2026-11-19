#include "VNS.h"
#include "VND.h"          // vnd()
#include "BuscaLocal.h"   // calculaDelta, descidaCompleta
#include "Utilitarios.h"
#include "Aleatorio.h"

#include <algorithm>      // std::swap
#include <iostream>
#include <vector>

// ============================================================================
// [FRAMEWORK] - Gera um vizinho aleatório
// ============================================================================
double geraUmVizinhoAleatorio(const Instancia &inst,
                              std::vector<int> &s, double fo)
{
    int n = inst.n;

    int i = inteiroAleatorio(0, n - 1);
    int j;

    do {
        j = inteiroAleatorio(0, n - 1);
    } while (i == j);

    double delta1 = calculaDelta(inst, s, i, j);

    std::swap(s[i], s[j]);

    double delta2 = calculaDelta(inst, s, i, j);

    return fo - delta1 + delta2;
}

// ============================================================================
// [FRAMEWORK] - Gera k vizinhos aleatórios
// ============================================================================
double geraKVizinhosAleatorios(const Instancia &inst,
                               std::vector<int> &s,
                               double fo, int k)
{
    double foVizinho = fo;

    for (int i = 0; i < k; i++)
    {
        foVizinho = geraUmVizinhoAleatorio(
            inst, s, foVizinho
        );
    }

    return foVizinho;
}

// ============================================================================
// [EXERCÍCIO] - VNS (Variable Neighborhood Search)
// ============================================================================
double vns(const Instancia &inst, std::vector<int> &s,
           int vnsMax, int r)
{
    double fo = custo(inst, s);

    int iter = 0;
    int melhorIter = 0;

    if (inst.n < 2 || vnsMax <= 0 || r <= 0)
        return fo;

    while (iter - melhorIter < vnsMax)
    {
        int k = 1;

        while (k <= r && iter - melhorIter < vnsMax)
        {
            // Copia a solução atual
            std::vector<int> sLinha = s;

            // Shaking: realiza k trocas aleatórias
            geraKVizinhosAleatorios(inst, sLinha, fo, k);

            // Busca local: Descida Completa
            double foDuasLinhas = descidaCompleta(inst, sLinha);

            iter++;

            // Critério de aceitação
            if (foDuasLinhas < fo)
            {
                s = sLinha;
                fo = foDuasLinhas;

                melhorIter = iter;

                // Retorna à primeira vizinhança
                k = 1;
            }
            else
            {
                // Avança para a próxima vizinhança
                k++;
            }
        }
    }

    return fo;
}

// ============================================================================
// [EXERCÍCIO] - GVNS (General Variable Neighborhood Search)
// ============================================================================
double gvns(const Instancia &inst, std::vector<int> &s,
            int vnsMax, int r)
{
    double fo = custo(inst, s);

    int iter = 0;
    int melhorIter = 0;

    if (inst.n < 2 || vnsMax <= 0 || r <= 0)
        return fo;

    while (iter - melhorIter < vnsMax)
    {
        int k = 1;

        while (k <= r && iter - melhorIter < vnsMax)
        {
            // Copia a solução atual
            std::vector<int> sLinha = s;

            // Shaking: realiza k trocas aleatórias
            geraKVizinhosAleatorios(inst, sLinha, fo, k);

            // Busca local: VND
            double foDuasLinhas = vnd(inst, sLinha, r);

            iter++;

            // Critério de aceitação
            if (foDuasLinhas < fo)
            {
                s = sLinha;
                fo = foDuasLinhas;

                melhorIter = iter;

                // Retorna à primeira vizinhança
                k = 1;
            }
            else
            {
                // Avança para a próxima vizinhança
                k++;
            }
        }
    }

    return fo;
}