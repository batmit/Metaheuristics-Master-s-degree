# Revisão das 60 instâncias PDSTSP-c: 2024 versus ORLab 2025

Data da revisão: **26/10/2026**. Todos os tempos de solução estão em minutos.

**Resultado principal:** 56 casos com ótimo reportado e quatro ainda abertos na fonte oficial consultada. Dos 23 abertos em 2024, 19 tiveram a otimalidade reportada posteriormente. Houve melhoria do limite superior em 19 casos, incluindo a 105-rc-e; portanto são 18 outras melhorias.

## Quais ainda estão abertos

|Instância|Clientes|Drones|UB 2024|LB 2025|UB 2025|Gap (%)|
|---|---|---|---|---|---|---|
|67-c-c|67|10|76|73|74|1.3514|
|165-r-c|165|10|164|156|160|2.5000|
|167-r-e|167|11|188|179|180|0.5556|
|181-r-e|181|9|224|215|216|0.4630|

Gap = 100 × (UB − LB) / UB. UB é uma solução viável; LB é o limite inferior reportado. Os quatro logs terminam com status FEASIBLE após aproximadamente 3600 s, com LB < UB. Isso não prova otimalidade. As respectivas linhas da planilha são 36, 54, 55 e 58.

Três intervalos têm largura de um minuto (67-c-c, 167-r-e, 181-r-e). O intervalo da 165-r-c tem largura de quatro minutos. Um resultado igual a UB empata a referência; para melhorar é preciso ficar abaixo de UB e não abaixo de LB, se o limite publicado é válido.

## Soluções melhores que em 2024

|Instância|UB 2024|UB 2025|Redução|Redução (%)|Situação 2025|
|---|---|---|---|---|---|
|67-c-c|76|74|2|2.6316|FEASIBLE|
|91-r-c|124|120|4|3.2258|OPTIMAL|
|99-rc-c|100|99|1|1.0000|OPTIMAL|
|101-r-c|120|116|4|3.3333|OPTIMAL|
|103-rc-c|124|120|4|3.2258|OPTIMAL|
|105-rc-e|120|112|8|6.6667|OPTIMAL|
|108-rc-e|136|135|1|0.7353|OPTIMAL|
|121-rc-e|124|121|3|2.4194|OPTIMAL|
|126-r-c|160|152|8|5.0000|OPTIMAL|
|126-rc-e|144|136|8|5.5556|OPTIMAL|
|144-rc-c|128|124|4|3.1250|OPTIMAL|
|154-c-c|72|68|4|5.5556|OPTIMAL|
|165-r-c|164|160|4|2.4390|FEASIBLE|
|167-r-e|188|180|8|4.2553|FEASIBLE|
|173-r-c|164|160|4|2.4390|OPTIMAL|
|173-rc-c|133|130|3|2.2556|OPTIMAL|
|181-r-e|224|216|8|3.5714|FEASIBLE|
|187-rc-e|196|188|8|4.0816|OPTIMAL|
|200-r-e|212|200|12|5.6604|OPTIMAL|

## Ótimos provados sem alteração da melhor solução

Melhoria de solução e prova de ótimo são avanços diferentes. Estes casos já tinham o valor abaixo em 2024, mas a prova estava em aberto:

|Instância|Valor de 2024 e ótimo de 2025|
|---|---|
|20-c-c|43|
|21-c-c|62|
|26-r-c|103|
|114-rc-c|96|

## Cobertura por tamanho

- Pequenas (até 30 clientes): 30/30 com ótimo reportado.
- Grandes (mais de 30 clientes, classificação do artigo): 26/30 com ótimo reportado; quatro abertas.
- No subconjunto até 100 clientes usado pelo projeto, só a 67-c-c permanece aberta nesta fonte.
- Não existe um limiar de tamanho que determine a ausência de ótimo: a 200-r-e, por exemplo, tem ótimo reportado de 200.

## Metodologia e grau de verificação

1. Li a aba PDSTSP-c da planilha oficial. G/H são os limites da referência anterior; J/K são os limites CP-interval. A aba contém 60 linhas de instâncias (linhas 3 a 62); os outros dados formatados não são instâncias adicionais.
2. Conferi individualmente os 60 logs results/logs/tsp/*.log e os campos finais de status, Makespan e Makespan Bound. O objective codificado do CP-SAT não foi usado como tempo em minutos.
3. Conferi os 60 pares de entrada e matriz contra os arquivos do projeto, byte a byte e por SHA-256. Os logs usam um caminhão, a frota do arquivo e velocidade 30.
4. Reconstruí cada cronograma publicado pelas precedências de cada drone, sincronizando as missões coletivas. Recalculei a rota Manhattan, a duração de cada missão pela matriz oficial, a cobertura exata de todos os clientes e a capacidade simultânea com frações exatas.
5. As 60 testemunhas resultaram no makespan declarado, e os 60 pares de limites dos logs coincidiram com J/K da planilha. Essa checagem valida a viabilidade, mas não reexecuta as provas de otimalidade dos solvers.
6. Cruzei a classificação antiga com o CSV histórico do projeto: são exatamente os mesmos 23 casos abertos. Os valores históricos disponíveis no projeto coincidem com a planilha. As diferenças de rótulo 144-rc-r/144-rc-c e 173-rc-r/173-rc-c seguem a correspondência já documentada.

## Limitação temporal da conclusão

A consulta na internet desta revisão não identificou uma fonte primária posterior que encerrasse esses quatro casos. Isso não demonstra que tal resultado não exista. A afirmação verificável é que esses são os quatro casos sem prova no pacote oficial de 2025 examinado, cujo hash está em resumo.json. Os 56 casos fechados permanecem fechados para os mesmos dados e regras.

## Consequência para os experimentos do projeto

A lista de 23 casos usada na main reflete 2024. Para testar somente casos ainda abertos de acordo com a fonte atualizada, o conjunto apropriado passa a ser 67-c-c, 165-r-c, 167-r-e e 181-r-e, com referências 74, 160, 180 e 216. Nenhuma configuração ou algoritmo de produção foi alterado nesta análise.

## Todas as 60 instâncias

|Instância|LB 2024|UB 2024|LB 2025|UB 2025|Status|Células da fonte|
|---|---|---|---|---|---|---|
|15-r-e|92|92|92|92|OPTIMAL|A4:M4|
|15-rc-c|44|44|44|44|OPTIMAL|A3:M3|
|16-c-c|60|60|60|60|OPTIMAL|A5:M5|
|16-r-e|112|112|112|112|OPTIMAL|A6:M6|
|18-c-c|56|56|56|56|OPTIMAL|A9:M9|
|18-r-e|96|96|96|96|OPTIMAL|A8:M8|
|18-rc-c|57|57|57|57|OPTIMAL|A7:M7|
|19-c-c|44|44|44|44|OPTIMAL|A10:M10|
|20-c-c|40|43|43|43|OPTIMAL|A11:M11|
|20-r-c|64|64|64|64|OPTIMAL|A12:M12|
|20-r-e|80|80|80|80|OPTIMAL|A14:M14|
|20-rc-c|96|96|96|96|OPTIMAL|A15:M15|
|20-rc-e|100|100|100|100|OPTIMAL|A13:M13|
|21-c-c|60|62|62|62|OPTIMAL|A17:M17|
|21-r-e|85|85|85|85|OPTIMAL|A16:M16|
|23-c-e|80|80|80|80|OPTIMAL|A18:M18|
|23-r-c|88|88|88|88|OPTIMAL|A19:M19|
|24-c-e|84|84|84|84|OPTIMAL|A21:M21|
|24-r-e|112|112|112|112|OPTIMAL|A22:M22|
|24-rc-c|70|70|70|70|OPTIMAL|A20:M20|
|25-c-c|56|56|56|56|OPTIMAL|A25:M25|
|25-r-e|104|104|104|104|OPTIMAL|A23:M23|
|25-rc-e|92|92|92|92|OPTIMAL|A24:M24|
|26-r-c|101|103|103|103|OPTIMAL|A26:M26|
|27-c-c|84|84|84|84|OPTIMAL|A30:M30|
|27-c-e|68|68|68|68|OPTIMAL|A28:M28|
|27-rc-c|100|100|100|100|OPTIMAL|A27:M27|
|27-rc-e|84|84|84|84|OPTIMAL|A29:M29|
|29-rc-e|116|116|116|116|OPTIMAL|A31:M31|
|30-c-c|96|96|96|96|OPTIMAL|A32:M32|
|50-r-e|116|116|116|116|OPTIMAL|A33:M33|
|53-r-e|132|132|132|132|OPTIMAL|A34:M34|
|66-rc-e|124|124|124|124|OPTIMAL|A35:M35|
|67-c-c|73|76|73|74|FEASIBLE|A36:M36|
|68-rc-c|76|76|76|76|OPTIMAL|A37:M37|
|76-c-c|52|52|52|52|OPTIMAL|A38:M38|
|82-c-e|64|64|64|64|OPTIMAL|A40:M40|
|82-rc-c|104|104|104|104|OPTIMAL|A39:M39|
|88-c-e|108|108|108|108|OPTIMAL|A41:M41|
|91-r-c|120|124|120|120|OPTIMAL|A42:M42|
|99-rc-c|98|100|99|99|OPTIMAL|A43:M43|
|101-r-c|114|120|116|116|OPTIMAL|A44:M44|
|103-rc-c|120|124|120|120|OPTIMAL|A45:M45|
|105-rc-e|109|120|112|112|OPTIMAL|A46:M46|
|108-rc-e|134|136|135|135|OPTIMAL|A47:M47|
|114-rc-c|94|96|96|96|OPTIMAL|A48:M48|
|121-rc-e|121|124|121|121|OPTIMAL|A49:M49|
|126-r-c|151|160|152|152|OPTIMAL|A51:M51|
|126-rc-e|136|144|136|136|OPTIMAL|A50:M50|
|144-rc-c|122|128|124|124|OPTIMAL|A52:M52|
|154-c-c|68|72|68|68|OPTIMAL|A53:M53|
|165-r-c|140|164|156|160|FEASIBLE|A54:M54|
|167-r-e|160|188|179|180|FEASIBLE|A55:M55|
|173-r-c|141|164|160|160|OPTIMAL|A57:M57|
|173-rc-c|115|133|130|130|OPTIMAL|A56:M56|
|181-r-e|199|224|215|216|FEASIBLE|A58:M58|
|185-c-c|96|96|96|96|OPTIMAL|A59:M59|
|187-rc-e|167|196|188|188|OPTIMAL|A60:M60|
|198-c-c|64|64|64|64|OPTIMAL|A61:M61|
|200-r-e|162|212|200|200|OPTIMAL|A62:M62|

## Fontes

- [Montemanni, Dell’Amico e Corsini (2024), Tabelas 1 e 2](https://doi.org/10.1016/j.cor.2023.106514).
- [Pacote oficial ORLab, dados e resultados de PDSTSP-c e PDSOP-c, 10/11/2025](https://orlab.com.vn/download/data-and-results-of-pdstsp-c-and-pdsop-c/).
- Arquivo interno: PDSOP-c/results.zip → Results of PDSTSP-c and PDSOP-c .xlsx, aba PDSTSP-c.
- Logs internos: PDSOP-c/results.zip → results/logs/tsp/.
- A planilha original está preservada em fontes/Resultados_ORLab_2025.xlsx e os 60 logs em fontes/logs_tsp/.

## Arquivos para revisão

- comparacao_60_instancias.csv: comparação completa com hashes, células e log por instância.
- ainda_abertas.csv: os quatro casos com LB < UB.
- melhorias_2024_2025.csv: os 19 casos com UB menor.
- testemunhas_validadas.json: as 60 rotas e cronogramas reconstruídos.
- resumo.json: contagens e limites da conclusão.
