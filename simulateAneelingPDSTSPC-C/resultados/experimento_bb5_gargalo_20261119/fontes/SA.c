#include "SA.h"
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif

/* Retorna o tempo de parede em segundos para limitar a busca. */
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

static void copiarSolucao(const Solucao *, Solucao *);

/* Sorteia propostas mistas apenas na calibracao, preservando a escala de T0. */
static int proporCalibracaoSA(const Instancia *inst,const Solucao *a,Solucao *b,
                    const ParametrosSA *p)
{
    if(p->trocasSimultaneas || p->reconstrucaoParcial) {
        double u=(double)rand()/((double)RAND_MAX+1.0);
        double troca=p->trocasSimultaneas?p->probTroca:0;
        if(u<troca) {return trocarRecursosCV(inst,a,b);}
        if(p->reconstrucaoParcial && u<troca+p->probReconstrucao) {
            return reconstruirParcialCV(inst,a,b,p->fracaoMinima,p->fracaoMaxima);
        }
    }
    return gerarVizinhoAvaliadoConfigurado(inst,a,b,NULL,NULL,p->movimentosGrupo);
}

/* Conserva movimentos locais e diversidade com parada natural e macros raros. */
ParametrosSA parametrosSARapido(void)
{
    return (ParametrosSA){
        .alpha=.99, .gamma=.8, .saMax=500, .tempInicial=10, .tempFinal=.01,
        .pesosAdaptativos=true, .taxaAdaptacao=.2, .pesoMinimo=.1,
        .movimentosGrupo=true, .trocasSimultaneas=true, .reconstrucaoParcial=true,
        .probTroca=.01, .probReconstrucao=.01, .fracaoMinima=.05, .fracaoMaxima=.15,
        .folgaElite=.10, .niveisSemMelhoraRetomada=20, .maxRetomadas=2,
        .reaquecimento=.15, .limiteSegundos=30,
        .minimoAvaliacoes=200000, .niveisParada=400,
        .refinoFinalBB=true, .maxNosBB=10000, .segundosPorBB=.02
    };
}

/* Verifica faixas e consistencia dos parametros antes da busca. */
int validarParametrosSA(const ParametrosSA *parametros)
{
    if (parametros == NULL)
        return 0;

    return parametros->estagnacaoRecursoBB>=0 && isfinite(parametros->segundosPorBB) &&
        parametros->segundosPorBB>=0 &&
        parametros->minimoAvaliacoes >= 0 && parametros->niveisParada >= 0 &&
        isfinite(parametros->alpha) &&
        parametros->alpha > 0.0 && parametros->alpha < 1.0 &&
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
        parametros->maxNosBB > 0 &&
        isfinite(parametros->probTroca) && parametros->probTroca>=0 &&
        isfinite(parametros->probReconstrucao) && parametros->probReconstrucao>=0 &&
        parametros->probTroca+parametros->probReconstrucao<=0.25 &&
        isfinite(parametros->fracaoMinima) && isfinite(parametros->fracaoMaxima) &&
        parametros->fracaoMinima>0 && parametros->fracaoMinima<=parametros->fracaoMaxima && parametros->fracaoMaxima<=1 &&
        isfinite(parametros->folgaElite) && parametros->folgaElite>=0 &&
        parametros->niveisSemMelhoraRetomada>0 && parametros->maxRetomadas>=0 &&
        isfinite(parametros->reaquecimento) && parametros->reaquecimento>0 && parametros->reaquecimento<=1 &&
        isfinite(parametros->limiteSegundos) && parametros->limiteSegundos>=0;
}

/* Inicializa os pesos e contadores dos operadores. */
PesosMovimentosSA inicializarPesosMovimentosSA(void)
{
    PesosMovimentosSA estado = {0};
    for (int i = 0; i < NUM_MOVIMENTOS; i++) estado.pesos[i] = 1.0;
    return estado;
}

/* Atualiza os pesos segundo o desempenho dos operadores. */
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

/* Calibra a temperatura usando vizinhos de uma copia da solucao. */
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
    double *deltas=malloc((size_t)parametros->saMax*sizeof *deltas);
    if(!deltas) goto finalizar;
    int n=0,passos=inst->n<100?2*inst->n:200;
    /* Caminhada de aquecimento em copia, sem BB nem mudancas na referencia. */
    for(int i=0;i<passos;i++) {
        if(parametros->limiteSegundos>0 && relogioSA()-inicio>=parametros->limiteSegundos) break;
        if(!proporCalibracaoSA(inst,&base,&candidata,parametros)) {free(deltas);goto finalizar;}
        if(candidata.makespan<=base.makespan || rand()%5==0) copiarSolucao(&candidata,&base);
    }
    for(int i=0;i<parametros->saMax;i++) {
        if(parametros->limiteSegundos>0 && relogioSA()-inicio>=parametros->limiteSegundos) break;
        if(!proporCalibracaoSA(inst,&base,&candidata,parametros)) {free(deltas);goto finalizar;}
        double d=candidata.makespan-base.makespan;
        if(d>0) deltas[n++]=d;
        if(d<=0 || rand()%5==0) copiarSolucao(&candidata,&base);
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
    free(deltas);*temperatura=t;sucesso=1;

finalizar:
    liberarSolucao(&base);
    liberarSolucao(&candidata);
    return sucesso;
}

/* Os dois destinos sao previamente alocados para a mesma instancia. */
static void copiarSolucao(const Solucao *origem, Solucao *destino)
{
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

    if (origem->prefixosDrone && destino->prefixosDrone)
        memcpy(destino->prefixosDrone, origem->prefixosDrone,
               ((size_t)origem->numMissoesDrone + 1) * sizeof(double));
}

/* Ordena a elite por makespan crescente. */
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

/* Conta diferencas de atribuicao dos clientes aos recursos. */
int distanciaAtribuicaoSA(const Instancia *inst,const Solucao *a,const Solucao *b)
{
    if(!inst || !a || !b || inst->n<1) return -1;
    /* Bitsets na pilha cobrem o benchmark; entradas maiores usam um buffer dinamico. */
    size_t palavras = ((size_t)inst->n + 63) / 64;
    unsigned long long local[4] = {0};
    unsigned long long *modal = palavras <= 4 ? local : calloc(palavras, sizeof *modal);
    if (!modal) return -1;
    for (int lado = 0; lado < 2; lado++) {
        const Solucao *s = lado ? b : a;
        for (int i = 0; i < s->numMissoesDrone; i++) {
            int j = s->missaoDrone[i].cliente;
            if (j <= 0 || j >= inst->n) {
                if (modal != local) free(modal);
                return -1;
            }
            modal[j / 64] ^= 1ULL << (j % 64);
        }
    }
    int distancia = 0;
    for (size_t i = 0; i < palavras; i++) {
        unsigned long long bits = modal[i];
        while (bits) { bits &= bits - 1; distancia++; }
    }
    if (modal != local) free(modal);
    return distancia;
}

/* Preserva o melhor e atribuicoes distintas dentro da folga permitida. */
int atualizarEliteDiversaSA(const Instancia *inst,const Solucao *c,Solucao melhores[NUM_MELHORES_SA],
                          int *quantidade,double folga)
{
    if(!inst || !c || !melhores || !quantidade || *quantidade<0 || *quantidade>NUM_MELHORES_SA ||
       !c->avaliacaoValida || !isfinite(c->makespan) || !isfinite(folga) || folga<0) return 0;
    int q=*quantidade,destino=q;
    if(q) {
        if(c->makespan>melhores[0].makespan+fmax(1.0,melhores[0].makespan)*folga) return 0;
        for(int i=0;i<q;i++) {
            int d=distanciaAtribuicaoSA(inst,c,&melhores[i]);
            if(d<0) return 0;
            if(!d) {
                if(c->makespan>melhores[i].makespan ||
                   (c->makespan==melhores[i].makespan &&
                    c->tempoCaminhao+c->tempoDrones>=melhores[i].tempoCaminhao+melhores[i].tempoDrones)) return 0;
                copiarSolucao(c,&melhores[i]);ordenarEliteSA(melhores,q);
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
    copiarSolucao(c,&melhores[destino]);
    if(q<NUM_MELHORES_SA) (*quantidade)++;
    ordenarEliteSA(melhores,*quantidade);
    /* Ao melhorar o melhor, retira slots que ficaram fora da tolerancia. */
    while(*quantidade>1 && melhores[*quantidade-1].makespan>
          melhores[0].makespan+fmax(1.0,melhores[0].makespan)*folga) (*quantidade)--;
    return 1;
}

/* Refina somente o melhor cronograma, sem historico nem escolha de outra elite. */
static int refinarMelhorSA(const Instancia *inst, Solucao *melhor, unsigned long long maxNos)
{
    if (melhor->tempoDrones <= melhor->tempoCaminhao) return 1;
    Solucao refinada = {0};
    Instancia dimensoes = {.n=inst->n, .numDrones=inst->numDrones};
    alocarSolucao(&dimensoes, &refinada);
    int ok = 0;
    if (!refinada.missaoCaminhao || !refinada.missaoDrone || !refinada.prefixosDrone) goto finalizar;
    copiarSolucao(melhor, &refinada);
    ResultadoBB resultado;
    double tempo = escalonarMissoesDroneBBMeta(inst, refinada.missaoDrone,
        refinada.numMissoesDrone, maxNos, refinada.tempoCaminhao, &resultado);
    if (!isfinite(tempo) || resultado.motivo == BB_ERRO ||
        !escalonarMissoesDrone(inst, &refinada) || !calcularMakespan(inst, &refinada)) goto finalizar;
    if (refinada.makespan < melhor->makespan) copiarSolucao(&refinada, melhor);
    ok = 1;
finalizar:
    liberarSolucao(&refinada);
    return ok;
}

/* Verifica se a solucao admite algum operador de vizinhanca. */
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

/* Executa a busca com aceitacao probabilistica e devolve o melhor viavel. */
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
    copiarSolucao(solucao, &atual);
    
    if (!escalonarMissoesDrone(inst, &atual) || !calcularMakespan(inst, &atual)){
        goto finalizar;
    }
    atualizarEliteDiversaSA(inst,&atual,melhores,&quantidadeMelhores,parametros->folgaElite);
    
    double temperatura = parametros->tempInicial;
    PesosMovimentosSA pesos = inicializarPesosMovimentosSA();
    int niveisSemMelhora = 0;
    int estagnacaoRetomada=0;
    int semMelhoraCaminhao=0,semMelhoraDrone=0;
    //enquanto a temperatura for maior do que a final e existir movimento
    while (temperatura > parametros->tempFinal && existeMovimento(inst, &atual)) {
        if(parametros->limiteSegundos>0 && relogioSA()-inicioBusca>=parametros->limiteSegundos) {
            estat->terminouPorTempo=true;break;
        }
        double melhorInicioNivel = melhores[0].makespan;
        
        //Dentro do SAmax
        for (int iteracao = 0; iteracao < parametros->saMax; iteracao++) {
            if(parametros->limiteSegundos>0 && relogioSA()-inicioBusca>=parametros->limiteSegundos) {
                estat->terminouPorTempo=true;break;
            }
            
            double antesCaminhao=atual.tempoCaminhao,antesDrone=atual.tempoDrones;
            //Aqui eu gero vizinho, escalono essa mudança e calculo o makespan
            TipoMovimento movimento;
            if (!gerarVizinhoAvaliadoConfigurado(inst,&atual,&candidata,
                    parametros->pesosAdaptativos?pesos.pesos:NULL,&movimento,parametros->movimentosGrupo))
                goto finalizar;
            estat->avaliacoes++;
            double delta = candidata.makespan - atual.makespan;
            
            int aceitar = delta <= 0.0;
            
            if (!aceitar) {
                double sorteio = (double)rand() / ((double)RAND_MAX + 1.0);
                aceitar = sorteio < exp(-delta / temperatura);
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
            atualizarEliteDiversaSA(inst,&candidata,melhores,&quantidadeMelhores,parametros->folgaElite);
            if (aceitar) {
                /* Troca apenas a propriedade dos buffers internos, sem copia rasa
                 * entre a solucao do chamador e as solucoes de trabalho. */
                Solucao auxiliar = atual;
                atual = candidata;
                candidata = auxiliar;
            }
            if(parametros->estagnacaoRecursoBB>0) {
                semMelhoraCaminhao=atual.tempoCaminhao<antesCaminhao-1e-9?0:semMelhoraCaminhao+1;
                semMelhoraDrone=atual.tempoDrones<antesDrone-1e-9?0:semMelhoraDrone+1;
                for(int recurso=0;recurso<2;recurso++) {
                    int *contador=recurso?&semMelhoraDrone:&semMelhoraCaminhao;
                    if(*contador<parametros->estagnacaoRecursoBB) continue;
                    *contador=0;
                    if(!recurso && parametros->apenasGargaloBBCaminhao &&
                       atual.tempoCaminhao<=atual.tempoDrones+1e-9) {
                        estat->bloqueiosBBCaminhao++;continue;
                    }
                    double inicioBB=relogioSA(),prazo=0;
                    if(parametros->limiteSegundos>0) prazo=inicioBusca+parametros->limiteSegundos;
                    if(prazo>0 && inicioBB>=prazo) break;
                    if(parametros->segundosPorBB>0) {
                        double fim=inicioBB+parametros->segundosPorBB;
                        prazo=prazo>0?fmin(prazo,fim):fim;
                    }
                    ResultadoBB r={0};
                    double antes=recurso?atual.tempoDrones:atual.tempoCaminhao;
                    if(recurso) {
                        double t=escalonarMissoesDroneBBPrazo(inst,atual.missaoDrone,
                            atual.numMissoesDrone,parametros->maxNosBB,-1,prazo,&r);
                        if(!isfinite(t) || r.motivo==BB_ERRO ||
                           !escalonarMissoesDrone(inst,&atual) || !calcularMakespan(inst,&atual)) goto finalizar;
                        estat->chamadasBBDrone++;estat->nosBBDrone+=r.nos;
                        estat->melhoriasBBDrone+=atual.tempoDrones<antes-1e-9;
                        estat->segundosBBDrone+=relogioSA()-inicioBB;
                    } else {
                        if(!otimizarCaminhaoBB(inst,&atual,parametros->maxNosBB,prazo,&r)) goto finalizar;
                        estat->chamadasBBCaminhao++;estat->nosBBCaminhao+=r.nos;
                        estat->melhoriasBBCaminhao+=atual.tempoCaminhao<antes-1e-9;
                        estat->segundosBBCaminhao+=relogioSA()-inicioBB;
                    }
                    atualizarEliteDiversaSA(inst,&atual,melhores,&quantidadeMelhores,parametros->folgaElite);
                }
            }
        }

        if(estat->terminouPorTempo || (parametros->limiteSegundos>0 && relogioSA()-inicioBusca>=parametros->limiteSegundos)) {
            estat->terminouPorTempo=true;break;
        }

        /* Apos cinco niveis estagnados, tenta um unico movimento composto. */
        if (niveisSemMelhora >= 5 &&
            (parametros->trocasSimultaneas || parametros->reconstrucaoParcial)) {
            int troca = parametros->trocasSimultaneas &&
                        (!parametros->reconstrucaoParcial || rand() % 2 == 0);
            int ok = troca ? trocarRecursosCV(inst, &atual, &candidata) :
                reconstruirParcialCV(inst, &atual, &candidata,
                                     parametros->fracaoMinima, parametros->fracaoMaxima);
            if (!ok) goto finalizar;
            estat->avaliacoes++;
            estat->trocas += troca;
            estat->reconstrucoes += !troca;
            double delta = candidata.makespan - atual.makespan;
            int aceitar = delta <= 0 || (double)rand() / ((double)RAND_MAX + 1) < exp(-delta / temperatura);
            estat->melhoriasMacro += aceitar && candidata.makespan < melhores[0].makespan;
            atualizarEliteDiversaSA(inst, &candidata, melhores, &quantidadeMelhores, parametros->folgaElite);
            if (aceitar) {
                estat->trocasAceitas += troca;
                estat->reconstrucoesAceitas += !troca;
                Solucao aux = atual; atual = candidata; candidata = aux;
            }
        }
        if (melhores[0].makespan < melhorInicioNivel) niveisSemMelhora = 0;
        else niveisSemMelhora++;
        if(parametros->limiteSegundos>0 && relogioSA()-inicioBusca>=parametros->limiteSegundos) {
            estat->terminouPorTempo=true;break;
        }
        if(melhores[0].makespan<melhorInicioNivel) estagnacaoRetomada=0;
        else estagnacaoRetomada++;
        if(estagnacaoRetomada>=parametros->niveisSemMelhoraRetomada &&
           estat->retomadas<(unsigned long long)parametros->maxRetomadas) {
            int opcoes[NUM_MELHORES_SA],q=0;
            for(int i=0;i<quantidadeMelhores;i++)
                if(distanciaAtribuicaoSA(inst,&atual,&melhores[i])>0) opcoes[q++]=i;
            int escolhido=q?opcoes[rand()%q]:rand()%quantidadeMelhores;
            if(distanciaAtribuicaoSA(inst,&atual,&melhores[escolhido])>0) estat->retomadasDeOutraAtribuicao++;
            copiarSolucao(&melhores[escolhido],&atual);
            temperatura=fmax(temperatura,parametros->tempInicial*parametros->reaquecimento);
            estat->retomadas++;estagnacaoRetomada=0;
        }
        if (parametros->niveisParada > 0 &&
            niveisSemMelhora >= parametros->niveisParada &&
            estat->avaliacoes >= (unsigned long long)parametros->minimoAvaliacoes) {
            estat->terminouPorEstagnacao = true;
            break;
        }
        if (parametros->pesosAdaptativos) atualizarPesosMovimentosSA(&pesos, parametros);
        double proxima = temperatura * parametros->alpha;
        
        if (proxima >= temperatura)
            goto finalizar;
        
        temperatura = parametros->limiteSegundos>0?fmax(parametros->tempFinal,proxima):proxima;
    }
    /* Uma chamada opcional ao B&B, exclusivamente no melhor e apos terminar o SA. */
    if (parametros->refinoFinalBB &&
        (parametros->limiteSegundos <= 0 || relogioSA()-inicioBusca < parametros->limiteSegundos)) {
        if (!refinarMelhorSA(inst, &melhores[0], parametros->maxNosBB)) goto finalizar;
    }
    /* Uma verificacao completa ao final tambem elimina deriva numerica dos deltas. */
    if (!calcularMakespan(inst, &melhores[0])) goto finalizar;
    copiarSolucao(&melhores[0], solucao);
    estat->temperaturaFinal=temperatura;
    estat->atribuicoesElite=quantidadeMelhores;
    sucesso = 1;

finalizar:
    liberarSolucao(&atual);
    liberarSolucao(&candidata);
    for (int i = 0; i < NUM_MELHORES_SA; i++) liberarSolucao(&melhores[i]);
    estat->segundosBusca=relogioSA()-inicioBusca;
    return sucesso;
}

/* Compartilha o prazo entre calibracao da temperatura e busca. */
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
