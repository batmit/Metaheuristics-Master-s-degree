"""Recalculo fisico e experimento pareado isolado nas 60 instancias/240 configuracoes."""
import argparse
import concurrent.futures
import csv
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import statistics
import subprocess
import threading
import time

from executar_benchmark import ROOT, catalogo, dados_instancia, validar

MODELS = ('official', 'equations_1min', 'equations_1sec')


def physics(out):
    original = ROOT/'instances/original/Upload-SuperDrone/power-consumption-calculation'
    source = (original/'energymodel.cpp').read_text(encoding='utf-8-sig')
    source = source.replace('#include "gnuplot-iostream.h"',
                            '#include <cassert>\n#include <cstdio>\nstatic bool paper=false;')
    source = source[:source.index('void EnergyModel::show_discharge_curve(')] + source[source.index('EnergyModel::EnergyModel('):]
    term = 'pow( get_c4(dc)*vh*vh , 2 )'
    if source.count(term) != 1:
        raise ValueError('Equacao 24 inesperada')
    source = source.replace(term, '(paper ? 0.0 : '+term+')')
    signature = 'double EnergyModel::get_alpha(const DroneConfig &dc, const double &w_parcel, const double &vh)\n{'
    if source.count(signature) != 1:
        raise ValueError('Equacao 25 inesperada')
    source = source.replace(signature, signature+r'''
    if(paper) {
        double lo=0,hi=90;
        for(int iteration=0;iteration<80;++iteration) {
            double angle=(lo+hi)/2, rad=angle*PI/180;
            double lhs=tan(rad)*((dc.getW_single_drone()+w_parcel/dc.num_drones)*g-
                       get_c5(dc)*pow(vh*cos(rad),2));
            if(lhs<get_c4(dc)*vh*vh) lo=angle; else hi=angle;
        }
        return (lo+hi)/2;
    }
''')
    begin = source.index('std::vector<double> EnergyModel::get_optimal_power(')
    end = source.index('//double EnergyModel::get_endurance', begin)
    # Potencia depende de velocidade/carga, nao da outra velocidade do par.
    # Pre-calcular os 242 valores preserva a malha original e evita repeti-los.
    source = source[:begin] + r'''
std::vector<double> EnergyModel::get_optimal_power(const DroneConfig &dc, const double &w, const double &d) {
    double currents[]={get_P_takeoff(dc,w)/dc.voltage,get_P_landing(dc,w)/dc.voltage,
                       get_P_takeoff(dc,0)/dc.voltage,get_P_landing(dc,0)/dc.voltage};
    double left=dc.single_capacity;
    double eff[4];
    for(int i=0;i<4;++i) {
        if(currents[i]>dc.max_discharge_current) return {-1,-1,-1};
        eff[i]=currents[i]*pow(currents[i]/dc.single_capacity,peukert_constant-1);
    }
    left -= (eff[0]+eff[2])*(h_flight/v_takeoff)/3600;
    left -= (eff[1]+eff[3])*(h_flight/v_landing)/3600;
    if(!dc.checkCapacity(left)) return {-1,-1,-1};
    struct Cruise {double v, energy;};
    vector<Cruise> incoming,outgoing;
    for(double v=4;v<=28;v+=0.2) {
        double i=get_P_flight(dc,w,v)/dc.voltage;
        if(i<=dc.max_discharge_current) incoming.push_back({v,i*pow(i/dc.single_capacity,peukert_constant-1)*(d/v)/3600});
    }
    for(double v=28;v>=4;v-=0.2) {
        double i=get_P_flight(dc,0,v)/dc.voltage;
        if(i<=dc.max_discharge_current) outgoing.push_back({v,i*pow(i/dc.single_capacity,peukert_constant-1)*(d/v)/3600});
    }
    double vin=-1,vout=-1,used=-1;
    for(const auto &a:incoming) {
        double after=left-a.energy;
        if(!dc.checkCapacity(after)) continue;
        for(const auto &b:outgoing) {
            double remain=after-b.energy;
            if(!dc.checkCapacity(remain)) continue;
            bool faster=1/vin+1/vout>1/a.v+1/b.v;
            bool friendly=1/vin+1/vout==1/a.v+1/b.v && dc.single_capacity-remain<used;
            if(vin==-1 || faster || friendly) {vin=a.v;vout=b.v;used=dc.single_capacity-remain;}
        }
    }
    return {vin,vout,used};
}

''' + source[end:]
    paths = sorted((ROOT/'instances/benchmark').glob('*/*.txt'))
    cases = []
    records = []
    for i, path in enumerate(paths):
        speed, fleet, nodes, tau = dados_instancia(path.stem)
        params = [float(s.split(',')[1]) for s in path.read_text().splitlines()[:6]]
        cases.append(dict(name=path.stem, data=(speed,fleet,nodes,tau), params=params))
        for j, node in enumerate(nodes[1:],1):
            records.append((i,j,fleet,node[1]-nodes[0][1],node[2]-nodes[0][2],node[3],*params[2:]))
    source += '\nstatic const double cases[][10] = {' + ','.join(
        '{'+','.join(map(str,r))+'}' for r in records) + '};\n'
    source += r'''
int main() {
    EnergyModel model(100,2,2,1.22,1.1);
    puts("instance_index,client,k,mode,tau,vin,vout,used_Ah,mass_kg");
    for(int mode=0;mode<2;++mode) {
        paper=mode==1;
        for(const auto &r:cases) {
            int id=(int)r[0],j=(int)r[1],fleet=(int)r[2];
            double distance=hypot(r[3],r[4])*1000;
            for(int k=1;k<=fleet;++k) {
                double mass=k*(1.2+r[6])+r[5];
                double duration=-1,vin=-1,vout=-1,used=-1;
                if(k<=6 && mass<=25) {
                    DroneConfig dc(k,1.2,r[6],4,0.15,r[7],r[9],r[8]);
                    auto v=model.get_optimal_power(dc,r[5],distance);
                    if(v[0]>0) {
                        vin=v[0];vout=v[1];used=v[2];
                        duration=ceil((distance/vin+distance/vout+200)/60+(k>1?1:0));
                    }
                }
                printf("%d,%d,%d,%d,%.17g,%.17g,%.17g,%.17g,%.17g\n",
                       id,j,k,mode,duration,vin,vout,used,mass);
            }
        }
    }
}
'''
    (out/'energymodel.h').write_bytes((original/'energymodel.h').read_bytes())
    cpp = out/'physics.cpp'
    cpp.write_text(source, encoding='utf-8')
    binary = out/('physics.exe' if os.name=='nt' else 'physics')
    subprocess.run(['g++','-std=c++11','-O2',str(cpp),'-o',str(binary)],check=True)
    print('Recalculando as 60 instancias: codigo distribuido e equacoes impressas...',flush=True)
    with (out/'physics.csv').open('w') as stream:
        subprocess.run([str(binary)],stdout=stream,check=True)
    tables = {m:{c['name']:[list(r) for r in c['data'][3]] for c in cases}
              for m in ('recalculated',*MODELS)}
    differences = {m:[] for m in ('recalculated','equations_1min','equations_1sec')}
    validated = 0
    # Reavalia independentemente em Python as seis fases de CADA par fisico viavel.
    with (out/'physics.csv').open() as stream:
        for row in csv.DictReader(stream):
            i,j,k,mode = (int(row[n]) for n in ('instance_index','client','k','mode'))
            case=cases[i]
            duration=int(float(row['tau']))
            model='recalculated' if mode==0 else 'equations_1min'
            tables[model][case['name']][j-1][k-1]=duration
            if mode==1:
                seconds=-1
                if duration>0:
                    vin,vout,used=(float(row[n]) for n in ('vin','vout','used_Ah'))
                    check_physics(case,j,k,vin,vout,used,duration)
                    validated+=1
                    nodes=case['data'][2]
                    d=math.hypot(nodes[j][1]-nodes[0][1],nodes[j][2]-nodes[0][2])*1000
                    seconds=math.ceil((d/vin+d/vout+200+(1 if k>1 else 0))/60)
                tables['equations_1sec'][case['name']][j-1][k-1]=seconds
    for model in differences:
        for case in cases:
            name=case['name']
            for j,(old,new) in enumerate(zip(case['data'][3],tables[model][name]),1):
                for k,(a,b) in enumerate(zip(old,new),1):
                    if a!=b:
                        differences[model].append(dict(instance=name,client=j,k=k,official=a,recalculated=b))
    for model in tables:
        folder=out/model
        folder.mkdir(exist_ok=True)
        for case in cases:
            name=case['name']
            text='request id '+' '.join(f'{k}-unit' for k in range(1,case['data'][1]+1))+'\n'
            text+=''.join(str(j)+' '+' '.join(map(str,r))+'\n' for j,r in enumerate(tables[model][name],1))
            (folder/f'tau_kj_{name}.txt').write_text(text)
    summary=dict(instances=len(cases),pairs=sum(len(c['data'][3])*c['data'][1] for c in cases),
                 independently_validated_feasible_pairs=validated,
                 differing_pairs={m:len(d) for m,d in differences.items()},differences=differences)
    (out/'physics_summary.json').write_text(json.dumps(summary,indent=2)+'\n')
    print(json.dumps({k:v for k,v in summary.items() if k!='differences'}),flush=True)
    return cases,tables


def check_physics(case,j,k,vin,vout,used,duration):
    _,_,nodes,_=case['data']
    _,_,battery,capacity,max_i,voltage=case['params']
    mass=1.2+battery
    parcel=nodes[j][3]
    if k>6 or k*mass+parcel>25 or not (4-1e-8<=vin<=28+1e-8 and 4-1e-8<=vout<=28+1e-8):
        raise ValueError('Limites fisicos')
    area=k*4*3.14159*0.15**2
    c1=0.8554/math.sqrt(2*1.22*area)
    def vertical(weight):
        thrust=(mass+weight/k)*9.81
        return 0.8554*thrust*(1+math.sqrt(1+thrust/(2*1.22*area)))+0.3177*thrust**1.5
    def horizontal(weight,speed):
        thrust=(mass+weight/k)*9.81
        lo,hi=0,math.pi/2
        for _ in range(70):
            a=(lo+hi)/2
            if math.tan(a)*(thrust-0.0279*(speed*math.cos(a))**2)<0.0296*speed**2:
                lo=a
            else:
                hi=a
        a=(lo+hi)/2
        residual=math.tan(a)*(thrust-0.0279*(speed*math.cos(a))**2)-0.0296*speed**2
        if abs(residual)>1e-7:
            raise ValueError('Equacao 25')
        return (c1+0.3177)*((thrust-0.0279*(speed*math.cos(a))**2)**2)**0.75+0.0296*speed**3
    d=math.hypot(nodes[j][1]-nodes[0][1],nodes[j][2]-nodes[0][2])*1000
    phases=[(vertical(parcel),50),(horizontal(parcel,vin),d/vin),(vertical(parcel),50),
            (vertical(0),50),(horizontal(0,vout),d/vout),(vertical(0),50)]
    energy=0
    for power,seconds in phases:
        current=power/voltage
        if current>max_i+1e-7:
            raise ValueError('Corrente maxima')
        energy+=current*(current/capacity)**0.1*seconds/3600
    if energy>0.9*capacity+1e-7 or not math.isclose(energy,used,rel_tol=1e-8,abs_tol=1e-8):
        raise ValueError(f'Peukert: {energy} versus {used}')
    if duration!=math.ceil((d/vin+d/vout+200)/60+(k>1)):
        raise ValueError('Duracao')


def configurations():
    rows=[dict(r,source_group='PDSVRP-C') for r in catalogo()]
    blocks=(ROOT/'instances/original/Upload-SuperDrone/bks.txt').read_text().split('---')
    for block in blocks:
        lines=block.strip().splitlines()
        if lines and lines[0].endswith('.txt'):
            name=lines[0][:-4]
            if (ROOT/f'instances/drone_times/tau_kj_{name}.txt').exists():
                rows.append(dict(instance=name,trucks=1,ub=float(lines[1]),lb=None,
                                 source_group='PDSTSP-C-original-certificate'))
    if len(rows)!=240:
        raise ValueError(f'Esperadas 240 configuracoes, encontradas {len(rows)}')
    return rows


def benchmark(out,cases,tables,args):
    binary=out/('worker.exe' if os.name=='nt' else 'worker')
    subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2',
                    'main.c','lerInstancias.c','cv.c','drone_bb.c','SA.c','-o',str(binary),'-lm'],cwd=ROOT,check=True)
    cases={c['name']:c for c in cases}
    rows=configurations()
    tasks=[(row,rep,model) for row in rows for rep in range(args.repetitions) for model in MODELS]
    folder=out/'runs'
    folder.mkdir(exist_ok=True)
    def run(task):
        row,rep,model=task
        name,trucks=row['instance'],int(row['trucks'])
        seed=(42+int(name.split('-')[0])*10+trucks+rep*1000003)%2**32
        record=folder/f'{name}_s{trucks}_{model}_r{rep+1}.json'
        data=(*cases[name]['data'][:3],tables[model][name])
        if record.exists():
            cached=json.loads(record.read_text())
            if cached['limit_seconds']!=args.seconds or cached['seed']!=seed:
                raise ValueError('Parametros diferentes no diretorio existente')
            validar(cached['best'],name,trucks,data=data)
            return cached
        size='small' if int(name.split('-')[0])<=30 else 'large'
        command=[str(binary),f'instances/benchmark/{size}/{name}.txt',
                 str(out/model/f'tau_kj_{name}.txt'),str(trucks),str(seed)]
        start=time.perf_counter()
        best=None
        count=0
        timeout=threading.Event()
        with subprocess.Popen(command,cwd=ROOT,env=dict(os.environ,PDSVRP_WORKER='1'),
                              stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True) as p:
            def stop():
                if p.poll() is None:
                    timeout.set()
                    p.kill()
            timer=threading.Timer(args.seconds,stop)
            timer.start()
            try:
                for line in p.stdout:
                    if line.startswith('CHECKPOINT '):
                        cert=json.loads(line[len('CHECKPOINT '):])
                        validar(cert,name,trucks,data=data)
                        count+=1
                        if best is None or cert['makespan']<best['makespan']:
                            best=cert
                p.wait()
                error=p.stderr.read()
            finally:
                timer.cancel()
                timer.join()
                if p.poll() is None:
                    p.kill()
                p.wait()
        if best is None or (p.returncode and not timeout.is_set()):
            raise RuntimeError(f'{name} S={trucks} {model}: {error}')
        result=dict(instance=name,trucks=trucks,model=model,repetition=rep+1,seed=seed,
                    limit_seconds=args.seconds,seconds=time.perf_counter()-start,
                    status='timeout' if timeout.is_set() else 'completed',checkpoints=count,best=best)
        record.write_text(json.dumps(result)+'\n')
        return result
    runs=[]
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.workers) as pool:
        for result in pool.map(run,tasks):
            runs.append(result)
            if len(runs)%60==0:
                print(f'{len(runs)}/{len(tasks)} execucoes validadas',flush=True)
    summarize(out,rows,runs,cases,tables,args)


def summarize(out,rows,runs,cases,tables,args):
    comparison=[]
    for row in rows:
        name,trucks=row['instance'],int(row['trucks'])
        ub=float(row['ub'])
        group=[r for r in runs if r['instance']==name and r['trucks']==trucks]
        values={m:min(r['best']['makespan'] for r in group if r['model']==m) for m in MODELS}
        item=dict(instance=name,trucks=trucks,reference=row['source_group'],literature_ub=ub,**values)
        for m in MODELS:
            item[m+'_gap_pct']=100*(values[m]-ub)/ub
        for m in MODELS[1:]:
            item[m+'_change_pct']=100*(values[m]-values['official'])/values['official']
        for m in MODELS:
            speed,_,nodes,_=cases[name]['data']
            lb=max(min(2*(abs(node[1]-nodes[0][1])+abs(node[2]-nodes[0][2]))/speed*60,
                       min((t for t in tables[m][name][j-1] if t>0),default=math.inf))
                   for j,node in enumerate(nodes[1:],1))
            item[m+'_local_lb']=lb
            item[m+'_reference_possible']=ub+1e-7>=lb
        comparison.append(item)
    with (out/'comparison.csv').open('w',newline='') as stream:
        writer=csv.DictWriter(stream,fieldnames=list(comparison[0]))
        writer.writeheader();writer.writerows(comparison)
    aggregates={}
    for label,subset in [('all',comparison),('PDSVRP-C',[r for r in comparison if r['trucks']>1]),
                         ('PDSTSP-C',[r for r in comparison if r['trucks']==1])]:
        aggregates[label]={}
        for m in MODELS:
            possible=[r for r in subset if r[m+'_reference_possible']]
            aggregates[label][m]=dict(configurations=len(subset),
                mean_gap_pct=statistics.mean(r[m+'_gap_pct'] for r in subset),
                reference_possible=len(possible),
                mean_gap_possible_pct=statistics.mean(r[m+'_gap_pct'] for r in possible) if possible else None,
                below_reference=sum(r[m]<r['literature_ub']-1e-7 for r in subset),
                equal_reference=sum(abs(r[m]-r['literature_ub'])<=1e-7 for r in subset),
                above_reference=sum(r[m]>r['literature_ub']+1e-7 for r in subset),
                better_than_official=sum(r[m]<r['official']-1e-7 for r in subset),
                equal_official=sum(abs(r[m]-r['official'])<=1e-7 for r in subset),
                worse_than_official=sum(r[m]>r['official']+1e-7 for r in subset),
                mean_change_pct=statistics.mean(100*(r[m]-r['official'])/r['official'] for r in subset))
    summary=dict(configurations=len(rows),runs=len(runs),repetitions=args.repetitions,
                 seconds_per_run=args.seconds,workers=args.workers,
                 checkpoints=sum(r['checkpoints'] for r in runs),
                 completed=sum(r['status']=='completed' for r in runs),aggregates=aggregates)
    (out/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
    print(json.dumps(summary,indent=2),flush=True)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seconds',type=float,default=2)
    parser.add_argument('--repetitions',type=int,default=3)
    parser.add_argument('--workers',type=int,default=4)
    parser.add_argument('--output',type=Path,default=ROOT/'output/experimento_equacoes')
    args=parser.parse_args()
    if args.seconds<=0 or args.repetitions<1 or args.workers<1:
        parser.error('Parametros devem ser positivos')
    out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    metadata=dict(platform=platform.platform(),python=platform.python_version(),
                  gcc=subprocess.check_output(['gcc','--version'],text=True).splitlines()[0],
                  pdf_sha256=hashlib.sha256((ROOT/'output/pdf/revisao126_20261111/artigo.pdf').read_bytes()).hexdigest(),
                  source_sha256={n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest()
                                 for n in ['main.c','cv.c','lerInstancias.c','SA.c','drone_bb.c',
                                           'scripts/experimentar_equacoes.py']})
    (out/'environment.json').write_text(json.dumps(metadata,indent=2)+'\n')
    cases,tables=physics(out)
    benchmark(out,cases,tables,args)


if __name__=='__main__':
    main()
