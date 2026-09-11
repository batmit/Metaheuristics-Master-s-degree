#ifndef ALEATORIO_H
#define ALEATORIO_H

#include <vector>

/*
 * ============================================================================
 *  Módulo: Aleatorio
 * ============================================================================
 *  O código original em C usava rand()/srand(), que é simples mas tem
 *  qualidade estatística ruim e comportamento diferente entre compiladores.
 *
 *  Este módulo concentra toda a geração de números aleatórios do projeto
 *  usando a biblioteca <random> do C++11, especificamente o gerador
 *  Mersenne Twister (std::mt19937), que é o gerador de uso geral mais
 *  recomendado para fins didáticos e de pesquisa.
 *
 *  Toda vez que um método precisar de aleatoriedade (sortear uma cidade,
 *  embaralhar uma rota, decidir uma probabilidade de mutação, etc.), use
 *  as funções deste módulo em vez de chamar rand() diretamente. Assim,
 *  toda a aleatoriedade do programa fica controlada em um único lugar,
 *  o que facilita reproduzir experimentos (usando semente()) e depurar.
 * ============================================================================
 */

// Define a semente do gerador de números aleatórios. Usar sempre a mesma
// semente faz o programa gerar exatamente a mesma sequência de números
// aleatórios em execuções diferentes -- muito útil para comparar métodos
// de forma justa ou para depurar um bug.
void semente(unsigned valor);

// Define uma semente "aleatória de verdade", baseada em uma fonte de
// entropia do sistema operacional (std::random_device). Use no início do
// programa quando você quiser resultados diferentes a cada execução.
void sementeAleatoria();

// Sorteia um inteiro no intervalo fechado [minimo, maximo].
int inteiroAleatorio(int minimo, int maximo);

// Sorteia um número real (double) no intervalo [minimo, maximo).
double realAleatorio(double minimo, double maximo);

// Embaralha os elementos do vetor em uma ordem aleatória (equivalente a
// sortear uma permutação uniformemente ao acaso).
void embaralhaVetor(std::vector<int> &vetor);

#endif // ALEATORIO_H
