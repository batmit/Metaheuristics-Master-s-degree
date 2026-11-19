"""Revisao isolada do PDF: energia minima, tempos, sensibilidades e prova local."""
import csv
import hashlib
import json
import math
import os
from pathlib import Path
import subprocess
from collections import Counter

from executar_benchmark import ROOT, dados_instancia, validar

OUT = ROOT/'output/pdf/revisao126_20261111'


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    original = ROOT/'instances/original/Upload-SuperDrone/power-consumption-calculation'
    source = (original/'energymodel.cpp').read_text()
    source = source.replace('#include "gnuplot-iostream.h"', '#include <cassert>\n#include <cstdio>\n#include <algorithm>\nstatic bool printed_equation = false;\nstatic bool exact_tilt = false;')
    source = source[:source.index('void EnergyModel::show_discharge_curve(')] + source[source.index('EnergyModel::EnergyModel('):]
    term = 'pow( get_c4(dc)*vh*vh , 2 )'
    assert source.count(term) == 1
    source = source.replace(term, '(printed_equation ? 0.0 : '+term+')')
    alpha_signature = 'double EnergyModel::get_alpha(const DroneConfig &dc, const double &w_parcel, const double &vh)\n{'
    assert source.count(alpha_signature) == 1
    source = source.replace(alpha_signature, alpha_signature + r'''
    if (exact_tilt) {
        double low=0, high=90;
        for(int iteration=0;iteration<100;++iteration) {
            double angle=(low+high)/2, radians=angle*PI/180;
            double lhs=tan(radians)*((dc.getW_single_drone()+w_parcel/dc.num_drones)*g-
                       get_c5(dc)*pow(vh*cos(radians),2));
            if(lhs<get_c4(dc)*vh*vh) low=angle; else high=angle;
        }
        return (low+high)/2;
    }
''')
    speed, fleet, nodes, tau = dados_instancia('126-rc-e')
    params = {line.split(',')[0]: float(line.split(',')[1]) for line in
              (ROOT/'instances/benchmark/large/126-rc-e.txt').read_text().splitlines()[:6]}
    (OUT/'energymodel.h').write_bytes((original/'energymodel.h').read_bytes())
    source += '\nstatic const double customers[][4] = {' + ','.join(
        '{'+','.join(map(str, (j, node[1]-nodes[0][1], node[2]-nodes[0][2], node[3])))+'}'
        for j, node in enumerate(nodes[1:], 1)) + '};\n'
    source += r'''
struct Cruise { double v, t, current, effective; };
int main() {
    EnergyModel model(100,2,2,1.22,1.1);
    puts("mode,client,k,total_mass_kg,vertical_max_A,min_energy_Ah,max_distance_km,raw_tau,vin,vout,used_Ah");
    const char *names[]={"distributed","printed24","exact_tilt_distributed","exact_tilt_printed24"};
    for (int mode=0; mode<4; ++mode) {
        printed_equation = mode == 1 || mode == 3;
        exact_tilt = mode >= 2;
        for (const auto &r: customers) for (int k=1; k<=12; ++k) {
            DroneConfig dc(k,1.2,1.55,4,0.15,12,12.8,24);
            double d=hypot(r[1],r[2])*1000;
            double currents[4]={model.get_P_takeoff(dc,r[3])/12.8,
                model.get_P_landing(dc,r[3])/12.8, model.get_P_takeoff(dc,0)/12.8,
                model.get_P_landing(dc,0)/12.8};
            double vertical=0, max_vertical=0;
            for (double i: currents) {vertical += i*pow(i/12,0.1)*50/3600; max_vertical=std::max(i,max_vertical);}
            vector<Cruise> incoming,outgoing;
            double e_in=INFINITY,e_out=INFINITY;
            for (int direction=0; direction<2; ++direction) {
                // Mesma malha de ponto flutuante do codigo original.
                for(double v=direction?28:4; direction?v>=4:v<=28; v+=direction?-0.2:0.2) {
                    double i=model.get_P_flight(dc,direction?0:r[3],v)/12.8;
                    if(i>24) continue;
                    double eff=i*pow(i/12,0.1);
                    (direction?outgoing:incoming).push_back({v,d/v,i,eff});
                    if(direction) e_out=std::min(e_out,eff/v); else e_in=std::min(e_in,eff/v);
                }
            }
            double min_energy=vertical+d*(e_in+e_out)/3600;
            double max_distance=std::max(0.0,(10.8-vertical)*3600/(e_in+e_out))/1000;
            double best=INFINITY,bvin=-1,bvout=-1,used=-1;
            if(max_vertical<=24) for(const auto &a:incoming) for(const auto &b:outgoing) {
                double energy=vertical+(a.effective*a.t+b.effective*b.t)/3600;
                double time=a.t+b.t;
                if(energy<=10.8 && (time<best || (time==best && energy<used))) {
                    best=time;bvin=a.v;bvout=b.v;used=energy;
                }
            }
            double duration=std::isfinite(best)?ceil((best+200)/60+(k>1?1:0)):-1;
            printf("%s,%d,%d,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g\n",
                names[mode],(int)r[0],k,k*2.75+r[3],max_vertical,
                min_energy,max_distance,duration,bvin,bvout,used);
        }
    }
}
'''
    cpp = OUT/'sensibilidade.cpp'
    cpp.write_text(source)
    binary = OUT/('sensibilidade.exe' if os.name == 'nt' else 'sensibilidade')
    subprocess.run(['g++', '-std=c++11', '-O2', str(cpp), '-o', str(binary)], check=True)
    with (OUT/'fisica.csv').open('w') as stream:
        subprocess.run([str(binary)], stdout=stream, check=True, timeout=120)
    with (OUT/'fisica.csv').open() as stream:
        rows = list(csv.DictReader(stream))
    by_mode = {}
    for row in rows:
        row = {key: (value if key == 'mode' else float(value)) for key, value in row.items()}
        by_mode.setdefault(row['mode'], {})[int(row['client']), int(row['k'])] = row
    differences, scenarios = [], {}
    for mode, physics in by_mode.items():
        for cap in (6, 12):
            for mass_limit in (25, math.inf):
                label = f'{mode}_k{cap}_mass{mass_limit}'
                values, witnesses = [], []
                for j in range(1, len(nodes)):
                    times = []
                    for k in range(1, fleet+1):
                        row = physics[j, k]
                        allowed = k <= cap and row['total_mass_kg'] <= mass_limit
                        value = int(row['raw_tau']) if allowed else -1
                        times.append(value)
                        if label == 'distributed_k6_mass25' and value != tau[j-1][k-1]:
                            differences.append(dict(client=j, k=k, table=tau[j-1][k-1], recalculated=value))
                    node = nodes[j]
                    truck = 2*(abs(node[1]-nodes[0][1])+abs(node[2]-nodes[0][2]))/speed*60
                    drone = min((v for v in times if v > 0), default=math.inf)
                    bound = min(truck, drone)
                    values.append(bound)
                    if bound > 76:
                        witnesses.append(dict(client=j, truck_round_trip=truck,
                                              min_drone=None if math.isinf(drone) else drone, lower_bound=bound))
                scenarios[label] = dict(local_lb=max(values), obstructions_to_76=witnesses)
    saved = json.loads((ROOT/'results/best.json').read_text(encoding='utf-8'))
    cert = saved.get('126-rc-e_s5', {}).get('best')
    upper = validar(cert, '126-rc-e', 5) if cert else None
    if cert:
        (OUT/'certificado_s5.json').write_text(json.dumps(cert, indent=2)+'\n')
    # Reconstroi a solucao original RnR a partir das cadeias individuais.
    blocks = (ROOT/'instances/original/Upload-SuperDrone/bks.txt').read_text().split('---')
    block = next(b.strip().splitlines() for b in blocks if b.strip().startswith('126-rc-e.txt'))
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
            raise ValueError('Ciclo nas cadeias de drones')
        for j in ready:
            k = counts[j]
            start = max((ends[p] for p in predecessors[j]), default=0)
            duration = tau[j-1][k-1]
            if duration <= 0:
                raise ValueError('Missao original inviavel')
            ends[j] = start+duration
            missions.append(dict(client=j, k=k, start=start, end=ends[j]))
    route = [j for j in truck if j != 0]
    truck_value = sum((abs(nodes[a][1]-nodes[b][1])+abs(nodes[a][2]-nodes[b][2]))/speed*60
                      for a, b in zip(truck, truck[1:]))
    original_cert = dict(routes=[route], missions=missions, makespan=max(truck_value, max(ends.values())))
    original_value = validar(original_cert, '126-rc-e', 1)
    (OUT/'certificado_rnr_s1.json').write_text(json.dumps(original_cert, indent=2)+'\n')
    hashes = {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
              for p in [ROOT/'instances/benchmark/large/126-rc-e.txt',
                        ROOT/'instances/drone_times/tau_kj_126-rc-e.txt', OUT/'artigo.pdf']}
    summary = dict(parameters=params, original_matrix_comparison=dict(pairs=126*12,
                   matching=126*12-len(differences), differences=differences), scenarios=scenarios,
                   selected_client46=[r for r in rows if int(r['client']) == 46],
                   certificate_s5_value=upper, original_rnr=dict(value=original_value,
                   truck_time=truck_value, drone_time=max(ends.values()), truck_clients=len(route),
                   drone_missions=len(missions)), sha256=hashes)
    (OUT/'resultado.json').write_text(json.dumps(summary, indent=2, allow_nan=False)+'\n')
    print(json.dumps(dict(comparison=summary['original_matrix_comparison'],
                          lower_bounds={key: value['local_lb'] for key, value in scenarios.items()},
                          certificate_s5_value=upper, original_rnr=summary['original_rnr']), indent=2), flush=True)


if __name__ == '__main__':
    main()
