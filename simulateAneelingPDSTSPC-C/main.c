#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "SA.h"
#include "main.h"
#include "lerInstancias.h"
#include "cv.h"

typedef struct CasoBenchmark {
    const char *nome;
    const char *pasta;
    double melhorLiteratura;
} CasoBenchmark;

/* Pequenas: ate 30 clientes; medias: 31 a 100 (convencao deste lote).
 * Melhor UB, nao LB, das Tabelas 1 e 2, Montemanni et al. (2024).
 * https://doi.org/10.1016/j.cor.2023.106514
 * Referencias fixas da publicacao; nao pressupoe otimalidade ou atualizacao online. */
static const CasoBenchmark casos[] = {
    {"15-r-e", "small", 92.0},
    {"15-rc-c", "small", 44.0},
    {"16-c-c", "small", 60.0},
    {"16-r-e", "small", 112.0},
    {"18-c-c", "small", 56.0},
    {"18-r-e", "small", 96.0},
    {"18-rc-c", "small", 57.0},
    {"19-c-c", "small", 44.0},
    {"20-c-c", "small", 43.0},
    {"20-r-c", "small", 64.0},
    {"20-r-e", "small", 80.0},
    {"20-rc-c", "small", 96.0},
    {"20-rc-e", "small", 100.0},
    {"21-c-c", "small", 62.0},
    {"21-r-e", "small", 85.0},
    {"23-c-e", "small", 80.0},
    {"23-r-c", "small", 88.0},
    {"24-c-e", "small", 84.0},
    {"24-r-e", "small", 112.0},
    {"24-rc-c", "small", 70.0},
    {"25-c-c", "small", 56.0},
    {"25-r-e", "small", 104.0},
    {"25-rc-e", "small", 92.0},
    {"26-r-c", "small", 103.0},
    {"27-c-c", "small", 84.0},
    {"27-c-e", "small", 68.0},
    {"27-rc-c", "small", 100.0},
    {"27-rc-e", "small", 84.0},
    {"29-rc-e", "small", 116.0},
    {"30-c-c", "small", 96.0},
    {"50-r-e", "large", 116.0},
    {"53-r-e", "large", 132.0},
    {"66-rc-e", "large", 124.0},
    {"67-c-c", "large", 76.0},
    {"68-rc-c", "large", 76.0},
    {"76-c-c", "large", 52.0},
    {"82-c-e", "large", 64.0},
    {"82-rc-c", "large", 104.0},
    {"88-c-e", "large", 108.0},
    {"91-r-c", "large", 124.0},
    {"99-rc-c", "large", 100.0},
};

static int testarInstancia(const CasoBenchmark *caso)

{
    const char *nome = caso->nome;
    Instancia inst = {0};
    Solucao solucao = {0};
    char caminhoInstancia[256], caminhoTempos[256];
    const char *etapa = "carregar instancia";
    int sucesso = 0;
    snprintf(caminhoInstancia, sizeof caminhoInstancia,
             "instances/pdstsp-c/%s/%s.txt", caso->pasta, nome);
    snprintf(caminhoTempos, sizeof caminhoTempos,
             "instances/pdstsp-c/%s/tau_kj_%s.txt", caso->pasta, nome);

    printf("%-10s ", nome);
    fflush(stdout);
    if (!lerInstancia(caminhoInstancia, &inst)) goto finalizar;

    etapa = "alocar matrizes";
    alocarInstancias(&inst);
    if (inst.tempoCaminhao == NULL || inst.tempoDrone == NULL) goto finalizar;
    for (int i = 0; i < inst.n; i++)
        if (inst.tempoCaminhao[i] == NULL || inst.tempoDrone[i] == NULL)
            goto finalizar;
    construirMatrizCaminhao(&inst);

    etapa = "carregar tempos dos drones";
    if (!lerTemposDrone(caminhoTempos, &inst)) goto finalizar;
    etapa = "alocar solucao";
    alocarSolucao(&inst, &solucao);
    if (solucao.missaoCaminhao == NULL || solucao.missaoDrone == NULL ||
        solucao.prefixosDrone == NULL) goto finalizar;
    etapa = "construir solucao inicial";
    if (!construirSolucaoInicial(&inst, &solucao)) goto finalizar;

    double makespanInicial = solucao.makespan;
    ParametrosSA parametros = parametrosSAPadrao();
    double temperaturaInicial;
    clock_t inicio = clock();
    etapa = "calibrar temperatura";
    if (!calculaTemperaturaInicial(&inst, &solucao, &parametros, &temperaturaInicial))
        goto finalizar;
    parametros.tempInicial = temperaturaInicial;
    etapa = "executar simulated annealing";
    if (!simulatedAnnealing(&inst, &solucao, &parametros)) goto finalizar;
    double segundos = (double)(clock() - inicio) / CLOCKS_PER_SEC;
    double melhoria = makespanInicial > 0.0 ?
        100.0 * (makespanInicial - solucao.makespan) / makespanInicial : 0.0;

    double gap = 100.0 * (solucao.makespan - caso->melhorLiteratura) / caso->melhorLiteratura;
    printf("%8d %6d %12.2f %12.2f %18.2f %+9.2f %10.2f %10.2f %10.3f OK\n",
           inst.n - 1, inst.numDrones, makespanInicial, solucao.makespan,
           caso->melhorLiteratura, gap, melhoria, temperaturaInicial, segundos);
    sucesso = 1;

finalizar:
    if (!sucesso) {
        printf("FALHA\n");
        fprintf(stderr, "Instancia %s: falha ao %s.\n", nome, etapa);
    }
    liberarSolucao(&solucao);
    liberarInstancia(&inst);
    return sucesso;
}

int main(void)
{
    const unsigned int semente = 123;
    const size_t total = sizeof casos / sizeof casos[0];
    size_t concluidas = 0;
    srand(semente);
    printf("PDSTSP-C: %zu instancias; uma execucao de SA por instancia; semente %u.\n", total, semente);
    printf("Pequenas: ate 30 clientes. Medias: 31 a 100 clientes.\n");
    printf("Melhor literatura: melhor UB das Tabelas 1 e 2, Montemanni et al. (2024).\n");
    printf("Fonte: https://doi.org/10.1016/j.cor.2023.106514\n");
    printf("Gap = 100 * (SA - literatura) / literatura. CPU inclui calibracao e SA.\n\n");
    printf("%-10s %8s %6s %12s %12s %18s %9s %10s %10s %10s %s\n",
           "Instancia", "Clientes", "Drones", "Inicial(min)", "SA(min)",
           "Melhor lit.(min)", "Gap(%)", "Ganho(%)", "T0", "CPU(s)", "Status");
    for (size_t i = 0; i < total; i++)
        concluidas += testarInstancia(&casos[i]);
    printf("\nConcluidas: %zu/%zu; falhas: %zu.\n", concluidas, total, total - concluidas);
    printf("Literatura: melhores solucoes conhecidas na referencia, nem todas otimas provadas.\n");
    return concluidas == total ? EXIT_SUCCESS : EXIT_FAILURE;
}
