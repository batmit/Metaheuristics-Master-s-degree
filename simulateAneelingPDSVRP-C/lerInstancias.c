/* Leitura e liberacao dos dados oficiais de clientes e tempos de voo. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "lerInstancias.h"
#include "cv.h"
#define TAM_LINHA 256
#define CAPACIDADE_INICIAL 16

/* Cada linha da matriz possui alocacao propria. */
static void liberarMatriz(double **matriz, int linhas)
{
    if (!matriz) return;
    for (int i = 0; i < linhas; ++i) free(matriz[i]);
    free(matriz);
}

/* Libera todos os dados de entrada e restabelece o estado vazio. */
void liberarInstancia(Instancia *inst)
{
    if (inst == NULL)
        return;

    free(inst->clientes);

    liberarMatriz(inst->tempoCaminhao, inst->n);

    liberarMatriz(inst->tempoDrone, inst->n);

    *inst = (Instancia){0};
}

/* Le parametros e clientes; exige IDs consecutivos e dados fisicos finitos. */
int lerInstancia(const char *nomeArquivo, Instancia *inst)
{
    if (nomeArquivo == NULL || inst == NULL)
        return 0;

    *inst = (Instancia){0};

    FILE *arquivo = fopen(nomeArquivo, "r");

    if (arquivo == NULL)
    {
        fprintf(stderr, "Erro ao abrir o arquivo: %s\n", nomeArquivo);
        return 0;
    }

    char linha[TAM_LINHA];

    /* Mesma regra de leitura para os seis parametros do cabecalho. */
    const char *formatos[] = {
        "TRUCK_SPEED, %lf", "BATTERY_WEIGHT, %lf", "BATTERY_CAPACITY, %lf",
        "MAX_DISCHARGE_CURRENT, %lf", "VOLTAGE, %lf"
    };
    double *parametros[] = {
        &inst->truckSpeed, &inst->batteryWeight, &inst->batteryCapacity,
        &inst->maxDischargeCurrent, &inst->voltage
    };
    if (!fgets(linha, sizeof(linha), arquivo) ||
        sscanf(linha, "NUM_DRONES, %d", &inst->numDrones) != 1) {
        fprintf(stderr, "Erro ao ler NUM_DRONES.\n");
        fclose(arquivo);
        return 0;
    }
    for (size_t i = 0; i < sizeof(parametros) / sizeof(*parametros); ++i) {
        if (!fgets(linha, sizeof(linha), arquivo) ||
            sscanf(linha, formatos[i], parametros[i]) != 1) {
            fprintf(stderr, "Erro ao ler parametro: %s\n", formatos[i]);
            fclose(arquivo);
            return 0;
        }
    }

    /* O cabecalho deve identificar as quatro colunas dos clientes. */
    if (fgets(linha, sizeof(linha), arquivo) == NULL)
    {
        fprintf(stderr, "Erro: cabecalho dos clientes nao encontrado.\n");
        fclose(arquivo);
        return 0;
    }

    char colunaNode[32];
    char colunaX[32];
    char colunaY[32];
    char colunaPeso[32];

    if (sscanf(linha, "%31s %31s %31s %31s",
               colunaNode, colunaX, colunaY, colunaPeso) != 4 ||
        strcmp(colunaNode, "node") != 0 ||
        strcmp(colunaX, "x") != 0 ||
        strcmp(colunaY, "y") != 0 ||
        strcmp(colunaPeso, "weight") != 0)
    {
        fprintf(stderr, "Erro: cabecalho esperado 'node x y weight'.\n");
        fclose(arquivo);
        return 0;
    }

    /* O numero de clientes e descoberto durante a leitura. */
    int capacidade = CAPACIDADE_INICIAL;

    inst->clientes = malloc((size_t)capacidade * sizeof(Cliente));

    if (inst->clientes == NULL)
    {
        fprintf(stderr, "Erro ao alocar memoria para os clientes.\n");
        fclose(arquivo);
        return 0;
    }

    while (fgets(linha, sizeof(linha), arquivo) != NULL)
    {
        /* Ignora linhas vazias. */
        if (linha[0] == '\n' || linha[0] == '\r' || linha[0] == '\0')
            continue;

        Cliente cliente = {0};

        if (sscanf(linha, "%d %lf %lf %lf",
                   &cliente.id,
                   &cliente.x,
                   &cliente.y,
                   &cliente.peso) != 4)
        {
            fprintf(stderr, "Erro ao ler linha de cliente: %s", linha);
            fclose(arquivo);
            liberarInstancia(inst);
            return 0;
        }

        if (cliente.id != inst->n || !isfinite(cliente.x) ||
            !isfinite(cliente.y) || !isfinite(cliente.peso) || cliente.peso < 0)
        {
            fprintf(stderr, "Erro: IDs devem ser sequenciais desde 0 e dados finitos.\n");
            fclose(arquivo);
            liberarInstancia(inst);
            return 0;
        }

        if (inst->n == capacidade)
        {
            capacidade *= 2;

            Cliente *novoVetor = realloc(
                inst->clientes,
                (size_t)capacidade * sizeof(Cliente)
            );

            if (novoVetor == NULL)
            {
                fprintf(stderr, "Erro ao realocar memoria para os clientes.\n");
                fclose(arquivo);
                liberarInstancia(inst);
                return 0;
            }

            inst->clientes = novoVetor;
        }

        inst->clientes[inst->n] = cliente;
        inst->n++;
    }

    int erroLeitura = ferror(arquivo);
    fclose(arquivo);

    if (erroLeitura || inst->n == 0 || inst->numDrones <= 0 ||
        !isfinite(inst->truckSpeed) || inst->truckSpeed <= 0 ||
        !isfinite(inst->batteryWeight) || inst->batteryWeight < 0 ||
        !isfinite(inst->batteryCapacity) || inst->batteryCapacity <= 0 ||
        !isfinite(inst->maxDischargeCurrent) || inst->maxDischargeCurrent <= 0 ||
        !isfinite(inst->voltage) || inst->voltage <= 0)
    {
        fprintf(stderr, "Erro: nenhum no foi encontrado na instancia.\n");
        liberarInstancia(inst);
        return 0;
    }

    /* Reduz o vetor para o tamanho exato. Falhar aqui nao e fatal,
       pois o vetor atual continua valido. */
    Cliente *vetorExato = realloc(
        inst->clientes,
        (size_t)inst->n * sizeof(Cliente)
    );

    if (vetorExato != NULL)
        inst->clientes = vetorExato;

    return 1;
}

/* Le a tabela oficial, converte -1 em infinito e deriva elegibilidade de clientes. */
int lerTemposDrone(const char *nomeArquivo, Instancia *inst)
{
    if (!nomeArquivo || !inst || !inst->clientes || inst->n < 1 ||
        !alocarTempoDrone(inst)) return 0;

    FILE *arquivo = fopen(nomeArquivo, "r");

    if (arquivo == NULL)
    {
        fprintf(stderr, "Erro ao abrir arquivo: %s\n", nomeArquivo);
        return 0;
    }

    // Ignora o cabecalho do arquivo
    char linha[256];
    if (fgets(linha, sizeof(linha), arquivo) == NULL)
    {
        fclose(arquivo);
        return 0;
    }

    // Inicializa a matriz e a elegibilidade
    for (int i = 0; i < inst->n; i++)
    {
        inst->clientes[i].elegivelDrone = false;
        inst->clientes[i].minDrones = 0;

        for (int k = 0; k <= inst->numDrones; k++)
        {
            inst->tempoDrone[i][k] = INFINITY;
        }
    }

    // Le os tempos de cada cliente
    for (int i = 1; i < inst->n; i++)
    {
        int id;

        if (fscanf(arquivo, "%d", &id) != 1 || id != i)
        {
            fprintf(stderr, "Erro ao ler cliente %d.\n", i);
            fclose(arquivo);
            return 0;
        }

        for (int k = 1; k <= inst->numDrones; k++)
        {
            double tempo;

            if (fscanf(arquivo, "%lf", &tempo) != 1)
            {
                fprintf(stderr, "Erro ao ler tempo do cliente %d.\n", i);
                fclose(arquivo);
                return 0;
            }

            // -1 significa que a entrega e inviavel
            if (tempo == -1)
            {
                inst->tempoDrone[i][k] = INFINITY;
            }
            else if (tempo >= 0 && isfinite(tempo))
            {
                inst->tempoDrone[i][k] = tempo;

                // Cliente pode ser atendido por drones
                inst->clientes[i].elegivelDrone = true;

                // Primeira quantidade viavel encontrada
                if (inst->clientes[i].minDrones == 0)
                {
                    inst->clientes[i].minDrones = k;
                }
            }
            else
            {
                fprintf(stderr, "Tempo invalido para cliente %d.\n", i);
                fclose(arquivo);
                return 0;
            }
        }
    }

    char extra;
    int dadosExtras = fscanf(arquivo, " %c", &extra) == 1;
    int erro = ferror(arquivo);
    fclose(arquivo);
    return !dadosExtras && !erro;
}

/* Carrega os dois arquivos e constroi os tempos dos caminhoes; desfaz falhas. */
int carregarInstancia(const char *nomeArquivo, const char *arquivoTempos,
                     int numCaminhoes, Instancia *inst)
{
    if (!inst || !nomeArquivo || !arquivoTempos || numCaminhoes <= 0) return 0;
    if (!lerInstancia(nomeArquivo, inst)) {
        liberarInstancia(inst);
        return 0;
    }
    inst->numCaminhoes = numCaminhoes;
    if (!construirMatrizCaminhao(inst) || !lerTemposDrone(arquivoTempos, inst)) {
        liberarInstancia(inst);
        return 0;
    }
    return 1;
}
