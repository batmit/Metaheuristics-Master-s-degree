#include<stdio.h>
#include <stdlib.h>
#include "cv1.h"
#include<math.h>
#include<float.h>
#include<string.h>


//Utilizarei o multi start com o vizinho mais próximo como inicial e logo após 
// o 2-opt para refinar

struct cidade{

    double latitude, longitude;
    int visitado;

};

int CIDADES = 0;



int main(int argc, char *argv[]){
    FILE *arquivo;
    Cidade *vetorCidades = NULL;
    double **matrizDistancia = NULL;
    int *resultadoFinal = NULL;
    char linha[256];
    char tipoDistancia[32] = "";
    char formatoMatriz[32] = "";
    int secaoCoordenadas = 0;
    int secaoMatriz = 0;

    if(argc != 2){
        fprintf(stderr, "Uso: %s <arquivo.tsp>\n", argv[0]);
        return EXIT_FAILURE;
    }

    arquivo = fopen(argv[1], "r");
    if(arquivo == NULL){
        perror("Nao foi possivel abrir a instancia");
        return EXIT_FAILURE;
    }

    while(fgets(linha, sizeof(linha), arquivo) != NULL){
        char *valor = strchr(linha, ':');

        if(strncmp(linha, "DIMENSION", 9) == 0 && valor != NULL){
            CIDADES = atoi(valor + 1);
        }else if(strncmp(linha, "EDGE_WEIGHT_TYPE", 16) == 0 && valor != NULL){
            sscanf(valor + 1, "%31s", tipoDistancia);
        }else if(strncmp(linha, "EDGE_WEIGHT_FORMAT", 18) == 0 && valor != NULL){
            sscanf(valor + 1, "%31s", formatoMatriz);
        }else if(strncmp(linha, "NODE_COORD_SECTION", 18) == 0){
            secaoCoordenadas = 1;
            break;
        }else if(strncmp(linha, "EDGE_WEIGHT_SECTION", 19) == 0){
            secaoMatriz = 1;
            break;
        }
    }

    if(CIDADES <= 1 || (!secaoCoordenadas && !secaoMatriz)){
        fprintf(stderr, "Arquivo TSPLIB invalido ou formato nao suportado.\n");
        fclose(arquivo);
        return EXIT_FAILURE;
    }

    vetorCidades = calloc(CIDADES, sizeof(Cidade));
    matrizDistancia = malloc(CIDADES * sizeof(double *));
    resultadoFinal = malloc(CIDADES * sizeof(int));

    //verificação
    if(vetorCidades == NULL || matrizDistancia == NULL || resultadoFinal == NULL){
        fprintf(stderr, "Memoria insuficiente.\n");
        fclose(arquivo);
        free(vetorCidades);
        free(matrizDistancia);
        free(resultadoFinal);
        return EXIT_FAILURE;
    }

    for(int i = 0; i < CIDADES; i++){
        matrizDistancia[i] = calloc(CIDADES, sizeof(double));
        if(matrizDistancia[i] == NULL){
            fprintf(stderr, "Memoria insuficiente.\n");
            fclose(arquivo);
            for(int j = 0; j < i; j++) free(matrizDistancia[j]);
            free(matrizDistancia);
            free(vetorCidades);
            free(resultadoFinal);
            return EXIT_FAILURE;
        }
    }

    //Cálculo das distâncias
    if(secaoCoordenadas){
        for(int i = 0; i < CIDADES; i++){
            int identificador;
            if(fscanf(arquivo, "%d %lf %lf", &identificador,
                      &vetorCidades[i].latitude,
                      &vetorCidades[i].longitude) != 3){
                fprintf(stderr, "Coordenadas incompletas na cidade %d.\n", i + 1);
                goto erro;
            }
        }

        for(int i = 0; i < CIDADES; i++){
            for(int j = i + 1; j < CIDADES; j++){
                //calculo do deltax
                double dx = vetorCidades[i].latitude - vetorCidades[j].latitude;
                double dy = vetorCidades[i].longitude - vetorCidades[j].longitude;
                double distancia;

                if(strcmp(tipoDistancia, "EUC_2D") == 0){
                    //o floor arredonda para o inteiro mais próximo
                    distancia = floor(sqrt(dx * dx + dy * dy) + 0.5);
                }else if(strcmp(tipoDistancia, "ATT") == 0){
                    //ATT é distância pseudo-euclidiana
                    double valorReal = sqrt((dx * dx + dy * dy) / 10.0);
                    double arredondado = floor(valorReal + 0.5);
                    distancia = arredondado < valorReal ? arredondado + 1.0 : arredondado;
                }else if(strcmp(tipoDistancia, "GEO") == 0){
                    //utiliza distância GEO
                    const double pi = 3.141592;
                    double grausXi = floor(vetorCidades[i].latitude);
                    double grausYi = floor(vetorCidades[i].longitude);
                    double grausXj = floor(vetorCidades[j].latitude);
                    double grausYj = floor(vetorCidades[j].longitude);
                    double lati = pi * (grausXi + 5.0 * (vetorCidades[i].latitude - grausXi) / 3.0) / 180.0;
                    double loni = pi * (grausYi + 5.0 * (vetorCidades[i].longitude - grausYi) / 3.0) / 180.0;
                    double latj = pi * (grausXj + 5.0 * (vetorCidades[j].latitude - grausXj) / 3.0) / 180.0;
                    double lonj = pi * (grausYj + 5.0 * (vetorCidades[j].longitude - grausYj) / 3.0) / 180.0;
                    double q1 = cos(loni - lonj);
                    double q2 = cos(lati - latj);
                    double q3 = cos(lati + latj);
                    distancia = floor(6378.388 * acos(0.5 * ((1.0 + q1) * q2 - (1.0 - q1) * q3)) + 1.0);
                }else{
                    fprintf(stderr, "EDGE_WEIGHT_TYPE nao suportado: %s\n", tipoDistancia);
                    goto erro;
                }
                matrizDistancia[i][j] = distancia;
                matrizDistancia[j][i] = distancia;
            }
        }
    }else if(strcmp(formatoMatriz, "FULL_MATRIX") == 0){
        //Já vem a matriz distâncias completa, apenas leio
        for(int i = 0; i < CIDADES; i++)
            for(int j = 0; j < CIDADES; j++)
                if(fscanf(arquivo, "%lf", &matrizDistancia[i][j]) != 1) goto matriz_invalida;
    }else if(strcmp(formatoMatriz, "UPPER_ROW") == 0){
        //vem com a matriz distância triangular superior
        for(int i = 0; i < CIDADES; i++){
            for(int j = i + 1; j < CIDADES; j++){
                if(fscanf(arquivo, "%lf", &matrizDistancia[i][j]) != 1) goto matriz_invalida;
                matrizDistancia[j][i] = matrizDistancia[i][j];
            }
        }
    }else if(strcmp(formatoMatriz, "LOWER_DIAG_ROW") == 0){
        //vem com a matriz de distâncias completa inferior
        for(int i = 0; i < CIDADES; i++){
            for(int j = 0; j <= i; j++){
                if(fscanf(arquivo, "%lf", &matrizDistancia[i][j]) != 1) goto matriz_invalida;
                matrizDistancia[j][i] = matrizDistancia[i][j];
            }
        }
    }else{
        fprintf(stderr, "EDGE_WEIGHT_FORMAT nao suportado: %s\n", formatoMatriz);
        goto erro;
    }

    fclose(arquivo);
    arquivo = NULL;

    resultadoFinal[0] = 0;
    vetorCidades[0].visitado = 1;
    int cidadeMaisProxima = 1;
    printf("Instancia: %s\n", argv[1]);
    printf("Cidades: %d\n", CIDADES);
    //Calculo qual a cidade mais próxima, para realizar um ciclo inicial

    for(int i = 2; i < CIDADES; i++){
        if(matrizDistancia[0][i] < matrizDistancia[0][cidadeMaisProxima])
            cidadeMaisProxima = i;
    }
    resultadoFinal[1] = cidadeMaisProxima;
    vetorCidades[cidadeMaisProxima].visitado = 1;

    vizinhoMaisProximo(vetorCidades, matrizDistancia, resultadoFinal, 2);
    printf("Rota encontrada para o vizinho mais proximo: ");

    for(int i = 0; i < CIDADES; i++){

        printf("%d ", resultadoFinal[i]);

    }
    double custo = 0.0;
    for(int i = 0; i < CIDADES; i++)
        custo += matrizDistancia[resultadoFinal[i]][resultadoFinal[(i + 1) % CIDADES]];

    printf("\nCusto: %lf\n", custo);

    //Resetar as matrizes para começar a heurístca de inserção mais barata

    for(int i = 1; i < CIDADES; i++){

        resultadoFinal[i] = 0;
        vetorCidades[i].visitado = 0;


    }
    resultadoFinal[1] = cidadeMaisProxima;

    vetorCidades[cidadeMaisProxima].visitado = 1;



    heuristicaInsercaoBarata(vetorCidades, matrizDistancia, resultadoFinal, 2);

    //calculo o cussto com base no resultado final(lembrando que pode ser cíclico)
    custo = 0.0;
    for(int i = 0; i < CIDADES; i++)
        custo += matrizDistancia[resultadoFinal[i]][resultadoFinal[(i + 1) % CIDADES]];


    printf("Custo da insercao barata: %.0f\n", custo);
    printf("Rota (identificadores TSPLIB): ");
    for(int i = 0; i < CIDADES; i++) printf("%d ", resultadoFinal[i] + 1);
    printf("%d\n", resultadoFinal[0] + 1);

    for(int i = 0; i < CIDADES; i++) free(matrizDistancia[i]);
    free(matrizDistancia);
    free(vetorCidades);
    free(resultadoFinal);
    return EXIT_SUCCESS;

matriz_invalida:
    fprintf(stderr, "Matriz de distancias incompleta.\n");
erro:
    if(arquivo != NULL) fclose(arquivo);
    for(int i = 0; i < CIDADES; i++) free(matrizDistancia[i]);
    free(matrizDistancia);
    free(vetorCidades);
    free(resultadoFinal);
    return EXIT_FAILURE;
}






void vizinhoMaisProximo(Cidade *vetorCidades, double **matrizDistancia, int *resultadoFinal, int posInicial){

    double soma = 0;
    int cont = posInicial - 1;

    int atual = resultadoFinal[posInicial - 1];
    //resultadoFinal[0] = 0;
    for(int pos = posInicial; pos < CIDADES; pos++){
        double distancia = DBL_MAX;
        int cidade = -1;

        for(int j = 1; j < CIDADES; j++){

            if(matrizDistancia[atual][j] < distancia && vetorCidades[j].visitado == 0){

                distancia = matrizDistancia[atual][j];
                cidade = j;
            }


        }

        if(cidade == -1){
            break;
        }
        resultadoFinal[pos] = cidade;
        soma+= distancia;
        vetorCidades[cidade].visitado = 1;
        cont++;
        atual = cidade;
        

    }

    soma+= matrizDistancia[atual][0];
    return soma;

}


void heuristicaInsercaoBarata(Cidade *vetorCidades, double **matrizDistancia, int *resultadoFinal, int nivel){




    //Como nesse código eu preciso de um ciclo mínimo, no caso de 2 cidades, 
    //eu vou pegar a primeira e também a mais próxima da primeira
    //resultadoFinal[0] = 0;
    //vetorCidades[0].visitado = 1;
    //double menosDist = matrizDistancia[0][1];
    //int cidade = 1;
    //apenas  acho a cidade mais proxima da primeira
    //for(int i = 2; i < CIDADES; i++){

    //    if(matrizDistancia[0][i] < menosDist){

    //        menosDist = matrizDistancia[0][i];
    //        cidade = i;

    //    }


    //}
    //vetorCidades[cidade].visitado = 1;
    //resultadoFinal[1] = cidade;
    int tamCiclo = nivel;
    //coloco a cidade 0 e a mais proxima dela no ciclo e começo

    //Enquanto o ciclo for menor que o numero de cidades total
    while(tamCiclo < CIDADES){

        //variáveis para eu achar a melhor cidade na melhor posição
        int melhorCidade = -1;
        int melhorPosicao = -1;
        double melhorDelta = DBL_MAX;

        for(int k = 0; k < CIDADES; k++){

            //se não tiver sido visitado
            if(vetorCidades[k].visitado == 0){


                //vou passando por todas as posições do ciclo
                for(int pos = 0; pos < tamCiclo; pos++){

                    //pego a cidade na posicao pos
                    int i = resultadoFinal[pos];
                    int j;

                    if(pos == tamCiclo - 1){
                        // se pegarmos a última cidade, a próxima será a primeira(estamos em um ciclo)

                        j = resultadoFinal[0];

                    }else{
                        //Senão apenas pegamos a próxima
                        j = resultadoFinal[pos+1];

                    }

                    //Eu vou inserir entre o i e o j, nesse caso eu estou tentando calcular o acréscimo da distância, subtraindo pela distancia
                    //entre o i e o j, que era o que estava antes
                    double delta = matrizDistancia[i][k] + matrizDistancia[k][j] - matrizDistancia[i][j];

                    if(delta < melhorDelta){
                        //salvo a cidade e a posição
                        melhorDelta = delta;
                        melhorCidade = k;
                        melhorPosicao = pos;

                    }

                }


            }

        }
        //empurro todos os valores para colocar na posição certa
        for(int m = tamCiclo; m > melhorPosicao + 1; m--){
            resultadoFinal[m] = resultadoFinal[m - 1];
        }

        resultadoFinal[melhorPosicao + 1] = melhorCidade;
        vetorCidades[melhorCidade].visitado = 1;
        tamCiclo++;

    }

}



double distanciaEuclidiana(double x1, double y1, double x2, double y2){

    double  resposta;


    resposta = ((x1 - y1)*(x1 - y1)) + ((x2 - y2) * (x2 - y2));
    resposta = sqrt(resposta);



    return resposta;
}
