#include "Construcao.h"
#include "Aleatorio.h"
#include "Utilitarios.h"

#include <algorithm>  // std::min_element, std::sort, std::find
#include <numeric>    // std::iota
#include <limits>
#include <cstddef>

// ============================================================================
// [EXERCÍCIO] Vizinho Mais Próximo (guloso)
// Aula: Heurística Construtivas
// Slide ref: "PCV - Vizinho mais Próximo"
// Complexidade: O(n^2)
// ============================================================================
double constroiSolucaoGulosaVizinhoMaisProximo(const Instancia &inst,
                                              std::vector<int> &s)
{
    int n = inst.n;

    double fo = 0;
    // Cidades ainda não inseridas na rota (todas, exceto a cidade 0).
    std::vector<int> naoVisitadas(n - 1);

    // Preenche a lista com as cidades de 1 .. (n-1)
    std::iota(naoVisitadas.begin(), naoVisitadas.end(), 1); 

    s.assign(n, -1);
    s[0] = 0; // a cidade origem é sempre a cidade 0

    //TODO
    int cidadeAtual = 0;

    for (int pos = 1; pos < n; pos++){

        double menorDistancia = std::numeric_limits<double>::max();
        int indiceMelhor = -1;

        
        for (int i = 0; i < naoVisitadas.size(); i++){

            int cidade = naoVisitadas[i];

            double distancia = inst.distancia(cidadeAtual, cidade);

            if (distancia < menorDistancia){

                menorDistancia = distancia;
                indiceMelhor = i;
            }
        }

        int proximaCidade = naoVisitadas[indiceMelhor];

        s[pos] = proximaCidade;

        fo += menorDistancia;

        //erase libera as cidades não visitadas
        naoVisitadas.erase(naoVisitadas.begin() + indiceMelhor);

        cidadeAtual = proximaCidade;
    }

    //Tenho que voltar para a cidade 0
    fo += inst.distancia(cidadeAtual, 0);


    return fo;
}

// ============================================================================
// [FRAMEWORK] Construção aleatória
// ============================================================================
double constroiSolucaoAleatoria(const Instancia &inst, std::vector<int> &s)
{
    int n = inst.n;

    // Cria a lista de cidades não visitadas
    std::vector<int> naoVisitadas(n - 1);

    // Preenche a lista com as cidades de 1 .. (n-1)
    std::iota(naoVisitadas.begin(), naoVisitadas.end(), 1);

    embaralhaVetor(naoVisitadas);

    // Redefine a solução com n posições
    s.assign(n, -1);

    // Insere a cidade de origem
    s[0] = 0;

    // Preenche com as demais cidades
    for (int j = 1; j < n; j++) {
        s[j] = naoVisitadas[j - 1];
    }

    return custo(inst, s);
}

// ============================================================================
// [EXERCÍCIO] Vizinho Mais Próximo parcialmente guloso (GRASP)
// ============================================================================
double constroiSolucaoParcialmenteGulosaVizinhoMaisProximo(const Instancia &inst,
                                                           std::vector<int> &s,
                                                           double alpha)
{
    int n = inst.n;
    double fo = 0;

    std::vector<int> naoVisitadas(n - 1);
    std::iota(naoVisitadas.begin(), naoVisitadas.end(), 1);

    s.assign(n, -1);
    s[0] = 0;

    
    //TODO

    return fo;
}

// ============================================================================
// [EXERCÍCIO] Inserção Mais Barata (gulosa)
// ============================================================================
double constroiSolucaoGulosaInsercaoMaisBarata(const Instancia &inst,
                                              std::vector<int> &s)
{
    int n = inst.n;

    double fo = 0;
    std::vector<int> naoVisitadas(n - 1);
    std::iota(naoVisitadas.begin(), naoVisitadas.end(), 1);

    std::vector<int> rota;
    rota.reserve(n);
    rota.push_back(0); // a cidade origem é sempre a cidade 0

    // Monta uma subrota inicial com 3 cidades usando o vizinho mais
    // próximo (mais simples do que aplicar inserção mais barata em uma
    // rota com menos de 3 cidades).

    constroiSolucaoGulosaVizinhoMaisProximo(inst, rota);
    rota.resize(3);
    //TODO

    //tirar as cidades já visitadas do vetor nao visitadas
    for (int i = 1; i < 3; i++){

        for (int j = 0; j < naoVisitadas.size(); j++){
            
            if (naoVisitadas[j] == rota[i]){

                naoVisitadas.erase(naoVisitadas.begin() + j);
                break;
            }
        }
    }

    //calcular o custo inicial com as 3 cidades
    fo = inst.distancia(rota[0], rota[1]) + inst.distancia(rota[1], rota[2]) + inst.distancia(rota[2], rota[0]);

    while (!naoVisitadas.empty()){

        double menorCusto = std::numeric_limits<double>::max();

        int indiceMelhorCidade = -1;
        int melhorPosicao = -1;

        for (int k = 0; k < naoVisitadas.size(); k++){

            int cidadeK = naoVisitadas[k];

            for (int i = 0; i < rota.size(); i++){

                int j = (i + 1) % rota.size();

                int cidadeI = rota[i];
                int cidadeJ = rota[j];

                double custo = inst.distancia(cidadeI, cidadeK) + inst.distancia(cidadeK, cidadeJ) - inst.distancia(cidadeI, cidadeJ);

                if (custo < menorCusto){

                    menorCusto = custo;
                    indiceMelhorCidade = k;
                    melhorPosicao = i + 1;
                }
            }
        }


        int cidadeEscolhida = naoVisitadas[indiceMelhorCidade];


        rota.insert(rota.begin() + melhorPosicao, cidadeEscolhida);

        fo += menorCusto;

        naoVisitadas.erase(naoVisitadas.begin() + indiceMelhorCidade);
    }

    s = rota;


    // A cada passo, insere a cidade k -- entre duas cidades i e j
    // consecutivas já presentes na rota -- que resulta no menor custo de
    // inserção: d(i,k) + d(k,j) - d(i,j).

    //TODO

    return fo;
}

// ============================================================================
// [EXERCÍCIO] Inserção Mais Barata parcialmente gulosa (GRASP)
// ============================================================================
// ============================================================================
double constroiSolucaoParcialmenteGulosaInsercaoMaisBarata(const Instancia &inst,
                                                           std::vector<int> &s,
                                                           double alpha)
{
    int n = inst.n;
    double fo = 0;

    std::vector<int> naoVisitadas(n - 1);
    std::iota(naoVisitadas.begin(), naoVisitadas.end(), 1);

    std::vector<int> rota;
    rota.reserve(n);
    rota.push_back(0);

    //TODO

    return fo;
}
