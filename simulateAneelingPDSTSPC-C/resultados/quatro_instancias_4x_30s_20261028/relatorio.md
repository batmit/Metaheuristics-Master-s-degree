# Quatro instâncias — quatro execuções de 30 segundos cada

Cada execução parte de nova construção inicial. Perfil novo com reparos e CP-SAT na 165-r-c; exploração atual nas outras três. Três processos concorrentes, fontes e parâmetros fixos, sementes e certificados preservados nesta pasta.

| Instância | Execução 1 | Execução 2 | Execução 3 | Execução 4 | Melhor | Média | Referência |
|---|---:|---:|---:|---:|---:|---:|---:|
| 67-c-c | 76 | 76 | 76 | 76 | 76 | 76.00 | 74 |
| 165-r-c | 168 | 168 | 164 | 168 | 164 | 167.00 | 160 |
| 167-r-e | 192 | 184 | 200 | 188 | 184 | 191.00 | 180 |
| 181-r-e | 228 | 224 | 228 | 228 | 224 | 227.00 | 216 |

Os 16 certificados passaram pela verificação independente com frações exatas: cobertura, percurso e retorno, durações da matriz, capacidade simultânea e atribuição física dos drones. A construção inicial fica fora do limite; operações em andamento podem causar pequenos excessos.

CP-SAT: 8 chamadas, 0 reparos bem-sucedidos, 1 vizinhanças inviáveis, 7 encerramentos pelo prazo e 0 erros. Inviabilidade é local à vizinhança; prazo não prova inviabilidade.

Execuções acima de 35 segundos decorridos: 0. Maior tempo observado: 30.046 s.
