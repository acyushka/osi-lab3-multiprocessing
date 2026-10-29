#include "det.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int det_sequential(const Matrix *m) {
    if (m->n == 1) return (int)llround(m->a[0]);
    if (m->n == 2)
        return (int)llround(m->a[0] * m->a[3] - m->a[1] * m->a[2]);

    int n = m->n;
    long long sum = 0;
    for (int j = 0; j < n; ++j) {
        Matrix *sub = matrix_minor(m, 0, j);
        if (!sub) {
            fprintf(stderr, "matrix_minor failed\n");
            exit(1);
        }
        int minor_det = det_sequential(sub);
        matrix_free(sub);
        long long term = (long long)llround(m->a[j]) * minor_det;
        sum += (j % 2 == 0) ? term : -term;
    }
    return (int)sum;
}

typedef struct {
    const Matrix *m;
    int           j;
    long long     result;
    SyncCtx      *ctx;
    int           error;
} Task;

// Учёт активных под мьютексом. Возвращает текущее число активных
static LONG stats_enter(SyncCtx *c) {
    WaitForSingleObject(c->mtx_stats, INFINITE);
    LONG now = InterlockedIncrement(&c->active);
    if (now > c->peak_active) c->peak_active = now;
    ReleaseMutex(c->mtx_stats);
    return now;
}

static void stats_leave(SyncCtx *c) {
    WaitForSingleObject(c->mtx_stats, INFINITE);
    InterlockedDecrement(&c->active);
    ReleaseMutex(c->mtx_stats);
}

static DWORD WINAPI worker(LPVOID arg) {
    Task *t = (Task *)arg;
    SyncCtx *c = t->ctx;

    // ограничитель: одновременно не более max_threads
    if (WaitForSingleObject(c->sem, INFINITE) != WAIT_OBJECT_0) {
        t->error = 1;
        return 0;
    }

    LONG now = stats_enter(c);
    printf("  [thread TID=%lu] j=%d started (active=%ld)\n",
           GetCurrentThreadId(), t->j, now);

    Matrix *sub = matrix_minor(t->m, 0, t->j);
    if (!sub) {
        t->error = 1;
    } else {
        int sub_det = det_sequential(sub);
        matrix_free(sub);
        long long term = (long long)llround(t->m->a[t->j]) * sub_det;
        t->result = (t->j % 2 == 0) ? term : -term;
    }

    stats_leave(c);
    ReleaseSemaphore(c->sem, 1, NULL);
    return 0;
}

int det_parallel(const Matrix *m, SyncCtx *ctx) {
    if (m->n == 1) return (int)llround(m->a[0]);
    if (m->n == 2)
        return (int)llround(m->a[0] * m->a[3] - m->a[1] * m->a[2]);

    int n = m->n;
    Task   *tasks   = (Task   *)calloc((size_t)n, sizeof(*tasks));
    HANDLE *threads = (HANDLE *)calloc((size_t)n, sizeof(*threads));
    if (!tasks || !threads) {
        perror("calloc");
        free(tasks); free(threads);
        exit(1);
    }

    int created = 0;
    for (int j = 0; j < n; ++j) {
        tasks[j] = (Task){ .m = m, .j = j, .result = 0, .ctx = ctx, .error = 0 };
        threads[j] = CreateThread(NULL, 0, worker,
                                  &tasks[j], 0, NULL);
        if (!threads[j]) {
            print_error("CreateThread");
            for (int k = 0; k < created; ++k) {
                WaitForSingleObject(threads[k], INFINITE);
                CloseHandle(threads[k]);
            }
            free(tasks); free(threads);
            exit(1);
        }
        ++created;
        InterlockedIncrement(&ctx->total_spawned);
    }

    DWORD remaining = (DWORD)n, offset = 0;
    while (remaining > 0) {
        DWORD batch = remaining > MAXIMUM_WAIT_OBJECTS
                      ? MAXIMUM_WAIT_OBJECTS : remaining;
        DWORD w = WaitForMultipleObjects(batch, &threads[offset],
                                         TRUE, INFINITE);
        if (w == WAIT_FAILED) {
            print_error("WaitForMultipleObjects");
            for (DWORD k = 0; k < n; ++k) CloseHandle(threads[k]);
            free(tasks); free(threads);
            exit(1);
        }
        offset    += batch;
        remaining -= batch;
    }

    long long sum = 0;
    int had_error = 0;
    for (int j = 0; j < n; ++j) {
        if (tasks[j].error) had_error = 1;
        sum += tasks[j].result;
        CloseHandle(threads[j]);
    }
    free(tasks);
    free(threads);

    if (had_error) {
        fprintf(stderr, "one or more workers failed\n");
        exit(1);
    }
    return (int)sum;
}
