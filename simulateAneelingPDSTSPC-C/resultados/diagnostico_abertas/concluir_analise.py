"""Valida os ensaios e escreve a analise, sem alterar codigo de producao."""
from pathlib import Path
from fractions import Fraction as F
import csv
import hashlib
import json
import math

HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[1]
measurements=json.loads((HERE/'medicoes.json').read_text())
features=json.loads((HERE/'caracteristicas_60.json').read_text())
refs={r['instance']:r for r in csv.DictReader((ROOT/'resultados/revisao_literatura_2025/comparacao_60_instancias.csv').open(encoding='utf-8-sig'))}


def parse_solution(line):
    h,t=line.split(' ROUTE ');a,b=t.split(' MISSIONS');v=h.split();kind,name=v[:2]
    if kind=='POLISH':claim,truck,drone=map(F,v[4:7])
    elif kind=='CROSS':claim,truck,drone=map(F,v[3:6])
    else:claim,truck,drone=map(F,v[2:5])
    return {'type':kind,'instance':name,'claimed':claim,'truck':truck,'drone':drone,
            'route':list(map(int,a.split())),
            'missions':[dict(zip(['client','k','start','end'],[int(v[0]),int(v[1]),F(v[2]),F(v[3])]))
                        for token in b.split() if (v:=token.split(':'))]}


def validate(s):
    name=s['instance'];folder='small' if int(name.split('-')[0])<=30 else 'large'
    raw=(ROOT/f'instances/pdstsp-c/{folder}/{name}.txt').read_text().splitlines()
    fleet=int(raw[0].split(',')[1]);speed=F(raw[1].split(',')[1].strip())
    xy={int(v[0]):(F(v[1]),F(v[2])) for line in raw[7:] if (v:=line.split())}
    tau={int(v[0]):[F(t) for t in v[1:]] for line in (ROOT/f'instances/pdstsp-c/{folder}/tau_kj_{name}.txt').read_text().splitlines()[1:] if (v:=line.split())}
    selected=s['route']+[m['client'] for m in s['missions']]
    assert len(selected)==len(xy)-1 and len(set(selected))==len(selected)
    assert set(selected)==set(xy)-{0}
    truck=sum(sum(abs(a-b) for a,b in zip(xy[i],xy[j]))*60/speed for i,j in zip([0]+s['route'],s['route']+[0]))
    for m in s['missions']:
        assert 1<=m['k']<=fleet and m['start']>=0
        duration=tau[m['client']][m['k']-1];assert duration>=0
        assert m['end']-m['start']==duration
    for t in {m['start'] for m in s['missions']}:
        assert sum(m['k'] for m in s['missions'] if m['start']<=t<m['end'])<=fleet
    drone=max(m['end'] for m in s['missions'])
    assert (truck,drone,max(truck,drone))==(s['truck'],s['drone'],s['claimed'])


all_solutions=[]
for file in ['execucoes_instrumentadas.txt','polimento.txt','trocas.txt']:
    for line in (HERE/file).read_text().splitlines():
        if ' ROUTE ' in line:
            s=parse_solution(line);validate(s);all_solutions.append(s)
assert len(all_solutions)==34

# A instrumentacao nao altera o caminho conhecido da 105 e da 20.
audit=(ROOT/'resultados/auditoria_aleatoriedade.txt').read_text(encoding='utf-8-sig').splitlines()
for name,seed in [('20-c-c',123),('105-rc-e',1792860329)]:
    original=next(x for x in audit if x.startswith(f'FINAL {name} {seed} ')).split(' ROUTE ')[1]
    replay=next(x for x in (HERE/'execucoes_instrumentadas.txt').read_text().splitlines() if x.startswith(f'SA_FINAL {name} ')).split(' ROUTE ')[1]
    assert original==replay
for name,result in [('67-c-c',76),('165-r-c',172),('167-r-e',196)]:
    assert measurements[name]['final']['makespan']==result
old=json.loads((ROOT/'resultados/105-rc-e/hashes_codigo.json').read_text())
for name in ['SA.c','cv.c','SA.h','cv.h','lerInstancias.c']:
    assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest()==old[name]

post={}
for s in all_solutions:
    if s['type'] in ['MINAREA','POLISH','CROSS']:
        post.setdefault(s['instance'],{})[s['type']]={k:int(s[k]) for k in ['claimed','truck','drone']}
summary={'date':'2026-10-26','baselines':measurements,'postprocessing':post,
         'independent_certificates_validated':34,
         'same_full_trajectory_certificates':['20-c-c seed 123','105-rc-e seed 1792860329'],
         'production_algorithm_unchanged':True,
         'sample_scope':'Uma trajetoria escolhida por instancia. Tres sementes reproduzem melhores conhecidos de lotes de 100; 181 usa semente 123, nao best-of-100. Sem estudo de ablacao multi-semente.'}
(HERE/'resumo_diagnostico.json').write_text(json.dumps(summary,indent=2),encoding='utf-8')

def table(headers,rows):
    return ['|'+'|'.join(headers)+'|','|'+'|'.join(['---']*len(headers))+'|']+['|'+'|'.join(map(str,r))+'|' for r in rows]

names=['67-c-c','165-r-c','167-r-e','181-r-e']
md=['# Por que o SA perde qualidade nos casos ainda abertos','',
    'Data: **26/10/2026**. Diagnóstico do código atual, dos dados e de ensaios separados. Nenhum algoritmo de produção foi alterado.','',
    '## Conclusão','',
    'A perda de qualidade não decorre da ausência de prova de ótimo nem da perda de aleatoriedade. '
    'Os mecanismos medidos apontam principalmente para atribuições caminhão/drones presas em regiões locais, '
    'falta de valorização da folga em soluções empatadas, pouca diversidade de atribuição nas elites e um orçamento '
    'que praticamente não cresce com o tamanho da instância. O B&B apenas refina quantidades sob ordem fixa, '
    'portanto não corrige sozinho uma atribuição inadequada.','',
    'Há evidência direta de uma limitação: nas soluções reproduzidas de 165-r-c e 167-r-e, a carga mínima '
    'dos clientes selecionados para drones já torna impossível alcançar as referências 160 e 180 sem mudar a atribuição.','',
    '## 1. Comparação correta com as referências de 2025','']
hist={r['instancia']:r for r in csv.DictReader((ROOT/'resultados/resumo_sem_otimo_100_execucoes.csv').open(encoding='utf-8-sig'))}
rows=[]
for n in names:
    if n in hist: value=float(hist[n]['melhor']);scope='Melhor de 100, lote histórico'
    else:value=measurements[n]['final']['makespan'];scope='Um ensaio, semente 123'
    ub=int(refs[n]['ub_2025'])
    rows.append([n,ub,int(value),f'{100*(value-ub)/ub:.2f}%',scope])
md+=table(['Instância','UB atual','SA','Gap frente ao UB','Base da comparação'],rows)
md+=['','O UB de um caso aberto não é um ótimo provado. O gap acima é relativo à melhor solução publicada, '
     'não a um ótimo conhecido. Não misturar o ensaio único da 181-r-e com melhores de 100 execuções. '
     'O lote novo de quatro casos estava incompleto quando este diagnóstico foi produzido; não foi interrompido.','',
     'A percepção de bom desempenho anterior também precisa da referência atualizada: a 154-c-c alcança 72 em '
     '92 das 100 execuções históricas, mas o ótimo atual é 68. A 126-r-c, já fechada, tem melhor SA 168 frente '
     'ao ótimo 152 (+10,53%); a 103-rc-c tem 132 frente a 120 (+10%). Portanto, casos já provados também são '
     'difíceis para a heurística. Os mesmos quatro casos abertos consumiram aproximadamente 3600 s nos logs '
     'oficiais sem fechar a prova, mas dificuldade de prova e qualidade da heurística são questões distintas.','',
     '## 2. Características dos dados','']
md+=table(['Instância','Clientes','Drones','Elegíveis','Obrigatórios no caminhão','Elegíveis que exigem grupo k>1','Elegíveis por drone'],
          [[n,features[n]['n'],features[n]['m'],features[n]['eligible'],features[n]['mandatory_truck'],features[n]['need_collective'],f"{features[n]['eligible_per_drone']:.2f}"] for n in names])
md+=['','Na 20-c-c, usada como controle fácil, são 19 elegíveis para seis drones (3,17 por drone). '
     'Nas três maiores abertas, esse indicador vai de 14,36 a 17,78, com cerca de cinco quantidades viáveis por cliente. '
     'A quantidade de decisões de atribuição, ordem e formação cresce muito mais que o número de drones. '
     'Esse indicador descreve as alternativas da busca; não significa que todos os elegíveis devam ir para drones.','',
     'A 67-c-c tem uma dificuldade diferente: 67 clientes em apenas 15 coordenadas distintas; 59 clientes compartilham '
     'coordenadas com outros, e há um local com 14 clientes. Na solução reproduzida, retirar isoladamente qualquer '
     'um dos 33 clientes elegíveis ainda no caminhão economiza zero no percurso atual. Para eliminar uma visita '
     'pode ser necessário transferir um grupo completo, não apenas um cliente.','',
     '## 3. O modelo consegue representar as soluções publicadas','',
     'As testemunhas oficiais foram importadas com as quantidades publicadas e prioridades em ordem de início. '
     'O escalonador C atual reproduziu os makespans 74, 160, 180 e 216 nos quatro casos. Também reproduziu 43 '
     'na 20-c-c, 112 na 105-rc-e e 68 na 154-c-c. Isso afasta, nestes ensaios, uma incompatibilidade de modelo '
     'ou da regra first-fit que impedisse representar as referências. Não prova ausência de qualquer defeito possível.','',
     '## 4. O B&B não consegue consertar uma atribuição errada','']
md+=table(['Instância','Clientes nos drones: SA / fonte','Carga mínima dos clientes do SA','LB dos drones, quaisquer quantidades','Referência'],
          [[n,f"{len(measurements[n]['final']['missions'])} / {features[n]['ref_drone_count']}",
            int(measurements[n]['conditional']['min_area']),measurements[n]['conditional']['lower_bound_drones'],features[n]['ub']] for n in names])
md+=['','O limite usado é ceil(sum_j min_k(k × tau_j(k)) / m), condicionado ao conjunto atual de clientes dos drones. '
     'Ele vale independentemente da ordem ou do escalonamento escolhido. Na 165-r-c, 1624/10 exige pelo menos 163; '
     'na 167-r-e, 1998/11 exige pelo menos 182. Logo, mesmo um escalonamento exato de todas as quantidades e ordens '
     'não alcança 160 e 180 sem mudar os clientes atribuídos. A contagem de missões, isoladamente, não basta: '
     'é a carga mínima que fundamenta essa impossibilidade.','',
     'A 181-r-e não tem essa impossibilidade pelo limite de carga (LB condicional 205 < 216), mas a rota atual '
     'exige 252. Na 67-c-c, a rota exige 76. Nos quatro estados examinados, o caminhão fixa o makespan ou '
     'está empatado com os drones; reduzir somente tempoDrones não reduz o objetivo naquele instante.','',
     '## 5. Soluções empatadas com folga útil são subvalorizadas','',
     'O objetivo é max(tempoCaminhao, tempoDrones). O SA aceita empates, mas lhes dá zero ponto no sistema de pesos. '
     'O arquivo cheio exige makespan estritamente menor que o da pior solução; logo, uma configuração com o mesmo '
     'makespan e muito mais folga pode não entrar. O refinamento da elite também copia o B&B de volta somente '
     'quando reduz o makespan. São escolhas de intensificação, não violações de viabilidade.','',
     'Um reparo barato escolheu, para cada missão, a quantidade viável que minimiza k × tau_j(k), mantendo '
     'clientes e ordem, e reconstruiu o cronograma:','']
md+=table(['Instância','Caminhão','Drones antes','Drones após reparo mínimo de carga','Makespan antes / depois'],
          [[n,int(measurements[n]['final']['truck']),int(measurements[n]['final']['drone']),post[n]['MINAREA']['drone'],
            f"{int(measurements[n]['final']['makespan'])} / {post[n]['MINAREA']['claimed']}"] for n in names])
md+=['','Na 181-r-e, os drones caem de 252 para 217, mas a solução continua valendo 252 porque o caminhão '
     'não mudou. Essa folga poderia receber novos clientes e permitir reconstruir uma rota menor. O critério '
     'atual não garante que ela seja preservada ou recompensada. Na 67-c-c, o mesmo reparo libera 13 minutos '
     'nos drones; esse espaço pode ajudar a transferir um grupo que retire uma visita inteira do caminhão. '
     'Isso demonstra folga disponível, não garante que a sequência seguinte encontre uma melhoria global.','',
     '## 6. Pouca diversidade de atribuição dentro da elite','',
     'Nas sete trajetórias instrumentadas, os quatro slots finais tinham apenas uma atribuição distinta '
     'caminhão/drones. Os vetores eram diferentes por ordem e/ou quantidades, mas compartilhavam o mesmo conjunto '
     'de clientes por recurso. Isso limita as regiões que o B&B recebe. A observação inclui o controle fácil: '
     'portanto é uma propriedade do mecanismo, não uma causa suficiente da dificuldade dos quatro abertos.','',
     '## 7. Mais nós de B&B não foram a primeira solução','']
md+=table(['Instância','Gatilhos de B&B no SA','Buscas efetivas','Ganhos globais via B&B','Makespan após B&B de 100 mil nós'],
          [[n,measurements[n]['trace']['bb_triggers'],measurements[n]['trace']['bb_executions'],measurements[n]['trace']['bb_global_gains'],post[n]['POLISH']['claimed']] for n in names])
md+=['','Os filtros escolhidos (drones dominantes, elite elegível e histórico) fazem muitos gatilhos não gerarem '
     'nova busca. Na 167-r-e, as oito buscas efetivas bateram o limite de 10 mil nós. Ainda assim, subir para '
     '100 mil nós em cada estado final não melhorou nenhum makespan, pois o caminhão continuou sendo barreira. '
     'Em vários casos, o reparo mínimo de carga foi melhor para tempoDrones do que o resultado limitado do B&B. '
     'Isso recomenda acrescentar tal reparo como incumbente inicial barato, em vez de apenas ampliar o orçamento.','',
     'O B&B continua restrito às quantidades e à regra de inserção sob ordem fixa. Seu status de ótimo não '
     'equivale ao ótimo de toda a instância. As regras de elegibilidade do B&B foram mantidas nestes ensaios.','',
     '## 8. Orçamento e resfriamento quase iguais para tamanhos muito diferentes','']
md+=table(['Instância','Propostas do SA','Última melhora global na fração da busca','Empates úteis sem recompensa'],
          [[n,measurements[n]['trace']['steps'],f"{100*measurements[n]['trace']['last_improvement']/measurements[n]['trace']['steps']:.1f}%",sum(measurements[n]['trace']['secondary'])] for n in ['20-c-c']+names])
md+=['','A 20-c-c recebeu 538500 propostas; as maiores receberam aproximadamente 548000 a 562000. '
     'O orçamento varia muito pouco, enquanto o espaço de decisões cresce fortemente. Nas trajetórias de '
     '165-r-c, 167-r-e e 181-r-e, a última melhora ocorreu perto do final. Isso sugere que um orçamento adaptativo '
     'ou ciclos de perturbação e reaquecimento merecem teste, sem concluir que bastará executar por mais tempo.','',
     'Ao final das sete trajetórias, todos os pesos estavam no piso 0,1. Portanto a seleção dos tipos disponíveis '
     'voltava a ser uniforme; não havia um operador desabilitado nem desaparecimento da aleatoriedade. '
     'Por outro lado, a adaptação deixou de distinguir os tipos nessa fase. Foram observados dezenas de milhares '
     'de empates com redução de tempoCaminhao+tempoDrones que recebem zero ponto pelo critério atual.','',
     '## 9. Testes que não deram melhora imediata','',
     'Uma descida exaustiva com 2-opt e reinserção individual, mantendo os clientes no caminhão, não reduziu '
     'nenhuma das sete rotas finais. Uma troca de um cliente entre modais, com reinserção na melhor posição '
     'do caminhão e teste de todas as quantidades viáveis da missão substituta, também não melhorou os estados '
     'testados sob a ordem restante fixa. Isso é evidência de barreiras que exigem coordenação maior; não prova '
     'ótimo local em toda vizinhança possível, nem torna inúteis essas operações em outras etapas da busca.','',
     '## 10. Melhorias propostas em ordem prática','',
     '1. **Guardar e premiar folga útil nos empates.** Manter makespan como objetivo primário; desempatar as '
     'elites por folga/tempo secundário e diversidade, preservando a melhor. Evitar premiar ciclos de estados repetidos.',
     '2. **Usar reparo por menor carga como mais um incumbente do B&B.** Comparar o recebido, o guloso de menor '
     'término já existente e o de menor k × tau. É uma heurística de construção de limite superior, não uma '
     'regra para eliminar quantidades da busca. Manter possibilidade de explorar todas as quantidades viáveis.',
     '3. **Mover grupos e reconstruir conjuntamente.** Na 67-c-c, considerar clientes da mesma coordenada. '
     'Nas maiores, retirar/reinserir pequenos grupos próximos, rever a atribuição e reotimizar a rota. '
     'Uma troca simples de um cliente isolado não foi suficiente no ensaio feito.',
     '4. **Diversificar atribuições da elite e estados iniciais.** Distinguir diversidade de rota de diversidade '
     'dos conjuntos atribuídos. Sortear entre construções e elites estruturalmente diferentes.',
     '5. **Ajustar esforço por tamanho/estagnação e reaquecer após perturbações.** Comparar por mesmo tempo '
     'ou número de avaliações, com múltiplas sementes e registro de melhor, média e tempo até a referência.',
     '6. **Não priorizar apenas mais B&B ou uma nova semente.** Ambos preservam o conjunto errado de clientes '
     'se não houver diversificação. A aleatoriedade atual existe; a vizinhança e os critérios determinam quais '
     'regiões e folgas são efetivamente aproveitadas.','',
     '## Alcance e validação','',
     'Esta é uma análise de mecanismos com sete trajetórias escolhidas, não um estudo estatístico de ablação. '
     'As sementes de 67-c-c, 165-r-c e 167-r-e reproduzem melhores históricos de 100; a 181-r-e usa só a '
     'semente 123. Os controles são 20-c-c, 105-rc-e e 154-c-c. Os experimentos extras não foram incorporados ao lote.',
     'As cópias instrumentadas não acrescentam sorteios. Os certificados completos de 20-c-c e 105-rc-e '
     'reproduziram exatamente os arquivos anteriores. Os hashes de SA.c, cv.c, SA.h, cv.h e lerInstancias.c '
     'permaneceram iguais aos da auditoria anterior. As 34 soluções de ensaios foram conferidas '
     'independentemente com frações exatas: cobertura, percurso, matriz de tempos, duração e uso simultâneo da frota.',
     'Para afirmar o ganho de cada proposta, o próximo passo é comparar versões em várias sementes com '
     'orçamento igual, alterando um componente por vez. Este diagnóstico não garante encontrar valores '
     'abaixo dos UBs publicados; eles podem já ser ótimos mesmo sem prova disponível.','',
     '## Arquivos de apoio','',
     '- medicoes.json e resumo_diagnostico.json: todas as métricas e limites condicionais.',
     '- execucoes_instrumentadas.txt: trajetórias de referência e do SA, com contadores.',
     '- polimento.txt: reparo mínimo de carga e B&B de 100 mil nós.',
     '- trocas.txt: ensaio de troca entre modais.',
     '- caracteristicas_60.json: estatísticas estruturais de todo o conjunto.',
     '- A referência oficial está na revisão anterior: resultados/revisao_literatura_2025/.','']
(HERE/'analise_causas.md').write_text('\n'.join(md),encoding='utf-8')
print('34 certificados independentes aprovados; producao intacta; relatorio analise_causas.md gerado.')
