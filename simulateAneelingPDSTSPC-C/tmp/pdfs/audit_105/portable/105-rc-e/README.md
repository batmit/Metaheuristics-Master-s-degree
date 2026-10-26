# Auditoria da 105-rc-e

Revisão de 26/10/2026. A solução do projeto com makespan **116** é viável.
O pacote oficial ORLab de 2025 já reporta **112 como ótimo**, com limite 112.
A testemunha de 112 também foi validada; a prova do solver não foi reexecutada.

- `relatorio_105_rc_e.pdf`: relatório de 12 páginas, com rota, cronograma,
  capacidade, todas as missões e arcos, estatísticas e comparação atualizada.
- `relatorio_105_rc_e.md`: versão textual pesquisável.
- `verificar_105.py`: verificador independente com biblioteca padrão do Python;
  usa frações exatas e rejeita seis tipos de certificados corrompidos.
- `validacao.json`: cálculos, verificações, estatísticas e hashes das fontes.
- `solucao_116.json`, `solucao_orlab_112.json`, `solucao_historica_120.json`:
  certificados completos de viabilidade.
- `missoes_116.csv`, `ocupacao_116.csv`, `arcos_caminhao_116.csv` e
  `execucoes_100.csv`: dados integrais da análise.
- `fontes/`: entradas oficiais, resultados publicados, solução registrada e
  extrato congelado das 100 execuções originais.
- `proveniencia.json`: comparação dos dados com os pacotes oficiais.
- `reproduzir_105.c`, `reproducao.txt`, `compilador.txt`, `hashes_codigo.json`:
  reprodução no C e identificação dos arquivos de código examinados.
- `gerar_relatorio.py`: construtor do PDF, requer ReportLab e fontes Arial.

Para repetir a validação, execute `python verificar_105.py` neste diretório
ou `python resultados/105-rc-e/verificar_105.py` a partir da raiz do projeto.
O pacote contém todas as entradas necessárias para a validação independente.
Se estiver dentro do projeto, os hashes das entradas da raiz também são conferidos.

Para reproduzir a trajetória do SA, compile `reproduzir_105.c` com `cv.c`,
`lerInstancias.c`, `SA.c`, `-I.`, `-O2 -std=c11 -Wall -Wextra -pedantic -lm`,
e execute da raiz. A biblioteca C/implementação de `rand()` precisa corresponder
à usada originalmente para reproduzir exatamente a mesma trajetória.

Fonte atualizada:
https://orlab.com.vn/download/data-and-results-of-pdstsp-c-and-pdsop-c/

Caminho no pacote: `PDSOP-c/results.zip` -> `results/logs/tsp/105-rc-e.log`.
O log registra `status: OPTIMAL`, `Makespan: 112`, `Makespan Bound: 112`,
CP-SAT 9.12.4550, um caminhão, 12 drones e a execução de 17/04/2025.
O campo `objective: 1266496` é codificado; não representa minutos.

A referência de 120 em `main.c` e os arquivos do lote foram preservados como
registro da comparação com 2024. Sua interpretação como melhor atual e a
classificação desta instância como sem ótimo precisam ser atualizadas.
