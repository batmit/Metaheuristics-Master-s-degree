
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
static double diagAgora(void) { LARGE_INTEGER t,f; QueryPerformanceCounter(&t); QueryPerformanceFrequency(&f); return (double)t.QuadPart/f.QuadPart; }
#else
#include <sys/time.h>
static double diagAgora(void) { struct timeval t; gettimeofday(&t, NULL); return t.tv_sec+t.tv_usec/1e6; }
#endif
#include "cv.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

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
    bool inteiros;
    unsigned long long nos;
} BuscaBB;

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
    ++b->nos;
    if (maximo >= b->superior) return;
    if (nivel == b->n) {
        b->superior = maximo;
        memcpy(b->melhor, b->parcial, (size_t)b->n * sizeof *b->melhor);
        return;
    }
    if (limiteCargaBB(b, fixada) >= (long double)b->superior) return;
    PerfilDrone *pai = &b->perfis[nivel];
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

static int escalonarInternoDiagnostico(const Instancia *inst, Solucao *s, ResultadoBB *resultado)
{
    if (resultado) *resultado = (ResultadoBB){0};
    if (!inst || !s || inst->n < 1 || inst->numDrones < 1 ||
        s->numMissoesDrone < 0 || s->numMissoesDrone >= inst->n ||
        !s->missaoDrone || !s->prefixosDrone || !inst->tempoDrone) return 0;
    BuscaBB b = {0};
    b.inst = inst; b.n = s->numMissoesDrone; b.frota = inst->numDrones;
    b.inteiros = true;
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
    b.perfis = calloc(n + 1, sizeof *b.perfis);
    b.eventos = malloc((n + 1) * capacidade * sizeof *b.eventos);
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
    for (size_t i = 0; i <= n; ++i) b.perfis[i].eventos = b.eventos + i * capacidade;
    inferior = (double)limiteCargaBB(&b, 0);
    if (!incumbenteBB(&b)) goto fim;
    explorarBB(&b, 0, 0, 0);
    memcpy(s->missaoDrone, b.melhor, n * sizeof *s->missaoDrone);
    s->prefixosDrone[0] = 0;
    for (int i = 0; i < b.n; ++i)
        s->prefixosDrone[i + 1] = fmax(s->prefixosDrone[i], s->missaoDrone[i].tempoFim);
    s->tempoDrones = b.superior;
    s->cronogramaValido = true; s->branchBoundAplicado = true;
    s->cronogramaExterno = false; s->avaliacaoValida = false; s->makespan = INFINITY;
    if (resultado) *resultado = (ResultadoBB){b.nos, inferior, true};
    sucesso = 1;
fim:
    free(b.clientes); free(b.quantidades); free(b.usado); free(b.minCarga); free(b.perfis);
    free(b.eventos); free(b.parcial); free(b.melhor);
    return sucesso;
}

int escalonarMissoesDrone(const Instancia *inst, Solucao *s)
{
    return escalonarMissoesDroneBB(inst, s, NULL);
}



int escalonarMissoesDroneBB(const Instancia *inst, Solucao *s, ResultadoBB *resultado) {
    ResultadoBB r = {0}; double t = diagAgora();
    fprintf(stderr, "BB_BEGIN n=%d t=%.9f\n", s->numMissoesDrone, t); fflush(stderr);
    int ok = escalonarInternoDiagnostico(inst,s,&r);
    fprintf(stderr, "BB_END n=%d sec=%.9f nodes=%llu lb=%.9f ub=%.9f\n", s->numMissoesDrone, diagAgora()-t, r.nos, r.limiteInferior,s->tempoDrones); fflush(stderr);
    if(resultado) *resultado=r;
    return ok;
}
