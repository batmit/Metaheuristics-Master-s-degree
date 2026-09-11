#include "SimulatedAnnealing.h"
#include "BuscaLocal.h"   // calculaDelta
#include "Utilitarios.h"
#include "Registro.h"
#include "Aleatorio.h"

#include <cmath>      // std::exp
#include <algorithm>  // std::swap

// ============================================================================
// [EXERCÍCIO]
// ============================================================================
double simulatedAnnealing(const Instancia &inst, std::vector<int> &s,
                           double alpha, int saMax,
                           double tempInicial, double tempFinal)
{
    int n = inst.n;
    std::vector<int> sEstrela = s;

    double temperatura = tempInicial;
    double fo = custo(inst, s);
    double foEstrela = fo;

    const std::string arquivoLog = "SAsaida.txt";
    limpaArquivo(arquivoLog);
    registraProgresso(arquivoLog, 0.0, 0, foEstrela);

    //TODO

    s = sEstrela;
    registraProgresso(arquivoLog, 0.0, 0, foEstrela);
    return foEstrela;
}

// ============================================================================
// [FRAMEWORK]
// ============================================================================
double calculaTemperaturaInicial(const Instancia &inst, std::vector<int> &s,
                                  double beta, double gamma, int saMax)
{
    int n = inst.n;
    double temperatura = 10.0; // chute inicial

    bool continua = true;
    while (continua) {
        int aceitos = 0;

        for (int iterT = 0; iterT < saMax; iterT++) {
            int i = inteiroAleatorio(0, n - 1);
            int j;
            do {
                j = inteiroAleatorio(0, n - 1);
            } while (j == i);

            double delta1 = calculaDelta(inst, s, i, j);
            std::swap(s[i], s[j]);
            double delta2 = calculaDelta(inst, s, i, j);
            double delta = delta2 - delta1;

            if (delta < 0) {
                aceitos++;
            } else if (realAleatorio(0.0, 1.0) < std::exp(-delta / temperatura)) {
                aceitos++;
            }

            std::swap(s[i], s[j]); // desfaz o movimento (aqui só testamos)
        }

        if (aceitos < gamma * saMax) {
            temperatura *= beta; // ainda aceitando pouco: aquece mais
        } else {
            continua = false;
        }
    }
    return temperatura;
}
