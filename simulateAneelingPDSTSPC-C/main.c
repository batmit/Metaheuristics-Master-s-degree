#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "SA.h"
#include "main.h"
#include "lerInstancias.h"
#include "cv.h"

int main(void)
{
    Instancia inst = {0};
    Solucao solucao = {0};

    if (!lerInstancia("18-r-e.txt", &inst))
        return EXIT_FAILURE;

    alocarInstancias(&inst);

    if (inst.tempoCaminhao == NULL || inst.tempoDrone == NULL)
    {
        liberarInstancia(&inst);
        return EXIT_FAILURE;
    }

    /* A alocação do caminhão pode ter falhado parcialmente. */
    for (int i = 0; i < inst.n; i++)
    {
        if (inst.tempoCaminhao[i] == NULL)
        {
            liberarInstancia(&inst);
            return EXIT_FAILURE;
        }
    }

    construirMatrizCaminhao(&inst);

    if (!lerTemposDrone("tau_kj_18-r-e.txt", &inst))
    {
        liberarInstancia(&inst);
        return EXIT_FAILURE;
    }

    alocarSolucao(&inst, &solucao);

    if (solucao.missaoCaminhao == NULL || solucao.missaoDrone == NULL)
    {
        liberarSolucao(&solucao);
        liberarInstancia(&inst);
        return EXIT_FAILURE;
    }

    //Na solução inicial criamos uma rota utilizando apenas o caminhão
    if (!construirSolucaoInicial(&inst, &solucao))
    {
        fprintf(stderr, "Erro ao construir solucao inicial.\n");
        liberarSolucao(&solucao);
        liberarInstancia(&inst);
        return EXIT_FAILURE;
    }


    ParametrosSA parametros = parametrosSAPadrao();
    //const unsigned int semente = 123;
    const unsigned int semente = 123;

    srand(semente);
    double temperaturaInicial;
    clock_t inicio = clock();
    printf("Makespan inicial: %.2f minutos\n", solucao.makespan);
    if (!calculaTemperaturaInicial(&inst, &solucao, &parametros, &temperaturaInicial))
    {
        fprintf(stderr, "Erro ao calibrar temperatura inicial.\n");
        liberarSolucao(&solucao);
        liberarInstancia(&inst);
        return EXIT_FAILURE;
    }
    
    parametros.tempInicial = temperaturaInicial;
    
    if (!simulatedAnnealing(&inst, &solucao, &parametros)) {
        fprintf(stderr, "Erro ao executar simulated annealing.\n");
        liberarSolucao(&solucao);
        liberarInstancia(&inst);
        return EXIT_FAILURE;
    }
    
    printf("Melhor makespan: %.2f minutos\n", solucao.makespan);
    
    printf("Rota do caminhao: 0");
    for (int i = 0; i < solucao.numMissoesCaminhao; i++){
        printf(" -> %d", solucao.missaoCaminhao[i].n);
    }
        printf(" -> 0\n");
    
    for (int i = 0; i < solucao.numMissoesDrone; i++) {
        const MissaoDrone *m = &solucao.missaoDrone[i];
        printf("Cliente %d: %d drones; inicio %.2f; fim %.2f\n",
               m->cliente, m->qtdDrones, m->tempoInicio, m->tempoFim);
    }
    
    printf("Tempo de CPU (calibracao + SA): %.3f segundos\n",
           (double)(clock() - inicio) / CLOCKS_PER_SEC);


    liberarSolucao(&solucao);
    liberarInstancia(&inst);

    return EXIT_SUCCESS;
}
