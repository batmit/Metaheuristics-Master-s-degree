"""Verificacao independente dos certificados e comparacao exploratoria pareada."""
from pathlib import Path
from fractions import Fraction as F
from collections import Counter
import ast
import csv
import hashlib
import json
import math
import statistics as st
import sys

ROOT=next(p for p in Path(__file__).resolve().parents if (p/'cv.c').is_file() and (p/'instances/pdstsp-c').is_dir())
HERE=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path(__file__).resolve().parent
manifest=json.loads((HERE/'manifesto.json').read_text())
assert 'finished' in manifest and manifest['sources_unchanged']
assert len(manifest['returncodes'])==80 and all(r['code']==0 for r in manifest['returncodes'])
rows=[json.loads(line) for path in HERE.glob('*_m*.jsonl') for line in path.read_text().splitlines()]
assert len(rows)==80
for name,h in manifest['source_sha256'].items():
    assert hashlib.sha256((HERE/'fontes_depois'/name).read_bytes()).hexdigest()==h
source=ast.parse((ROOT/'resultados/grupos_quatro_100/analisar.py').read_text())
function=next(n for n in source.body if isinstance(n,ast.FunctionDef) and n.name=='verify_instance')
scope={'ROOT':ROOT,'F':F}
exec(compile(ast.Module(body=[function],type_ignores=[]),'<independent-validator>','exec'),scope)
refs={'67-c-c':74,'165-r-c':160,'167-r-e':180,'181-r-e':216}
result=[]
for c,(name,ub) in enumerate(refs.items()):
    subset=[r for r in rows if r['instance']==name]
    scope['verify_instance'](name,subset)
    versions={m:{r['seed']:r for r in subset if r['mask']==m} for m in (0,15)}
    assert all(len(v)==10 for v in versions.values())
    assert versions[0].keys()==versions[15].keys()==set(range(manifest['seed']+c*10,manifest['seed']+(c+1)*10))
    delta=[versions[15][s]['claimed']-versions[0][s]['claimed'] for s in versions[0]]
    summary={'instance':name,'n':10,'UB':ub,'wins':sum(d<0 for d in delta),
             'ties':sum(d==0 for d in delta),'losses':sum(d>0 for d in delta),'mean_delta':st.mean(delta)}
    for m,label in ((0,'control'),(15,'new')):
        vv=list(versions[m].values());val=[r['claimed'] for r in vv]
        summary.update({f'{label}_best':min(val),f'{label}_mean':st.mean(val),
                        f'{label}_at_UB':sum(v<=ub for v in val),
                        f'{label}_mean_seconds':st.mean(r['seconds'] for r in vv),
                        f'{label}_mean_evaluations':st.mean(r['stats']['evaluations'] for r in vv),
                        f'{label}_median_temperature':st.median(r['temperature'] for r in vv),
                        f'{label}_mean_elite_assignments':st.mean(r['stats']['elite_assignments'] for r in vv),
                        f'{label}_macro_improvements':sum(r['stats']['macro_improvements'] for r in vv)})
        best=min(vv,key=lambda r:(r['claimed'],r['seed']))
        (HERE/f'melhor_{name}_{label}.json').write_text(json.dumps(best,indent=2))
    result.append(summary)
(HERE/'comparacao.json').write_text(json.dumps({'validated':80,'results':result},indent=2))
with (HERE/'comparacao.csv').open('w',newline='') as f:
    writer=csv.DictWriter(f,fieldnames=list(result[0]));writer.writeheader();writer.writerows(result)
print(json.dumps(result,indent=2))
text='''# Implementacao das propostas 1, 2, 6 e 7

## O que foi integrado

- Troca bidirecional: local inteiro ou grupo proximo do caminhao para drones, acompanhado de retornos para liberar carga. Metade das ancoras prioriza economia de percurso, a outra metade e uniforme. Retornos alternam alivio de carga/custo de insercao e sorteio uniforme. Avalia somente a cobertura completa.
- Reconstrucao parcial: fracao aleatoria de 5% a 15%, selecao aleatoria, por proximidade ou contribuicao ao gargalo; ordem de reinsercao embaralhada. Cada reinsercao compara a melhor posicao no caminhao com todas as quantidades viaveis em ate tres prioridades dos drones. Dois passes de 2-opt finalizam a rota.
- Calibracao mista: caminhada em copia, operadores configurados e alvo 0,8 da aceitacao media dos deltas positivos. O SA com tempo limitado continua apos o resfriamento, com retomadas e reaquecimento apos estagnacao; sem limite usa resfriamento e numero finito de retomadas.
- Elite diversa: melhor makespan absoluto preservado, ate tres outras atribuicoes distintas dentro de 10% do melhor; distancia de Hamming dos clientes por recurso. Empates da mesma atribuicao usam a soma dos tempos. Retomada sorteia preferencialmente uma atribuicao diferente e a reconstrui quando essa opcao esta ativa.

Nao foi adicionado CP-SAT, nem alterada a regra de ordem fixa do B&B. Os macromovimentos recebem 1% de probabilidade cada; os movimentos locais preservam pelo menos 75% pela validacao dos parametros. Probabilidades, fracoes, folga, estagnacao, reaquecimento e limite sao configuraveis. As outras propostas nao foram integradas.

## Uso

Compilar: `gcc -std=c11 -O2 -Wall -Wextra -pedantic main.c SA.c cv.c lerInstancias.c -lm -o sa_exploracao.exe`.

- Padrao da main: quatro casos, 100 execucoes por caso, semente do relogio, 30 segundos por execucao (calibracao + SA), propostas 1/2/6/7 ativas.
- Exemplo curto: `./sa_exploracao.exe --instancia 67-c-c --execucoes 10 --segundos 30`.
- Comparacao com sementes registradas: acrescentar `--semente 123`; repetir com `--controle` para desligar as quatro novidades, mantendo grupos anteriores. Por tempo, a quantidade de avaliacoes pode variar entre repeticoes; para reproducao deterministica do ciclo usar `--segundos 0`.
- `--segundos 0` encerra pelo resfriamento; `--controle` sem limite explicito conserva o ciclo anterior.
- A inicializacao fica fora do limite. Um movimento/reparo em andamento e concluido antes da verificacao seguinte, portanto o limite pode ser excedido pelo custo de uma operacao e de fechamento.
- Resumos da main: `resultados/resumo_quatro_abertas_exploracao.csv` e `resultados/resumo_quatro_abertas_controle.csv`, separados dos resultados historicos.

## Verificacao

Suíte de 15 testes C, incluindo `test_exploracao`: checagem direta de cobertura, duracoes e capacidade; remocao de 100% dos clientes; caminhada de macromovimentos; independencia de buffers; manutencao do melhor e diversidade; calibracao reproduzivel sem modificar a referencia; retomadas; limite total. Compilacao C11/O2 com avisos ativos.

## Comparacao exploratoria por tempo igual

Dez sementes novas por instancia e por versao, 10 segundos por execucao incluindo calibracao: 80 execucoes, quatro processos concorrentes, mesma compilacao. A ordem das versoes alterna entre sementes/casos. Todos os 80 certificados foram validados independentemente com fracoes exatas. A matriz de duracoes, cobertura, percurso e frota sao conferidos sem chamar o avaliador C.

O controle usa as mesmas funcoes locais e calibracao legada. Sob limite de tempo, ele tambem continua na temperatura final ate esgotar o prazo; isso permite comparar o mesmo esforco em tempo, sem obrigar uma versao a encerrar antes. O lote nao substitui uma confirmacao de 100 sementes nem uma avaliacao do padrao de 30 s. Tempos medidos sob concorrencia nao representam desempenho isolado.

| Instancia | Melhor controle | Melhor novo | Media controle | Media nova | Vitorias/empates/derrotas | UB |
|---|---:|---:|---:|---:|---|---:|
'''
for r in result:
    text+=f"| {r['instance']} | {r['control_best']:g} | {r['new_best']:g} | {r['control_mean']:.2f} | {r['new_mean']:.2f} | {r['wins']}/{r['ties']}/{r['losses']} | {r['UB']} |\n"
text+='''
Uma alteracao implementada nao equivale a melhoria comprovada. Este lote avalia o conjunto das quatro propostas; nao isola os efeitos individuais nem demonstra que a melhor combinacao foi encontrada. As metricas adicionais estao em comparacao.json, incluindo avaliacoes, temperaturas, diversidade e melhorias originadas nos macromovimentos.

Fontes anteriores em fontes_antes/, fontes usadas no lote em fontes_depois/, sementes e hashes no manifesto.json. Para reproduzir outro lote, preservar os arquivos existentes ou usar uma pasta nova: rodar.py gera sementes do relogio e os resultados sao identificados por semente.
'''
if HERE.name=='resfriamento_tempo':
    text=text.replace('Fontes anteriores em fontes_antes/','Fontes anteriores em ../fontes_antes/')
    text+='''
## Ajuste do resfriamento durante a verificacao

A primeira comparacao de 80 execucoes, preservada na pasta superior, mostrou que os macromovimentos reduziam o numero de propostas por segundo e, nas maiores, podiam consumir o prazo antes de concluir os niveis de resfriamento. Essa versao teve pioras grandes nas instancias 167 e 181. Por isso o padrao final usa `resfriamentoPorTempo=true`: a temperatura percorre geometricamente de T0 a tempFinal em funcao da fracao de tempo consumida. Um reaquecimento tambem decai ate tempFinal dentro do prazo restante. Sem limite de tempo, conserva o resfriamento por alpha e limita o numero de retomadas.

Esta segunda comparacao usa sementes novas, com um novo controle pareado; nao reutiliza somente os melhores resultados da primeira. Dez sementes ainda sao uma amostra exploratoria, nao uma confirmacao estatistica do ganho ou do efeito separado de cada proposta. A temperatura final por execucao esta nos certificados.

Os contadores de avaliacoes e macromovimentos descrevem o laco principal; nao incluem as tentativas da calibracao, os nos do B&B nem as reconstrucoes das retomadas. O tempo inclui todas essas etapas.

A suite final de 15 testes passou. O modo sem novidades e sem limite de tempo reproduziu exatamente, nas quatro instancias com semente 731, as temperaturas, makespans, rotas, quantidades e cronogramas obtidos pelos fontes anteriores congelados. A verificacao esta em ../verificar_compatibilidade.py e ../compatibilidade.json. A main compilada tambem passou no teste de uma execucao curta dos quatro casos e rejeitou argumentos invalidos antes de abrir o resumo.
'''
(HERE/'relatorio.md').write_text(text,encoding='utf-8')
