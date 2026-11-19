# Releitura integral e recalculo da 126-rc-e

Data: 11/11/2026. Foram relidas as 23 paginas do artigo, alem da capa do PDF
fornecido, e conferidas visualmente as paginas com formulas e parametros.
Este trabalho nao modifica o SA, as entradas, a tabela publica ou best.json.

## Resultado principal

Com as entradas locais e o modelo energetico distribuido, **76 minutos e
impossivel e 112 minutos e otimo com cinco caminhoes**. A conclusao nao
depende de uma tentativa malsucedida do SA: existe um limite inferior de
112 e um certificado viavel de 112, recalculado nesta revisao.

O artigo anexado e Nguyen e Ha, Transportation Science 57(4), 866-888,
2023, DOI 10.1287/trsc.2022.1192. Ele estuda **um caminhao** e publica para
126-rc-e RnR fast=148 e RnR=144 na Tabela 3, p.882, pagina 18 do PDF.
O valor **76 com cinco caminhoes** pertence a outro trabalho: Montemanni,
Dell'Amico e Corsini, MIC 2024, Tabela 1, p.140, DOI
[10.1007/978-3-031-62912-9_13](https://doi.org/10.1007/978-3-031-62912-9_13).
O par de limites publicado e [41,76]; 76 e um limite superior, nao uma
prova de otimo global.

A tabela MIC foi novamente localizada no indice do PDF institucional:
[fonte primaria](https://iris.unimore.it/bitstream/11380/1342368/2/chapter.pdf).
O acesso direto ao PDF retornou 403; a tabela indexada permite confirmar
a celula, mas nao fornece o certificado de rotas e missoes.

## Mapa da leitura

| Parte | Conteudo relevante |
| --- | --- |
| Secoes 1-2, pp.866-869 | Motivacao dos drones coletivos e autonomia dependente de carga/velocidade; nao existe velocidade constante universal. |
| 3.1-3.3, pp.870-871 | Missao deposito-cliente-deposito, carga compartilhada, seis fases, corrente maxima e capacidade com Peukert. |
| 3.4-3.5, pp.871-873 | MILP de um caminhao e extracao das cadeias de drones; objetivo e ultimo retorno, nao soma das duracoes. |
| 3.6, pp.873-874 | Potencia de voo, equilibrio do angulo e busca das velocidades a cada 0,2 m/s. |
| Secao 4, pp.874-879 | RnR, remocao/insercao e avaliacao por ordenacao topologica; o metodo do artigo nao e SA. |
| Secao 5, pp.879-882 | Geracao do benchmark, arredondamento, resultados de 30 repeticoes e resultado 144 para a 126-rc-e. |
| Secao 6, pp.881-887 | Sensibilidade; parametros de bateria, limites de massa e cooperacao. Aumentar bateria tambem aumenta massa. |
| Secao 7 e referencias, pp.887-888 | Conclusoes e limites da investigacao experimental. |

## Como calcular o custo de tempo de uma missao

Para cada cliente j e quantidade k, precalcular uma duracao tau[j,k]. A
decisao de usar k unidades nao divide diretamente o tempo por k: divide a
carga transportada por unidade, alterando potencia, energia e velocidades.

1. Distancia aerea de ida, em metros:

   `d = 1000*sqrt((xj-x0)^2 + (yj-y0)^2)`.

2. Carga por unidade `w_bar = wj/k`. Massa de cada drone sem encomenda
   `W = frame + bateria = 1,20 + 1,55 = 2,75 kg` nesta instancia.
   A massa total de uma formacao e `k*W + wj`; nao e apenas o peso da carga.

3. Checar regras de admissibilidade: k entre 1 e a frota, limite de seis
   unidades por formacao e massa combinada de no maximo 25 kg na reproducao
   principal. A Secao 6.6 confirma que o limite inclui baterias e drones:
   seu exemplo de quatro unidades de 4,64 kg deixa apenas 6,44 kg para carga.

4. Considerar seis fases: decolagem com carga, cruzeiro com carga, pouso
   com carga, decolagem sem carga, cruzeiro sem carga e pouso sem carga.
   Para h=100 m, v_to=v_ld=2 m/s, cada fase vertical custa 50 segundos:
   sao **200 segundos verticais**, nao 100 e nao 200 minutos.

5. Para cada par de velocidades de cruzeiro em [4,28] m/s, com passo 0,2,
   calcular a potencia das fases. A ida usa w_bar; a volta usa carga zero.
   Obter a corrente `I_l = P_l / 12,8` em amperes e exigir `I_l <= 24`
   em todas as fases.

6. Calcular capacidade consumida por unidade com Peukert:

   `C_usada = sum_l I_l*(I_l/I_nom)^(p-1)*Delta_t_l/3600`,

   com Delta_t em segundos, p=1,1 e I_nom=12 A para uma bateria de 12 Ah
   na taxa nominal 1C. Exigir `C_usada <= 0,9*12 = 10,8 Ah`.
   O fator /3600 e indispensavel. Se a formula for escrita com duracoes
   em horas, ele ja esta incorporado na unidade de Delta_t.

   O teste acima e por unidade: nao se deve multiplicar a capacidade por k
   mantendo a potencia por unidade, pois isso contaria a bateria duas vezes.
   A energia de Peukert tambem nao deve ser substituida simplesmente por
   potencia vezes tempo comparada a um numero em Ah.

7. Entre os pares admissiveis, escolher o que minimiza `d/v_ida+d/v_volta`,
   e nao o que minimiza energia. Minimizar energia e um teste auxiliar de
   viabilidade, nao o objetivo temporal da missao.

8. Aplicar a Equacao (1) e arredondar para cima em minutos:

   `tau[j,k] = ceil((2*h/v_to + 2*h/v_ld + d/v_ida + d/v_volta)/60 + preparacao)`.

   Na reproducao principal, preparacao=1 minuto para k>1 e 0 para k=1.
   Se nenhum par e admissivel, a celula vale -1, e nao zero.

Exemplo recalculado: cliente 1 em (7,14), carga 0,46 kg, k=2. O melhor
par e aproximadamente 14,8 m/s na ida e 14,6 m/s na volta; consumo
10,7971 Ah por unidade. A expressao acima resulta em **40 minutos**,
igual a matriz local. Seu tempo de atendimento nao e 40/2.

No escalonamento, uma missao iniciada em s termina em s+tau e reserva k
drones nesse intervalo, inclusive durante o preparo. A soma dos k ativos
nao pode ultrapassar 12. O objetivo global e o maximo entre retornos dos
caminhoes e fins das missoes; nao se soma tau entre missoes paralelas.

## Potencia e ambiguidades que precisam ser documentadas

A Equacao (23) usa T=(W+w_bar)*g e a potencia vertical:

`P_vertical = k1*T*(v_vertical/2 + sqrt((v_vertical/2)^2 + T/k2^2)) + c2*T^(3/2)`.

No codigo distribuido, a potencia horizontal e:

`P_cruise = (c1+c2)*((T-c5*(v*cos(alpha))^2)^2 + (c4*v^2)^2)^(3/4) + c4*v^3`.

O angulo satisfaz a Equacao (25):

`tan(alpha)*(T-c5*(v*cos(alpha))^2) = c4*v^2`.

O codigo define k1=0,8554, c2=0,3177, c4=0,0296,
c5=0,0279, g=9,81, rho=1,22 e area A=k*4*pi*0,15^2, com
k2=sqrt(2*rho*A) e c1=k1/k2. Esses coeficientes/detalhes foram obtidos do
codigo original; o PDF remete a Liu et al. para a calibracao e nao fornece
um conjunto executavel completo e inequivoco apenas nas equacoes.

Divergencias conferidas nas imagens das paginas:

- Equacao (4), p.871: imprime C_d >= beta*C_nom. O texto imediatamente
  anterior limita o consumo a no maximo beta da bateria, o que implica <=.
  A reproducao usa <=, como o codigo original, sem tratar o sinal impresso
  como uma regra fisicamente coerente.
- Tabela 5, p.883: imprime preparacao em segundos com valor 1. As Secoes
  5.1 e 6.2.1 dizem um minuto; other-params.txt tambem registra 1 na
  convencao temporal do benchmark. A reproducao principal usa um minuto.
- Algoritmo 2, linha 14, p.874: repete v_in no par atribuido. O texto e o
  codigo selecionam v_in e v_out separadamente.
- Equacao (24), p.873: nao mostra o termo `(c4*v^2)^2` que aparece no C++.
  Essa diferenca pode alterar viabilidade, nao apenas minutos arredondados.
- Secao 5.1 diz frota grande entre 5 e 10; o arquivo local da 126-rc-e
  informa 12. Isso reforca a necessidade de rastrear a versao exata dos
  dados. Nao foi alterada a frota local para forcar correspondencia.

Nao e correto escolher silenciosamente a combinacao de ambiguidades que
produza o valor desejado. Os cenarios abaixo ficam separados dos dados
usados pelo programa.

## Recalculo e prova da inviabilidade de 76

Foram enumeradas as velocidades na mesma malha do codigo distribuido,
reutilizando suas funcoes de potencia e conferindo todas as correntes.
O recalculo com k<=6 e massa<=25 reproduziu **1512 de 1512 celulas**, sem
divergencias. A soma das correntes efetivas permite ainda calcular o menor
consumo possivel para cada cliente/k, independentemente de buscar o par
mais rapido.

### Cliente 46

Posicao (14,14), carga 2,25 kg, distancia aerea de ida 19,798989873 km.
Capacidade utilizavel 10,8 Ah por unidade.

| k | Massa combinada (kg) | Menor consumo por unidade na malha (Ah) | Admissibilidade |
| --- | ---: | ---: | --- |
| 1 | 5,00 | Inviavel por corrente | Decolagem exige 39,6373 A > 24 A |
| 2 | 7,75 | 15,9230 | Excede bateria |
| 3 | 10,50 | 13,5202 | Excede bateria |
| 4 | 13,25 | 12,2496 | Excede bateria |
| 5 | 16,00 | 11,4416 | Excede bateria |
| 6 | 18,75 | 10,8715 | Excede bateria |
| 7 | 21,50 | 10,4415 | Energia viavel, mas excede seis unidades |
| 8 | 24,25 | 10,1033 | Energia viavel, mas excede seis unidades |

Os valores de consumo sao totais de uma missao por unidade, incluindo
ida/volta e fases verticais, nao uma potencia instantanea. Para k=6,
a maior distancia aerea de ida que a bateria comporta nesta malha e
19,6609 km, abaixo de 19,7990 km. A diferenca nao e removida por ceil.

Logo o cliente 46 precisa de caminhao. O menor passeio fechado que o
contenha tem ao menos `2*(14+14)=56 km`, por desigualdade triangular.
A 30 km/h: **56/30*60 = 112 minutos**. Acrescentar caminhoes nao reduz a
ida e volta de um cliente que deve ser servido por um unico caminhao.

Ha sete clientes que, individualmente, impedem makespan 76 no modelo
principal: 28,46,52,70,91,96,97. Seus limites individuais sao,
respectivamente, 92,112,100,88,104,104,108 minutos.

### Cliente 97: obstaculo que nao depende da Equacao (24)

Posicao (12,15), carga **13,83 kg**. Pela massa maxima, k<=4:
k=4 pesa 24,83 kg, enquanto k=5 ja pesa 27,58 kg.
Porem, com quatro unidades, a corrente de decolagem e **35,4094 A**, acima
de 24 A. Quantidades menores exigem correntes ainda maiores. k=6 reduz a
corrente de decolagem para 23,9210 A, mas pesa **30,33 kg**, excedendo 25 kg.
Portanto nenhuma formacao satisfaz simultaneamente massa e corrente.

Esse cliente e obrigatorio no caminhao independentemente da potencia
horizontal: `2*(12+15)/30*60 = 108 minutos`. Nem uma correcao isolada da
Equacao (24), do tempo de preparacao ou da aproximacao de alpha permite 76.

## Sensibilidades realizadas

| Interpretacao | Limite inferior demonstrado | 76 possivel? |
| --- | ---: | --- |
| Codigo distribuido + k<=6 + massa<=25 | 112 | Nao |
| Equacao (24) como impressa + mesmas regras | 108 | Nao |
| Codigo distribuido + permitir ate 12 unidades + massa<=25 | 108 | Nao |
| Equacao impressa + ate 12 unidades + massa<=25 | 108 | Nao |
| Codigo distribuido + ate 12 unidades + sem limite de massa | 108 | Nao |
| Equacao impressa + ate 12 unidades + sem limite de massa | 104 | Nao |

As relaxacoes de tamanho/massa sao experimentos hipoteticos; nao foram
aceitas como alteracoes do problema. Um limite inferior nao e, em geral,
um valor atingivel; a tabela apenas demonstra que 76 continua impossivel.

Como o cliente 46 fica perto do limite de bateria, foi repetida tambem a
analise com alpha obtido por 100 passos de bissecao da Equacao (25), em vez
da tolerancia de 5 N/grade de 0,5 grau do codigo. Com k=6, o menor consumo
passa a 10,9302 Ah no modelo distribuido: permanece inviavel. Na equacao
horizontal impressa, passa a 10,7511 Ah e tau=54 (o alpha aproximado dava
tau=52). Os limites globais da tabela continuam iguais.

Usar um segundo em vez de um minuto de preparacao nao altera o consumo
energetico considerado pelo modelo, nem elimina os clientes obrigatorios
no caminhao. Portanto tampouco torna 76 viavel.

## Certificados recalculados

1. Solucao local com cinco caminhoes: tamanhos das rotas 51,32,40,3,0;
   duracoes 112,112,112,40,0; nenhuma missao de drone. Cobertura completa e
   atendimento unico conferidos pelo validador Python. Makespan 112.
   Junto ao limite 112, prova o otimo local para S=5.
2. Solucao original em bks.txt, um caminhao: as cadeias individuais foram
   convertidas em precedencias, as quantidades k contadas e os horarios
   reconstruidos por ordenacao topologica. Sao 93 clientes em caminhao e
   33 missoes de drones; caminhao=144, drones=138, makespan=144. Coincide
   com RnR da Tabela 3 do PDF. Isso verifica a interpretacao de tempos e
   IDs, mas nao reproduz o experimento MIC de cinco caminhoes.

Nao foi necessario insistir no SA para tentar violar um limite inferior.
Semente e tempo de busca nao podem reduzir o valor local abaixo de 112.

## O que falta para reproduzir 76

E necessario obter do experimento MIC a matriz tau exata, os arquivos da
instancia, as unidades/parametros do modelo fisico e o certificado com rotas
e missoes (cliente,k,inicio,fim). A presenca dos clientes 46 e 97 permite
conferir rapidamente se os dados sao os mesmos. Nao se conclui que o
resultado publicado e invalido; conclui-se que ele nao corresponde a uma
solucao viavel das entradas locais com as regras aqui reproduzidas.

## Artefatos e reproducao

`python3 scripts/revisar_fisica126.py` (requer G++) recompila apenas uma
copia experimental do modelo. A dependência de plotagem foi removida dessa
copia; a implementacao original foi preservada. A malha de velocidades,
coeficientes, comparacoes, valores e arquivos-fonte ficam em
`output/pdf/revisao126_20261111/`:

- `fisica.csv`: 6048 linhas, quatro variantes energeticas, 126 clientes,
  k=1..12; tau bruto antes das regras de massa/tamanho.
- `resultado.json`: comparacao completa, limites, clientes impeditivos,
  parametros e hashes de PDF/entradas.
- `certificado_s5.json` e `certificado_rnr_s1.json`: certificados conferidos.
- `sensibilidade.cpp` e `energymodel.h`: calculo reproduzivel.
- `pagina_*.png`: paginas conferidas visualmente.

Os consumos minimos e inviabilidades energeticas sao relativos ao modelo
e a malha de velocidades publicados/distribuidos, nao uma prova sobre
qualquer drone fisico nem sobre um problema continuo com outras equacoes.
O limite 112 e a prova de otimo sao exatos para a matriz local atual.
