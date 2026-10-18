#include <stdio.h>
#include "cv.h"
#include "main.h"
#include<math.h>
#include<stdbool.h>
#include <stdlib.h>



void alocarInstancias(Instancia *inst){

    alocarMatrizCaminhao(inst);
    alocarTempoDrone(inst);

}

void alocarSolucao(Instancia *inst, Solucao *solucao){

    solucao->missaoCaminhao = calloc(inst->n, sizeof(Missao));
    solucao->missaoDrone = calloc(inst->n, sizeof(MissaoDrone));
    solucao->numMissoesCaminhao = 0;
    solucao->numMissoesDrone = 0;
    solucao->makespan = INFINITY;

}

void liberarSolucao(Solucao *solucao){

    free(solucao->missaoCaminhao);
    free(solucao->missaoDrone);
    *solucao = (Solucao){0};

}

void construirMatrizCaminhao(Instancia *inst){

    //No PDSTSP-C o caminhão utiliza distância manhathan
    //A distância dos drones é sim euclidiana
    for (int i = 0; i < inst->n; i++) {

        for (int j = 0; j < inst->n; j++) {

            double dx = fabs(inst->clientes[i].x -
                             inst->clientes[j].x);

            double dy = fabs(inst->clientes[i].y -
                             inst->clientes[j].y);

            inst->tempoCaminhao[i][j] = (dx + dy)/ inst->truckSpeed * 60.0;
        }
    }


}


int alocarMatrizCaminhao(Instancia *inst)
{
    inst->tempoCaminhao = calloc(
        inst->n, sizeof(double *)
    );

    if (inst->tempoCaminhao == NULL)
        return 0;

    for (int i = 0; i < inst->n; i++)
    {
        inst->tempoCaminhao[i] = malloc(
            inst->n * sizeof(double)
        );

        if (inst->tempoCaminhao[i] == NULL)
            return 0;
    }

    return 1;
}

void alocarTempoDrone(Instancia *inst)
{
    // Aloca as linhas
    inst->tempoDrone = malloc(inst->n * sizeof(double *));

    if (inst->tempoDrone == NULL) {
        printf("Erro ao alocar matriz tempoDrone.\n");
        return;
    }

    // Aloca as colunas de cada linha
    for (int i = 0; i < inst->n; i++) {

        inst->tempoDrone[i] = malloc(
            (inst->numDrones + 1) * sizeof(double)
        );

        if (inst->tempoDrone[i] == NULL) {
            printf("Erro ao alocar linha %d.\n", i);

            // Libera as linhas ja alocadas
            for (int j = 0; j < i; j++) {
                free(inst->tempoDrone[j]);
            }

            free(inst->tempoDrone);
            inst->tempoDrone = NULL;
            return;
        }

        // Inicializa os tempos como inviaveis
        for (int k = 0; k <= inst->numDrones; k++) {
            inst->tempoDrone[i][k] = INFINITY;
        }
    }
}

//Todos os clientes no caminhão
int construirSolucaoInicial(const Instancia *inst, Solucao *solucao)
{
    if (solucao == NULL)
        return 0;

    solucao->makespan = INFINITY;
    solucao->numMissoesCaminhao = 0;
    solucao->numMissoesDrone = 0;

    if (inst == NULL || inst->n < 1 ||
        (inst->n > 1 && solucao->missaoCaminhao == NULL))
        return 0;

    /* Cada cliente e inserido uma vez; a avaliacao acrescenta os depositos. */
    for (int cliente = 1; cliente < inst->n; cliente++)
        solucao->missaoCaminhao[cliente - 1].n = cliente;

    solucao->numMissoesCaminhao = inst->n - 1;
    return calcularMakespan(inst, solucao);
}

//Apenas retorna se o cliente pode usar Drone
static bool clientePodeUsarDrone(const Instancia *inst, int cliente){

    if (inst->clientes == NULL || inst->tempoDrone == NULL ||
        !inst->clientes[cliente].elegivelDrone || inst->tempoDrone[cliente] == NULL)
        return false;
    int k = inst->clientes[cliente].minDrones;
    return k >= 1 && k <= inst->numDrones &&
        isfinite(inst->tempoDrone[cliente][k]) && inst->tempoDrone[cliente][k] >= 0.0;
}


int gerarVizinho(const Instancia *inst, const Solucao *atual, Solucao *vizinho){

    if (inst == NULL || atual == NULL || vizinho == NULL || atual == vizinho ||
        inst->n < 1 || inst->numDrones < 0)
        return 0;

    
    //nc = numero de missoes caminhao
    //nd = numero de missoes drone

    int nc = atual->numMissoesCaminhao, nd = atual->numMissoesDrone;
    
    //verificação de falha
    if (nc < 0 || nc > inst->n - 1 || nd != inst->n - 1 - nc ||
        (nc > 0 && atual->missaoCaminhao == NULL) ||
        (nd > 0 && atual->missaoDrone == NULL) ||
        vizinho->missaoCaminhao == NULL || vizinho->missaoDrone == NULL ||
        vizinho->missaoCaminhao == atual->missaoCaminhao ||
        vizinho->missaoDrone == atual->missaoDrone)
        return 0;


    bool *visitado = calloc(inst->n, sizeof *visitado);
    if (visitado == NULL)
        return 0;
    int elegiveis = 0;


    //O objetivo é elencar a quantidade de clientes que estão no caminhão e que podem ser elegíveis para drones
    for (int i = 0; i < nc + nd; i++) {
        //Pego o cliente a ser analisado
        int cliente = i < nc ? atual->missaoCaminhao[i].n :
            atual->missaoDrone[i - nc].cliente;
        //Verificação se posso usar drone no cliente
        if (cliente <= 0 || cliente >= inst->n || visitado[cliente] ||
            (i >= nc && !clientePodeUsarDrone(inst, cliente))) {
            free(visitado);
            return 0;
        }
        visitado[cliente] = true;

        if (i < nc && clientePodeUsarDrone(inst, cliente))
            elegiveis++;
    }
    free(visitado);

    enum { TROCAR_CAMINHAO, TROCAR_DRONES, PARA_DRONES, PARA_CAMINHAO };
    int movimentos[4], quantidade = 0;

    //Defino a quantidade de movimentos que estão disponíveis, e efetivamente quais são eles
    //Uma maneira muito elegante, crio um vetor que recebe progressivamente quais tipos de movimentos
    //são possíveis de serem realizados e, paralelamente, um contar para quantos são
    if (nc >= 2) movimentos[quantidade++] = TROCAR_CAMINHAO;
    if (nd >= 2) movimentos[quantidade++] = TROCAR_DRONES;
    if (elegiveis > 0) movimentos[quantidade++] = PARA_DRONES;
    if (nd > 0) movimentos[quantidade++] = PARA_CAMINHAO;
    if (quantidade == 0)
        return 0;

    //Inicialmente eu copio a solução atual para o vizinho
    for (int i = 0; i < nc; i++)
        vizinho->missaoCaminhao[i] = atual->missaoCaminhao[i];
    for (int i = 0; i < nd; i++)
        vizinho->missaoDrone[i] = atual->missaoDrone[i];

    //Apenas um sorteio para ver qual movimento é possível
    int movimento = movimentos[rand() % quantidade];

    if (movimento == TROCAR_CAMINHAO || movimento == TROCAR_DRONES) {

        //pego o tamanho
        int tamanho = movimento == TROCAR_CAMINHAO ? nc : nd;

        //sorteio as duas posições a serem trocadas
        int i = rand() % tamanho, j = rand() % (tamanho - 1);
        
        //preciso desse if j >= i para conseguir alcançar a última posição do vetor
        if (j >= i) j++;
        if (movimento == TROCAR_CAMINHAO) {
            Missao auxiliar = vizinho->missaoCaminhao[i];
            vizinho->missaoCaminhao[i] = vizinho->missaoCaminhao[j];
            vizinho->missaoCaminhao[j] = auxiliar;
        } else {
            MissaoDrone auxiliar = vizinho->missaoDrone[i];
            vizinho->missaoDrone[i] = vizinho->missaoDrone[j];
            vizinho->missaoDrone[j] = auxiliar;
        }

    } else if (movimento == PARA_DRONES) {
        //sorteio um dos elegíveis
        int escolhido = rand() % elegiveis, origem = 0;
        //dentro do vetor de clientes do caminhão, analisamos aqueles que podem ir de drone, e pegamos a posição escolhida
        for (; origem < nc; origem++) {
            if (clientePodeUsarDrone(inst, vizinho->missaoCaminhao[origem].n) &&
                escolhido-- == 0)
                break;
        }
        //selecionamos aquele que será alterado
        int cliente = vizinho->missaoCaminhao[origem].n;
        int destino = rand() % (nd + 1);
        //Sorteamos a posição de inserção e colocamos no vetor
        for (int i = origem; i < nc - 1; i++)
            vizinho->missaoCaminhao[i] = vizinho->missaoCaminhao[i + 1];
        for (int i = nd; i > destino; i--)
            vizinho->missaoDrone[i] = vizinho->missaoDrone[i - 1];
        vizinho->missaoDrone[destino] = (MissaoDrone){cliente, 0, 0.0, 0.0};
        nc--; nd++;
    } else {
        //sorteamos um dos drones e colocamos no caminhão
        int origem = rand() % nd, destino = rand() % (nc + 1);
        int cliente = vizinho->missaoDrone[origem].cliente;
        for (int i = origem; i < nd - 1; i++)
            vizinho->missaoDrone[i] = vizinho->missaoDrone[i + 1];
        for (int i = nc; i > destino; i--)
            vizinho->missaoCaminhao[i] = vizinho->missaoCaminhao[i - 1];
        vizinho->missaoCaminhao[destino].n = cliente;
        nd--; nc++;
    }
    vizinho->numMissoesCaminhao = nc;
    vizinho->numMissoesDrone = nd;
    vizinho->makespan = INFINITY;

    //Depois vamos apenas salvar os dados, até porque foi alterado o vetor dos drones
    //O escalonamento será feito posteriormente
    for (int i = 0; i < nd; i++) {
        MissaoDrone *m = &vizinho->missaoDrone[i];
        m->qtdDrones = inst->clientes[m->cliente].minDrones;
        m->tempoInicio = INFINITY;
        m->tempoFim = INFINITY;
    }
    return 1;
}

int escalonarMissoesDrone(const Instancia *inst, Solucao *solucao){
    
    if (solucao == NULL)
        return 0;
    solucao->makespan = INFINITY;
    if (inst == NULL || inst->n < 1 || inst->numDrones < 0 ||
        solucao->numMissoesDrone < 0 || solucao->numMissoesDrone > inst->n - 1)
        return 0;
    int quantidade = solucao->numMissoesDrone;
    if (quantidade == 0)
        return 1;
    if (inst->numDrones == 0 || inst->clientes == NULL ||
        inst->tempoDrone == NULL || solucao->missaoDrone == NULL)
        return 0;

    double *disponibilidade = calloc(inst->numDrones, sizeof *disponibilidade);
    bool *selecionado = calloc(inst->numDrones, sizeof *selecionado);
    bool *visitado = calloc(inst->n, sizeof *visitado);
    MissaoDrone *cronograma = calloc(quantidade, sizeof *cronograma);
    int sucesso = 0;
    if (disponibilidade == NULL || selecionado == NULL ||
        visitado == NULL || cronograma == NULL)
        goto finalizar;

    for (int i = 0; i < quantidade; i++) {
        int cliente = solucao->missaoDrone[i].cliente;
        if (cliente <= 0 || cliente >= inst->n || visitado[cliente] ||
            !inst->clientes[cliente].elegivelDrone || inst->tempoDrone[cliente] == NULL)
            goto finalizar;
        int k = inst->clientes[cliente].minDrones;
        if (k < 1 || k > inst->numDrones)
            goto finalizar;
        double duracao = inst->tempoDrone[cliente][k];
        if (!isfinite(duracao) || duracao < 0.0)
            goto finalizar;
        visitado[cliente] = true;

        for (int d = 0; d < inst->numDrones; d++)
            selecionado[d] = false;
        double inicio = 0.0;
        for (int unidade = 0; unidade < k; unidade++) {
            int melhor = -1;
            for (int d = 0; d < inst->numDrones; d++) {
                if (!selecionado[d] &&
                    (melhor == -1 || disponibilidade[d] < disponibilidade[melhor]))
                    melhor = d;
            }
            /* Empates sao resolvidos pelo menor indice do drone. */
            selecionado[melhor] = true;
            inicio = fmax(inicio, disponibilidade[melhor]);
        }
        double fim = inicio + duracao;
        if (!isfinite(fim))
            goto finalizar;
        for (int d = 0; d < inst->numDrones; d++) {
            if (selecionado[d])
                disponibilidade[d] = fim;
        }
        cronograma[i] = (MissaoDrone){cliente, k, inicio, fim};
    }

    /* Publica somente depois de validar e escalonar todas as missoes. */
    for (int i = 0; i < quantidade; i++)
        solucao->missaoDrone[i] = cronograma[i];
    sucesso = 1;

finalizar:
    free(disponibilidade);
    free(selecionado);
    free(visitado);
    free(cronograma);
    return sucesso;
}

int calcularMakespan(const Instancia *inst, Solucao *solucao)
{
    const double tolerancia = 1e-9;
    if (solucao == NULL)
        return 0;
    solucao->makespan = INFINITY;

    if (inst == NULL || inst->n < 1 || inst->numDrones < 0 ||
        solucao->numMissoesCaminhao < 0 || solucao->numMissoesDrone < 0 ||
        solucao->numMissoesCaminhao > inst->n - 1 ||
        solucao->numMissoesDrone != inst->n - 1 - solucao->numMissoesCaminhao ||
        (solucao->numMissoesCaminhao > 0 &&
         (solucao->missaoCaminhao == NULL || inst->tempoCaminhao == NULL)) ||
        (solucao->numMissoesDrone > 0 &&
         (solucao->missaoDrone == NULL || inst->tempoDrone == NULL)))
        return 0;

    bool *visitado = calloc(inst->n, sizeof *visitado);
    if (visitado == NULL)
        return 0;

    double tempoCaminhao = 0.0, tempoDrones = 0.0;
    int anterior = 0;
    for (int i = 0; i < solucao->numMissoesCaminhao; i++) {
        int cliente = solucao->missaoCaminhao[i].n;
        if (cliente <= 0 || cliente >= inst->n || visitado[cliente] ||
            inst->tempoCaminhao[anterior] == NULL)
            goto invalida;
        double trecho = inst->tempoCaminhao[anterior][cliente];
        if (!isfinite(trecho) || trecho < 0.0)
            goto invalida;
        tempoCaminhao += trecho;
        visitado[cliente] = true;
        anterior = cliente;
    }
    if (solucao->numMissoesCaminhao > 0) {
        if (inst->tempoCaminhao[anterior] == NULL)
            goto invalida;
        double retorno = inst->tempoCaminhao[anterior][0];
        if (!isfinite(retorno) || retorno < 0.0)
            goto invalida;
        tempoCaminhao += retorno;
    }

    for (int i = 0; i < solucao->numMissoesDrone; i++) {
        const MissaoDrone *m = &solucao->missaoDrone[i];
        if (m->cliente <= 0 || m->cliente >= inst->n || visitado[m->cliente] ||
            m->qtdDrones < 1 || m->qtdDrones > inst->numDrones ||
            !isfinite(m->tempoInicio) || !isfinite(m->tempoFim) ||
            m->tempoInicio < 0.0 || m->tempoFim < m->tempoInicio ||
            inst->tempoDrone[m->cliente] == NULL)
            goto invalida;
        double duracao = inst->tempoDrone[m->cliente][m->qtdDrones];
        if (!isfinite(duracao) || duracao < 0.0 ||
            fabs((m->tempoFim - m->tempoInicio) - duracao) >
                tolerancia * fmax(1.0, duracao))
            goto invalida;
        visitado[m->cliente] = true;
        tempoDrones = fmax(tempoDrones, m->tempoFim);
    }

    /* O uso da frota so pode aumentar no inicio de uma missao.
     * Intervalos [inicio, fim): drones que retornam podem sair no mesmo instante. */
    for (int i = 0; i < solucao->numMissoesDrone; i++) {
        double instante = solucao->missaoDrone[i].tempoInicio;
        int disponiveis = inst->numDrones;
        for (int j = 0; j < solucao->numMissoesDrone; j++) {
            const MissaoDrone *m = &solucao->missaoDrone[j];
            if (m->tempoInicio <= instante && instante < m->tempoFim) {
                if (m->qtdDrones > disponiveis)
                    goto invalida;
                disponiveis -= m->qtdDrones;
            }
        }
    }

    if (!isfinite(tempoCaminhao))
        goto invalida;
    solucao->makespan = fmax(tempoCaminhao, tempoDrones);
    free(visitado);
    return 1;

invalida:
    free(visitado);
    return 0;
}

