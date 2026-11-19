/* Confere certificados congelados em JSON (convertidos para stdin pelo Python)
 * com o avaliador C, sem iniciar SA nem tocar no registro central. */
#include "../lerInstancias.h"
#include "../SA.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* O ultimo argumento opcional constroi e calibra a solucao inicial para diagnostico. */
int main(int argc, char **argv)
{
    if (argc != 4 && argc != 5) return 1;
    Instancia inst = {0};
    Solucao s = {0};
    int ok = 0;
    if (!carregarInstancia(argv[1], argv[2], atoi(argv[3]), &inst) || !alocarSolucao(&inst, &s)) goto fim;
    if (argc == 5 && strcmp(argv[4], "initial") == 0) {
        srand(1307);
        if (!construirSolucaoInicial(&inst, &s)) goto fim;
        ParametrosSA p = parametrosSAPadrao();
        double temp;
        if (!calculaTemperaturaInicial(&inst, &s, p.beta, p.gamma, p.saMax, &temp)) goto fim;
        printf("{\"initial\":%.17g,\"temperature\":%.17g,\"routes\":[", s.makespan, temp);
        for (int c = 0; c < s.numCaminhoes; ++c) {
            if (c) putchar(',');
            printf("%.17g", s.rotasCaminhao[c].tempo);
        }
        puts("]}");
        ok = 1;
        goto fim;
    }
    for (int c = 0; c < s.numCaminhoes; ++c) {
        RotaCaminhao *r = &s.rotasCaminhao[c];
        if (scanf("%d", &r->tamanho) != 1 || r->tamanho < 0 || r->tamanho > r->capacidade) goto fim;
        for (int i = 0; i < r->tamanho; ++i) if (scanf("%d", &r->rota[i]) != 1) goto fim;
    }
    if (scanf("%d", &s.numMissoesDrone) != 1 || s.numMissoesDrone < 0 || s.numMissoesDrone >= inst.n) goto fim;
    for (int i = 0; i < s.numMissoesDrone; ++i) {
        MissaoDrone *m = &s.missaoDrone[i];
        if (scanf("%d %d %lf %lf", &m->cliente, &m->qtdDrones, &m->tempoInicio, &m->tempoFim) != 4) goto fim;
    }
    double claimed;
    if (scanf("%lf", &claimed) != 1) goto fim;
    s.cronogramaValido = true;
    if (!calcularMakespan(&inst, &s) || !isfinite(claimed) || fabs(s.makespan-claimed)>1e-7) goto fim;
    printf("%.17g\n", s.makespan);
    ok = 1;
fim:
    liberarSolucao(&s);
    liberarInstancia(&inst);
    return ok ? 0 : 1;
}
