#include "lerInstancias.h"
#include "SA.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void registrarMelhor(const Solucao *s, void *contexto)
{
    (void)contexto;
    /* Uma linha completa por incumbente: o executor recupera e valida mesmo
     * quando um processo e encerrado dentro do BB pelo prazo externo. */
    printf("CHECKPOINT {\"makespan\":%.17g,\"routes\":[", s->makespan);
    for (int c = 0; c < s->numCaminhoes; ++c) {
        if (c) putchar(',');
        putchar('[');
        for (int p = 0; p < s->rotasCaminhao[c].tamanho; ++p) {
            if (p) putchar(',');
            printf("%d", s->rotasCaminhao[c].rota[p]);
        }
        putchar(']');
    }
    printf("],\"missions\":[");
    for (int i = 0; i < s->numMissoesDrone; ++i) {
        const MissaoDrone *m = &s->missaoDrone[i];
        if (i) putchar(',');
        printf("{\"client\":%d,\"k\":%d,\"start\":%.17g,\"end\":%.17g}",
               m->cliente, m->qtdDrones, m->tempoInicio, m->tempoFim);
    }
    puts("]}");
    fflush(stdout);
}

static int referencia(const char *arquivo, int frota, double *lb, double *ub, int *provado)
{
    const char *nome = arquivo;
    for (const char *p = arquivo; *p; ++p) if (*p == '/' || *p == '\\') nome = p + 1;
    char stem[80];
    if (strlen(nome) >= sizeof stem) return 0;
    strcpy(stem, nome);
    char *extensao = strrchr(stem, '.');
    if (extensao) *extensao = '\0';
    FILE *f = fopen("instances/literature_pdsvrp_c.csv", "r");
    if (!f) return 0;
    char linha[256], id[80];
    int s, p, achou = 0;
    double l, u;
    while (fgets(linha, sizeof linha, f))
        if (sscanf(linha, "%79[^,],%d,%lf,%lf,%d", id, &s, &l, &u, &p) == 5 &&
            strcmp(id, stem) == 0 && s == frota) {
            *lb = l; *ub = u; *provado = p; achou = 1; break;
        }
    fclose(f);
    return achou;
}

static int executarLote(void)
{
#ifdef _WIN32
    if (system("python -c \"import sys\" >NUL 2>&1") == 0)
        return system("python scripts/executar_benchmark.py") == 0 ? 0 : 1;
    if (system("py -3 -c \"import sys\" >NUL 2>&1") == 0)
        return system("py -3 scripts/executar_benchmark.py") == 0 ? 0 : 1;
#else
    if (system("python3 -c 'import sys' >/dev/null 2>&1") == 0)
        return system("python3 scripts/executar_benchmark.py") == 0 ? 0 : 1;
    if (system("python -c 'import sys' >/dev/null 2>&1") == 0)
        return system("python scripts/executar_benchmark.py") == 0 ? 0 : 1;
#endif
    fprintf(stderr, "Python 3 nao encontrado. Instale Python 3 ou execute o lote com seu interpretador.\n");
    return 1;
}

int main(int argc, char **argv)
{
    if (argc == 1) return executarLote();
    if (argc != 4 && argc != 5) {
        fprintf(stderr, "Uso: %s instancia.txt tau_kj.txt numCaminhoes [semente]\n", argv[0]);
        return 1;
    }
    char *fim;
    errno = 0;
    long frota = strtol(argv[3], &fim, 10);
    if (errno || !*argv[3] || *fim || frota < 1 || frota > INT_MAX) return 1;
    double lb, ub;
    int provado;
    if (!referencia(argv[1], (int)frota, &lb, &ub, &provado)) {
        fprintf(stderr, "Configuracao sem referencia verificada; nao executada.\n");
        return 1;
    }
    printf("Literatura: LB=%.6f UB=%.6f otimo_comprovado=%d\n", lb, ub, provado);
    if (provado) {
        puts("IGNORADA: otimo comprovado na literatura; SA nao executado.");
        return 0;
    }
    unsigned seed = (unsigned)time(NULL);
    if (argc == 5) {
        errno = 0;
        unsigned long valor = strtoul(argv[4], &fim, 10);
        if (errno || !*argv[4] || *fim || *argv[4] == '-' || valor > UINT_MAX) return 1;
        seed = (unsigned)valor;
    }
    srand(seed);
    Instancia inst = {0};
    Solucao solucao = {0};
    int status = 1;
    ParametrosSA parametros = parametrosSAPadrao();
    EstatisticasSA estatisticas = {0};
    parametros.estatisticas = &estatisticas;
    parametros.registrarMelhor = registrarMelhor;
    if (!carregarInstancia(argv[1], argv[2], (int)frota, &inst) ||
        !alocarSolucao(&inst, &solucao) || !construirSolucaoInicial(&inst, &solucao)) goto fim;
    printf("Semente: %u\nMakespan inicial: %.6f\n", seed, solucao.makespan);
    registrarMelhor(&solucao, NULL);
    if (!calculaTemperaturaInicial(&inst, &solucao, parametros.beta, parametros.gamma,
                                  parametros.saMax, &parametros.tempInicial)) goto fim;
    printf("Temperatura inicial: %.6f\n", parametros.tempInicial);
    if (!simulatedAnnealing(&inst, &solucao, &parametros) ||
        !calcularMakespan(&inst, &solucao)) goto fim;
    printf("Makespan final: %.6f\n", solucao.makespan);
    printf("ESTATISTICAS tentativas=%llu niveis=%llu chamadasBB=%llu nosBB=%llu melhoriasBB=%llu bbLimitados=%llu\n",
           estatisticas.tentativas, estatisticas.niveis, estatisticas.chamadasBB,
           estatisticas.nosBB, estatisticas.melhoriasBB, estatisticas.bbLimitados);
    for (int c = 0; c < solucao.numCaminhoes; ++c) {
        const RotaCaminhao *r = &solucao.rotasCaminhao[c];
        printf("Caminhao %d (%.6f): 0", c + 1, r->tempo);
        for (int p = 0; p < r->tamanho; ++p) printf(" %d", inst.clientes[r->rota[p]].id);
        printf(" 0\n");
    }
    for (int i = 0; i < solucao.numMissoesDrone; ++i) {
        const MissaoDrone *m = &solucao.missaoDrone[i];
        printf("Drone: cliente=%d quantidade=%d inicio=%.6f fim=%.6f\n",
               inst.clientes[m->cliente].id, m->qtdDrones, m->tempoInicio, m->tempoFim);
    }
    status = 0;
fim:
    if (status) fprintf(stderr, "Falha na carga, construcao, calibracao ou busca.\n");
    liberarSolucao(&solucao);
    liberarInstancia(&inst);
    return status;
}
