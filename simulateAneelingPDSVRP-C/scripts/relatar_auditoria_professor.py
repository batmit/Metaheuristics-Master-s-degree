"""Empacota evidencia validada das 14 melhorias do experimento oficial."""
import csv
import json
import hashlib
from executar_benchmark import ROOT, catalogo, validar


def main():
    out=ROOT/'output/auditoria_professor'
    out.mkdir(exist_ok=True)
    exp=ROOT/'output/experimento_equacoes'
    rows=[r for r in csv.DictReader((exp/'comparison.csv').open())
          if int(r['trucks'])>1 and float(r['official'])<float(r['literature_ub'])]
    runs=[json.loads(p.read_text()) for p in (exp/'runs').glob('*_official_*.json')]
    cat=catalogo()
    evidence=[]
    for row in rows:
        name,s=row['instance'],int(row['trucks'])
        run=min((r for r in runs if r['instance']==name and r['trucks']==s),key=lambda r:r['best']['makespan'])
        cert=run['best']
        value=validar(cert,name,s)
        if cert['missions']:
            raise ValueError('A conclusao de independencia de drones requer zero missoes')
        # Um caminhao adicional pode ficar ocioso: UB nao deve aumentar com S.
        ub=min(float(r['ub']) for r in cat if r['instance']==name and int(r['trucks'])<=s)
        evidence.append(dict(instance=name,trucks=s,published_ub=float(row['literature_ub']),
            monotone_ub=ub,local=value,gap_pct=100*(value-ub)/ub,drone_missions=0,
            seed=run['seed'],certificate=f'{name}_s{s}.json'))
        (out/f'{name}_s{s}.json').write_text(json.dumps(run,indent=2)+'\n')
    with (out/'melhorias.csv').open('w',newline='') as stream:
        writer=csv.DictWriter(stream,fieldnames=list(evidence[0]));writer.writeheader();writer.writerows(evidence)
    tests=json.loads((ROOT/'output/verificacao.json').read_text(encoding='utf-8'))
    report='''# Auditoria para apresentacao ao professor — 17/11/2026

## Parecer

O programa implementa uma heuristica SA com refinamento BB aplicavel ao PDSVRP-C
com os dados locais. A revisao e os testes nao identificaram violacao de viabilidade
nos casos verificados. Isso nao e prova formal de ausencia de defeitos nem de
otimalidade global. A contribuicao e uma heuristica implementada e validada para
um modelo existente; nao foi demonstrada a novidade de uma formulacao matematica.

## Revisao bibliografica e alcance

Foram usados o PDF local do fundador Nguyen e Ha (2023), as auditorias de leitura
das equacoes, o texto COR 2024 e a tabela MIC 2024 novamente localizada no indice
institucional. O acesso direto ao PDF MIC retornou 403. Buscas por titulo,
PDSVRP-c e collective drones com anos 2025/2026 localizaram tambem tese de Corsini
e trabalhos de variantes robustas/multideposito. Nao se afirma leitura integral
de todos os artigos existentes, nem revisao sistematica exaustiva ou inexistencia
de resultados posteriores. Variantes com outros objetivos, incerteza, varios
depositos ou drones nao coletivos nao fornecem automaticamente BKS comparaveis.

- Fundador: PDSTSP-C, um caminhao, modelo energetico, MILP e RnR.
  https://doi.org/10.1287/trsc.2022.1192
- COR: extensao PDSVRP-C, varios caminhoes, MILP e CP; dados sob solicitacao.
  https://doi.org/10.1016/j.cor.2023.106514
- MIC: heuristica min-max VRP com CP-SAT, tabela de 30 instancias grandes.
  https://doi.org/10.1007/978-3-031-62912-9_13
  https://iris.unimore.it/bitstream/11380/1342368/2/chapter.pdf
- Tese de Corsini: contexto dos modelos CP e benchmark, nao novo conjunto
  automaticamente independente de resultados.
  https://www.iris.unimore.it/retrieve/e3ed862d-71a6-453b-8374-582a09782343/TesiDefinitiva_CorsiniAndrea.pdf

## Conformidade do codigo

| Componente | Evidencia e alcance |
| --- | --- |
| Leitura | IDs consecutivos, parametros finitos, dimensoes; arquivos comparados ao ZIP oficial. |
| Caminhoes | Manhattan, velocidade do arquivo, minutos; rotas fechadas no deposito; sem carga maxima adicional. |
| Drones | Missoes deposito-cliente-deposito; k fixo durante toda a missao; duracao tau; -1 proibido. |
| Cobertura | Cada cliente exatamente uma vez, por caminhao ou missao. |
| Sincronizacao | Recurso cumulativo: soma dos k ativos <= frota; reutilizacao no termino. Para drones homogeneos, permite atribuir unidades fisicas. |
| Objetivo | Maximo entre retorno de todos os caminhoes e termino de todas as missoes. |
| Vizinhos | Troca dentro da rota, transferencia, alteracao de k, 2-opt e reinsercao; deltas conferidos contra recalculo integral. |
| SA | Calibracao, aceita pioras por exp(-delta/T), resfriamento, atual e melhor separados. |
| BB | Refina missoes com clientes/k fixos; teto de 100.000 nos e parada por meta do caminhao; nao prova otimo global. |
| Persistencia | Valida em C e Python, grava melhorias estritas com troca atomica e bloqueio; registros historicos nao sao novas execucoes. |

Os parametros energeticos sao lidos mas a producao consulta tau pronto. Alterar
bateria/peso sem regenerar tau nao atualiza a fisica. O modelo impresso tem
contradicoes conhecidas, explicitadas no relatorio equacoes_completas_20261117.md.
O SA usa uma proposta nao necessariamente simetrica sem correcao de Hastings:
e uma heuristica de otimizacao, sem alegacao de amostragem exata de Boltzmann
ou garantia assintotica derivada apenas da formula de aceitacao.

Limites de robustez: o leitor C aceita duracao zero, enquanto o validador Python
rejeita missoes de duracao zero; os arquivos oficiais possuem apenas positivos
ou -1. A validacao C confere matriz mas nao recalcula energia. Limites de massa
e tamanho de formacao dependem da autenticidade de tau. A busca nao implementa
warm start a partir do registro. Essas limitacoes nao invalidaram certificados
oficiais verificados, mas impedem garantir comportamento fisico para entradas
arbitrariamente modificadas. Nao foi refeita nesta rodada a instrumentacao ASan/UBSan.

## Verificacao executada nesta rodada

'''
    report+='```json\n'+json.dumps(tests,indent=2,ensure_ascii=False)+'\n```\n\n'
    report+='''Tambem foram revalidados os 14 certificados abaixo usando a implementacao
Python independente. A auditoria anterior validou 2.160 execucoes, 480 transferencias
de certificados entre matrizes e os 60 certificados originais de um caminhao.
Resultados de duas rodadas nao devem ser confundidos: a suite atual testa o
codigo atual; o experimento de 2 segundos tem seus proprios hashes e protocolo.

## Achado principal: as 14 solucoes usam somente caminhoes

Este achado corrige a justificativa excessivamente ampla dada anteriormente:
a matriz de drones desconhecida de MIC nao impede validar ESTAS 14 solucoes,
pois nenhuma delas utiliza drones. Elas sao solucoes min-max VRP admissiveis
tambem no PDSVRP-C, que permite deixar drones ociosos. Resta conferir identidade
de coordenadas, deposito, custos de caminhao, numero de caminhoes e atualidade
do BKS antes de anunciar recordes universais. Os certificados originais e as
convencoes declaradas sustentam a comparacao, mas nao substituem arquivos MIC.

O UB foi fortalecido por monotonicidade: mais caminhoes nao podem piorar a
melhor solucao, pois podem ficar ociosos. Assim, 144-rc-c/S=5 usa referencia
72 (S=4), em vez de 92; 187-rc-e/S=4 e S=5 usam 124 (S=3), em vez de 148/128.
As 14 melhorias persistem mesmo contra essas referencias mais fortes.
Essa normalizacao e analitica e nao altera o catalogo historico publicado.

| Instancia | S | UB publicado da linha | UB monotono | Local | Gap vs UB monotono % |
| --- | ---: | ---: | ---: | ---: | ---: |
'''
    for r in evidence:
        report+=f"| {r['instance']} | {r['trucks']} | {r['published_ub']:g} | {r['monotone_ub']:g} | {r['local']:g} | {r['gap_pct']:.3f} |\n"
    report+='''
Nao atribuir essas melhorias especificamente ao SA ou ao BB sem ablacao:
a construcao inicial e o 2-opt tambem participam. Os testes anteriores tiveram
apenas 2 segundos por execucao; muitos foram interrompidos. Nao demonstram
superioridade estatistica, desempenho final do SA nem vantagem de tempo frente
aos solvers dos artigos. As variantes Euclidiana e de equacoes fisicas devem
ser apresentadas como sensibilidades, nunca misturadas aos 14 casos oficiais.

## Formulacao recomendada para a apresentacao

“Desenvolvemos uma heuristica SA+BB para o PDSVRP-C e verificamos sua viabilidade
por validadores independentes e testes sistematicos. No benchmark local oficial,
obtivemos certificados com valores inferiores aos UBs MIC 2024 consultados em
14 configuracoes, inclusive apos fortalecer as referencias por monotonicidade.
Esses certificados usam somente caminhoes e independem dos tempos de drones.
Sua caracterizacao como novos melhores resultados atuais requer confirmar
a correspondencia dos dados de caminhões e revisar resultados posteriores.”

## Resumo breve do algoritmo

Constroi rotas por insercao gulosa e 2-opt, explora cinco tipos de vizinhanca
com Simulated Annealing e escalona drones coletivos como recurso cumulativo.
Um BB limitado refina a ordem das missoes. Minimiza o ultimo retorno ao deposito
e conserva apenas melhorias validadas. E uma solucao heuristica valida para o
modelo local, com evidencia de valores menores que referencias publicadas,
sem prova de otimalidade global ou superioridade geral do metodo.

Reproducao: python scripts/verificar_projeto.py; python scripts/relatar_auditoria_professor.py.
Certificados, sementes e CSV: output/auditoria_professor/.
'''
    (ROOT/'docs/auditorias/parecer_professor_20261117.md').write_text(report,encoding='utf-8')
    (out/'relatorio.md').write_text(report,encoding='utf-8')
    print(json.dumps(dict(certificates=len(evidence),all_truck_only=True,
        still_better_after_monotonicity=sum(r['local']<r['monotone_ub'] for r in evidence))))


if __name__=='__main__':
    main()
