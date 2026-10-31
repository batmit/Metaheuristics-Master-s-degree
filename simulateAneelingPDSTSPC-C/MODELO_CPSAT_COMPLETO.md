# Busca completa de uma solução abaixo de 160 na 165-r-c

`cp_sat_completo.py` e um modelo independente do reparo local usado no SA.
Ele nao altera os fontes C nem fixa decisoes a partir de solucoes anteriores.

## Decisoes e restricoes

- Cada um dos 165 clientes pode ir para o caminhao ou para uma missao de
  drones, respeitando as combinacoes viaveis da matriz oficial. Clientes sem
  qualquer combinacao viavel de drones sao obrigatorios no caminhao.
- O circuito do caminhao e decidido pelo solver. Locais com coordenadas iguais
  sao agrupados somente no circuito: as decisoes dos clientes continuam
  individuais. Como nao ha tempo de servico e a distancia e metrica, visitas
  repetidas ao mesmo local podem ser reunidas sem aumentar o custo.
- Cada missao escolhe quantidade de drones, duracao, inicio e termino.
  Intervalos opcionais e uma restricao cumulativa respeitam a frota de dez.
- A carga exata e a carga minima das missoes reforcam as restricoes de tempo.
- O objetivo e minimizar o makespan com um teto. Na busca principal, o teto e
  159. A integralidade das coordenadas, a velocidade 30 e a distancia Manhattan
  tornam os tempos de circuito multiplos de quatro: a rota precisa ser <=156.
- Quantidades com demanda e duracao simultaneamente piores sao removidas por
  dominancia. Uma missao dominada pode ser substituida sem aumentar consumo
  de drones nem termino; essa remocao nao exclui a existencia de uma solucao.
- A reversao de uma rota simetrica tem mesmo custo; uma quebra de simetria
  conserva uma das duas orientacoes.

Nao ha subconjunto de clientes fixado, blocos externos, trechos de rota
preservados ou compromisso com as atribuicoes da heuristica. A testemunha
oficial de 160 e fornecida apenas como hint, que pode ser desrespeitado pelo
solver. O limite inferior publicado de 156 nao e imposto como restricao: o
solver calcula seus proprios limites neste modelo.

## Executar

Usar o ambiente OR-Tools ja instalado no projeto, a partir da raiz:

```powershell
.venv-cpsat/Scripts/python.exe cp_sat_completo.py --instance 165-r-c --horizon 159 --seconds 3600 --workers 8 --out resultados/nova_busca_completa_165
```

O script impede sobrescrever uma pasta que ja contenha seu manifesto.
O limite `--seconds` e o tempo de busca do solver; a construcao do modelo e
a verificacao do certificado ocorrem fora dele. O solver para na primeira
solucao dentro do alvo e a valida independentemente com fracoes exatas.

## Interpretacao

| Resultado | O que permite concluir |
|---|---|
| Solucao validada de ate 159 | Existe uma solucao melhor que 160. |
| INFEASIBLE com teto 159 | O modelo completo nao admite makespan <=159. Junto a uma testemunha valida de 160 e a integralidade dos tempos, demonstra otimalidade 160 para estes dados e regras. |
| UNKNOWN por prazo | Nao encontrou solucao nem demonstrou impossibilidade; a questao permanece aberta. |
| MODEL_INVALID ou erro | Falha de formulacao/execucao; nenhuma conclusao sobre a instancia. |

O status OPTIMAL de uma execucao que encontrou uma solucao so deve ser
interpretado junto ao objetivo e ao limite do solver. O script interrompe a
busca apos o primeiro certificado; em geral isso retorna FEASIBLE, sem provar
otimalidade. Um limite inferior nao e uma solucao viavel.

## Auditoria

Cada pasta de execucao contem:

- `manifesto.json`: parametros, versao OR-Tools, hashes de entradas/codigo/modelo.
- `modelo.pbtxt`: modelo completo exportado para inspecao.
- `cp_sat_completo.py`: fonte congelado da execucao.
- `solver.log`: log detalhado do CP-SAT.
- `progresso.json`: ultimo heartbeat, emitido aproximadamente a cada 30 s.
- `solucao.json`, quando encontrada: rota e cronograma completos.
- `resultado.json` e `relatorio.md`: status final, limites e interpretacao.

Antes da busca principal, o modelo reproduziu a testemunha de 160 com
caminhao=160 e drones=157, em cerca de 13 segundos de solver. Esse controle
esta em `resultados/cpsat_completo_165_validacao160_20261028/`.

Os seis testes de `tests/test_cp_sat_completo.py` cobrem atribuicoes livres,
inviabilidade global em caso pequeno, hint que nao fixa decisoes, dominancia,
clientes co-localizados/no deposito e capacidade cumulativa.

```powershell
.venv-cpsat/Scripts/python.exe -m unittest discover -s tests -p test_cp_sat_completo.py -v
```

Encontrar um certificado e provar inviabilidade sao verificacoes diferentes:
as solucoes exportadas sao conferidas por um verificador independente; uma
conclusao de inviabilidade e o resultado do solver sobre o modelo exportado.
