"""Audita as 60 tabelas fisicas e as convencoes do benchmark, sem alterar entradas.

Reproducao pelo codigo energetico original, com regras textuais do artigo:
seis drones por formacao, massa total <=25kg e preparacao coletiva de 1min.
As ambiguidades entre o texto impresso e o codigo permanecem documentadas.
"""
import csv
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import subprocess
import time

from executar_benchmark import ROOT, dados_instancia

OUT = ROOT/'output/auditoria_pdsvrpc'


def main(exclude=(), output=OUT):
    """Recalcula com parametros e deposito de cada instancia, nao os da 126."""
    output.mkdir(parents=True,exist_ok=True)
    paths = sorted(p for p in (ROOT/'instances/benchmark').glob('*/*.txt') if p.stem not in exclude)
    if not paths:
        raise ValueError('Nenhuma instancia selecionada para auditoria')
    cases, records = [], []
    for index,path in enumerate(paths):
        lines = path.read_text().splitlines()
        params = {line.split(',')[0]:float(line.split(',')[1]) for line in lines[:6]}
        speed,fleet,nodes,tau = dados_instancia(path.stem)
        cases.append(dict(instance=path.stem,trucks_speed=speed,fleet=fleet,nodes=nodes,tau=tau,
                          params=params,sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
        for j, node in enumerate(nodes[1:],1):
            records.append([index,j,fleet,node[1]-nodes[0][1],node[2]-nodes[0][2],node[3],
                            params['BATTERY_WEIGHT'],params['BATTERY_CAPACITY'],
                            params['MAX_DISCHARGE_CURRENT'],params['VOLTAGE']])
    original = ROOT/'instances/original/Upload-SuperDrone/power-consumption-calculation'
    source = (original/'energymodel.cpp').read_text()
    source = source.replace('#include "gnuplot-iostream.h"','#include <cassert>\n#include <cmath>\n#include <cstdio>')
    source = source[:source.index('void EnergyModel::show_discharge_curve(')]+source[source.index('EnergyModel::EnergyModel('):]
    (output/'energymodel.h').write_bytes((original/'energymodel.h').read_bytes())
    source += '\nstatic const double cases[][10] = {'+','.join('{'+','.join(map(str,r))+'}' for r in records)+'};\n'
    source += r'''
int main() {
    EnergyModel model(100,2,2,1.22,1.1);
    puts("instance_index,client,k,duration,vin,vout,consumed_Ah,total_mass_kg");
    for (const auto &r : cases) {
        int id=(int)r[0], j=(int)r[1], fleet=(int)r[2];
        double distance=sqrt(r[3]*r[3]+r[4]*r[4])*1000;
        for (int k=1; k<=fleet; ++k) {
            double mass=k*(1.2+r[6])+r[5];
            double duration=-1, vin=-1, vout=-1, consumed=-1;
            if (k<=6 && mass<=25) {
                DroneConfig dc(k,1.2,r[6],4,0.15,r[7],r[9],r[8]);
                auto v=model.get_optimal_power(dc,r[5],distance);
                if (v[0]>0) {
                    vin=v[0]; vout=v[1]; consumed=v[2];
                    duration=ceil((distance/vin+distance/vout+200)/60+(k>1 ? 1:0));
                }
            }
            printf("%d,%d,%d,%.17g,%.17g,%.17g,%.17g,%.17g\n",id,j,k,duration,vin,vout,consumed,mass);
        }
    }
}
'''
    cpp = output/'recalculo.cpp'
    cpp.write_text(source)
    binary = output/('recalculo.exe' if os.name=='nt' else 'recalculo')
    subprocess.run(['g++','-std=c++11','-O2',str(cpp),'-o',str(binary)],check=True)
    start=time.monotonic()
    print(f'Recalculando {len(paths)} tabelas pelo modelo original...',flush=True)
    with (output/'pares_fisicos.csv').open('w') as stream:
        subprocess.run([str(binary)],stdout=stream,check=True,timeout=300)
    computed = {i:{j:[-1]*case['fleet'] for j in range(1,len(case['nodes']))} for i,case in enumerate(cases)}
    differences=[]
    total=0
    with (output/'pares_fisicos.csv').open() as stream:
        for row in csv.DictReader(stream):
            i,j,k = (int(row[f]) for f in ('instance_index','client','k'))
            actual=int(float(row['duration']))
            expected=cases[i]['tau'][j-1][k-1]
            computed[i][j][k-1]=actual
            total+=1
            if actual!=expected:
                differences.append(dict(instance=cases[i]['instance'],client=j,k=k,table=expected,recalculated=actual))
    target=output/'tau_recalculados'
    target.mkdir(exist_ok=True)
    per_case=[]
    for i,case in enumerate(cases):
        name=case['instance']
        original_tau=ROOT/f'instances/drone_times/tau_kj_{name}.txt'
        content=original_tau.read_text().splitlines()[0]+'\n'
        content+='\n'.join(str(j)+' '+' '.join(map(str,values)) for j,values in computed[i].items())+'\n'
        (target/original_tau.name).write_text(content)
        nodes=case['nodes']
        fractional=0
        for a in nodes:
            for b in nodes:
                value=(abs(a[1]-b[1])+abs(a[2]-b[2]))/case['trucks_speed']*60
                if abs(value-round(value))>1e-9: fractional+=1
        per_case.append(dict(instance=name,clients=len(nodes)-1,fleet=case['fleet'],
                             differing_pairs=sum(d['instance']==name for d in differences),
                             truck_edges_with_fractional_minutes=fractional))
    summary=dict(instances=len(cases),clients=len(records),pairs=total,
                 matching_pairs=total-len(differences),differing_pairs=len(differences),
                 differences=differences,per_instance=per_case,
                 fractional_truck_edges=sum(c['truck_edges_with_fractional_minutes'] for c in per_case),
                 seconds=time.monotonic()-start,
                 scope='Codigo fisico original + regras textuais; nao equivalencia literal de cada equacao impressa.')
    summary['excluded'] = sorted(exclude)
    (output/'tabelas_fisicas.json').write_text(json.dumps(summary,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print(json.dumps({k:v for k,v in summary.items() if k not in ('per_instance','differences')},indent=2),flush=True)
    if differences: print('Primeiras divergencias:',differences[:10],flush=True)


if __name__=='__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exclude', action='append', default=[], help='Instancia a excluir pelo nome.')
    parser.add_argument('--output', type=Path, default=OUT, help='Diretorio isolado para os resultados.')
    args = parser.parse_args()
    main(args.exclude, args.output)
