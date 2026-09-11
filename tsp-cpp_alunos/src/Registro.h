#ifndef REGISTRO_H
#define REGISTRO_H

#include <string>

/*
 * ============================================================================
 *  Módulo: Registro
 * ============================================================================
 *  Vários métodos (Descida, Busca Tabu, Simulated Annealing, ...) gravam,
 *  a cada melhora, uma linha em um arquivo de texto no formato:
 *
 *      <tempo_em_segundos>   <iteracao>   <fo_corrente>
 *
 *  Esse arquivo é útil depois para plotar a curva de convergência do
 *  método (fo x tempo, ou fo x iteração) em uma planilha ou script Python.
 * ============================================================================
 */

// [FRAMEWORK]
// Apaga o conteúdo do arquivo (ou cria um arquivo vazio, se não existir).
// Chame no início de cada método, antes de começar a gravar o progresso.
void limpaArquivo(const std::string &nomeArquivo);

// [FRAMEWORK]
// Acrescenta uma linha "tempo iteracao fo" ao final do arquivo.
void registraProgresso(const std::string &nomeArquivo,
                        double tempoEmSegundos,
                        int iteracao,
                        double fo);

#endif // REGISTRO_H
