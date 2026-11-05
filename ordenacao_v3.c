// Versao 3: Counting Sort de 32 bits com preenchimento em blocos.
// A leitura, a alocacao inicial e a verificacao nao entram no cronometro.
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static void falha(const char *mensagem) {
    fprintf(stderr, "Erro: %s\n", mensagem);
    exit(EXIT_FAILURE);
}

static int32_t *ler(const char *nome, size_t *n) {
    FILE *f = fopen(nome, "rb");
    if (!f) falha("arquivo nao encontrado");
    size_t capacidade = 1u << 20;
    int32_t *a = malloc(capacidade * sizeof(*a));
    if (!a) falha("memoria insuficiente");
    *n = 0;
    int64_t numero;
    while (fscanf(f, " %" SCNd64, &numero) == 1) {
        if (numero < INT32_MIN || numero > INT32_MAX)
            falha("numero fora de 32 bits: use ordenacao.c");
        if (*n == capacidade) {
            if (capacidade > SIZE_MAX / (2 * sizeof(*a))) falha("arquivo grande demais");
            capacidade *= 2;
            int32_t *novo = realloc(a, capacidade * sizeof(*a));
            if (!novo) falha("memoria insuficiente");
            a = novo;
        }
        a[(*n)++] = (int32_t)numero;
    }
    if (ferror(f)) falha("falha na leitura");
    int c;
    while ((c = fgetc(f)) != EOF)
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n' && c != '\v' && c != '\f')
            falha("formato do arquivo invalido");
    fclose(f);
    if (!*n) falha("arquivo vazio");
    return a;
}

static int cmp(const void *a, const void *b) {
    int32_t x = *(const int32_t *)a, y = *(const int32_t *)b;
    return (x > y) - (x < y);
}

static int ordenar_v3(int32_t *a, size_t n) {
    int32_t minimo = a[0], maximo = a[0];
    for (size_t i = 1; i < n; ++i) {
        int32_t x = a[i];
        if (x < minimo) minimo = x;
        if (x > maximo) maximo = x;
    }

    uint64_t largura = (uint64_t)((int64_t)maximo - minimo) + 1;
    if (largura > n || largura > SIZE_MAX / sizeof(uint32_t) || n > UINT32_MAX) {
        qsort(a, n, sizeof(*a), cmp);
        return 0;
    }
    uint32_t *freq = calloc((size_t)largura, sizeof(*freq));
    if (!freq) falha("sem memoria para as frequencias");

    const uint32_t base = (uint32_t)minimo;
#ifdef CONTAGEM_EM_BLOCOS
    size_t i = 0;
    for (; i + 3 < n; i += 4) {
        ++freq[(uint32_t)a[i] - base];
        ++freq[(uint32_t)a[i + 1] - base];
        ++freq[(uint32_t)a[i + 2] - base];
        ++freq[(uint32_t)a[i + 3] - base];
    }
    for (; i < n; ++i) ++freq[(uint32_t)a[i] - base];
#else
    for (size_t i = 0; i < n; ++i) ++freq[(uint32_t)a[i] - base];
#endif

    size_t pos = 0;
    for (size_t i = 0; i < (size_t)largura; ++i) {
        uint32_t quantidade = freq[i];
        if (!quantidade) continue;
        int32_t valor = (int32_t)((int64_t)minimo + (int64_t)i);
        // Reduz o custo do controle de loop para sequencias repetidas.
        while (quantidade >= 8) {
            a[pos] = valor; a[pos + 1] = valor;
            a[pos + 2] = valor; a[pos + 3] = valor;
            a[pos + 4] = valor; a[pos + 5] = valor;
            a[pos + 6] = valor; a[pos + 7] = valor;
            pos += 8;
            quantidade -= 8;
        }
        while (quantidade--) a[pos++] = valor;
    }
    free(freq);
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s arquivo.txt\n", argv[0]);
        return EXIT_FAILURE;
    }
    size_t n;
    int32_t *a = ler(argv[1], &n);
    clock_t inicio = clock();
    int contando = ordenar_v3(a, n);
    clock_t fim = clock();
    if (inicio == (clock_t)-1 || fim == (clock_t)-1) falha("clock indisponivel");

    for (size_t i = 1; i < n; ++i)
        if (a[i - 1] > a[i]) falha("ordenacao incorreta");
    double segundos = (double)(fim - inicio) / CLOCKS_PER_SEC;
    printf("Elementos: %zu\n", n);
#ifdef CONTAGEM_EM_BLOCOS
    printf("Algoritmo: %s (versao 4)\n", contando ? "Counting Sort" : "qsort");
#else
    printf("Algoritmo: %s (versao 3)\n", contando ? "Counting Sort" : "qsort");
#endif
    printf("Tempo de ordenacao: %.6f segundos (%.3f ms)\n", segundos, segundos * 1000.0);
    printf("Verificacao: OK (ordem nao decrescente)\n");
    free(a);
    return EXIT_SUCCESS;
}
