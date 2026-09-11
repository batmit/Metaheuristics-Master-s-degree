/*
 * ============================================================================
 *  stb_image_write_impl.cpp
 * ============================================================================
 *  A stb_image_write.h é uma biblioteca de UM ÚNICO ARQUIVO DE CABEÇALHO
 *  (header-only), de domínio público, sem nenhuma dependência externa e
 *  sem necessidade de instalação -- funciona da mesma forma em Linux,
 *  Windows e macOS. Fonte: https://github.com/nothings/stb
 *
 *  Por convenção da própria stb, o código de implementação da biblioteca
 *  só deve ser compilado em UM arquivo .cpp do projeto (definindo
 *  STB_IMAGE_WRITE_IMPLEMENTATION antes do #include); nos demais arquivos,
 *  basta incluir "stb_image_write.h" normalmente (sem a macro) para ter
 *  acesso às funções (como stbi_write_png), sem duplicar o código gerado.
 *  Este arquivo existe só para isso -- não precisa mexer nele.
 * ============================================================================
 */

// A stb_image_write.h é código de terceiros e gera alguns warnings
// (-Wmissing-field-initializers) com as flags -Wall -Wextra deste
// projeto. Como não faz sentido "corrigir" o código de uma biblioteca
// externa, apenas silenciamos esse warning especificamente ao redor da
// implementação -- o restante do projeto continua com todos os warnings
// ativados normalmente.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#pragma GCC diagnostic pop
