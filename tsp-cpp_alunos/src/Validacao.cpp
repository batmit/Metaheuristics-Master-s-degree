#include "Validacao.h"
#include "Utilitarios.h"

#include <iostream>
#include <cmath>

// [FRAMEWORK - pronto, não precisa mexer]

bool validaSolucao(const Instancia &inst, const std::vector<int> &s,
                    double foInformada, bool imprimeDetalhes)
{
    bool valida = true;
    int n = inst.n;

    // 1) Confere o tamanho da rota.
    if (static_cast<int>(s.size()) != n) {
        if (imprimeDetalhes) {
            std::cout << "[VALIDACAO] ERRO: a rota tem " << s.size()
                       << " posicoes, mas a instancia tem " << n << " cidades.\n";
        }
        return false; // sem o tamanho certo, nem vale a pena checar o resto
    }

    // 2) Confere se cada cidade 0..n-1 aparece exatamente uma vez.
    std::vector<int> ocorrencias(n, 0);
    for (int cidade : s) {
        if (cidade < 0 || cidade >= n) {
            if (imprimeDetalhes) {
                std::cout << "[VALIDACAO] ERRO: cidade invalida na rota: " << cidade << '\n';
            }
            valida = false;
        } else {
            ocorrencias[cidade]++;
        }
    }

    for (int cidade = 0; cidade < n; cidade++) {
        if (ocorrencias[cidade] == 0) {
            if (imprimeDetalhes) {
                std::cout << "[VALIDACAO] ERRO: cidade " << cidade << " esta faltando na rota.\n";
            }
            valida = false;
        } else if (ocorrencias[cidade] > 1) {
            if (imprimeDetalhes) {
                std::cout << "[VALIDACAO] ERRO: cidade " << cidade << " aparece "
                           << ocorrencias[cidade] << " vezes na rota.\n";
            }
            valida = false;
        }
    }

    // 3) Recalcula a fo do zero, a partir da rota, e confere com a fo informada.
    double foRecalculada = custo(inst, s);
    if (foInformada >= 0.0) {
        // Tolerância pequena para diferenças de arredondamento de ponto
        // flutuante -- não é uma questão de "quase igual", é o mesmo
        // valor recalculado de duas formas.
        const double tolerancia = 1e-3;
        if (std::abs(foRecalculada - foInformada) > tolerancia) {
            if (imprimeDetalhes) {
                std::cout << "[VALIDACAO] ERRO: fo informada (" << foInformada
                           << ") e fo recalculada a partir da rota (" << foRecalculada
                           << ") NAO batem! Diferenca = "
                           << (foInformada - foRecalculada) << '\n';
            }
            valida = false;
        }
    }

    if (imprimeDetalhes) {
        if (valida) {
            std::cout << "[VALIDACAO] OK: rota valida (todas as " << n
                       << " cidades presentes, sem repeticao). fo = " << foRecalculada << '\n';
        } else {
            std::cout << "[VALIDACAO] fo recalculada a partir da rota = " << foRecalculada << '\n';
        }
    }

    return valida;
}
