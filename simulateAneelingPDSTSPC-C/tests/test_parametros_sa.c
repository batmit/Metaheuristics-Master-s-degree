#include <assert.h>
#include <math.h>
#include <stddef.h>
#include "SA.h"

int main(void)
{
    ParametrosSA padrao = parametrosSAPadrao();
    assert(padrao.beta == 1.1 && padrao.gamma == 0.95 && padrao.saMax == 500);
    assert(validarParametrosSA(&padrao));
    assert(!validarParametrosSA(NULL));
    ParametrosSA p = padrao;
    p.alpha = 0; assert(!validarParametrosSA(&p));
    p.alpha = 1; assert(!validarParametrosSA(&p));
    p.alpha = NAN; assert(!validarParametrosSA(&p));
    p = padrao; p.beta = 1; assert(!validarParametrosSA(&p));
    p.beta = INFINITY; assert(!validarParametrosSA(&p));
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
