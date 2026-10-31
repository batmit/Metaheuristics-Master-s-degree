# CP-SAT completo — 165-r-c, alvo 160

Status: **FEASIBLE**. Tempo de solver: 12.955 s. Limite de busca: 30 s; 8 workers.

Todas as atribuições elegíveis, rota, quantidades e horários foram decisões do modelo. Não foram fixados clientes a partir de soluções anteriores nem trechos da rota. Clientes fisicamente inelegíveis permanecem obrigatórios no caminhão.

Foi encontrada e validada independentemente uma solução com makespan **160**, caminhão 160 e drones 157.

Limite inferior reportado pelo solver: 0. Interpretar junto ao status; não é uma solução.

Locais iguais são agregados somente no circuito do caminhão; sem serviço e com distância métrica, reunir visitas não aumenta custo. Cada cliente mantém sua decisão de recurso. Quantidades dominadas por outra de demanda e duração menores são eliminadas por dominância válida.

O modelo exportado, código congelado, hashes das entradas, log e resposta completa estão nesta pasta. A testemunha oficial foi validada antes da busca; novas soluções usam verificação independente com frações exatas.
