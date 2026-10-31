#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include <stdio.h>
#include "cv.h"
#include "main.h"
#include<math.h>
#include<stdbool.h>
#include <stdlib.h>
#include <string.h>



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
    solucao->tempoCaminhao = solucao->tempoDrones = INFINITY;
    solucao->prefixosDrone = calloc((size_t)inst->n, sizeof(double));
    solucao->cronogramaValido = solucao->avaliacaoValida = false;
    solucao->cronogramaExterno = false;
    solucao->branchBoundAplicado = false;

}

void liberarSolucao(Solucao *solucao){

    free(solucao->missaoCaminhao);
    free(solucao->missaoDrone);
    free(solucao->prefixosDrone);
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

static double tempoTrecho(const Instancia *inst, int a, int b);
static double arestaRota(const Instancia *inst, const Solucao *s, int p);

/* Delta da reversao inclui arcos internos, inclusive em matrizes assimetricas. */
static double deltaDoisOpt(const Instancia *inst, const Solucao *s, int i, int j)
{
    int a = i == 0 ? 0 : s->missaoCaminhao[i - 1].n;
    int b = s->missaoCaminhao[i].n;
    int c = s->missaoCaminhao[j].n;
    int d = j + 1 == s->numMissoesCaminhao ? 0 : s->missaoCaminhao[j + 1].n;
    double delta = tempoTrecho(inst,a,c) + tempoTrecho(inst,b,d)
                 - tempoTrecho(inst,a,b) - tempoTrecho(inst,c,d);
    for (int p = i; p < j; p++) {
        int u=s->missaoCaminhao[p].n, v=s->missaoCaminhao[p+1].n;
        delta += tempoTrecho(inst,v,u) - tempoTrecho(inst,u,v);
    }
    return delta;
}

static void inverterTrecho(Missao *rota, int i, int j)
{
    while (i < j) { Missao aux=rota[i]; rota[i++]=rota[j]; rota[j--]=aux; }
}

//Todos os clientes no caminhão
int construirSolucaoInicial(const Instancia *inst, Solucao *solucao)
{
    if (solucao == NULL)
        return 0;

    solucao->makespan = INFINITY;
    solucao->branchBoundAplicado = false;
    solucao->cronogramaValido = false;
    solucao->cronogramaExterno = false;
    solucao->numMissoesCaminhao = 0;
    solucao->numMissoesDrone = 0;

    if (inst == NULL || inst->n < 1 ||
        (inst->n > 1 && solucao->missaoCaminhao == NULL))
        return 0;

    /* Cada cliente e inserido uma vez; a avaliacao acrescenta os depositos. */
    for (int cliente = 1; cliente < inst->n; cliente++)
        solucao->missaoCaminhao[cliente - 1].n = cliente;

    solucao->numMissoesCaminhao = inst->n - 1;
    if (!calcularMakespan(inst, solucao)) return 0;
    if (inst->n <= 2) return 1;
    Missao *original = malloc((size_t)(inst->n - 1) * sizeof *original);
    bool *visitado = calloc((size_t)inst->n, sizeof *visitado);
    if (!original || !visitado) { free(original); free(visitado); return 0; }
    memcpy(original, solucao->missaoCaminhao, (size_t)(inst->n - 1) * sizeof *original);
    double referencia = solucao->makespan;
    int anterior = 0;
    for (int p = 0; p < inst->n - 1; p++) {
        int proximo = -1;
        for (int c = 1; c < inst->n; c++)
            if (!visitado[c] && isfinite(tempoTrecho(inst,anterior,c)) &&
                (proximo < 0 || tempoTrecho(inst,anterior,c) < tempoTrecho(inst,anterior,proximo)))
                proximo = c;
        if (proximo < 0) break;
        solucao->missaoCaminhao[p].n = proximo;
        visitado[proximo] = true;
        anterior = proximo;
    }
    if (!calcularMakespan(inst, solucao) || solucao->makespan > referencia)
        memcpy(solucao->missaoCaminhao, original, (size_t)(inst->n - 1) * sizeof *original);
    free(original); free(visitado);
    bool melhorou;
    do {
        melhorou = false;
        for (int i = 0; i < inst->n - 2 && !melhorou; i++)
            for (int j = i + 1; j < inst->n - 1; j++) {
                double delta = deltaDoisOpt(inst, solucao, i, j);
                if (isfinite(delta) && delta < -1e-9) {
                    inverterTrecho(solucao->missaoCaminhao, i, j);
                    melhorou = true; break;
                }
            }
    } while (melhorou);
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


/* Conta ou sorteia quantidades viaveis, excluindo a quantidade atual. */
static int quantidadeDroneAlternativa(const Instancia *inst, int cliente,
                                      int excluir, bool sortear){
    int quantidade = 0;
    for (int k = inst->clientes[cliente].minDrones; k <= inst->numDrones; k++)
        if (k != excluir && isfinite(inst->tempoDrone[cliente][k]) &&
            inst->tempoDrone[cliente][k] >= 0.0)
            quantidade++;
    if (!sortear || quantidade == 0)
        return quantidade;
    int escolhido = rand() % quantidade;
    for (int k = inst->clientes[cliente].minDrones; k <= inst->numDrones; k++)
        if (k != excluir && isfinite(inst->tempoDrone[cliente][k]) &&
            inst->tempoDrone[cliente][k] >= 0.0 && escolhido-- == 0)
            return k;
    return 0;
}

typedef struct AlteracaoVizinho {
    int primeiraDrone;
    double deltaCaminhao;
} AlteracaoVizinho;

static double tempoTrecho(const Instancia *inst, int a, int b){
    if (inst->tempoCaminhao == NULL || inst->tempoCaminhao[a] == NULL)
        return NAN;
    double tempo = inst->tempoCaminhao[a][b];
    return isfinite(tempo) && tempo >= 0.0 ? tempo : NAN;
}

/* Aresta de indice p: deposito/cliente anterior -> cliente/deposito seguinte. */
static double arestaRota(const Instancia *inst, const Solucao *s, int p){
    if (s->numMissoesCaminhao == 0) return 0.0;
    int a = p == 0 ? 0 : s->missaoCaminhao[p - 1].n;
    int b = p == s->numMissoesCaminhao ? 0 : s->missaoCaminhao[p].n;
    return tempoTrecho(inst, a, b);
}

/* Movimentos conjuntos: a ordem dos drones restantes e preservada. */
static double custoRotaMelhoria(const Instancia *inst, const Solucao *s)
{
    double valor=0.0; int anterior=0;
    for(int i=0;i<s->numMissoesCaminhao;i++) {
        int j=s->missaoCaminhao[i].n;
        valor+=tempoTrecho(inst,anterior,j); anterior=j;
    }
    return valor+tempoTrecho(inst,anterior,0);
}

static int moverGrupoMelhoria(const Instancia *inst, Solucao *s, bool paraDrones)
{
    int nc=s->numMissoesCaminhao, nd=s->numMissoesDrone;
    int tamanho=paraDrones?nc:nd;
    if(tamanho<2 || !inst->clientes) return 0;
    int *selecionados=calloc((size_t)inst->n,sizeof *selecionados);
    if(!selecionados) return -1;
    int ancora=-1, contagem=0;
    /* Sorteio uniforme entre as ancoras viaveis. */
    for(int i=0;i<tamanho;i++) {
        int j=paraDrones?s->missaoCaminhao[i].n:s->missaoDrone[i].cliente;
        if(!paraDrones || clientePodeUsarDrone(inst,j)) {
            contagem++; if(rand()%contagem==0) ancora=j;
        }
    }
    if(ancora<0 || contagem<2) {free(selecionados); return 0;}
    int qtd=0;
    bool localCompleto=true;
    /* Grupo inteiro da mesma coordenada, se todos forem elegiveis. */
    for(int i=0;i<tamanho;i++) {
        int j=paraDrones?s->missaoCaminhao[i].n:s->missaoDrone[i].cliente;
        if(inst->clientes[j].x==inst->clientes[ancora].x &&
           inst->clientes[j].y==inst->clientes[ancora].y) {
            if(paraDrones && !clientePodeUsarDrone(inst,j)) {localCompleto=false; break;}
            selecionados[j]=1; qtd++;
        }
    }
    if(!localCompleto || qtd<2 || rand()%4==0) {
        memset(selecionados,0,(size_t)inst->n*sizeof *selecionados);
        selecionados[ancora]=1; qtd=1;
        int alvo=2+rand()%3; if(alvo>contagem) alvo=contagem;
        while(qtd<alvo) {
            int proximo=-1; double distancia=INFINITY;
            for(int i=0;i<tamanho;i++) {
                int j=paraDrones?s->missaoCaminhao[i].n:s->missaoDrone[i].cliente;
                if(selecionados[j] || (paraDrones && !clientePodeUsarDrone(inst,j))) continue;
                double d=fabs(inst->clientes[j].x-inst->clientes[ancora].x)+
                         fabs(inst->clientes[j].y-inst->clientes[ancora].y);
                if(d<distancia || (d==distancia && rand()%2)) {proximo=j;distancia=d;}
            }
            if(proximo<0) break;
            selecionados[proximo]=1; qtd++;
        }
    }
    if(paraDrones) {
        int saida=0, destino=rand()%(nd+1);
        memmove(s->missaoDrone+destino+qtd,s->missaoDrone+destino,(size_t)(nd-destino)*sizeof *s->missaoDrone);
        int pos=destino;
        for(int i=0;i<nc;i++) {
            int j=s->missaoCaminhao[i].n;
            if(selecionados[j]) s->missaoDrone[pos++]=(MissaoDrone){j,quantidadeDroneAlternativa(inst,j,0,true),0,0};
            else s->missaoCaminhao[saida++]=s->missaoCaminhao[i];
        }
        s->numMissoesCaminhao=saida; s->numMissoesDrone=nd+qtd;
    } else {
        int saida=0;
        for(int i=0;i<nd;i++) {
            int j=s->missaoDrone[i].cliente;
            if(!selecionados[j]) {s->missaoDrone[saida++]=s->missaoDrone[i];continue;}
            /* Melhor insercao, sorteando empates. */
            int destino=0, empates=0; double menor=INFINITY;
            for(int p=0;p<=nc;p++) {
                int a=p?s->missaoCaminhao[p-1].n:0;
                int b=p<nc?s->missaoCaminhao[p].n:0;
                double delta=tempoTrecho(inst,a,j)+tempoTrecho(inst,j,b)-(nc?tempoTrecho(inst,a,b):0.0);
                if(delta<menor) {menor=delta;destino=p;empates=1;}
                else if(delta==menor && rand()%++empates==0) destino=p;
            }
            memmove(s->missaoCaminhao+destino+1,s->missaoCaminhao+destino,(size_t)(nc-destino)*sizeof *s->missaoCaminhao);
            s->missaoCaminhao[destino].n=j; nc++;
        }
        s->numMissoesCaminhao=nc; s->numMissoesDrone=saida;
    }
    free(selecionados); return 1;
}

static int gerarVizinhoInterno(const Instancia *inst, const Solucao *atual,
                              Solucao *vizinho, AlteracaoVizinho *alteracao,
                              const double *pesos, TipoMovimento *tipo, bool grupos){

    if (inst == NULL || atual == NULL || vizinho == NULL || atual == vizinho ||
        inst->n < 1 || inst->numDrones < 0)
        return 0;

    
    //nc = numero de missoes caminhao
    //nd = numero de missoes drone

    int nc = atual->numMissoesCaminhao, nd = atual->numMissoesDrone;
    if (pesos != NULL)
        for (int i = 0; i < NUM_MOVIMENTOS; i++)
            if (!isfinite(pesos[i]) || pesos[i] <= 0.0) return 0;
    
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
    int elegiveis = 0, alteraveis = 0;


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
        if (i >= nc && quantidadeDroneAlternativa(inst, cliente,
                atual->missaoDrone[i - nc].qtdDrones, false) > 0)
            alteraveis++;
    }
    free(visitado);

    int movimentos[NUM_MOVIMENTOS], quantidade = 0;

    //Defino a quantidade de movimentos que estão disponíveis, e efetivamente quais são eles
    //Uma maneira muito elegante, crio um vetor que recebe progressivamente quais tipos de movimentos
    //são possíveis de serem realizados e, paralelamente, um contar para quantos são
    if (nc >= 2) movimentos[quantidade++] = TROCAR_CAMINHAO;
    if (nc >= 3) movimentos[quantidade++] = REINSERIR_CAMINHAO;
    if (nc >= 3) movimentos[quantidade++] = DOIS_OPT_CAMINHAO;
    if (nd >= 2) movimentos[quantidade++] = TROCAR_DRONES;
    if (elegiveis > 0) movimentos[quantidade++] = PARA_DRONES;
    if (nd > 0) movimentos[quantidade++] = PARA_CAMINHAO;
    if (alteraveis > 0) movimentos[quantidade++] = ALTERAR_QUANTIDADE_DRONES;
    if (quantidade == 0)
        return 0;

    //Inicialmente eu copio a solução atual para o vizinho
    for (int i = 0; i < nc; i++)
        vizinho->missaoCaminhao[i] = atual->missaoCaminhao[i];
    for (int i = 0; i < nd; i++)
        vizinho->missaoDrone[i] = atual->missaoDrone[i];

    //Apenas um sorteio para ver qual movimento é possível
    vizinho->numMissoesCaminhao = nc;
    vizinho->numMissoesDrone = nd;
    if (alteracao) *alteracao = (AlteracaoVizinho){nd, 0.0};
    int movimento;
    if (pesos == NULL) {
        movimento = movimentos[rand() % quantidade];
    } else {
        /* Normaliza pelo maior peso para evitar overflow na soma. */
        double maior = 0.0, soma = 0.0;
        for (int i = 0; i < quantidade; i++) maior = fmax(maior, pesos[movimentos[i]]);
        for (int i = 0; i < quantidade; i++) soma += pesos[movimentos[i]] / maior;
        double sorteio = ((double)rand() / ((double)RAND_MAX + 1.0)) * soma;
        movimento = movimentos[quantidade - 1];
        for (int i = 0; i < quantidade; i++) {
            double parcela = pesos[movimentos[i]] / maior;
            if (sorteio < parcela) { movimento = movimentos[i]; break; }
            sorteio -= parcela;
        }
    }

    int grupo=0;
    if(grupos && (movimento==PARA_DRONES || movimento==PARA_CAMINHAO) && rand()%4==0) {
        grupo=moverGrupoMelhoria(inst,vizinho,movimento==PARA_DRONES);
        if(grupo<0) return 0;
    }
    if(grupo) {
        nc=vizinho->numMissoesCaminhao; nd=vizinho->numMissoesDrone;
        if(alteracao) {alteracao->primeiraDrone=0;alteracao->deltaCaminhao=custoRotaMelhoria(inst,vizinho)-atual->tempoCaminhao;}
    } else if (movimento == TROCAR_CAMINHAO || movimento == TROCAR_DRONES) {

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
            if (alteracao) {
                int posicoes[] = {i, i + 1, j, j + 1};
                for (int a = 0; a < 4; a++) {
                    int repetida = 0;
                    for (int b = 0; b < a; b++)
                        if (posicoes[a] == posicoes[b]) repetida = 1;
                    if (!repetida)
                        alteracao->deltaCaminhao += arestaRota(inst, vizinho, posicoes[a])
                            - arestaRota(inst, atual, posicoes[a]);
                }
            }
        } else {
            MissaoDrone auxiliar = vizinho->missaoDrone[i];
            vizinho->missaoDrone[i] = vizinho->missaoDrone[j];
            vizinho->missaoDrone[j] = auxiliar;
            if (alteracao) alteracao->primeiraDrone = i < j ? i : j;
        }

    } else if (movimento == REINSERIR_CAMINHAO) {
        int origem=rand()%nc, destino=rand()%(nc-1);
        if (destino >= origem) destino++;
        int a=origem==0?0:atual->missaoCaminhao[origem-1].n;
        int x=atual->missaoCaminhao[origem].n;
        int b=origem==nc-1?0:atual->missaoCaminhao[origem+1].n;
        Missao movida=vizinho->missaoCaminhao[origem];
        if (origem < destino)
            for(int p=origem;p<destino;p++) vizinho->missaoCaminhao[p]=vizinho->missaoCaminhao[p+1];
        else
            for(int p=origem;p>destino;p--) vizinho->missaoCaminhao[p]=vizinho->missaoCaminhao[p-1];
        vizinho->missaoCaminhao[destino]=movida;
        int u=destino==0?0:vizinho->missaoCaminhao[destino-1].n;
        int v=destino==nc-1?0:vizinho->missaoCaminhao[destino+1].n;
        if(alteracao) alteracao->deltaCaminhao = tempoTrecho(inst,a,b)-tempoTrecho(inst,a,x)
            -tempoTrecho(inst,x,b)+tempoTrecho(inst,u,x)+tempoTrecho(inst,x,v)-tempoTrecho(inst,u,v);
    } else if (movimento == DOIS_OPT_CAMINHAO) {
        int i=rand()%nc, j=rand()%(nc-1);
        if(j>=i) j++;
        if(i>j) {int aux=i;i=j;j=aux;}
        if(alteracao) alteracao->deltaCaminhao=deltaDoisOpt(inst,atual,i,j);
        inverterTrecho(vizinho->missaoCaminhao,i,j);
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
        if (alteracao) {
            int a = origem == 0 ? 0 : atual->missaoCaminhao[origem - 1].n;
            int b = origem == nc - 1 ? 0 : atual->missaoCaminhao[origem + 1].n;
            alteracao->deltaCaminhao = (nc == 1 ? 0.0 : tempoTrecho(inst, a, b))
                - arestaRota(inst, atual, origem) - arestaRota(inst, atual, origem + 1);
            alteracao->primeiraDrone = destino;
        }
        //Sorteamos a posição de inserção e colocamos no vetor
        for (int i = origem; i < nc - 1; i++)
            vizinho->missaoCaminhao[i] = vizinho->missaoCaminhao[i + 1];
        for (int i = nd; i > destino; i--)
            vizinho->missaoDrone[i] = vizinho->missaoDrone[i - 1];
        int k = quantidadeDroneAlternativa(inst, cliente, 0, true);
        vizinho->missaoDrone[destino] = (MissaoDrone){cliente, k, 0.0, 0.0};
        nc--; nd++;
    } else if (movimento == ALTERAR_QUANTIDADE_DRONES) {
        int escolhido = rand() % alteraveis;
        for (int i = 0; i < nd; i++) {
            MissaoDrone *m = &vizinho->missaoDrone[i];
            if (quantidadeDroneAlternativa(inst, m->cliente, m->qtdDrones, false) > 0 &&
                escolhido-- == 0) {
                if (alteracao) alteracao->primeiraDrone = i;
                m->qtdDrones = quantidadeDroneAlternativa(inst, m->cliente, m->qtdDrones, true);
                break;
            }
        }
    } else {
        //sorteamos um dos drones e colocamos no caminhão
        int origem = rand() % nd, destino = rand() % (nc + 1);
        int cliente = vizinho->missaoDrone[origem].cliente;
        if (alteracao) {
            int a = destino == 0 ? 0 : atual->missaoCaminhao[destino - 1].n;
            int b = destino == nc ? 0 : atual->missaoCaminhao[destino].n;
            alteracao->deltaCaminhao = tempoTrecho(inst, a, cliente)
                + tempoTrecho(inst, cliente, b) - arestaRota(inst, atual, destino);
            alteracao->primeiraDrone = origem;
        }
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
    vizinho->avaliacaoValida = vizinho->cronogramaValido = false;
    vizinho->branchBoundAplicado = atual->branchBoundAplicado &&
        (movimento == TROCAR_CAMINHAO || movimento == REINSERIR_CAMINHAO ||
         movimento == DOIS_OPT_CAMINHAO);
    vizinho->cronogramaExterno = atual->cronogramaExterno &&
        (movimento == TROCAR_CAMINHAO || movimento == REINSERIR_CAMINHAO ||
         movimento == DOIS_OPT_CAMINHAO);

    //Depois vamos apenas salvar os dados, até porque foi alterado o vetor dos drones
    //O escalonamento será feito posteriormente
    for (int i = 0; i < nd; i++) {
        MissaoDrone *m = &vizinho->missaoDrone[i];
        if (m->qtdDrones == 0)
            m->qtdDrones = inst->clientes[m->cliente].minDrones;
        if (!alteracao || i >= alteracao->primeiraDrone) {
            m->tempoInicio = INFINITY;
            m->tempoFim = INFINITY;
        }
    }
    if (tipo != NULL) *tipo = (TipoMovimento)movimento;
    return 1;
}

int gerarVizinho(const Instancia *inst, const Solucao *atual, Solucao *vizinho){
    return gerarVizinhoInterno(inst, atual, vizinho, NULL, NULL, NULL, false);
}

/* Perfil de ocupacao: eventos ordenados; retornos e partidas no mesmo
 * instante sao agregados, respeitando intervalos [inicio,fim). */
typedef struct EventoDrone { double tempo; int delta; } EventoDrone;
typedef struct PerfilDrone { EventoDrone *eventos; int quantidade; } PerfilDrone;

static void inserirEventoDrone(PerfilDrone *p, double tempo, int delta)
{
    int i=0;
    while(i<p->quantidade && p->eventos[i].tempo<tempo) i++;
    if(i<p->quantidade && p->eventos[i].tempo==tempo) {
        p->eventos[i].delta+=delta;
        if(p->eventos[i].delta==0) {
            memmove(p->eventos+i,p->eventos+i+1,(size_t)(p->quantidade-i-1)*sizeof(EventoDrone));
            p->quantidade--;
        }
    } else {
        memmove(p->eventos+i+1,p->eventos+i,(size_t)(p->quantidade-i)*sizeof(EventoDrone));
        p->eventos[i]=(EventoDrone){tempo,delta}; p->quantidade++;
    }
}

static void ocuparPerfilDrone(PerfilDrone *p, double inicio, double fim, int drones)
{
    if(inicio==fim) return;
    inserirEventoDrone(p,inicio,drones);
    inserirEventoDrone(p,fim,-drones);
}

/* Uma varredura do perfil encontra a primeira janela continua com capacidade. */
static double primeiroInicioPerfil(const PerfilDrone *p, int frota, double duracao, int drones)
{
    if(duracao==0.0) return 0.0;
    int uso=0, capacidade=frota-drones;
    double candidato=0.0;
    for(int i=0;i<p->quantidade;i++) {
        double t=p->eventos[i].tempo;
        if(uso>capacidade) candidato=t;
        else if(t-candidato>=duracao) return candidato;
        uso+=p->eventos[i].delta;
        if(uso>capacidade) candidato=t;
    }
    return candidato;
}

static int escalonarDesde(const Instancia *inst, Solucao *solucao,
                         const Solucao *base, int primeira){
    
    if (solucao == NULL)
        return 0;
    solucao->makespan = INFINITY;
    solucao->avaliacaoValida = solucao->cronogramaValido = solucao->cronogramaExterno = false;
    solucao->branchBoundAplicado = false;
    
    if (inst == NULL || inst->n < 1 || inst->numDrones < 0 ||
        solucao->numMissoesDrone < 0 || solucao->numMissoesDrone > inst->n - 1)
        return 0;
    
    int quantidade = solucao->numMissoesDrone;
    
    if (quantidade == 0) {
        solucao->tempoDrones = 0.0;
        solucao->cronogramaValido = true;
        if (solucao->prefixosDrone)
            solucao->prefixosDrone[0] = 0.0;
        return 1;
    }
    if (inst->numDrones == 0 || inst->clientes == NULL || inst->tempoDrone == NULL || solucao->missaoDrone == NULL)
        return 0;

    
    bool *visitado = calloc(inst->n, sizeof *visitado);
    MissaoDrone *cronograma = calloc(quantidade, sizeof *cronograma);
    EventoDrone *eventos = malloc((size_t)quantidade * 2 * sizeof *eventos);
    PerfilDrone perfil = {eventos,0};
    int sucesso = 0;
    if (visitado == NULL || cronograma == NULL || eventos == NULL) goto finalizar;
    double maximo = primeira > 0 ? base->prefixosDrone[primeira] : 0.0;
    for (int i = 0; i < primeira; i++) {
        cronograma[i] = solucao->missaoDrone[i];
        visitado[cronograma[i].cliente] = true;
        ocuparPerfilDrone(&perfil,cronograma[i].tempoInicio,cronograma[i].tempoFim,cronograma[i].qtdDrones);
    }
    for (int i = primeira; i < quantidade; i++) {
        int cliente = solucao->missaoDrone[i].cliente;
        if (cliente <= 0 || cliente >= inst->n || visitado[cliente] ||
            !inst->clientes[cliente].elegivelDrone || inst->tempoDrone[cliente] == NULL)
            goto finalizar;
        int minimo = inst->clientes[cliente].minDrones;
        int k = solucao->missaoDrone[i].qtdDrones;
        if (k == 0) k = minimo;
        if (minimo < 1 || k < minimo || k > inst->numDrones)
            goto finalizar;
        double duracao = inst->tempoDrone[cliente][k];
        if (!isfinite(duracao) || duracao < 0.0)
            goto finalizar;
        visitado[cliente] = true;

        double inicio = primeiroInicioPerfil(&perfil, inst->numDrones, duracao, k);
        double fim = inicio + duracao;
        if (!isfinite(fim)) goto finalizar;
        cronograma[i] = (MissaoDrone){cliente, k, inicio, fim};
        ocuparPerfilDrone(&perfil,inicio,fim,k);
        maximo = fmax(maximo, fim);
    }

    /* Publica somente depois de validar e escalonar todas as missoes. */
    if (solucao->prefixosDrone) {
        solucao->prefixosDrone[0] = 0.0;
        for (int i = 0; i < quantidade; i++)
            solucao->prefixosDrone[i + 1] = fmax(solucao->prefixosDrone[i], cronograma[i].tempoFim);
    }
    for (int i = 0; i < quantidade; i++)
        solucao->missaoDrone[i] = cronograma[i];
    solucao->tempoDrones = maximo;
    solucao->cronogramaValido = true;
    sucesso = 1;

finalizar:
    free(visitado);
    free(cronograma);
    free(eventos);
    return sucesso;
}

//Com muito lixo, a função em sí precisa de pouca coisa
int escalonarMissoesDrone(const Instancia *inst, Solucao *solucao){
    return escalonarDesde(inst, solucao, NULL, 0);
}

/* Branch and Bound apenas sobre as quantidades de drones.
 * O nivel da arvore corresponde ao mesmo indice no vetor de entrada.
 * Cada configuracao usa insercao no primeiro intervalo viavel do cronograma;
 * a ordem de processamento e fixa e nao representa uma precedencia temporal. */
typedef struct OpcaoDroneBB {
    int k;
    double inicio, fim;
    long double consumo;
} OpcaoDroneBB;

typedef struct BuscaDroneBB {
    int quantidade, frota;
    const Instancia *inst;
    MissaoDrone *tarefas, *parcial, *melhor;
    long double *cargaSufixo;
    PerfilDrone *perfis;
    EventoDrone *eventos;
    OpcaoDroneBB *opcoes;
    bool temposInteiros, interrompida, metaAtingida;
    double limiteSuperior, meta;
    unsigned long long nos, maxNos;
} BuscaDroneBB;

static double duracaoConfiguracaoBB(const BuscaDroneBB *b, int tarefa, int k)
{
    int cliente=b->tarefas[tarefa].cliente;
    if(k<b->inst->clientes[cliente].minDrones) return INFINITY;
    double t=b->inst->tempoDrone[cliente][k];
    return isfinite(t) && t>=0.0 ? t : INFINITY;
}

/* Desigualdade (16), com soma minima do sufixo pre-calculada: O(1) por no. */
static long double limiteCargaDroneBB(const BuscaDroneBB *b, int nivel, long double fixada)
{
    long double carga=nextafterl(fixada+b->cargaSufixo[nivel],-INFINITY);
    long double limite=nextafterl(carga/b->frota,-INFINITY);
    return b->temposInteiros ? ceill(limite) : limite;
}

static int prepararOpcoesBB(BuscaDroneBB *b, int nivel)
{
    OpcaoDroneBB *opcoes=b->opcoes+(size_t)nivel*b->frota;
    int total=0;
    for(int k=1;k<=b->frota;k++) {
        double duracao=duracaoConfiguracaoBB(b,nivel,k);
        if(!isfinite(duracao)) continue;
        double inicio=primeiroInicioPerfil(&b->perfis[nivel],b->frota,duracao,k);
        double fim=inicio+duracao;
        if(!isfinite(fim)) continue;
        OpcaoDroneBB opcao={k,inicio,fim,nextafterl((long double)k*duracao,-INFINITY)};
        int j=total;
        while(j>0 && (opcao.fim<opcoes[j-1].fim ||
              (opcao.fim==opcoes[j-1].fim && opcao.consumo<opcoes[j-1].consumo))) {
            opcoes[j]=opcoes[j-1]; j--;
        }
        opcoes[j]=opcao; total++;
    }
    return total;
}

static void explorarDroneBB(BuscaDroneBB *b, int nivel, double makespan, long double cargaFixada)
{
    if(b->maxNos && b->nos>=b->maxNos) {b->interrompida=true; return;}
    b->nos++;
    if(makespan>=b->limiteSuperior) return;
    if(nivel==b->quantidade) {
        b->limiteSuperior=makespan;
        memcpy(b->melhor,b->parcial,(size_t)nivel*sizeof *b->melhor);
        if(b->meta>=0.0 && makespan<=b->meta) b->metaAtingida=true;
        return;
    }
    if(limiteCargaDroneBB(b,nivel,cargaFixada)>=(long double)b->limiteSuperior) return;
    /* Limite de cada missao restante, consultando o perfil em tempo linear. */
    for(int i=nivel;i<b->quantidade;i++) {
        double menor=INFINITY;
        for(int k=1;k<=b->frota;k++) {
            double duracao=duracaoConfiguracaoBB(b,i,k);
            if(!isfinite(duracao)) continue;
            double inicio=primeiroInicioPerfil(&b->perfis[nivel],b->frota,duracao,k);
            menor=fmin(menor,inicio+duracao);
        }
        if(menor>=b->limiteSuperior) return;
    }
    int total=prepararOpcoesBB(b,nivel);
    OpcaoDroneBB *opcoes=b->opcoes+(size_t)nivel*b->frota;
    for(int j=0;j<total;j++) {
        OpcaoDroneBB opcao=opcoes[j];
        if(opcao.fim>=b->limiteSuperior) continue;
        b->parcial[nivel]=(MissaoDrone){b->tarefas[nivel].cliente,opcao.k,opcao.inicio,opcao.fim};
        PerfilDrone *filho=&b->perfis[nivel+1], *pai=&b->perfis[nivel];
        filho->quantidade=pai->quantidade;
        memcpy(filho->eventos,pai->eventos,(size_t)pai->quantidade*sizeof(EventoDrone));
        ocuparPerfilDrone(filho,opcao.inicio,opcao.fim,opcao.k);
        long double carga=nextafterl(cargaFixada+opcao.consumo,-INFINITY);
        explorarDroneBB(b,nivel+1,fmax(makespan,opcao.fim),carga);
        if(b->interrompida || b->metaAtingida) break;
    }
}

/* Reconstrucao de incumbentes com a regra compartilhada. Passo 0 utiliza
 * as quantidades recebidas; passo 1 escolhe menor termino em cada missao. */
static void construirIncumbenteBB(BuscaDroneBB *b, bool guloso)
{
    PerfilDrone *perfil=&b->perfis[0]; perfil->quantidade=0;
    double maximo=0.0;
    for(int i=0;i<b->quantidade;i++) {
        int escolhido=0; double inicioMelhor=0.0, fimMelhor=INFINITY;
        int recebido=b->tarefas[i].qtdDrones;
        if(recebido==0) recebido=b->inst->clientes[b->tarefas[i].cliente].minDrones;
        for(int k=1;k<=b->frota;k++) {
            if(!guloso && k!=recebido) continue;
            double duracao=duracaoConfiguracaoBB(b,i,k);
            if(!isfinite(duracao)) continue;
            double inicio=primeiroInicioPerfil(perfil,b->frota,duracao,k);
            if(inicio+duracao<fimMelhor) {
                escolhido=k; inicioMelhor=inicio; fimMelhor=inicio+duracao;
            }
        }
        if(!escolhido || !isfinite(fimMelhor)) {perfil->quantidade=0; return;}
        b->parcial[i]=(MissaoDrone){b->tarefas[i].cliente,escolhido,inicioMelhor,fimMelhor};
        ocuparPerfilDrone(perfil,inicioMelhor,fimMelhor,escolhido);
        maximo=fmax(maximo,fimMelhor);
    }
    if(maximo<b->limiteSuperior) {
        b->limiteSuperior=maximo;
        memcpy(b->melhor,b->parcial,(size_t)b->quantidade*sizeof *b->melhor);
    }
    perfil->quantidade=0;
}

double escalonarMissoesDroneBBMeta(const Instancia *inst, MissaoDrone *missoes,
                                 int quantidade, unsigned long long maxNos,
                                 double meta, ResultadoBB *resultado)
{
    if(resultado) *resultado=(ResultadoBB){.motivo=BB_ERRO};
    if(!inst || inst->n<1 || inst->numDrones<0 || quantidade<0 || quantidade>inst->n-1 ||
       !isfinite(meta) || (meta<0.0 && meta!=-1.0)) return INFINITY;
    if(quantidade==0) {
        if(resultado) *resultado=(ResultadoBB){.motivo=BB_OTIMO};
        return 0.0;
    }
    if(!missoes || !inst->clientes || !inst->tempoDrone || inst->numDrones==0) return INFINITY;
    BuscaDroneBB b={0}; b.inst=inst; b.quantidade=quantidade; b.frota=inst->numDrones;
    b.temposInteiros=true; b.maxNos=maxNos; b.meta=meta; b.limiteSuperior=INFINITY;
    size_t n=(size_t)quantidade, capacidadePerfil=2*n+2;
    b.tarefas=malloc(n*sizeof *b.tarefas); b.parcial=malloc(n*sizeof *b.parcial);
    b.melhor=malloc(n*sizeof *b.melhor);
    b.cargaSufixo=calloc(n+1,sizeof *b.cargaSufixo);
    b.perfis=calloc(n+1,sizeof *b.perfis);
    b.eventos=malloc((n+1)*capacidadePerfil*sizeof *b.eventos);
    b.opcoes=malloc(n*(size_t)b.frota*sizeof *b.opcoes);
    if(!b.tarefas || !b.parcial || !b.melhor || !b.cargaSufixo ||
       !b.perfis || !b.eventos || !b.opcoes) goto finalizar;
    for(size_t i=0;i<=n;i++) b.perfis[i].eventos=b.eventos+i*capacidadePerfil;
    for(int i=0;i<quantidade;i++) {
        int cliente=missoes[i].cliente;
        if(cliente<=0 || cliente>=inst->n || !inst->clientes[cliente].elegivelDrone ||
           !inst->tempoDrone[cliente]) goto finalizar;
        for(int j=0;j<i;j++) if(missoes[j].cliente==cliente) goto finalizar;
        int minimo=inst->clientes[cliente].minDrones;
        if(minimo<1 || minimo>b.frota) goto finalizar;
        b.tarefas[i]=missoes[i];
    }
    for(int i=quantidade-1;i>=0;i--) {
        long double menor=INFINITY;
        for(int k=1;k<=b.frota;k++) {
            double duracao=duracaoConfiguracaoBB(&b,i,k);
            if(!isfinite(duracao)) continue;
            if(floor(duracao)!=duracao) b.temposInteiros=false;
            menor=fminl(menor,nextafterl((long double)k*duracao,-INFINITY));
        }
        if(!isfinite(menor)) goto finalizar;
        b.cargaSufixo[i]=nextafterl(menor+b.cargaSufixo[i+1],-INFINITY);
    }
    construirIncumbenteBB(&b,false);
    construirIncumbenteBB(&b,true);
    if(!isfinite(b.limiteSuperior)) goto finalizar;
    if(meta>=0.0 && b.limiteSuperior<=meta) b.metaAtingida=true;
    else explorarDroneBB(&b,0,0.0,0.0L);
    memcpy(missoes,b.melhor,n*sizeof *missoes);
    if(resultado) {
        resultado->motivo=b.metaAtingida ? BB_META_ATINGIDA : b.interrompida ? BB_LIMITE_NOS : BB_OTIMO;
        resultado->nos=b.nos;
        resultado->limiteInferior=(double)limiteCargaDroneBB(&b,0,0.0L);
    }
finalizar:
    free(b.tarefas); free(b.parcial); free(b.melhor); free(b.cargaSufixo);
    free(b.perfis); free(b.eventos); free(b.opcoes);
    return b.limiteSuperior;
}

double escalonarMissoesDroneBBLimitado(const Instancia *inst, MissaoDrone *missoes,
                                     int quantidade, unsigned long long maxNos, bool *otimoProvado)
{
    ResultadoBB resultado;
    double valor=escalonarMissoesDroneBBMeta(inst,missoes,quantidade,maxNos,-1.0,&resultado);
    if(otimoProvado) *otimoProvado=resultado.motivo==BB_OTIMO;
    return valor;
}

double escalonarMissoesDroneBB(const Instancia *inst, MissaoDrone *missoes, int quantidade)
{
    return escalonarMissoesDroneBBMeta(inst,missoes,quantidade,0,-1.0,NULL);
}

int gerarVizinhoAvaliadoConfigurado(const Instancia *inst, const Solucao *atual,
                                Solucao *vizinho, const double pesos[NUM_MOVIMENTOS],
                                TipoMovimento *tipo, bool grupos){
    if (inst == NULL || atual == NULL || vizinho == NULL ||
        !atual->avaliacaoValida || (!atual->cronogramaValido && !atual->branchBoundAplicado && !atual->cronogramaExterno) ||
        atual->prefixosDrone == NULL || vizinho->prefixosDrone == NULL)
        return 0;
    AlteracaoVizinho alteracao;
    if (!gerarVizinhoInterno(inst, atual, vizinho, &alteracao, pesos, tipo, grupos)) return 0;
    if (alteracao.primeiraDrone == atual->numMissoesDrone &&
        vizinho->numMissoesDrone == atual->numMissoesDrone) {
        /* Apenas a rota mudou: preserva todo o cronograma e seu estado. */
        memcpy(vizinho->prefixosDrone, atual->prefixosDrone,
               ((size_t)atual->numMissoesDrone + 1) * sizeof(double));
        vizinho->tempoDrones = atual->tempoDrones;
        vizinho->cronogramaValido = atual->cronogramaValido;
    } else if (!escalonarDesde(inst, vizinho, atual,
                    atual->cronogramaValido ? alteracao.primeiraDrone : 0)) return 0;
    double tempo = atual->tempoCaminhao + alteracao.deltaCaminhao;
    if (vizinho->numMissoesCaminhao == 0) tempo = 0.0;
    if (!isfinite(tempo) || tempo < -1e-9) return 0;
    vizinho->tempoCaminhao = fmax(0.0, tempo);
    vizinho->makespan = fmax(vizinho->tempoCaminhao, vizinho->tempoDrones);
    vizinho->avaliacaoValida = true;
    return 1;
}

int gerarVizinhoAvaliadoPonderado(const Instancia *inst, const Solucao *atual,
                                Solucao *vizinho, const double *pesos, TipoMovimento *tipo){
    return gerarVizinhoAvaliadoConfigurado(inst,atual,vizinho,pesos,tipo,false);
}
int gerarVizinhoAvaliado(const Instancia *inst, const Solucao *atual, Solucao *vizinho){
    return gerarVizinhoAvaliadoPonderado(inst, atual, vizinho, NULL, NULL);
}

int calcularMakespan(const Instancia *inst, Solucao *solucao){

    const double tolerancia = 1e-9;
    if (solucao == NULL)
        return 0;
    solucao->makespan = INFINITY;
    solucao->avaliacaoValida = false;

    if (inst == NULL || inst->n < 1 || inst->numDrones < 0 || solucao->numMissoesCaminhao < 0 || solucao->numMissoesDrone < 0 || solucao->numMissoesCaminhao > inst->n - 1 || solucao->numMissoesDrone != inst->n - 1 - solucao->numMissoesCaminhao || (solucao->numMissoesCaminhao > 0 && (solucao->missaoCaminhao == NULL || inst->tempoCaminhao == NULL)) || (solucao->numMissoesDrone > 0 && (solucao->missaoDrone == NULL || inst->tempoDrone == NULL)))
        return 0;

    bool *visitado = calloc(inst->n, sizeof *visitado);
    if (visitado == NULL)
        return 0;

    double tempoCaminhao = 0.0, tempoDrones = 0.0;
    
    int anterior = 0;
    
    //Apenas somar o tempo do caminhao da forma mais padrao de todas
    for (int i = 0; i < solucao->numMissoesCaminhao; i++) {

        int cliente = solucao->missaoCaminhao[i].n;
        
        if (cliente <= 0 || cliente >= inst->n || visitado[cliente] || inst->tempoCaminhao[anterior] == NULL) goto invalida;
        
            double trecho = inst->tempoCaminhao[anterior][cliente];
        
        if (!isfinite(trecho) || trecho < 0.0) goto invalida;
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
        
        if (m->cliente <= 0 || m->cliente >= inst->n || visitado[m->cliente] || m->qtdDrones < 1 || m->qtdDrones > inst->numDrones || !isfinite(m->tempoInicio) || !isfinite(m->tempoFim) || m->tempoInicio < 0.0 || m->tempoFim < m->tempoInicio || inst->tempoDrone[m->cliente] == NULL){
            goto invalida;
        }
        
        double duracao = inst->tempoDrone[m->cliente][m->qtdDrones];
        
        if (!isfinite(duracao) || duracao < 0.0 || fabs((m->tempoFim - m->tempoInicio) - duracao) > tolerancia * fmax(1.0, duracao)){
            goto invalida;
        }
        visitado[m->cliente] = true;

        //pego o maior tempoDrones
        //Não é o escalonamento, estou apenas pegando o maior tempo fim
        //Vou em cada missao e pego o maior tempo fim
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
    solucao->tempoCaminhao = tempoCaminhao;
    solucao->tempoDrones = tempoDrones;
    solucao->makespan = fmax(tempoCaminhao, tempoDrones);
    solucao->avaliacaoValida = true;
    free(visitado);
    return 1;

invalida:
    free(visitado);
    return 0;
}

/* Reconstrucoes trabalham com cobertura parcial apenas nestas funcoes privadas.
 * Nenhuma solucao parcial e publicada ao SA ou passada a calcularMakespan. */
static void copiarDecisoesCV(const Solucao *a, Solucao *b)
{
    b->numMissoesCaminhao=a->numMissoesCaminhao;
    b->numMissoesDrone=a->numMissoesDrone;
    memcpy(b->missaoCaminhao,a->missaoCaminhao,(size_t)a->numMissoesCaminhao*sizeof(Missao));
    memcpy(b->missaoDrone,a->missaoDrone,(size_t)a->numMissoesDrone*sizeof(MissaoDrone));
    b->avaliacaoValida=b->cronogramaValido=b->branchBoundAplicado=false;
    b->cronogramaExterno=false;
}

static int avaliarParcialCV(const Instancia *inst, Solucao *s)
{
    if(!escalonarMissoesDrone(inst,s)) return 0;
    s->tempoCaminhao=custoRotaMelhoria(inst,s);
    s->makespan=fmax(s->tempoCaminhao,s->tempoDrones);
    return isfinite(s->makespan);
}

static double cargaMinimaClienteCV(const Instancia *inst,int j)
{
    double menor=INFINITY;
    if(clientePodeUsarDrone(inst,j))
        for(int k=inst->clientes[j].minDrones;k<=inst->numDrones;k++) {
            double t=inst->tempoDrone[j][k];
            if(isfinite(t) && t>=0) menor=fmin(menor,k*t);
        }
    return menor;
}

static int melhorPosicaoCaminhaoCV(const Instancia *inst,const Solucao *s,int j,double *delta)
{
    int pos=0,empates=0; *delta=INFINITY;
    for(int p=0;p<=s->numMissoesCaminhao;p++) {
        int a=p?s->missaoCaminhao[p-1].n:0;
        int b=p<s->numMissoesCaminhao?s->missaoCaminhao[p].n:0;
        double d=tempoTrecho(inst,a,j)+tempoTrecho(inst,j,b)-tempoTrecho(inst,a,b);
        if(d<*delta-1e-9) {*delta=d;pos=p;empates=1;}
        else if(fabs(d-*delta)<1e-9 && rand()%++empates==0) pos=p;
    }
    return pos;
}

static void inserirCaminhaoCV(Solucao *s,int j,int pos)
{
    memmove(s->missaoCaminhao+pos+1,s->missaoCaminhao+pos,
            (size_t)(s->numMissoesCaminhao-pos)*sizeof(Missao));
    s->missaoCaminhao[pos].n=j; s->numMissoesCaminhao++;
}

static int menorCargaKCV(const Instancia *inst,int j)
{
    int escolhido=0;double menor=INFINITY;
    for(int k=inst->clientes[j].minDrones;k<=inst->numDrones;k++) {
        double t=inst->tempoDrone[j][k];
        if(isfinite(t) && t>=0 && k*t<menor) {menor=k*t;escolhido=k;}
    }
    return escolhido;
}

/* destino: -1 livre, 0 caminhao, 1 drones. Compara makespan e soma de tempos.
 * A posicao aleatoria e os desempates mantem caminhos diferentes entre sementes. */
static int inserirReconstrucaoCV(const Instancia *inst,Solucao *s,Solucao *teste,
                                Solucao *melhor,int j,int destino)
{
    double valor=INFINITY,soma=INFINITY;
    if(destino!=1) {
        double delta;int p=melhorPosicaoCaminhaoCV(inst,s,j,&delta);
        copiarDecisoesCV(s,melhor);inserirCaminhaoCV(melhor,j,p);
        if(!avaliarParcialCV(inst,melhor)) return 0;
        valor=melhor->makespan;soma=melhor->tempoCaminhao+melhor->tempoDrones;
    }
    if(destino!=0 && clientePodeUsarDrone(inst,j)) {
        int nd=s->numMissoesDrone;
        int posicoes[3]={0,nd,rand()%(nd+1)};
        for(int h=0;h<3;h++) {
            int pos=posicoes[h];
            if((h==1 && pos==posicoes[0]) || (h==2 && (pos==posicoes[0] || pos==posicoes[1]))) continue;
            for(int k=inst->clientes[j].minDrones;k<=inst->numDrones;k++) {
                double t=inst->tempoDrone[j][k];if(!isfinite(t) || t<0) continue;
                copiarDecisoesCV(s,teste);
                memmove(teste->missaoDrone+pos+1,teste->missaoDrone+pos,(size_t)(nd-pos)*sizeof(MissaoDrone));
                teste->missaoDrone[pos]=(MissaoDrone){j,k,0,0};teste->numMissoesDrone++;
                if(!avaliarParcialCV(inst,teste)) return 0;
                double total=teste->tempoCaminhao+teste->tempoDrones;
                if(teste->makespan<valor-1e-9 ||
                   (fabs(teste->makespan-valor)<1e-9 && (total<soma-1e-9 ||
                    (fabs(total-soma)<1e-9 && rand()%2)))) {
                    copiarDecisoesCV(teste,melhor);valor=teste->makespan;soma=total;
                }
            }
        }
    }
    if(!isfinite(valor)) return 0;
    copiarDecisoesCV(melhor,s);return avaliarParcialCV(inst,s);
}

static void polirRotaReconstrucaoCV(const Instancia *inst,Solucao *s)
{
    /* Passes limitados: custo previsivel por proposta, sem impedir pioras no SA. */
    for(int passe=0;passe<2;passe++) {
        bool mudou=false;
        for(int i=0;i<s->numMissoesCaminhao-1;i++)
            for(int j=i+1;j<s->numMissoesCaminhao;j++)
                if(deltaDoisOpt(inst,s,i,j)<-1e-9) {inverterTrecho(s->missaoCaminhao,i,j);mudou=true;}
        if(!mudou) break;
    }
}

static int prepararMacroCV(const Instancia *inst,const Solucao *a,Solucao *b,
                          Solucao *teste,Solucao *melhor)
{
    if(!inst || inst->n<1 || !a || !b || a==b || !a->avaliacaoValida || !inst->clientes ||
       !b->missaoCaminhao || !b->missaoDrone || !b->prefixosDrone ||
       a->missaoCaminhao==b->missaoCaminhao || a->missaoDrone==b->missaoDrone) return 0;
    Instancia dimensoes={.n=inst->n,.numDrones=inst->numDrones};
    alocarSolucao(&dimensoes,teste);alocarSolucao(&dimensoes,melhor);
    if(!teste->missaoCaminhao || !teste->missaoDrone || !teste->prefixosDrone ||
       !melhor->missaoCaminhao || !melhor->missaoDrone || !melhor->prefixosDrone) return 0;
    copiarDecisoesCV(a,b);return 1;
}

int trocarRecursosCV(const Instancia *inst,const Solucao *atual,Solucao *s)
{
    Solucao teste={0},melhor={0};int ok=0;int *grupo=NULL;
    if(!prepararMacroCV(inst,atual,s,&teste,&melhor)) goto fim;
    grupo=calloc((size_t)inst->n,sizeof *grupo);if(!grupo) goto fim;
    int ancora=-1,total=0;
    for(int i=0;i<s->numMissoesCaminhao;i++) {
        int j=s->missaoCaminhao[i].n;
        if(clientePodeUsarDrone(inst,j) && rand()%++total==0) ancora=j;
    }
    if(ancora<0 || !s->numMissoesDrone) {ok=escalonarMissoesDrone(inst,s)&&calcularMakespan(inst,s);goto fim;}
    /* Metade das propostas prioriza um local que efetivamente encurta a rota;
     * a outra metade conserva a ancora uniforme. */
    if(rand()%2) {
        double economia=0;
        for(int h=0;h<s->numMissoesCaminhao;h++) {
            int x=s->missaoCaminhao[h].n,anterior=0;double rota=0;bool viavel=true;
            if(!clientePodeUsarDrone(inst,x)) continue;
            for(int i=0;i<s->numMissoesCaminhao;i++) {
                int j=s->missaoCaminhao[i].n;
                if(inst->clientes[j].x==inst->clientes[x].x && inst->clientes[j].y==inst->clientes[x].y) {
                    if(!clientePodeUsarDrone(inst,j)) {viavel=false;break;}
                } else {rota+=tempoTrecho(inst,anterior,j);anterior=j;}
            }
            rota+=tempoTrecho(inst,anterior,0);
            double ganho=atual->tempoCaminhao-rota;
            if(viavel && ganho>economia+1e-9) {economia=ganho;ancora=x;}
        }
    }
    /* Um local inteiro pode conter mais clientes que a fracao de um LNS. */
    bool completo=true;
    for(int i=0;i<s->numMissoesCaminhao;i++) {
        int j=s->missaoCaminhao[i].n;
        if(inst->clientes[j].x==inst->clientes[ancora].x && inst->clientes[j].y==inst->clientes[ancora].y) {
            if(!clientePodeUsarDrone(inst,j)) completo=false;
            grupo[j]=1;
        }
    }
    if(!completo) {memset(grupo,0,(size_t)inst->n*sizeof *grupo);grupo[ancora]=1;}
    int tamanhoGrupo=0;for(int j=1;j<inst->n;j++) tamanhoGrupo+=grupo[j];
    if(tamanhoGrupo==1 && rand()%2) {
        int alvoGrupo=2+rand()%3;
        while(tamanhoGrupo<alvoGrupo) {
            int proximo=-1;double distancia=INFINITY;
            for(int i=0;i<s->numMissoesCaminhao;i++) {
                int j=s->missaoCaminhao[i].n;
                if(grupo[j] || !clientePodeUsarDrone(inst,j)) continue;
                double d=fabs(inst->clientes[j].x-inst->clientes[ancora].x)+fabs(inst->clientes[j].y-inst->clientes[ancora].y);
                if(d<distancia || (d==distancia && rand()%2)) {distancia=d;proximo=j;}
            }
            if(proximo<0) break;
            grupo[proximo]=1;tamanhoGrupo++;
        }
    }
    int nc=0;
    double carga=0;
    for(int i=0;i<s->numMissoesCaminhao;i++) {
        int j=s->missaoCaminhao[i].n;
        if(grupo[j]) carga+=cargaMinimaClienteCV(inst,j);
        else s->missaoCaminhao[nc++]=s->missaoCaminhao[i];
    }
    s->numMissoesCaminhao=nc;
    for(int i=0;i<s->numMissoesDrone;i++) carga+=cargaMinimaClienteCV(inst,s->missaoDrone[i].cliente);
    double alvo=fmin(atual->makespan,custoRotaMelhoria(inst,s));
    /* Pelo menos um retorno, depois libera carga suficiente para a meta estimada.
     * 20% de escolhas uniformes evitam uma unica regra de retorno. */
    int retornados=0;
    while(s->numMissoesDrone && (!retornados || carga>inst->numDrones*alvo)) {
        int escolhido=rand()%s->numMissoesDrone;
        if(rand()%5) {
            double maior=-INFINITY;
            for(int i=0;i<s->numMissoesDrone;i++) {
                int j=s->missaoDrone[i].cliente;double delta;
                melhorPosicaoCaminhaoCV(inst,s,j,&delta);
                double ganho=cargaMinimaClienteCV(inst,j)/(1.0+fmax(0,delta));
                if(ganho>maior) {maior=ganho;escolhido=i;}
            }
        }
        int j=s->missaoDrone[escolhido].cliente;double delta;
        int pos=melhorPosicaoCaminhaoCV(inst,s,j,&delta);inserirCaminhaoCV(s,j,pos);
        carga-=cargaMinimaClienteCV(inst,j);
        memmove(s->missaoDrone+escolhido,s->missaoDrone+escolhido+1,
                (size_t)(s->numMissoesDrone-escolhido-1)*sizeof(MissaoDrone));
        s->numMissoesDrone--;retornados++;
        alvo=fmin(atual->makespan,custoRotaMelhoria(inst,s));
    }
    /* A carga minima e um incumbente de reparo, nunca uma poda de quantidades. */
    for(int i=0;i<s->numMissoesDrone;i++)
        s->missaoDrone[i].qtdDrones=menorCargaKCV(inst,s->missaoDrone[i].cliente);
    if(!avaliarParcialCV(inst,s)) goto fim;
    int inicio=rand()%(inst->n-1)+1;
    for(int h=0;h<inst->n-1;h++) {
        int j=1+(inicio-1+h)%(inst->n-1);
        if(grupo[j] && !inserirReconstrucaoCV(inst,s,&teste,&melhor,j,1)) goto fim;
    }
    polirRotaReconstrucaoCV(inst,s);
    ok=escalonarMissoesDrone(inst,s)&&calcularMakespan(inst,s);
fim:
    free(grupo);liberarSolucao(&teste);liberarSolucao(&melhor);return ok;
}

int reconstruirParcialCV(const Instancia *inst,const Solucao *atual,Solucao *s,
                        double minimo,double maximo)
{
    Solucao teste={0},melhor={0};int ok=0;int *removidos=NULL,*lista=NULL;
    if(!isfinite(minimo) || !isfinite(maximo) || minimo<=0 || minimo>maximo || maximo>1) return 0;
    if(!prepararMacroCV(inst,atual,s,&teste,&melhor)) goto fim;
    if(inst->n<=1) {ok=escalonarMissoesDrone(inst,s)&&calcularMakespan(inst,s);goto fim;}
    removidos=calloc((size_t)inst->n,sizeof *removidos);
    lista=malloc((size_t)inst->n*sizeof *lista);if(!removidos || !lista) goto fim;
    double fracao=minimo+(maximo-minimo)*(double)rand()/((double)RAND_MAX+1.0);
    int alvo=(int)ceil(fracao*(inst->n-1));if(alvo<1) alvo=1;
    int modo=rand()%3,ancora=1+rand()%(inst->n-1);
    for(int h=0;h<alvo;h++) {
        int j=0,total=0;double maior=-INFINITY;
        for(int x=1;x<inst->n;x++) if(!removidos[x]) {
            if(!modo) {if(rand()%++total==0) j=x;continue;}
            double valor;
            if(modo==1) valor=-fabs(inst->clientes[x].x-inst->clientes[ancora].x)-fabs(inst->clientes[x].y-inst->clientes[ancora].y);
            else {
                valor=-1;
                for(int i=0;i<atual->numMissoesCaminhao;i++) if(atual->missaoCaminhao[i].n==x) {
                    int a=i?atual->missaoCaminhao[i-1].n:0;
                    int b=i+1<atual->numMissoesCaminhao?atual->missaoCaminhao[i+1].n:0;
                    valor=tempoTrecho(inst,a,x)+tempoTrecho(inst,x,b)-tempoTrecho(inst,a,b);
                }
                for(int i=0;i<atual->numMissoesDrone;i++) if(atual->missaoDrone[i].cliente==x)
                    valor=cargaMinimaClienteCV(inst,x)/fmax(1,inst->numDrones);
            }
            if(valor>maior || (valor==maior && rand()%2)) {maior=valor;j=x;}
        }
        removidos[j]=1;lista[h]=j;
    }
    int nc=0,nd=0;
    for(int i=0;i<s->numMissoesCaminhao;i++) if(!removidos[s->missaoCaminhao[i].n]) s->missaoCaminhao[nc++]=s->missaoCaminhao[i];
    for(int i=0;i<s->numMissoesDrone;i++) if(!removidos[s->missaoDrone[i].cliente]) s->missaoDrone[nd++]=s->missaoDrone[i];
    s->numMissoesCaminhao=nc;s->numMissoesDrone=nd;
    for(int i=alvo-1;i>0;i--) {int j=rand()%(i+1),aux=lista[i];lista[i]=lista[j];lista[j]=aux;}
    if(!avaliarParcialCV(inst,s)) goto fim;
    for(int i=0;i<alvo;i++) if(!inserirReconstrucaoCV(inst,s,&teste,&melhor,lista[i],-1)) goto fim;
    polirRotaReconstrucaoCV(inst,s);
    ok=escalonarMissoesDrone(inst,s)&&calcularMakespan(inst,s);
fim:
    free(removidos);free(lista);liberarSolucao(&teste);liberarSolucao(&melhor);return ok;
}

/* Implementacao privada: conserva os comandos de compilacao existentes. */
#include "reparo_alvo.inc"

