#include <assert.h>
#include <math.h>
#include <stddef.h>
#include "SA.h"

int main(void)
{
    ParametrosSA padrao = parametrosSARapido();
    assert(padrao.gamma == 0.8 && padrao.saMax == 500);
    assert(validarParametrosSA(&padrao));
    assert(!validarParametrosSA(NULL));
    ParametrosSA p = padrao;
    p.alpha = 0; assert(!validarParametrosSA(&p));
    p.alpha = 1; assert(!validarParametrosSA(&p));
    p.alpha = NAN; assert(!validarParametrosSA(&p));
    p = padrao; p.gamma = 0; assert(!validarParametrosSA(&p));
    p.gamma = 1; assert(!validarParametrosSA(&p));
    p.gamma = NAN; assert(!validarParametrosSA(&p));
    p = padrao; p.saMax = 0; assert(!validarParametrosSA(&p));
    p.saMax = -1; assert(!validarParametrosSA(&p));
    p = padrao; p.tempFinal = 0; assert(!validarParametrosSA(&p));
    p.tempFinal = -1; assert(!validarParametrosSA(&p));
    p.tempFinal = NAN; assert(!validarParametrosSA(&p));
    p = padrao; p.tempInicial = p.tempFinal; assert(!validarParametrosSA(&p));
    p.tempInicial = 0; assert(!validarParametrosSA(&p));
    p.tempInicial = INFINITY; assert(!validarParametrosSA(&p));
    return 0;
}
