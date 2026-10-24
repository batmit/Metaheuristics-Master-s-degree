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

/* Todas as instancias disponiveis sem otimo comprovado nas Tabelas 1 e 2
 * de Montemanni et al. (2024): os melhores limites LB e UB nao coincidem.
 * A coluna melhorLiteratura guarda o UB (melhor solucao viavel conhecida).
 * https://doi.org/10.1016/j.cor.2023.106514 */
static const CasoBenchmark casos[] = {
    {"20-c-c", "small", 43.0}, /* LB=40 */
    {"21-c-c", "small", 62.0}, /* LB=60 */
    {"26-r-c", "small", 103.0}, /* LB=101 */
    {"67-c-c", "large", 76.0}, /* LB=73 */
    {"91-r-c", "large", 124.0}, /* LB=120 */
    {"99-rc-c", "large", 100.0}, /* LB=98 */
    {"101-r-c", "large", 120.0}, /* LB=114 */
    {"103-rc-c", "large", 124.0}, /* LB=120 */
    {"105-rc-e", "large", 120.0}, /* LB=109 */
    {"108-rc-e", "large", 136.0}, /* LB=134 */
    {"114-rc-c", "large", 96.0}, /* LB=94 */
    {"121-rc-e", "large", 124.0}, /* LB=121 */
    {"126-r-c", "large", 160.0}, /* LB=151 */
    {"126-rc-e", "large", 144.0}, /* LB=136 */
    {"144-rc-c", "large", 128.0}, /* LB=122; rotulo 144-rc-r na Tabela 2 */
    {"154-c-c", "large", 72.0}, /* LB=68 */
    {"165-r-c", "large", 164.0}, /* LB=140 */
    {"167-r-e", "large", 188.0}, /* LB=160 */
    {"173-r-c", "large", 164.0}, /* LB=141 */
    {"173-rc-c", "large", 133.0}, /* LB=115; rotulo 173-rc-r na Tabela 2 */
    {"181-r-e", "large", 224.0}, /* LB=199 */
    {"187-rc-e", "large", 196.0}, /* LB=167 */
    {"200-r-e", "large", 212.0}, /* LB=162 */
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
    FILE *resumo = fopen("resultados/resumo_sem_otimo_100_execucoes.csv", "w");
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
        printf("Sem otimo comprovado na referencia de 2024. Cada execucao reconstroi a solucao inicial.\n");
        printf("Melhor literatura: melhor UB das Tabelas 1 e 2, Montemanni et al. (2024).\n");
        printf("Fonte: https://doi.org/10.1016/j.cor.2023.106514\n");
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
    printf("Literatura: melhores solucoes conhecidas, sem otimo comprovado nestas instancias.\n");
    return concluidasTotal == totalCasos * total && !erroResumo ? EXIT_SUCCESS : EXIT_FAILURE;
}
