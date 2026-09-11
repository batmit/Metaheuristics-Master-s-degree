#include "BuscaLocal.h"
#include "Utilitarios.h"
#include "Registro.h"
#include "Aleatorio.h"

#include <numeric>   // std::iota
#include <algorithm> // std::swap

// ============================================================================
// [EXERCÍCIO]
// ============================================================================
double calculaDelta(const Instancia &inst, const std::vector<int> &s, int i, int j)
{
    int n = inst.n;
    int iAntes = (i == 0) ? n - 1 : i - 1;
    int iDepois = (i == n - 1) ? 0 : i + 1;
    int jAntes = (j == 0) ? n - 1 : j - 1;
    int jDepois = (j == n - 1) ? 0 : j + 1;

    return inst.d[s[iAntes]][s[i]] + inst.d[s[i]][s[iDepois]]
         + inst.d[s[jAntes]][s[j]] + inst.d[s[j]][s[jDepois]];
}


// ============================================================================
// [EXERCÍCIO]
// ============================================================================
double descida(const Instancia &inst, std::vector<int> &s)
{
    
    return descidaCompleta(inst, s);

}


// ============================================================================
// [EXERCÍCIO]
// ============================================================================
double melhorVizinho(const Instancia &inst, std::vector<int> &s, double fo,
                      int &melhorI, int &melhorJ)
{
    
    double foMelhorVizinho = fo;

    //TODO

    return foMelhorVizinho;
}

// ============================================================================
// [EXERCÍCIO]
// ============================================================================
double descidaCompleta(const Instancia &inst, std::vector<int> &s)
{
    const std::string arquivoLog = "DescidaCompleta.txt";
    double fo = custo(inst, s);

    limpaArquivo(arquivoLog);
    registraProgresso(arquivoLog, 0.0, 0, fo);

    bool melhorou;
    
    //TODO

    return fo;
}

// ============================================================================
// [FRAMEWORK]
// ============================================================================
double vizinhoRandomico(const Instancia &inst, std::vector<int> &s, double fo,
                      int &melhorI, int &melhorJ)
{
    int n = inst.n;
    
    int j = inteiroAleatorio(0, n - 1);
    int i;
    do {
        i = inteiroAleatorio(0, n - 1);
    } while (i == j);

    double delta1 = calculaDelta(inst, s, i, j);
    std::swap(s[i], s[j]);
    double delta2 = calculaDelta(inst, s, i, j);
    double foVizinho = fo - delta1 + delta2;

    std::swap(s[i], s[j]); // desfaz o movimento

    melhorI = i;
    melhorJ = j;

    return foVizinho;
}

// ============================================================================
// [EXERCÍCIO]
// ============================================================================
double descidaRandomica(const Instancia &inst, std::vector<int> &s, int iterMax)
{
    int n = inst.n;
    double fo = custo(inst, s);
    int iter = 0;

    
    return fo;
}

// ============================================================================
// [EXERCÍCIO]
// ============================================================================
double vizinhoPrimeiroMelhora(const Instancia &inst, std::vector<int> &s, double fo,
                               int &melhorI, int &melhorJ)
{
    int n = inst.n;
    double foMelhorVizinho = fo;
    bool melhorou = false;

    // Visita as posições da rota em ordem aleatória, para não introduzir
    // um viés sistemático de sempre explorar as posições iniciais primeiro.
    std::vector<int> ordem(n);
    std::iota(ordem.begin(), ordem.end(), 0);
    embaralhaVetor(ordem);

    //TODO

    return foMelhorVizinho;
}

// ============================================================================
// [EXERCÍCIO]
// ============================================================================
double descidaPrimeiroMelhora(const Instancia &inst, std::vector<int> &s)
{
    const std::string arquivoLog = "DescidaPrimeiroMelhora.txt";
    double fo = custo(inst, s);

    limpaArquivo(arquivoLog);
    registraProgresso(arquivoLog, 0.0, 0, fo);

    bool melhorou;

    //TODO

    return fo;
}
