#ifndef PLOTAGEM_H
#define PLOTAGEM_H

#include <string>
#include <vector>
#include "Instancia.h"

/*
 * ============================================================================
 *  Módulo: Plotagem
 * ============================================================================
 *  Gera uma imagem da rota, para o aluno visualizar rapidamente a solução
 *  encontrada por um método, sem precisar de nenhuma ferramenta externa.
 *
 *  Duas opções de formato, cada uma com seu ponto forte:
 *
 *    - plotaRotaSVG: gera um arquivo de texto (XML) no formato SVG
 *      (Scalable Vector Graphics). NÃO usa nenhuma biblioteca externa --
 *      é só um arquivo de texto escrito com <fstream>. Abre em qualquer
 *      navegador (Chrome, Firefox, Edge, Safari...) em qualquer sistema
 *      operacional, basta dar duplo clique. Como é vetorial, pode ser
 *      ampliado sem perder qualidade, e o arquivo inclui o número de
 *      cada cidade ao lado do seu marcador -- útil para conferir a rota
 *      cidade a cidade.
 *
 *    - plotaRotaPNG: gera uma imagem raster (matriz de pixels) de verdade,
 *      usando a biblioteca stb_image_write.h (biblioteca de cabeçalho
 *      único, de domínio público, sem necessidade de instalação --
 *      https://github.com/nothings/stb). Útil quando você precisa de um
 *      arquivo .png "de verdade" (por exemplo, para colar em um
 *      relatório ou apresentação de slides). Não inclui os números das
 *      cidades (desenhar texto em uma imagem raster exige uma fonte
 *      bitmap, fora do escopo aqui) -- para isso, prefira o SVG.
 *
 *  As duas funções recebem a instância e a rota (na mesma convenção do
 *  resto do projeto: um std::vector<int> com uma permutação das cidades
 *  0..n-1) e usam as coordenadas (inst.x, inst.y) para posicionar cada
 *  cidade na imagem.
 * ============================================================================
 */

// [FRAMEWORK]
// Gera um arquivo SVG com a rota s. A cidade 0 (origem) é destacada em
// vermelho; as demais, em preto. O título da imagem mostra o número de
// cidades e o custo da rota (recalculado internamente com custo()).
void plotaRotaSVG(const Instancia &inst, const std::vector<int> &s,
                   const std::string &nomeArquivo,
                   int larguraCanvas = 800, int alturaCanvas = 800);

// [FRAMEWORK]
// Gera um arquivo PNG com a rota s, com o mesmo esquema de cores do SVG.
void plotaRotaPNG(const Instancia &inst, const std::vector<int> &s,
                   const std::string &nomeArquivo,
                   int largura = 800, int altura = 800);

#endif // PLOTAGEM_H
