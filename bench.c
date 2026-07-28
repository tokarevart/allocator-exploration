#include <assert.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <time.h>

typedef struct {
    size_t vmsize;
    size_t vmrss;
} mem_stats_t;

static void
get_mem_stats(mem_stats_t *s) {
    memset(s, 0, sizeof(*s));

    FILE *f = fopen("/proc/self/status", "r");
    if (!f) {
        perror("fopen /proc/self/status");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        size_t val = 0;
        if (sscanf(line, "VmSize: %zu kB", &val) == 1) {
            s->vmsize = val;
        } else if (sscanf(line, "VmRSS: %zu kB", &val) == 1) {
            s->vmrss = val;
        }
    }
    fclose(f);
}

static void
print_mem_stats(const mem_stats_t *s, const mem_stats_t *prev, size_t count) {
    ssize_t dv = prev ? (ssize_t)s->vmsize - (ssize_t)prev->vmsize : 0;
    ssize_t dr = prev ? (ssize_t)s->vmrss - (ssize_t)prev->vmrss : 0;

    if (prev && count) {
        double pv = (double)dv / count * 1024;
        double pr = (double)dr / count * 1024;
        if (pv < 1024.0) {
            printf("  VmSize: %8zu kB  (%+zd, %+.0f B/alloc)\n", s->vmsize, dv, pv);
        } else {
            printf("  VmSize: %8zu kB  (%+zd, %+.1f kB/alloc)\n", s->vmsize, dv, pv / 1024);
        }
        if (pr < 1024.0) {
            printf("  VmRSS:  %8zu kB  (%+zd, %+.0f B/alloc)\n", s->vmrss, dr, pr);
        } else {
            printf("  VmRSS:  %8zu kB  (%+zd, %+.1f kB/alloc)\n", s->vmrss, dr, pr / 1024);
        }
    } else if (prev) {
        printf("  VmSize: %8zu kB  (%+zd)\n", s->vmsize, dv);
        printf("  VmRSS:  %8zu kB  (%+zd)\n", s->vmrss, dr);
    } else {
        printf("  VmSize: %8zu kB\n", s->vmsize);
        printf("  VmRSS:  %8zu kB\n", s->vmrss);
    }
}

static void
print_per_alloc(const char *label, double bytes) {
    constexpr double KB = 1024.0;
    constexpr double MB = KB * 1024.0;
    constexpr double GB = MB * 1024.0;

    if (bytes < KB) {
        printf("  %s: %.0f B/alloc\n", label, bytes);
    } else if (bytes < MB) {
        printf("  %s: %.2f KB/alloc\n", label, bytes / KB);
    } else if (bytes < GB) {
        printf("  %s: %.2f MB/alloc\n", label, bytes / MB);
    } else {
        printf("  %s: %.2f GB/alloc\n", label, bytes / GB);
    }
}

static double
elapsed_ms(struct timespec *start, struct timespec *end) {
    return (end->tv_sec - start->tv_sec) * 1000.0 + (end->tv_nsec - start->tv_nsec) / 1e6;
}

typedef struct {
    double elapsed_ms;
    double per_alloc_ns;
    size_t alloc_count;
} bench_stats_t;

typedef struct {
    mtx_t mtx;
    cnd_t cnd;
    size_t waiting;
    size_t total;
} barrier_t;

static void
barrier_init(barrier_t *b, size_t total) {
    mtx_init(&b->mtx, mtx_plain);
    cnd_init(&b->cnd);
    b->waiting = 0;
    b->total = total;
}

static void
barrier_wait(barrier_t *b) {
    mtx_lock(&b->mtx);
    b->waiting++;
    if (b->waiting == b->total) {
        b->waiting = 0;
        cnd_broadcast(&b->cnd);
    } else {
        cnd_wait(&b->cnd, &b->mtx);
    }
    mtx_unlock(&b->mtx);
}

static void
barrier_destroy(barrier_t *b) {
    mtx_destroy(&b->mtx);
    cnd_destroy(&b->cnd);
}

typedef struct {
    mtx_t mtx;
    barrier_t barrier;
    size_t next_idx;
    size_t chunk_size;
    size_t alloc_count;
    bench_stats_t *results;
    void **ptrs;
} bench_ctx_t;

static int
bench_thread(void *arg) {
    bench_ctx_t *ctx = arg;

    mtx_lock(&ctx->mtx);
    size_t idx = ctx->next_idx++;
    mtx_unlock(&ctx->mtx);

    void **my_ptrs = ctx->ptrs + (idx * ctx->alloc_count);

    struct timespec t_start, t_end;
    barrier_wait(&ctx->barrier);
    clock_gettime(CLOCK_MONOTONIC, &t_start);
    for (size_t i = 0; i < ctx->alloc_count; i++) {
        my_ptrs[i] = malloc(ctx->chunk_size);
    }
    clock_gettime(CLOCK_MONOTONIC, &t_end);

    bench_stats_t *s = &ctx->results[idx];
    s->elapsed_ms = elapsed_ms(&t_start, &t_end);
    s->per_alloc_ns = s->elapsed_ms / ctx->alloc_count * 1e6;
    s->alloc_count = ctx->alloc_count;

    return 0;
}

static void
print_stats(const bench_stats_t *stats, size_t count, int show_each) {
    if (show_each) {
        for (size_t i = 0; i < count; i++) {
            printf(
                "  [thread %zu] elapsed: %.2f ms  (%.3f us/alloc)\n",
                i,
                stats[i].elapsed_ms,
                stats[i].per_alloc_ns / 1000.0
            );
        }
    }

    double avg_elapsed = 0;
    double avg_per_alloc = 0;
    for (size_t i = 0; i < count; i++) {
        avg_elapsed += stats[i].elapsed_ms;
        avg_per_alloc += stats[i].per_alloc_ns;
    }
    avg_elapsed /= count;
    avg_per_alloc /= count;

    if (show_each) {
        printf(
            "  [avg]      elapsed: %.2f ms  (%.3f us/alloc)\n", avg_elapsed, avg_per_alloc / 1000.0
        );
    } else {
        printf("%6zu     %6.2f ms    %7.3f\n", count, avg_elapsed, avg_per_alloc / 1000.0);
    }
}

int
main(void) {
    const char *env = getenv("THREADS_MAX");
    size_t threads_max = env ? (size_t)atol(env) : 8;
    assert(
        threads_max > 0 && (threads_max & (threads_max - 1)) == 0 &&
        "THREADS_MAX must be power of 2"
    );

    int show_thread_stats = getenv("THREAD_STATS") != NULL;

    mem_stats_t before, after;
    thrd_t *tids = malloc(threads_max * sizeof(thrd_t));

    bench_ctx_t ctx = {
        .chunk_size = 0,
        .alloc_count = 0,
        .results = malloc(threads_max * sizeof(bench_stats_t)),
    };

    /* Phase 1: 10K x 1-byte allocations */
    printf("--- Phase 1: 10,000 x malloc(1) = 10KB ---\n");

    ctx.chunk_size = 1;
    ctx.alloc_count = 10000;

    if (!show_thread_stats) {
        printf("\nthreads  elapsed/thr  us/alloc\n");
    }

    ssize_t sum_vmsize_delta = 0;
    ssize_t sum_vmrss_delta = 0;
    size_t sum_threads = 0;

    for (size_t threads = 1; threads <= threads_max; threads *= 2) {
        if (show_thread_stats) {
            printf("\n  --- %zu thread%s ---\n", threads, threads > 1 ? "s" : "");
        }

        malloc_trim(0);

        ctx.next_idx = 0;
        ctx.ptrs = malloc(threads * ctx.alloc_count * sizeof(void *));
        mtx_init(&ctx.mtx, mtx_plain);
        barrier_init(&ctx.barrier, threads);

        get_mem_stats(&before);

        for (size_t i = 0; i < threads; i++) {
            thrd_create(&tids[i], bench_thread, &ctx);
        }
        for (size_t i = 0; i < threads; i++) {
            thrd_join(tids[i], NULL);
        }

        get_mem_stats(&after);

        for (size_t i = 0; i < threads * ctx.alloc_count; i++) {
            free(ctx.ptrs[i]);
        }
        free(ctx.ptrs);

        sum_vmsize_delta += (ssize_t)after.vmsize - (ssize_t)before.vmsize;
        sum_vmrss_delta += (ssize_t)after.vmrss - (ssize_t)before.vmrss;
        sum_threads += threads;

        print_stats(ctx.results, threads, show_thread_stats && threads > 1);
        barrier_destroy(&ctx.barrier);
        mtx_destroy(&ctx.mtx);
    }

    print_per_alloc("vmsize", (double)sum_vmsize_delta * 1024 / (sum_threads * ctx.alloc_count));
    print_per_alloc("vmrss", (double)sum_vmrss_delta * 1024 / (sum_threads * ctx.alloc_count));
    printf("\n");

    /* Phase 2: 16,384 x malloc(128KB) = 2GB */
    printf("--- Phase 2: 16,384 x malloc(128KB) = 2GB ---\n");

    ctx.chunk_size = 128 * 1024;
    ctx.alloc_count = 16384;

    if (!show_thread_stats) {
        printf("\nthreads  elapsed/thr  us/alloc\n");
    }

    sum_vmsize_delta = 0;
    sum_vmrss_delta = 0;
    sum_threads = 0;

    for (size_t threads = 1; threads <= threads_max; threads *= 2) {
        if (show_thread_stats) {
            printf("\n  --- %zu thread%s ---\n", threads, threads > 1 ? "s" : "");
        }

        malloc_trim(0);

        ctx.next_idx = 0;
        ctx.ptrs = malloc(threads * ctx.alloc_count * sizeof(void *));
        mtx_init(&ctx.mtx, mtx_plain);
        barrier_init(&ctx.barrier, threads);

        get_mem_stats(&before);

        for (size_t i = 0; i < threads; i++) {
            thrd_create(&tids[i], bench_thread, &ctx);
        }
        for (size_t i = 0; i < threads; i++) {
            thrd_join(tids[i], NULL);
        }

        get_mem_stats(&after);

        for (size_t i = 0; i < threads * ctx.alloc_count; i++) {
            free(ctx.ptrs[i]);
        }
        free(ctx.ptrs);

        sum_vmsize_delta += (ssize_t)after.vmsize - (ssize_t)before.vmsize;
        sum_vmrss_delta += (ssize_t)after.vmrss - (ssize_t)before.vmrss;
        sum_threads += threads;

        print_stats(ctx.results, threads, show_thread_stats && threads > 1);
        barrier_destroy(&ctx.barrier);
        mtx_destroy(&ctx.mtx);
    }

    print_per_alloc("vmsize", (double)sum_vmsize_delta * 1024 / (sum_threads * ctx.alloc_count));
    print_per_alloc("vmrss", (double)sum_vmrss_delta * 1024 / (sum_threads * ctx.alloc_count));
    printf("\n");

    /* Phase 3: 32 x malloc(128MB) = 4GB */
    printf("--- Phase 3: 32 x malloc(128MB) = 4GB ---\n");

    ctx.chunk_size = 128UL * 1024 * 1024;
    ctx.alloc_count = 32;

    if (!show_thread_stats) {
        printf("\nthreads  elapsed/thr  us/alloc\n");
    }

    sum_vmsize_delta = 0;
    sum_vmrss_delta = 0;
    sum_threads = 0;

    for (size_t threads = 1; threads <= threads_max; threads *= 2) {
        if (show_thread_stats) {
            printf("\n  --- %zu thread%s ---\n", threads, threads > 1 ? "s" : "");
        }

        malloc_trim(0);

        ctx.next_idx = 0;
        ctx.ptrs = malloc(threads * ctx.alloc_count * sizeof(void *));
        mtx_init(&ctx.mtx, mtx_plain);
        barrier_init(&ctx.barrier, threads);

        get_mem_stats(&before);

        for (size_t i = 0; i < threads; i++) {
            thrd_create(&tids[i], bench_thread, &ctx);
        }
        for (size_t i = 0; i < threads; i++) {
            thrd_join(tids[i], NULL);
        }

        get_mem_stats(&after);

        for (size_t i = 0; i < threads * ctx.alloc_count; i++) {
            free(ctx.ptrs[i]);
        }
        free(ctx.ptrs);

        sum_vmsize_delta += (ssize_t)after.vmsize - (ssize_t)before.vmsize;
        sum_vmrss_delta += (ssize_t)after.vmrss - (ssize_t)before.vmrss;
        sum_threads += threads;

        print_stats(ctx.results, threads, show_thread_stats && threads > 1);
        barrier_destroy(&ctx.barrier);
        mtx_destroy(&ctx.mtx);
    }

    print_per_alloc("vmsize", (double)sum_vmsize_delta * 1024 / (sum_threads * ctx.alloc_count));
    print_per_alloc("vmrss", (double)sum_vmrss_delta * 1024 / (sum_threads * ctx.alloc_count));
    printf("\n");

    /* Phase 4: 100 x malloc(1GB) = 100GB */
    printf("--- Phase 4: 100 x malloc(1GB) = 100GB ---\n");

    ctx.chunk_size = 1UL * 1024 * 1024 * 1024;
    ctx.alloc_count = 100;

    if (!show_thread_stats) {
        printf("\nthreads  elapsed/thr  us/alloc\n");
    }

    sum_vmsize_delta = 0;
    sum_vmrss_delta = 0;
    sum_threads = 0;

    for (size_t threads = 1; threads <= threads_max; threads *= 2) {
        if (show_thread_stats) {
            printf("\n  --- %zu thread%s ---\n", threads, threads > 1 ? "s" : "");
        }

        malloc_trim(0);

        ctx.next_idx = 0;
        ctx.ptrs = malloc(threads * ctx.alloc_count * sizeof(void *));
        mtx_init(&ctx.mtx, mtx_plain);
        barrier_init(&ctx.barrier, threads);

        get_mem_stats(&before);

        for (size_t i = 0; i < threads; i++) {
            thrd_create(&tids[i], bench_thread, &ctx);
        }
        for (size_t i = 0; i < threads; i++) {
            thrd_join(tids[i], NULL);
        }

        get_mem_stats(&after);

        for (size_t i = 0; i < threads * ctx.alloc_count; i++) {
            free(ctx.ptrs[i]);
        }
        free(ctx.ptrs);

        sum_vmsize_delta += (ssize_t)after.vmsize - (ssize_t)before.vmsize;
        sum_vmrss_delta += (ssize_t)after.vmrss - (ssize_t)before.vmrss;
        sum_threads += threads;

        print_stats(ctx.results, threads, show_thread_stats && threads > 1);
        barrier_destroy(&ctx.barrier);
        mtx_destroy(&ctx.mtx);
    }

    print_per_alloc("vmsize", (double)sum_vmsize_delta * 1024 / (sum_threads * ctx.alloc_count));
    print_per_alloc("vmrss", (double)sum_vmrss_delta * 1024 / (sum_threads * ctx.alloc_count));
    printf("\n");

    /* Phase 5: single 100GB allocation */
    printf("--- Phase 5: 1 x malloc(100GB) = 100GB ---\n");
    get_mem_stats(&before);
    printf("Before:\n");
    print_mem_stats(&before, NULL, 0);

    size_t big5 = 100UL * 1024 * 1024 * 1024;
    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);
    void *ptr5 = malloc(big5);
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    if (!ptr5) {
        printf("  malloc(100GB) FAILED (returned NULL)\n");
    } else {
        printf("  malloc(100GB) returned %p (succeeded as virtual)\n", ptr5);

        get_mem_stats(&after);
        printf("After:\n");
        print_mem_stats(&after, &before, 0);

        free(ptr5);
        malloc_trim(0);
    }
    printf("  elapsed:  %.2f ms\n", elapsed_ms(&t_start, &t_end));

    free(tids);
    free(ctx.results);

    return 0;
}
