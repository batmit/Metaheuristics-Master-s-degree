/* Lancador publico e worker interno do SA; certificados ficam no canal capturado pelo Python. */
#include "lerInstancias.h"
#include "SA.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

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

/* O executor Python concentra catalogo, tabela e persistencia. O processo C
 * interno recebe PDSVRP_WORKER e envia apenas certificados pelo stdout. */
static int executarPython(int argc, char **argv)
{
#ifdef _WIN32
    const char *interpretes[] = {"python", "python3", "py"};
#else
    const char *interpretes[] = {"python3", "python", "py"};
#endif
    char **argumentos = calloc((size_t)argc + 4, sizeof *argumentos);
    if (!argumentos) return 1;
    for (int i = 0; i < 3; ++i) {
        int n = 0;
        argumentos[n++] = (char *)interpretes[i];
        if (i == 2) argumentos[n++] = "-3";
        argumentos[n++] = "scripts/executar_benchmark.py";
        for (int j = 1; j < argc; ++j) argumentos[n++] = argv[j];
        argumentos[n] = NULL;
#ifdef _WIN32
        intptr_t status = _spawnvp(_P_WAIT, interpretes[i], (const char *const *)argumentos);
        if (status != -1) { free(argumentos); return status == 0 ? 0 : 1; }
#else
        pid_t pid = fork();
        if (pid == 0) { execvp(interpretes[i], argumentos); _exit(127); }
        if (pid < 0) { free(argumentos); return 1; }
        int status;
        if (waitpid(pid, &status, 0) < 0) { free(argumentos); return 1; }
        if (WIFEXITED(status) && WEXITSTATUS(status) != 127) {
            free(argumentos); return WEXITSTATUS(status);
        }
#endif
    }
    free(argumentos);
    fprintf(stderr, "Python 3 nao encontrado.\n");
    return 1;
}

/* Entrada publica chama o executor; entrada interna executa uma unica configuracao. */
int main(int argc, char **argv)
{
    if (!getenv("PDSVRP_WORKER")) return executarPython(argc, argv);
    if (argc != 4 && argc != 5) {
        fprintf(stderr, "Uso: %s instancia.txt tau_kj.txt numCaminhoes [semente]\n", argv[0]);
        return 1;
    }
    char *fim;
    errno = 0;
    long frota = strtol(argv[3], &fim, 10);
    if (errno || !*argv[3] || *fim || frota < 1 || frota > INT_MAX) return 1;
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
    parametros.registrarMelhor = registrarMelhor;
    if (!carregarInstancia(argv[1], argv[2], (int)frota, &inst) ||
        !alocarSolucao(&inst, &solucao) || !construirSolucaoInicial(&inst, &solucao)) goto fim;
    /* A construcao inicial ja foi integralmente validada em C. */
    registrarMelhor(&solucao, NULL);
    if (!calculaTemperaturaInicial(&inst, &solucao, parametros.beta, parametros.gamma,
                                  parametros.saMax, &parametros.tempInicial)) goto fim;
    if (!simulatedAnnealing(&inst, &solucao, &parametros) ||
        !calcularMakespan(&inst, &solucao)) goto fim;
    status = 0;
fim:
    if (status) fprintf(stderr, "Falha na carga, construcao, calibracao ou busca.\n");
    liberarSolucao(&solucao);
    liberarInstancia(&inst);
    return status;
}
