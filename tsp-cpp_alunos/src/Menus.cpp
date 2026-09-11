#include "Menus.h"

#include <iostream>

// [FRAMEWORK - pronto, não precisa mexer]

int menuPrincipal()
{
    int escolha;
    do {
        std::cout << "\n*******************Menu Principal************************* \n"
                   << "ATENCAO: Necessario gerar solucao inicial antes de refinar\n"
                   << "                1. Gere solucao inicial \n"
                   << "                2. Descida \n"
                   << "                3. Descida randomica \n"
                   << "                4. Descida com Primeiro de Melhora \n"
                   << "                5. Multi-Start \n"
                   << "                6. Simulated Annealing \n"
                   << "                7. Busca Tabu \n"
                   << "                8. ILS \n"
                   << "                9. GRASP \n"
                   << "               10. VND \n"
                   << "               11. VNS \n"
                   << "               12. Algoritmos Geneticos \n"
                   << "               13. Algoritmos Memeticos \n"
                   << "               14. Colonia de Formigas \n"
                   << "               15. LAHC (Late Acceptance Hill-Climbing) \n"
                   << "               16. PSO Discreto \n"
                   << "               17. BRKGA \n"
                   << "               18. Busca Dispersa \n"
                   << "               19. Plotar rota atual (SVG + PNG) \n"
                   << "                0. Sair \n"
                   << "                Escolha: ";
        std::cin >> escolha;
    } while (escolha < 0 || escolha > 19);
    return escolha;
}

int menuSolucaoInicial()
{
    int escolha;
    do {
        std::cout << "\n************Geracao da Solucao Inicial**************** \n"
                   << "                1. Gulosa (Vizinho mais proximo) \n"
                   << "                2. Parcialmente gulosa (Vizinho mais proximo) \n"
                   << "                3. Gulosa (Insercao Mais Barata) \n"
                   << "                4. Parcialmente gulosa (Insercao Mais Barata) \n"
                   << "                5. Aleatoria \n"
                   << "                Escolha: ";
        std::cin >> escolha;
    } while (escolha < 1 || escolha > 5);
    return escolha;
}

int menuGRASP()
{
    int escolha;
    do {
        std::cout << "\n*******************Menu GRASP************************* \n"
                   << "                1. Vizinho Mais Proximo \n"
                   << "                2. Insercao Mais Barata \n"
                   << "                Escolha: ";
        std::cin >> escolha;
    } while (escolha < 1 || escolha > 2);
    return escolha;
}

int menuAG()
{
    int escolha;
    do {
        std::cout << "\n****************Menu Algoritmos Geneticos********************** \n"
                   << "                1. Operador OX \n"
                   << "                2. Operador ERX \n"
                   << "                Escolha: ";
        std::cin >> escolha;
    } while (escolha < 1 || escolha > 2);
    return escolha;
}
