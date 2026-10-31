static double secundarioSA(const Solucao *s) {return s->tempoCaminhao+s->tempoDrones;}

static int distanciaAtribuicaoSA(const Instancia *inst, const Solucao *a, const Solucao *b)
{
    unsigned char mapa[inst->n]; memset(mapa,0,sizeof mapa);
    for(int i=0;i<a->numMissoesDrone;i++) mapa[a->missaoDrone[i].cliente]^=1;
    for(int i=0;i<b->numMissoesDrone;i++) mapa[b->missaoDrone[i].cliente]^=1;
    int d=0; for(int i=1;i<inst->n;i++) d+=mapa[i]; return d;
}

static int atualizarEliteOpcoesSA(const Instancia *inst, const Solucao *c,
                                  Solucao elite[NUM_MELHORES_SA], int *n, unsigned opcoes)
{
    if(!(opcoes&(SA_FOLGA|SA_DIVERSIDADE))) return atualizarMelhoresSolucoesSA(inst,c,elite,n);
    if(!c->avaliacaoValida || !isfinite(c->makespan)) return 0;
    bool folga=(opcoes&SA_FOLGA)!=0, diversa=(opcoes&SA_DIVERSIDADE)!=0;
    int destino=-1;
    for(int i=0;i<*n;i++) {
        if(mesmasDecisoesSA(c,&elite[i])) {
            elite[i].branchBoundAplicado|=c->branchBoundAplicado;return 0;
        }
        if(diversa && distanciaAtribuicaoSA(inst,c,&elite[i])==0) {
            if(c->makespan<elite[i].makespan ||
               (folga && c->makespan==elite[i].makespan && secundarioSA(c)<secundarioSA(&elite[i]))) destino=i;
            else return 0;
            break;
        }
    }
    if(destino<0 && *n<NUM_MELHORES_SA) destino=(*n)++;
    if(destino<0) {
        int pior=*n-1;
        if(c->makespan<elite[pior].makespan ||
           (folga && c->makespan==elite[pior].makespan && secundarioSA(c)<secundarioSA(&elite[pior]))) destino=pior;
        else if(diversa && c->makespan<=elite[0].makespan+2.0) {
            int nova=inst->n, velha=inst->n, remover=-1;
            for(int i=0;i<*n;i++) {
                int d=distanciaAtribuicaoSA(inst,c,&elite[i]);if(d<nova) nova=d;
                if(i) {
                    int minimo=inst->n;
                    for(int j=0;j<*n;j++) if(i!=j) {
                        d=distanciaAtribuicaoSA(inst,&elite[i],&elite[j]);if(d<minimo) minimo=d;
                    }
                    if(minimo<=velha) {velha=minimo;remover=i;}
                }
            }
            if(nova>velha) destino=remover;
        }
    }
    if(destino<0) return 0;
    copiarSolucao(inst,c,&elite[destino]); ordenarEliteSA(elite,*n);
    if(folga) for(int i=1;i<*n;i++) for(int j=i;j>0 && elite[j].makespan==elite[j-1].makespan &&
        secundarioSA(&elite[j])<secundarioSA(&elite[j-1]);j--) {
        Solucao aux=elite[j];elite[j]=elite[j-1];elite[j-1]=aux;
    }
    return 1;
}
