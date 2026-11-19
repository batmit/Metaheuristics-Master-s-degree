#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <wchar.h>
#else
#include <sys/stat.h>
#endif

/* Parametros do lote: uma busca por instancia, sem argumentos de terminal. */
#ifndef SA_SEGUNDOS_POR_INSTANCIA
#define SA_SEGUNDOS_POR_INSTANCIA 30.0
#endif
#ifndef MOSTRAR_RECORDES
#define MOSTRAR_RECORDES 0
#endif
#include "SA.h"
#include "lerInstancias.h"

typedef struct CasoBenchmark {
    const char *nome;
    const char *pasta;
    double literatura2024;
    double literatura2025;
    double segundosLiteratura2025;
} CasoBenchmark;

/* Referencias UB de 2024 e 2025 da revisao local das 60 instancias.
 * Referencias e proveniencia: instances/pdstsp-c/referencias.csv.
 * A pasta segue o catalogo oficial de entradas. */
static const CasoBenchmark casos[] = {
    {"15-r-e", "small", 92.0, 92.0, 0.0900199},
    {"15-rc-c", "small", 44.0, 44.0, 0.461427},
    {"16-c-c", "small", 60.0, 60.0, 0.0470937},
    {"16-r-e", "small", 112.0, 112.0, 0.0639521},
    {"18-c-c", "small", 56.0, 56.0, 0.271486},
    {"18-r-e", "small", 96.0, 96.0, 0.235289},
    {"18-rc-c", "small", 57.0, 57.0, 0.966265},
    {"19-c-c", "small", 44.0, 44.0, 0.867991},
    {"20-c-c", "small", 43.0, 43.0, 0.170847},
    {"20-r-c", "small", 64.0, 64.0, 0.413625},
    {"20-r-e", "small", 80.0, 80.0, 0.291596},
    {"20-rc-c", "small", 96.0, 96.0, 0.056689},
    {"20-rc-e", "small", 100.0, 100.0, 0.0759249},
    {"21-c-c", "small", 62.0, 62.0, 14.2166},
    {"21-r-e", "small", 85.0, 85.0, 22.9676},
    {"23-c-e", "small", 80.0, 80.0, 0.0555869},
    {"23-r-c", "small", 88.0, 88.0, 0.43017},
    {"24-c-e", "small", 84.0, 84.0, 0.0786485},
    {"24-r-e", "small", 112.0, 112.0, 1.7949},
    {"24-rc-c", "small", 70.0, 70.0, 0.308217},
    {"25-c-c", "small", 56.0, 56.0, 0.630344},
    {"25-r-e", "small", 104.0, 104.0, 2.43328},
    {"25-rc-e", "small", 92.0, 92.0, 1.57797},
    {"26-r-c", "small", 103.0, 103.0, 9.1018},
    {"27-c-c", "small", 84.0, 84.0, 0.205662},
    {"27-c-e", "small", 68.0, 68.0, 1.61391},
    {"27-rc-c", "small", 100.0, 100.0, 0.136626},
    {"27-rc-e", "small", 84.0, 84.0, 5.72948},
    {"29-rc-e", "small", 116.0, 116.0, 0.892377},
    {"30-c-c", "small", 96.0, 96.0, 0.108423},
    {"50-r-e", "large", 116.0, 116.0, 41.2642},
    {"53-r-e", "large", 132.0, 132.0, 11.4045},
    {"66-rc-e", "large", 124.0, 124.0, 5.70918},
    {"67-c-c", "large", 76.0, 74.0, 3600.04},
    {"68-rc-c", "large", 76.0, 76.0, 11.6876},
    {"76-c-c", "large", 52.0, 52.0, 7.46216},
    {"82-c-e", "large", 64.0, 64.0, 3.21081},
    {"82-rc-c", "large", 104.0, 104.0, 17.9718},
    {"88-c-e", "large", 108.0, 108.0, 2.55103},
    {"91-r-c", "large", 124.0, 120.0, 6.75553},
    {"99-rc-c", "large", 100.0, 99.0, 636.502},
    {"101-r-c", "large", 120.0, 116.0, 41.2618},
    {"103-rc-c", "large", 124.0, 120.0, 101.102},
    {"105-rc-e", "large", 120.0, 112.0, 125.257},
    {"108-rc-e", "large", 136.0, 135.0, 162.858},
    {"114-rc-c", "large", 96.0, 96.0, 769.218},
    {"121-rc-e", "large", 124.0, 121.0, 808.557},
    {"126-r-c", "large", 160.0, 152.0, 141.049},
    {"126-rc-e", "large", 144.0, 136.0, 232.423},
    {"144-rc-c", "large", 128.0, 124.0, 132.403},
    {"154-c-c", "large", 72.0, 68.0, 27.2542},
    {"165-r-c", "large", 164.0, 160.0, 3600.1},
    {"167-r-e", "large", 188.0, 180.0, 3600.11},
    {"173-r-c", "large", 164.0, 160.0, 1356.87},
    {"173-rc-c", "large", 133.0, 130.0, 1685.77},
    {"181-r-e", "large", 224.0, 216.0, 3600.19},
    {"185-c-c", "large", 96.0, 96.0, 24.9943},
    {"187-rc-e", "large", 196.0, 188.0, 114.949},
    {"198-c-c", "large", 64.0, 64.0, 48.5712},
    {"200-r-e", "large", 212.0, 200.0, 543.184},
};

/* Carrega uma instancia e suas matrizes uma unica vez por lote. */
static int carregarCaso(const CasoBenchmark *caso, Instancia *inst)
{
    char arquivo[256];
    snprintf(arquivo, sizeof arquivo, "instances/pdstsp-c/%s/%s.txt", caso->pasta, caso->nome);
    if (!lerInstancia(arquivo, inst)) return 0;
    alocarInstancias(inst);
    if (!inst->tempoCaminhao || !inst->tempoDrone) return 0;
    for (int i = 0; i < inst->n; i++)
        if (!inst->tempoCaminhao[i] || !inst->tempoDrone[i]) return 0;
    construirMatrizCaminhao(inst);
    snprintf(arquivo, sizeof arquivo, "instances/pdstsp-c/%s/tau_kj_%s.txt", caso->pasta, caso->nome);
    return lerTemposDrone(arquivo, inst);
}

/* Constroi e resolve uma solucao, liberando seus vetores ao terminar. */
static int resolverCaso(Instancia *inst, const ParametrosSA *configuracao, double *resultado)
{
    Solucao solucao = {0};
    alocarSolucao(inst, &solucao);
    int ok = solucao.missaoCaminhao && solucao.missaoDrone && solucao.prefixosDrone;
    double temperatura;
    if (ok) ok = construirSolucaoInicial(inst, &solucao) &&
                 resolverComCalibracaoSA(inst, &solucao, configuracao, &temperatura);
    if (ok) *resultado = solucao.makespan;
    liberarSolucao(&solucao);
    return ok;
}

#define RELATORIO "resultados/relatorio.csv"
#define TOTAL_CASOS (sizeof casos / sizeof casos[0])
static const char cabecalho[] = "instancia,melhor_algoritmo,literatura_2024,literatura_2025,gap_percentual,tempo_ultima_execucao_s,tempo_literatura_2025_s\n";

/* Recupera os melhores, aceitando UTF-8 com BOM e finais de linha LF/CRLF. */
static int lerRelatorio(double melhores[TOTAL_CASOS], double tempos[TOTAL_CASOS])
{
    FILE *f = fopen(RELATORIO, "rb");
    if (!f) {
        if (errno == ENOENT) return 1;
        perror("Nao foi possivel abrir " RELATORIO);
        return 0;
    }
    char linha[512], nome[64];
    if (!fgets(linha, sizeof linha, f)) {
        /* Arquivo vazio ainda nao contem resultados; sera inicializado no lote. */
        int ok = !ferror(f);
        if (!ok) perror("Erro de leitura em " RELATORIO);
        if (fclose(f)) ok = 0;
        return ok;
    }
    char *titulo = linha;
    if (strncmp(titulo, "\xEF\xBB\xBF", 3) == 0) titulo += 3;
    titulo[strcspn(titulo, "\r\n")] = '\0';
    size_t esperado = strcspn(cabecalho, "\r\n");
    int antigo = !strcmp(titulo, "instancia,melhor_algoritmo,literatura_2024,literatura_2025,gap_percentual");
    if (!antigo && (strlen(titulo) != esperado || strncmp(titulo, cabecalho, esperado))) {
        fprintf(stderr, "Cabecalho invalido em " RELATORIO
                ". Esperado: %s", cabecalho);
        fclose(f);
        return 0;
    }
    while (fgets(linha, sizeof linha, f)) {
        double valor;
        if (sscanf(linha, "%63[^,],%lf", nome, &valor) != 2 || !isfinite(valor) || valor <= 0) continue;
        for (size_t c = 0; c < TOTAL_CASOS; c++)
            if (!strcmp(nome, casos[c].nome)) {
                if (valor < melhores[c]) melhores[c] = valor;
                double tempo;
                if (!antigo && sscanf(linha, "%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%lf", &tempo) == 1 &&
                    isfinite(tempo) && tempo >= 0) tempos[c] = tempo;
            }
    }
    int ok = !ferror(f);
    if (!ok) perror("Erro de leitura em " RELATORIO);
    if (fclose(f)) ok = 0;
    return ok;
}

/* Grava uma linha no CSV; vazio indica ausencia de resultado. */
static void escreverLinha(FILE *f, size_t c, double melhor, double tempo)
{
    const CasoBenchmark *caso = &casos[c];
    if (isfinite(melhor))
        fprintf(f, "%s,%.9f,%.9f,%.9f,%.9f", caso->nome, melhor,
                caso->literatura2024, caso->literatura2025,
                100.0 * (melhor - caso->literatura2025) / caso->literatura2025);
    else fprintf(f, "%s,,%.9f,%.9f,", caso->nome, caso->literatura2024, caso->literatura2025);
    if (isfinite(tempo)) fprintf(f, ",%.9f", tempo);
    else fputc(',', f);
    fprintf(f, ",%.9f\n", caso->segundosLiteratura2025);
}

/* Consolida uma linha por instancia preservando os melhores anteriores. */
static int salvarRelatorio(const double melhores[TOTAL_CASOS], const double tempos[TOTAL_CASOS])
{
    FILE *f = fopen(RELATORIO, "w");
    if (!f) return 0;
    fputs(cabecalho, f);
    for (size_t c = 0; c < TOTAL_CASOS; c++) escreverLinha(f, c, melhores[c], tempos[c]);
    int ok = !ferror(f);
    if (fclose(f)) ok = 0;
    return ok;
}

/* Localiza as entradas ao abrir por duplo clique e cria a pasta de saida. */
static int prepararDiretorio(void)
{
#ifdef _WIN32
    /* Caminho UTF-16 suporta nomes de usuario e pastas com acentos. */
    wchar_t caminho[32768];
    DWORD tamanho = GetModuleFileNameW(NULL, caminho, 32768);
    if (!tamanho || tamanho >= 32768) return 0;
    wchar_t *barra = wcsrchr(caminho, L'\\');
    if (!barra) return 0;
    *barra = L'\0';
    if (!SetCurrentDirectoryW(caminho)) return 0;
    return _mkdir("resultados") == 0 || errno == EEXIST;
#else
    return mkdir("resultados", 0755) == 0 || errno == EEXIST;
#endif
}

/* Mostra somente os campos do relatorio em colunas alinhadas. */
static void mostrarResultado(size_t c, double melhor, double tempo)
{
    const CasoBenchmark *caso = &casos[c];
    if (isfinite(melhor))
        printf("%-12s %18.2f %16.2f %16.2f %12.2f", caso->nome, melhor,
               caso->literatura2024, caso->literatura2025,
               100.0 * (melhor - caso->literatura2025) / caso->literatura2025);
    else printf("%-12s %18s %16.2f %16.2f %12s", caso->nome, "--",
                caso->literatura2024, caso->literatura2025, "--");
    if (isfinite(tempo)) printf(" %18.6f", tempo);
    else printf(" %18s", "--");
    printf(" %20.6f\n", caso->segundosLiteratura2025);
    fflush(stdout);
}

/* Relogio de parede; inclui carregamento, construcao, calibracao e busca. */
static double segundosAgora(void)
{
#ifdef _WIN32
    LARGE_INTEGER contador, frequencia;
    QueryPerformanceFrequency(&frequencia);
    QueryPerformanceCounter(&contador);
    return (double)contador.QuadPart / (double)frequencia.QuadPart;
#else
    struct timespec agora;
    timespec_get(&agora, TIME_UTC);
    return (double)agora.tv_sec + (double)agora.tv_nsec / 1e9;
#endif
}

/* Executa exatamente uma busca para cada uma das 60 instancias. */
static int executarLote(void)
{
    if (!prepararDiretorio()) {
        perror("Falha ao preparar diretorio do executavel");
        return EXIT_FAILURE;
    }
    ParametrosSA configuracao = parametrosSARapido();
    configuracao.limiteSegundos = SA_SEGUNDOS_POR_INSTANCIA;
    unsigned int semente = (unsigned int)time(NULL);
    double melhores[TOTAL_CASOS], tempos[TOTAL_CASOS];
    for (size_t c = 0; c < TOTAL_CASOS; c++) { melhores[c] = INFINITY; tempos[c] = NAN; }
    if (!lerRelatorio(melhores, tempos)) {
        fprintf(stderr, "Falha ao ler " RELATORIO ".\n");
        return EXIT_FAILURE;
    }
    int sucesso = 1;
    printf("%-12s %18s %16s %16s %12s %18s %20s\n", "Instancia", "Resultado atual",
           "Literatura 2024", "Literatura 2025", "Gap (%)", "Ultima execucao (s)", "Tempo lit. 2025 (s)");
    fflush(stdout);
    for (size_t c = 0; c < TOTAL_CASOS; c++) {
        Instancia inst = {0};
        double resultado = INFINITY;
        double inicio = segundosAgora();
        srand(semente + (unsigned int)c);
        if (!carregarCaso(&casos[c], &inst)) {
            fprintf(stderr, "Falha ao carregar %s.\n", casos[c].nome);
            sucesso = 0;
        } else if (!resolverCaso(&inst, &configuracao, &resultado)) {
            resultado = INFINITY;
            fprintf(stderr, "Falha ao resolver %s.\n", casos[c].nome);
            sucesso = 0;
        } else if (resultado < melhores[c]) {
            /* Somente uma melhora substitui o recorde persistido da instancia. */
            melhores[c] = resultado;
        }
        tempos[c] = segundosAgora() - inicio;
        liberarInstancia(&inst);
        if (!salvarRelatorio(melhores, tempos)) {
            perror("Falha ao salvar " RELATORIO);
            return EXIT_FAILURE;
        }
        mostrarResultado(c, resultado, tempos[c]);
        fflush(stdout);
    }
    return sucesso ? EXIT_SUCCESS : EXIT_FAILURE;
}

/* Consulta os recordes persistidos sem executar buscas nem alterar o CSV. */
static int mostrarRecordes(void)
{
    if (!prepararDiretorio()) {
        perror("Falha ao preparar diretorio do executavel");
        return EXIT_FAILURE;
    }
    double melhores[TOTAL_CASOS], tempos[TOTAL_CASOS];
    for (size_t c = 0; c < TOTAL_CASOS; c++) {
        melhores[c] = INFINITY;
        tempos[c] = NAN;
    }
    if (!lerRelatorio(melhores, tempos)) return EXIT_FAILURE;
    printf("%-12s %18s %16s %16s %12s %18s %20s\n", "Instancia", "Melhor algoritmo",
           "Literatura 2024", "Literatura 2025", "Gap (%)", "Ultima execucao (s)", "Tempo lit. 2025 (s)");
    for (size_t c = 0; c < TOTAL_CASOS; c++) mostrarResultado(c, melhores[c], tempos[c]);
    return EXIT_SUCCESS;
}

/* Ao abrir com duplo clique, conserva a tabela na janela ate pressionar Enter. */
int main(void)
{
    int resultado = MOSTRAR_RECORDES ? mostrarRecordes() : executarLote();
#ifdef _WIN32
    DWORD processos[2];
    if (GetConsoleProcessList(processos, 2) == 1) {
        fprintf(stderr, "\nPressione Enter para fechar.\n");
        (void)getchar();
    }
#endif
    return resultado;
}
