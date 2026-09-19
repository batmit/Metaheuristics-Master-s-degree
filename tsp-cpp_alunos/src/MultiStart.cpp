#include "MultiStart.h"
#include "Construcao.h"
#include "BuscaLocal.h"
#include "Utilitarios.h"

// ============================================================================
// [EXERCÍCIO]
// ============================================================================
double multiStart(const Instancia &inst, std::vector<int> &s, int itermax)
{
    // Primeira tentativa: construção gulosa, usada como referência inicial.
    std::vector<int> sEstrela;
    double foEstrela = 0;
    
    //TODO

    //toda vez que eu melhor eu zero o contador

    //foEstrela = constroiSolucaoAleatoria(inst, sEstrela);
    foEstrela = constroiSolucaoGulosaInsercaoMaisBarata(inst, sEstrela);

    //melhora 
    foEstrela = descidaPrimeiroMelhora(inst, sEstrela);

    std::vector<int> ValorAleatorio;
    double  fo = 0;
    for(int i = 0; i < itermax; i++){

        //Apenas pra construir o vetor de forma aleatória
        fo = constroiSolucaoAleatoria(inst, ValorAleatorio);

        //heurística
        fo = descidaCompleta(inst, ValorAleatorio);
        //fo = descidaPrimeiroMelhora(inst, ValorAleatorio);
        

        if(fo < foEstrela){

            foEstrela = fo;
            sEstrela = ValorAleatorio;

            i = -1;

        }




    }


    s = sEstrela;
    return foEstrela;
}
