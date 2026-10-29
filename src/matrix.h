#ifndef MATRIX_H
#define MATRIX_H

typedef struct {
    int     n;
    double *a;   /* a[i*n + j] */
} Matrix;

Matrix *matrix_create(int n);
void    matrix_free(Matrix *m);
void    matrix_random_fill(Matrix *m, unsigned seed);
void    matrix_print(const Matrix *m);

Matrix *matrix_minor(const Matrix *m, int row, int col);

#endif