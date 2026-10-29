#ifndef DET_H
#define DET_H

#include <windows.h>
#include "matrix.h"

typedef struct {
    HANDLE sem;           /* ограничитель одновременных потоков */
    HANDLE mtx_stats;     /* мьютекс для счётчиков */
    LONG   active;        /* активных сейчас */
    LONG   peak_active;   /* пик активных */
    LONG   total_spawned; /* всего создано потоков */
} SyncCtx;

int det_sequential(const Matrix *m);
int det_parallel(const Matrix *m, SyncCtx *ctx);

#endif
