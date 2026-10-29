#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "common.h"
#include "matrix.h"
#include "det.h"

static void usage(const char *prog) {
    fprintf(stderr,
            "Usage: %s -t <max_threads> -n <size> [-s <seed>] [-p]\n"
            "  -t N   max simultaneously running threads (N > 0)\n"
            "  -n N   matrix size (N >= 2)\n"
            "  -s N   random seed (default 42)\n"
            "  -p     print matrix\n", prog);
}

static int parse_int(const char *s, const char *what, long min, long max) {
    errno = 0;
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (errno || !end || *end != '\0' || v < min || v > max) {
        fprintf(stderr, "invalid %s: '%s' (expected %ld..%ld)\n",
                what, s, min, max);
        exit(1);
    }
    return (int)v;
}

int main(int argc, char **argv) {
    int      max_threads = 1;
    int      n           = 4;
    unsigned seed        = 42;
    int      print_mat   = 0;

    for (int i = 1; i < argc; ++i) {
        if      (!strcmp(argv[i], "-t") && i + 1 < argc) max_threads = parse_int(argv[++i], "max_threads", 1, 1024);
        else if (!strcmp(argv[i], "-n") && i + 1 < argc) n           = parse_int(argv[++i], "size",        2, 15);
        else if (!strcmp(argv[i], "-s") && i + 1 < argc) seed        = (unsigned)parse_int(argv[++i], "seed", 0, 0x7fffffff);
        else if (!strcmp(argv[i], "-p"))                 print_mat   = 1;
        else { usage(argv[0]); return 1; }
    }

    printf("Process PID = %lu\n", GetCurrentProcessId());
    printf("Matrix size = %d, max_threads = %d, seed = %u\n\n",
           n, max_threads, seed);

    Matrix *m = matrix_create(n);
    if (!m) return 1;
    matrix_random_fill(m, seed);
    if (print_mat) { matrix_print(m); printf("\n"); }

    LARGE_INTEGER f, t0, t1;
    if (!QueryPerformanceFrequency(&f)) {
        print_error("QueryPerformanceFrequency");
        matrix_free(m);
        return 1;
    }

    QueryPerformanceCounter(&t0);
    int det_seq = det_sequential(m);
    QueryPerformanceCounter(&t1);
    double t_seq = (double)(t1.QuadPart - t0.QuadPart) / (double)f.QuadPart;

    SyncCtx ctx = {0};
    ctx.sem       = CreateSemaphoreA(NULL, max_threads,
                                     max_threads, NULL);
    ctx.mtx_stats = CreateMutexA(NULL, FALSE, NULL);
    if (!ctx.sem || !ctx.mtx_stats) {
        print_error("CreateSemaphore/Mutex");
        if (ctx.sem)       CloseHandle(ctx.sem);
        if (ctx.mtx_stats) CloseHandle(ctx.mtx_stats);
        matrix_free(m);
        return 1;
    }

    QueryPerformanceCounter(&t0);
    int det_par = det_parallel(m, &ctx);
    QueryPerformanceCounter(&t1);
    double t_par = (double)(t1.QuadPart - t0.QuadPart) / (double)f.QuadPart;

    printf("\ndet (sequential) = %d\n", det_seq);
    printf("det (parallel)   = %d\n", det_par);
    printf("match: %s\n", det_seq == det_par ? "YES" : "NO");

    printf("\nTiming:\n");
    printf("  sequential: %.6f s\n", t_seq);
    printf("  parallel:   %.6f s\n", t_par);
    printf("  speedup:    %.2fx\n", t_par > 0 ? t_seq / t_par : 0.0);

    printf("\nThreads:\n");
    printf("  max_threads (key): %d\n",  max_threads);
    printf("  total_spawned:     %ld\n", (long)ctx.total_spawned);
    printf("  peak_active:       %ld\n", (long)ctx.peak_active);

    CloseHandle(ctx.sem);
    CloseHandle(ctx.mtx_stats);
    matrix_free(m);
    return 0;
}
