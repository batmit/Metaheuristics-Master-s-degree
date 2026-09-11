#include "Aleatorio.h"

#include <algorithm>  // std::shuffle
#include <random>

// [FRAMEWORK - pronto, não precisa mexer]

// O gerador de números aleatórios fica "escondido" aqui dentro do arquivo
// .cpp (variável com "static" tem escopo limitado a este arquivo). Ele é
// inicializado uma única vez, a partir de uma semente aleatória, então o
// programa já funciona corretamente mesmo que ninguém chame semente()
// explicitamente.
static std::mt19937 gerador(std::random_device{}());

void semente(unsigned valor)
{
    gerador.seed(valor);
}

void sementeAleatoria()
{
    std::random_device fonteDeEntropia;
    gerador.seed(fonteDeEntropia());
}

int inteiroAleatorio(int minimo, int maximo)
{
    std::uniform_int_distribution<int> distribuicao(minimo, maximo);
    return distribuicao(gerador);
}

double realAleatorio(double minimo, double maximo)
{
    std::uniform_real_distribution<double> distribuicao(minimo, maximo);
    return distribuicao(gerador);
}

void embaralhaVetor(std::vector<int> &vetor)
{
    // std::shuffle já implementa o embaralhamento (algoritmo de
    // Fisher-Yates) de forma correta e eficiente -- não reimplemente isso
    // na mão com trocas aleatórias.
    std::shuffle(vetor.begin(), vetor.end(), gerador);
}
