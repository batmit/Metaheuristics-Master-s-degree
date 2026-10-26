"""Cruza 60 casos do PDSTSP-c: 2024 x ORLab 2025; valida testemunhas.

Leitura somente: nao modifica o algoritmo, main ou as referencias historicas.
Usa somente biblioteca padrao do Python. Matrizes comparadas byte a byte.
"""
from pathlib import Path
from fractions import Fraction as F
import csv
import hashlib
import io
import json
import re
import zipfile

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
SOURCE=HERE/'fontes'
URL='https://orlab.com.vn/download/data-and-results-of-pdstsp-c-and-pdsop-c/'


def last(pattern,text):
    found=re.findall(pattern,text,re.M)
    assert found,pattern
    return found[-1]


def verify_witness(log,instance,tau,claimed):
    lines=instance.decode().splitlines()
    params={x.split(',')[0]:F(x.split(',')[1].strip()) for x in lines[:6]}
    m=int(params['NUM_DRONES']); speed=params['TRUCK_SPEED']
    points={int(v[0]):(F(v[1]),F(v[2])) for line in lines[7:] if (v:=line.split())}
    n=len(points)-1; assert set(points)==set(range(n+1))
    times={int(v[0]):[F(t) for t in v[1:]] for line in tau.decode().splitlines()[1:] if (v:=line.split())}
    assert set(times)==set(range(1,n+1)) and all(len(t)==m for t in times.values())
    assert int(last(r'n_customer: (\d+)',log))==n
    assert int(last(r'n_drone: (\d+)',log))==m
    assert F(last(r'truck_speed: ([\d.]+)',log))==speed
    assert int(last(r'n_truck: (\d+)',log))==1
    route=list(map(int,last(r'^Truck 0: (.+)$',log).split()))
    assert route[0]==route[-1]==0 and route.count(0)==2
    seq={int(i):list(map(int,s.split())) for i,s in re.findall(r'^Drone (\d+):[ \t]*(.*)$',log,re.M)}
    assert set(seq)==set(range(m))
    assert all(len(x)==len(set(x)) for x in seq.values())
    nodes={j for line in seq.values() for j in line}
    assert len(route[1:-1])+len(nodes)==n
    assert set(route[1:-1]).isdisjoint(nodes)
    assert set(route[1:-1])|nodes==set(range(1,n+1))
    truck=sum((abs(points[a][0]-points[b][0])+abs(points[a][1]-points[b][1]))*60/speed for a,b in zip(route,route[1:]))
    demands={j:sum(j in line for line in seq.values()) for j in nodes}
    preds={j:set() for j in nodes}
    for line in seq.values():
        for a,b in zip(line,line[1:]): preds[b].add(a)
    ends={}; missions=[]
    while len(ends)<len(nodes):
        ready=sorted(j for j in nodes if j not in ends and preds[j]<=ends.keys())
        assert ready,'Ciclo nas precedencias de sincronizacao'
        for j in ready:
            k=demands[j]; assert 1<=k<=m
            duration=times[j][k-1];assert duration>=0
            start=max((ends[a] for a in preds[j]),default=F(0))
            ends[j]=start+duration
            missions.append({'client':j,'k':k,'start':start,'end':ends[j]})
    for t in sorted({x['start'] for x in missions}):
        use=sum(x['k'] for x in missions if x['start']<=t<x['end'])
        assert use<=m
    # As sequencias originais comprovam identidades e sincronizacao, nao so capacidade agregada.
    for line in seq.values():
        for a,b in zip(line,line[1:]):
            current=next(x for x in missions if x['client']==b)
            assert ends[a]<=current['start']
    drone=max(ends.values(),default=F(0))
    assert max(truck,drone)==claimed,(truck,drone,claimed)
    return {'n':n,'drones':m,'truck':int(truck) if truck.denominator==1 else float(truck),
            'drone':int(drone) if drone.denominator==1 else float(drone),
            'makespan':int(claimed),'route':route,
            'missions':[{k:int(v) if isinstance(v,F) and v.denominator==1 else float(v) if isinstance(v,F) else v for k,v in x.items()} for x in missions],
            'drone_sequences':seq,'verified':True}


def write_csv(name,rows):
    with (HERE/name).open('w',encoding='utf-8-sig',newline='') as f:
        w=csv.DictWriter(f,fieldnames=list(rows[0]));w.writeheader();w.writerows(rows)


def main():
    package=ROOT/'tmp/pdfs/audit_105/orlab_2025.zip'
    package_sha=hashlib.sha256(package.read_bytes()).hexdigest()
    outer=zipfile.ZipFile(package)
    data=zipfile.ZipFile(io.BytesIO(outer.read('PDSOP-c/data.zip')))
    logs=zipfile.ZipFile(io.BytesIO(outer.read('PDSOP-c/results.zip')))
    (SOURCE/'logs_tsp').mkdir(exist_ok=True)
    cells=json.loads((SOURCE/'planilha_celulas.json').read_text())
    sheet={}
    for row in cells['rows']:
        found=[(k,v) for k,v in row.items() if k.startswith('A') and re.fullmatch(r'\d+-(?:r|c|rc)-(?:r|c|e)',v)]
        if not found: continue
        cell,name=found[0];i=re.search(r'\d+',cell).group()
        sheet[name]={'row':int(i),'old_lb':F(row['G'+i]),'old_ub':F(row['H'+i]),
                     'new_lb':F(row['J'+i]),'new_ub':F(row['K'+i])}
    catalog=list(csv.DictReader((ROOT/'instances/pdstsp-c/catalog.csv').open(encoding='utf-8-sig')))
    old_unproven={r['instance']:r for r in csv.DictReader((ROOT/'instances/pdstsp-c/literature_unproven.csv').open())}
    historical={r['instance']:F(r['best_known_minutes']) for r in csv.DictReader((ROOT/'instances/pdstsp-c/literature_small_medium.csv').open())}
    historical.update({n:F(r['best_known_minutes']) for n,r in old_unproven.items()})
    assert len(sheet)==len(catalog)==60
    assert {n for n,r in sheet.items() if r['old_lb']<r['old_ub']}==set(old_unproven)
    results=[]; certificates={}
    for entry in sorted(catalog,key=lambda x:(int(x['customers']),x['instance'])):
        n=entry['instance']; s=sheet[n]
        if n in historical: assert historical[n]==s['old_ub']
        if n in old_unproven:assert F(old_unproven[n]['lower_bound'])==s['old_lb']
        inst_path='data/instances/tsp/'+n+'.txt';tau_path='data/instances/tau_jk/tau_kj_'+n+'.txt'
        inst=data.read(inst_path);tau=data.read(tau_path)
        for content,path,field in [(inst,entry['instance_file'],'instance_sha256'),(tau,entry['drone_times_file'],'drone_times_sha256')]:
            assert content==(ROOT/'instances/pdstsp-c'/path).read_bytes()
            assert hashlib.sha256(content).hexdigest()==entry[field]
        log_path='results/logs/tsp/'+n+'.log'; raw=logs.read(log_path)
        (SOURCE/'logs_tsp'/(n+'.log')).write_bytes(raw); log=raw.decode()
        status=last(r'^status: (\w+)',log)
        lb=F(last(r'Makespan Bound: ([\d.]+)',log));ub=F(last(r'Makespan: ([\d.]+)',log))
        assert (lb,ub)==(s['new_lb'],s['new_ub'])
        assert lb<=ub<=s['old_ub']
        assert (status=='OPTIMAL')==(lb==ub)
        assert status in ['OPTIMAL','FEASIBLE']
        certificate=verify_witness(log,inst,tau,ub)
        assert certificate['n']==int(entry['customers']) and certificate['drones']==int(entry['drones'])
        certificates[n]=certificate
        row={'instance':n,'customers':certificate['n'],'drones':certificate['drones'],
             'lb_2024':int(s['old_lb']),'ub_2024':int(s['old_ub']),
             'lb_2025':int(lb),'ub_2025':int(ub),'status_2025':status,
             'optimal_reported':status=='OPTIMAL','improved_ub':ub<s['old_ub'],
             'improvement_minutes':int(s['old_ub']-ub),'improvement_percent':float(100*(s['old_ub']-ub)/s['old_ub']),
             'gap_ub_percent':float(100*(ub-lb)/ub),'solver_seconds':float(last(r'\[info\].*?Time: ([\d.]+)',log)),
             'closed_since_2024':n in old_unproven and status=='OPTIMAL',
             'witness_validated':True,'source_sheet':cells['sheet'],'source_range':f'A{s["row"]}:M{s["row"]}',
             'source_log':log_path,'source_log_sha256':hashlib.sha256(raw).hexdigest(),
             'source_url':URL}
        results.append(row)
    still=[r for r in results if not r['optimal_reported']]
    improved=[r for r in results if r['improved_ub']]
    closed=[r for r in results if r['closed_since_2024']]
    assert len(still)==4 and len(improved)==19 and len(closed)==19
    write_csv('comparacao_60_instancias.csv',results)
    write_csv('ainda_abertas.csv',still)
    write_csv('melhorias_2024_2025.csv',improved)
    summary={'review_date':'2026-10-26','package_date':'2025-11-10','source_url':URL,
             'package_sha256':package_sha,'total':60,'optimal_2024':37,'open_2024':23,
             'optimal_2025':56,'open_2025':4,'improved_upper_bounds':19,
             'new_optimality_reports':19,'witnesses_validated':60,
             'data_pairs_identical':60,'spreadsheet_log_agreements':60,
             'still_open':still,'improved':improved,'newly_closed':closed,
             'scope':'Fonte oficial 2025 consultada em 26/10/2026; provas dos solvers nao reexecutadas. Ausencia de otimo em fonte consultada nao e prova de ausencia em toda publicacao.'}
    (HERE/'resumo.json').write_text(json.dumps(summary,indent=2,ensure_ascii=False),encoding='utf-8')
    (HERE/'testemunhas_validadas.json').write_text(json.dumps(certificates,indent=2),encoding='utf-8')
    def md_table(headers,rows):
        return ['|'+'|'.join(headers)+'|','|'+'|'.join(['---']*len(headers))+'|']+['|'+'|'.join(map(str,row))+'|' for row in rows]
    md=['# Revisão das 60 instâncias PDSTSP-c: 2024 versus ORLab 2025','',
        'Data da revisão: **26/10/2026**. Todos os tempos de solução estão em minutos.','',
        '**Resultado principal:** 56 casos com ótimo reportado e quatro ainda abertos na fonte oficial consultada. '
        'Dos 23 abertos em 2024, 19 tiveram a otimalidade reportada posteriormente. Houve melhoria do limite superior em 19 casos, incluindo a 105-rc-e; portanto são 18 outras melhorias.','',
        '## Quais ainda estão abertos','']
    md+=md_table(['Instância','Clientes','Drones','UB 2024','LB 2025','UB 2025','Gap (%)'],
                 [[r['instance'],r['customers'],r['drones'],r['ub_2024'],r['lb_2025'],r['ub_2025'],f"{r['gap_ub_percent']:.4f}"] for r in still])
    md+=['','Gap = 100 × (UB − LB) / UB. UB é uma solução viável; LB é o limite inferior reportado. '
         'Os quatro logs terminam com status FEASIBLE após aproximadamente 3600 s, com LB < UB. '
         'Isso não prova otimalidade. As respectivas linhas da planilha são 36, 54, 55 e 58.','',
         'Três intervalos têm largura de um minuto (67-c-c, 167-r-e, 181-r-e). O intervalo da 165-r-c tem largura de quatro minutos. '
         'Um resultado igual a UB empata a referência; para melhorar é preciso ficar abaixo de UB e não abaixo de LB, se o limite publicado é válido.','',
         '## Soluções melhores que em 2024','']
    md+=md_table(['Instância','UB 2024','UB 2025','Redução','Redução (%)','Situação 2025'],
        [[r['instance'],r['ub_2024'],r['ub_2025'],r['improvement_minutes'],f"{r['improvement_percent']:.4f}",r['status_2025']] for r in improved])
    same=[r for r in closed if not r['improved_ub']]
    md+=['','## Ótimos provados sem alteração da melhor solução','',
         'Melhoria de solução e prova de ótimo são avanços diferentes. Estes casos já tinham o valor abaixo em 2024, mas a prova estava em aberto:','']
    md+=md_table(['Instância','Valor de 2024 e ótimo de 2025'],[[r['instance'],r['ub_2025']] for r in same])
    md+=['','## Cobertura por tamanho','',
         '- Pequenas (até 30 clientes): 30/30 com ótimo reportado.',
         '- Grandes (mais de 30 clientes, classificação do artigo): 26/30 com ótimo reportado; quatro abertas.',
         '- No subconjunto até 100 clientes usado pelo projeto, só a 67-c-c permanece aberta nesta fonte.',
         '- Não existe um limiar de tamanho que determine a ausência de ótimo: a 200-r-e, por exemplo, tem ótimo reportado de 200.','',
         '## Metodologia e grau de verificação','',
         '1. Li a aba PDSTSP-c da planilha oficial. G/H são os limites da referência anterior; J/K são os limites CP-interval. '
         'A aba contém 60 linhas de instâncias (linhas 3 a 62); os outros dados formatados não são instâncias adicionais.',
         '2. Conferi individualmente os 60 logs results/logs/tsp/*.log e os campos finais de status, Makespan e Makespan Bound. '
         'O objective codificado do CP-SAT não foi usado como tempo em minutos.',
         '3. Conferi os 60 pares de entrada e matriz contra os arquivos do projeto, byte a byte e por SHA-256. '
         'Os logs usam um caminhão, a frota do arquivo e velocidade 30.',
         '4. Reconstruí cada cronograma publicado pelas precedências de cada drone, sincronizando as missões coletivas. '
         'Recalculei a rota Manhattan, a duração de cada missão pela matriz oficial, a cobertura exata de todos os clientes e a capacidade simultânea com frações exatas.',
         '5. As 60 testemunhas resultaram no makespan declarado, e os 60 pares de limites dos logs coincidiram com J/K da planilha. '
         'Essa checagem valida a viabilidade, mas não reexecuta as provas de otimalidade dos solvers.',
         '6. Cruzei a classificação antiga com o CSV histórico do projeto: são exatamente os mesmos 23 casos abertos. '
         'Os valores históricos disponíveis no projeto coincidem com a planilha. As diferenças de rótulo 144-rc-r/144-rc-c e 173-rc-r/173-rc-c seguem a correspondência já documentada.','',
         '## Limitação temporal da conclusão','',
         'A consulta na internet desta revisão não identificou uma fonte primária posterior que encerrasse esses quatro casos. '
         'Isso não demonstra que tal resultado não exista. A afirmação verificável é que esses são os quatro casos sem prova '
         'no pacote oficial de 2025 examinado, cujo hash está em resumo.json. Os 56 casos fechados permanecem fechados para os mesmos dados e regras.',
         '', '## Consequência para os experimentos do projeto','',
         'A lista de 23 casos usada na main reflete 2024. Para testar somente casos ainda abertos de acordo com a fonte atualizada, '
         'o conjunto apropriado passa a ser 67-c-c, 165-r-c, 167-r-e e 181-r-e, com referências 74, 160, 180 e 216. '
         'Nenhuma configuração ou algoritmo de produção foi alterado nesta análise.','',
         '## Todas as 60 instâncias','']
    md+=md_table(['Instância','LB 2024','UB 2024','LB 2025','UB 2025','Status','Células da fonte'],
        [[r['instance'],r['lb_2024'],r['ub_2024'],r['lb_2025'],r['ub_2025'],r['status_2025'],r['source_range']] for r in results])
    md+=['','## Fontes','',
         '- [Montemanni, Dell’Amico e Corsini (2024), Tabelas 1 e 2](https://doi.org/10.1016/j.cor.2023.106514).',
         '- [Pacote oficial ORLab, dados e resultados de PDSTSP-c e PDSOP-c, 10/11/2025]('+URL+').',
         '- Arquivo interno: PDSOP-c/results.zip → Results of PDSTSP-c and PDSOP-c .xlsx, aba PDSTSP-c.',
         '- Logs internos: PDSOP-c/results.zip → results/logs/tsp/.',
         '- A planilha original está preservada em fontes/Resultados_ORLab_2025.xlsx e os 60 logs em fontes/logs_tsp/.','',
         '## Arquivos para revisão','',
         '- comparacao_60_instancias.csv: comparação completa com hashes, células e log por instância.',
         '- ainda_abertas.csv: os quatro casos com LB < UB.',
         '- melhorias_2024_2025.csv: os 19 casos com UB menor.',
         '- testemunhas_validadas.json: as 60 rotas e cronogramas reconstruídos.',
         '- resumo.json: contagens e limites da conclusão.','']
    (HERE/'relatorio_revisao_2024_2025.md').write_text('\n'.join(md),encoding='utf-8')
    print(json.dumps({k:v for k,v in summary.items() if k not in ['improved','newly_closed','still_open']},ensure_ascii=True,indent=2))
    print('OPEN',[(r['instance'],r['lb_2025'],r['ub_2025']) for r in still])
    print('IMPROVED',[(r['instance'],r['ub_2024'],r['ub_2025']) for r in improved])


if __name__=='__main__': main()
