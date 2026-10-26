"""Gera relatorio PDF e Markdown a partir da auditoria independente."""
import csv
from collections import Counter
import hashlib
import json
from math import ceil
from pathlib import Path
import shutil
import re

from reportlab.pdfgen import canvas
from reportlab.lib import colors
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.platypus import Paragraph, Table, TableStyle
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
OUTPUT=ROOT/'output/pdf'; OUTPUT.mkdir(parents=True,exist_ok=True)
D=json.loads((HERE/'validacao.json').read_text(encoding='utf-8'))
S=json.loads((HERE/'solucao_116.json').read_text(encoding='utf-8'))
OFF=json.loads((HERE/'solucao_orlab_112.json').read_text(encoding='utf-8'))
O=D['ours']; ST=D['statistics']; P=Path('C:/Windows/Fonts')
pdfmetrics.registerFont(TTFont('Arial',str(P/'arial.ttf')))
pdfmetrics.registerFont(TTFont('ArialBold',str(P/'arialbd.ttf')))
pdfmetrics.registerFontFamily('Arial',normal='Arial',bold='ArialBold',italic='Arial',boldItalic='ArialBold')
INK=colors.HexColor('#12283e'); BLUE=colors.HexColor('#226a9c')
GREEN=colors.HexColor('#16756c'); LIGHT=colors.HexColor('#edf3f7')
GRAY=colors.HexColor('#536576'); ORANGE=colors.HexColor('#b66b20')
W,H=A4; LEFT=43; RIGHT=W-43; WIDTH=RIGHT-LEFT
C=canvas.Canvas(str(OUTPUT/'relatorio_105_rc_e.pdf'),pagesize=A4)
C.setTitle('Auditoria da instancia 105-rc-e: solucao 116 e referencia atualizada 112')
C.setAuthor('Auditoria computacional do projeto PDSTSP-C')
page_no=0
Y=0
sections=[]

ACCENTS=dict(zip(
    'solucao solucoes referencia referencias instancia instancias relatorio tecnico tecnica verificacao verificacoes calculos calculo reproducao viavel viaveis otimo otimalidade criterio criterios sincronizacao duracao duracoes ocupacao atribuicao atribuicoes apos ate simultanea simultaneo maxima minimo minima maximo pre-calculado pre-calculados nao estao esta estatisticas estatistica media padrao hipotese grafo evidencias horario horarios deposito ultimo ultima numero numeros identica identicos codigo parametros atualizacao conclusao tres explicita explicito aerodinamico energetica fisico diario'.split(),
    'solução soluções referência referências instância instâncias relatório técnico técnica verificação verificações cálculos cálculo reprodução viável viáveis ótimo otimalidade critério critérios sincronização duração durações ocupação atribuição atribuições após até simultânea simultâneo máxima mínimo mínima máximo pré-calculado pré-calculados não estão está estatísticas estatística média padrão hipótese grafo evidências horário horários depósito último última número números idêntica idênticos código parâmetros atualização conclusão três explícita explícito aerodinâmico energética físico diário'.split()))
ACCENTS.pop('esta')
ACCENTS.update({'sao':'são','ja':'já','tambem':'também','metrica':'métrica','aritmetica':'aritmética',
                'capitulo':'capítulo','periodo':'período','posicoes':'posições',
                'caminhao':'caminhão','caminhoes':'caminhões','missao':'missão','missoes':'missões',
                'sequencia':'sequência','sequencias':'sequências','historica':'histórica','historico':'histórico',
                'revisao':'revisão','atribuida':'atribuída','atribuidas':'atribuídas','alocacao':'alocação',
                'restricao':'restrição','restricoes':'restrições','numerica':'numérica','validacao':'validação',
                'codificacao':'codificação','informacao':'informação','comparacao':'comparação',
                'comparaveis':'comparáveis','rastreabilidade':'rastreabilidade','trabalhadores':'trabalhadores',
                'simultaneamente':'simultaneamente','periodico':'periódico','proprias':'próprias',
                'contagens':'contagens','precedencia':'precedência','cronologica':'cronológica',
                'recompilacao':'recompilação','compilacao':'compilação','aerodinamico':'aerodinâmico',
                'indicios':'indícios','estatistico':'estatístico','variacao':'variação',
                'sincronizacao':'sincronização','vinculos':'vínculos','distribuicao':'distribuição'})

def pt(s):
    s=str(s)
    for a,b in [('A solucao e viavel.','A solução é viável.'),('a solucao de 116 minutos esta correta','a solução de 116 minutos está correta'),
                (' e viavel',' é viável'),(' e o numero',' é o número'),(' e uma',' é uma'),
                ('esta correta','está correta'),('esta desatualizada','está desatualizada'),
                ('esta confirmado','está confirmado'),
                (' e o makespan',' é o makespan'),(' e 116',' é 116'),(' e 12',' é 12'),(' e 112',' é 112'),(' e 30',' é 30')]:
        s=s.replace(a,b)
    pattern=r'(<[^>]+>|[\w/.-]+\.(?:json|txt|py|c|h|csv|pdf|log|zip|exe))'
    parts=re.split(pattern,s)
    def accent(m):
        word=m.group(); repl=ACCENTS.get(word.lower(),word)
        return repl[0].upper()+repl[1:] if word[0].isupper() else repl
    return ''.join(part if i%2 else re.sub(r'\b[\w-]+\b',accent,part) for i,part in enumerate(parts))


def br(v,d=2):
    if isinstance(v,int) or (isinstance(v,float) and v.is_integer()): return str(int(v))
    return f'{v:.{d}f}'.replace('.',',')


def page(title,sub=''):
    global page_no,Y
    if page_no: C.showPage()
    page_no+=1
    C.setFillColor(INK); C.rect(0,H-15,W,15,fill=1,stroke=0)
    C.setFont('ArialBold',8); C.drawString(LEFT,H-38,'PDSTSP-C  /  AUDITORIA COMPUTACIONAL')
    C.setFont('Arial',8); C.drawRightString(RIGHT,H-38,'26 OUT 2026')
    C.setFont('ArialBold',22); C.drawString(LEFT,H-75,pt(title))
    if sub:
        C.setFillColor(GRAY); C.setFont('Arial',9); C.drawString(LEFT,H-94,pt(sub))
    C.setStrokeColor(colors.HexColor('#cbd6df')); C.line(LEFT,43,RIGHT,43)
    C.setFont('Arial',8); C.setFillColor(GRAY)
    C.drawString(LEFT,29,'105-rc-e | dados, calculos exatos e testemunhas de viabilidade')
    C.drawRightString(RIGHT,29,str(page_no))
    Y=H-116


def text(s,size=10,after=9,color=INK):
    global Y
    p=Paragraph(pt(s),ParagraphStyle('body',fontName='Arial',fontSize=size,leading=size*1.4,textColor=color))
    _,h=p.wrap(WIDTH,800)
    assert Y-h>=54,(page_no,s[:50],Y,h)
    p.drawOn(C,LEFT,Y-h); Y-=h+after


def heading(s):
    text(s,12,8,BLUE)


def table(headers,rows,widths=None,size=8,row_height=None):
    global Y
    sty=ParagraphStyle('cell',fontName='Arial',fontSize=size,leading=size*1.24,textColor=INK)
    header=ParagraphStyle('head',parent=sty,fontName='ArialBold',textColor=colors.white)
    vals=[[Paragraph(pt(x),header) for x in headers]]
    vals += [[Paragraph(pt(x),sty) for x in row] for row in rows]
    t=Table(vals,colWidths=widths or [WIDTH/len(headers)]*len(headers),
            rowHeights=row_height,repeatRows=1,hAlign='LEFT')
    t.setStyle(TableStyle([('BACKGROUND',(0,0),(-1,0),INK),
                          ('ROWBACKGROUNDS',(0,1),(-1,-1),[colors.white,LIGHT]),
                          ('VALIGN',(0,0),(-1,-1),'TOP'),('LEFTPADDING',(0,0),(-1,-1),6),
                          ('RIGHTPADDING',(0,0),(-1,-1),6),('TOPPADDING',(0,0),(-1,-1),2 if row_height else 4),
                          ('BOTTOMPADDING',(0,0),(-1,-1),2 if row_height else 4),
                          ('LINEBELOW',(0,0),(-1,0),0.5,INK)]))
    _,h=t.wrap(WIDTH,1000)
    assert Y-h>=54,(page_no,'table',Y,h)
    t.drawOn(C,LEFT,Y-h);Y-=h+13


def section(s,paragraphs):
    sections.append((s,paragraphs))


def mission_color(j):
    palette=['#246d9a','#1d8473','#8b649a','#bb7625','#ac546a','#557637','#45649f','#6e6760']
    return colors.HexColor(palette[j%len(palette)])


page('A solucao e viavel. A referencia mudou.', 'Relatorio tecnico | verificacao da solucao de 116 minutos e dos resultados publicados')
text('<b>Conclusao:</b> a solucao de 116 minutos esta correta para os dados e as restricoes do benchmark. '
     'Ela melhora os 120 minutos usados no projeto, mas o pacote oficial ORLab de 2025 ja reporta '
     '<b>112 minutos como otimo</b>. A solucao publicada de 112 tambem passou pela verificacao independente.')
table(['Indicador','Resultado confirmado'],[
    ['Instancia','105 clientes + deposito; 1 caminhao; 12 drones'],
    ['Solucao do projeto','Caminhao 116 min; drones 115 min; makespan 116 min'],
    ['Atendimento','72 clientes no caminhao e 33 em missoes de drones'],
    ['Referencia de 2024','LB=109; UB=120; melhoria historica de 4 min (3,33%)'],
    ['Pacote oficial de 2025','Makespan=112; Makespan Bound=112; status OPTIMAL'],
    ['Comparacao atualizada','116 - 112 = 4 min; gap relativo ao otimo reportado: 3,57%'],
    ['Reproducao','Semente 1792860329; mesma rota, quantidades e horarios'],
], [153,WIDTH-153],9)
heading('Tres afirmacoes diferentes')
text('<b>Viabilidade de 116:</b> confirmada por calculos independentes em Python, usando aritmetica racional exata, '
     'e pela reproducao do programa C. <b>Viabilidade de 112:</b> confirmada a partir das sequencias publicadas '
     'para os drones e da rota do caminhao. <b>Otimalidade de 112:</b> reportada pelo CP-SAT no log oficial; '
     'a prova de busca do solver nao foi reexecutada nesta auditoria.')
text('A classificacao desta instancia como "sem otimo comprovado" e a coluna de 120 minutos refletem a tabela de 2024. '
     'Elas estao desatualizadas em relacao ao pacote consultado nesta revisao.',10,color=ORANGE)
section('Conclusao executiva',[
    'A solucao de 116 minutos e viavel. O resultado melhora o UB de 120 do artigo de 2024 em 3,33%.',
    'O pacote oficial ORLab de 2025 registra OPTIMAL, Makespan=112 e Makespan Bound=112; a testemunha de 112 foi validada.',
    '116 fica 3,57% acima de 112. Esta auditoria nao reexecutou a prova de otimalidade do solver.'])

page('Dados e rastreabilidade','O par de arquivos usado na busca foi confrontado com os pacotes oficiais')
text('Foram baixados novamente os pacotes originais e o pacote disponibilizado pelo ORLab em 2025. '
     'Os hashes dos pacotes coincidem com os registrados no projeto. O arquivo de coordenadas coincide '
     'com o original; o par de coordenadas e matriz de tempos coincide com o pacote de 2025.')
table(['Parametro','Valor'],[['Clientes / nos totais','105 / 106'],['Drones / caminhoes','12 / 1'],
    ['Velocidade do caminhao','30 km/h'],['Bateria por drone','peso 1,55; capacidade 12; tensao 12,8; corrente maxima 24'],
    ['Tempos do caminhao','distancia Manhattan / 30 x 60, em minutos'],
    ['Tempos dos drones','tau_j(k) pre-calculado; -1 representa quantidade inviavel']], [180,WIDTH-180],9)
heading('Identificadores SHA-256 do par de entrada')
for name in ['105-rc-e.txt','tau_kj_105-rc-e.txt']:
    text('<b>'+name+'</b>',9,4)
    h=D['source_hashes'][name];text(h[:32]+'<br/>'+h[32:],8,12)
heading('Fontes preservadas no pacote da auditoria')
text('O diretorio <b>fontes/</b> contem o par de entrada, a linha completa da solucao de 116, '
     'o extrato do BKS original de 120 e o log de 2025 com a solucao de 112. '
     'O arquivo validacao.json registra os hashes e os resultados de cada verificacao.')
text('A viabilidade energetica e a duracao de cada configuracao sao herdadas da matriz oficial. '
     'Nao foi recalculado o modelo aerodinamico/bateria. Isso preserva o mesmo benchmark: '
     'a auditoria confirma que a busca utiliza exclusivamente celulas viaveis da matriz publicada.')
section('Modelo e dados', ['Um caminhao faz um circuito fechado no deposito; drones identicos realizam missoes deposito-cliente-deposito.',
    'Uma missao coletiva ocupa k drones simultaneamente durante toda a sua duracao. Quantidades inviaveis da matriz sao proibidas.',
    'Nao ha recuperacao de drones pelo caminhao, precedencia entre o vetor completo de missoes, nem recarga acumulativa entre missoes no benchmark.',
    'A validacao usa os tempos completos oficiais, sem dividir tau por dois ou somar tempos extras sem respaldo nos dados.'])

page('Metodo de verificacao independente','Os calculos de viabilidade nao chamam o avaliador nem o escalonador do projeto')
table(['Verificacao','Evidencia / resultado'],[
    ['Cobertura','Clientes 1..105 presentes exatamente uma vez; nenhum deposito tratado como cliente'],
    ['Circuito do caminhao','Deposito inicial/final implicitos; 73 arcos recalculados a partir das coordenadas'],
    ['Tempos e unidades','Manhattan e 30 km/h; fracao exata em cada arco; soma igual a 116'],
    ['Elegibilidade / quantidade','Cada k esta entre 1 e 12 e tau_j(k) e finito e nao negativo'],
    ['Duracao','fim-inicio = tau_j(k), sem tolerancia numerica nessa igualdade'],
    ['Sincronizacao','Os k drones de cada missao partem juntos e retornam juntos'],
    ['Capacidade','Varredura de todos os eventos e checagem adicional de cada minuto: uso <= 12'],
    ['Identidade dos drones','Alocacao construtiva a D1..D12, sem sobreposicao ou troca durante uma missao'],
    ['Regra de insercao','Outro first-fit, enumerando 0 e retornos, reproduziu todos os horarios da solucao'],
    ['Reproducao C','Recompilacao e semente fixa reproduziram exatamente a linha registrada'],
], [128,WIDTH-128],8.5)
heading('Testes de rejeicao e alcance da conclusao')
text('O verificador rejeitou seis certificados corrompidos: cliente duplicado, cliente ausente, duracao alterada, '
     'capacidade excedida, quantidade 13 e horario negativo. Os intervalos sao [inicio,fim): '
     'um drone pode retornar e partir no mesmo instante. Isso representa reuso no deposito, nao sobreposicao.')
text('A reexecucao usa o mesmo runtime C da busca original. O log fonte de 2025 registra CP-SAT 9.12.4550, '
     '16 trabalhadores e status OPTIMAL. A auditoria verifica sua testemunha viavel e conserva o log, '
     'mas nao transforma esse log em uma prova formal independente de otimalidade.')
section('Verificacoes', ['Cobertura unica, retorno ao deposito, unidades, matriz viavel, duracoes, capacidade em eventos e por minuto: aprovados.',
    'Identidades D1..D12 e sincronizacao das missoes coletivas: aprovadas.',
    'Regra first-fit independente e reproducao C: exatas. Seis corrupcoes artificiais foram rejeitadas.'])

page('Rota do caminhao','72 clientes | 73 arcos | 58 unidades de distancia Manhattan | 116 minutos')
coords={}
for line in (HERE/'fontes/105-rc-e.txt').read_text().splitlines()[7:]:
    v=line.split()
    if v: coords[int(v[0])]=(float(v[1]),float(v[2]))
x0=LEFT+35; y0=Y-305; pw=WIDTH-62; ph=280
xmax=max(x for x,y in coords.values())+1; ymax=max(y for x,y in coords.values())+1
scale=min(pw/xmax,ph/ymax)
def point(j):
    x,y=coords[j];return x0+x*scale,y0+y*scale
C.setStrokeColor(colors.HexColor('#d6e0e7')); C.setLineWidth(.4)
for v in range(0,int(xmax)+1,5):
    C.line(x0+v*scale,y0,x0+v*scale,y0+ymax*scale)
    C.setFont('Arial',7);C.setFillColor(GRAY);C.drawCentredString(x0+v*scale,y0-12,str(v))
for v in range(0,int(ymax)+1,5):
    C.line(x0,y0+v*scale,x0+xmax*scale,y0+v*scale)
    C.drawRightString(x0-7,y0+v*scale-2,str(v))
C.setStrokeColor(BLUE);C.setLineWidth(.85)
for a,b in zip([0]+S['route'],S['route']+[0]):
    ax,ay=point(a);bx,by=point(b);C.line(ax,ay,bx,ay);C.line(bx,ay,bx,by)
for j in range(1,106):
    x,y=point(j);C.setFillColor(BLUE if j in S['route'] else ORANGE);C.circle(x,y,2.4,fill=1,stroke=0)
x,y=point(0);C.setFillColor(INK);C.rect(x-4,y-4,8,8,fill=1,stroke=0)
Y=y0-30
text('Azul: cliente do caminhao. Laranja: cliente dos drones. Quadrado: deposito. '
     'O desenho usa caminhos ortogonais para representar a metrica Manhattan; nao representa uma rede viaria real.',8)
heading('Sequencia completa, com retorno ao deposito')
text('0 &gt; '+' &gt; '.join(map(str,S['route']))+' &gt; 0',9)
text('O total recalculado e 116 minutos. Nenhuma reversao 2-opt nem reinsercao de um unico cliente '
     'reduz esse tempo mantendo a atribuicao atual. Trata-se de uma verificacao local dessas vizinhancas, '
     'nao de uma prova de otimalidade do TSP com esses 72 clientes.',9)
section('Rota', ['0 -> '+' -> '.join(map(str,S['route']))+' -> 0',
    'Tempo exato: 116 min; distancia Manhattan total: 58. Nenhum 2-opt ou reinsercao individual reduz a rota atual.'])

page('Cronograma com drones identificados','Testemunha construtiva de que as 33 missoes coletivas cabem na frota de 12 drones')
gx=LEFT+33; gy=Y-325; gw=WIDTH-40; gh=300
for i in range(12):
    yy=gy+gh-(i+1)*24
    C.setFillColor(LIGHT if i%2==0 else colors.white);C.rect(gx,yy,gw,21,fill=1,stroke=0)
    C.setFillColor(INK);C.setFont('Arial',8);C.drawRightString(gx-7,yy+7,f'D{i+1}')
for m in S['missions']:
    ids=O['drone_ids'][str(m['client'])]
    for ident in ids:
        yy=gy+gh-ident*24+1; xx=gx+m['start']/116*gw; width=(m['end']-m['start'])/116*gw
        C.setFillColor(mission_color(m['client']));C.rect(xx,yy,width,18,fill=1,stroke=0)
        if width>11:
            C.setFillColor(colors.white);C.setFont('Arial',6.3);C.drawCentredString(xx+width/2,yy+6,str(m['client']))
for t in range(0,117,10):
    xx=gx+t/116*gw; C.setStrokeColor(colors.HexColor('#bac9d4'));C.setLineWidth(.25);C.line(xx,gy+12,xx,gy+gh)
    C.setFillColor(GRAY);C.setFont('Arial',7);C.drawCentredString(xx,gy-1,str(t))
Y=gy-25
text('Numeros nos blocos identificam os clientes. A mesma missao aparece em k linhas, com inicio e fim iguais. '
     'As identidades foram atribuidas pelo verificador; elas nao sao uma decisao explicita armazenada pelo SA.',9)
heading('Leitura do cronograma')
text('Todas as missoes saem do deposito e retornam a ele. O termino final dos drones ocorre em '
     '<b>115 minutos</b>, na missao do cliente 20. O caminhao retorna um minuto depois, definindo o makespan 116. '
     'O cliente 19 ocupa quatro drones de 0 a 46; o cliente 35 ocupa quatro de 84 a 112.')
text('A ordem do vetor e uma ordem de prioridade de insercao, nao uma precedencia cronologica obrigatoria. '
     'Por exemplo, a missao do cliente 95 e processada depois da 24, mas entra em uma janela que comeca antes. '
     'Isso e permitido pelo modelo e foi reproduzido pelo first-fit independente.')
section('Drones', ['33 missoes, com ocupacao simultanea maxima de 12. Ultimo retorno: cliente 20 em t=115.',
    '1326 drone-minutos ocupados. Todas as missoes receberam identidades fisicas D1..D12 sem sobreposicoes.'])

page('Ocupacao, folgas e limites inferiores','A restricao de capacidade foi verificada em todos os intervalos de eventos')
gx=LEFT+30;gy=Y-180;gw=WIDTH-45;gh=155
for u in range(0,13,2):
    yy=gy+u/12*gh;C.setStrokeColor(colors.HexColor('#d6e0e7'));C.line(gx,yy,gx+gw,yy)
    C.setFillColor(GRAY);C.setFont('Arial',7);C.drawRightString(gx-7,yy-2,str(u))
for row in O['intervals']:
    a=row['start'];b=row['end'];u=row['use'];C.setFillColor(GREEN)
    C.rect(gx+a/116*gw,gy,(b-a)/116*gw,u/12*gh,fill=1,stroke=0)
C.setStrokeColor(ORANGE);C.setLineWidth(1);C.line(gx,gy+gh,gx+gw,gy+gh)
for t in range(0,117,10):
    C.setFillColor(GRAY);C.drawCentredString(gx+t/116*gw,gy-13,str(t))
Y=gy-36
table(['Medida','Valor / interpretacao'],[
    ['Carga realizada','sum k_j x tau_j(k_j) = 1326 drone-minutos'],
    ['Limite de carga, quantidades fixas','1326 / 12 = 110,5; ao arredondar para tempos inteiros: 111'],
    ['Carga minima, mesmos 33 clientes','1271 / 12 = 105,9167; limite inteiro 106, variando quantidades'],
    ['Uso no horizonte global','1326 / (12 x 116) = 95,26%; folga de 66 drone-minutos'],
    ['Uso ate o ultimo drone','1326 / (12 x 115) = 96,09%; folga de 54 drone-minutos'],
    ['Frota completamente ocupada','85 minutos; o pico de uso e 12, sem excedentes'],
], [175,WIDTH-175],8.5)
text('Os limites 111 e 106 sao condicionados ao conjunto de clientes dos drones (e 111 tambem as quantidades). '
     'Nao sao limites globais do PDSTSP-C. A desigualdade de carga nao captura toda a perda por sincronizacao.')
text('Mantendo exatamente esta rota, nenhum ajuste apenas das quantidades ou horarios dos drones pode '
     'reduzir o makespan abaixo de 116, porque o caminhao ja exige 116. Para alcancar 112, sera necessario '
     'melhorar a rota e/ou mudar a atribuicao, alem de manter o tempo dos drones em no maximo 112.',9)
section('Carga e limites', ['Carga realizada 1326, limite de carga fixo 111. Carga minima para os mesmos clientes 1271, limite 106.',
    'Utilizacao global 95,26%; folga total 66 drone-minutos. Limites condicionais nao provam otimalidade global.',
    'O caminhao e o gargalo da testemunha de 116; refinar somente os drones nao reduz esse valor.'])

page('Todas as missoes da solucao de 116','Ordem de prioridade original; inicio e fim em minutos; identidades atribuidas pela auditoria')
rows=[]
for m in S['missions']:
    ids=O['drone_ids'][str(m['client'])]
    rows.append([m['priority'],m['client'],m['k'],m['end']-m['start'],m['start'],m['end'],','.join(map(str,ids))])
table(['Ordem','Cliente','k','Duracao','Inicio','Fim','Drones'],rows,[42,51,30,58,48,48,WIDTH-277],8)
text('Cada duracao foi comparada diretamente com tau_j(k). Quantidade maior que a minima pode ser viavel e '
     'vantajosa; nao se assume monotonicidade de duracao ou de consumo. Todos os grupos permanecem juntos '
     'durante a missao.',8.5)

for start,end,title in [(0,37,'Arcos do caminhao - parte 1'),(37,73,'Arcos do caminhao - parte 2')]:
    page(title,'Recalculo independente da rota a partir de coordenadas e velocidade de 30 km/h')
    rows=[[x['arc'],x['from'],x['to'],br(x['duration']),br(x['departure']),br(x['arrival'])] for x in O['arcs'][start:end]]
    table(['Arco','Origem','Destino','Tempo','Saida','Chegada'],rows,[43,70,70,100,100,WIDTH-383],8,row_height=16)
    text('Tempo de arco = (|x_i-x_j| + |y_i-y_j|) x 60 / 30. '
         'As chegadas sao acumuladas desde t=0; o ultimo arco retorna ao deposito em t=116.',8.5)

page('Comparacao historica e resultado de 2025','A mesma instancia e a mesma matriz; atualizacao da referencia do projeto')
table(['Solucao','Caminhao','Drones','Cmax','Clientes C/D'],[
    ['BKS original / tabela 2024','120','120','120','82 / 23'],
    ['SA do projeto','116','115','116','72 / 33'],
    ['CP-SAT / pacote 2025','112','112','112','79 / 26'],
], [150,80,65,65,WIDTH-360],8.5)
heading('O que o arquivo oficial registra')
text('Caminho interno: <b>PDSOP-c/results.zip → results/logs/tsp/105-rc-e.log</b>. '
     'O log aponta a entrada <b>data/instances/tsp/105-rc-e.txt</b> e a matriz '
     '<b>data/instances/tau_jk/tau_kj_105-rc-e.txt</b>; registra 105 clientes, 12 drones, '
     'velocidade 30 e um caminhao. A execucao e datada de 17/04/2025; o pacote esta na pagina ORLab de 10/11/2025.')
table(['Campo do log','Valor'],[['Solver','CP-SAT 9.12.4550'],['Status','OPTIMAL'],
    ['Makespan / Makespan Bound','112 / 112'],['Tempo registrado no solver','125,257 s; 16 trabalhadores'],
    ['Objetivo interno codificado','1266496; nao e o makespan em minutos']], [180,WIDTH-180],9)
text('Reconstruimos um grafo de precedencias com as sequencias de cada drone publicado. Para cada cliente, '
     'k e o numero de drones cuja sequencia contem a missao. Cada missao so comeca depois do retorno de '
     'todas as predecessoras dos seus drones. O cronograma resultante termina em 112 e respeita a capacidade.')
text('A carga da solucao publicada e <b>1344 = 12 x 112</b> drone-minutos: ela ocupa a frota sem folga '
     'no horizonte de 112. Essa igualdade confirma um limite de 112 para aquelas quantidades, '
     'mas, isoladamente, nao prova que outras atribuicoes nao possam melhorar. A prova global reportada vem do solver.')
section('Referencia atualizada', ['Pacote ORLab de 2025, log results/logs/tsp/105-rc-e.log: status OPTIMAL, Makespan=112, Makespan Bound=112.',
    'A solucao publicada foi reconstruida como DAG de sincronizacao e validada em 112. Usa 79 clientes no caminhao e 26 nos drones.',
    'A carga 1344=12x112 e uma igualdade para aquela solucao, nao uma prova global independente. O objetivo codificado 1266496 nao deve ser confundido com minutos.'])

page('As 100 execucoes da instancia','Estatisticas do lote original, sem misturar os ensaios adicionais de reproducao')
table(['Estatistica','Resultado'],[
    ['Execucoes / sementes distintas','100 / 100'],['Melhor / pior','116 / 140 min'],
    ['Media / mediana',br(ST['mean'])+' / '+br(ST['median'])+' min'],
    ['Desvio padrao amostral',br(ST['sample_std'])+' min'],
    ['Abaixo / iguais / acima de 120','2 / 34 / 64'],['Execucoes com 112','0'],
    ['Soma de CPU registrada',br(ST['total_cpu'])+' s, incluindo calibracao e SA'],
    ['Melhores execucoes','82 e 92; sementes 1792860329 e 1792860339'],
], [205,WIDTH-205],9)
entries=list(csv.DictReader((HERE/'execucoes_100.csv').open()))
counts=Counter(float(e['makespan']) for e in entries); keys=sorted(counts)
gx=LEFT+25;gy=Y-155;gw=WIDTH-35;gh=130;bw=gw/len(keys)
for i,k in enumerate(keys):
    hh=counts[k]/max(counts.values())*gh
    C.setFillColor(GREEN if k<120 else BLUE);C.rect(gx+i*bw+3,gy,bw-6,hh,fill=1,stroke=0)
    C.setFont('Arial',8);C.setFillColor(INK);C.drawCentredString(gx+(i+.5)*bw,gy+hh+5,str(counts[k]))
    C.drawCentredString(gx+(i+.5)*bw,gy-13,br(k))
Y=gy-34
text('Distribuicao dos makespans em minutos; numeros acima das barras indicam a frequencia. '
     'As duas melhorias da referencia de 120 chegaram a 116. Nenhuma execucao atingiu o valor 112.',9)
text('A melhora de uma melhor execucao nao implica que a media supere a referencia: 124,67 ainda fica '
     '3,89% acima de 120 e 11,31% acima de 112. Tempos de CPU do lote e tempos do artigo/CP-SAT '
     'nao sao comparaveis diretamente, pois hardware, paralelismo e orcamentos sao diferentes.',9)
section('Estatisticas', ['100 execucoes: melhor 116, pior 140, media 124,67, mediana 124 e desvio amostral 4,7611.',
    '2 resultados abaixo de 120, 34 iguais e 64 acima; nenhum igual a 112. CPU total registrada 723,88 s.',
    'Melhores nas execucoes 82 e 92. Tempo isolado nao fundamenta comparacao de desempenho com o solver oficial.'])

page('Parecer e reproducao','O que esta confirmado, o que deve ser corrigido e como repetir a auditoria')
heading('Parecer tecnico')
text('<b>APROVADA a viabilidade da solucao de 116 minutos.</b> Nao foi encontrado erro de cobertura, '
     'unidade, duracao, sincronizacao, capacidade ou calculo do makespan. A mesma semente reproduz '
     'a testemunha completa no ambiente C examinado.')
text('<b>ATUALIZACAO NECESSARIA da referencia.</b> O numero 120 permanece correto como referencia '
     'historica do artigo de 2024. A fonte oficial de 2025 ja registra 112 como otimo; '
     'assim, 116 nao e um novo melhor conhecido atual e esta instancia nao deve integrar uma lista '
     'atual de casos sem otimo apenas por ter LB&lt;UB na tabela antiga.')
text('Os arquivos originais do lote e a configuracao da busca foram preservados. O relatorio anterior de '
     'aleatoriedade recebeu uma nota de atualizacao. A revisao das referencias das outras instancias '
     'exige consultar suas respectivas fontes; a descoberta sobre 105-rc-e nao deve ser generalizada sem verificacao.')
heading('Reproduzir os calculos')
text('Da raiz do projeto: <b>python resultados/105-rc-e/verificar_105.py</b>. '
     'O verificador usa apenas a biblioteca padrao do Python. Ele exige os arquivos de fonte e '
     'a reproducao.txt que acompanham este pacote.',9)
text('Recompilar a testemunha C: <b>gcc -O2 -std=c11 -Wall -Wextra -pedantic -I. '
     'resultados/105-rc-e/reproduzir_105.c cv.c lerInstancias.c SA.c -lm -o reproduzir_105.exe</b>. '
     'Executar da raiz e comparar a linha FINAL com fontes/solucao_registrada.txt. '
     'Geradores rand() de outros runtimes podem produzir outra trajetoria para a mesma semente.',8.5)
heading('Referencias e arquivos de apoio')
for label,url in [
    ('Artigo de 2024, Tabela 2 / preprint local, p. 17','https://doi.org/10.1016/j.cor.2023.106514'),
    ('ORLab: pacote original de instancias e BKS','https://orlab.com.vn/download/instances-for-the-parallel-drone-scheduling-traveling-salesman-problem-with-collective-drones/'),
    ('ORLab: dados e resultados de 2025, incluindo o log de 112','https://orlab.com.vn/download/data-and-results-of-pdstsp-c-and-pdsop-c/')]:
    text('<link href="'+url+'" color="#226a9c">'+label+'</link>',8.5,6)
text('Acompanha: validacao.json, certificados solucao_116.json e solucao_orlab_112.json, tabelas de missoes, '
     'ocupacao e arcos, dados das 100 execucoes, codigo de reproducao, verificador e fontes congeladas. '
     'Data desta revisao: 26/10/2026.',8.5)
section('Parecer final', ['Viabilidade de 116 aprovada. Comparacao com 120 e historica; frente ao otimo reportado 112 o gap e 3,57%.',
    'Main e logs do lote foram preservados para reproducibilidade. A lista de instancias sem otimo baseada em 2024 precisa ser revista com fontes atualizadas.',
    'O script verificar_105.py e independente do avaliador C e utiliza aritmetica racional exata.'])
C.save()

# Versao textual completa, com tabelas integrais para busca e revisao.
md=['# Auditoria detalhada da instancia 105-rc-e','', '**Data:** 26/10/2026.','',
    '**Parecer:** 116 e viavel; a fonte oficial de 2025 reporta otimo 112. Gap atualizado: +3,57%.','']
for title,paras in sections:
    md += ['## '+title,'']
    for p in paras: md += [p,'']
md += ['## Todas as missoes de 116','', '|Prioridade|Cliente|k|Inicio|Fim|Duracao|Drones identificados|',
       '|---:|---:|---:|---:|---:|---:|---|']
for m in S['missions']:
    ids=O['drone_ids'][str(m['client'])]
    md.append(f"|{m['priority']}|{m['client']}|{m['k']}|{m['start']}|{m['end']}|{m['end']-m['start']}|{','.join(map(str,ids))}|")
md += ['', '## Todos os arcos do caminhao','', '|Arco|Origem|Destino|Tempo|Saida|Chegada|', '|---:|---:|---:|---:|---:|---:|']
for a in O['arcs']:md.append('|'+ '|'.join(str(a[k]) for k in ['arc','from','to','duration','departure','arrival'])+'|')
md += ['', '## Fontes e rastreabilidade','',
    '[Artigo, 2024](https://doi.org/10.1016/j.cor.2023.106514). Tabela 2: LB=109, UB=120.',
    '[Pacote ORLab 2025](https://orlab.com.vn/download/data-and-results-of-pdstsp-c-and-pdsop-c/). Log results/logs/tsp/105-rc-e.log.',
    '', 'O log registra execucao em 17/04/2025, CP-SAT 9.12.4550, 16 trabalhadores, tempo 125,257 s, OPTIMAL, Makespan=112 e Makespan Bound=112.',
    'O campo objective=1266496 e uma codificacao do objetivo; nao e o makespan.', '', '### Hashes SHA-256','']
for name,h in D['source_hashes'].items():md += [f'- `{name}`: `{h}`']
md += ['','### Reproducao','', '```powershell', 'python resultados/105-rc-e/verificar_105.py', '```','',
       'As restricoes energeticas sao verificadas pela admissibilidade na matriz oficial, sem recalculo fisico independente. '
       'A prova de otimalidade reportada pelo solver de 2025 nao foi reexecutada; a viabilidade da sua testemunha foi.', '']
(HERE/'relatorio_105_rc_e.md').write_text(pt('\n'.join(md)),encoding='utf-8')
source_hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [ROOT/x for x in ['SA.c','SA.h','cv.c','cv.h','main.c','main.h','lerInstancias.c']]}
(HERE/'hashes_codigo.json').write_text(json.dumps(source_hashes,indent=2),encoding='utf-8')
shutil.copyfile(OUTPUT/'relatorio_105_rc_e.pdf',HERE/'relatorio_105_rc_e.pdf')
print(f'PDF gerado: {page_no} paginas')

