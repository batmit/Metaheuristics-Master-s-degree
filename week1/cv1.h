#ifndef CV_2_H
#define CV_2_H

extern int CIDADES;

typedef struct cidade Cidade;

double distanciaEuclidiana(double x1, double y1, double x2, double y2);

void branchAndBound(Cidade *vetorCidades, int nivel, int caminho[], double custoAtual, double **matrizDistancia, double *melhorCusto, int resultadoFinal[], int resultadoParcial[], double **matrizAtual, double limiteAtual, int **vizinhosOrdenados);

double calcularLimiteInferior(Cidade *vetorCidades, int caminho[], int nivel, double custoAtual, double **matrizDistancia);
void heuristicaInsercaoBarata(Cidade *vetorCidades, double **matrizDistancia, int *resultadoFinal, int nivel);
double multiStart(Cidade *vetorCidades,double **matrizDistancia, int *resultadoFinal);

void ordenarVizinhos(int **vizinhosOrdenados, Cidade *vetorCidades, double **matrizDistancia);
double **copiarMatriz(double **original);
void liberarMatriz(double **matriz);


void vizinhoMaisProximo(Cidade *vetorCidades, double **matrizDistancia, int* resultadoFinal, int posicaoInicial);
void twoopt(Cidade *vetorCidades, double **matrizDistancia, int *resultadoFinal);
void inverterTrecho(int *rota, int inicio, int fim);
double calcularCusto(double **matrizDistancia, int *resultado);
double reduzirMatriz(double **m);

#endif
