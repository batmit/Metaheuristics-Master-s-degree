#include "SA.h"
#include "cv.h"
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>
#include <time.h>
typedef struct Diag {
    unsigned long long attempts[NUM_MOVIMENTOS], accepted[NUM_MOVIMENTOS];
    unsigned long long neutral[NUM_MOVIMENTOS], secondary[NUM_MOVIMENTOS];
    unsigned long long lower[NUM_MOVIMENTOS], uphill[NUM_MOVIMENTOS];
    unsigned long long late[NUM_MOVIMENTOS], late_neutral[NUM_MOVIMENTOS];
    unsigned long long global[NUM_MOVIMENTOS], statuses[4], nodes;
    unsigned long long step, last_improvement, equal_rejects;
    unsigned long long bb_triggers, bb_applied, bb_global_gains;
    unsigned long long drone_dominant, truck_dominant, tied;
    double bb_seconds;
} Diag;
static Diag diag;
static int diag_same_assignment(const Solucao *a,const Solucao *b) {
    if(a->numMissoesDrone!=b->numMissoesDrone)return 0;
    for(int i=0;i<a->numMissoesDrone;i++) {
        int found=0;
        for(int j=0;j<b->numMissoesDrone;j++)
            if(a->missaoDrone[i].cliente==b->missaoDrone[j].cliente) {found=1;break;}
        if(!found)return 0;
    }
    return 1;
}
static void diag_array(const char *key,const unsigned long long *values) {
    printf(",\"%s\":[",key);
    for(int i=0;i<NUM_MOVIMENTOS;i++)printf("%s%llu",i?",":"",values[i]);
    printf("]");
}
static void diag_print(const Solucao *elite,int n,const PesosMovimentosSA *weights,
                       const HistoricoBBSA *history) {
    int unique=0;
    for(int i=0;i<n;i++) {
        int duplicate=0;
        for(int j=0;j<i;j++)if(diag_same_assignment(&elite[i],&elite[j]))duplicate=1;
        if(!duplicate)unique++;
    }
    printf("DIAG {\"steps\":%llu,\"last_improvement\":%llu,\"equal_elite_rejections\":%llu,"
           "\"bb_triggers\":%llu,\"bb_applied\":%llu,\"bb_global_gains\":%llu,"
           "\"bb_executions\":%llu,\"bb_reuses\":%llu,\"bb_nodes\":%llu,\"bb_seconds\":%.6f,"
           "\"bb_status\":[%llu,%llu,%llu,%llu],\"final_elite_size\":%d,\"unique_elite_assignments\":%d,"
           "\"truck_dominant\":%llu,\"drone_dominant\":%llu,\"tied\":%llu",
           diag.step,diag.last_improvement,diag.equal_rejects,diag.bb_triggers,
           diag.bb_applied,diag.bb_global_gains,history->execucoes,history->reutilizacoes,
           diag.nodes,diag.bb_seconds,diag.statuses[0],diag.statuses[1],diag.statuses[2],
           diag.statuses[3],n,unique,diag.truck_dominant,diag.drone_dominant,diag.tied);
    diag_array("attempts",diag.attempts);diag_array("accepted",diag.accepted);
    diag_array("neutral",diag.neutral);diag_array("secondary",diag.secondary);
    diag_array("lower",diag.lower);diag_array("uphill",diag.uphill);
    diag_array("late",diag.late);diag_array("late_neutral",diag.late_neutral);
    diag_array("global",diag.global);
    printf(",\"final_weights\":[");
    for(int i=0;i<NUM_MOVIMENTOS;i++)printf("%s%.9f",i?",":"",weights->pesos[i]);
    puts("]}");fflush(stdout);
}


ParametrosSA parametrosSAPadrao(void)
{
    return (ParametrosSA){
        .alpha = 0.99,
        .beta = 1.1,
        .gamma = 0.95,
        .saMax = 500,
        .tempInicial = 10.0,
        .tempFinal = 0.01,
        .pesosAdaptativos = true,
        .taxaAdaptacao = 0.2,
        .pesoMinimo = 0.1,
        .bbPeriodico = true,
        .niveisSemMelhoraBB = 5,
        .maxNosBB = 10000
    };
}

int validarParametrosSA(const ParametrosSA *parametros)
{
    if (parametros == NULL)
        return 0;

    return isfinite(parametros->alpha) &&
        parametros->alpha > 0.0 && parametros->alpha < 1.0 &&
        isfinite(parametros->beta) && parametros->beta > 1.0 &&
        isfinite(parametros->gamma) &&
        parametros->gamma > 0.0 && parametros->gamma < 1.0 &&
        parametros->saMax > 0 &&
        isfinite(parametros->tempInicial) &&
        isfinite(parametros->tempFinal) &&
        parametros->tempFinal > 0.0 &&
        parametros->tempInicial > parametros->tempFinal &&
        isfinite(parametros->taxaAdaptacao) && parametros->taxaAdaptacao > 0.0 &&
        parametros->taxaAdaptacao <= 1.0 && isfinite(parametros->pesoMinimo) &&
        parametros->pesoMinimo > 0.0 && parametros->pesoMinimo <= 1.0 &&
        parametros->niveisSemMelhoraBB > 0 && parametros->maxNosBB > 0;
}

PesosMovimentosSA inicializarPesosMovimentosSA(void)
{
    PesosMovimentosSA estado = {0};
    for (int i = 0; i < NUM_MOVIMENTOS; i++) estado.pesos[i] = 1.0;
    return estado;
}

void atualizarPesosMovimentosSA(PesosMovimentosSA *estado, const ParametrosSA *p)
{
    for (int i = 0; i < NUM_MOVIMENTOS; i++) {
        if (estado->tentativas[i] > 0) {
            double media = estado->pontos[i] / estado->tentativas[i];
            estado->pesos[i] = fmax(p->pesoMinimo,
                (1.0 - p->taxaAdaptacao) * estado->pesos[i] + p->taxaAdaptacao * media);
        }
        estado->pontos[i] = 0.0;
        estado->tentativas[i] = 0;
    }
}

int calculaTemperaturaInicial(const Instancia *inst, const Solucao *referencia, const ParametrosSA *parametros, double *temperatura)
{
    if (!validarParametrosSA(parametros) || inst == NULL || referencia == NULL ||
        temperatura == NULL || inst->n < 1 || inst->numDrones < 0)
        return 0;
    int nc = referencia->numMissoesCaminhao;
    int nd = referencia->numMissoesDrone;
    if (nc < 0 || nc > inst->n - 1 || nd != inst->n - 1 - nc ||
        (nc > 0 && referencia->missaoCaminhao == NULL) ||
        (nd > 0 && referencia->missaoDrone == NULL))
        return 0;

    Solucao base = {0}, candidata = {0};
    /* A alocacao usa apenas n; a instancia original permanece constante. */
    Instancia dimensoes = {.n = inst->n, .numDrones = inst->numDrones};
    alocarSolucao(&dimensoes, &base);
    alocarSolucao(&dimensoes, &candidata);
    int sucesso = 0;
    if (base.missaoCaminhao == NULL || base.missaoDrone == NULL ||
        candidata.missaoCaminhao == NULL || candidata.missaoDrone == NULL || base.prefixosDrone == NULL || candidata.prefixosDrone == NULL)
        goto finalizar;
    base.numMissoesCaminhao = nc;
    base.numMissoesDrone = nd;
    for (int i = 0; i < nc; i++)
        base.missaoCaminhao[i] = referencia->missaoCaminhao[i];
    for (int i = 0; i < nd; i++)
        base.missaoDrone[i] = referencia->missaoDrone[i];
    if (!escalonarMissoesDrone(inst, &base) || !calcularMakespan(inst, &base))
        goto finalizar;

    double atual = parametros->tempInicial;
    /* Limite defensivo para beta muito proximo de 1 e metas nao atingidas. */
    const int maxNiveis = 10000;
    for (int nivel = 0; nivel < maxNiveis; nivel++) {
        int aceitos = 0;
        for (int iteracao = 0; iteracao < parametros->saMax; iteracao++) {
            /* Cada teste parte da mesma base; nunca se aceita um movimento nela. */
            if (!gerarVizinhoAvaliado(inst, &base, &candidata))
                goto finalizar;
            double delta = candidata.makespan - base.makespan;
            if (delta <= 0.0) {
                aceitos++;
            } else {
                double sorteio = (double)rand() / ((double)RAND_MAX + 1.0);
                if (sorteio < exp(-delta / atual))
                    aceitos++;
            }
        }
        if ((double)aceitos >= parametros->gamma * parametros->saMax) {
            *temperatura = atual;
            sucesso = 1;
            break;
        }
        double proxima = atual * parametros->beta;
        if (!isfinite(proxima) || proxima <= atual)
            goto finalizar;
        atual = proxima;
    }

finalizar:
    liberarSolucao(&base);
    liberarSolucao(&candidata);
    return sucesso;
}

/* Os dois destinos sao previamente alocados para a mesma instancia. */
static void copiarSolucao(const Instancia *inst, const Solucao *origem, Solucao *destino)
{
    (void)inst;
    for (int i = 0; i < origem->numMissoesCaminhao; i++)
        destino->missaoCaminhao[i] = origem->missaoCaminhao[i];
    for (int i = 0; i < origem->numMissoesDrone; i++)
        destino->missaoDrone[i] = origem->missaoDrone[i];
    destino->numMissoesCaminhao = origem->numMissoesCaminhao;
    destino->numMissoesDrone = origem->numMissoesDrone;
    destino->makespan = origem->makespan;
    destino->tempoCaminhao = origem->tempoCaminhao;
    destino->tempoDrones = origem->tempoDrones;
    destino->avaliacaoValida = origem->avaliacaoValida;
    destino->cronogramaValido = origem->cronogramaValido;
    destino->branchBoundAplicado = origem->branchBoundAplicado;
    if (origem->prefixosDrone && destino->prefixosDrone)
        memcpy(destino->prefixosDrone, origem->prefixosDrone,
               ((size_t)origem->numMissoesDrone + 1) * sizeof(double));
}

static int mesmasDecisoesSA(const Solucao *a, const Solucao *b)
{
    if (a->numMissoesCaminhao != b->numMissoesCaminhao ||
        a->numMissoesDrone != b->numMissoesDrone) return 0;
    for (int i = 0; i < a->numMissoesCaminhao; i++)
        if (a->missaoCaminhao[i].n != b->missaoCaminhao[i].n) return 0;
    for (int i = 0; i < a->numMissoesDrone; i++)
        if (a->missaoDrone[i].cliente != b->missaoDrone[i].cliente ||
            a->missaoDrone[i].qtdDrones != b->missaoDrone[i].qtdDrones) return 0;
    return 1;
}

static void ordenarEliteSA(Solucao melhores[NUM_MELHORES_SA], int quantidade);

int atualizarMelhoresSolucoesSA(const Instancia *inst, const Solucao *candidata,
                              Solucao melhores[NUM_MELHORES_SA], int *quantidade)
{
    if (inst == NULL || candidata == NULL || melhores == NULL || quantidade == NULL ||
        *quantidade < 0 || *quantidade > NUM_MELHORES_SA ||
        !candidata->avaliacaoValida || !isfinite(candidata->makespan)) return 0;
    for (int i = 0; i < *quantidade; i++) {
        if (!mesmasDecisoesSA(candidata, &melhores[i])) continue;
        bool jaAplicado = melhores[i].branchBoundAplicado || candidata->branchBoundAplicado;
        if (candidata->makespan < melhores[i].makespan) {
            copiarSolucao(inst, candidata, &melhores[i]);
            melhores[i].branchBoundAplicado = jaAplicado;
            ordenarEliteSA(melhores, *quantidade);
            return 1;
        }
        melhores[i].branchBoundAplicado = jaAplicado;
        return 0;
    }
    int destino = *quantidade;
    if (destino == NUM_MELHORES_SA) {
        destino--;
        if (candidata->makespan >= melhores[destino].makespan) { if(candidata->makespan == melhores[destino].makespan)diag.equal_rejects++; return 0; }
    }
    copiarSolucao(inst, candidata, &melhores[destino]);
    if (*quantidade < NUM_MELHORES_SA) (*quantidade)++;
    /* Troca a propriedade de buffers independentes para manter a ordem. */
    while (destino > 0 && melhores[destino].makespan < melhores[destino - 1].makespan) {
        Solucao auxiliar = melhores[destino - 1];
        melhores[destino - 1] = melhores[destino];
        melhores[destino] = auxiliar;
        destino--;
    }
    return 1;
}

static void ordenarEliteSA(Solucao melhores[NUM_MELHORES_SA], int quantidade)
{
    for (int i = 1; i < quantidade; i++) {
        int j = i;
        while (j > 0 && melhores[j].makespan < melhores[j - 1].makespan) {
            Solucao aux = melhores[j]; melhores[j] = melhores[j - 1]; melhores[j - 1] = aux;
            j--;
        }
    }
}

struct EntradaHistoricoBBSA {
    int quantidade;
    MissaoDrone *missoes;
    double tempo;
    unsigned long long maxNos;
    ResultadoBB resultado;
};

void liberarHistoricoBBSA(HistoricoBBSA *h)
{
    if(!h) return;
    for(size_t i=0;i<h->quantidade;i++) free(h->entradas[i].missoes);
    free(h->entradas); *h=(HistoricoBBSA){0};
}

static size_t localizarHistoricoBBSA(const HistoricoBBSA *h, const Solucao *s)
{
    for(size_t i=0;i<h->quantidade;i++) {
        const struct EntradaHistoricoBBSA *e=&h->entradas[i];
        if(e->quantidade!=s->numMissoesDrone) continue;
        int j=0;
        while(j<e->quantidade && e->missoes[j].cliente==s->missaoDrone[j].cliente) j++;
        if(j==e->quantidade) return i;
    }
    return h->quantidade;
}

static bool repetirHistoricoBBSA(const struct EntradaHistoricoBBSA *e,
                                const Solucao *s, unsigned long long maxNos)
{
    if(e->resultado.motivo==BB_OTIMO || e->tempo<=s->tempoCaminhao) return false;
    /* Meta anterior deixou de bastar, ou existe maior orcamento disponivel. */
    if(e->resultado.motivo==BB_META_ATINGIDA) return true;
    return e->maxNos!=0 && (maxNos==0 || maxNos>e->maxNos);
}

static int guardarHistoricoBBSA(HistoricoBBSA *h, size_t indice, const Solucao *s,
                               unsigned long long maxNos, ResultadoBB resultado)
{
    if(indice==h->quantidade) {
        if(h->quantidade==h->capacidade) {
            size_t capacidade=h->capacidade ? 2*h->capacidade : 16;
            void *novas=realloc(h->entradas,capacidade*sizeof *h->entradas);
            if(!novas) return 0;
            h->entradas=novas; h->capacidade=capacidade;
        }
        MissaoDrone *missoes=malloc((size_t)s->numMissoesDrone*sizeof *missoes);
        if(!missoes) return 0;
        h->entradas[indice]=(struct EntradaHistoricoBBSA){.quantidade=s->numMissoesDrone,.missoes=missoes};
        h->quantidade++;
    }
    struct EntradaHistoricoBBSA *e=&h->entradas[indice];
    memcpy(e->missoes,s->missaoDrone,(size_t)s->numMissoesDrone*sizeof *e->missoes);
    e->tempo=s->tempoDrones; e->maxNos=maxNos; e->resultado=resultado;
    return 1;
}

int refinarEliteComHistoricoSA(const Instancia *inst, Solucao melhores[NUM_MELHORES_SA],
                             int *quantidade, unsigned long long maxNos, HistoricoBBSA *h)
{
    if(!inst || !melhores || !quantidade || !h || *quantidade<0 ||
       *quantidade>NUM_MELHORES_SA || (h->inst && h->inst!=inst)) return -1;
    h->inst=inst;
    int elegiveis[NUM_MELHORES_SA], total=0;
    for(int i=0;i<*quantidade;i++) {
        Solucao *s=&melhores[i];
        if(!s->avaliacaoValida || s->tempoDrones<=s->tempoCaminhao) continue;
        size_t indice=localizarHistoricoBBSA(h,s);
        bool repetir=indice<h->quantidade && repetirHistoricoBBSA(&h->entradas[indice],s,maxNos);
        if(!s->branchBoundAplicado || repetir) elegiveis[total++]=i;
    }
    if(total==0) return 0;
    int escolhido=elegiveis[rand()%total];
    Solucao refinada={0}; Instancia dimensoes={.n=inst->n,.numDrones=inst->numDrones};
    alocarSolucao(&dimensoes,&refinada);
    int retorno=-1;
    if(!refinada.missaoCaminhao || !refinada.missaoDrone || !refinada.prefixosDrone) goto finalizar;
    copiarSolucao(inst,&melhores[escolhido],&refinada);
    size_t indice=localizarHistoricoBBSA(h,&refinada);
    bool executar=indice==h->quantidade;
    if(!executar) {
        const struct EntradaHistoricoBBSA *e=&h->entradas[indice];
        executar=repetirHistoricoBBSA(e,&refinada,maxNos);
        if(e->tempo<refinada.tempoDrones) {
            memcpy(refinada.missaoDrone,e->missoes,(size_t)e->quantidade*sizeof *e->missoes);
            if(!escalonarMissoesDrone(inst,&refinada) || !calcularMakespan(inst,&refinada)) goto finalizar;
        }
        h->reutilizacoes++;
    }
    if(executar) {
        ResultadoBB resultado;
        clock_t diag_bb_start=clock();
        double tempo=escalonarMissoesDroneBBMeta(inst,refinada.missaoDrone,refinada.numMissoesDrone,
                                               maxNos,refinada.tempoCaminhao,&resultado);
        diag.bb_seconds+=(double)(clock()-diag_bb_start)/CLOCKS_PER_SEC;
        diag.nodes+=resultado.nos;diag.statuses[resultado.motivo]++;
        if(!isfinite(tempo) || resultado.motivo==BB_ERRO ||
           !escalonarMissoesDrone(inst,&refinada) || !calcularMakespan(inst,&refinada)) goto finalizar;
        if(!guardarHistoricoBBSA(h,indice,&refinada,maxNos,resultado)) goto finalizar;
        h->execucoes++;
    }
    refinada.branchBoundAplicado=true;
    melhores[escolhido].branchBoundAplicado=true;
    if(refinada.makespan<melhores[escolhido].makespan)
        copiarSolucao(inst,&refinada,&melhores[escolhido]);
    ordenarEliteSA(melhores,*quantidade);
    for(int i=0;i<*quantidade;i++) {
        for(int j=i+1;j<*quantidade;) {
            if(!mesmasDecisoesSA(&melhores[i],&melhores[j])) {j++;continue;}
            melhores[i].branchBoundAplicado |= melhores[j].branchBoundAplicado;
            Solucao aux=melhores[j];
            for(int k=j;k<*quantidade-1;k++) melhores[k]=melhores[k+1];
            melhores[--(*quantidade)]=aux;
        }
    }
    retorno=1;
finalizar:
    liberarSolucao(&refinada); return retorno;
}

int refinarEliteAleatoriaSA(const Instancia *inst, Solucao melhores[NUM_MELHORES_SA],
                           int *quantidade, unsigned long long maxNos)
{
    HistoricoBBSA historico={0};
    int retorno=refinarEliteComHistoricoSA(inst,melhores,quantidade,maxNos,&historico);
    liberarHistoricoBBSA(&historico); return retorno;
}

static int existeMovimento(const Instancia *inst, const Solucao *solucao)
{
    if (solucao->numMissoesCaminhao >= 2 || solucao->numMissoesDrone > 0)
        return 1;
    if (solucao->numMissoesCaminhao == 0 || inst->clientes == NULL ||
        inst->tempoDrone == NULL)
        return 0;
    int cliente = solucao->missaoCaminhao[0].n;
    int k = inst->clientes[cliente].minDrones;
    return inst->clientes[cliente].elegivelDrone && k >= 1 &&
        k <= inst->numDrones && inst->tempoDrone[cliente] != NULL &&
        isfinite(inst->tempoDrone[cliente][k]) && inst->tempoDrone[cliente][k] >= 0.0;
}

int simulatedAnnealing(const Instancia *inst, Solucao *solucao, const ParametrosSA *parametros){
    memset(&diag,0,sizeof diag);

    if (!validarParametrosSA(parametros) || inst == NULL || solucao == NULL || inst->n < 1 || inst->numDrones < 0 || solucao->missaoCaminhao == NULL || solucao->missaoDrone == NULL)
        return 0;

    int nc = solucao->numMissoesCaminhao, nd = solucao->numMissoesDrone;
    
    if (nc < 0 || nc > inst->n - 1 || nd != inst->n - 1 - nc)
        return 0;

    /* Atual, candidata e quatro melhores com buffers independentes. */
    Solucao atual = {0}, candidata = {0};
    Solucao melhores[NUM_MELHORES_SA] = {0};
    int quantidadeMelhores = 0;
    HistoricoBBSA historicoBB = {0};
    
    Instancia dimensoes = {.n = inst->n, .numDrones = inst->numDrones};
    
    alocarSolucao(&dimensoes, &atual);

    alocarSolucao(&dimensoes, &candidata);
    
    for (int i = 0; i < NUM_MELHORES_SA; i++)
        alocarSolucao(&dimensoes, &melhores[i]);
    
    int sucesso = 0;
    
    if (atual.missaoCaminhao == NULL || atual.missaoDrone == NULL || candidata.missaoCaminhao == NULL || candidata.missaoDrone == NULL || atual.prefixosDrone == NULL || candidata.prefixosDrone == NULL){
        goto finalizar;
    }
    for (int i = 0; i < NUM_MELHORES_SA; i++)
        if (melhores[i].missaoCaminhao == NULL || melhores[i].missaoDrone == NULL ||
            melhores[i].prefixosDrone == NULL) goto finalizar;
    copiarSolucao(inst, solucao, &atual);
    
    if (!escalonarMissoesDrone(inst, &atual) || !calcularMakespan(inst, &atual)){
        goto finalizar;
    }
    atualizarMelhoresSolucoesSA(inst, &atual, melhores, &quantidadeMelhores);
    
    double temperatura = parametros->tempInicial;
    PesosMovimentosSA pesos = inicializarPesosMovimentosSA();
    int niveisSemMelhora = 0;
    
    //enquanto a temperatura for maior do que a final e existir movimento
    while (temperatura > parametros->tempFinal && existeMovimento(inst, &atual)) {
        double melhorInicioNivel = melhores[0].makespan;
        
        //Dentro do SAmax
        for (int iteracao = 0; iteracao < parametros->saMax; iteracao++) {
            
            //Aqui eu gero vizinho, escalono essa mudanÃ§a e calculo o makespan
            TipoMovimento movimento;
            if (!gerarVizinhoAvaliadoPonderado(inst, &atual, &candidata,
                    parametros->pesosAdaptativos ? pesos.pesos : NULL, &movimento))
                goto finalizar;

            double delta = candidata.makespan - atual.makespan;
            diag.step++;
            if(atual.tempoCaminhao>atual.tempoDrones)diag.truck_dominant++;
            else if(atual.tempoDrones>atual.tempoCaminhao)diag.drone_dominant++;
            else diag.tied++;

            
            int aceitar = delta <= 0.0;
            
            if (!aceitar) {
                double sorteio = (double)rand() / ((double)RAND_MAX + 1.0);
                aceitar = sorteio < exp(-delta / temperatura);
            }

            diag.attempts[movimento]++;diag.accepted[movimento]+=aceitar;
            diag.neutral[movimento]+=delta==0.0;
            diag.lower[movimento]+=delta<0.0;
            diag.uphill[movimento]+=aceitar && delta>0.0;
            diag.secondary[movimento]+=delta==0.0 &&
                candidata.tempoCaminhao+candidata.tempoDrones<atual.tempoCaminhao+atual.tempoDrones;
            if(temperatura<=1.0) {
                diag.late[movimento]++;diag.late_neutral[movimento]+=delta==0.0;
            }
            if(candidata.makespan<melhores[0].makespan) {
                diag.global[movimento]++;diag.last_improvement=diag.step;
            }
            if (parametros->pesosAdaptativos) {
                pesos.tentativas[movimento]++;
                if (aceitar && candidata.makespan < melhores[0].makespan)
                    pesos.pontos[movimento] += 5.0;
                else if (delta < 0.0)
                    pesos.pontos[movimento] += 3.0;
                else if (aceitar && delta > 0.0)
                    pesos.pontos[movimento] += 1.0;
            }
            /* Guarda candidatas boas mesmo quando o SA rejeita a transicao. */
            atualizarMelhoresSolucoesSA(inst, &candidata, melhores, &quantidadeMelhores);
            if (aceitar) {
                /* Troca apenas a propriedade dos buffers internos, sem copia rasa
                 * entre a solucao do chamador e as solucoes de trabalho. */
                Solucao auxiliar = atual;
                atual = candidata;
                candidata = auxiliar;
            }
        }

        if (melhores[0].makespan < melhorInicioNivel) niveisSemMelhora = 0;
        else niveisSemMelhora++;
        if (parametros->bbPeriodico && niveisSemMelhora >= parametros->niveisSemMelhoraBB) {
            diag.bb_triggers++;
            double antes = melhores[0].makespan;
            int refinou = refinarEliteComHistoricoSA(inst, melhores, &quantidadeMelhores,
                                                parametros->maxNosBB, &historicoBB);
            if (refinou < 0) goto finalizar;
            diag.bb_applied+=refinou>0;
            if(melhores[0].makespan<antes) {diag.bb_global_gains++;diag.last_improvement=diag.step;}
            if (refinou && melhores[0].makespan < antes)
                copiarSolucao(inst, &melhores[0], &atual);
            niveisSemMelhora = 0;
        }
        if (parametros->pesosAdaptativos) atualizarPesosMovimentosSA(&pesos, parametros);
        double proxima = temperatura * parametros->alpha;
        
        if (proxima >= temperatura)
            goto finalizar;
        
        temperatura = proxima;
    }
    /* Uma verificacao completa ao final tambem elimina deriva numerica dos deltas. */
    if (!calcularMakespan(inst, &melhores[0])) goto finalizar;
    diag_print(melhores,quantidadeMelhores,&pesos,&historicoBB);
    copiarSolucao(inst, &melhores[0], solucao);
    sucesso = 1;

finalizar:
    liberarSolucao(&atual);
    liberarSolucao(&candidata);
    for (int i = 0; i < NUM_MELHORES_SA; i++) liberarSolucao(&melhores[i]);
    liberarHistoricoBBSA(&historicoBB);
    return sucesso;
}
