#include "VND.h"
#include "BuscaLocal.h" // calculaDelta
#include "Utilitarios.h"

#include <algorithm> // std::swap
#include <iostream>

// ============================================================================
// [EXERCÍCIO]
// ============================================================================
double vnd(const Instancia &inst, std::vector<int> &s, int r)
{
    double fo = custo(inst, s);
    int k = 1;

    while (k <= r)
    {
        double melhorFO = fo;
        int melhorI = -1;
        int melhorJ = -1;

        // Explora a vizinhança Nk
        for (int i = 0; i + k < inst.n; i++)
        {
            int j = i + k;

            double delta1 = calculaDelta(inst, s, i, j);

            std::swap(s[i], s[j]);

            double delta2 = calculaDelta(inst, s, i, j);

            double foVizinho = fo - delta1 + delta2;

            // Desfaz a troca
            std::swap(s[i], s[j]);

            if (foVizinho < melhorFO)
            {
                melhorFO = foVizinho;
                melhorI = i;
                melhorJ = j;
            }
        }

        // Se encontrou melhoria
        if (melhorI != -1)
        {
            std::swap(s[melhorI], s[melhorJ]);

            fo = melhorFO;

            // Retorna à primeira vizinhança
            k = 1;
        }
        else
        {
            // Avança para a próxima vizinhança
            k++;
        }
    }

    return fo;
}