"""Integra apenas o movimento em grupos; demais propostas ficam nos ensaios."""
from pathlib import Path
import hashlib
import json

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
original=HERE/'original'
hashes=json.loads((HERE/'hashes_originais.json').read_text())
assert all(hashlib.sha256((ROOT/n).read_bytes()).hexdigest()==h for n,h in hashes.items()), 'Fontes de producao mudaram durante o ensaio.'

def change(text,old,new):
    assert text.count(old)==1,old
    return text.replace(old,new,1)

cv=(HERE/'candidato/cv.c').read_text()
start=cv.index('\nint repararCargaDrone(')
end=cv.index('\nstatic int gerarVizinhoInterno(',start)
cv=cv[:start]+cv[end:]
header=(HERE/'candidato/cv.h').read_text()
header=header.replace('int repararCargaDrone(const Instancia *, Solucao *);\n','')
header=header.replace('int construirSolucaoInicialDiversa(const Instancia *, Solucao *);\n','')
header=header.replace('int gerarVizinhoAvaliadoConfigurado(const Instancia *, const Solucao *, Solucao *, const double *, TipoMovimento *, bool);', '''/* Quando grupos=true, 1/4 das transferencias tenta mover clientes da mesma
 * coordenada ou 2 a 4 clientes proximos. Os movimentos individuais continuam
 * disponiveis; quantidades sao sorteadas entre as viaveis. A atual nao muda.
 * O grupo usa avaliacao da rota alterada e reconstroi o cronograma dos drones. */
int gerarVizinhoAvaliadoConfigurado(const Instancia *inst, const Solucao *atual,
                                   Solucao *vizinho, const double *pesos,
                                   TipoMovimento *tipo, bool grupos);''')
sa=(original/'SA.c').read_text()
sa=change(sa,'        .maxNosBB = 10000','        .maxNosBB = 10000,\n        .movimentosGrupo = false')
sa=change(sa,'if (!gerarVizinhoAvaliadoPonderado(inst, &atual, &candidata,','if (!gerarVizinhoAvaliadoConfigurado(inst, &atual, &candidata,')
sa=change(sa,'parametros->pesosAdaptativos ? pesos.pesos : NULL, &movimento))','parametros->pesosAdaptativos ? pesos.pesos : NULL, &movimento, parametros->movimentosGrupo))')
sah=(original/'SA.h').read_text()
sah=change(sah,'    unsigned long long maxNosBB; /* Orcamento por chamada periodica. */','    unsigned long long maxNosBB; /* Orcamento por chamada periodica. */\n    bool movimentosGrupo; /* Transferencias conjuntas; false restaura o SA anterior. */')
sah=change(sah,' * BB periodico ativo apos 5 niveis sem melhora, com ate 10000 nos por chamada. */',' * BB periodico ativo apos 5 niveis sem melhora, com ate 10000 nos por chamada.\n * Grupos sao opcionais (false); a main ativa na 67-c-c, validada nos ensaios. */')
main=(original/'main.c').read_text()
main=change(main,'    double melhorLiteratura;','    double melhorLiteratura;\n    bool movimentosGrupo; /* Ativar apenas onde houve validacao de desempenho. */')
for nome,ub,ativo in [('67-c-c','74.0','true'),('165-r-c','160.0','false'),('167-r-e','180.0','false'),('181-r-e','216.0','false')]:
    main=change(main,'{"'+nome+'", "large", '+ub+'}', '{"'+nome+'", "large", '+ub+', '+ativo+'}')
main=change(main,'    ParametrosSA parametros = parametrosSAPadrao();','    ParametrosSA parametros = parametrosSAPadrao();\n    parametros.movimentosGrupo = caso->movimentosGrupo;')
for name,text in [('cv.c',cv),('cv.h',header),('SA.c',sa),('SA.h',sah),('main.c',main)]:
    (ROOT/name).write_text(text)
(HERE/'hashes_integrados.json').write_text(json.dumps({n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in hashes},indent=2))
print('Integrados somente movimentos em grupos; main ativa apenas na 67-c-c. Quatro casos e repeticoes preservados.')
