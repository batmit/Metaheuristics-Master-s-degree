#include "Utilitarios.h"

#include <cmath>
#include <iostream>
#include <numeric> // std::accumulate

// [FRAMEWORK - pronto, não precisa mexer]

double custo(const Instancia &inst, const std::vector<int> &s)
{
    double distanciaPercorrida = 0.0;
    int n = inst.n;

    for (int j = 0; j < n - 1; j++) {
        distanciaPercorrida += inst.d[s[j]][s[j + 1]];
    }
    distanciaPercorrida += inst.d[s[n - 1]][s[0]]; // fecha o ciclo

    return distanciaPercorrida;
}

void imprimeRota(const std::vector<int> &s)
{
    for (std::size_t j = 0; j < s.size(); j++) {
        std::cout << s[j] << " -> ";
    }
    std::cout << s[0] << '\n';
}

double desvioPadrao(const std::vector<double> &valores)
{
    int n = static_cast<int>(valores.size());

    // std::accumulate soma os elementos do intervalo -- evita um laço
    // "na mão" só para somar um vetor.
    double media = std::accumulate(valores.begin(), valores.end(), 0.0) / n;

    double somaDosQuadrados = 0.0;
    for (double v : valores) {
        somaDosQuadrados += (v - media) * (v - media);
    }

    return std::sqrt(somaDosQuadrados / (n - 1));
}
