#include "ILS.h"
#include "BuscaLocal.h"
#include "Utilitarios.h"
#include "Aleatorio.h"

#include <algorithm> // std::swap
#include <iostream>

// ============================================================================
// [EXERCÍCIO]
// ============================================================================
double ils(const Instancia &inst, std::vector<int> &s, int vezesNivel, int ilsMax)
{
    
    int n = inst.n;

    double fo;
    int iter = 0, melhorIter = 0;
    int nivel = 1;

    int x, y;
    double foEstrela = descidaCompleta(inst, s);

    std::vector<int> sEstrela;

    while(iter - melhorIter < ilsMax){

        iter++;


        for(int i = 0; i < nivel + 1; i++){


            x = rand() % s.size();

            y = rand () % s.size();

            while(y == x){
                
                y = rand () % s.size();



            }

            std::swap(s[x], s[y]);


        }

        fo = descidaCompleta(inst, s);

        if(fo < foEstrela){

            foEstrela = fo;
            melhorIter = iter;
            nivel = 1;
            sEstrela = s;        

        }else{

            
            nivel++;
        }


    }
    
    s = sEstrela;
    return foEstrela;

}
