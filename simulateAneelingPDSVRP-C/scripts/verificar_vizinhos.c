/* Percorre vizinhos aleatorios e compara cada custo incremental com a
 * avaliacao integral. O processo recebe uma instancia oficial por chamada. */
#include "../lerInstancias.h"
#include "../cv.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/* Pequenas usam S=2,3; grandes usam S=2,3,4,5, como no catalogo. */
int main(int argc, char **argv)
{
    assert(argc == 3);
    int configuracoes = 0, movimentos[5] = {0};
    for (int frota = 2; frota <= 5; ++frota) {
        Instancia inst = {0};
        Solucao atual = {0}, vizinho = {0}, integral = {0};
        assert(carregarInstancia(argv[1], argv[2], frota, &inst));
        if (inst.n <= 31 && frota > 3) { liberarInstancia(&inst); break; }
        assert(alocarSolucao(&inst, &atual) && alocarSolucao(&inst, &vizinho) &&
               alocarSolucao(&inst, &integral));
        assert(construirSolucaoInicial(&inst, &atual));
        srand((unsigned)(inst.n * 100 + frota));
        for (int i = 0; i < 500; ++i) {
            TipoMovimento tipo;
            assert(gerarVizinhoAvaliado(&inst, &atual, &vizinho, &tipo) == 1);
            ++movimentos[tipo];
            assert(copiarSolucao(&inst, &vizinho, &integral));
            assert(calcularMakespan(&inst, &integral));
            assert(fabs(integral.makespan - vizinho.makespan) < 1e-7);
            for (int c = 0; c < frota; ++c)
                assert(fabs(integral.rotasCaminhao[c].tempo - vizinho.rotasCaminhao[c].tempo) < 1e-7);
            Solucao aux = atual; atual = vizinho; vizinho = aux;
        }
        ++configuracoes;
        liberarSolucao(&atual); liberarSolucao(&vizinho); liberarSolucao(&integral);
        liberarInstancia(&inst);
    }
    for (int i = 0; i < 5; ++i) assert(movimentos[i] > 0);
    printf("%d\n", configuracoes);
    return 0;
}
