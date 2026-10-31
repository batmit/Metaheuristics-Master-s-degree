#ifndef SA_H
#define SA_H

#include "cv.h"
#include <stddef.h>

typedef struct ParametrosSA {
    double alpha;       /* Resfriamento: 0 < alpha < 1. */
    double beta;        /* Aquecimento da calibracao: beta > 1. */
    double gamma;       /* Fracao alvo de aceitacao: 0 < gamma < 1. */
    int saMax;          /* Tentativas por temperatura, inclusive na calibracao. */
    double tempInicial; /* Temperatura manual ou ponto de partida da calibracao. */
    double tempFinal;   /* 0 < tempFinal < tempInicial. */
    bool pesosAdaptativos; /* false permite comparar com o sorteio uniforme. */
    double taxaAdaptacao;  /* 0 < rho <= 1. */
    double pesoMinimo;     /* 0 < pesoMinimo <= 1 (peso inicial). */
    bool bbPeriodico;
    int niveisSemMelhoraBB; /* Disparo apos este numero de niveis sem melhora. */
    unsigned long long maxNosBB; /* Orcamento por chamada periodica. */
    bool movimentosGrupo; /* Transferencias conjuntas; false restaura o SA anterior. */
} ParametrosSA;

typedef struct PesosMovimentosSA {
    double pesos[NUM_MOVIMENTOS];
    double pontos[NUM_MOVIMENTOS];
    int tentativas[NUM_MOVIMENTOS];
} PesosMovimentosSA;

PesosMovimentosSA inicializarPesosMovimentosSA(void);
/* Ao final de cada temperatura: combina peso anterior com pontuacao media,
 * aplica piso e zera contadores; tipos nao tentados mantem seus pesos.
 * Requer parametros validos. Pontos exclusivos: melhor global=5, melhora=3,
 * piora aceita=1; empate ou rejeicao=0. */
void atualizarPesosMovimentosSA(PesosMovimentosSA *estado, const ParametrosSA *parametros);

/* beta=1.1, gamma=0.95, saMax=500.
 * Valores iniciais ajustaveis para os demais: alpha=0.99,
 * tempInicial=10.0 e tempFinal=0.01, na escala do makespan em minutos.
 * Pesos adaptativos ativos, taxaAdaptacao=0.2 e pesoMinimo=0.1.
 * BB periodico ativo apos 5 niveis sem melhora, com ate 10000 nos por chamada.
 * Grupos sao opcionais (false); a main ativa nas quatro instancias selecionadas. */
ParametrosSA parametrosSAPadrao(void);

/* Retorna 1 somente se todos os parametros forem finitos e validos. */
int validarParametrosSA(const ParametrosSA *parametros);

/* A calibracao testa vizinhos sem modificar a solucao de referencia;
 * aumenta a temperatura por beta ate atingir gamma de aceitacao.
 * Reconstroi e avalia uma copia da referencia antes dos testes.
 * Usa selecao uniforme, sem aprender ou alterar pesos.
 * Retorna 1 e escreve temperatura somente no sucesso; 0 na falha, inclusive
 * ausencia de movimentos, erro de avaliacao, overflow ou limite de 10000 niveis.
 * Inicializar srand uma unica vez no chamador. */
int calculaTemperaturaInicial(const Instancia *inst, const Solucao *referencia,
                             const ParametrosSA *parametros, double *temperatura);

enum { NUM_MELHORES_SA = 4 };

/* Arquivo ordenado por makespan, da melhor para a pior. Os quatro slots devem
 * estar alocados para inst; quantidade inicia em zero. Copia a candidata sem
 * compartilhar buffers. Repeticoes de rota e missoes/quantidades sao ignoradas.
 * Empates distintos preenchem vagas; arquivo cheio exige melhora estrita da pior.
 * Retorna 1 se inseriu; 0 caso contrario. Requer candidata avaliada e viavel. */
int atualizarMelhoresSolucoesSA(const Instancia *inst, const Solucao *candidata,
                              Solucao melhores[NUM_MELHORES_SA], int *quantidade);

/* Historico pertence a uma instancia imutavel e a uma execucao. Inicializar
 * com {0}; liberar ao final. Chave: sequencia de clientes dos drones, sem
 * quantidades ou rota do caminhao. Entradas guardam cronograma e status do BB. */
struct EntradaHistoricoBBSA;
typedef struct HistoricoBBSA {
    const Instancia *inst;
    struct EntradaHistoricoBBSA *entradas;
    size_t quantidade, capacidade;
    unsigned long long execucoes, reutilizacoes;
} HistoricoBBSA;

void liberarHistoricoBBSA(HistoricoBBSA *historico);
int refinarEliteComHistoricoSA(const Instancia *inst, Solucao melhores[NUM_MELHORES_SA],
                             int *quantidade, unsigned long long maxNos,
                             HistoricoBBSA *historico);

/* Escolhe uniformemente uma elite ainda nao refinada e com drones dominantes.
 * Retorna 1 se aplicou, 0 se nao ha elegiveis, -1 em falha. Refina em copia,
 * preserva o incumbente se nao melhorar e ordena/remove duplicatas do arquivo.
 * Marca a tentativa mesmo quando o orcamento nao prova otimalidade.
 * Esta interface usa historico temporario. O SA usa a variante ComHistorico
 * para conservar resultados entre chamadas. Alvo do BB: tempoCaminhao. */
int refinarEliteAleatoriaSA(const Instancia *inst, Solucao melhores[NUM_MELHORES_SA],
                           int *quantidade, unsigned long long maxNos);

/* solucao alocada por alocarSolucao para esta instancia:
 * entrada inicial, saida melhor encontrada, incluindo seu cronograma.
 * Esta interface utiliza tempInicial; a calibracao sera uma chamada separada.
 * Pesos iniciam em 1 a cada chamada e sao atualizados por temperatura.
 * Desativar pesosAdaptativos restaura a selecao uniforme.
 * A semente aleatoria deve ser inicializada uma unica vez pelo chamador.
 * Retorna 1 no sucesso (inclusive sem movimentos); 0 na falha, preservando
 * a solucao do chamador. Reconstroi a avaliacao inicial numa copia.
 * Mantem quatro melhores solucoes distintas durante a busca, considerando
 * todas as candidatas avaliadas; retorna a melhor delas em solucao.
 * Apos niveisSemMelhoraBB niveis sem melhora global, sorteia uma elite com
 * tempoDrones > tempoCaminhao e ainda elegivel para refinamento/reuso.
 * O historico evita repetir buscas para a mesma sequencia; tentativas limitadas
 * podem voltar com maior orcamento, ou quando uma meta anterior nao basta.
 * Se melhorar o melhor global, retoma a busca a partir dessa solucao.
 * Calibracao nao aplica BB. maxNosBB limita o esforco por chamada. */
int simulatedAnnealing(const Instancia *inst, Solucao *solucao,
                      const ParametrosSA *parametros);

#endif
