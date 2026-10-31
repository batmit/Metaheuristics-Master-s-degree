#include "SA.h"
#include "cv.h"
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#endif

double relogioSA(void)
{
#ifdef _WIN32
    LARGE_INTEGER valor,frequencia;
    QueryPerformanceCounter(&valor);QueryPerformanceFrequency(&frequencia);
    return (double)valor.QuadPart/(double)frequencia.QuadPart;
#else
    struct timespec t;
    timespec_get(&t,TIME_UTC);
    return (double)t.tv_sec+1e-9*t.tv_nsec;
#endif
}

static void copiarSolucao(const Instancia *,const Solucao *,Solucao *);

/* macro=0 local, 1 troca bidirecional, 2 reconstrucao. O sorteio local
 * continua com probabilidade >=75% e seus pesos permanecem positivos. */
static int proporSA(const Instancia *inst,const Solucao *a,Solucao *b,
                    const ParametrosSA *p,const double *pesos,TipoMovimento *tipo,int *macro)
{
    *macro=0;
    if(p->trocasSimultaneas || p->reconstrucaoParcial) {
        double u=(double)rand()/((double)RAND_MAX+1.0);
        double troca=p->trocasSimultaneas?p->probTroca:0;
        if(u<troca) {*macro=1;return trocarRecursosCV(inst,a,b);}
        if(p->reconstrucaoParcial && u<troca+p->probReconstrucao) {
            *macro=2;return reconstruirParcialCV(inst,a,b,p->fracaoMinima,p->fracaoMaxima);
        }
    }
    return gerarVizinhoAvaliadoConfigurado(inst,a,b,pesos,tipo,p->movimentosGrupo);
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
        .maxNosBB = 10000,
        .movimentosGrupo = false,
        .probTroca=0.01,.probReconstrucao=0.01,
        .fracaoMinima=0.05,.fracaoMaxima=0.15,
        .folgaElite=0.10,.niveisSemMelhoraRetomada=20,.maxRetomadas=3,
        .reaquecimento=0.15,
        .niveisSemMelhoraReparo=12,.larguraCadeia=6,.profundidadeCadeia=4,
        .clientesLivresCP=24,.maxChamadasCP=2,
        .segundosReparo=.06,.segundosCP=2.5,.fracaoTempoCP=.20,
#ifdef _WIN32
        .pythonCP=".venv-cpsat/Scripts/python.exe",
#else
        .pythonCP=".venv-cpsat/bin/python",
#endif
        .scriptCP="reparo_cpsat.py"
    };
}

ParametrosSA parametrosSAExploracao(void)
{
    ParametrosSA p=parametrosSAPadrao();
    p.trocasSimultaneas=p.reconstrucaoParcial=p.calibracaoMista=p.elitesDiversas=true;
    p.resfriamentoPorTempo=true;
    p.movimentosGrupo=true;p.gamma=0.8;p.limiteSegundos=30;
    return p;
}

ParametrosSA parametrosSAFoco165(void)
{
    ParametrosSA p=parametrosSAExploracao();
    p.reparoDirigido=p.cadeiasEjecao=p.filtrosReparo=p.reparoCP=true;
    return p;
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
        parametros->niveisSemMelhoraBB > 0 && parametros->maxNosBB > 0 &&
        isfinite(parametros->probTroca) && parametros->probTroca>=0 &&
        isfinite(parametros->probReconstrucao) && parametros->probReconstrucao>=0 &&
        parametros->probTroca+parametros->probReconstrucao<=0.25 &&
        isfinite(parametros->fracaoMinima) && isfinite(parametros->fracaoMaxima) &&
        parametros->fracaoMinima>0 && parametros->fracaoMinima<=parametros->fracaoMaxima && parametros->fracaoMaxima<=1 &&
        isfinite(parametros->folgaElite) && parametros->folgaElite>=0 &&
        parametros->niveisSemMelhoraRetomada>0 && parametros->maxRetomadas>=0 &&
        isfinite(parametros->reaquecimento) && parametros->reaquecimento>0 && parametros->reaquecimento<=1 &&
        (!parametros->reparoDirigido || (parametros->niveisSemMelhoraReparo>0 &&
          parametros->larguraCadeia>=1 && parametros->larguraCadeia<=16 &&
          parametros->profundidadeCadeia>=1 && parametros->profundidadeCadeia<=6 &&
          isfinite(parametros->segundosReparo) && parametros->segundosReparo>0)) &&
        (!parametros->reparoCP || (parametros->reparoDirigido && parametros->clientesLivresCP>=1 &&
          parametros->clientesLivresCP<=64 && parametros->maxChamadasCP>=1 &&
          isfinite(parametros->segundosCP) && parametros->segundosCP>0 && parametros->segundosCP<=3600 &&
          isfinite(parametros->fracaoTempoCP) && parametros->fracaoTempoCP>0 && parametros->fracaoTempoCP<=.5 &&
          parametros->pythonCP && *parametros->pythonCP && parametros->scriptCP && *parametros->scriptCP)) &&
        isfinite(parametros->limiteSegundos) && parametros->limiteSegundos>=0;
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

    double inicio=relogioSA();
    if(parametros->calibracaoMista) {
        double *deltas=malloc((size_t)parametros->saMax*sizeof *deltas);
        if(!deltas) goto finalizar;
        int n=0,passos=inst->n<100?2*inst->n:200;
        /* Caminhada de aquecimento em copia, sem BB nem mudancas na referencia. */
        for(int i=0;i<passos;i++) {
            TipoMovimento tipo;int macro;
            if(parametros->limiteSegundos>0 && relogioSA()-inicio>=parametros->limiteSegundos) break;
            if(!proporSA(inst,&base,&candidata,parametros,NULL,&tipo,&macro)) {free(deltas);goto finalizar;}
            if(candidata.makespan<=base.makespan || rand()%5==0) copiarSolucao(inst,&candidata,&base);
        }
        for(int i=0;i<parametros->saMax;i++) {
            TipoMovimento tipo;int macro;
            if(parametros->limiteSegundos>0 && relogioSA()-inicio>=parametros->limiteSegundos) break;
            if(!proporSA(inst,&base,&candidata,parametros,NULL,&tipo,&macro)) {free(deltas);goto finalizar;}
            double d=candidata.makespan-base.makespan;
            if(d>0) deltas[n++]=d;
            if(d<=0 || rand()%5==0) copiarSolucao(inst,&candidata,&base);
        }
        double t=parametros->tempInicial;
        if(n) {
            double maior=0;
            for(int i=0;i<n;i++) maior=fmax(maior,deltas[i]);
            double baixo=0,alto=-maior/log(parametros->gamma);
            for(int it=0;it<64;it++) {
                double meio=(baixo+alto)/2,media=0;
                for(int i=0;i<n;i++) media+=exp(-deltas[i]/meio);
                if(media/n<parametros->gamma) baixo=meio;else alto=meio;
            }
            t=fmax(alto,nextafter(parametros->tempFinal,INFINITY));
        }
        free(deltas);*temperatura=t;sucesso=1;goto finalizar;
    }

    double atual = parametros->tempInicial;
    /* Limite defensivo para beta muito proximo de 1 e metas nao atingidas. */
    const int maxNiveis = 10000;
    for (int nivel = 0; nivel < maxNiveis; nivel++) {
        int aceitos = 0;
        for (int iteracao = 0; iteracao < parametros->saMax; iteracao++) {
            if(parametros->limiteSegundos>0 && relogioSA()-inicio>=parametros->limiteSegundos) {
                *temperatura=atual;sucesso=1;goto finalizar;
            }
            /* Cada teste parte da mesma base; nunca se aceita um movimento nela. */
            if (!gerarVizinhoAvaliado(inst,&base,&candidata))
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
    destino->cronogramaExterno = origem->cronogramaExterno;
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
        if (candidata->makespan >= melhores[destino].makespan) return 0;
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

int distanciaAtribuicaoSA(const Instancia *inst,const Solucao *a,const Solucao *b)
{
    if(!inst || !a || !b || inst->n<1) return -1;
    unsigned char *modal=calloc((size_t)inst->n,1);if(!modal) return -1;
    for(int i=0;i<a->numMissoesDrone;i++) {
        int j=a->missaoDrone[i].cliente;if(j<=0 || j>=inst->n) {free(modal);return -1;}modal[j]^=1;
    }
    for(int i=0;i<b->numMissoesDrone;i++) {
        int j=b->missaoDrone[i].cliente;if(j<=0 || j>=inst->n) {free(modal);return -1;}modal[j]^=1;
    }
    int distancia=0;for(int j=1;j<inst->n;j++) distancia+=modal[j];
    free(modal);return distancia;
}

int atualizarEliteDiversaSA(const Instancia *inst,const Solucao *c,Solucao melhores[NUM_MELHORES_SA],
                          int *quantidade,double folga)
{
    if(!inst || !c || !melhores || !quantidade || *quantidade<0 || *quantidade>NUM_MELHORES_SA ||
       !c->avaliacaoValida || !isfinite(c->makespan) || !isfinite(folga) || folga<0) return 0;
    int q=*quantidade,destino=q;
    if(q) {
        for(int i=0;i<q;i++) {
            int d=distanciaAtribuicaoSA(inst,c,&melhores[i]);
            if(d<0) return 0;
            if(!d) {
                if(c->makespan>melhores[i].makespan ||
                   (c->makespan==melhores[i].makespan &&
                    c->tempoCaminhao+c->tempoDrones>=melhores[i].tempoCaminhao+melhores[i].tempoDrones)) return 0;
                copiarSolucao(inst,c,&melhores[i]);ordenarEliteSA(melhores,q);
                while(*quantidade>1 && melhores[*quantidade-1].makespan>
                      melhores[0].makespan+fmax(1.0,melhores[0].makespan)*folga) (*quantidade)--;
                return 1;
            }
        }
        if(c->makespan>melhores[0].makespan+fmax(1.0,melhores[0].makespan)*folga) return 0;
        if(q==NUM_MELHORES_SA) {
            /* Nunca substitui o slot zero por uma solucao pior. */
            destino=q-1;int distancia=inst->n;
            for(int i=1;i<q;i++) {
                int d=distanciaAtribuicaoSA(inst,c,&melhores[i]);
                if(d<0) return 0;
                if(d<distancia || (d==distancia && melhores[i].makespan>melhores[destino].makespan)) {distancia=d;destino=i;}
            }
            if(c->makespan>=melhores[destino].makespan) {
                int antiga=inst->n;
                for(int j=0;j<q;j++) if(j!=destino) {
                    int d=distanciaAtribuicaoSA(inst,&melhores[destino],&melhores[j]);
                    if(d<0) return 0;
                    if(d<antiga) antiga=d;
                }
                int nova=inst->n;
                for(int j=0;j<q;j++) if(j!=destino) {
                    int d=distanciaAtribuicaoSA(inst,c,&melhores[j]);
                    if(d<0) return 0;
                    if(d<nova) nova=d;
                }
                if(nova<=antiga) return 0;
            }
        }
    }
    copiarSolucao(inst,c,&melhores[destino]);
    if(q<NUM_MELHORES_SA) (*quantidade)++;
    ordenarEliteSA(melhores,*quantidade);
    /* Ao melhorar o melhor, retira slots que ficaram fora da tolerancia. */
    while(*quantidade>1 && melhores[*quantidade-1].makespan>
          melhores[0].makespan+fmax(1.0,melhores[0].makespan)*folga) (*quantidade)--;
    return 1;
}

static int guardarEliteSA(const Instancia *inst,const Solucao *c,Solucao *elite,int *q,const ParametrosSA *p)
{
    return p->elitesDiversas?atualizarEliteDiversaSA(inst,c,elite,q,p->folgaElite):
        atualizarMelhoresSolucoesSA(inst,c,elite,q);
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
        double tempo=escalonarMissoesDroneBBMeta(inst,refinada.missaoDrone,refinada.numMissoesDrone,
                                               maxNos,refinada.tempoCaminhao,&resultado);
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

static double temperaturaNoPrazoSA(const ParametrosSA *p,double decorrido,
                                  double inicioReaquecimento,double reaquecida)
{
    double progresso=fmin(1.0,fmax(0.0,decorrido/p->limiteSegundos));
    double t=p->tempInicial*exp(log(p->tempFinal/p->tempInicial)*progresso);
    if(inicioReaquecimento>=0 && inicioReaquecimento<p->limiteSegundos && reaquecida>p->tempFinal) {
        double fracao=fmin(1.0,fmax(0.0,(decorrido-inicioReaquecimento)/(p->limiteSegundos-inicioReaquecimento)));
        t=fmax(t,reaquecida*exp(log(p->tempFinal/reaquecida)*fracao));
    }
    return fmax(p->tempFinal,t);
}

int simulatedAnnealing(const Instancia *inst, Solucao *solucao, const ParametrosSA *parametros){

    if (!validarParametrosSA(parametros) || inst == NULL || solucao == NULL || inst->n < 1 || inst->numDrones < 0 || solucao->missaoCaminhao == NULL || solucao->missaoDrone == NULL)
        return 0;

    int nc = solucao->numMissoesCaminhao, nd = solucao->numMissoesDrone;
    double inicioBusca=relogioSA();
    EstatisticasSA local={0};
    EstatisticasSA *estat=parametros->estatisticas?parametros->estatisticas:&local;
    *estat=(EstatisticasSA){.temperaturaInicial=parametros->tempInicial};
    
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
    guardarEliteSA(inst,&atual,melhores,&quantidadeMelhores,parametros);
    
    double temperatura = parametros->tempInicial;
    double inicioReaquecimento=-1,temperaturaReaquecida=0;
    PesosMovimentosSA pesos = inicializarPesosMovimentosSA();
    int niveisSemMelhora = 0;
    int estagnacaoRetomada=0;
    int estagnacaoReparo=0;
    bool cpDisponivel=true;
    double proximoCP=parametros->limiteSegundos>0?.4*parametros->limiteSegundos:0;
    
    //enquanto a temperatura for maior do que a final e existir movimento
    while ((temperatura > parametros->tempFinal || parametros->limiteSegundos>0) && existeMovimento(inst, &atual)) {
        if(parametros->limiteSegundos>0 && relogioSA()-inicioBusca>=parametros->limiteSegundos) {
            estat->terminouPorTempo=true;break;
        }
        double melhorInicioNivel = melhores[0].makespan;
        
        //Dentro do SAmax
        for (int iteracao = 0; iteracao < parametros->saMax; iteracao++) {
            if(parametros->limiteSegundos>0 && relogioSA()-inicioBusca>=parametros->limiteSegundos) {
                estat->terminouPorTempo=true;break;
            }
            
            //Aqui eu gero vizinho, escalono essa mudança e calculo o makespan
            TipoMovimento movimento;
            int macro;
            if (!proporSA(inst,&atual,&candidata,parametros,
                    parametros->pesosAdaptativos?pesos.pesos:NULL,&movimento,&macro))
                goto finalizar;
            estat->avaliacoes++;
            estat->trocas+=macro==1;estat->reconstrucoes+=macro==2;
            if(parametros->resfriamentoPorTempo && parametros->limiteSegundos>0)
                temperatura=temperaturaNoPrazoSA(parametros,relogioSA()-inicioBusca,inicioReaquecimento,temperaturaReaquecida);

            double delta = candidata.makespan - atual.makespan;
            
            int aceitar = delta <= 0.0;
            
            if (!aceitar) {
                double sorteio = (double)rand() / ((double)RAND_MAX + 1.0);
                aceitar = sorteio < exp(-delta / temperatura);
            }
            if (parametros->pesosAdaptativos && !macro) {
                pesos.tentativas[movimento]++;
                if (aceitar && candidata.makespan < melhores[0].makespan)
                    pesos.pontos[movimento] += 5.0;
                else if (delta < 0.0)
                    pesos.pontos[movimento] += 3.0;
                else if (aceitar && delta > 0.0)
                    pesos.pontos[movimento] += 1.0;
            }
            if(aceitar && macro) {
                estat->trocasAceitas+=macro==1;estat->reconstrucoesAceitas+=macro==2;
                estat->melhoriasMacro+=candidata.makespan<melhores[0].makespan;
            }
            /* Guarda candidatas boas mesmo quando o SA rejeita a transicao. */
            guardarEliteSA(inst,&candidata,melhores,&quantidadeMelhores,parametros);
            if (aceitar) {
                /* Troca apenas a propriedade dos buffers internos, sem copia rasa
                 * entre a solucao do chamador e as solucoes de trabalho. */
                Solucao auxiliar = atual;
                atual = candidata;
                candidata = auxiliar;
            }
        }

        if(estat->terminouPorTempo || (parametros->limiteSegundos>0 && relogioSA()-inicioBusca>=parametros->limiteSegundos)) {
            estat->terminouPorTempo=true;break;
        }

        if(parametros->reparoDirigido) {
            if(melhores[0].makespan<melhorInicioNivel)estagnacaoReparo=0;
            else estagnacaoReparo++;
            if(estagnacaoReparo>=parametros->niveisSemMelhoraReparo) {
                double elapsed=relogioSA()-inicioBusca;
                double restante=parametros->limiteSegundos>0?parametros->limiteSegundos-elapsed:INFINITY;
                double nativeBudget=parametros->limiteSegundos>0?.15*parametros->limiteSegundos:INFINITY;
                if(restante>.005 && estat->reparo.segundosNativo<nativeBudget) {
                    int r=repararComMetaCV(inst,&melhores[0],melhores[0].makespan,&candidata,
                          parametros->cadeiasEjecao,parametros->filtrosReparo,
                          parametros->larguraCadeia,parametros->profundidadeCadeia,
                          fmin(parametros->segundosReparo,fmin(restante,nativeBudget-estat->reparo.segundosNativo)),&estat->reparo);
                    if(r<0)goto finalizar;
                    if(r){guardarEliteSA(inst,&candidata,melhores,&quantidadeMelhores,parametros);copiarSolucao(inst,&melhores[0],&atual);}
                }
                elapsed=relogioSA()-inicioBusca;
                restante=parametros->limiteSegundos>0?parametros->limiteSegundos-elapsed:INFINITY;
                double cpBudget=parametros->limiteSegundos>0?parametros->fracaoTempoCP*parametros->limiteSegundos:INFINITY;
                double janela=fmin(parametros->segundosCP,fmin(restante,cpBudget-estat->reparo.segundosCP));
                if(parametros->reparoCP&&cpDisponivel&&elapsed>=proximoCP&&janela>=.2&&
                   estat->reparo.chamadasCP<(unsigned)parametros->maxChamadasCP) {
                    int r=repararCPSATCV(inst,&melhores[0],melhores[0].makespan,&candidata,
                          parametros->clientesLivresCP,janela,(unsigned)rand(),
                          parametros->pythonCP,parametros->scriptCP,&estat->reparo);
                    if(r<0){fprintf(stderr,"Reparo CP-SAT indisponivel ou certificado invalido; verificar Python/script/dependencias.\n");cpDisponivel=false;}
                    if(r>0){guardarEliteSA(inst,&candidata,melhores,&quantidadeMelhores,parametros);copiarSolucao(inst,&melhores[0],&atual);}
                    proximoCP=relogioSA()-inicioBusca+fmax(parametros->segundosCP,parametros->limiteSegundos*.2);
                }
                estagnacaoReparo=0;
            }
        }
        if (melhores[0].makespan < melhorInicioNivel) niveisSemMelhora = 0;
        else niveisSemMelhora++;
        if (parametros->bbPeriodico && niveisSemMelhora >= parametros->niveisSemMelhoraBB) {
            double antes = melhores[0].makespan;
            int refinou = refinarEliteComHistoricoSA(inst, melhores, &quantidadeMelhores,
                                                parametros->maxNosBB, &historicoBB);
            if (refinou < 0) goto finalizar;
            if (refinou && melhores[0].makespan < antes)
                copiarSolucao(inst, &melhores[0], &atual);
            if(parametros->elitesDiversas)
                while(quantidadeMelhores>1 && melhores[quantidadeMelhores-1].makespan>
                      melhores[0].makespan+fmax(1.0,melhores[0].makespan)*parametros->folgaElite) quantidadeMelhores--;
            niveisSemMelhora = 0;
        }
        if(parametros->limiteSegundos>0 && relogioSA()-inicioBusca>=parametros->limiteSegundos) {
            estat->terminouPorTempo=true;break;
        }
        if(melhores[0].makespan<melhorInicioNivel) estagnacaoRetomada=0;
        else estagnacaoRetomada++;
        if(parametros->elitesDiversas && estagnacaoRetomada>=parametros->niveisSemMelhoraRetomada &&
           parametros->maxRetomadas>0 && (parametros->limiteSegundos>0 ||
           estat->retomadas<(unsigned long long)parametros->maxRetomadas)) {
            int opcoes[NUM_MELHORES_SA],q=0;
            for(int i=0;i<quantidadeMelhores;i++)
                if(distanciaAtribuicaoSA(inst,&atual,&melhores[i])>0) opcoes[q++]=i;
            int escolhido=q?opcoes[rand()%q]:rand()%quantidadeMelhores;
            if(distanciaAtribuicaoSA(inst,&atual,&melhores[escolhido])>0) estat->retomadasDeOutraAtribuicao++;
            copiarSolucao(inst,&melhores[escolhido],&atual);
            if(parametros->reconstrucaoParcial) {
                if(!reconstruirParcialCV(inst,&atual,&candidata,parametros->fracaoMinima,parametros->fracaoMaxima)) goto finalizar;
                guardarEliteSA(inst,&candidata,melhores,&quantidadeMelhores,parametros);
                Solucao aux=atual;atual=candidata;candidata=aux;
            }
            temperatura=fmax(temperatura,parametros->tempInicial*parametros->reaquecimento);
            inicioReaquecimento=relogioSA()-inicioBusca;temperaturaReaquecida=temperatura;
            estat->retomadas++;estagnacaoRetomada=0;
        }
        if (parametros->pesosAdaptativos) atualizarPesosMovimentosSA(&pesos, parametros);
        double proxima = temperatura * parametros->alpha;
        
        if (proxima >= temperatura)
            goto finalizar;
        
        temperatura = parametros->limiteSegundos>0?fmax(parametros->tempFinal,proxima):proxima;
    }
    /* Uma verificacao completa ao final tambem elimina deriva numerica dos deltas. */
    if (!calcularMakespan(inst, &melhores[0])) goto finalizar;
    copiarSolucao(inst, &melhores[0], solucao);
    estat->temperaturaFinal=temperatura;
    estat->atribuicoesElite=quantidadeMelhores;
    if(!parametros->elitesDiversas) {
        estat->atribuicoesElite=0;
        for(int i=0;i<quantidadeMelhores;i++) {
            bool nova=true;
            for(int j=0;j<i;j++) if(distanciaAtribuicaoSA(inst,&melhores[i],&melhores[j])==0) nova=false;
            estat->atribuicoesElite+=nova;
        }
    }
    sucesso = 1;

finalizar:
    liberarSolucao(&atual);
    liberarSolucao(&candidata);
    for (int i = 0; i < NUM_MELHORES_SA; i++) liberarSolucao(&melhores[i]);
    liberarHistoricoBBSA(&historicoBB);
    estat->segundosBusca=relogioSA()-inicioBusca;
    return sucesso;
}

int resolverComCalibracaoSA(const Instancia *inst,Solucao *s,const ParametrosSA *p,double *temperatura)
{
    if(!validarParametrosSA(p) || !temperatura) return 0;
    double inicio=relogioSA();ParametrosSA busca=*p;
    if(!calculaTemperaturaInicial(inst,s,p,temperatura)) return 0;
    double gasto=relogioSA()-inicio;
    busca.tempInicial=*temperatura;
    if(p->limiteSegundos>0) {
        double restante=p->limiteSegundos-gasto;
        if(restante<=0) {
            if(p->estatisticas) *p->estatisticas=(EstatisticasSA){.segundosCalibracao=gasto,
                .temperaturaInicial=*temperatura,.terminouPorTempo=true};
            return escalonarMissoesDrone(inst,s)&&calcularMakespan(inst,s);
        }
        busca.limiteSegundos=restante;
    }
    int ok=simulatedAnnealing(inst,s,&busca);
    if(p->estatisticas) p->estatisticas->segundosCalibracao=gasto;
    return ok;
}
