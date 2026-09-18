#include "BuscaLocal.h"
#include "Utilitarios.h"
#include "Registro.h"
#include "Aleatorio.h"

#include <numeric>   // std::iota
#include <algorithm> // std::swap
#include <iostream>

// ============================================================================
// [EXERCÍCIO]
// ============================================================================
double calculaDelta(const Instancia &inst, const std::vector<int> &s, int i, int j)
{
    int n = inst.n;
    int iAntes = (i == 0) ? n - 1 : i - 1;
    int iDepois = (i == n - 1) ? 0 : i + 1;
    int jAntes = (j == 0) ? n - 1 : j - 1;
    int jDepois = (j == n - 1) ? 0 : j + 1;

    return inst.d[s[iAntes]][s[i]] + inst.d[s[i]][s[iDepois]]
         + inst.d[s[jAntes]][s[j]] + inst.d[s[j]][s[jDepois]];
}


// ============================================================================
// [EXERCÍCIO]
// ============================================================================
double descida(const Instancia &inst, std::vector<int> &s)
{
    
    return descidaCompleta(inst, s);

}


// ============================================================================
// [EXERCÍCIO]
// ============================================================================
double melhorVizinho(const Instancia &inst, std::vector<int> &s, double fo,
                      int &melhorI, int &melhorJ)
{
    
    double foMelhorVizinho = fo;

    //TODO

    for(int i = 0; i < inst.n; i++){

        
        for(int j = i+1; j < inst.n; j++){


            //No delta eu apenas calculo o custo das arestas

            double deltaAntigo = calculaDelta(inst, s, i, j);
            std::swap(s[i], s[j]);
            double deltaNovo = calculaDelta(inst, s, i, j);
            std::swap(s[i], s[j]);


            double novoValor = fo + deltaNovo - deltaAntigo;

            if(novoValor < foMelhorVizinho){
                melhorI = i;
                melhorJ = j;

                foMelhorVizinho = novoValor;

            }





        }







    }


    return foMelhorVizinho;
}

// ============================================================================
// [EXERCÍCIO]
// ============================================================================
double descidaCompleta(const Instancia &inst, std::vector<int> &s)
{
    const std::string arquivoLog = "DescidaCompleta.txt";
    double fo = custo(inst, s);

    limpaArquivo(arquivoLog);
    registraProgresso(arquivoLog, 0.0, 0, fo);

    bool melhorou = true;
    
    //TODO

    //toda vez que fizer troca tem que atualizar fo


    while(melhorou){
        //tenho que definir essas variáveis inicialmente, caso o contrário daria o erro de loop
        int melhorI = -1, melhorJ = -1;

        double novo = melhorVizinho(inst, s,  fo, melhorI, melhorJ);
        if(novo < fo &&  melhorI != -1){

            std::swap(s[melhorI], s[melhorJ]);

            fo = novo; 



        }else{
            melhorou = false;
        }


    }



    return fo;
}

// ============================================================================
// [FRAMEWORK]
// ============================================================================
double vizinhoRandomico(const Instancia &inst, std::vector<int> &s, double fo,
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

// ============================================================================
// [EXERCÍCIO]
// ============================================================================
double descidaRandomica(const Instancia &inst, std::vector<int> &s, int iterMax)
{
    //int n = inst.n;
    double fo = custo(inst, s);
    int iter = 0;

    while(iter < iterMax){

        int melhorI = 1, melhorJ = -1;

        double melhora = vizinhoRandomico(inst, s, fo, melhorI, melhorJ);

        if(melhora < fo){

            std::swap(s[melhorI], s[melhorJ]);
            fo = melhora;
            iter = 0;
        }else{
            iter++;
        }


    }
    
    return fo;
}

// ============================================================================
// [EXERCÍCIO]
// ============================================================================
double vizinhoPrimeiroMelhora(const Instancia &inst, std::vector<int> &s, double fo,
                               int &melhorI, int &melhorJ)
{
    int n = inst.n;
    double foMelhorVizinho = fo;
    //bool melhorou = false;

    // Visita as posições da rota em ordem aleatória, para não introduzir
    // um viés sistemático de sempre explorar as posições iniciais primeiro.
    std::vector<int> ordem(n);
    std::iota(ordem.begin(), ordem.end(), 0);
    embaralhaVetor(ordem);
    //ordem é um vetor de índices já embaralhado, posso apenas percorrer ele

    //TODO
    int pos1, pos2;
    for(int i = 0; i < n; i++){

        pos1 = ordem[i];

        for(int j = i + 1; j < n; j++){

            pos2 = ordem[j];

            double deltaAntigo = calculaDelta(inst, s, pos1, pos2);

            std::swap(s[pos1], s[pos2]);

            double deltaNovo = calculaDelta(inst, s, pos1, pos2);

            std::swap(s[pos1], s[pos2]);

            double valorNovo = fo + deltaNovo - deltaAntigo;

            if(valorNovo < fo){
                melhorI = pos1;
                melhorJ = pos2;

                return valorNovo; 
            }






        }


    }

    return foMelhorVizinho;
}

// ============================================================================
// [EXERCÍCIO]
// ============================================================================
double descidaPrimeiroMelhora(const Instancia &inst, std::vector<int> &s)
{
    const std::string arquivoLog = "DescidaPrimeiroMelhora.txt";
    double fo = custo(inst, s);

    limpaArquivo(arquivoLog);
    registraProgresso(arquivoLog, 0.0, 0, fo);

    bool melhorou = true;

    //TODO

    while(melhorou){
        
        int melhorI = -1, melhorJ = -1;

        double melhora = vizinhoPrimeiroMelhora(inst, s, fo, melhorI, melhorJ);

        if(melhorI != -1){

            std::swap(s[melhorI], s[melhorJ]);
            fo = melhora;

        }else{

            break;

            //eu não preciso de contador pois a vizinho primeiro melhora já faz o percusso de N no último caso
            //Logo, se não melhorar significa que chegamos em um mínimo local

        }




    }

    return fo;
}
