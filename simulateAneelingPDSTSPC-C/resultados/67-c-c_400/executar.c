#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "SA.h"
#include "lerInstancias.h"

/* Executor experimental; usa SA.c/cv.c originais, sem instrumentar a busca. */
int main(int argc, char **argv)
{
    if (argc != 6) return 2;
    unsigned int base = (unsigned int)strtoul(argv[1], NULL, 10);
    int primeiro = atoi(argv[2]), quantidade = atoi(argv[3]);
    FILE *registros = fopen(argv[4], "w");
    FILE *log = fopen(argv[5], "w");
    if (!registros || !log) return 3;
    for (int i = primeiro; i < primeiro + quantidade; i++) {
        Instancia inst = {0};
        Solucao s = {0};
        unsigned int seed = base + (unsigned int)i;
        srand(seed);
        if (!lerInstancia("instances/pdstsp-c/large/67-c-c.txt", &inst)) return 4;
        alocarInstancias(&inst);
        if (!inst.tempoCaminhao || !inst.tempoDrone) return 5;
        for (int j = 0; j < inst.n; j++)
            if (!inst.tempoCaminhao[j] || !inst.tempoDrone[j]) return 5;
        construirMatrizCaminhao(&inst);
        if (!lerTemposDrone("instances/pdstsp-c/large/tau_kj_67-c-c.txt", &inst)) return 6;
        alocarSolucao(&inst, &s);
        if (!s.missaoCaminhao || !s.missaoDrone || !s.prefixosDrone) return 7;
        if (!construirSolucaoInicial(&inst, &s)) return 8;
        double inicial = s.makespan, temperatura;
        ParametrosSA p = parametrosSAPadrao();
        clock_t inicio = clock();
        if (!calculaTemperaturaInicial(&inst, &s, &p, &temperatura)) return 9;
        p.tempInicial = temperatura;
        if (!simulatedAnnealing(&inst, &s, &p)) return 10;
        double segundos = (double)(clock() - inicio) / CLOCKS_PER_SEC;
        fprintf(registros, "{\"run\":%d,\"seed\":%u,\"initial\":%.17g,\"temperature\":%.17g,\"seconds\":%.6f,\"claimed\":%.17g,\"truck\":%.17g,\"drone\":%.17g,\"route\":[",
                i + 1, seed, inicial, temperatura, segundos, s.makespan, s.tempoCaminhao, s.tempoDrones);
        for (int j = 0; j < s.numMissoesCaminhao; j++)
            fprintf(registros, "%s%d", j ? "," : "", s.missaoCaminhao[j].n);
        fprintf(registros, "],\"missions\":[");
        for (int j = 0; j < s.numMissoesDrone; j++) {
            MissaoDrone *m = &s.missaoDrone[j];
            fprintf(registros, "%s{\"client\":%d,\"k\":%d,\"start\":%.17g,\"end\":%.17g}",
                    j ? "," : "", m->cliente, m->qtdDrones, m->tempoInicio, m->tempoFim);
        }
        fprintf(registros, "]}\n");
        fprintf(log, "Execucao %d/400; semente %u; makespan %.9f; caminhao %.9f; drones %.9f; segundos %.3f\n",
                i + 1, seed, s.makespan, s.tempoCaminhao, s.tempoDrones, segundos);
        fflush(registros);
        fflush(log);
        if (ferror(registros) || ferror(log)) return 11;
        liberarSolucao(&s);
        liberarInstancia(&inst);
    }
    int erro = fclose(registros) != 0;
    erro |= fclose(log) != 0;
    return erro ? 12 : 0;
}
