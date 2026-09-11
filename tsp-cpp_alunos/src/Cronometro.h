#ifndef CRONOMETRO_H
#define CRONOMETRO_H

#include <chrono>

// [FRAMEWORK]
// Pequeno cronômetro de parede (wall-clock), usado para medir o tempo de
// execução de cada método. std::chrono é a forma moderna e portável
// (Linux/Windows) de medir tempo em C++ -- substitui clock()/CLOCKS_PER_SEC.
class Cronometro {
public:
    Cronometro() : inicio(std::chrono::steady_clock::now()) {}
    double segundosDecorridos() const
    {
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - inicio).count();
    }
private:
    std::chrono::steady_clock::time_point inicio;
};

#endif // CRONOMETRO_H