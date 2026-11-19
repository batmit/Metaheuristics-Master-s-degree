# Revisao de execucao e gaps — 07/11/2026

Revisados: lancador/worker `main.c`, leitura `lerInstancias.c`, representacao,
construcao e vizinhancas `cv.c`, calibracao e busca `SA.c`, escalonamento e
podas `drone_bb.c`, executor Python, catalogo de referencias e testes.
Os certificados existentes foram recalculados sem alterar `results/best.json`.

## Gravacao

O traceback aponta falha de permissao em `os.replace`, e nao falha numerica
do SA. O caminho esta em um volume Windows sincronizado com OneDrive;
nao e possivel determinar pelo traceback qual processo/permissao bloqueou
a substituicao. A versao anterior fazia uma unica tentativa e repetia a
mesma gravacao no `finally`, mascarando a excecao original.

A gravacao agora usa temporario exclusivo no mesmo diretorio, fsync e
substituicao atomica, com oito tentativas e espera crescente para
PermissionError. Falha persistente preserva o registro anterior e deixa
o novo JSON completo no temporario informado no erro. O encerramento nao
repete uma gravacao que ja falhou. `--results` permite usar armazenamento
Linux no WSL. Nao foi feita substituicao direta destrutiva do arquivo antigo.

## Gap e incompatibilidade comprovada

A formula percentual esta correta: `100*(encontrado-UB)/UB`.
56 contra 44 resulta em 27,2727%, nao em 12% nem 0,2727%.

Para cada cliente j, qualquer solucao satisfaz:

`makespan >= min(2*distancia_Manhattan(deposito,j)/velocidade*60, min_k tau[j,k])`.

O minimo de um conjunto de missoes inviaveis e infinito. A desigualdade
triangular garante o limite da rota de caminhao, independentemente da
quantidade de outros clientes ou caminhoes. O maior desses limites tambem
e um limite inferior valido para a instancia.

Na 126-rc-e, o deposito e (0,0), o cliente 46 e (14,14), a velocidade e
30 km/h, e todas as 12 entradas de drones desse cliente sao -1. Logo,
qualquer solucao com os dados locais exige ao menos 112 minutos.

| Caminhoes | UB de referencia | Limite local | Comparacao |
| --- | ---: | ---: | --- |
| 2 | 152 | 112 | Sem contradicao por este limite |
| 3 | 100 | 112 | Incompativel |
| 4 | 84 | 112 | Incompativel |
| 5 | 76 | 112 | Incompativel |

Estas sao as tres contradicoes encontradas nas 180 configuracoes.
O executor agora mostra N/C para elas, preserva os valores publicados e
registra limite/cliente em memoria e nas proximas gravacoes do JSON.
Ausencia de contradicao nao demonstra identidade dos dados dos artigos.
Nao foi alterada nenhuma matriz para aproximar artificialmente os BKS.

## Experimentos isolados

Worker Windows existente, semente 42, sem limite externo, certificados
conferidos pelo validador Python. Resultados em `output/auditoria_gap.json`.

| Instancia | S | Inicial | Melhor | Referencia | Interpretacao |
| --- | ---: | ---: | ---: | ---: | --- |
| 68-rc-c | 5 | 76 | 52 | 44 | Gap real de 18,18% para a referencia |
| 126-rc-e | 5 | 112 | 112 | 76 | Otimo para os dados locais pelo limite 112; referencia incompativel |
| 200-r-e | 2 | 208 | 208 | 216 | Diferenca de -3,70%; nao e prova de novo recorde com dados identicos |

As primeiras linhas ao vivo sao solucoes parciais, inclusive a construcao
inicial somente com caminhoes. A calibracao nao publica seus vizinhos;
o SA usa movimentos aleatorios e o BB so refina ordem/horarios com k fixo.
Nao ha garantia de atingir a melhor referencia. Os testes de viabilidade
e memoria nao demonstram qualidade otima de uma metaheuristica.

## Verificacoes

27 testes Python passaram no Windows. No WSL, ASan/UBSan passou nos
oraculos de BB, gatilho apos cinco tentativas e trajetorias SA de instancias
pequena, media e grande. A verificacao integrada confere compilacao estrita,
90.000 vizinhos nas 180 configuracoes, integridade das 60 entradas/matrizes,
certificados existentes e uma execucao de um segundo com registro isolado.
O resumo da verificacao integrada fica em `output/verificacao.json`.

Os testes novos cobrem bloqueio transitorio, bloqueio persistente com copia
recuperavel, preservacao da excecao original, limite 112, N/C nas tres
configuracoes e preservacao de gaps numericos validos.
