#include "Instancia.h"

#include <cmath>      // std::hypot
#include <fstream>
#include <iostream>
#include <cstdlib>    // std::exit

// [FRAMEWORK - pronto, não precisa mexer]

void leParametrosInstancia(const std::string &nomeArquivo,
                            int &n,
                            double &melhorValorLiteratura)
{
    std::ifstream arquivo(nomeArquivo);
    if (!arquivo) {
        std::cerr << "O arquivo " << nomeArquivo << " nao pode ser aberto.\n";
        std::exit(EXIT_FAILURE);
    }

    arquivo >> n >> melhorValorLiteratura;
}

Instancia leInstancia(const std::string &nomeArquivoCoordenadas, int n)
{
    std::ifstream arquivo(nomeArquivoCoordenadas);
    if (!arquivo) {
        std::cerr << "O arquivo " << nomeArquivoCoordenadas
                   << " nao pode ser aberto.\n";
        std::exit(EXIT_FAILURE);
    }

    // Lê as coordenadas (x, y) de cada cidade. O índice da cidade no
    // arquivo é usado diretamente como posição no vetor.
    std::vector<double> x(n), y(n);
    int indice;
    double coordX, coordY;
    while (arquivo >> indice >> coordX >> coordY) {
        x[indice] = coordX;
        y[indice] = coordY;
    }

    // Monta a instância e calcula a matriz de distâncias euclidianas.
    // Observação: std::hypot(a, b) calcula sqrt(a*a + b*b) com melhor
    // precisão numérica do que fazer a conta "na mão" -- é a função
    // certa da biblioteca padrão para distância euclidiana em 2D.
    Instancia inst;
    inst.n = n;
    inst.x = x;
    inst.y = y;
    inst.d.assign(n, std::vector<double>(n, 0.0));

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            double dist = std::hypot(x[i] - x[j], y[i] - y[j]);
            inst.d[i][j] = dist;
            inst.d[j][i] = dist; // matriz simétrica: d[i][j] == d[j][i]
        }
    }

    return inst;
}
