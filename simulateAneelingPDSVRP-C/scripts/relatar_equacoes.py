"""Revalida os certificados do experimento e gera relatorio persistente."""
from collections import Counter
from copy import deepcopy
import csv
import json
import math
from pathlib import Path
import statistics

from executar_benchmark import ROOT, dados_instancia, validar
from experimentar_equacoes import MODELS, configurations

OUT=ROOT/'output/experimento_equacoes'


def original_certificates():
    results=[]
    blocks=(ROOT/'instances/original/Upload-SuperDrone/bks.txt').read_text().split('---')
    for block in blocks:
        lines=block.strip().splitlines()
        if not lines or not lines[0].endswith('.txt'):
            continue
        name=lines[0][:-4]
        if not (ROOT/f'instances/drone_times/tau_kj_{name}.txt').exists():
            continue
        speed,_,nodes,tau=dados_instancia(name)
        published=float(lines[1])
        try:
            truck=list(map(int,lines[2].split()[3:]))
            chains=[list(map(int,line.split()[3:])) for line in lines[3:] if line.strip()]
            counts=Counter(j for chain in chains for j in chain)
            predecessors={j:set() for j in counts}
            for chain in chains:
                for a,b in zip(chain,chain[1:]):
                    predecessors[b].add(a)
            ends,missions={},[]
            while len(ends)<len(counts):
                ready=[j for j in counts if j not in ends and predecessors[j]<=ends.keys()]
                if not ready:
                    raise ValueError('Ciclo')
                for j in ready:
                    k=counts[j]
                    duration=tau[j-1][k-1]
                    if duration<=0:
                        raise ValueError(f'Cliente {j} k={k} inviavel na matriz atual')
                    start=max((ends[p] for p in predecessors[j]),default=0)
                    ends[j]=start+duration
                    missions.append(dict(client=j,k=k,start=start,end=ends[j]))
            value=sum((abs(nodes[a][1]-nodes[b][1])+abs(nodes[a][2]-nodes[b][2]))/speed*60
                      for a,b in zip(truck,truck[1:]))
            cert=dict(routes=[[j for j in truck if j]],missions=missions,
                      makespan=max(value,max(ends.values(),default=0)))
            validated=validar(cert,name,1)
            results.append(dict(instance=name,reference=published,reconstructed=validated,
                                matches=math.isclose(published,validated)))
        except (ValueError,IndexError) as error:
            results.append(dict(instance=name,reference=published,error=str(error),matches=False))
    return results


def main():
    summary=json.loads((OUT/'summary.json').read_text())
    physics=json.loads((OUT/'physics_summary.json').read_text())
    rows=list(csv.DictReader((OUT/'comparison.csv').open()))
    originals=original_certificates()
    (OUT/'original_certificate_validation.json').write_text(json.dumps(originals,indent=2)+'\n')
    runs=[json.loads(p.read_text()) for p in (OUT/'runs').glob('*.json')]
    expected=len(configurations())*summary['repetitions']*len(MODELS)
    if len(runs)!=expected:
        raise ValueError(f'Esperados {expected} registros, encontrados {len(runs)}')
    data={}
    for model in MODELS:
        for name in {r['instance'] for r in runs}:
            speed,fleet,nodes,_=dados_instancia(name)
            times=tuple(tuple(map(int,line.split()[1:])) for line in
                        (OUT/model/f'tau_kj_{name}.txt').read_text().splitlines()[1:])
            data[model,name]=(speed,fleet,nodes,times)
    for run in runs:
        validar(run['best'],run['instance'],run['trucks'],data=data[run['model'],run['instance']])
    transfers=[]
    transfer_dir=OUT/'transferred_certificates'
    transfer_dir.mkdir(exist_ok=True)
    for row in rows:
        name,trucks=row['instance'],int(row['trucks'])
        baseline=min((r for r in runs if r['instance']==name and r['trucks']==trucks and r['model']=='official'),
                     key=lambda r:r['best']['makespan'])
        for model in MODELS[1:]:
            cert=deepcopy(baseline['best'])
            speed,_,nodes,times=data[model,name]
            for mission in cert['missions']:
                duration=times[mission['client']-1][mission['k']-1]
                if duration<=0 or duration>mission['end']-mission['start']+1e-7:
                    raise ValueError('Transferencia nao monotona')
                mission['end']=mission['start']+duration
            truck_time=0
            for route in cert['routes']:
                path=[0,*route,0]
                truck_time=max(truck_time,sum((abs(nodes[a][1]-nodes[b][1])+abs(nodes[a][2]-nodes[b][2]))/speed*60
                                              for a,b in zip(path,path[1:])))
            cert['makespan']=max(truck_time,max((m['end'] for m in cert['missions']),default=0))
            validar(cert,name,trucks,data=data[model,name])
            change=100*(cert['makespan']-baseline['best']['makespan'])/baseline['best']['makespan']
            transfers.append(dict(instance=name,trucks=trucks,model=model,official=baseline['best']['makespan'],
                                  transferred=cert['makespan'],change_pct=change))
            (transfer_dir/f'{name}_s{trucks}_{model}.json').write_text(json.dumps(cert)+'\n')
    with (OUT/'fixed_assignment_comparison.csv').open('w',newline='') as stream:
        writer=csv.DictWriter(stream,fieldnames=list(transfers[0]))
        writer.writeheader();writer.writerows(transfers)
    transfer_summary={}
    for group in ('PDSVRP-C','PDSTSP-C'):
        transfer_summary[group]={}
        for model in MODELS[1:]:
            subset=[r for r in transfers if r['model']==model and ((r['trucks']>1)==(group=='PDSVRP-C'))]
            transfer_summary[group][model]=dict(configurations=len(subset),
                mean_change_pct=statistics.mean(r['change_pct'] for r in subset),
                improved=sum(r['change_pct']<-1e-7 for r in subset),
                equal=sum(abs(r['change_pct'])<=1e-7 for r in subset),
                worse=sum(r['change_pct']>1e-7 for r in subset))
    (OUT/'fixed_assignment_summary.json').write_text(json.dumps(transfer_summary,indent=2)+'\n')
    paired={}
    bykey={(r['instance'],r['trucks'],r['repetition'],r['model']):r['best']['makespan'] for r in runs}
    for model in MODELS[1:]:
        changes=[]
        for r in runs:
            if r['model']!='official':
                continue
            baseline=r['best']['makespan']
            changed=bykey[r['instance'],r['trucks'],r['repetition'],model]
            changes.append(100*(changed-baseline)/baseline)
        paired[model]=dict(pairs=len(changes),mean_change_pct=statistics.mean(changes),
                           improved=sum(v<-1e-7 for v in changes),
                           equal=sum(abs(v)<=1e-7 for v in changes),worse=sum(v>1e-7 for v in changes))
    (OUT/'paired_summary.json').write_text(json.dumps(paired,indent=2)+'\n')
    labels={'official':'Tabelas oficiais','equations_1min':'Equacoes + preparo 1 min',
            'equations_1sec':'Equacoes + preparo 1 s'}
    text=[
        '# Validacao das equacoes do artigo fundador: todas as instancias', '',
        'Data: 17/11/2026. Objetivo em minutos; menor e melhor.', '',
        '## Escopo e escolhas explicitas', '',
        'Foram conferidas visualmente as pp.870-874, 880 e 883 de Nguyen e Ha, '
        '*The Parallel Drone Scheduling Traveling Salesman Problem with Collective Drones*, '
        'Transportation Science 57(4), 866-888 (2023), DOI 10.1287/trsc.2022.1192. '
        'O SHA-256 do PDF e os hashes dos fontes estao em environment.json.', '',
        'Nao existe uma interpretacao literal unica e fisicamente coerente de todas as '
        'linhas impressas: a Eq.(4) imprime consumo >=90% da capacidade, mas o texto '
        'exige no maximo 90%; a Tabela 5 diz preparo de 1 segundo e o texto diz 1 minuto; '
        'a linha 14 do Algoritmo 2 repete vin onde o texto descreve vin e vout. '
        'A restricao de bateria usada neste experimento e consumo <=0,9*Cnom; '
        'os pares vin/vout sao independentes. Nao foi aplicado o sinal >= da Eq.(4), '
        'pois ele permitiria missoes acima da capacidade e excluiria algumas de baixo consumo. '
        'Assim, este experimento aplica as equacoes fisicas impressas com essas correcoes '
        'textuais explicitadas; nao afirma uma reproducao literal sem excecoes.', '',
        '| Cenario | Definicao |','| --- | --- |',
        '| Tabelas oficiais | Matrizes distribuidas, sem alteracao. |',
        '| Equacoes + 1 min | Eq.(24) SEM o termo de arrasto quadratico adicional do C++; '
        'Eq.(25) resolvida por bissecao (80 iteracoes); preparo de 1 min para k>1. |',
        '| Equacoes + 1 s | Mesma fisica anterior; preparo de 1 s para k>1, conforme Tabela 5. |','',
        'Em ambos os cenarios recalculados: Eq.(23) para decolagem/pouso; seis fases de voo; '
        'carga w/k na ida e zero na volta; corrente P/V limitada em todas as fases (Eq.2); '
        'Peukert (Eq.3), reserva de 10%, altitude 100 m, velocidades verticais 2 m/s; '
        'busca dos pares de velocidades entre 4 e 28 m/s com passo 0,2; Eq.(1), '
        'conversao para minutos e teto; maximo de seis drones por formacao e massa total '
        '<=25 kg. Coeficientes calibrados, geometria e aproximacao numerica de pi sao '
        'os do codigo original disponibilizado, pois o PDF nao fornece sozinho todos '
        'os detalhes executaveis da calibracao.', '',
        'A potencia de cruzeiro foi pre-calculada por velocidade/carga para evitar '
        'repeti-la para cada par vin/vout. A ordem e os valores de ponto flutuante da '
        'malha original foram conservados. A reproducao do codigo distribuido continuou '
        'com exatamente as duas divergencias previamente conhecidas.', '',
        'O otimizador permanece o SA+BB atual. As Eqs.(5)-(22) do MILP fundador sao '
        'representadas pelas restricoes de rotas, cobertura, duracoes e recurso cumulativo; '
        'nao foi implementado nem resolvido esse MILP ou substituido o SA pelo RnR. '
        'Com varios caminhoes, usa-se a extensao PDSVRP-C, e nao o modelo de um caminhao '
        'literal do fundador. Manhattan e velocidade dos caminhoes foram preservadas.', '',
        '## Validacao fisica', '',
        f"Foram recalculadas {physics['instances']} instancias, {physics['pairs']} pares cliente/k. "
        f"Todas as {physics['independently_validated_feasible_pairs']} combinacoes viaveis "
        'da fisica impressa tiveram angulo, potencia, corrente, Peukert, reserva de bateria, '
        'massa e duracao reavaliados independentemente em Python. Esta conferencia valida '
        'a viabilidade das velocidades selecionadas; nao e uma segunda enumeracao independente '
        'de todos os pares de velocidades nem validacao experimental de drones reais.', '',
        '| Matriz | Entradas diferentes da oficial |','| --- | ---: |']
    for model in ('recalculated','equations_1min','equations_1sec'):
        text.append(f"| {labels.get(model,'Reproducao do C++ original')} | {physics['differing_pairs'][model]} |")
    text += ['', '| Cenario | Novos pares viaveis | Tempos menores em pares ja viaveis | Tempos maiores | Pares que ficaram inviaveis |',
             '| --- | ---: | ---: | ---: | ---: |']
    for model in MODELS[1:]:
        diffs=physics['differences'][model]
        text.append(f"| {labels[model]} | "
                    f"{sum(d['official']==-1 and d['recalculated']>0 for d in diffs)} | "
                    f"{sum(0<d['recalculated']<d['official'] for d in diffs)} | "
                    f"{sum(d['recalculated']>d['official']>0 for d in diffs)} | "
                    f"{sum(d['official']>0 and d['recalculated']==-1 for d in diffs)} |")
    text += ['', 'As alteracoes fisicas observadas so reduzem duracoes ou acrescentam '
             'alternativas viaveis. Por isso o otimo teorico nao piora, mas uma execucao '
             'curta da heuristica pode encontrar solucao pior por seguir outra trajetoria.']
    text += ['', '## Protocolo de otimizacao', '',
        f"Foram executadas {summary['configurations']} configuracoes: as 180 do catalogo PDSVRP-C "
        'e as 60 instancias com S=1 do PDSTSP-C fundador. Para S=1, a referencia numerica '
        'e bks.txt do pacote original, sem assumir que todos os seus valores sejam otimos '
        'ou que representem resultados posteriores. Para S>1, usa-se o catalogo local '
        'COR2024/MIC2024, sem busca exaustiva de novos BKS.', '',
        f"Tres cenarios, {summary['repetitions']} sementes por cenario e configuracao, "
        f"{summary['seconds_per_run']:g} segundos por execucao, {summary['workers']} processos simultaneos: "
        f"{summary['runs']} execucoes, {summary['checkpoints']} checkpoints validados. "
        f"{summary['completed']} execucoes encerraram antes do limite; as demais conservam "
        'o melhor checkpoint anterior ao corte. Semente efetiva: '
        '(42 + 10*n + S + 1000003*r) modulo 2^32, r=0,1,2.', '',
        'As tabelas abaixo usam o melhor valor das tres repeticoes por configuracao. '
        'Medias percentuais sao aritmeticas dos percentuais individuais; valores '
        'negativos indicam reducao. O limite curto pode cortar a calibracao do SA, '
        'e as mudancas de tau alteram trajetorias. Esta e uma comparacao exploratoria, '
        'sem prova geral de otimalidade ou significancia estatistica. Tempos de execucao '
        'nao sao comparados diretamente com computadores e solvers dos artigos.', '',
        f"Certificados originais S=1 reconstruidos com sucesso e objetivo coincidente: "
        f"{sum(r['matches'] for r in originals)}/{len(originals)}. "
        'Detalhes e eventuais incompatibilidades estao em original_certificate_validation.json.', '']
    for group in ('PDSVRP-C','PDSTSP-C'):
        text += [f'## Resultados agregados: {group}', '',
                 '| Cenario | Gap medio vs referencia % | Abaixo / igual / acima da referencia | Melhor / igual / pior que oficial | Mudanca media vs oficial % |',
                 '| --- | ---: | ---: | ---: | ---: |']
        for model in MODELS:
            a=summary['aggregates'][group][model]
            text.append(f"| {labels[model]} | {a['mean_gap_pct']:.3f} | "
                        f"{a['below_reference']} / {a['equal_reference']} / {a['above_reference']} | "
                        f"{a['better_than_official']} / {a['equal_official']} / {a['worse_than_official']} | "
                        f"{a['mean_change_pct']:.3f} |")
        text += ['', 'Compatibilidade necessaria: UB publicado >= limite inferior local por cliente. '
                 'Passar esse teste nao demonstra identidade entre os dados dos artigos e os locais.', '',
                 '| Cenario | Referencias que passam o teste | Gap medio nessas referencias % |',
                 '| --- | ---: | ---: |']
        for model in MODELS:
            a=summary['aggregates'][group][model]
            text.append(f"| {labels[model]} | {a['reference_possible']}/{a['configurations']} | {a['mean_gap_possible_pct']:.3f} |")
        text.append('')
    text += ['## Efeito fisico mantendo as mesmas decisoes', '',
             'Para isolar a mudanca fisica, o melhor certificado oficial de cada configuracao '
             'foi transferido para cada nova matriz. Foram mantidos rotas, clientes, k e '
             'instantes de partida; somente os terminos de missoes foram atualizados. '
             'Todos os 480 certificados resultantes foram validados, incluindo capacidade. '
             'Isso nao acrescenta outra busca nem e o resultado de uma execucao SA com warm start.', '',
             '| Grupo | Cenario | Melhor / igual / pior | Mudanca media % |',
             '| --- | --- | ---: | ---: |']
    for group in transfer_summary:
        for model,a in transfer_summary[group].items():
            text.append(f"| {group} | {labels[model]} | {a['improved']} / {a['equal']} / {a['worse']} | {a['mean_change_pct']:.3f} |")
    text += ['', '## Interpretacao', '',
             'Melhorar contra a tabela oficial significa obter menor objetivo no respectivo '
             'cenario fisico. As novas equacoes alteram tempos e combinacoes permitidas, '
             'portanto um objetivo inferior ao publicado nao constitui automaticamente '
             'novo recorde para o conjunto original. A matriz exata usada em MIC 2024 '
             'continua indisponivel. Comparacoes S=1 usam o certificado distribuido do '
             'fundador; S>1 usa a literatura da extensao com varios caminhoes.', '',
             '## 126-rc-e', '',
             'O cliente 46 passa de inviavel por drones para 54 min com k=6 '
             '(53 min no preparo de 1 s). Entretanto, o cliente 97 permanece '
             'inviavel por drones e exige ida e volta de caminhao de 108 min. '
             'Logo, as mudancas nao explicam o UB MIC de 76 para S=5.', '',
             '| S | Referencia | Oficial | Equacoes 1 min | Equacoes 1 s |',
             '| --- | ---: | ---: | ---: | ---: |']
    for r in sorted((r for r in rows if r['instance']=='126-rc-e'),key=lambda r:int(r['trucks'])):
        text.append('| '+' | '.join([r['trucks'],f"{float(r['literature_ub']):.3f}",
                    *(f'{float(r[m]):.3f}' for m in MODELS)])+' |')
    text += ['', '## Todas as configuracoes', '',
             '| Instancia | S | Referencia | Oficial | Equacoes 1 min | Equacoes 1 s | Gap Eq.1min % |',
             '| --- | ---: | ---: | ---: | ---: | ---: | ---: |']
    for r in sorted(rows,key=lambda r:(int(r['instance'].split('-')[0]),r['instance'],int(r['trucks']))):
        text.append('| '+' | '.join([r['instance'],r['trucks'],f"{float(r['literature_ub']):.3f}",
                    *(f'{float(r[m]):.3f}' for m in MODELS),f"{float(r['equations_1min_gap_pct']):.3f}"])+' |')
    text += ['', '## Reproducao e arquivos', '',
             '`python scripts/experimentar_equacoes.py --seconds 2 --repetitions 3 --workers 4`', '',
             '`python scripts/relatar_equacoes.py`', '',
             'O executor retoma execucoes individuais ja registradas com mesmos parametros. '
             'Para um experimento novo, use outro --output. Os fontes de producao, '
             'executavel principal, instancias oficiais e results/best.json sao preservados.', '',
             'Saidas: output/experimento_equacoes/physics.csv, physics_summary.json, '
             'comparison.csv, summary.json, paired_summary.json, environment.json, '
             'original_certificate_validation.json, fixed_assignment_comparison.csv, '
             'fixed_assignment_summary.json, transferred_certificates/*.json e runs/*.json com certificados.', '',
             'Referencias: https://doi.org/10.1287/trsc.2022.1192 ; '
             'https://doi.org/10.1016/j.cor.2023.106514 ; '
             'https://doi.org/10.1007/978-3-031-62912-9_13 .']
    report='\n'.join(text)+'\n'
    (OUT/'relatorio.md').write_text(report,encoding='utf-8')
    (ROOT/'docs/auditorias/equacoes_completas_20261117.md').write_text(report,encoding='utf-8')
    print(json.dumps(dict(revalidated=len(runs),original_certificates_matching=sum(r['matches'] for r in originals),
                          paired=paired),indent=2))


if __name__=='__main__':
    main()
