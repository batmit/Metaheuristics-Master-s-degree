#ifndef INSTANCIA_H
#define INSTANCIA_H

#include <string>
#include <vector>

/*
 * ============================================================================
 *  Módulo: Instancia
 * ============================================================================
 *  Este módulo concentra TUDO que descreve uma instância do Problema do
 *  Caixeiro Viajante (PCV/TSP):
 *
 *    - o número de cidades (n);
 *    - a matriz de distâncias entre cada par de cidades (d[i][j]).
 *
 *  No código original em C, esses dois dados (n e a matriz d) sempre
 *  "viajavam juntos" como dois parâmetros separados em praticamente toda
 *  função (int n, float **d). Isso é chato de escrever e fácil de bagunçar
 *  (por exemplo, esquecer de atualizar um dos dois).
 *
 *  Aqui, agrupamos os dois em uma única struct. Continuamos passando os
 *  dados de forma bem explícita para cada método (nada fica escondido
 *  dentro de uma classe complexa) — só que agora em um único parâmetro:
 *
 *      double custo(const Instancia &inst, const std::vector<int> &s);
 *
 *  Dentro de qualquer função, você acessa os dados normalmente:
 *
 *      inst.n            // número de cidades
 *      inst.d[i][j]      // distância entre a cidade i e a cidade j
 *
 *  IMPORTANTE: a Instancia é somente LEITURA durante a execução dos
 *  métodos de otimização (ela não muda). Por isso, ela é quase sempre
 *  passada por "const Instancia &" (referência constante): isso evita
 *  copiar a matriz inteira a cada chamada de função, mas impede que o
 *  método altere os dados do problema por engano.
 * ============================================================================
 */
struct Instancia {
    int n = 0;                                  // número de cidades
    std::vector<std::vector<double>> d;          // matriz n x n de distâncias
    std::vector<double> x, y;                    // coordenadas das cidades
                                                  // (usadas para plotar a rota; veja Plotagem.h)

    // Retorna a distância entre as cidades i e j.
    // Um pequeno "atalho" para inst.d[i][j] — puramente estilístico,
    // use como preferir (inst.d[i][j] ou inst.distancia(i,j) dão no mesmo).
    double distancia(int i, int j) const { return d[i][j]; }
};

/* ----------------------------------------------------------------------
 * Lê o arquivo de "informações" da instância, no formato:
 *
 *     <numero_de_cidades> <melhor_valor_conhecido_na_literatura>
 *
 * Exemplo (C50INFO.TXT):  "51 429.98"
 *
 * Preenche n e melhorValorLiteratura por referência.
 * -------------------------------------------------------------------- */
void leParametrosInstancia(const std::string &nomeArquivo,
                            int &n,
                            double &melhorValorLiteratura);

/* ----------------------------------------------------------------------
 * Lê o arquivo de coordenadas da instância, no formato (uma linha por
 * cidade, em qualquer ordem):
 *
 *     <indice_da_cidade> <coordenada_x> <coordenada_y>
 *
 * Exemplo (C50.TXT):  "0 30 40"
 *
 * A partir das coordenadas, calcula a matriz de distâncias euclidianas
 * entre todos os pares de cidades e devolve uma Instancia já pronta
 * para uso (com n cidades e a matriz d preenchida).
 * -------------------------------------------------------------------- */
Instancia leInstancia(const std::string &nomeArquivoCoordenadas, int n);

#endif // INSTANCIA_H
