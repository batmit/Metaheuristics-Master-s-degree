#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include "SA.h"
#include "lerInstancias.h"

/* Verificacao direta de cobertura, tempos e ocupacao, sem usar o escalonador. */
static void verificar(const Instancia *in,const Solucao *s)
{
    int *v=calloc((size_t)in->n,sizeof *v);assert(v);
    int anterior=0;double truck=0,drone=0;
    for(int i=0;i<s->numMissoesCaminhao;i++) {
        int j=s->missaoCaminhao[i].n;assert(j>0 && j<in->n && !v[j]++);
        truck+=in->tempoCaminhao[anterior][j];anterior=j;
    }
    truck+=in->tempoCaminhao[anterior][0];
    for(int i=0;i<s->numMissoesDrone;i++) {
        MissaoDrone m=s->missaoDrone[i];assert(m.cliente>0 && m.cliente<in->n && !v[m.cliente]++);
        assert(m.qtdDrones>=in->clientes[m.cliente].minDrones && m.qtdDrones<=in->numDrones);
        assert(m.tempoInicio>=0 && in->tempoDrone[m.cliente][m.qtdDrones]>=0);
        assert(fabs(m.tempoFim-m.tempoInicio-in->tempoDrone[m.cliente][m.qtdDrones])<1e-9);
        int uso=0;
        for(int j=0;j<s->numMissoesDrone;j++) {
            MissaoDrone x=s->missaoDrone[j];
            if(x.tempoInicio<=m.tempoInicio && x.tempoFim>m.tempoInicio) uso+=x.qtdDrones;
        }
        assert(uso<=in->numDrones);drone=fmax(drone,m.tempoFim);
    }
    for(int j=1;j<in->n;j++) assert(v[j]==1);
    assert(s->avaliacaoValida && s->cronogramaValido);
    assert(fabs(s->tempoCaminhao-truck)<1e-9 && fabs(s->tempoDrones-drone)<1e-9);
    assert(fabs(s->makespan-fmax(truck,drone))<1e-9);free(v);
}

static void inicializar(Instancia *in,Solucao *s)
{
    assert(construirSolucaoInicial(in,s));
    assert(escalonarMissoesDrone(in,s) && calcularMakespan(in,s));
}

int main(void)
{
    ParametrosSA p=parametrosSAExploracao();assert(validarParametrosSA(&p));
    ParametrosSA errado=p;errado.fracaoMaxima=2;assert(!validarParametrosSA(&errado));
    errado=p;errado.probTroca=NAN;assert(!validarParametrosSA(&errado));
    errado=p;errado.probReconstrucao=.4;assert(!validarParametrosSA(&errado));
    errado=p;errado.limiteSegundos=-1;assert(!validarParametrosSA(&errado));
    errado=p;errado.maxRetomadas=-1;assert(!validarParametrosSA(&errado));
    Instancia in={0};assert(lerInstancia("instances/pdstsp-c/large/67-c-c.txt",&in));
    alocarInstancias(&in);construirMatrizCaminhao(&in);
    assert(lerTemposDrone("instances/pdstsp-c/large/tau_kj_67-c-c.txt",&in));
    Solucao a={0},b={0},c={0},elite[NUM_MELHORES_SA]={0};
    alocarSolucao(&in,&a);alocarSolucao(&in,&b);alocarSolucao(&in,&c);
    for(int i=0;i<NUM_MELHORES_SA;i++) alocarSolucao(&in,&elite[i]);
    inicializar(&in,&a);srand(731);
    /* Todos no caminhao, remocao completa e recuperacao dos obrigatorios. */
    assert(reconstruirParcialCV(&in,&a,&b,1,1));verificar(&in,&b);
    assert(a.numMissoesDrone==0);verificar(&in,&a);
    assert(trocarRecursosCV(&in,&a,&c));verificar(&in,&c);
    assert(!reconstruirParcialCV(&in,&a,&c,.2,.1));
    int nosDoisSentidos=0;
    for(int passo=0;passo<160;passo++) {
        if(passo%2) assert(reconstruirParcialCV(&in,&b,&c,.05,.15));
        else {
            assert(trocarRecursosCV(&in,&b,&c));
            int saiu=0,voltou=0;
            for(int i=0;i<b.numMissoesCaminhao;i++)
                for(int j=0;j<c.numMissoesDrone;j++) saiu+=b.missaoCaminhao[i].n==c.missaoDrone[j].cliente;
            for(int i=0;i<b.numMissoesDrone;i++)
                for(int j=0;j<c.numMissoesCaminhao;j++) voltou+=b.missaoDrone[i].cliente==c.missaoCaminhao[j].n;
            nosDoisSentidos+=saiu>0 && voltou>0;
        }
        verificar(&in,&c);
        double valor=c.makespan;assert(calcularMakespan(&in,&c) && fabs(c.makespan-valor)<1e-9);
        assert(escalonarMissoesDrone(&in,&c) && calcularMakespan(&in,&c));verificar(&in,&c);
        Solucao aux=b;b=c;c=aux;
    }
    assert(nosDoisSentidos>0);
    /* Arquivo diverso preserva a melhor e nao guarda a mesma atribuicao duas vezes. */
    int q=0;double melhor=INFINITY;
    for(int i=0;i<40;i++) {
        assert(reconstruirParcialCV(&in,&b,&c,.05,.25));
        melhor=fmin(melhor,c.makespan);
        atualizarEliteDiversaSA(&in,&c,elite,&q,1.0);
        assert(q>0 && elite[0].makespan==melhor);
        for(int j=0;j<q;j++) {
            verificar(&in,&elite[j]);
            for(int k=0;k<j;k++) {
                assert(distanciaAtribuicaoSA(&in,&elite[j],&elite[k])>0);
                assert(elite[j].missaoDrone!=elite[k].missaoDrone);
            }
        }
        Solucao aux=b;b=c;c=aux;
    }
    assert(q>1);
    /* Calibracao configurada reproduzivel e sem alterar a referencia. */
    inicializar(&in,&a);double original=a.makespan,t1,t2;
    p.limiteSegundos=0;p.saMax=80;p.alpha=.5;p.maxRetomadas=1;p.niveisSemMelhoraRetomada=1;
    srand(921);assert(calculaTemperaturaInicial(&in,&a,&p,&t1));
    assert(a.makespan==original && a.numMissoesDrone==0);verificar(&in,&a);
    srand(921);assert(calculaTemperaturaInicial(&in,&a,&p,&t2));assert(t1==t2 && t1>p.tempFinal);
    EstatisticasSA e={0};p.estatisticas=&e;
    srand(123);assert(resolverComCalibracaoSA(&in,&a,&p,&t1));verificar(&in,&a);
    assert(a.makespan<=original && e.trocas>0 && e.reconstrucoes>0 && e.retomadas==1);
    assert(e.atribuicoesElite>1);double obtido=a.makespan;
    inicializar(&in,&a);srand(123);
    assert(resolverComCalibracaoSA(&in,&a,&p,&t2));assert(a.makespan==obtido);
    /* Orçamento total inclui calibracao e termina com certificado completo. */
    p.limiteSegundos=.08;inicializar(&in,&a);
    double inicio=relogioSA();assert(resolverComCalibracaoSA(&in,&a,&p,&t1));
    assert(relogioSA()-inicio<2 && e.terminouPorTempo);verificar(&in,&a);
    liberarSolucao(&a);liberarSolucao(&b);liberarSolucao(&c);
    for(int i=0;i<NUM_MELHORES_SA;i++) liberarSolucao(&elite[i]);
    liberarInstancia(&in);return 0;
}
