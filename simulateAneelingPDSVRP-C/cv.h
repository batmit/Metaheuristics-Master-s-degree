#ifndef CV_H
#define CV_H

#include "main.h"

/* Instancia deve ter clientes carregados. Retornam 1 no sucesso, 0 na falha. */
int alocarMatrizCaminhao(Instancia *inst);
int alocarTempoDrone(Instancia *inst);
int alocarInstancias(Instancia *inst);
/* Distancia Manhattan, coordenadas em km e velocidade em km/h; saida em min. */
int construirMatrizCaminhao(Instancia *inst);

/* Inicializar solucao com {0}. Cada rota reserva n-1 clientes.
 * Apenas aloca armazenamento: nao constroi nem avalia uma solucao.
 * Liberar antes de alocar novamente; na falha, preserva a saida. */
int alocarSolucao(const Instancia *inst, Solucao *solucao);
void liberarSolucao(Solucao *solucao);

typedef enum TipoMovimento { TROCAR_POSICOES, TRANSFERIR_CLIENTE, ALTERAR_QUANTIDADE_DRONES,
                            DOIS_OPT_CAMINHAO, REINSERIR_CAMINHAO } TipoMovimento;
/* Destino ja alocado e independente; copia profunda. */
int copiarSolucao(const Instancia *inst, const Solucao *origem, Solucao *destino);
/* Insercao pelo menor makespan resultante; melhora cada rota por 2-opt. */
int construirSolucaoInicial(const Instancia *inst, Solucao *solucao);
typedef struct ResultadoBB {
    unsigned long long nos;
    double limiteInferior;
    bool otimoProvado;
    bool metaAtingida;
    bool limiteNosAtingido;
} ResultadoBB;
/* Busca exata sobre ordem com quantidade de drones FIXA por missao, sem sorteios
 * e sem limite de nos. Decodificacao serial pelo primeiro intervalo viavel.
 * Preserva k de cada cliente; ignora horarios recebidos.
 * Publica apenas no sucesso. Custo exponencial; vazio tem makespan zero. */
int escalonarMissoesDroneBB(const Instancia *inst, Solucao *solucao,
                           ResultadoBB *resultado);
/* Para ao obter cronograma <= meta (makespan global ja definido pelos
 * caminhoes). Meta=-1 exige otimo isolado dos drones. */
int escalonarMissoesDroneBBMeta(const Instancia *inst, Solucao *solucao,
                              double meta, ResultadoBB *resultado);
/* maxNos=0 sem limite; retorna melhor cronograma mesmo sem prova. */
int escalonarMissoesDroneBBLimitado(const Instancia *inst, Solucao *solucao,
                                  double meta, unsigned long long maxNos,
                                  ResultadoBB *resultado);
/* Construcao gulosa deterministica, sem ramificacao; usada nas iteracoes. */
int escalonarMissoesDrone(const Instancia *inst, Solucao *solucao);
/* Avaliacao integral, valida cobertura, duracoes e capacidade. */
int calcularMakespan(const Instancia *inst, Solucao *solucao);
/* Entrada avaliada, saida alocada independente. 1 sucesso, 0 sem movimento,
 * -1 erro. Categorias disponiveis equiprovaveis; trocas dentro de caminhoes
 * ou transferencia cliente-destino viavel, ou alteracao aleatoria de k.
 * BB decide apenas ordem/horarios. Usa deltas das rotas e preserva
 * cronograma quando clientes e quantidades dos drones nao mudam. */
int gerarVizinhoAvaliado(const Instancia *inst, const Solucao *atual,
                        Solucao *vizinho, TipoMovimento *tipo);

#endif
