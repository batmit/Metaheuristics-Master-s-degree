#include "cv.h"
#include <math.h>
#include <stdlib.h>

static int alocarMatriz(double ***destino, int linhas, int colunas, double valor)
{
    if (*destino != NULL) return 1;
    double **matriz = calloc((size_t)linhas, sizeof *matriz);
    if (!matriz) return 0;
    for (int i = 0; i < linhas; ++i) {
        matriz[i] = malloc((size_t)colunas * sizeof **matriz);
        if (!matriz[i]) {
            for (int j = 0; j < i; ++j) free(matriz[j]);
            free(matriz);
            return 0;
        }
        for (int j = 0; j < colunas; ++j) matriz[i][j] = valor;
    }
    *destino = matriz;
    return 1;
}

int alocarMatrizCaminhao(Instancia *inst)
{
    return inst && inst->n > 0 &&
        alocarMatriz(&inst->tempoCaminhao, inst->n, inst->n, 0.0);
}

int alocarTempoDrone(Instancia *inst)
{
    return inst && inst->n > 0 && inst->numDrones > 0 &&
        alocarMatriz(&inst->tempoDrone, inst->n, inst->numDrones + 1, INFINITY);
}

int alocarInstancias(Instancia *inst)
{
    return alocarMatrizCaminhao(inst) && alocarTempoDrone(inst);
}

int construirMatrizCaminhao(Instancia *inst)
{
    if (!inst || !inst->clientes || !isfinite(inst->truckSpeed) ||
        inst->truckSpeed <= 0 || !alocarMatrizCaminhao(inst)) return 0;
    for (int i = 0; i < inst->n; ++i)
        for (int j = 0; j < inst->n; ++j) {
            double dx = fabs(inst->clientes[i].x - inst->clientes[j].x);
            double dy = fabs(inst->clientes[i].y - inst->clientes[j].y);
            inst->tempoCaminhao[i][j] = (dx + dy) / inst->truckSpeed * 60.0;
        }
    return 1;
}

void liberarSolucao(Solucao *solucao)
{
    if (!solucao) return;
    if (solucao->rotasCaminhao)
        for (int c = 0; c < solucao->numCaminhoes; ++c)
            free(solucao->rotasCaminhao[c].rota);
    free(solucao->rotasCaminhao);
    free(solucao->missaoDrone);
    free(solucao->prefixosDrone);
    *solucao = (Solucao){0};
}

int alocarSolucao(const Instancia *inst, Solucao *solucao)
{
    if (!inst || !solucao || inst->n < 1 || inst->numCaminhoes < 1) return 0;
    Solucao nova = {0};
    nova.numCaminhoes = inst->numCaminhoes;
    nova.rotasCaminhao = calloc((size_t)nova.numCaminhoes, sizeof *nova.rotasCaminhao);
    if (!nova.rotasCaminhao) return 0;
    for (int c = 0; c < nova.numCaminhoes; ++c) {
        RotaCaminhao *rota = &nova.rotasCaminhao[c];
        rota->capacidade = inst->n - 1;
        if (rota->capacidade) {
            rota->rota = calloc((size_t)rota->capacidade, sizeof *rota->rota);
            if (!rota->rota) goto erro;
        }
    }
    nova.missaoDrone = calloc((size_t)inst->n, sizeof *nova.missaoDrone);
    nova.prefixosDrone = calloc((size_t)inst->n, sizeof *nova.prefixosDrone);
    if (!nova.missaoDrone || !nova.prefixosDrone) goto erro;
    nova.makespan = INFINITY;
    *solucao = nova;
    return 1;
erro:
    liberarSolucao(&nova);
    return 0;
}

#include <string.h>

int copiarSolucao(const Instancia *inst, const Solucao *a, Solucao *b)
{
    if (!inst || !a || !b || a == b ||
        a->numCaminhoes != inst->numCaminhoes || b->numCaminhoes != inst->numCaminhoes ||
        !a->rotasCaminhao || !b->rotasCaminhao || !a->missaoDrone ||
        !b->missaoDrone || !a->prefixosDrone || !b->prefixosDrone ||
        a->numMissoesDrone < 0 || a->numMissoesDrone >= inst->n) return 0;
    for (int c = 0; c < a->numCaminhoes; ++c) {
        int n = a->rotasCaminhao[c].tamanho;
        if (n < 0 || n > b->rotasCaminhao[c].capacidade ||
            n > a->rotasCaminhao[c].capacidade) return 0;
    }
    for (int c = 0; c < a->numCaminhoes; ++c) {
        RotaCaminhao *r = &b->rotasCaminhao[c];
        r->tamanho = a->rotasCaminhao[c].tamanho;
        r->tempo = a->rotasCaminhao[c].tempo;
        if (r->tamanho) memcpy(r->rota, a->rotasCaminhao[c].rota,
                              (size_t)r->tamanho * sizeof *r->rota);
    }
    b->numMissoesDrone = a->numMissoesDrone;
    memcpy(b->missaoDrone, a->missaoDrone, (size_t)a->numMissoesDrone * sizeof *b->missaoDrone);
    memcpy(b->prefixosDrone, a->prefixosDrone,
           ((size_t)a->numMissoesDrone + 1) * sizeof *b->prefixosDrone);
    b->makespan = a->makespan;
    b->tempoCaminhoes = a->tempoCaminhoes;
    b->tempoDrones = a->tempoDrones;
    b->cronogramaValido = a->cronogramaValido;
    b->avaliacaoValida = a->avaliacaoValida;
    b->cronogramaExterno = a->cronogramaExterno;
    b->branchBoundAplicado = a->branchBoundAplicado;
    return 1;
}

int calcularMakespan(const Instancia *inst, Solucao *s)
{
    if (!s) return 0;
    s->avaliacaoValida = false;
    s->makespan = INFINITY;
    if (!inst || inst->n < 1 || s->numCaminhoes != inst->numCaminhoes ||
        !s->rotasCaminhao || !inst->tempoCaminhao || !inst->tempoDrone ||
        !s->missaoDrone || s->numMissoesDrone < 0 || s->numMissoesDrone >= inst->n ||
        !s->cronogramaValido) return 0;
    bool *visto = calloc((size_t)inst->n, sizeof *visto);
    if (!visto) return 0;
    int total = 0, ok = 0;
    double maxC = 0, maxD = 0;
    for (int c = 0; c < s->numCaminhoes; ++c) {
        RotaCaminhao *r = &s->rotasCaminhao[c];
        if (r->tamanho < 0 || r->tamanho > r->capacidade ||
            (r->tamanho && !r->rota)) goto fim;
        int anterior = 0;
        double tempo = 0;
        for (int p = 0; p < r->tamanho; ++p) {
            int j = r->rota[p];
            if (j <= 0 || j >= inst->n || visto[j]) goto fim;
            double trecho = inst->tempoCaminhao[anterior][j];
            if (!isfinite(trecho) || trecho < 0) goto fim;
            tempo += trecho;
            visto[j] = true;
            ++total;
            anterior = j;
        }
        if (r->tamanho) tempo += inst->tempoCaminhao[anterior][0];
        if (!isfinite(tempo) || tempo < 0) goto fim;
        r->tempo = tempo;
        maxC = fmax(maxC, tempo);
    }
    for (int i = 0; i < s->numMissoesDrone; ++i) {
        const MissaoDrone *m = &s->missaoDrone[i];
        if (m->cliente <= 0 || m->cliente >= inst->n || visto[m->cliente] ||
            m->qtdDrones < 1 || m->qtdDrones > inst->numDrones ||
            !isfinite(m->tempoInicio) || !isfinite(m->tempoFim) ||
            m->tempoInicio < 0 || m->tempoFim < m->tempoInicio) goto fim;
        double duracao = inst->tempoDrone[m->cliente][m->qtdDrones];
        if (!isfinite(duracao) || duracao < 0 ||
            fabs(m->tempoFim - m->tempoInicio - duracao) > 1e-9 * fmax(1, duracao)) goto fim;
        visto[m->cliente] = true;
        ++total;
        maxD = fmax(maxD, m->tempoFim);
        int usados = 0;
        for (int j = 0; j < s->numMissoesDrone; ++j) {
            const MissaoDrone *outro = &s->missaoDrone[j];
            if (outro->tempoInicio <= m->tempoInicio && m->tempoInicio < outro->tempoFim)
                usados += outro->qtdDrones;
        }
        if (usados > inst->numDrones) goto fim;
    }
    if (total != inst->n - 1) goto fim;
    s->tempoCaminhoes = maxC;
    s->tempoDrones = maxD;
    s->makespan = fmax(maxC, maxD);
    s->avaliacaoValida = true;
    ok = 1;
fim:
    free(visto);
    return ok;
}

int construirSolucaoInicial(const Instancia *inst, Solucao *s)
{
    if (!inst || !s || inst->numCaminhoes < 1 ||
        s->numCaminhoes != inst->numCaminhoes) return 0;
    int *clientes = malloc((size_t)inst->n * sizeof *clientes);
    if (!clientes) return 0;
    for (int c = 0; c < s->numCaminhoes; ++c) s->rotasCaminhao[c].tamanho = 0;
    s->numMissoesDrone = 0;
    for (int i = 0; i < inst->n - 1; ++i) clientes[i] = i + 1;
    /* Clientes distantes primeiro; desempate por indice, sem sorteios. */
    for (int i = 1; i < inst->n - 1; ++i) {
        int j = i, cliente = clientes[i];
        while (j > 0 && inst->tempoCaminhao[0][clientes[j-1]] < inst->tempoCaminhao[0][cliente]) {
            clientes[j] = clientes[j-1]; --j;
        }
        clientes[j] = cliente;
    }
    for (int c = 0; c < s->numCaminhoes; ++c) s->rotasCaminhao[c].tempo = 0;
    for (int i = 0; i < inst->n - 1; ++i) {
        double melhor = INFINITY, melhorDelta = INFINITY;
        int caminhao = 0, posicao = 0, cliente = clientes[i];
        for (int c = 0; c < s->numCaminhoes; ++c) {
            RotaCaminhao *r = &s->rotasCaminhao[c];
            double outras = 0;
            for (int d = 0; d < s->numCaminhoes; ++d)
                if (d != c) outras = fmax(outras, s->rotasCaminhao[d].tempo);
            for (int p = 0; p <= r->tamanho; ++p) {
                int a = p ? r->rota[p-1] : 0, b = p < r->tamanho ? r->rota[p] : 0;
                double delta = inst->tempoCaminhao[a][cliente] + inst->tempoCaminhao[cliente][b] - inst->tempoCaminhao[a][b];
                double valor = fmax(outras, r->tempo + delta);
                if (valor < melhor || (valor == melhor && delta < melhorDelta)) {
                    melhor = valor; melhorDelta = delta; caminhao = c; posicao = p;
                }
            }
        }
        RotaCaminhao *r = &s->rotasCaminhao[caminhao];
        memmove(r->rota + posicao + 1, r->rota + posicao,
                (size_t)(r->tamanho-posicao) * sizeof *r->rota);
        r->rota[posicao] = cliente; ++r->tamanho; r->tempo += melhorDelta;
    }
    for (int c = 0; c < s->numCaminhoes; ++c) {
        RotaCaminhao *r = &s->rotasCaminhao[c];
        for (;;) {
            double deltaMelhor = -1e-9;
            int inicio = -1, fim = -1;
            for (int i = 0; i < r->tamanho; ++i)
                for (int j = i+1; j < r->tamanho; ++j) {
                    int a = i ? r->rota[i-1] : 0, b = j+1 < r->tamanho ? r->rota[j+1] : 0;
                    double delta = inst->tempoCaminhao[a][r->rota[j]] + inst->tempoCaminhao[r->rota[i]][b]
                        - inst->tempoCaminhao[a][r->rota[i]] - inst->tempoCaminhao[r->rota[j]][b];
                    if (delta < deltaMelhor) { deltaMelhor = delta; inicio = i; fim = j; }
                }
            if (inicio < 0) break;
            for (int i = inicio, j = fim; i < j; ++i, --j) {
                int aux = r->rota[i]; r->rota[i] = r->rota[j]; r->rota[j] = aux;
            }
            r->tempo += deltaMelhor;
        }
    }
    free(clientes);
    return escalonarMissoesDrone(inst, s) && calcularMakespan(inst, s);
}

static int tamanhoVetor(const Solucao *s, int v)
{
    return v == s->numCaminhoes ? s->numMissoesDrone : s->rotasCaminhao[v].tamanho;
}

static int clienteVetor(const Solucao *s, int v, int p)
{
    return v == s->numCaminhoes ? s->missaoDrone[p].cliente : s->rotasCaminhao[v].rota[p];
}

static int quantidadeViavel(const Instancia *inst, int cliente)
{
    for (int k = 1; k <= inst->numDrones; ++k)
        if (isfinite(inst->tempoDrone[cliente][k]) && inst->tempoDrone[cliente][k] >= 0)
            return k;
    return 0;
}

static int sortearQuantidade(const Instancia *inst, int cliente, int excluir)
{
    int total = 0, escolhido = 0;
    for (int k = 1; k <= inst->numDrones; ++k)
        if (k != excluir && isfinite(inst->tempoDrone[cliente][k]) &&
            inst->tempoDrone[cliente][k] >= 0) {
            ++total;
            if (rand() % total == 0) escolhido = k;
        }
    return escolhido;
}

static double aresta(const Instancia *inst, const RotaCaminhao *r, int p)
{
    int a = p == 0 ? 0 : r->rota[p - 1];
    int b = p == r->tamanho ? 0 : r->rota[p];
    return inst->tempoCaminhao[a][b];
}

static void removerCaminhao(const Instancia *inst, RotaCaminhao *r, int p)
{
    int a = p == 0 ? 0 : r->rota[p - 1];
    int b = p + 1 == r->tamanho ? 0 : r->rota[p + 1];
    r->tempo += inst->tempoCaminhao[a][b] - aresta(inst, r, p) - aresta(inst, r, p + 1);
    memmove(r->rota + p, r->rota + p + 1, (size_t)(r->tamanho - p - 1) * sizeof *r->rota);
    if (--r->tamanho == 0) r->tempo = 0;
}

static void inserirCaminhao(const Instancia *inst, RotaCaminhao *r, int p, int j)
{
    int a = p == 0 ? 0 : r->rota[p - 1];
    int b = p == r->tamanho ? 0 : r->rota[p];
    r->tempo += inst->tempoCaminhao[a][j] + inst->tempoCaminhao[j][b] - aresta(inst, r, p);
    memmove(r->rota + p + 1, r->rota + p, (size_t)(r->tamanho - p) * sizeof *r->rota);
    r->rota[p] = j;
    ++r->tamanho;
}

int gerarVizinhoAvaliado(const Instancia *inst, const Solucao *a, Solucao *b, TipoMovimento *tipo)
{
    if (!inst || !a || !b || !a->avaliacaoValida || !a->cronogramaValido) return -1;
    int s = a->numCaminhoes, trocaveis = 0, transferencias = 0;
    /* Conta pares cliente-destino viaveis, inclusive destinos vazios. */
    for (int v = 0; v <= s; ++v) {
        int n = tamanhoVetor(a, v);
        if (v < s && n >= 2) ++trocaveis;
        for (int p = 0; p < n; ++p) {
            transferencias += s - (v < s);
            if (v < s && quantidadeViavel(inst, clienteVetor(a, v, p))) ++transferencias;
        }
    }
    int alteraveis = 0;
    for (int p = 0; p < a->numMissoesDrone; ++p) {
        const MissaoDrone *m = &a->missaoDrone[p];
        for (int k = 1; k <= inst->numDrones; ++k)
            if (k != m->qtdDrones && isfinite(inst->tempoDrone[m->cliente][k]) &&
                inst->tempoDrone[m->cliente][k] >= 0) { ++alteraveis; break; }
    }
    TipoMovimento disponiveis[5];
    int totalTipos = 0;
    if (trocaveis) disponiveis[totalTipos++] = TROCAR_POSICOES;
    if (transferencias) disponiveis[totalTipos++] = TRANSFERIR_CLIENTE;
    if (alteraveis) disponiveis[totalTipos++] = ALTERAR_QUANTIDADE_DRONES;
    if (trocaveis) {
        disponiveis[totalTipos++] = DOIS_OPT_CAMINHAO;
        disponiveis[totalTipos++] = REINSERIR_CAMINHAO;
    }
    if (!totalTipos) return 0;
    TipoMovimento movimento = disponiveis[rand() % totalTipos];
    if (!copiarSolucao(inst, a, b)) return -1;
    bool mudouDrone = false;
    if (movimento == TROCAR_POSICOES) {
        int escolha = rand() % trocaveis, v = 0;
        for (; v < s; ++v) if (tamanhoVetor(a, v) >= 2 && escolha-- == 0) break;
        int n = tamanhoVetor(a, v), i = rand() % n, j = rand() % (n - 1);
        if (j >= i) ++j;
        {
            RotaCaminhao *r = &b->rotasCaminhao[v];
            int indices[4] = {i, i + 1, j, j + 1};
            double antes = 0, depois = 0;
            for (int k = 0; k < 4; ++k) {
                bool repetida = false;
                for (int l = 0; l < k; ++l) if (indices[k] == indices[l]) repetida = true;
                if (!repetida) antes += aresta(inst, r, indices[k]);
            }
            int aux = r->rota[i]; r->rota[i] = r->rota[j]; r->rota[j] = aux;
            for (int k = 0; k < 4; ++k) {
                bool repetida = false;
                for (int l = 0; l < k; ++l) if (indices[k] == indices[l]) repetida = true;
                if (!repetida) depois += aresta(inst, r, indices[k]);
            }
            r->tempo += depois - antes;
        }
    } else if (movimento == DOIS_OPT_CAMINHAO || movimento == REINSERIR_CAMINHAO) {
        int escolha = rand() % trocaveis, c = 0;
        for (; c < s; ++c) if (b->rotasCaminhao[c].tamanho >= 2 && escolha-- == 0) break;
        RotaCaminhao *r = &b->rotasCaminhao[c];
        int i = rand() % r->tamanho, j = rand() % (r->tamanho-1);
        if (j >= i) ++j;
        if (movimento == REINSERIR_CAMINHAO) {
            int cliente = r->rota[i];
            removerCaminhao(inst, r, i);
            inserirCaminhao(inst, r, j, cliente);
        } else {
            if (i > j) { int aux = i; i = j; j = aux; }
            int anterior = i ? r->rota[i-1] : 0;
            int proximo = j+1 < r->tamanho ? r->rota[j+1] : 0;
            r->tempo += inst->tempoCaminhao[anterior][r->rota[j]] + inst->tempoCaminhao[r->rota[i]][proximo]
                - inst->tempoCaminhao[anterior][r->rota[i]] - inst->tempoCaminhao[r->rota[j]][proximo];
            while (i < j) { int aux = r->rota[i]; r->rota[i++] = r->rota[j]; r->rota[j--] = aux; }
        }
    } else if (movimento == ALTERAR_QUANTIDADE_DRONES) {
        int escolha = rand() % alteraveis;
        for (int p = 0; p < b->numMissoesDrone; ++p) {
            MissaoDrone *m = &b->missaoDrone[p];
            bool alternativa = false;
            for (int k = 1; k <= inst->numDrones; ++k)
                if (k != m->qtdDrones && isfinite(inst->tempoDrone[m->cliente][k]) &&
                    inst->tempoDrone[m->cliente][k] >= 0) { alternativa = true; break; }
            if (alternativa && escolha-- == 0) {
                m->qtdDrones = sortearQuantidade(inst, m->cliente, m->qtdDrones);
                break;
            }
        }
        mudouDrone = true;
    } else {
        int escolha = rand() % transferencias, origem = -1, pos = -1, destino = -1;
        for (int v = 0; v <= s && origem < 0; ++v)
            for (int p = 0; p < tamanhoVetor(a, v) && origem < 0; ++p)
                for (int d = 0; d <= s; ++d) {
                    if (v == d || (d == s && !quantidadeViavel(inst, clienteVetor(a, v, p)))) continue;
                    if (escolha-- == 0) { origem = v; pos = p; destino = d; break; }
                }
        if (origem < 0) return -1;
        int cliente = clienteVetor(a, origem, pos);
        int insercao = destino == s ? b->numMissoesDrone :
            rand() % (tamanhoVetor(a, destino) + 1);
        if (origem == s) {
            memmove(b->missaoDrone + pos, b->missaoDrone + pos + 1,
                    (size_t)(b->numMissoesDrone - pos - 1) * sizeof *b->missaoDrone);
            --b->numMissoesDrone;
            mudouDrone = true;
        } else removerCaminhao(inst, &b->rotasCaminhao[origem], pos);
        if (destino == s) {
            memmove(b->missaoDrone + insercao + 1, b->missaoDrone + insercao,
                    (size_t)(b->numMissoesDrone - insercao) * sizeof *b->missaoDrone);
            b->missaoDrone[insercao] = (MissaoDrone){cliente, sortearQuantidade(inst, cliente, 0), 0, 0};
            ++b->numMissoesDrone;
            mudouDrone = true;
        } else inserirCaminhao(inst, &b->rotasCaminhao[destino], insercao, cliente);
    }
    b->avaliacaoValida = false;
    if (mudouDrone && !escalonarMissoesDrone(inst, b)) return -1;
    b->tempoCaminhoes = 0;
    for (int c = 0; c < s; ++c) {
        double t = b->rotasCaminhao[c].tempo;
        if (!isfinite(t) || t < -1e-8) return -1;
        b->rotasCaminhao[c].tempo = fmax(0, t);
        b->tempoCaminhoes = fmax(b->tempoCaminhoes, b->rotasCaminhao[c].tempo);
    }
    b->makespan = fmax(b->tempoCaminhoes, b->tempoDrones);
    b->avaliacaoValida = true;
    if (tipo) *tipo = movimento;
    return 1;
}



