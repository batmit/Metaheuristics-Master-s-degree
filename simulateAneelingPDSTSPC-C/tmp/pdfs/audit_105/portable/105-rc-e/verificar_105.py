"""Auditoria independente. Executar com Python 3; usa apenas a biblioteca padrao.

Nao importa nem chama calcularMakespan, o escalonador C ou o B&B.
Os arquivos de fonte congelados acompanham o relatorio.
"""
from __future__ import annotations

import copy
import csv
from fractions import Fraction as F
import hashlib
import json
from pathlib import Path
import re
import statistics

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
SRC = HERE / "fontes"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def parse_instance(path):
    lines = path.read_text().splitlines()
    params = {s.split(',')[0]: F(s.split(',')[1].strip()) for s in lines[:6]}
    coords, weights = {}, {}
    for line in lines[7:]:
        if not line.strip():
            continue
        i, x, y, w = line.split()
        assert int(i) not in coords
        coords[int(i)] = (F(x), F(y))
        weights[int(i)] = F(w)
    assert set(coords) == set(range(106))
    assert params['NUM_DRONES'] == 12 and params['TRUCK_SPEED'] == 30
    return params, coords, weights


PARAMS, XY, WEIGHTS = parse_instance(SRC / "105-rc-e.txt")
FLEET = int(PARAMS['NUM_DRONES'])
TAU = {}
for line in (SRC / "tau_kj_105-rc-e.txt").read_text().splitlines()[1:]:
    v = line.split()
    if v:
        assert len(v) == FLEET + 1
        TAU[int(v[0])] = {k: F(t) for k, t in enumerate(v[1:], 1)}
assert set(TAU) == set(range(1, 106))


def duration(client, k):
    assert 1 <= k <= FLEET
    assert client in TAU and k in TAU[client]
    t = TAU[client][k]
    assert t >= 0, f"Quantidade inviavel: cliente {client}, k={k}"
    return t


def parse_record(text):
    line = next(s for s in text.splitlines() if s.startswith('FINAL 105-rc-e '))
    head, tail = line.split(' ROUTE ')
    route, missions = tail.split(' MISSIONS')
    meta = dict(re.findall(r'(\w+)=([\d.]+)', head))
    result = {'route': list(map(int, route.split())), 'missions': [],
              'seed': int(head.split()[2]), 'temperature': float(meta['T0']),
              'claimed': F(meta['MAKESPAN'])}
    for priority, token in enumerate(missions.split(), 1):
        j, k, a, b = token.split(':')
        result['missions'].append({'client': int(j), 'k': int(k), 'start': F(a),
                                   'end': F(b), 'priority': priority})
    return result


def travel(a, b):
    return sum(abs(u-v) for u, v in zip(XY[a], XY[b])) / PARAMS['TRUCK_SPEED'] * 60


def check_capacity(missions):
    events = sorted({t for m in missions for t in (m['start'], m['end'])})
    intervals = []
    for a, b in zip(events, events[1:]):
        use = sum(m['k'] for m in missions if m['start'] <= a < m['end'])
        assert 0 <= use <= FLEET, f"Capacidade excedida no intervalo {a}..{b}: {use}"
        intervals.append({'start': a, 'end': b, 'use': use, 'free': FLEET-use})
    return intervals


def verify(solution):
    route, missions = solution['route'], solution['missions']
    clients = route + [m['client'] for m in missions]
    assert len(clients) == 105 and len(set(clients)) == 105, 'Cobertura/duplicata'
    assert set(clients) == set(range(1, 106)), 'Cliente ausente ou invalido'
    assert all(j in XY for j in route)
    arcs, total = [], F(0)
    for i, (a, b) in enumerate(zip([0]+route, route+[0]), 1):
        t = travel(a, b)
        assert t >= 0
        arcs.append({'arc': i, 'from': a, 'to': b, 'duration': t,
                     'departure': total, 'arrival': total+t})
        total += t
    for m in missions:
        assert m['start'] >= 0 and m['end'] >= m['start'], 'Horario invalido'
        assert m['end']-m['start'] == duration(m['client'], m['k']), 'Duracao divergente'
    intervals = check_capacity(missions)
    # Segunda verificacao: todos os tempos sao inteiros, entao basta cada celula.
    assert all(t.denominator == 1 for m in missions for t in (m['start'], m['end']))
    drone = max(m['end'] for m in missions)
    for t in range(int(drone)):
        assert sum(m['k'] for m in missions if m['start'] <= t < m['end']) <= FLEET
    # Testemunha construtiva com drones identificados, sem preempcao.
    available = [F(0)] * FLEET
    identifiers = {}
    for m in sorted(missions, key=lambda m: (m['start'], m['priority'])):
        ids = [i for i, t in enumerate(available) if t <= m['start']][:m['k']]
        assert len(ids) == m['k'], 'Nao foi possivel alocar drones fisicos'
        identifiers[m['client']] = [i+1 for i in ids]
        for i in ids:
            available[i] = m['end']
    makespan = max(total, drone)
    if 'claimed' in solution:
        assert makespan == solution['claimed'], 'Makespan declarado divergente'
    workload = sum(m['k'] * (m['end']-m['start']) for m in missions)
    return {'truck': total, 'drone': drone, 'makespan': makespan,
            'truck_clients': len(route), 'drone_clients': len(missions),
            'arcs': arcs, 'intervals': intervals, 'drone_ids': identifiers,
            'workload': workload, 'peak': max(x['use'] for x in intervals),
            'idle_horizon': FLEET*makespan-workload,
            'idle_drone_horizon': FLEET*drone-workload,
            'utilization': workload/(FLEET*makespan),
            'min_workload': sum(min(k*t for k,t in TAU[m['client']].items() if t>=0)
                                for m in missions)}


def first_fit_again(solution):
    """Outro algoritmo: enumera 0 e retornos, testa intervalos completos."""
    placed = []
    for m in solution['missions']:
        candidates = sorted({F(0)} | {x['end'] for x in placed})
        for a in candidates:
            trial = dict(m, start=a, end=a+duration(m['client'],m['k']))
            try:
                check_capacity(placed+[trial])
            except AssertionError:
                continue
            assert a == m['start'] and trial['end'] == m['end'], 'First-fit divergiu'
            placed.append(trial)
            break
        else:
            raise AssertionError('Sem janela no first-fit independente')
    return True


def from_drone_sequences(route, sequences):
    """Reconstrucao das missoes coletivas como DAG das precedencias por drone."""
    assert len(sequences) == FLEET
    nodes = {j for s in sequences for j in s}
    assert all(len(set(s)) == len(s) for s in sequences)
    demands = {j: sum(j in s for s in sequences) for j in nodes}
    preds = {j: set() for j in nodes}
    for seq in sequences:
        for a, b in zip(seq,seq[1:]): preds[b].add(a)
    finishes, missions = {}, []
    while len(finishes) < len(nodes):
        ready = sorted(j for j in nodes if j not in finishes and preds[j] <= finishes.keys())
        assert ready, 'Ciclo na sincronizacao publicada'
        for j in ready:
            a = max((finishes[p] for p in preds[j]), default=F(0))
            finishes[j] = a + duration(j,demands[j])
            missions.append({'client':j, 'k':demands[j], 'start':a, 'end':finishes[j],
                             'priority':len(missions)+1})
    return {'route':route, 'missions':missions}


def numeric(obj):
    if isinstance(obj,F): return int(obj) if obj.denominator==1 else float(obj)
    if isinstance(obj,dict): return {str(k):numeric(v) for k,v in obj.items()}
    if isinstance(obj,(list,tuple)): return [numeric(x) for x in obj]
    return obj


def save_json(name,obj):
    (HERE/name).write_text(json.dumps(numeric(obj),ensure_ascii=False,indent=2),encoding='utf-8')


def save_csv(name,rows):
    with (HERE/name).open('w',newline='',encoding='utf-8') as f:
        w=csv.DictWriter(f,fieldnames=list(rows[0])); w.writeheader()
        w.writerows(numeric(rows))


def main():
    record=(SRC/'solucao_registrada.txt').read_text(encoding='utf-8')
    ours=parse_record(record); audit=verify(ours); first_fit_again(ours)
    replay=(HERE/'reproducao.txt').read_text(encoding='utf-8-sig').strip()
    assert replay == record.strip(), 'Reproducao C difere da solucao registrada'
    source_hashes={p.name:digest(p) for p in SRC.iterdir() if p.is_file()}
    catalog=list(csv.DictReader((SRC/'catalog_105.csv').open(encoding='utf-8-sig')))
    row=next(x for x in catalog if x['instance']=='105-rc-e')
    # Todos os hashes do par aparecem na linha original do catalogo.
    assert source_hashes['105-rc-e.txt'] in row.values()
    assert source_hashes['tau_kj_105-rc-e.txt'] in row.values()
    for name in ['105-rc-e.txt','tau_kj_105-rc-e.txt']:
        project_file=ROOT/'instances/pdstsp-c/large'/name
        if project_file.exists():
            assert digest(project_file) == source_hashes[name]
    # Fonte de 2025: metadados, decisao e prova reportada; nao confundir objetivo codificado com makespan.
    log=(SRC/'orlab_2025_105.log').read_text()
    for marker in ['n_customer: 105','n_drone: 12','truck_speed: 30','n_truck: 1',
                   'status: OPTIMAL','Makespan: 112','Makespan Bound: 112']:
        assert marker in log
    rt=list(map(int,re.findall(r'^Truck 0: (.+)$',log,re.M)[-1].split()))
    assert rt[0]==rt[-1]==0
    seq={int(i):list(map(int,s.split())) for i,s in re.findall(r'^Drone (\d+): (.*)$',log,re.M)}
    official=from_drone_sequences(rt[1:-1],[seq[i] for i in range(FLEET)])
    official['claimed']=F(112); check_official=verify(official)
    # Referencia original de 120: os conjuntos de drones devem sincronizar nas mesmas missoes.
    bks=(SRC/'original_bks_105.txt').read_text().splitlines()
    bks=[s for s in bks if s.strip()]
    rt=list(map(int,bks[2].split()[3:])); assert rt[0]==rt[-1]==0
    seq=[list(map(int,s.split()[3:])) for s in bks[3:]]
    historical=from_drone_sequences(rt[1:-1],seq); historical['claimed']=F(120)
    check_historical=verify(historical)
    # Testes negativos: a auditoria tem que rejeitar certificados corrompidos.
    invalid=[]
    for name in ['duplicata','ausente','duracao','capacidade','quantidade','negativo']:
        bad=copy.deepcopy(ours)
        if name=='duplicata': bad['route'][1]=bad['route'][0]
        elif name=='ausente': bad['route'].pop()
        elif name=='duracao': bad['missions'][0]['end']+=1
        elif name=='capacidade':
            for m in bad['missions']: m['start']=F(0); m['end']=duration(m['client'],m['k'])
        elif name=='quantidade': bad['missions'][0]['k']=13
        elif name=='negativo': bad['missions'][0]['start']-=1; bad['missions'][0]['end']-=1
        try: verify(bad)
        except AssertionError: invalid.append(name)
        else: raise AssertionError('Certificado corrompido foi aceito: '+name)
    # Estatisticas das 100 execucoes originais, extraidas das linhas individuais.
    raw=(SRC/'execucoes_105_originais.txt').read_text(encoding='utf-8-sig')
    entries=[]
    for run,seed,values in re.findall(r'Execucao (\d+)/100; semente (\d+)\s*\n(105-rc-e\s+[^\n]+OK)',raw):
        v=values.split()
        entries.append({'run':int(run),'seed':int(seed),'makespan':float(v[4]),
                        'initial':float(v[3]),'temperature':float(v[8]),'cpu':float(v[9])})
    assert len(entries)==100 and len({e['seed'] for e in entries})==100
    vals=[e['makespan'] for e in entries]
    stats={'n':len(vals),'best':min(vals),'worst':max(vals),'mean':statistics.mean(vals),
           'median':statistics.median(vals),'sample_std':statistics.stdev(vals),
           'below_2024':sum(v<120 for v in vals),'equal_2024':vals.count(120),
           'equal_2025':vals.count(112),'total_cpu':sum(e['cpu'] for e in entries),
           'best_runs':[e for e in entries if e['makespan']==min(vals)]}
    assert stats['best']==116 and abs(stats['mean']-124.67)<1e-10
    # Diagnostico de vizinhancas, mantendo atribuicoes e cronograma dos drones.
    rt=ours['route']; cost=audit['truck']; best2=cost; best_reloc=cost
    def cost_route(route): return sum(travel(a,b) for a,b in zip([0]+route,route+[0]))
    for i in range(len(rt)):
        for j in range(i+1,len(rt)):
            best2=min(best2,cost_route(rt[:i]+list(reversed(rt[i:j+1]))+rt[j+1:]))
        removed=rt[:i]+rt[i+1:]
        for j in range(len(rt)):
            best_reloc=min(best_reloc,cost_route(removed[:j]+[rt[i]]+removed[j:]))
    summary={'date':'2026-10-26','instance':'105-rc-e','validated':True,
             'ours':audit,'official_2025':check_official,'historical_2024':check_historical,
             'statistics':stats,'source_hashes':source_hashes,'negative_tests_rejected':invalid,
             'exact_reproduction':True,'independent_first_fit':True,
             'best_truck_2opt':best2,'best_truck_relocate':best_reloc,
             'delta_vs_120_percent':F(-10,3),'gap_vs_112_percent':F(25,7),
             'optimality_scope':'112 reportado OPTIMAL no log ORLab; prova CP nao reexecutada nesta auditoria.'}
    save_json('validacao.json',summary)
    save_json('solucao_116.json',ours); save_json('solucao_orlab_112.json',official)
    save_json('solucao_historica_120.json',historical)
    save_csv('arcos_caminhao_116.csv',audit['arcs'])
    save_csv('ocupacao_116.csv',audit['intervals'])
    save_csv('missoes_116.csv',[dict(m,drone_ids=' '.join(map(str,audit['drone_ids'][m['client']]))) for m in ours['missions']])
    save_csv('execucoes_100.csv',entries)
    print(json.dumps(numeric({'validated':True,'ours':[audit['truck'],audit['drone'],audit['makespan']],
                             'official':[check_official['truck'],check_official['drone'],check_official['makespan']],
                             'historical':[check_historical['truck'],check_historical['drone'],check_historical['makespan']],
                             'counts':[audit['truck_clients'],audit['drone_clients']],
                             'workload':audit['workload'],'utilization':audit['utilization'],
                             'min_workload':audit['min_workload'],'stats':stats,
                             'truck_local':[best2,best_reloc],'negative_tests':len(invalid)}),indent=2))


if __name__=='__main__':
    main()
