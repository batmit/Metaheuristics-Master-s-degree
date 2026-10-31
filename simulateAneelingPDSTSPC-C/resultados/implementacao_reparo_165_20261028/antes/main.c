#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <limits.h>
#include "SA.h"
#include "main.h"
#include "lerInstancias.h"
#include "cv.h"

typedef struct CasoBenchmark {
    const char *nome;
    const char *pasta;
    double melhorLiteratura;
    bool movimentosGrupo; /* Habilita transferencias conjuntas neste caso. */
} CasoBenchmark;

/* Quatro instancias ainda abertas no pacote oficial ORLab de 2025.
 * Logs results/logs/tsp/: status FEASIBLE e LB < UB.
 * melhorLiteratura guarda o UB; a revisao dos 60 casos esta em
 * resultados/revisao_literatura_2025/relatorio_revisao_2024_2025.md.
 * https://orlab.com.vn/download/data-and-results-of-pdstsp-c-and-pdsop-c/ */
static const CasoBenchmark casos[] = {
    {"67-c-c", "large", 74.0, true}, /* LB=73 */
    {"165-r-c", "large", 160.0, true}, /* LB=156 */
    {"167-r-e", "large", 180.0, true}, /* LB=179 */
    {"181-r-e", "large", 216.0, true}, /* LB=215 */
};

static int testarInstancia(const CasoBenchmark *caso, double *resultado,const ParametrosSA *configuracao)

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
    ParametrosSA parametros = *configuracao;
    parametros.movimentosGrupo = caso->movimentosGrupo;
    EstatisticasSA estatisticas={0};parametros.estatisticas=&estatisticas;
    double temperaturaInicial;
    double inicio = relogioSA();
    etapa = "calibrar e executar simulated annealing";
    if(!resolverComCalibracaoSA(&inst,&solucao,&parametros,&temperaturaInicial)) goto finalizar;
    double segundos = relogioSA()-inicio;
    double melhoria = makespanInicial > 0.0 ?
        100.0 * (makespanInicial - solucao.makespan) / makespanInicial : 0.0;

    double gap = 100.0 * (solucao.makespan - caso->melhorLiteratura) / caso->melhorLiteratura;
    printf("%8d %6d %12.2f %12.2f %18.2f %+9.2f %10.2f %10.2f %10.3f OK\n",
           inst.n - 1, inst.numDrones, makespanInicial, solucao.makespan,
           caso->melhorLiteratura, gap, melhoria, temperaturaInicial, segundos);
    printf("  Avaliacoes=%llu; trocas=%llu; reconstrucoes=%llu; retomadas=%llu; atribuicoes na elite=%d\n",
           estatisticas.avaliacoes,estatisticas.trocas,estatisticas.reconstrucoes,
           estatisticas.retomadas,estatisticas.atribuicoesElite);
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

int main(int argc,char **argv)
{
    unsigned int semente = (unsigned int)time(NULL);
    size_t total = 10;bool controle=false;double limite=-1;
    const char *filtro=NULL;
    for(int i=1;i<argc;i++) {
        if(!strcmp(argv[i],"--controle")) {controle=true;continue;}
        if(i+1>=argc) goto uso;
        const char *opcao=argv[i],*valor=argv[++i];char *fim;errno=0;
        if(!strcmp(opcao,"--segundos")) {
            limite=strtod(valor,&fim);
            if(errno || !*valor || *fim || !isfinite(limite) || limite<0) goto uso;
        } else if(!strcmp(opcao,"--execucoes") || !strcmp(opcao,"--semente")) {
            unsigned long n=strtoul(valor,&fim,10);
            if(errno || !*valor || *fim || *valor=='-' || n>UINT_MAX) goto uso;
            if(!strcmp(opcao,"--semente")) semente=(unsigned int)n;
            else {if(!n) goto uso;total=(size_t)n;}
        } else if(!strcmp(opcao,"--instancia")) filtro=valor;
        else goto uso;
    }
    ParametrosSA configuracao=controle?parametrosSAPadrao():parametrosSAExploracao();
    if(limite>=0) configuracao.limiteSegundos=limite;
    const size_t totalCasos = sizeof casos / sizeof casos[0];
    if(filtro) {
        bool encontrado=false;
        for(size_t c=0;c<totalCasos;c++) if(!strcmp(filtro,casos[c].nome)) encontrado=true;
        if(!encontrado) goto uso;
    }
    size_t concluidasTotal = 0;
    FILE *resumo = fopen(controle?"resultados/resumo_quatro_abertas_controle.csv":
                       "resultados/resumo_quatro_abertas_exploracao.csv", "w");
    if (resumo == NULL) {
        perror("Nao foi possivel criar o resumo em resultados");
        return EXIT_FAILURE;
    }
    fprintf(resumo, "instancia,concluidas,melhor,media,melhor_literatura,gap_melhor_percentual,execucao_melhor,semente_melhor,abaixo_referencia,iguais_referencia,modo,limite_segundos\n");
    size_t casosSelecionados=filtro?1:totalCasos;
    printf("PDSTSP-C: %zu instancias; %zu execucoes por instancia; semente base %u.\n", casosSelecionados, total, semente);
    printf("Modo: %s; limite por execucao (calibracao + busca): %.3f s (zero usa resfriamento).\n",
           controle?"controle anterior":"propostas 1, 2, 6 e 7",configuracao.limiteSegundos);
    if(configuracao.limiteSegundos>0) {
        double previstos=casosSelecionados*(double)total*configuracao.limiteSegundos;
        printf("Duracao prevista do lote sequencial: %.1f minutos, mais inicializacao e fechamento.\n",previstos/60.0);
    }
    for (size_t c = 0; c < totalCasos; c++) {
        const CasoBenchmark *caso = &casos[c];
        if(filtro && strcmp(filtro,caso->nome)) continue;
        size_t concluidas = 0;
        double melhor = 0.0, soma = 0.0;
        size_t execucaoMelhor = 0, melhoresLiteratura = 0, iguaisLiteratura = 0;
        printf("\nInstancia %zu/%zu: %s; %zu execucoes independentes de SA.\n", c + 1, totalCasos, caso->nome, total);
        printf("Sem otimo comprovado no pacote ORLab de 2025. Cada execucao reconstroi a solucao inicial.\n");
        printf("Transferencias em grupos: %s.\n", caso->movimentosGrupo ? "ativas" : "desativadas");
        printf("Melhor literatura: UB dos logs PDSTSP-c do pacote oficial ORLab (2025).\n");
        printf("Fonte: https://orlab.com.vn/download/data-and-results-of-pdstsp-c-and-pdsop-c/\n");
        printf("Gap = 100 * (SA - literatura) / literatura. Tempo de parede inclui calibracao e SA.\n\n");
        printf("%-10s %8s %6s %12s %12s %18s %9s %10s %10s %10s %s\n",
               "Instancia", "Clientes", "Drones", "Inicial(min)", "SA(min)",
               "Melhor lit.(min)", "Gap(%)", "Ganho(%)", "T0", "Tempo(s)", "Status");
        for (size_t i = 0; i < total; i++) {
            unsigned int sementeExecucao = semente + (unsigned int)(c * total + i);
            double resultado;
            srand(sementeExecucao);
            printf("Execucao %zu/%zu; semente %u\n", i + 1, total, sementeExecucao);
            if (testarInstancia(caso, &resultado,&configuracao)) {
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
            fprintf(resumo, "%s,%zu,%.9f,%.9f,%.9f,%.9f,%zu,%u,%zu,%zu,%s,%.6f\n",
                    caso->nome, concluidas, melhor, soma / concluidas, caso->melhorLiteratura,
                    100.0 * (melhor - caso->melhorLiteratura) / caso->melhorLiteratura,
                    execucaoMelhor, semente + (unsigned int)(c * total + execucaoMelhor - 1),
                    melhoresLiteratura, iguaisLiteratura,controle?"controle":"exploracao",configuracao.limiteSegundos);
        } else {
            fprintf(resumo, "%s,0,,,,,,,,,%s,%.6f\n", caso->nome,controle?"controle":"exploracao",configuracao.limiteSegundos);
        }
        fflush(resumo);
        concluidasTotal += concluidas;
    }
    int erroResumo = ferror(resumo);
    if (fclose(resumo) != 0) erroResumo = 1;
    size_t esperadas=(filtro?1:totalCasos)*total;
    printf("\nTOTAL: %zu/%zu execucoes concluidas.\n", concluidasTotal, esperadas);
    printf("Literatura: melhores solucoes conhecidas; quatro casos sem otimo comprovado na fonte ORLab de 2025.\n");
    return concluidasTotal == esperadas && !erroResumo ? EXIT_SUCCESS : EXIT_FAILURE;
uso:
    fprintf(stderr,"Uso: %s [--controle] [--execucoes N] [--segundos S] [--semente N] [--instancia nome]\n",argv[0]);
    return EXIT_FAILURE;
}
