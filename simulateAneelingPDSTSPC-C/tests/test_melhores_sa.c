#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include "SA.h"

int main(void)
{
    double tc[4][4] = {{0,1,6,9},{3,0,2,8},{7,4,0,5},{2,10,3,0}};
    double *linhas[] = {tc[0],tc[1],tc[2],tc[3]};
    Instancia inst = {.n=4,.tempoCaminhao=linhas};
    const int rotas[6][3] = {{1,2,3},{1,3,2},{2,1,3},{2,3,1},{3,1,2},{3,2,1}};
    double custos[6]; int ordem[] = {0,1,2,3,4,5};
    for (int i = 0; i < 6; i++) {
        custos[i] = tc[0][rotas[i][0]] + tc[rotas[i][0]][rotas[i][1]] +
                    tc[rotas[i][1]][rotas[i][2]] + tc[rotas[i][2]][0];
    }
    /* Recebe primeiro as piores, obrigando a substituir slots preenchidos. */
    for (int i = 0; i < 6; i++)
        for (int j = i + 1; j < 6; j++)
            if (custos[ordem[j]] > custos[ordem[i]]) {
                int aux = ordem[i]; ordem[i] = ordem[j]; ordem[j] = aux;
            }
    Solucao candidata = {0}, melhores[NUM_MELHORES_SA] = {0};
    alocarSolucao(&inst,&candidata);
    for (int i = 0; i < NUM_MELHORES_SA; i++) alocarSolucao(&inst,&melhores[i]);
    int quantidade = 0;
    assert(construirSolucaoInicial(&inst,&candidata));
    assert(escalonarMissoesDrone(&inst,&candidata));
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 3; j++) candidata.missaoCaminhao[j].n = rotas[ordem[i]][j];
        assert(calcularMakespan(&inst,&candidata));
        assert(atualizarMelhoresSolucoesSA(&inst,&candidata,melhores,&quantidade));
        assert(quantidade == (i < 4 ? i + 1 : 4));
        assert(!atualizarMelhoresSolucoesSA(&inst,&candidata,melhores,&quantidade));
        for (int j = 0; j < quantidade; j++) {
            assert(melhores[j].missaoCaminhao != candidata.missaoCaminhao);
            assert(melhores[j].missaoDrone != candidata.missaoDrone);
            assert(melhores[j].prefixosDrone != candidata.prefixosDrone);
            double armazenado = melhores[j].makespan;
            assert(calcularMakespan(&inst,&melhores[j]) && melhores[j].makespan == armazenado);
            if (j) assert(melhores[j-1].makespan <= melhores[j].makespan);
            for (int k = 0; k < j; k++)
                assert(melhores[j].missaoCaminhao != melhores[k].missaoCaminhao);
        }
    }
    for (int i = 0; i < 4; i++) assert(melhores[i].makespan == custos[ordem[5-i]]);
    /* Uma solucao descartada anteriormente nao pode expulsar uma melhor. */
    for (int j = 0; j < 3; j++) candidata.missaoCaminhao[j].n = rotas[ordem[0]][j];
    assert(calcularMakespan(&inst,&candidata));
    assert(!atualizarMelhoresSolucoesSA(&inst,&candidata,melhores,&quantidade));
    candidata.missaoCaminhao[0].n = 99;
    for (int i = 0; i < 4; i++) assert(calcularMakespan(&inst,&melhores[i]));
    liberarSolucao(&candidata);
    for (int i = 0; i < NUM_MELHORES_SA; i++) liberarSolucao(&melhores[i]);
    return 0;
}
