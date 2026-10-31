#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "lerInstancias.h"
#include "cv.h"
#define TAM_LINHA 256
#define CAPACIDADE_INICIAL 16

/*
 * Inicializa a struct para um estado seguro.
 * Assim, em caso de erro durante a leitura, podemos liberar a memoria
 * sem acessar ponteiros nao inicializados.
 */
static void inicializarInstancia(Instancia *inst)
{
    inst->n = 0;
    inst->numDrones = 0;
    inst->numCaminhoes = 0;

    inst->truckSpeed = 0.0;
    inst->batteryWeight = 0.0;
    inst->batteryCapacity = 0.0;
    inst->maxDischargeCurrent = 0.0;
    inst->voltage = 0.0;

    inst->clientes = NULL;
    inst->tempoCaminhao = NULL;
    inst->tempoDrone = NULL;
}

/*
 * Libera uma matriz caso ela ja tenha sido criada em outra etapa
 * do projeto.
 */
static void liberarMatriz(double **matriz, int linhas)
{
    if (matriz == NULL)
        return;

    for (int i = 0; i < linhas; i++)
        free(matriz[i]);

    free(matriz);
}

void liberarInstancia(Instancia *inst)
{
    if (inst == NULL)
        return;

    free(inst->clientes);
    inst->clientes = NULL;

    liberarMatriz(inst->tempoCaminhao, inst->n);
    inst->tempoCaminhao = NULL;

    liberarMatriz(inst->tempoDrone, inst->n);
    inst->tempoDrone = NULL;

    inicializarInstancia(inst);
}

int lerInstancia(const char *nomeArquivo, Instancia *inst)
{
    if (nomeArquivo == NULL || inst == NULL)
        return 0;

    inicializarInstancia(inst);

    FILE *arquivo = fopen(nomeArquivo, "r");

    if (arquivo == NULL)
    {
        fprintf(stderr, "Erro ao abrir o arquivo: %s\n", nomeArquivo);
        return 0;
    }

    char linha[TAM_LINHA];

    /* ========================================================
       1. Le os parametros gerais e salva diretamente em Instancia
       ======================================================== */

    if (fgets(linha, sizeof(linha), arquivo) == NULL ||
        sscanf(linha, "NUM_DRONES, %d", &inst->numDrones) != 1)
    {
        fprintf(stderr, "Erro ao ler NUM_DRONES.\n");
        fclose(arquivo);
        return 0;
    }

    if (fgets(linha, sizeof(linha), arquivo) == NULL ||
        sscanf(linha, "TRUCK_SPEED, %lf", &inst->truckSpeed) != 1)
    {
        fprintf(stderr, "Erro ao ler TRUCK_SPEED.\n");
        fclose(arquivo);
        return 0;
    }

    if (fgets(linha, sizeof(linha), arquivo) == NULL ||
        sscanf(linha, "BATTERY_WEIGHT, %lf", &inst->batteryWeight) != 1)
    {
        fprintf(stderr, "Erro ao ler BATTERY_WEIGHT.\n");
        fclose(arquivo);
        return 0;
    }

    if (fgets(linha, sizeof(linha), arquivo) == NULL ||
        sscanf(linha, "BATTERY_CAPACITY, %lf", &inst->batteryCapacity) != 1)
    {
        fprintf(stderr, "Erro ao ler BATTERY_CAPACITY.\n");
        fclose(arquivo);
        return 0;
    }

    if (fgets(linha, sizeof(linha), arquivo) == NULL ||
        sscanf(linha, "MAX_DISCHARGE_CURRENT, %lf", &inst->maxDischargeCurrent) != 1)
    {
        fprintf(stderr, "Erro ao ler MAX_DISCHARGE_CURRENT.\n");
        fclose(arquivo);
        return 0;
    }

    if (fgets(linha, sizeof(linha), arquivo) == NULL ||
        sscanf(linha, "VOLTAGE, %lf", &inst->voltage) != 1)
    {
        fprintf(stderr, "Erro ao ler VOLTAGE.\n");
        fclose(arquivo);
        return 0;
    }

    /* ========================================================
       2. Le e valida o cabecalho da tabela de nos
       ======================================================== */

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

    /* ========================================================
       3. Le os clientes.

       Como o arquivo nao informa previamente quantos nos existem,
       usamos realloc para aumentar o vetor conforme necessario.
       ======================================================== */

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

        /* Esses dois campos NAO existem no arquivo.
           Serao calculados posteriormente pelo modelo do PDSTSP-C. */
        cliente.elegivelDrone = false;
        cliente.minDrones = 0;

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

    /* Nesta etapa, esses dados ainda nao sao calculados. */
    inst->tempoCaminhao = NULL;
    inst->tempoDrone = NULL;

    return 1;
}

void imprimirInstancia(const Instancia *inst)
{
    if (inst == NULL)
        return;

    printf("NUM_DRONES: %d\n", inst->numDrones);
    printf("NUM_CAMINHOES: %d\n", inst->numCaminhoes);
    printf("TRUCK_SPEED: %.2f\n", inst->truckSpeed);
    printf("BATTERY_WEIGHT: %.2f\n", inst->batteryWeight);
    printf("BATTERY_CAPACITY: %.2f\n", inst->batteryCapacity);
    printf("MAX_DISCHARGE_CURRENT: %.2f\n", inst->maxDischargeCurrent);
    printf("VOLTAGE: %.2f\n", inst->voltage);
    printf("NUM_NOS: %d\n\n", inst->n);

    printf("node\tx\ty\tweight\n");

    for (int i = 0; i < inst->n; i++)
    {
        const Cliente *c = &inst->clientes[i];

        printf("%d\t%.2f\t%.2f\t%.2f\n",
               c->id,
               c->x,
               c->y,
               c->peso);
    }
}

int lerTemposDrone(const char *nomeArquivo, Instancia *inst)
{
    if (!nomeArquivo || !inst || !inst->clientes || inst->n < 1 ||
        !alocarTempoDrone(inst)) return 0;

    FILE *arquivo = fopen(nomeArquivo, "r");

    if (arquivo == NULL)
    {
        printf("Erro ao abrir arquivo: %s\n", nomeArquivo);
        return 0;
    }

    // Verifica se a matriz ja foi alocada
    if (inst->tempoDrone == NULL)
    {
        printf("Matriz tempoDrone nao foi alocada.\n");
        fclose(arquivo);
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
            printf("Erro ao ler cliente %d.\n", i);
            fclose(arquivo);
            return 0;
        }

        for (int k = 1; k <= inst->numDrones; k++)
        {
            double tempo;

            if (fscanf(arquivo, "%lf", &tempo) != 1)
            {
                printf("Erro ao ler tempo do cliente %d.\n", i);
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
                printf("Tempo invalido para cliente %d.\n", i);
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
