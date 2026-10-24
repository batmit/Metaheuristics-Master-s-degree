#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "cv.h"

/* Oraculo independente: enumera configuracoes e busca o primeiro inicio
 * em uma grade inteira, sempre seguindo a ordem recebida. */
static int ocupacao[64], duracoes[5], quantidades[5], otimo;
static void enumerar(int i, int n, int frota, int maximo)
{
    if (i == n) { if (maximo < otimo) otimo = maximo; return; }
    for (int t = 0; t + duracoes[i] < otimo; t++) {
        int cabe = 1;
        for (int h = t; h < t + duracoes[i]; h++)
            if (ocupacao[h] + quantidades[i] > frota) cabe = 0;
        if (!cabe) continue;
        for (int h = t; h < t + duracoes[i]; h++) ocupacao[h] += quantidades[i];
        int fim = t + duracoes[i];
        enumerar(i + 1, n, frota, fim > maximo ? fim : maximo);
        for (int h = t; h < t + duracoes[i]; h++) ocupacao[h] -= quantidades[i];
        break; /* A regra fixa nao permite escolher um inicio posterior. */
    }
}

static void enumerarConfiguracoes(int i, int n, const Instancia *inst)
{
    if (i == n) {
        memset(ocupacao, 0, sizeof ocupacao);
        enumerar(0, n, inst->numDrones, 0);
        return;
    }
    for (int k = inst->clientes[i + 1].minDrones; k <= inst->numDrones; k++) {
        double tempo = inst->tempoDrone[i + 1][k];
        if (!isfinite(tempo)) continue;
        quantidades[i] = k;
        duracoes[i] = (int)tempo;
        enumerarConfiguracoes(i + 1, n, inst);
    }
}

int main(void)
{
    Cliente clientes[6] = {0};
    double tempos[6][4]; double *linhas[6];
    for (int i = 0; i < 6; i++) {
        linhas[i] = tempos[i];
        for (int k = 0; k < 4; k++) tempos[i][k] = INFINITY;
        clientes[i].elegivelDrone = true; clientes[i].minDrones = 1;
    }
    Instancia inst = {.n = 6, .numDrones = 2, .clientes = clientes, .tempoDrone = linhas};
    MissaoDrone missoes[5];
    const int p[] = {3, 3, 2, 2, 2};
    for (int i = 0; i < 5; i++) {
        tempos[i + 1][1] = p[i];
        missoes[i] = (MissaoDrone){i + 1, 1, 99, 99};
    }
    Solucao s = {.missaoDrone = missoes, .numMissoesDrone = 5};
    assert(escalonarMissoesDrone(&inst, &s));
    assert(calcularMakespan(&inst, &s) && s.makespan == 7);
    assert(escalonarMissoesDroneBB(&inst, missoes, 5) == 7);
    for (int i = 0; i < 5; i++) assert(missoes[i].cliente == i + 1);
    assert(calcularMakespan(&inst, &s) && s.makespan == 7);

    srand(456);
    inst.numDrones = 3;
    for (int exemplo = 0; exemplo < 40; exemplo++) {
        int soma = 0;
        for (int i = 0; i < 4; i++) {
            for (int k = 1; k <= 3; k++) tempos[i + 1][k] = INFINITY;
            duracoes[i] = 1 + rand() % 4;
            quantidades[i] = 1 + rand() % 3;
            soma += duracoes[i];
            tempos[i + 1][quantidades[i]] = duracoes[i];
            missoes[i] = (MissaoDrone){i + 1, quantidades[i], 99, 99};
        }
        memset(ocupacao, 0, sizeof ocupacao);
        otimo = soma;
        enumerar(0, 4, 3, 0);
        assert(escalonarMissoesDroneBB(&inst, missoes, 4) == otimo);
        for (int i = 0; i < 4; i++) assert(missoes[i].cliente == i + 1);
        /* O quinto cliente permanece no caminhao, com tempos zero. */
        double tc[6][6] = {{0}}; double *caminhao[6];
        for (int i = 0; i < 6; i++) caminhao[i] = tc[i];
        inst.tempoCaminhao = caminhao;
        Missao restante = {5};
        s.missaoCaminhao = &restante; s.numMissoesCaminhao = 1;
        s.numMissoesDrone = 4;
        assert(calcularMakespan(&inst, &s) && s.makespan == otimo);
    }
    /* Falhas preservam o vetor, inclusive se um cliente posterior e invalido. */
    MissaoDrone copia[5];
    missoes[3].cliente = missoes[0].cliente;
    memcpy(copia, missoes, sizeof copia);
    assert(isinf(escalonarMissoesDroneBB(&inst, missoes, 4)));
    assert(memcmp(copia, missoes, sizeof copia) == 0);
    assert(isinf(escalonarMissoesDroneBB(NULL, missoes, 4)));
    assert(isinf(escalonarMissoesDroneBB(&inst, NULL, 1)));
    assert(isinf(escalonarMissoesDroneBB(&inst, missoes, -1)));
    assert(escalonarMissoesDroneBB(&inst, NULL, 0) == 0);
    missoes[0] = (MissaoDrone){1, 0, 99, 99};
    tempos[1][2] = tempos[1][3] = INFINITY;
    tempos[1][1] = 1.25;
    assert(escalonarMissoesDroneBB(&inst, missoes, 1) == 1.25);
    assert(missoes[0].qtdDrones == 1 && missoes[0].tempoInicio == 0);
    tempos[1][1] = 0;
    assert(escalonarMissoesDroneBB(&inst, missoes, 1) == 0);
    tempos[1][1] = INFINITY;
    assert(isinf(escalonarMissoesDroneBB(&inst, missoes, 1)));
    /* Quantidade recebida nao restringe a busca: e uma decisao de saida. */
    missoes[0].qtdDrones = 99;
    tempos[1][1] = 10; tempos[1][3] = 3;
    assert(escalonarMissoesDroneBB(&inst, missoes, 1) == 3);
    assert(missoes[0].qtdDrones == 3);
    /* Menos drones pode vencer a configuracao individual mais rapida. */
    inst.numDrones = 2;
    tempos[1][1] = tempos[2][1] = 6;
    tempos[1][2] = tempos[2][2] = 4;
    missoes[0] = (MissaoDrone){1, 2, 99, 99};
    missoes[1] = (MissaoDrone){2, 2, 99, 99};
    assert(escalonarMissoesDroneBB(&inst, missoes, 2) == 6);
    assert(missoes[0].qtdDrones == 1 && missoes[1].qtdDrones == 1);

    inst.numDrones = 3;
    for (int exemplo = 0; exemplo < 40; exemplo++) {
        for (int i = 1; i <= 3; i++) {
            clientes[i].minDrones = 1 + rand() % 2;
            for (int k = 1; k <= 3; k++)
                tempos[i][k] = k < clientes[i].minDrones || rand() % 4 == 0 ?
                    INFINITY : 1 + rand() % 4;
            tempos[i][3] = 1 + rand() % 4;
            missoes[i - 1] = (MissaoDrone){i, 0, 99, 99};
        }
        otimo = 13; /* Qualquer escolha serial tem duracao <= 12. */
        enumerarConfiguracoes(0, 3, &inst);
        double esperado = otimo;
        /* Mesmos problemas em escala fracionaria: arredondar LB para cima
         * nesses casos poderia podar o otimo incorretamente. */
        if (exemplo % 2) {
            esperado /= 4.0;
            for (int i = 1; i <= 3; i++)
                for (int k = 1; k <= 3; k++)
                    if (isfinite(tempos[i][k])) tempos[i][k] /= 4.0;
        }
        assert(escalonarMissoesDroneBB(&inst, missoes, 3) == esperado);
        for (int i = 0; i < 3; i++) assert(missoes[i].cliente == i + 1);
        for (int i = 0; i < 3; i++) {
            MissaoDrone *m = &missoes[i];
            assert(m->qtdDrones >= clientes[m->cliente].minDrones);
            assert(m->tempoFim - m->tempoInicio == tempos[m->cliente][m->qtdDrones]);
            int usados = 0;
            for (int j = 0; j < 3; j++)
                if (missoes[j].tempoInicio <= m->tempoInicio &&
                    m->tempoInicio < missoes[j].tempoFim) usados += missoes[j].qtdDrones;
            assert(usados <= 3);
        }
    }
    return 0;
}
