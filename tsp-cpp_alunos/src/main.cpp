/*
 * ============================================================================
 *  Tecnicas Metaheuristicas para Otimizacao Combinatoria (BCC466)
 *  Problema do Caixeiro Viajante (PCV / TSP) -- programa principal
 *
 *  Baseado no material original de: Marcone Jamilson Freitas Souza
 *  DECOM/UFOP -- www.decom.ufop.br/prof/marcone
 *  Refatorado para C++ (STL) por: [ajuste os créditos conforme desejar]
 * ============================================================================
 */

#include <iostream>

#include "Instancia.h"
#include "Utilitarios.h"
#include "Aleatorio.h"
#include "Validacao.h"
#include "Cronometro.h"
#include "Plotagem.h"
#include "Menus.h"

#include "Construcao.h"
#include "BuscaLocal.h"
#include "MultiStart.h"
#include "SimulatedAnnealing.h"
#include "BuscaTabu.h"
#include "ILS.h"
#include "GRASP.h"
#include "VND.h"
#include "VNS.h"
#include "AlgoritmoGenetico.h"
#include "Memetico.h"
#include "ColoniaFormigas.h"
#include "LAHC.h"
#include "PSODiscreto.h"
#include "BRKGA.h"
#include "BuscaDispersa.h"



int main()
{
    // Nomes dos arquivos da instância a ser resolvida. Para trocar de
    // instância, basta alterar estes dois nomes (ex.: "C50INFO.TXT" /
    // "C50.TXT").
    const std::string arquivoInfo = "instancias/C50INFO.TXT";
    const std::string arquivoCoordenadas = "instancias/C50.TXT";

    int n;
    double melhorFoLiteratura;
    leParametrosInstancia(arquivoInfo, n, melhorFoLiteratura);
    Instancia inst = leInstancia(arquivoCoordenadas, n);

    std::cout << "Instancia com " << inst.n << " cidades carregada. "
               << "Melhor fo conhecida na literatura = " << melhorFoLiteratura << "\n";

    // Semente do gerador de números aleatórios. Troque para semente(1000)
    // (uma semente fixa) se quiser resultados reprodutíveis entre
    // execuções -- útil para depurar ou comparar métodos de forma justa.
    sementeAleatoria();

    std::vector<int> s; // solução corrente
    double fo;

    int escolha = 0;
    do {
        escolha = menuPrincipal();

        // Todo método além da opção 1 (gerar solução inicial) precisa de
        // uma solução corrente já construída.
        if (escolha != 0 && escolha != 1 && s.empty()) {
            std::cout << "\nGere uma solucao inicial antes (opcao 1)!\n";
            continue;
        }

        switch (escolha) {

        case 1: { // Geração de uma solução inicial
            Cronometro cron;
            switch (menuSolucaoInicial()) {
            case 1:
                fo = constroiSolucaoGulosaVizinhoMaisProximo(inst, s);
                std::cout << "\nSolucao construida de forma gulosa (Vizinho Mais Proximo):\n";
                break;
            case 2:
                fo = constroiSolucaoParcialmenteGulosaVizinhoMaisProximo(inst, s, 0.05);
                std::cout << "\nSolucao construida de forma parcialmente gulosa (Vizinho Mais Proximo):\n";
                break;
            case 3:
                fo = constroiSolucaoGulosaInsercaoMaisBarata(inst, s);
                std::cout << "\nSolucao construida de forma gulosa (Insercao Mais Barata):\n";
                break;
            case 4:
                fo = constroiSolucaoParcialmenteGulosaInsercaoMaisBarata(inst, s, 0.05);
                std::cout << "\nSolucao construida de forma parcialmente gulosa (Insercao Mais Barata):\n";
                break;
            case 5:
                fo = constroiSolucaoAleatoria(inst, s);
                std::cout << "\nSolucao construida de forma aleatoria:\n";
                break;
            }
            imprimeRota(s);
            std::cout << "\nFuncao objetivo = " << fo << '\n';
            std::cout << "Tempo execucao = " << cron.segundosDecorridos() << " segundos\n";
            break;
        }

        case 2: { // Descida
            Cronometro cron;
            double fo = descida(inst, s);
            std::cout << "Tempo execucao = " << cron.segundosDecorridos() << " segundos\n"
                       << "Distancia percorrida = " << fo << '\n';
            imprimeRota(s);
            validaSolucao(inst, s, fo);
            break;
        }

        case 3: { // Descida Randômica
            Cronometro cron;
            double fo = descidaRandomica(inst, s, static_cast<int>(0.7 * n * (n - 1) / 2));
            std::cout << "Distancia percorrida = " << fo << '\n'
                       << "Tempo execucao = " << cron.segundosDecorridos() << " segundos\n";
            imprimeRota(s);
            validaSolucao(inst, s, fo);
            break;
        }

        case 4: { // Descida com Primeiro de Melhora
            Cronometro cron;
            double fo = descidaPrimeiroMelhora(inst, s);
            std::cout << "Tempo execucao = " << cron.segundosDecorridos() << " segundos\n"
                       << "Distancia percorrida = " << fo << '\n';
            imprimeRota(s);
            validaSolucao(inst, s, fo);
            break;
        }

        case 5: { // Multi-Start
            Cronometro cron;
            double fo = multiStart(inst, s, 100);
            std::cout << "fo = " << fo << '\n'
                       << "Tempo de execucao = " << cron.segundosDecorridos() << " segundos\n";
            imprimeRota(s);
            validaSolucao(inst, s, fo);
            break;
        }

        case 6: { // Simulated Annealing
            Cronometro cron;
            double tempInicial = calculaTemperaturaInicial(inst, s, 1.1, 0.95, 500);
            double fo = simulatedAnnealing(inst, s, 0.99, 2 * n, tempInicial, 0.01);
            std::cout << "fo = " << fo << '\n'
                       << "Tempo de execucao = " << cron.segundosDecorridos() << " segundos\n";
            imprimeRota(s);
            validaSolucao(inst, s, fo);
            break;
        }

        case 7: { // Busca Tabu
            Cronometro cron;
            double fo = buscaTabu(inst, s, 2, 100);
            std::cout << "Tempo execucao = " << cron.segundosDecorridos() << " segundos\n"
                       << "Distancia percorrida = " << fo << '\n';
            imprimeRota(s);
            validaSolucao(inst, s, fo);
            break;
        }

        case 8: { // Iterated Local Search
            Cronometro cron;
            double fo = ils(inst, s, 15, 30);
            std::cout << "fo = " << fo << '\n'
                       << "Tempo de execucao = " << cron.segundosDecorridos() << " segundos\n";
            imprimeRota(s);
            validaSolucao(inst, s, fo);
            break;
        }

        case 9: { // GRASP
            int tipoConstrucao = menuGRASP();
            Cronometro cron;
            double fo = grasp(inst, s, 0.05, 100, tipoConstrucao);
            std::cout << "\nSolucao gerada pelo Metodo GRASP:\n";
            imprimeRota(s);
            std::cout << "\nMelhor fo encontrada = " << fo << '\n'
                       << "Tempo de CPU = " << cron.segundosDecorridos() << " segundos\n";
            validaSolucao(inst, s, fo);
            break;
        }

        case 10: { // VND
            Cronometro cron;
            double fo = vnd(inst, s, 3);
            std::cout << "fo = " << fo << '\n'
                       << "Tempo de execucao = " << cron.segundosDecorridos() << " segundos\n";
            validaSolucao(inst, s, fo);
            break;
        }

        case 11: { // VNS
            Cronometro cron;
            double fo = vns(inst, s, 20, 10);
            std::cout << "fo = " << fo << '\n'
                       << "Tempo de execucao = " << cron.segundosDecorridos() << " segundos\n";
            imprimeRota(s);
            validaSolucao(inst, s, fo);
            break;
        }

        case 12: { // Algoritmo Genético
            int tipoOperador = menuAG();
            Cronometro cron;
            double fo = algoritmoGenetico(inst, s, 100, 0.03, 0.85, 0.01, tipoOperador);
            std::cout << "Solucao por AG:\n"
                       << "fo = " << fo << '\n'
                       << "Tempo de execucao = " << cron.segundosDecorridos() << " segundos\n";
            imprimeRota(s);
            validaSolucao(inst, s, fo);
            break;
        }

        case 13: { // Algoritmo Memético
            Cronometro cron;
            double fo = memetico(inst, s, 100, 0.03, 50, 0.85, 0.01, 1);
            std::cout << "Solucao por Memeticos usando operador OX:\n"
                       << "fo = " << fo << '\n'
                       << "Tempo de execucao = " << cron.segundosDecorridos() << " segundos\n";
            imprimeRota(s);
            validaSolucao(inst, s, fo);
            break;
        }

        case 14: { // Colônia de Formigas
            Cronometro cron;
            double fo = coloniaDeFormigas(inst, s, 20, 1.0, 2.0, 0.5, 100);
            std::cout << "fo = " << fo << '\n'
                       << "Tempo de execucao = " << cron.segundosDecorridos() << " segundos\n";
            validaSolucao(inst, s, fo);
            break;
        }

        case 15: { // LAHC
            Cronometro cron;
            double fo = lahc(inst, s, 30, 100);
            std::cout << "fo = " << fo << '\n'
                       << "Tempo de execucao = " << cron.segundosDecorridos() << " segundos\n";
            validaSolucao(inst, s, fo);
            break;
        }

        case 16: { // PSO Discreto
            Cronometro cron;
            double fo = psoDiscreto(inst, s, 20, 100);
            std::cout << "fo = " << fo << '\n'
                       << "Tempo de execucao = " << cron.segundosDecorridos() << " segundos\n";
            validaSolucao(inst, s, fo);
            break;
        }

        case 17: { // BRKGA
            Cronometro cron;
            double fo = brkga(inst, s, 100, 0.20, 0.15, 0.70, 200);
            std::cout << "fo = " << fo << '\n'
                       << "Tempo de execucao = " << cron.segundosDecorridos() << " segundos\n";
            validaSolucao(inst, s, fo);
            break;
        }

        case 18: { // Busca Dispersa
            Cronometro cron;
            double fo = buscaDispersa(inst, s, 10, 100);
            std::cout << "fo = " << fo << '\n'
                       << "Tempo de execucao = " << cron.segundosDecorridos() << " segundos\n";
            validaSolucao(inst, s, fo);
            break;
        }

        case 19: { // Plotar rota atual
            plotaRotaSVG(inst, s, "rota.svg");
            plotaRotaPNG(inst, s, "rota.png");
            break;
        }

        default:
            break;
        }
    } while (escolha != 0);

    return 0;
}
