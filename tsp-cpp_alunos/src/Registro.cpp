#include "Registro.h"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstdlib>

// [FRAMEWORK - pronto, não precisa mexer]

void limpaArquivo(const std::string &nomeArquivo)
{
    std::ofstream arquivo(nomeArquivo, std::ios::trunc);
    if (!arquivo) {
        std::cerr << "O arquivo " << nomeArquivo << " nao pode ser aberto.\n";
        std::exit(EXIT_FAILURE);
    }
}

void registraProgresso(const std::string &nomeArquivo,
                        double tempoEmSegundos,
                        int iteracao,
                        double fo)
{
    std::ofstream arquivo(nomeArquivo, std::ios::app);
    if (!arquivo) {
        std::cerr << "O arquivo " << nomeArquivo << " nao pode ser aberto.\n";
        std::exit(EXIT_FAILURE);
    }

    arquivo << std::fixed << std::setprecision(2) << tempoEmSegundos << '\t'
            << std::setw(5) << iteracao << '\t'
            << std::setprecision(2) << fo << '\n';
}
