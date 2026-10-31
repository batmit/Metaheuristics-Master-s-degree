#include "LAHC.h"
#include "Utilitarios.h"

#include <iostream>

// ============================================================================
// [EXERCÍCIO - método futuro do cronograma]
// ============================================================================
double lahc(const Instancia &inst, std::vector<int> &s, int tamanhoLista, int lahcMax)
{
    (void)tamanhoLista;
    (void)lahcMax;

    std::vector<int> sEstrela = s;
    double fo = custo(inst, s);


    int x, y;
    double foEstrela = fo;
    int indice;
    std::vector<double> historico;
    std::vector<int> novoS;
    double foNovo;
    for(int i = 0; i < tamanhoLista; i++){

        historico.push_back(fo);

    }

    int iter = 0;
    int r = 0;
    while(r <= lahcMax){

        novoS = s;

        x = rand() % s.size();

        y = rand () % s.size();

        while(y == x){
                
            y = rand () % s.size();



        }

        std::swap(novoS[x], novoS[y]);
        
        foNovo = custo(inst, novoS);

        indice = iter % tamanhoLista;

        if(foNovo <= fo || foNovo <= historico[indice]){

            if(foNovo < fo){
                r = 0;
            }    

            s = novoS;
            fo = foNovo;

        }

        if(fo < foEstrela){

            sEstrela = s;
            foEstrela = fo;

        }


        if(fo < historico[indice]){
            historico[indice] = fo;
        }

        iter++;
        r++;
    }

    s = sEstrela;

    return custo(inst, s);
}

