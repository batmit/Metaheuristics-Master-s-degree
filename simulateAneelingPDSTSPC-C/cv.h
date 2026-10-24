#ifndef CV_H
#define CV_H

#include "main.h"
void construirMatrizCaminhao(Instancia *inst);

void alocarTempoDrone(Instancia *inst);

int alocarMatrizCaminhao(Instancia *inst);

void alocarInstancias(Instancia *inst);

void alocarSolucao(Instancia *inst, Solucao *solucao);

void liberarSolucao(Solucao *solucao);

/* Requer solucao previamente alocada por alocarSolucao para esta instancia.
 * Constroi 0 -> 1 -> ... -> n-1 -> 0, sem missoes de drones, e avalia.
 * Retorna 1 no sucesso; 0 na falha, com makespan INFINITY. */
int construirSolucaoInicial(const Instancia *inst, Solucao *solucao);

/* atual deve conter cada cliente uma unica vez. vizinho deve estar alocado
 * por alocarSolucao para a mesma instancia, com vetores independentes.
 * Aplica troca, transferencia ou alteracao aleatoria da quantidade de drones
 * de uma missao, entre minDrones e numDrones com tempo viavel, sem modificar atual.
 * Retorna 1 no sucesso, com makespan INFINITY e horarios a reconstruir;
 * retorna 0 sem alterar vizinho se nao houver movimento ou os dados forem invalidos.
 * Inicializar srand uma unica vez no programa, nunca a cada chamada. */
int gerarVizinho(const Instancia *inst, const Solucao *atual, Solucao *vizinho);

/* Requer atual escalonada e avaliada, e ambas alocadas para esta instancia.
 * Instancia e solucao atual nao podem ter sido editadas desde a avaliacao.
 * Atualiza apenas as arestas afetadas e reescala o sufixo de drones alterado.
 * Retorna vizinho com makespan pronto; nao exige avaliacao completa adicional.
 * Falha de avaliacao deixa o vizinho invalidado. A atual permanece intacta. */
int gerarVizinhoAvaliado(const Instancia *inst, const Solucao *atual, Solucao *vizinho);

/* Processa missaoDrone na ordem do vetor, respeitando qtdDrones por missao.
 * Quantidade zero usa minDrones por cliente; quantidades invalidas falham.
 * Reconstroi os horarios desde o instante zero; nao calcula o makespan.
 * Retorna 1 no sucesso. Na falha, preserva as missoes anteriores.
 * Em ambos os casos invalida o makespan (INFINITY); reavaliar apos sucesso.
 * Pressupoe drones identicos com saida e retorno ao deposito. */
int escalonarMissoesDrone(const Instancia *inst, Solucao *solucao);

/* Branch and Bound exato para o subconjunto recebido, escolhendo ordem,
 * horarios e quantidade de drones entre minDrones e numDrones com tempo viavel.
 * qtdDrones e horarios de entrada sao ignorados; sao decisoes de saida.
 * Recebe apenas o vetor de missoes (nao a Solucao), sua quantidade e a
 * instancia que informa frota, elegibilidade e tempos por cliente/quantidade.
 * Retorna o makespan otimo dos drones e publica ordem/horarios no vetor.
 * Retorna INFINITY em dados invalidos ou falha de memoria, sem alterar o vetor;
 * conjunto vazio retorna zero. Sem limite de busca: custo exponencial.
 * Poda pela carga de drone-tempo / frota (desigualdade 16 do artigo),
 * usando consumo real nas escolhas fixadas e minimo viavel nas restantes.
 * Combina esse limite com makespan parcial e primeiro termino por missao.
 * Nao altera o conjunto de clientes atribuidos aos drones ou ao caminhao.
 * Se o vetor pertencer a Solucao, invalidar cronogramaValido/avaliacaoValida:
 * seus caches de prefixos nao sao reconstruidos por esta funcao. */
double escalonarMissoesDroneBB(const Instancia *inst, MissaoDrone *missoes,
                               int quantidade);

/* Caminhao: clientes sem os depositos inicial/final, implicitos no indice 0.
 * Drones: horarios em minutos e tempoDrone incluindo a missao completa.
 * Retorna 1 para solucao viavel; 0 e makespan INFINITY em caso de erro.
 * Nao escolhe horarios nem quantidades de drones. */
int calcularMakespan(const Instancia *inst, Solucao *solucao);


#endif
