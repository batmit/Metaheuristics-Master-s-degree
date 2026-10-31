#ifndef CV_H
#define CV_H

#include "main.h"

typedef enum TipoMovimento {
    TROCAR_CAMINHAO, TROCAR_DRONES, PARA_DRONES, PARA_CAMINHAO,
    ALTERAR_QUANTIDADE_DRONES, REINSERIR_CAMINHAO, DOIS_OPT_CAMINHAO, NUM_MOVIMENTOS
} TipoMovimento;
int repararCargaDrone(const Instancia *, Solucao *);
int construirSolucaoInicialDiversa(const Instancia *, Solucao *);
int gerarVizinhoAvaliadoConfigurado(const Instancia *, const Solucao *, Solucao *, const double *, TipoMovimento *, bool);
void construirMatrizCaminhao(Instancia *inst);

void alocarTempoDrone(Instancia *inst);

int alocarMatrizCaminhao(Instancia *inst);

void alocarInstancias(Instancia *inst);

void alocarSolucao(Instancia *inst, Solucao *solucao);

void liberarSolucao(Solucao *solucao);

/* Requer solucao previamente alocada por alocarSolucao para esta instancia.
 * Constroi rota por vizinho mais proximo e melhora por 2-opt, sem missoes
 * de drones. Preserva a rota por indice caso ela seja melhor; avalia ao final.
 * Retorna 1 no sucesso; 0 na falha, com makespan INFINITY. */
int construirSolucaoInicial(const Instancia *inst, Solucao *solucao);

/* atual deve conter cada cliente uma unica vez. vizinho deve estar alocado
 * por alocarSolucao para a mesma instancia, com vetores independentes.
 * Aplica troca, reinsercao, 2-opt, transferencia ou alteracao de quantidade de drones
 * de uma missao, entre minDrones e numDrones com tempo viavel, sem modificar atual.
 * Retorna 1 no sucesso, com makespan INFINITY e horarios a reconstruir;
 * retorna 0 sem alterar vizinho se nao houver movimento ou os dados forem invalidos.
 * Inicializar srand uma unica vez no programa, nunca a cada chamada. */
int gerarVizinho(const Instancia *inst, const Solucao *atual, Solucao *vizinho);

/* Requer atual escalonada e avaliada, e ambas alocadas para esta instancia.
 * Instancia e solucao atual nao podem ter sido editadas desde a avaliacao.
 * Atualiza apenas as arestas afetadas e reescala o sufixo de drones alterado.
 * Retorna vizinho com makespan pronto; nao exige avaliacao completa adicional.
 * Falha de avaliacao deixa o vizinho invalidado. A atual permanece intacta.
 * Aceita cronogramas de BB sem cache de prefixos: mudancas apenas na rota
 * preservam os horarios; mudancas nos drones reconstroem o cronograma completo. */
int gerarVizinhoAvaliado(const Instancia *inst, const Solucao *atual, Solucao *vizinho);

/* Sorteia apenas entre tipos disponiveis, proporcionalmente aos pesos positivos
 * e finitos. NULL preserva sorteio uniforme. Informa o tipo no sucesso.
 * Pesos nao mudam o sorteio das posicoes/clientes dentro do movimento. */
int gerarVizinhoAvaliadoPonderado(const Instancia *inst, const Solucao *atual,
                                Solucao *vizinho, const double pesos[NUM_MOVIMENTOS],
                                TipoMovimento *tipo);

/* Processa missaoDrone na ordem do vetor, respeitando qtdDrones por missao.
 * Quantidade zero usa minDrones por cliente; quantidades invalidas falham.
 * Usa a mesma insercao do BB: primeiro intervalo livre com capacidade,
 * preservando a ordem de processamento (nao exige partidas nessa ordem).
 * Reconstroi os horarios desde o instante zero e os maximos por prefixo;
 * nao calcula o makespan.
 * Retorna 1 no sucesso. Na falha, preserva as missoes anteriores.
 * Em ambos os casos invalida o makespan (INFINITY); reavaliar apos sucesso.
 * Pressupoe drones identicos com saida e retorno ao deposito. */
int escalonarMissoesDrone(const Instancia *inst, Solucao *solucao);

/* Branch and Bound sobre quantidades de drones entre minDrones e numDrones
 * com tempo viavel, mantendo os clientes na ordem do vetor recebido.
 * Cada combinacao e escalonada por insercao no primeiro intervalo viavel.
 * Ordem de processamento fixa nao impoe ordem cronologica de partida.
 * As quantidades recebidas nao restringem a busca; se viaveis, fornecem um
 * incumbente inicial reconstruido. Horarios sao recalculados e sao saida.
 * Recebe apenas o vetor de missoes (nao a Solucao), sua quantidade e a
 * instancia que informa frota, elegibilidade e tempos por cliente/quantidade.
 * Retorna o menor makespan entre essas combinacoes e publica quantidades e
 * horarios no vetor, sem alterar a ordem. Nao busca o otimo de outras ordens.
 * Retorna INFINITY em dados invalidos ou falha de memoria, sem alterar o vetor;
 * conjunto vazio retorna zero. Sem limite de busca: custo exponencial.
 * Poda pela carga de drone-tempo / frota (desigualdade 16 do artigo),
 * usando consumo real nas escolhas fixadas e minimo viavel nas restantes.
 * Combina esse limite com makespan parcial e primeiro termino por missao.
 * Nao altera o conjunto de clientes atribuidos aos drones ou ao caminhao.
 * Se o vetor pertencer a Solucao, invalidar cronogramaValido/avaliacaoValida:
 * seus caches de prefixos nao sao reconstruidos por esta funcao.
 * escalonarMissoesDrone reproduz os horarios para a mesma ordem/quantidades
 * e reconstroi esses caches usando a mesma regra compartilhada. */
double escalonarMissoesDroneBB(const Instancia *inst, MissaoDrone *missoes,
                               int quantidade);

typedef enum MotivoTerminoBB {
    BB_ERRO, BB_OTIMO, BB_META_ATINGIDA, BB_LIMITE_NOS
} MotivoTerminoBB;

typedef struct ResultadoBB {
    MotivoTerminoBB motivo;
    unsigned long long nos;
    double limiteInferior;
} ResultadoBB;

/* Meta >= 0 permite parar quando Cmax_drones <= tempo do caminhao.
 * Meta=-1 desativa parada por alvo. Otimalidade aqui e das quantidades sob
 * ordem/regra fixas. Atingir meta nao afirma otimo isolado dos drones.
 * Reconstroi tambem o cronograma recebido como incumbente inicial, se viavel. */
double escalonarMissoesDroneBBMeta(const Instancia *inst, MissaoDrone *missoes,
                                 int quantidade, unsigned long long maxNos,
                                 double meta, ResultadoBB *resultado);

/* Versao com orcamento de nos para uso dentro do SA. Retorna a melhor solucao
 * encontrada; otimoProvado informa se a busca terminou sem atingir o limite.
 * maxNos=0 significa busca sem limite. Mesma ordem fixa da versao exata.
 * Usa soma de carga por sufixo O(1), perfil de eventos e ramos por menor termino. */
double escalonarMissoesDroneBBLimitado(const Instancia *inst, MissaoDrone *missoes,
                                     int quantidade, unsigned long long maxNos,
                                     bool *otimoProvado);

/* Caminhao: clientes sem os depositos inicial/final, implicitos no indice 0.
 * Drones: horarios em minutos e tempoDrone incluindo a missao completa.
 * Retorna 1 para solucao viavel; 0 e makespan INFINITY em caso de erro.
 * Nao escolhe horarios nem quantidades de drones. */
int calcularMakespan(const Instancia *inst, Solucao *solucao);


#endif
