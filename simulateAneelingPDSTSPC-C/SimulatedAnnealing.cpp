#include "SimulatedAnnealing.h"
#include "BuscaLocal.h"   // calculaDelta
#include "Utilitarios.h"
#include "Registro.h"
#include "Aleatorio.h"
#include<iostream>
#include <cmath>      // std::exp
#include <algorithm>  // std::swap

// ============================================================================
// [EXERCÍCIO]
// ============================================================================

double vizinhoRandomicoSimulatedAnnealing(const Instancia &inst, std::vector<int> &s, double fo,
                      int &melhorI, int &melhorJ)
{
    int n = inst.n;
    
    int j = inteiroAleatorio(0, n - 1);
    int i;
    do {
        i = inteiroAleatorio(0, n - 1);
    } while (i == j);

    double delta1 = calculaDelta(inst, s, i, j);
    std::swap(s[i], s[j]);
    double delta2 = calculaDelta(inst, s, i, j);
    double foVizinho = fo - delta1 + delta2;

    std::swap(s[i], s[j]); // desfaz o movimento

    melhorI = i;
    melhorJ = j;

    return foVizinho;
}

double simulatedAnnealing(const Instancia &inst, std::vector<int> &s,
                           double alpha, int saMax,
                           double tempInicial, double tempFinal)
{
    //int n = inst.n;
    std::vector<int> sEstrela = s;

    double temperatura = tempInicial;
    double fo = custo(inst, s);
    double foEstrela = fo;

    const std::string arquivoLog = "SAsaida.txt";
    limpaArquivo(arquivoLog);
    registraProgresso(arquivoLog, 0.0, 0, foEstrela);

    //TODO

    int i = 0, j = 0;
    double novoCusto = 0;
    double delta = 0;
    double x = 0;
    while(temperatura > tempFinal){
        
        for(int w = 0; w < saMax; w++){


            novoCusto = vizinhoRandomicoSimulatedAnnealing(inst, s, fo, i, j);
            
            delta = novoCusto - fo;

            if(delta < 0){

                std::swap(s[i], s[j]);
                fo = novoCusto;

                if(novoCusto < foEstrela){
                    sEstrela = s;
                    foEstrela = fo;
                }


                


            }else{


                x = realAleatorio(0, 1);

                if(x < std::exp(-delta/temperatura)){

                    std::swap(s[i], s[j]);
                    fo = novoCusto;

                }
                


            }

        }

        temperatura = temperatura * alpha;

   }



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
