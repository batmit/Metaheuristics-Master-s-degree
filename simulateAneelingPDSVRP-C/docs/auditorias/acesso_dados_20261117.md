# Acesso aos arquivos experimentais — 17/11/2026

## Resultado

Os arquivos públicos utilizados pelo projeto estão acessíveis localmente. Nesta verificação, `python scripts/validate_instances.py` terminou com sucesso: 60 instâncias, 60 tabelas de tempos de drones e 4.316 clientes. O validador verifica a correspondência com o pacote preservado de 2025. Isso não identifica automaticamente as matrizes exatas usadas pelos autores nos experimentos de 2024.

Pacotes preservados em `instances/archives/`:

- `pdstsp-c-original.zip`: 59.165 bytes; benchmark original de Nguyen e Hà.
- `pdstsp-c-pdsop-c-2025.zip`: 21.699.000 bytes; dados e resultados de PDSTSP-c/PDSOP-c.

As entradas operacionais estão em `instances/benchmark/` e `instances/drone_times/`. A origem e o procedimento de validação estão em `instances/README.md`.

## Busca externa

Foram consultados o catálogo ORLab, páginas de download, registros dos artigos COR/MIC e buscas por PDSVRP-c associado a dados, GitHub e material suplementar. Não foi localizado um pacote público inequivocamente identificado como os arquivos exatos dos experimentos COR/MIC 2024. Algumas páginas do repositório institucional retornaram HTTP 403; portanto, a busca não prova inexistência de material adicional.

- Original: https://orlab.com.vn/download/instances-for-the-parallel-drone-scheduling-traveling-salesman-problem-with-collective-drones/
- Pacote de 2025: https://orlab.com.vn/download/data-and-results-of-pdstsp-c-and-pdsop-c/
- Catálogo: https://orlab.com.vn/materials/
- COR: https://www.sciencedirect.com/science/article/pii/S0305054823003787 — a declaração de disponibilidade indica dados mediante solicitação.
- Repositório COR: https://iris.unimore.it/handle/11380/1328966
- Repositório MIC: https://iris.unimo.it/handle/11380/1342368

O download ORLab chamado “Data and detailed results for the PDSVRP paper” identifica Nguyen, Dang, Hà e Pham e foi criado em 2021. Corresponde a outra linha de trabalho (min-cost PDSVRP), não constitui evidência de obtenção do pacote PDSVRP-c de Montemanni, Dell'Amico e Corsini.

## O que falta para encerrar a comparação

Obter dos autores as instâncias e matrizes tau efetivamente lidas nos experimentos COR/MIC 2024, parâmetros e regras de arredondamento, além dos certificados/logs das melhores soluções, particularmente 126-rc-e. Comparar hashes, coordenadas, velocidades, unidades e tempos com nossos arquivos. Nenhuma mensagem foi enviada aos autores nesta verificação.

Os 14 certificados locais anteriormente validados continuam sendo resultados sobre os dados disponíveis. Acesso aos arquivos locais não encerra a confirmação de equivalência experimental com a literatura nem a verificação de recordes atuais.
