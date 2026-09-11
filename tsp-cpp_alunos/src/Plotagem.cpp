#include "Plotagem.h"
#include "Utilitarios.h" // custo()

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>

#include "stb_image_write.h"

// ============================================================================
// Transformação de coordenadas: mapeia as coordenadas (x,y) da instância
// (em qualquer escala/unidade) para a área desenhável do canvas (em
// pixels/pontos SVG), preservando a proporção (sem "esticar" o desenho) e
// deixando uma margem fixa em volta. Compartilhada pelo SVG e pelo PNG,
// para os dois desenharem exatamente a mesma coisa.
// ============================================================================
namespace {

struct Transformacao {
    double escala;
    double minX, minY;
    int alturaCanvas;
    int margem;

    // Devolve a posição (px, py) no canvas correspondente à cidade de
    // coordenadas (x, y). O eixo Y é invertido porque, em imagens, y
    // cresce para BAIXO, enquanto em um gráfico convencional y cresce
    // para CIMA.
    std::pair<double, double> mapeia(double x, double y) const
    {
        double px = margem + (x - minX) * escala;
        double py = alturaCanvas - margem - (y - minY) * escala;
        return {px, py};
    }
};

Transformacao calculaTransformacao(const Instancia &inst, int largura, int altura, int margem)
{
    double minX = *std::min_element(inst.x.begin(), inst.x.end());
    double maxX = *std::max_element(inst.x.begin(), inst.x.end());
    double minY = *std::min_element(inst.y.begin(), inst.y.end());
    double maxY = *std::max_element(inst.y.begin(), inst.y.end());

    // std::max(..., 1.0) evita divisao por zero no caso (raro) de todas
    // as cidades terem a mesma coordenada x ou y.
    double rangeX = std::max(maxX - minX, 1.0);
    double rangeY = std::max(maxY - minY, 1.0);

    double escalaX = (largura - 2.0 * margem) / rangeX;
    double escalaY = (altura - 2.0 * margem) / rangeY;

    Transformacao t;
    t.escala = std::min(escalaX, escalaY); // preserva a proporção
    t.minX = minX;
    t.minY = minY;
    t.alturaCanvas = altura;
    t.margem = margem;
    return t;
}

} // namespace

// ============================================================================
// [FRAMEWORK] Plot em SVG (vetorial, sem dependências externas)
// ============================================================================
void plotaRotaSVG(const Instancia &inst, const std::vector<int> &s,
                   const std::string &nomeArquivo,
                   int larguraCanvas, int alturaCanvas)
{
    const int margem = 40;
    Transformacao t = calculaTransformacao(inst, larguraCanvas, alturaCanvas, margem);
    double fo = custo(inst, s);

    std::ofstream arquivo(nomeArquivo);
    if (!arquivo) {
        std::cerr << "Nao foi possivel criar o arquivo " << nomeArquivo << '\n';
        return;
    }

    arquivo << "<svg width=\"" << larguraCanvas << "\" height=\"" << alturaCanvas
            << "\" xmlns=\"http://www.w3.org/2000/svg\">\n";
    arquivo << "  <rect width=\"100%\" height=\"100%\" fill=\"white\"/>\n";
    arquivo << "  <text x=\"" << margem << "\" y=\"20\" font-family=\"sans-serif\" "
            << "font-size=\"14\" fill=\"black\">"
            << "n = " << inst.n << "   fo = " << fo << "</text>\n";

    // Arestas da rota (inclui a aresta que fecha o ciclo, da ultima
    // cidade de volta para a primeira).
    arquivo << "  <polyline fill=\"none\" stroke=\"steelblue\" stroke-width=\"1.5\" points=\"";
    for (int cidade : s) {
        auto [px, py] = t.mapeia(inst.x[cidade], inst.y[cidade]);
        arquivo << px << ',' << py << ' ';
    }
    auto [pxInicio, pyInicio] = t.mapeia(inst.x[s[0]], inst.y[s[0]]);
    arquivo << pxInicio << ',' << pyInicio << "\"/>\n";

    // Marcadores das cidades, com o número de cada uma ao lado.
    for (int cidade = 0; cidade < inst.n; cidade++) {
        auto [px, py] = t.mapeia(inst.x[cidade], inst.y[cidade]);
        const char *cor = (cidade == s[0]) ? "crimson" : "black";

        arquivo << "  <circle cx=\"" << px << "\" cy=\"" << py << "\" r=\"4\" "
                << "fill=\"white\" stroke=\"" << cor << "\" stroke-width=\"1.5\"/>\n";
        arquivo << "  <text x=\"" << (px + 6) << "\" y=\"" << (py - 6)
                << "\" font-family=\"sans-serif\" font-size=\"10\" fill=\"" << cor << "\">"
                << cidade << "</text>\n";
    }

    arquivo << "</svg>\n";
    std::cout << "Rota salva em " << nomeArquivo << " (fo = " << fo << ")\n";
}

// ============================================================================
// Funções auxiliares de desenho em uma matriz de pixels (usadas só pelo
// PNG). "pixels" guarda os canais R,G,B de cada pixel em sequencia:
// pixels[(y*largura + x)*3 + 0/1/2] = R/G/B.
// ============================================================================
namespace {

void definePixel(std::vector<unsigned char> &pixels, int largura, int altura,
                  int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
    if (x < 0 || x >= largura || y < 0 || y >= altura) return; // fora do canvas
    int indice = (y * largura + x) * 3;
    pixels[indice + 0] = r;
    pixels[indice + 1] = g;
    pixels[indice + 2] = b;
}

// Desenha um pequeno quadrado (em vez de um único pixel) em volta de
// (x,y), só para a linha/marcador ficar visivelmente mais grossa.
void definePixelGrosso(std::vector<unsigned char> &pixels, int largura, int altura,
                        int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++)
            definePixel(pixels, largura, altura, x + dx, y + dy, r, g, b);
}

// Algoritmo de Bresenham: desenha a reta entre (x0,y0) e (x1,y1).
void desenhaLinha(std::vector<unsigned char> &pixels, int largura, int altura,
                   int x0, int y0, int x1, int y1,
                   unsigned char r, unsigned char g, unsigned char b)
{
    int dx = std::abs(x1 - x0), sx = (x0 < x1) ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = (y0 < y1) ? 1 : -1;
    int erro = dx + dy;

    while (true) {
        definePixelGrosso(pixels, largura, altura, x0, y0, r, g, b);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * erro;
        if (e2 >= dy) { erro += dy; x0 += sx; }
        if (e2 <= dx) { erro += dx; y0 += sy; }
    }
}

void desenhaCirculoPreenchido(std::vector<unsigned char> &pixels, int largura, int altura,
                               int cx, int cy, int raio,
                               unsigned char r, unsigned char g, unsigned char b)
{
    for (int y = -raio; y <= raio; y++) {
        for (int x = -raio; x <= raio; x++) {
            if (x * x + y * y <= raio * raio) {
                definePixel(pixels, largura, altura, cx + x, cy + y, r, g, b);
            }
        }
    }
}

} // namespace

// ============================================================================
// [FRAMEWORK] Plot em PNG (raster, via stb_image_write.h)
// ============================================================================
void plotaRotaPNG(const Instancia &inst, const std::vector<int> &s,
                   const std::string &nomeArquivo,
                   int largura, int altura)
{
    const int margem = 40;
    Transformacao t = calculaTransformacao(inst, largura, altura, margem);
    double fo = custo(inst, s);

    // Canvas branco (3 canais: R, G, B).
    std::vector<unsigned char> pixels(static_cast<std::size_t>(largura) * altura * 3, 255);

    // Arestas da rota.
    for (std::size_t i = 0; i < s.size(); i++) {
        int cidadeA = s[i];
        int cidadeB = s[(i + 1) % s.size()]; // fecha o ciclo na ultima aresta

        auto [pxA, pyA] = t.mapeia(inst.x[cidadeA], inst.y[cidadeA]);
        auto [pxB, pyB] = t.mapeia(inst.x[cidadeB], inst.y[cidadeB]);

        desenhaLinha(pixels, largura, altura,
                     static_cast<int>(std::lround(pxA)), static_cast<int>(std::lround(pyA)),
                     static_cast<int>(std::lround(pxB)), static_cast<int>(std::lround(pyB)),
                     70, 130, 180); // azul (steelblue)
    }

    // Marcadores das cidades (a cidade 0/origem em vermelho).
    for (int cidade = 0; cidade < inst.n; cidade++) {
        auto [px, py] = t.mapeia(inst.x[cidade], inst.y[cidade]);
        int cx = static_cast<int>(std::lround(px));
        int cy = static_cast<int>(std::lround(py));

        if (cidade == s[0]) {
            desenhaCirculoPreenchido(pixels, largura, altura, cx, cy, 4, 220, 20, 60); // crimson
        } else {
            desenhaCirculoPreenchido(pixels, largura, altura, cx, cy, 3, 0, 0, 0); // preto
        }
    }

    int sucesso = stbi_write_png(nomeArquivo.c_str(), largura, altura, 3,
                                  pixels.data(), largura * 3);
    if (!sucesso) {
        std::cerr << "Nao foi possivel salvar o arquivo " << nomeArquivo << '\n';
        return;
    }
    std::cout << "Rota salva em " << nomeArquivo << " (fo = " << fo << ")\n";
}
