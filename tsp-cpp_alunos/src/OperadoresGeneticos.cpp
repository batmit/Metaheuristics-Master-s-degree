#include "OperadoresGeneticos.h"
#include "Utilitarios.h"
#include "Aleatorio.h"

#include <algorithm> // std::find, std::swap
#include <limits>
#include <numeric>

// ============================================================================
// [EXERCÍCIO]
// ============================================================================
int roleta(const std::vector<double> &foPop)
{
    int nind = static_cast<int>(foPop.size());

    double foMin = *std::min_element(foPop.begin(), foPop.end());
    double foMax = *std::max_element(foPop.begin(), foPop.end());
    double tgAlfa = 100.0 / (foMax - foMin);

    // Aptidão: quanto MENOR a fo (mais curta a rota), MAIOR a fatia da
    // roleta -- por isso a fórmula usa (foMax - foPop[j]).
    std::vector<double> aptidao(nind);
    double soma = 0.0;
    for (int j = 0; j < nind; j++) {
        aptidao[j] = tgAlfa * (foMax - foPop[j]);
        soma += aptidao[j];
    }

    // "escala" acumula as fatias da roleta: escala[j] é o limite superior
    // da fatia do indivíduo j (as fatias juntas cobrem o intervalo [0,1]).
    std::vector<double> escala(nind);
    escala[0] = aptidao[0] / soma;
    for (int j = 1; j < nind; j++) {
        escala[j] = escala[j - 1] + aptidao[j] / soma;
    }

    double giro = realAleatorio(0.0, 1.0);
    int j = 0;
    while (escala[j] < giro) j++;
    return j;
}

// ============================================================================
// [EXERCÍCIO]
// ============================================================================
int roletaScaling(const std::vector<double> &foPop)
{
    int nind = static_cast<int>(foPop.size());
    double alfa = 2.0;

    double media = std::accumulate(foPop.begin(), foPop.end(), 0.0) / nind;
    double desvio = desvioPadrao(foPop) * media;

    std::vector<double> aptidao(nind);
    double soma = 0.0;
    for (int j = 0; j < nind; j++) {
        aptidao[j] = media + alfa * desvio - foPop[j];
        if (aptidao[j] < 0) aptidao[j] = 0;
        soma += aptidao[j];
    }

    std::vector<double> escala(nind);
    escala[0] = aptidao[0] / soma;
    for (int j = 1; j < nind; j++) {
        escala[j] = escala[j - 1] + aptidao[j] / soma;
    }

    double giro = realAleatorio(0.0, 1.0);
    int j = 0;
    while (escala[j] < giro) j++;
    return j;
}

// ============================================================================
// [EXERCÍCIO]
// ============================================================================
void mutacao(std::vector<int> &s)
{
    int n = static_cast<int>(s.size());
    int i = inteiroAleatorio(0, n - 1);
    int j;
    do {
        j = inteiroAleatorio(0, n - 1);
    } while (i == j);

    std::swap(s[i], s[j]);
}

// ============================================================================
// [EXERCÍCIO]
// ============================================================================
void crossoverOX(const std::vector<int> &pai1, const std::vector<int> &pai2,
                  std::vector<int> &filho1, std::vector<int> &filho2)
{
    int n = static_cast<int>(pai1.size());
    filho1.assign(n, -1);
    filho2.assign(n, -1);

    int corte1 = inteiroAleatorio(2, (n - 1) / 2);
    int corte2 = inteiroAleatorio((n + 1) / 2, n - 3);

    // Copia o segmento entre os dois pontos de corte diretamente.
    for (int i = corte1; i <= corte2; i++) {
        filho1[i] = pai1[i];
        filho2[i] = pai2[i];
    }

    int tamanhoLista = n - (corte2 - corte1 + 1);
    std::vector<int> listaPai1, listaPai2;
    listaPai1.reserve(tamanhoLista);
    listaPai2.reserve(tamanhoLista);

    // Percorre pai1, a partir de logo após o segundo ponto de corte
    // (dando a volta ciclicamente), coletando as cidades que ainda não
    // estão no filho2 -- serão usadas para completar o filho1.
    int i = corte2 + 1;
    while (static_cast<int>(listaPai1.size()) < tamanhoLista) {
        bool jaEsta = false;
        for (int k = corte1; k <= corte2; k++) {
            if (filho2[k] == pai1[i]) { jaEsta = true; break; }
        }
        if (!jaEsta) listaPai1.push_back(pai1[i]);
        i = (i == n - 1) ? 0 : i + 1;
    }

    i = corte2 + 1;
    while (static_cast<int>(listaPai2.size()) < tamanhoLista) {
        bool jaEsta = false;
        for (int k = corte1; k <= corte2; k++) {
            if (filho1[k] == pai2[i]) { jaEsta = true; break; }
        }
        if (!jaEsta) listaPai2.push_back(pai2[i]);
        i = (i == n - 1) ? 0 : i + 1;
    }

    // Completa as posições restantes de cada filho, na mesma ordem
    // cíclica, com as cidades coletadas do outro pai.
    int j = corte2 + 1;
    for (int k = 0; k < tamanhoLista; k++) {
        filho1[j] = listaPai2[k];
        filho2[j] = listaPai1[k];
        j = (j == n - 1) ? 0 : j + 1;
    }
}

namespace {

// Marca, em todas as linhas da tabela de arestas, que a cidade
// "proximaCidade" já foi usada -- remove-a da lista de vizinhos
// disponíveis de qualquer outra cidade que ainda a tivesse como vizinha,
// e atualiza a contagem de arestas restantes (posição 0 de cada linha).
void atualizaArestas(std::vector<std::vector<int>> &arestas, int proximaCidade)
{
    int n = static_cast<int>(arestas.size());
    int m = static_cast<int>(arestas[0].size());

    for (int i = 0; i < n; i++) {
        for (int j = 1; j < m; j++) {
            if (arestas[i][j] == proximaCidade) {
                arestas[i][j] = -1;
                arestas[i][0]--;
            }
        }
    }
}

// Constrói, para o pai dado, a tabela de arestas: para cada cidade,
// registra as até 4 cidades adjacentes a ela nos dois pais (removendo
// duplicatas). arestas1 e arestas2 começam idênticas (uma cópia para
// construir cada filho, já que a construção de um filho "consome" as
// arestas da tabela).
void constroiTabelaDeArestas(const std::vector<int> &pai1, const std::vector<int> &pai2,
                              std::vector<std::vector<int>> &arestas1,
                              std::vector<std::vector<int>> &arestas2)
{
    int n = static_cast<int>(pai1.size());
    const int m = 5; // posição 0 = nº de arestas; posições 1..4 = vizinhos
    arestas1.assign(n, std::vector<int>(m, 0));
    arestas2.assign(n, std::vector<int>(m, 0));

    for (int cidade = 0; cidade < n; cidade++) {
        int posP1 = static_cast<int>(std::find(pai1.begin(), pai1.end(), cidade) - pai1.begin());
        int posP2 = static_cast<int>(std::find(pai2.begin(), pai2.end(), cidade) - pai2.begin());

        int j = 1;
        auto adiciona = [&](int vizinho) {
            arestas1[cidade][j] = vizinho;
            arestas2[cidade][j] = vizinho;
            j++;
        };

        // Vizinhos da cidade no pai1 (anterior e posterior, cíclico).
        if (posP1 == 0) {
            adiciona(pai1[posP1 + 1]);
            adiciona(pai1[n - 1]);
        } else if (posP1 == n - 1) {
            adiciona(pai1[posP1 - 1]);
            adiciona(pai1[0]);
        } else {
            adiciona(pai1[posP1 - 1]);
            adiciona(pai1[posP1 + 1]);
        }

        // Vizinhos da cidade no pai2 (anterior e posterior, cíclico).
        if (posP2 == 0) {
            adiciona(pai2[posP2 + 1]);
            adiciona(pai2[n - 1]);
        } else if (posP2 == n - 1) {
            adiciona(pai2[posP2 - 1]);
            adiciona(pai2[0]);
        } else {
            adiciona(pai2[posP2 - 1]);
            adiciona(pai2[posP2 + 1]);
        }

        // Remove vizinhos repetidos (a mesma cidade aparecendo em ambos os pais).
        int nArestas = j - 1;
        if (arestas1[cidade][3] == arestas1[cidade][1] || arestas1[cidade][3] == arestas1[cidade][2]) {
            arestas1[cidade][3] = arestas2[cidade][3] = -1;
            nArestas--;
        }
        if (arestas1[cidade][4] == arestas1[cidade][1] || arestas1[cidade][4] == arestas1[cidade][2]) {
            arestas1[cidade][4] = arestas2[cidade][4] = -1;
            nArestas--;
        }
        arestas1[cidade][0] = arestas2[cidade][0] = nArestas;
    }
}

// Constrói um filho a partir da tabela de arestas: parte da cidade
// inicial e, a cada passo, escolhe entre as cidades adjacentes à última
// inserida aquela com MENOS arestas restantes (heurística gulosa da
// ERX). Se nenhuma estiver disponível, sorteia uma cidade qualquer ainda
// não visitada.
void constroiFilhoERX(std::vector<std::vector<int>> &arestas, int cidadeInicial,
                       std::vector<int> &filho)
{
    int n = static_cast<int>(arestas.size());
    const int m = static_cast<int>(arestas[0].size());
    filho.assign(n, -1);

    filho[0] = cidadeInicial;
    atualizaArestas(arestas, cidadeInicial);

    for (int i = 1; i < n; i++) {
        int cidadeRota = filho[i - 1];
        int numArestasMin = std::numeric_limits<int>::max();
        int proximaCidade = -1;

        for (int j = 1; j < m; j++) {
            int cidadeCandidata = arestas[cidadeRota][j];
            if (cidadeCandidata == -1) continue;

            if (arestas[cidadeCandidata][0] < numArestasMin) {
                numArestasMin = arestas[cidadeCandidata][0];
                proximaCidade = cidadeCandidata;
            } else if (arestas[cidadeCandidata][0] == numArestasMin && cidadeCandidata < proximaCidade) {
                proximaCidade = cidadeCandidata;
            }
        }

        if (proximaCidade == -1) {
            // Nenhum vizinho disponível: sorteia uma cidade ainda não
            // usada no filho.
            int cidadeAleatoria;
            do {
                cidadeAleatoria = inteiroAleatorio(0, n - 1);
            } while (std::find(filho.begin(), filho.begin() + i, cidadeAleatoria) != filho.begin() + i);
            proximaCidade = cidadeAleatoria;
        }

        filho[i] = proximaCidade;
        atualizaArestas(arestas, proximaCidade);
    }
}

} // namespace (funções auxiliares internas da ERX)

// ============================================================================
// [EXERCÍCIO]
// ============================================================================
void crossoverERX(const std::vector<int> &pai1, const std::vector<int> &pai2,
                   std::vector<int> &filho1, std::vector<int> &filho2)
{
    std::vector<std::vector<int>> arestas1, arestas2;
    constroiTabelaDeArestas(pai1, pai2, arestas1, arestas2);

    constroiFilhoERX(arestas1, pai1[0], filho1);
    constroiFilhoERX(arestas2, pai2[0], filho2);
}
