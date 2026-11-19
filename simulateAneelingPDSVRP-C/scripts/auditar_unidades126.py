"""Confere unidades da 126-rc-e por arquivos oficiais e certificado original."""
from collections import Counter
from io import BytesIO
import json
import math
from zipfile import ZipFile

from executar_benchmark import ROOT, dados_instancia, validar


def main():
    name = '126-rc-e'
    path = ROOT/f'instances/benchmark/large/{name}.txt'
    tau_path = ROOT/f'instances/drone_times/tau_kj_{name}.txt'
    original = ROOT/f'instances/original/Upload-SuperDrone/instances/large/{name}.txt'
    speed, fleet, nodes, tau = dados_instancia(name)
    with ZipFile(ROOT/'instances/archives/pdstsp-c-pdsop-c-2025.zip') as outer:
        with ZipFile(BytesIO(outer.read('PDSOP-c/data.zip'))) as official:
            same_instance = path.read_bytes() == official.read(f'data/instances/tsp/{name}.txt')
            same_tau = tau_path.read_bytes() == official.read(f'data/instances/tau_jk/tau_kj_{name}.txt')
    if not (same_instance and same_tau and path.read_bytes() == original.read_bytes()):
        raise ValueError('Entradas locais divergem dos pacotes oficiais')
    blocks = (ROOT/'instances/original/Upload-SuperDrone/bks.txt').read_text().split('---')
    block = next(b.strip().splitlines() for b in blocks if b.strip().startswith(name+'.txt'))
    published = float(block[1])
    truck = list(map(int, block[2].split()[3:]))
    chains = [list(map(int, line.split()[3:])) for line in block[3:] if line.strip()]
    counts = Counter(j for chain in chains for j in chain)
    predecessors = {j: set() for j in counts}
    for chain in chains:
        for a, b in zip(chain, chain[1:]):
            predecessors[b].add(a)
    ends, missions = {}, []
    while len(ends) < len(counts):
        ready = [j for j in counts if j not in ends and predecessors[j] <= ends.keys()]
        if not ready:
            raise ValueError('Ciclo nas cadeias originais')
        for j in ready:
            k = counts[j]
            start = max((ends[p] for p in predecessors[j]), default=0)
            duration = tau[j-1][k-1]
            if duration <= 0:
                raise ValueError('Missao original inviavel')
            ends[j] = start+duration
            missions.append(dict(client=j, k=k, start=start, end=ends[j]))
    distance = sum(abs(nodes[a][1]-nodes[b][1])+abs(nodes[a][2]-nodes[b][2])
                   for a, b in zip(truck, truck[1:]))
    truck_minutes = distance/speed*60
    cert = dict(routes=[[j for j in truck if j]], missions=missions,
                makespan=max(truck_minutes, max(ends.values())))
    value = validar(cert, name, 1)
    if not math.isclose(value, published):
        raise ValueError('Objetivo original nao reproduzido')
    j = 46
    dx, dy = nodes[j][1]-nodes[0][1], nodes[j][2]-nodes[0][2]
    result = dict(instance=name, units=dict(coordinates='km', truck_speed='km/h',
                  objective_and_tau='min', energy_model_distance='m',
                  energy_model_speed='m/s', energy_model_time='s', capacity='Ah'),
                  original_instance_identical=True, official_2025_instance_identical=same_instance,
                  official_2025_tau_identical=same_tau, truck_speed=speed, fleet=fleet,
                  original_certificate=dict(published=published, truck_distance_km=distance,
                       truck_minutes=truck_minutes, drones_minutes=max(ends.values()),
                       validated_makespan=value, truck_customers=len(cert['routes'][0]),
                       drone_missions=len(missions)),
                  blocking_client=dict(id=j, coordinates=nodes[j][1:3], parcel_kg=nodes[j][3],
                       tau=tau[j-1], manhattan_roundtrip_km=2*(abs(dx)+abs(dy)),
                       manhattan_roundtrip_minutes=2*(abs(dx)+abs(dy))/speed*60,
                       euclidean_roundtrip_minutes=2*math.hypot(dx,dy)/speed*60),
                  literature_s5_ub=76,
                  qualification='Unidades locais consistentes; matriz exata MIC 2024 indisponivel.')
    out = ROOT/'output/auditoria_unidades126'
    out.mkdir(parents=True, exist_ok=True)
    (out/'resultado.json').write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
    (out/'certificado_original.json').write_text(json.dumps(cert, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
