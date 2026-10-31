from pathlib import Path
import csv
import datetime
from fractions import Fraction as F
import hashlib
import json
import random
import statistics

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
NAMES=['67-c-c','165-r-c','167-r-e','181-r-e']
UB={'67-c-c':74,'165-r-c':160,'167-r-e':180,'181-r-e':216}
LB={'67-c-c':73,'165-r-c':156,'167-r-e':179,'181-r-e':215}
result=json.loads((HERE/'resumo.json').read_text())
manifest=json.loads((HERE/'manifesto.json').read_text())
assert result['validated']==800
assert manifest['sources_unchanged']
for name,h in manifest['source_sha256'].items():
    assert hashlib.sha256((HERE/'fontes'/name).read_bytes()).hexdigest()==h
    assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest()==h
for name,h in manifest['input_sha256'].items():assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest()==h
records=[json.loads(line) for p in HERE.glob('*.jsonl') for line in p.read_text().splitlines()]
assert len(records)==800
rng=random.Random(20261026)  # Apenas bootstrap da analise; nao interfere no SA.
summary=[]
for name in NAMES:
    old={r['seed']:r for r in records if r['instance']==name and r['mask']==0}
    new={r['seed']:r for r in records if r['instance']==name and r['mask']==1}
    assert len(old)==len(new)==100 and old.keys()==new.keys()
    seeds=sorted(old)
    diffs=[new[s]['claimed']-old[s]['claimed'] for s in seeds]
    boots=sorted(statistics.mean(rng.choices(diffs,k=100)) for _ in range(10000))
    a=[old[s]['claimed'] for s in seeds]; b=[new[s]['claimed'] for s in seeds]
    best=min(new.values(),key=lambda r:(r['claimed'],r['seed']))
    row={'instance':name,'runs_before':100,'runs_after':100,'best_before':min(a),'best_after':min(b),
         'mean_before':statistics.mean(a),'mean_after':statistics.mean(b),
         'median_before':statistics.median(a),'median_after':statistics.median(b),
         'worst_before':max(a),'worst_after':max(b),
         'mean_reduction_percent':100*(statistics.mean(a)-statistics.mean(b))/statistics.mean(a),
         'paired_mean_delta':statistics.mean(diffs),'bootstrap95_low':boots[249],'bootstrap95_high':boots[9749],
         'wins':sum(d<0 for d in diffs),'ties':sum(d==0 for d in diffs),'losses':sum(d>0 for d in diffs),
         'literature_UB':UB[name],'literature_LB':LB[name],
         'gap_best_after_percent':100*(min(b)-UB[name])/UB[name],
         'below_literature_before':sum(v<UB[name] for v in a),'equal_literature_before':sum(v==UB[name] for v in a),
         'below_literature_after':sum(v<UB[name] for v in b),'equal_literature_after':sum(v==UB[name] for v in b),
         'best_seed_after':best['seed'],'best_run_after':best['run'],
         'seconds_mean_before':statistics.mean(old[s]['seconds'] for s in seeds),
         'seconds_mean_after':statistics.mean(new[s]['seconds'] for s in seeds)}
    tau={}
    for line in (ROOT/f'instances/pdstsp-c/large/tau_kj_{name}.txt').read_text().splitlines()[1:]:
        values=line.split()
        if values:tau[int(values[0])]=[(k,F(t)) for k,t in enumerate(values[1:],1) if F(t)>=0]
    fleet=int((ROOT/f'instances/pdstsp-c/large/{name}.txt').read_text().splitlines()[0].split(',')[1])
    for suffix,version in [('before',old),('after',new)]:
        values=list(version.values())
        row[f'truck_mean_{suffix}']=statistics.mean(r['truck'] for r in values)
        row[f'drone_mean_{suffix}']=statistics.mean(r['drone'] for r in values)
        row[f'drone_clients_mean_{suffix}']=statistics.mean(len(r['missions']) for r in values)
        row[f'assignments_bound_above_UB_{suffix}']=sum(
            sum((min(k*t for k,t in tau[m['client']]) for m in r['missions']),F(0))/fleet>UB[name]
            for r in values)
        row[f'truck_dominant_{suffix}']=sum(r['truck']>r['drone'] for r in values)
        row[f'drone_dominant_{suffix}']=sum(r['drone']>r['truck'] for r in values)
    summary.append(row)
    (HERE/f'melhor_{name}_grupos.json').write_text(json.dumps(best,indent=2))
with (HERE/'comparacao.csv').open('w',newline='',encoding='utf-8') as out:
    w=csv.DictWriter(out,fieldnames=list(summary[0]));w.writeheader();w.writerows(summary)
summary_fields=['instancia','concluidas','melhor','media','melhor_literatura','gap_melhor_percentual',
                'execucao_melhor','semente_melhor','abaixo_referencia','iguais_referencia']
with (ROOT/'resultados/resumo_quatro_abertas_grupos_100_execucoes.csv').open('w',newline='',encoding='utf-8') as out:
    w=csv.writer(out);w.writerow(summary_fields)
    for r in summary:w.writerow([r['instance'],100,r['best_after'],r['mean_after'],r['literature_UB'],
        r['gap_best_after_percent'],r['best_run_after'],r['best_seed_after'],r['below_literature_after'],r['equal_literature_after']])
elapsed=(datetime.datetime.fromisoformat(manifest['finished'])-datetime.datetime.fromisoformat(manifest['started'])).total_seconds()
result_final={'validated':800,'changed_runs':400,'control_runs':400,'full_cooling':True,
              'elapsed_seconds':elapsed,'comparisons':summary}
(HERE/'comparacao.json').write_text(json.dumps(result_final,ensure_ascii=False,indent=2),encoding='utf-8')
text='''# Transferencias em grupos nas quatro instancias abertas

Foram concluidas **100 execucoes com grupos e 100 sem grupos para cada instancia**: 400 execucoes novas e 400 controles pareados. Todos os 800 certificados passaram pela validacao independente com fracoes exatas.

As quatro configuracoes da main agora usam `movimentosGrupo=true`. A API conserva a opcao para desligar grupos. O algoritmo e exatamente o integrado no ensaio anterior; nao foram adicionados folga, diversidade de elites, reparo de carga ou retomadas.

## Metodo

- Fontes de producao congelados em `fontes/`, compilados em C11 com GCC 10.3.0 e `-O2`.
- Controle e tratamento diferem somente em `ParametrosSA.movimentosGrupo`.
- Mesmas 100 sementes dentro de cada comparacao; faixas distintas entre instancias, originadas do relogio e ajustadas para nao repetir os lotes recentes. Sementes e hashes constam do manifesto e dos registros.
- Resfriamento completo, sem limite adicional de tempo, sem reduzir o numero de propostas ou o orcamento do B&B.
- Quatro processos concorrentes, blocos de cinco, alternando ordem de casos e versoes. Os tempos registrados incluem construcao inicial, calibracao e SA sob concorrencia; nao sao um benchmark de tempo isolado.
- O controle foi executado novamente; os resultados nao foram comparados apenas com melhores historicos obtidos com outras sementes.

## Resultados em minutos

| Instancia | Melhor sem grupos | Melhor com grupos | Media sem grupos | Media com grupos | Reducao da media | UB literatura | Gap melhor novo |
|---|---:|---:|---:|---:|---:|---:|---:|
'''
for r in summary:text+=f"| {r['instance']} | {r['best_before']:g} | {r['best_after']:g} | {r['mean_before']:.2f} | {r['mean_after']:.2f} | {r['mean_reduction_percent']:+.2f}% | {r['literature_UB']} | {r['gap_best_after_percent']:+.2f}% |\n"
text+='\nPercentual positivo na reducao indica melhora; negativo indica piora. Melhorar a media e melhorar o melhor resultado sao criterios diferentes.\n\n'
text+='## Comparacao por semente\n\n| Instancia | Vitorias grupos | Empates | Derrotas grupos | Diferenca media novo - controle | IC bootstrap 95% da diferenca |\n|---|---:|---:|---:|---:|---|\n'
for r in summary:text+=f"| {r['instance']} | {r['wins']} | {r['ties']} | {r['losses']} | {r['paired_mean_delta']:+.2f} | [{r['bootstrap95_low']:+.2f}, {r['bootstrap95_high']:+.2f}] |\n"
text+='''
O bootstrap reamostra os 100 pares de cada instancia (10.000 reamostragens), usando uma semente fixa apenas na analise. Os intervalos descrevem essa amostra e pressupõem pares representativos; nao demonstram superioridade universal. O teste de sinais descritivo esta em `resumo.json`; nao foi usado como garantia de otimalidade.

## Distribuicao final entre recursos

| Instancia | Clientes medios nos drones, antes/depois | Caminhao medio, antes/depois | Drones medio, antes/depois | Atribuicoes com limite de carga acima do UB, antes/depois |
|---|---:|---:|---:|---:|
'''
for r in summary:text+=f"| {r['instance']} | {r['drone_clients_mean_before']:.2f} / {r['drone_clients_mean_after']:.2f} | {r['truck_mean_before']:.2f} / {r['truck_mean_after']:.2f} | {r['drone_mean_before']:.2f} / {r['drone_mean_after']:.2f} | {r['assignments_bound_above_UB_before']} / {r['assignments_bound_above_UB_after']} |\n"
text+='''
O ultimo indicador conta, em cada lote de 100, os conjuntos de clientes dos drones cuja soma de min_k(k*tau_j(k))/m ja excede o UB publicado. Nesses estados finais, melhorar somente quantidades ou ordem dos drones nao basta para atingir a referencia; e preciso mudar a atribuicao de clientes. Os indicadores descrevem os estados finais, sem estabelecer a causa da diferenca de desempenho.

## Analise por instancia

'''
for r in summary:
    direction='melhorou' if r['mean_after']<r['mean_before'] else 'piorou' if r['mean_after']>r['mean_before'] else 'empatou'
    best_direction='melhorou' if r['best_after']<r['best_before'] else 'piorou' if r['best_after']>r['best_before'] else 'empatou'
    text+=f"- **{r['instance']}**: a media {direction}, de {r['mean_before']:.2f} para {r['mean_after']:.2f}; o melhor {best_direction}, de {r['best_before']:g} para {r['best_after']:g}. Grupos ganhou {r['wins']}, empatou {r['ties']} e perdeu {r['losses']} pares. Resultados iguais ao UB publicado: {r['equal_literature_after']}/100; abaixo: {r['below_literature_after']}/100.\n"
text+='''
Esses resultados avaliam o efeito da nova vizinhanca na qualidade final. Nao isolam a causa mecanica de eventuais regressoes: o sorteio de grupos modifica a trajetoria aleatoria e os pesos dos tipos de transferencia, e uma investigacao causal exigiria instrumentacao adicional. Ganhos observados na 67-c-c nao devem ser extrapolados automaticamente para as maiores.

## Referencias e validacao

Referencias do pacote oficial ORLab de 2025: 67-c-c [73,74], 165-r-c [156,160], 167-r-e [179,180] e 181-r-e [215,216]. O UB nao e um otimo comprovado. Fonte: https://orlab.com.vn/download/data-and-results-of-pdstsp-c-and-pdsop-c/

A validacao independente recalcula cobertura unica dos clientes, rota Manhattan e retorno ao deposito, duracoes da matriz tau, capacidade simultanea e uma atribuicao de drones fisicos sem interrupcao de missoes. Nao chama o avaliador C. Os fontes de producao e dados permaneceram com os hashes registrados durante o lote.

Arquivos principais: `comparacao.csv` e `comparacao.json` (resumo), `execucoes.csv` (800 execucoes), `*.jsonl` (todos os vetores), `melhor_*_grupos.json` (melhor certificado de cada instancia), `manifesto.json` e `fontes/` (reproducao). A tabela de 400 execucoes com grupos tambem foi salva em `resultados/resumo_quatro_abertas_grupos_100_execucoes.csv`.
'''
(HERE/'relatorio.md').write_text(text,encoding='utf-8')
print(json.dumps(result_final,ensure_ascii=False,indent=2))
