#include "cv.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

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

typedef struct BuscaBB {
    const Instancia *inst;
    int n, frota;
    int *clientes;
    int *quantidades;
    bool *usado;
    long double *minCarga;
    PerfilDrone *perfis;
    EventoDrone *eventos;
    MissaoDrone *parcial, *melhor;
    double superior;
    double meta;
    bool metaAtingida;
    bool inteiros;
    unsigned long long nos;
    unsigned long long maxNos;
    bool interrompida;
    struct EstadoMemoBB **memo;
    unsigned char *chaveMemo;
    size_t memoriaMemo;
    size_t entradasMemo;
    double limiteEstrutural;
} BuscaBB;

typedef struct EstadoMemoBB {
    unsigned long long hash;
    size_t tamanho;
    unsigned char chave[];
} EstadoMemoBB;

enum { CAPACIDADE_MEMO_BB = 1048576 };

/* Permutacoes distintas frequentemente chegam ao mesmo conjunto de tarefas
 * e perfil de ocupacao. Elimina somente estados identicos (compara a chave
 * completa, nao apenas o hash). O teto de memoria limita o cache, nunca a
 * busca: ao encher, continua a enumeracao exata sem novas entradas. */
static bool estadoRepetidoBB(BuscaBB *b, const PerfilDrone *p)
{
    if (!b->memo) return false;
    size_t bits = ((size_t)b->n + 7) / 8;
    unsigned char *chave = b->chaveMemo;
    memset(chave, 0, bits);
    for (int i = 0; i < b->n; ++i) if (b->usado[i]) chave[i/8] |= (unsigned char)(1u << (i%8));
    size_t tamanho = bits;
    bool compacta = b->inteiros && b->superior <= USHRT_MAX && b->frota <= SCHAR_MAX;
    chave[tamanho++] = (unsigned char)compacta;
    for (int i = 0; i < p->quantidade; ++i) {
        if (compacta) {
            unsigned short t=(unsigned short)p->eventos[i].tempo;
            signed char d=(signed char)p->eventos[i].delta;
            memcpy(chave+tamanho,&t,sizeof t); tamanho+=sizeof t;
            memcpy(chave+tamanho,&d,sizeof d); tamanho+=sizeof d;
        } else {
            memcpy(chave+tamanho, &p->eventos[i].tempo, sizeof(double)); tamanho += sizeof(double);
            memcpy(chave+tamanho, &p->eventos[i].delta, sizeof(int)); tamanho += sizeof(int);
        }
    }
    unsigned long long hash = 1469598103934665603ULL;
    for (size_t i = 0; i < tamanho; ++i) { hash ^= chave[i]; hash *= 1099511628211ULL; }
    size_t posicao = (size_t)hash % CAPACIDADE_MEMO_BB;
    for (int passo = 0; passo < CAPACIDADE_MEMO_BB; ++passo) {
        EstadoMemoBB *estado = b->memo[posicao];
        if (!estado) {
            size_t bytes = sizeof *estado + tamanho;
            if (b->entradasMemo < CAPACIDADE_MEMO_BB*3u/4u &&
                b->memoriaMemo + bytes <= 128u * 1024u * 1024u) {
                estado = malloc(bytes);
                if (estado) {
                    estado->hash=hash; estado->tamanho=tamanho;
                    memcpy(estado->chave,chave,tamanho);
                    b->memo[posicao]=estado; b->memoriaMemo+=bytes;
                    ++b->entradasMemo;
                }
            }
            return false;
        }
        if (estado->hash == hash && estado->tamanho == tamanho && !memcmp(estado->chave,chave,tamanho)) return true;
        posicao = (posicao+1) % CAPACIDADE_MEMO_BB;
    }
    return false;
}

static double duracaoBB(const BuscaBB *b, int i, int k)
{
    if (k != b->quantidades[i]) return INFINITY;
    double t = b->inst->tempoDrone[b->clientes[i]][k];
    return isfinite(t) && t >= 0 ? t : INFINITY;
}

/* Desigualdade (16) da referencia: drone-tempo dividido pela frota.
 * Usa k recebido, fixo: carga das tarefas fixadas e restantes.
 * nextafterl evita arredondar o limite para cima por erro de ponto flutuante. */
static long double limiteCargaBB(const BuscaBB *b, long double fixada)
{
    long double carga = fixada;
    for (int i = 0; i < b->n; ++i)
        if (!b->usado[i]) carga = nextafterl(carga + b->minCarga[i], -INFINITY);
    long double limite = nextafterl(carga / b->frota, -INFINITY);
    return b->inteiros ? ceill(limite) : limite;
}

static bool tarefasIguaisBB(const BuscaBB *b, int i, int j)
{
    return b->quantidades[i] == b->quantidades[j] &&
        duracaoBB(b, i, b->quantidades[i]) == duracaoBB(b, j, b->quantidades[j]);
}

/* Enumera todas as ordens com as quantidades recebidas fixas.
 * Para makespan (objetivo regular), existe um cronograma ativo otimo
 * obtido pela geracao serial em alguma ordem, mantendo k fixo. Nao se fixa a ordem
 * de entrada, diferentemente do BB de quantidades da pasta de referencia. */
static void explorarBB(BuscaBB *b, int nivel, double maximo, long double fixada)
{
    if (b->metaAtingida) return;
    if (b->interrompida) return;
    if (b->maxNos && b->nos >= b->maxNos) { b->interrompida=true; return; }
    ++b->nos;
    if (maximo >= b->superior) return;
    if (nivel == b->n) {
        b->superior = maximo;
        memcpy(b->melhor, b->parcial, (size_t)b->n * sizeof *b->melhor);
        if (b->meta >= 0 && b->superior <= b->meta) b->metaAtingida = true;
        return;
    }
    if (limiteCargaBB(b, fixada) >= (long double)b->superior) return;
    if (b->limiteEstrutural >= b->superior) return;
    PerfilDrone *pai = &b->perfis[nivel];
    if (estadoRepetidoBB(b, pai)) return;
    /* Cada tarefa ainda precisa terminar, mesmo ignorando as outras restantes. */
    for (int i = 0; i < b->n; ++i) if (!b->usado[i]) {
        double menor = INFINITY;
        for (int k = b->quantidades[i]; k <= b->quantidades[i]; ++k) {
            double t = duracaoBB(b, i, k);
            if (isfinite(t)) menor = fmin(menor, primeiroInicioPerfil(pai, b->frota, t, k) + t);
        }
        if (menor >= b->superior) return;
    }
    for (int i = 0; i < b->n; ++i) if (!b->usado[i]) {
        bool simetrica = false;
        for (int j = 0; j < i; ++j)
            if (!b->usado[j] && tarefasIguaisBB(b, i, j)) { simetrica = true; break; }
        if (simetrica) continue;
        b->usado[i] = true;
        for (int k = b->quantidades[i]; k <= b->quantidades[i]; ++k) {
            double t = duracaoBB(b, i, k);
            if (!isfinite(t)) continue;
            double inicio = primeiroInicioPerfil(pai, b->frota, t, k), fim = inicio + t;
            if (!isfinite(fim) || fim >= b->superior) continue;
            b->parcial[nivel] = (MissaoDrone){b->clientes[i], k, inicio, fim};
            PerfilDrone *filho = &b->perfis[nivel + 1];
            filho->quantidade = pai->quantidade;
            memcpy(filho->eventos, pai->eventos, (size_t)pai->quantidade * sizeof *pai->eventos);
            ocuparPerfilDrone(filho, inicio, fim, k);
            long double carga = nextafterl(fixada +
                nextafterl((long double)k * t, -INFINITY), -INFINITY);
            explorarBB(b, nivel + 1, fmax(maximo, fim), carga);
            if (b->interrompida || b->metaAtingida) { b->usado[i]=false; return; }
        }
        b->usado[i] = false;
    }
}

/* Incumbente deterministico: a cada passo, menor termino; desempate por
 * carga e cliente. Preserva k; fornece limite superior para a busca exata. */
static int incumbenteBB(BuscaBB *b)
{
    PerfilDrone *p = &b->perfis[0];
    double maximo = 0;
    for (int nivel = 0; nivel < b->n; ++nivel) {
        int tarefa = -1, quantidade = 0;
        double melhorFim = INFINITY, melhorInicio = 0;
        long double menorCarga = INFINITY;
        for (int i = 0; i < b->n; ++i) if (!b->usado[i])
            for (int k = b->quantidades[i]; k <= b->quantidades[i]; ++k) {
                double t = duracaoBB(b, i, k);
                if (!isfinite(t)) continue;
                double inicio = primeiroInicioPerfil(p, b->frota, t, k), fim = inicio + t;
                long double carga = (long double)k * t;
                if (fim < melhorFim || (fim == melhorFim && carga < menorCarga)) {
                    tarefa = i; quantidade = k; melhorFim = fim;
                    melhorInicio = inicio; menorCarga = carga;
                }
            }
        if (tarefa < 0 || !isfinite(melhorFim)) return 0;
        b->usado[tarefa] = true;
        b->melhor[nivel] = (MissaoDrone){b->clientes[tarefa], quantidade, melhorInicio, melhorFim};
        ocuparPerfilDrone(p, melhorInicio, melhorFim, quantidade);
        maximo = fmax(maximo, melhorFim);
    }
    memset(b->usado, 0, (size_t)b->n * sizeof *b->usado);
    p->quantidade = 0;
    b->superior = maximo;
    return 1;
}

static bool avaliarOrdemIncumbenteBB(BuscaBB *b, const int *ordem)
{
    PerfilDrone *p=&b->perfis[0]; p->quantidade=0;
    double maximo=0;
    for (int nivel=0; nivel<b->n; ++nivel) {
        int i=ordem[nivel], k=b->quantidades[i];
        double duracao=duracaoBB(b,i,k);
        double inicio=primeiroInicioPerfil(p,b->frota,duracao,k), fim=inicio+duracao;
        b->parcial[nivel]=(MissaoDrone){b->clientes[i],k,inicio,fim};
        ocuparPerfilDrone(p,inicio,fim,k);
        maximo=fmax(maximo,fim);
    }
    p->quantidade=0;
    if (maximo>=b->superior) return false;
    b->superior=maximo;
    memcpy(b->melhor,b->parcial,(size_t)b->n*sizeof *b->melhor);
    return true;
}

/* Limites superiores deterministas mais fortes antes da enumeracao: ordens
 * por duracao/carga/demanda e trocas locais da melhor ordem. Nao sorteia k
 * nem substitui a prova do BB quando a meta dos caminhoes nao e atingida. */
static void melhorarIncumbenteBB(BuscaBB *b)
{
    int *ordem=malloc((size_t)b->n*sizeof *ordem);
    if (!ordem) return;
    for (int modo=0; modo<7; ++modo) {
        for (int i=0; i<b->n; ++i) ordem[i]=i;
        for (int i=1; i<b->n; ++i) {
            int v=ordem[i], j=i;
            double t=duracaoBB(b,v,b->quantidades[v]);
            double score=modo==0?t:modo==1?t*b->quantidades[v]:modo==2?b->quantidades[v]*100000.0+t:
                modo==3?-t:modo==4?-b->quantidades[v]*100000.0+t:modo==5?v:-v;
            while (j>0) {
                int a=ordem[j-1]; double d=duracaoBB(b,a,b->quantidades[a]);
                double outro=modo==0?d:modo==1?d*b->quantidades[a]:modo==2?b->quantidades[a]*100000.0+d:
                    modo==3?-d:modo==4?-b->quantidades[a]*100000.0+d:modo==5?a:-a;
                if (outro>=score) break;
                ordem[j]=a; --j;
            }
            ordem[j]=v;
        }
        avaliarOrdemIncumbenteBB(b,ordem);
        if (b->meta>=0 && b->superior<=b->meta) goto fim;
    }
    for (;;) {
        for (int i=0; i<b->n; ++i)
            for (int j=0; j<b->n; ++j)
                if (b->melhor[i].cliente==b->clientes[j]) {ordem[i]=j; break;}
        bool melhorou=false;
        for (int i=0; i<b->n && !melhorou; ++i)
            for (int j=i+1; j<b->n; ++j) {
                int aux=ordem[i]; ordem[i]=ordem[j]; ordem[j]=aux;
                melhorou=avaliarOrdemIncumbenteBB(b,ordem);
                aux=ordem[i]; ordem[i]=ordem[j]; ordem[j]=aux;
                if (melhorou) break;
            }
        if (!melhorou || (b->meta>=0 && b->superior<=b->meta)) break;
    }
fim:
    free(ordem);
}

/* Para cada demanda minima r, no maximo floor(frota/r) dessas missoes
 * podem coexistir. A soma das duracoes / esse numero e um LB adicional;
 * as duas menores entre as c+1 maiores tambem devem caber sequencialmente
 * em algum dos c intervalos de recurso. Nao altera o LB de carga reportado. */
static double limiteEstruturalBB(const BuscaBB *b)
{
    double *duracoes = malloc((size_t)b->n * sizeof *duracoes);
    if (!duracoes) return 0;
    long double melhor = 0;
    for (int r=1; r<=b->frota; ++r) {
        int quantidade=0, paralelas=b->frota/r;
        long double soma=0;
        for (int i=0; i<b->n; ++i) if (b->quantidades[i]>=r) {
            double t=duracaoBB(b,i,b->quantidades[i]);
            soma=nextafterl(soma+t,-INFINITY);
            int j=quantidade++;
            while (j>0 && duracoes[j-1]<t) {duracoes[j]=duracoes[j-1]; --j;}
            duracoes[j]=t;
        }
        long double limite=nextafterl(soma/paralelas,-INFINITY);
        if (b->inteiros) limite=ceill(limite);
        melhor=fmaxl(melhor,limite);
        if (quantidade) melhor=fmaxl(melhor,duracoes[0]);
        if (quantidade>paralelas)
            melhor=fmaxl(melhor,nextafterl((long double)duracoes[paralelas-1]+duracoes[paralelas],-INFINITY));
    }
    free(duracoes);
    return (double)melhor;
}

static int escalonarInterno(const Instancia *inst, Solucao *s, ResultadoBB *resultado,
                           bool busca, double meta, unsigned long long maxNos)
{
    if (resultado) *resultado = (ResultadoBB){0};
    if (!inst || !s || inst->n < 1 || inst->numDrones < 1 ||
        s->numMissoesDrone < 0 || s->numMissoesDrone >= inst->n ||
        !s->missaoDrone || !s->prefixosDrone || !inst->tempoDrone) return 0;
    BuscaBB b = {0};
    b.inst = inst; b.n = s->numMissoesDrone; b.frota = inst->numDrones;
    b.inteiros = true;
    b.meta = meta;
    b.maxNos = maxNos;
    int sucesso = 0;
    double inferior = 0;
    if (!b.n) {
        s->tempoDrones = 0; s->prefixosDrone[0] = 0;
        s->cronogramaValido = true; s->avaliacaoValida = false;
        s->makespan = INFINITY; s->branchBoundAplicado = true; s->cronogramaExterno = false;
        if (resultado) resultado->otimoProvado = true;
        return 1;
    }
    size_t n = (size_t)b.n, capacidade = 2 * n + 2;
    b.clientes = malloc(n * sizeof *b.clientes);
    b.quantidades = malloc(n * sizeof *b.quantidades);
    b.usado = calloc(n, sizeof *b.usado);
    b.minCarga = malloc(n * sizeof *b.minCarga);
    size_t niveis = busca ? n + 1 : 1;
    b.perfis = calloc(niveis, sizeof *b.perfis);
    b.eventos = malloc(niveis * capacidade * sizeof *b.eventos);
    b.parcial = malloc(n * sizeof *b.parcial);
    b.melhor = malloc(n * sizeof *b.melhor);
    if (!b.clientes || !b.quantidades || !b.usado || !b.minCarga || !b.perfis ||
        !b.eventos || !b.parcial || !b.melhor) goto fim;
    /* Canonicaliza o conjunto: o resultado nao depende da ordem recebida. */
    for (int i = 0; i < b.n; ++i) {
        int cliente = s->missaoDrone[i].cliente;
        int k = s->missaoDrone[i].qtdDrones;
        if (cliente <= 0 || cliente >= inst->n || !inst->tempoDrone[cliente]) goto fim;
        if (k < 1 || k > b.frota || !isfinite(inst->tempoDrone[cliente][k]) ||
            inst->tempoDrone[cliente][k] < 0) goto fim;
        int j = i;
        while (j > 0 && b.clientes[j - 1] > cliente) {
            b.clientes[j] = b.clientes[j - 1]; --j;
            b.quantidades[j + 1] = b.quantidades[j];
        }
        b.clientes[j] = cliente;
        b.quantidades[j] = k;
    }
    for (int i = 0; i < b.n; ++i) {
        if (i && b.clientes[i] == b.clientes[i - 1]) goto fim;
        long double menor = INFINITY;
        for (int k = b.quantidades[i]; k <= b.quantidades[i]; ++k) {
            double t = duracaoBB(&b, i, k);
            if (!isfinite(t)) continue;
            if (floor(t) != t) b.inteiros = false;
            menor = fminl(menor, nextafterl((long double)k * t, -INFINITY));
        }
        if (!isfinite(menor)) goto fim;
        b.minCarga[i] = menor;
    }
    for (size_t i = 0; i < niveis; ++i) b.perfis[i].eventos = b.eventos + i * capacidade;
    inferior = (double)limiteCargaBB(&b, 0);
    b.limiteEstrutural = limiteEstruturalBB(&b);
    if (!incumbenteBB(&b)) goto fim;
    if (busca) {
        melhorarIncumbenteBB(&b);
        if (meta >= 0 && b.superior <= meta) b.metaAtingida = true;
        else {
            if (b.n >= 8) {
                b.memo = calloc(CAPACIDADE_MEMO_BB, sizeof *b.memo);
                b.chaveMemo = malloc((n+7)/8 + 1 + capacidade*(sizeof(double)+sizeof(int)));
                if (!b.chaveMemo) { free(b.memo); b.memo=NULL; }
            }
            explorarBB(&b, 0, 0, 0);
        }
    }
    memcpy(s->missaoDrone, b.melhor, n * sizeof *s->missaoDrone);
    s->prefixosDrone[0] = 0;
    for (int i = 0; i < b.n; ++i)
        s->prefixosDrone[i + 1] = fmax(s->prefixosDrone[i], s->missaoDrone[i].tempoFim);
    s->tempoDrones = b.superior;
    s->cronogramaValido = true; s->branchBoundAplicado = busca;
    s->cronogramaExterno = false; s->avaliacaoValida = false; s->makespan = INFINITY;
    if (resultado) *resultado = (ResultadoBB){b.nos, inferior,
        busca && ((!b.metaAtingida && !b.interrompida) || inferior >= b.superior),
        b.metaAtingida, b.interrompida};
    sucesso = 1;
fim:
    if (b.memo) for (int i=0; i<CAPACIDADE_MEMO_BB; ++i) free(b.memo[i]);
    free(b.memo); free(b.chaveMemo);
    free(b.clientes); free(b.quantidades); free(b.usado); free(b.minCarga); free(b.perfis);
    free(b.eventos); free(b.parcial); free(b.melhor);
    return sucesso;
}

int escalonarMissoesDrone(const Instancia *inst, Solucao *s)
{
    return escalonarInterno(inst, s, NULL, false, -1, 0);
}

int escalonarMissoesDroneBB(const Instancia *inst, Solucao *s, ResultadoBB *r)
{
    return escalonarInterno(inst, s, r, true, -1, 0);
}

int escalonarMissoesDroneBBMeta(const Instancia *inst, Solucao *s, double meta, ResultadoBB *r)
{
    if (!isfinite(meta) || (meta < 0 && meta != -1)) return 0;
    return escalonarInterno(inst, s, r, true, meta, 0);
}

int escalonarMissoesDroneBBLimitado(const Instancia *inst, Solucao *s, double meta,
                                  unsigned long long maxNos, ResultadoBB *r)
{
    if (!isfinite(meta) || (meta<0 && meta!=-1)) return 0;
    return escalonarInterno(inst,s,r,true,meta,maxNos);
}


