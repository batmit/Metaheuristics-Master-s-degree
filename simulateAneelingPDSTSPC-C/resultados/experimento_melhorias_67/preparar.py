from pathlib import Path
import hashlib
import json

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
ORIGINAL=HERE/'original'
CANDIDATE=HERE/'candidato'
FILES=['SA.c','SA.h','cv.c','cv.h','main.h','lerInstancias.c','lerInstancias.h','main.c']
if not ORIGINAL.exists():
    ORIGINAL.mkdir()
    for name in FILES: (ORIGINAL/name).write_bytes((ROOT/name).read_bytes())
    (HERE/'hashes_originais.json').write_text(json.dumps({n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in FILES},indent=2))
CANDIDATE.mkdir(exist_ok=True)
if (HERE/'triagem'/'resumo.json').exists() and not (HERE/'triagem'/'fontes').exists():
    frozen=HERE/'triagem'/'fontes';frozen.mkdir()
    for file in CANDIDATE.glob('*'):
        if file.is_file(): (frozen/file.name).write_bytes(file.read_bytes())
for name in FILES: (CANDIDATE/name).write_bytes((ORIGINAL/name).read_bytes())

def change(s, old, new):
    assert s.count(old)==1, (s.count(old),old[:100])
    return s.replace(old,new,1)

h=(CANDIDATE/'SA.h').read_text()
h=change(h,'typedef struct ParametrosSA {', '''enum { SA_GRUPOS=1u, SA_FOLGA=2u, SA_DIVERSIDADE=4u, SA_REPARO=8u,
       SA_INICIO_DIVERSO=16u, SA_REINICIOS=32u };
typedef struct ParametrosSA {
    unsigned melhorias; /* Bits independentes; zero reproduz o algoritmo anterior. */
    double maxSegundos; /* Zero usa somente o resfriamento. Inclui apenas o SA. */
    unsigned long long maxAvaliacoes; /* Zero: sem limite adicional. */''')
(CANDIDATE/'SA.h').write_text(h)
h=(CANDIDATE/'cv.h').read_text()
# TipoMovimento precisa estar definido antes dos novos prototipos.
h=change(h,'} TipoMovimento;', '''} TipoMovimento;
int repararCargaDrone(const Instancia *, Solucao *);
int construirSolucaoInicialDiversa(const Instancia *, Solucao *);
int gerarVizinhoAvaliadoConfigurado(const Instancia *, const Solucao *, Solucao *, const double *, TipoMovimento *, bool);''')
(CANDIDATE/'cv.h').write_text(h)

c=(CANDIDATE/'cv.c').read_text()
c=change(c,'static int gerarVizinhoInterno(', (HERE/'cv_melhorias.inc').read_text()+'\nstatic int gerarVizinhoInterno(')
c=change(c,'const double *pesos, TipoMovimento *tipo){','const double *pesos, TipoMovimento *tipo, bool grupos){')
c=change(c,'    if (movimento == TROCAR_CAMINHAO || movimento == TROCAR_DRONES) {', '''    int grupo=0;
    if(grupos && (movimento==PARA_DRONES || movimento==PARA_CAMINHAO) && rand()%4==0) {
        grupo=moverGrupoMelhoria(inst,vizinho,movimento==PARA_DRONES);
        if(grupo<0) return 0;
    }
    if(grupo) {
        nc=vizinho->numMissoesCaminhao; nd=vizinho->numMissoesDrone;
        if(alteracao) {alteracao->primeiraDrone=0;alteracao->deltaCaminhao=custoRotaMelhoria(inst,vizinho)-atual->tempoCaminhao;}
    } else if (movimento == TROCAR_CAMINHAO || movimento == TROCAR_DRONES) {''')
c=change(c,'gerarVizinhoInterno(inst, atual, vizinho, NULL, NULL, NULL);','gerarVizinhoInterno(inst, atual, vizinho, NULL, NULL, NULL, false);')
c=change(c,'int gerarVizinhoAvaliadoPonderado(const Instancia *inst, const Solucao *atual,','int gerarVizinhoAvaliadoConfigurado(const Instancia *inst, const Solucao *atual,')
c=change(c,'TipoMovimento *tipo){\n    if (inst == NULL || atual == NULL || vizinho == NULL ||','TipoMovimento *tipo, bool grupos){\n    if (inst == NULL || atual == NULL || vizinho == NULL ||')
c=change(c,'gerarVizinhoInterno(inst, atual, vizinho, &alteracao, pesos, tipo)','gerarVizinhoInterno(inst, atual, vizinho, &alteracao, pesos, tipo, grupos)')
c=change(c,'int gerarVizinhoAvaliado(const Instancia *inst, const Solucao *atual, Solucao *vizinho){', '''int gerarVizinhoAvaliadoPonderado(const Instancia *inst, const Solucao *atual,
                                Solucao *vizinho, const double *pesos, TipoMovimento *tipo){
    return gerarVizinhoAvaliadoConfigurado(inst,atual,vizinho,pesos,tipo,false);
}
int gerarVizinhoAvaliado(const Instancia *inst, const Solucao *atual, Solucao *vizinho){''')
(CANDIDATE/'cv.c').write_text(c)

s=(CANDIDATE/'SA.c').read_text()
s=change(s,'#include <string.h>','#include <string.h>\n#include <time.h>')
s=change(s,'return isfinite(parametros->alpha) &&','return isfinite(parametros->maxSegundos) && parametros->maxSegundos>=0 &&\n        !(parametros->melhorias & ~63u) && isfinite(parametros->alpha) &&')
s=change(s,'int refinarEliteComHistoricoSA(', (HERE/'SA_melhorias.inc').read_text()+'''\nstatic int refinarEliteComOpcoesSA(''')
s=change(s,'unsigned long long maxNos, HistoricoBBSA *h)\n{','unsigned long long maxNos, HistoricoBBSA *h, unsigned opcoes)\n{')
s=change(s,'    if(executar) {\n        ResultadoBB resultado;','    if(executar) {\n        if((opcoes&SA_REPARO) && !repararCargaDrone(inst,&refinada)) goto finalizar;\n        ResultadoBB resultado;')
s=change(s,'if(refinada.makespan<melhores[escolhido].makespan)', '''if(refinada.makespan<melhores[escolhido].makespan ||
       ((opcoes&SA_FOLGA) && refinada.makespan==melhores[escolhido].makespan &&
        secundarioSA(&refinada)<secundarioSA(&melhores[escolhido])))''')
s=change(s,'int refinarEliteAleatoriaSA(', '''int refinarEliteComHistoricoSA(const Instancia *inst, Solucao melhores[NUM_MELHORES_SA],
                             int *quantidade, unsigned long long maxNos, HistoricoBBSA *h) {
    return refinarEliteComOpcoesSA(inst,melhores,quantidade,maxNos,h,0);
}
int refinarEliteAleatoriaSA(''')
s=change(s,'atualizarMelhoresSolucoesSA(inst, &atual, melhores, &quantidadeMelhores);','atualizarEliteOpcoesSA(inst, &atual, melhores, &quantidadeMelhores,parametros->melhorias);')
s=change(s,'    int niveisSemMelhora = 0;', '''    int niveisSemMelhora = 0, estagnacao=0, niveis=0, reinicios=0;
    unsigned long long avaliacoes=0;
    clock_t inicioBusca=clock();
    bool esgotado=false;''')
s=change(s,'while (temperatura > parametros->tempFinal && existeMovimento(inst, &atual)) {','while ((parametros->maxSegundos>0 || parametros->maxAvaliacoes>0 || temperatura>parametros->tempFinal) && !esgotado && existeMovimento(inst, &atual)) {')
s=change(s,'            TipoMovimento movimento;','''            if((parametros->maxAvaliacoes && avaliacoes>=parametros->maxAvaliacoes) ||
               (parametros->maxSegundos>0 && avaliacoes%64==0 &&
                (double)(clock()-inicioBusca)/CLOCKS_PER_SEC>=parametros->maxSegundos)) {esgotado=true;break;}
            TipoMovimento movimento;''')
s=change(s,'if (!gerarVizinhoAvaliadoPonderado(inst, &atual, &candidata,','if (!gerarVizinhoAvaliadoConfigurado(inst, &atual, &candidata,')
s=change(s,'parametros->pesosAdaptativos ? pesos.pesos : NULL, &movimento))','parametros->pesosAdaptativos ? pesos.pesos : NULL, &movimento,(parametros->melhorias&SA_GRUPOS)!=0))')
s=change(s,'            double delta = candidata.makespan - atual.makespan;', '''            avaliacoes++;
            /* A combinacao grupos+carga avalia a transferencia e o reparo juntos. */
            if((parametros->melhorias&SA_REPARO) &&
               abs(candidata.numMissoesCaminhao-atual.numMissoesCaminhao)>=2 &&
               !repararCargaDrone(inst,&candidata)) goto finalizar;
            double delta = candidata.makespan - atual.makespan;''')
s=change(s,'                else if (aceitar && delta > 0.0)', '''                else if(aceitar && delta==0.0 && (parametros->melhorias&SA_FOLGA) &&
                        secundarioSA(&candidata)<secundarioSA(&atual) && candidata.makespan<=melhores[0].makespan)
                    pesos.pontos[movimento] += 0.5;
                else if (aceitar && delta > 0.0)''')
s=change(s,'atualizarMelhoresSolucoesSA(inst, &candidata, melhores, &quantidadeMelhores);','atualizarEliteOpcoesSA(inst, &candidata, melhores, &quantidadeMelhores,parametros->melhorias);')
s=change(s,'        if (melhores[0].makespan < melhorInicioNivel) niveisSemMelhora = 0;', '''        niveis++;
        if(!esgotado && (parametros->melhorias&SA_REPARO) && niveis%10==0) {
            if(!repararCargaDrone(inst,&atual)) goto finalizar;
            atualizarEliteOpcoesSA(inst,&atual,melhores,&quantidadeMelhores,parametros->melhorias);
        }
        if(melhores[0].makespan<melhorInicioNivel) estagnacao=0; else estagnacao++;
        if(!esgotado && (parametros->melhorias&SA_REINICIOS) && estagnacao>=80 && reinicios<3) {
            copiarSolucao(inst,&melhores[rand()%quantidadeMelhores],&atual);
            for(int q=0;q<8;q++) {
                if(!gerarVizinhoAvaliadoConfigurado(inst,&atual,&candidata,NULL,NULL,true)) goto finalizar;
                Solucao aux=atual;atual=candidata;candidata=aux;
                avaliacoes++;
            }
            temperatura=fmax(temperatura,parametros->tempInicial*0.02);
            estagnacao=0;reinicios++;
        }
        if (melhores[0].makespan < melhorInicioNivel) niveisSemMelhora = 0;''')
s=change(s,'if (parametros->bbPeriodico && niveisSemMelhora >= parametros->niveisSemMelhoraBB)', 'if (!esgotado && parametros->bbPeriodico && niveisSemMelhora >= parametros->niveisSemMelhoraBB)')
s=change(s,'int refinou = refinarEliteComHistoricoSA(inst, melhores, &quantidadeMelhores,','int refinou = refinarEliteComOpcoesSA(inst, melhores, &quantidadeMelhores,')
s=change(s,'parametros->maxNosBB, &historicoBB);','parametros->maxNosBB, &historicoBB,parametros->melhorias);')
s=change(s,'        temperatura = proxima;','        temperatura = fmax(proxima,parametros->tempFinal*0.5);')
# Expor folga refinada ao estado atual sem forcar trajetorias empatadas a cada nivel.
s=change(s,'if (refinou && melhores[0].makespan < antes)','if (refinou && (melhores[0].makespan < antes || ((parametros->melhorias&SA_FOLGA) && melhores[0].makespan==antes && melhores[0].makespan==atual.makespan && secundarioSA(&melhores[0])<secundarioSA(&atual))))')
(CANDIDATE/'SA.c').write_text(s)
print('Fontes experimentais preparados; producao preservada.')
