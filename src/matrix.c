#include "matrix.h"
#include <stdio.h>
#include <stdlib.h>

Matrix *matrix_create(int n) {
    Matrix *m = (Matrix *)malloc(sizeof(*m));
    if (!m) {
        perror("malloc(Matrix)");
        return NULL;
    }
    m->n = n;
    m->a = (double *)malloc((size_t)n * n * sizeof(*m->a));
    if (!m->a) {
        perror("malloc(data)");
        free(m);
        return NULL;
    }
    return m;
}

void matrix_free(Matrix *m) {
    if (!m) return;
    free(m->a);
    free(m);
}

void matrix_random_fill(Matrix *m, unsigned seed) {
    srand(seed);
    for (int i = 0; i < m->n * m->n; ++i)
        m->a[i] = (double)(rand() % 200 - 100) / 10.0;
}

void matrix_print(const Matrix *m) {
    for (int i = 0; i < m->n; ++i) {
        for (int j = 0; j < m->n; ++j)
            printf("%8.2f ", m->a[i * m->n + j]);
        printf("\n");
    }
}

Matrix *matrix_minor(const Matrix *m, int row, int col) {
    int n = m->n - 1;
    Matrix *sub = matrix_create(n);
    if (!sub) return NULL;

    int di = 0;
    for (int i = 0; i < m->n; ++i) {
        if (i == row) continue;
        int dj = 0;
        for (int j = 0; j < m->n; ++j) {
            if (j == col) continue;
            sub->a[di * n + dj] = m->a[i * m->n + j];
            ++dj;
        }
        ++di;
    }
    return sub;
}
