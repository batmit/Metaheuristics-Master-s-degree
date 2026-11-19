"""Confere 14 certificados em C, Python e aritmetica decimal independente."""
import csv
from decimal import Decimal, ROUND_CEILING
from io import BytesIO
import hashlib
import json
import subprocess
from zipfile import ZipFile
from executar_benchmark import ROOT, validar


def main():
    folder=ROOT/'output/auditoria_professor'
    out=ROOT/'output/confirmacao_14'
    out.mkdir(exist_ok=True)
    binary=out/'auditar.exe'
    subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2',
        'lerInstancias.c','cv.c','drone_bb.c','SA.c','scripts/auditar_certificados.c',
        '-o',str(binary),'-lm'],cwd=ROOT,check=True)
    with ZipFile(ROOT/'instances/archives/pdstsp-c-pdsop-c-2025.zip') as outer:
        packed=outer.read('PDSOP-c/data.zip')
    records=[]
    with ZipFile(BytesIO(packed)) as official:
        for row in csv.DictReader((folder/'melhorias.csv').open()):
            name,s=row['instance'],int(row['trucks'])
            path=ROOT/f'instances/benchmark/large/{name}.txt'
            raw=path.read_bytes()
            original=ROOT/f'instances/original/Upload-SuperDrone/instances/large/{name}.txt'
            if raw!=original.read_bytes() or raw!=official.read(f'data/instances/tsp/{name}.txt'):
                raise ValueError('Arquivo diverge das fontes oficiais')
            cert=json.loads((folder/row['certificate']).read_text())['best']
            if cert['missions'] or len(cert['routes'])!=s:
                raise ValueError('Certificado diferente do esperado')
            lines=path.read_text().splitlines()
            speed=Decimal(lines[1].split(',')[1].strip())
            nodes={int(v[0]):(Decimal(v[1]),Decimal(v[2])) for line in lines[7:] if (v:=line.split())}
            served=[j for route in cert['routes'] for j in route]
            if sorted(served)!=list(range(1,len(nodes))):
                raise ValueError('Cobertura nao exata')
            durations=[]
            for route in cert['routes']:
                route_path=[0,*route,0]
                costs=[(abs(nodes[a][0]-nodes[b][0])+abs(nodes[a][1]-nodes[b][1]))*60/speed
                       for a,b in zip(route_path,route_path[1:])]
                if any(c!=c.to_integral_value(rounding=ROUND_CEILING) for c in costs):
                    raise ValueError('Aresta fracionaria: conferir convencao de arredondamento')
                durations.append(sum(costs,Decimal(0)))
            value=max(durations)
            if value!=Decimal(str(cert['makespan'])) or value>=Decimal(row['monotone_ub']):
                raise ValueError('Valor incorreto ou sem melhora')
            py=validar(cert,name,s)
            payload='\n'.join(str(len(r))+' '+' '.join(map(str,r)) for r in cert['routes'])
            payload+='\n0\n'+str(cert['makespan'])+'\n'
            checked=subprocess.run([str(binary),str(path),str(ROOT/f'instances/drone_times/tau_kj_{name}.txt'),str(s)],
                                   input=payload,capture_output=True,text=True,check=True)
            if Decimal(checked.stdout.strip())!=value or Decimal(str(py))!=value:
                raise ValueError('Validadores discordam')
            records.append(dict(row,customers=len(served),route_minutes=list(map(str,durations)),
                exact_minutes=str(value),c_valid=True,python_valid=True,decimal_valid=True,
                original_bytes_equal=True,official_2025_bytes_equal=True,
                instance_sha256=hashlib.sha256(raw).hexdigest(),
                certificate_sha256=hashlib.sha256((folder/row['certificate']).read_bytes()).hexdigest()))
    if len(records)!=14:
        raise ValueError('Quantidade inesperada')
    (out/'verificacao.json').write_text(json.dumps(records,indent=2)+'\n')
    report='''# Confirmacao dos 14 certificados — 17/11/2026

## Uso de drones e opcional

O PDSVRP-C classico nao exige nenhuma missao de drone. Na atribuicao de cada
cliente elegivel, as alternativas caminhao e drones somam um; e permitido
escolher caminhao para todos. Nas formulacoes de fluxo, definir z=0, f=0 e y=0
satisfaz a parte dos drones. O limite de frota e superior, nao minimo de uso.
Os tempos dos clientes atendidos por caminhoes continuam sendo definidos pelas
rotas; nao se deve zerar indiscriminadamente todas as variaveis temporais CP2.

Evidencia textual direta: MIC 2024, p.137, apresenta o caso sem drones como
min-max VRP e utiliza solucoes so de caminhoes como hints. A secao 3, p.141,
explica a construcao dessas solucoes. Portanto, usar somente caminhoes e
admissivel no proprio problema publicado, nao uma dispensa inventada localmente.

Fontes primarias:
- https://iris.unimore.it/bitstream/11380/1342368/2/chapter.pdf
- https://iris.unimore.it/retrieve/0929c99b-71a5-4f38-ba73-8c5a6d1e2eb0/chapter.pdf
- https://arxiv.org/html/2307.03523v1 (secoes 2.2 e 3.2-3.4)
- https://doi.org/10.1016/j.cor.2023.106514 (versao publicada)

## Verificacao independente realizada

Os 14 certificados foram conferidos no avaliador C recompilado com avisos como
erros, no validador Python e num terceiro recalculo independente usando Decimal.
O terceiro verifica cobertura exata, S rotas, ausencia de missoes, deposito nas
duas extremidades, Manhattan, velocidade do arquivo e minutos. Todas as arestas
usadas sao inteiras em minutos, de modo que ceil nao altera os resultados.
As entradas coincidem byte a byte com a copia original e com o ZIP oficial de 2025.
Rotas, hashes e tempos individuais estao em output/confirmacao_14/verificacao.json;
certificados integrais em output/auditoria_professor/*.json.

| Instancia | S | UB publicado | Referencia fortalecida | Valor exato | Tempos das rotas |
| --- | ---: | ---: | ---: | ---: | --- |
'''
    for r in records:
        report+=f"| {r['instance']} | {r['trucks']} | {r['published_ub']} | {r['monotone_ub']} | {r['exact_minutes']} | {', '.join(r['route_minutes'])} |\n"
    report+='''
Referencia fortalecida = menor UB do catalogo para a mesma instancia com no
maximo S caminhoes. Os UBs de 144-rc-c/S=5 e 187-rc-e/S=4,5 foram reduzidos
analiticamente dessa forma; os 14 certificados continuam estritamente melhores.

## Conclusao precisamente delimitada

Confirmados integralmente: viabilidade local, valores dos certificados,
ausencia de obrigatoriedade de drones no PDSVRP-C classico e desigualdade estrita
contra as referencias do catalogo consultado. Nao e necessario conhecer tau dos
autores para validar estas solucoes, pois nao utilizam drones.

Nao confirmado integralmente: identidade com arquivos de caminhao efetivamente
executados em MIC 2024 e inexistencia de resultados posteriores melhores. A
tabela publicada foi consultada via indice institucional nas pesquisas desta
conversa; acesso direto ao PDF apresentou 403. Nao declarar “novos recordes
mundiais comprovados” ou superioridade estatistica do SA+BB. As solucoes validas
podem ser apresentadas como novos UBs locais inferiores aos UBs MIC consultados,
sob as convencoes publicadas e as instancias oficiais distribuidas.

Nenhum parametro foi alterado para obter a confirmacao; nenhuma nova otimizacao
foi necessaria. Reproduzir: python scripts/confirmar_14.py.
'''
    (ROOT/'docs/auditorias/confirmacao_14_20261117.md').write_text(report,encoding='utf-8')
    print(json.dumps(dict(certificates=14,c=14,python=14,exact_decimal=14,official_files_identical=14,
                         strictly_better_than_strengthened_reference=14)))


if __name__=='__main__':
    main()
