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
 * Aplica uma troca ou transferencia aleatoria sem modificar atual.
 * Retorna 1 no sucesso, com makespan INFINITY e horarios a reconstruir;
 * retorna 0 sem alterar vizinho se nao houver movimento ou os dados forem invalidos.
 * Inicializar srand uma unica vez no programa, nunca a cada chamada. */
int gerarVizinho(const Instancia *inst, const Solucao *atual, Solucao *vizinho);

/* Processa missaoDrone na ordem do vetor, usando minDrones por cliente.
 * Reconstroi os horarios desde o instante zero; nao calcula o makespan.
 * Retorna 1 no sucesso. Na falha, preserva as missoes anteriores.
 * Em ambos os casos invalida o makespan (INFINITY); reavaliar apos sucesso.
 * Pressupoe drones identicos com saida e retorno ao deposito. */
int escalonarMissoesDrone(const Instancia *inst, Solucao *solucao);

/* Caminhao: clientes sem os depositos inicial/final, implicitos no indice 0.
 * Drones: horarios em minutos e tempoDrone incluindo a missao completa.
 * Retorna 1 para solucao viavel; 0 e makespan INFINITY em caso de erro.
 * Nao escolhe horarios nem quantidades de drones. */
int calcularMakespan(const Instancia *inst, Solucao *solucao);


#endif
