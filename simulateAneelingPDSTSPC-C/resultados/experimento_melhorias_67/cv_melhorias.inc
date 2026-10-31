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

int repararCargaDrone(const Instancia *inst, Solucao *s)
{
    if(!inst || !s || !s->avaliacaoValida) return 0;
    Solucao alternativa={0}; Instancia dimensoes={.n=inst->n,.numDrones=inst->numDrones};
    alocarSolucao(&dimensoes,&alternativa);
    if(!alternativa.missaoCaminhao || !alternativa.missaoDrone || !alternativa.prefixosDrone) {liberarSolucao(&alternativa);return 0;}
    alternativa.numMissoesCaminhao=s->numMissoesCaminhao;
    alternativa.numMissoesDrone=s->numMissoesDrone;
    memcpy(alternativa.missaoCaminhao,s->missaoCaminhao,(size_t)s->numMissoesCaminhao*sizeof *s->missaoCaminhao);
    memcpy(alternativa.missaoDrone,s->missaoDrone,(size_t)s->numMissoesDrone*sizeof *s->missaoDrone);
    for(int i=0;i<alternativa.numMissoesDrone;i++) {
        int j=alternativa.missaoDrone[i].cliente, escolhido=0; double menor=INFINITY;
        for(int k=inst->clientes[j].minDrones;k<=inst->numDrones;k++) {
            double t=inst->tempoDrone[j][k];
            if(isfinite(t) && t>=0 && k*t<menor) {menor=k*t;escolhido=k;}
        }
        if(!escolhido) {liberarSolucao(&alternativa);return 0;}
        alternativa.missaoDrone[i].qtdDrones=escolhido;
    }
    int ok=escalonarMissoesDrone(inst,&alternativa) && calcularMakespan(inst,&alternativa);
    if(ok && alternativa.tempoDrones<s->tempoDrones) {
        Solucao aux=*s; *s=alternativa; alternativa=aux;
        /* Quantidades mudaram; a chave de ordem no historico continua valida. */
        s->branchBoundAplicado=false;
    }
    liberarSolucao(&alternativa); return ok;
}

int construirSolucaoInicialDiversa(const Instancia *inst, Solucao *s)
{
    if(!construirSolucaoInicial(inst,s)) return 0;
    int modo=rand()%3;
    if(modo==0 || inst->n<3) return escalonarMissoesDrone(inst,s) && calcularMakespan(inst,s);
    bool *v=calloc((size_t)inst->n,sizeof *v);
    if(!v) return 0;
    int anterior=0;
    for(int p=0;p<inst->n-1;p++) {
        int lista[3]={-1,-1,-1}, total=0;
        for(int j=1;j<inst->n;j++) if(!v[j]) {
            int q=total<3?total++:2;
            if(q==2 && lista[2]>=0 && tempoTrecho(inst,anterior,j)>=tempoTrecho(inst,anterior,lista[2])) continue;
            while(q>0 && tempoTrecho(inst,anterior,j)<tempoTrecho(inst,anterior,lista[q-1])) {lista[q]=lista[q-1];q--;}
            lista[q]=j;
        }
        int j=lista[rand()%total];s->missaoCaminhao[p].n=j;v[j]=true;anterior=j;
    }
    free(v);
    bool melhorou;
    do {
        melhorou=false;
        for(int i=0;i<s->numMissoesCaminhao-1 && !melhorou;i++)
            for(int j=i+1;j<s->numMissoesCaminhao;j++) if(deltaDoisOpt(inst,s,i,j)<-1e-9) {
                inverterTrecho(s->missaoCaminhao,i,j);melhorou=true;break;
            }
    } while(melhorou);
    if(modo==2) for(int q=0;q<3;q++) {
        int r=moverGrupoMelhoria(inst,s,true); if(r<0) return 0;
    }
    return escalonarMissoesDrone(inst,s) && calcularMakespan(inst,s);
}
