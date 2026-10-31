# CP-SAT completo — 165-r-c, alvo 159

Status: **UNKNOWN**. Tempo de solver: 6570.557 s. Limite de busca: 3600 s; 8 workers.

Todas as atribuições elegíveis, rota, quantidades e horários foram decisões do modelo. Não foram fixados clientes a partir de soluções anteriores nem trechos da rota. Clientes fisicamente inelegíveis permanecem obrigatórios no caminhão.

Não foi encontrada uma solução dentro do alvo nem foi demonstrada sua impossibilidade. O encerramento pelo prazo mantém a questão aberta.

Limite inferior reportado pelo solver: 154. Interpretar junto ao status; não é uma solução.

Locais iguais são agregados somente no circuito do caminhão; sem serviço e com distância métrica, reunir visitas não aumenta custo. Cada cliente mantém sua decisão de recurso. Quantidades dominadas por outra de demanda e duração menores são eliminadas por dominância válida.

O modelo exportado, código congelado, hashes das entradas, log e resposta completa estão nesta pasta. A testemunha oficial foi validada antes da busca; novas soluções usam verificação independente com frações exatas.

## Ocorrência de tempo e limites de interpretação

O prazo configurado foi de 3.600 segundos, mas o solver registrou 6.570,557 segundos decorridos (cerca de 109,5 minutos). Houve um intervalo prolongado sem heartbeats; a causa não foi determinada. Não se deve interpretar esse valor como uma medição de tempo útil de CPU. O encerramento permaneceu UNKNOWN.

O campo objective: 159 da resposta UNKNOWN não é um certificado de solução: não foi encontrada solução viável dentro do alvo. O limite interno 154 é mais fraco que o limite publicado 156; o intervalo conhecido 156–160 não foi estreitado por esta execução. A solução oficial de 160 continua válida e sua otimalidade não foi demonstrada.

Modelo protobuf, fonte congelado e hashes das duas entradas foram conferidos após o encerramento; o modelo contém 165 variáveis de atribuição por cliente, um Circuit e um Cumulative.
