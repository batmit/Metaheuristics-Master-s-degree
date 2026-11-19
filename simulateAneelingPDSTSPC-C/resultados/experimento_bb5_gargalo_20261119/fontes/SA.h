#ifndef SA_H
#define SA_H
#include "cv.h"

/* Contadores opcionais para testes e analises; nao aparecem no CSV. */
typedef struct EstatisticasSA {
    unsigned long long avaliacoes, trocas, reconstrucoes, retomadas;
    unsigned long long trocasAceitas, reconstrucoesAceitas, melhoriasMacro;
    unsigned long long retomadasDeOutraAtribuicao;
    int atribuicoesElite;
    bool terminouPorTempo, terminouPorEstagnacao;
    double segundosCalibracao, segundosBusca, temperaturaInicial, temperaturaFinal;
    unsigned long long chamadasBBCaminhao, chamadasBBDrone, nosBBCaminhao, nosBBDrone;
    unsigned long long melhoriasBBCaminhao, melhoriasBBDrone;
    double segundosBBCaminhao, segundosBBDrone;
    unsigned long long bloqueiosBBCaminhao;
} EstatisticasSA;

/* Um unico perfil, ajustavel aqui e em parametrosSARapido(). Tempos em segundos. */
typedef struct ParametrosSA {
    double alpha, gamma;           /* Resfriamento (0..1) e aceitacao alvo (0..1). */
    int saMax;                    /* Tentativas locais por nivel de temperatura. */
    double tempInicial, tempFinal;
    bool pesosAdaptativos;
    double taxaAdaptacao, pesoMinimo;
    bool movimentosGrupo, trocasSimultaneas, reconstrucaoParcial;
    double probTroca, probReconstrucao; /* Probabilidades somente na calibracao. */
    double fracaoMinima, fracaoMaxima;   /* Fracao removida na reconstrucao. */
    double folgaElite;            /* Margem relativa dos slots de diversidade. */
    int niveisSemMelhoraRetomada, maxRetomadas;
    double reaquecimento;         /* Fracao de T0 usada nas retomadas. */
    int minimoAvaliacoes, niveisParada; /* Requisitos da parada por estagnacao. */
    double limiteSegundos;        /* Teto; zero desativa o teto de tempo. */
    bool refinoFinalBB;
    unsigned long long maxNosBB;  /* Teto por chamada de B&B, inclusive a final. */
    EstatisticasSA *estatisticas;
    int estagnacaoRecursoBB;       /* 0 desativa; 5 = cinco propostas sem melhora aceita. */
    double segundosPorBB;          /* Teto por chamada, limitado tambem pelo prazo do SA. */
    bool apenasGargaloBBCaminhao;   /* Chama apenas se tempoCaminhao > tempoDrones + 1e-9. */
} ParametrosSA;

typedef struct PesosMovimentosSA {
    double pesos[NUM_MOVIMENTOS], pontos[NUM_MOVIMENTOS];
    int tentativas[NUM_MOVIMENTOS];
} PesosMovimentosSA;

enum { NUM_MELHORES_SA = 4 };

/* Configuracao validada: parada natural, macros por estagnacao e B&B final. */
ParametrosSA parametrosSARapido(void);
/* Retorna 1 para parametros finitos e dentro das faixas aceitas. */
int validarParametrosSA(const ParametrosSA *parametros);
/* Relogio de parede em segundos. */
double relogioSA(void);
/* Pesos comecam iguais; depois seguem a pontuacao media de cada operador. */
PesosMovimentosSA inicializarPesosMovimentosSA(void);
void atualizarPesosMovimentosSA(PesosMovimentosSA *estado, const ParametrosSA *parametros);
/* Calibra T0 numa copia, sem modificar a solucao original. */
int calculaTemperaturaInicial(const Instancia *inst, const Solucao *referencia,
                             const ParametrosSA *parametros, double *temperatura);
/* Entrada/saida alocada para inst; retorna o melhor viavel, preservando-a na falha. */
int simulatedAnnealing(const Instancia *inst, Solucao *solucao, const ParametrosSA *parametros);
/* Compartilha o teto de tempo entre calibracao e busca. */
int resolverComCalibracaoSA(const Instancia *inst, Solucao *solucao,
                          const ParametrosSA *parametros, double *temperatura);
/* Preserva o melhor e ate tres atribuicoes distintas dentro da folga. */
int atualizarEliteDiversaSA(const Instancia *inst, const Solucao *candidata,
                          Solucao melhores[NUM_MELHORES_SA], int *quantidade, double folgaRelativa);
/* Distancia: numero de clientes com recurso diferente (caminhao/drone). */
int distanciaAtribuicaoSA(const Instancia *inst, const Solucao *a, const Solucao *b);
#endif
