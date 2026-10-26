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

/* Quatro instancias ainda abertas no pacote oficial ORLab de 2025.
 * Logs results/logs/tsp/: status FEASIBLE e LB < UB.
 * melhorLiteratura guarda o UB; a revisao dos 60 casos esta em
 * resultados/revisao_literatura_2025/relatorio_revisao_2024_2025.md.
 * https://orlab.com.vn/download/data-and-results-of-pdstsp-c-and-pdsop-c/ */
static const CasoBenchmark casos[] = {
    {"67-c-c", "large", 74.0}, /* LB=73 */
    {"165-r-c", "large", 160.0}, /* LB=156 */
    {"167-r-e", "large", 180.0}, /* LB=179 */
    {"181-r-e", "large", 216.0}, /* LB=215 */
};

static int testarInstancia(const CasoBenchmark *caso, double *resultado)

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
    *resultado = solucao.makespan;
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
    const unsigned int semente = (unsigned int)time(NULL);
    const size_t total = 100;
    const size_t totalCasos = sizeof casos / sizeof casos[0];
    size_t concluidasTotal = 0;
    FILE *resumo = fopen("resultados/resumo_quatro_abertas_2025_100_execucoes.csv", "w");
    if (resumo == NULL) {
        perror("Nao foi possivel criar o resumo em resultados");
        return EXIT_FAILURE;
    }
    fprintf(resumo, "instancia,concluidas,melhor,media,melhor_literatura,gap_melhor_percentual,execucao_melhor,semente_melhor,abaixo_referencia,iguais_referencia\n");
    printf("PDSTSP-C: %zu instancias; %zu execucoes por instancia; semente base %u.\n", totalCasos, total, semente);
    for (size_t c = 0; c < totalCasos; c++) {
        const CasoBenchmark *caso = &casos[c];
        size_t concluidas = 0;
        double melhor = 0.0, soma = 0.0;
        size_t execucaoMelhor = 0, melhoresLiteratura = 0, iguaisLiteratura = 0;
        printf("\nInstancia %zu/%zu: %s; %zu execucoes independentes de SA.\n", c + 1, totalCasos, caso->nome, total);
        printf("Sem otimo comprovado no pacote ORLab de 2025. Cada execucao reconstroi a solucao inicial.\n");
        printf("Melhor literatura: UB dos logs PDSTSP-c do pacote oficial ORLab (2025).\n");
        printf("Fonte: https://orlab.com.vn/download/data-and-results-of-pdstsp-c-and-pdsop-c/\n");
        printf("Gap = 100 * (SA - literatura) / literatura. CPU inclui calibracao e SA.\n\n");
        printf("%-10s %8s %6s %12s %12s %18s %9s %10s %10s %10s %s\n",
               "Instancia", "Clientes", "Drones", "Inicial(min)", "SA(min)",
               "Melhor lit.(min)", "Gap(%)", "Ganho(%)", "T0", "CPU(s)", "Status");
        for (size_t i = 0; i < total; i++) {
            unsigned int sementeExecucao = semente + (unsigned int)(c * total + i);
            double resultado;
            srand(sementeExecucao);
            printf("Execucao %zu/%zu; semente %u\n", i + 1, total, sementeExecucao);
            if (testarInstancia(caso, &resultado)) {
                if (concluidas == 0 || resultado < melhor) {
                    melhor = resultado;
                    execucaoMelhor = i + 1;
                }
                soma += resultado;
                concluidas++;
                if (resultado < caso->melhorLiteratura - 1e-9) melhoresLiteratura++;
                else if (resultado <= caso->melhorLiteratura + 1e-9) iguaisLiteratura++;
            }
        }
        printf("\nConcluidas: %zu/%zu; falhas: %zu.\n", concluidas, total, total - concluidas);
        if (concluidas > 0) {
            printf("Melhor: %.2f min (execucao %zu); media: %.2f min; literatura: %.2f min; gap do melhor: %+.2f%%.\n",
                   melhor, execucaoMelhor, soma / concluidas, caso->melhorLiteratura,
                   100.0 * (melhor - caso->melhorLiteratura) / caso->melhorLiteratura);
            printf("Resultados abaixo da referencia: %zu; iguais: %zu.\n", melhoresLiteratura, iguaisLiteratura);
            fprintf(resumo, "%s,%zu,%.9f,%.9f,%.9f,%.9f,%zu,%u,%zu,%zu\n",
                    caso->nome, concluidas, melhor, soma / concluidas, caso->melhorLiteratura,
                    100.0 * (melhor - caso->melhorLiteratura) / caso->melhorLiteratura,
                    execucaoMelhor, semente + (unsigned int)(c * total + execucaoMelhor - 1),
                    melhoresLiteratura, iguaisLiteratura);
        } else {
            fprintf(resumo, "%s,0,,,,,,,,\n", caso->nome);
        }
        fflush(resumo);
        concluidasTotal += concluidas;
    }
    int erroResumo = ferror(resumo);
    if (fclose(resumo) != 0) erroResumo = 1;
    printf("\nTOTAL: %zu/%zu execucoes concluidas.\n", concluidasTotal, totalCasos * total);
    printf("Literatura: melhores solucoes conhecidas; quatro casos sem otimo comprovado na fonte ORLab de 2025.\n");
    return concluidasTotal == totalCasos * total && !erroResumo ? EXIT_SUCCESS : EXIT_FAILURE;
}
