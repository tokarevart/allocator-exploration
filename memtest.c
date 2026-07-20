#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <unistd.h>

// void
// free(void *ptr) {
//     (void)ptr;
//     char *msg = "free: aborting\n";
//     write(STDERR_FILENO, msg, strlen(msg));
//     abort();
// }

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
        if (sscanf(line, "VmSize: %zu kB", &val) == 1)
            s->vmsize = val;
        else if (sscanf(line, "VmRSS: %zu kB", &val) == 1)
            s->vmrss = val;
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
        if (pv < 1024.0)
            printf("  VmSize: %8zu kB  (%+zd, %+.0f B/alloc)\n", s->vmsize, dv, pv);
        else
            printf("  VmSize: %8zu kB  (%+zd, %+.1f kB/alloc)\n", s->vmsize, dv, pv / 1024);
        if (pr < 1024.0)
            printf("  VmRSS:  %8zu kB  (%+zd, %+.0f B/alloc)\n", s->vmrss, dr, pr);
        else
            printf("  VmRSS:  %8zu kB  (%+zd, %+.1f kB/alloc)\n", s->vmrss, dr, pr / 1024);
    } else if (prev) {
        printf("  VmSize: %8zu kB  (%+zd)\n", s->vmsize, dv);
        printf("  VmRSS:  %8zu kB  (%+zd)\n", s->vmrss, dr);
    } else {
        printf("  VmSize: %8zu kB\n", s->vmsize);
        printf("  VmRSS:  %8zu kB\n", s->vmrss);
    }
}

int
main(void) {
    mem_stats_t before, after;

    /* Phase 1: 1M x 1-byte allocations */
    printf("--- Phase 1: 1,000,000 x malloc(1) = 1MB ---\n");
    get_mem_stats(&before);
    printf("Before:\n");
    print_mem_stats(&before, NULL, 0);

    size_t n1 = 1000000;
    void **ptrs1 = malloc(n1 * sizeof(void *));
    for (size_t i = 0; i < n1; i++) {
        ptrs1[i] = malloc(1);
    }

    get_mem_stats(&after);
    printf("After:\n");
    print_mem_stats(&after, &before, n1);

    for (size_t i = 0; i < n1; i++) {
        free(ptrs1[i]);
    }
    free(ptrs1);
    malloc_trim(0);
    printf("\n");

    /* Phase 2: 2GB of 128KB allocations */
    printf("--- Phase 2: 16,384 x malloc(128KB) = 2GB ---\n");
    get_mem_stats(&before);
    printf("Before:\n");
    print_mem_stats(&before, NULL, 0);

    size_t chunk2 = 128 * 1024;
    size_t total2 = (size_t)2 * 1024 * 1024 * 1024;
    size_t n2 = total2 / chunk2;
    void **ptrs2 = malloc(n2 * sizeof(void *));
    for (size_t i = 0; i < n2; i++) {
        ptrs2[i] = malloc(chunk2);
    }

    get_mem_stats(&after);
    printf("After:\n");
    print_mem_stats(&after, &before, n2);

    for (size_t i = 0; i < n2; i++) {
        free(ptrs2[i]);
    }
    free(ptrs2);
    malloc_trim(0);
    printf("\n");

    /* Phase 3: 4GB of 128MB allocations */
    printf("--- Phase 3: 32 x malloc(128MB) = 4GB ---\n");
    get_mem_stats(&before);
    printf("Before:\n");
    print_mem_stats(&before, NULL, 0);

    size_t chunk3 = 128UL * 1024 * 1024;
    size_t total3 = 4UL * 1024 * 1024 * 1024;
    size_t n3 = total3 / chunk3;
    void **ptrs3 = malloc(n3 * sizeof(void *));
    for (size_t i = 0; i < n3; i++) {
        ptrs3[i] = malloc(chunk3);
    }

    get_mem_stats(&after);
    printf("After:\n");
    print_mem_stats(&after, &before, n3);

    for (size_t i = 0; i < n3; i++) {
        free(ptrs3[i]);
    }
    free(ptrs3);
    malloc_trim(0);
    printf("\n");

    /* Phase 4: 100GB of 1GB allocations */
    printf("--- Phase 4: 100 x malloc(1GB) = 100GB ---\n");
    get_mem_stats(&before);
    printf("Before:\n");
    print_mem_stats(&before, NULL, 0);

    size_t chunk4 = 1UL * 1024 * 1024 * 1024;
    size_t total4 = 100UL * 1024 * 1024 * 1024;
    size_t n4 = total4 / chunk4;
    void **ptrs4 = malloc(n4 * sizeof(void *));
    for (size_t i = 0; i < n4; i++) {
        ptrs4[i] = malloc(chunk4);
    }

    get_mem_stats(&after);
    printf("After:\n");
    print_mem_stats(&after, &before, n4);

    for (size_t i = 0; i < n4; i++) {
        free(ptrs4[i]);
    }
    free(ptrs4);
    malloc_trim(0);
    printf("\n");

    /* Phase 5: single 100GB allocation */
    printf("--- Phase 5: 1 x malloc(100GB) = 100GB ---\n");
    get_mem_stats(&before);
    printf("Before:\n");
    print_mem_stats(&before, NULL, 0);

    size_t big5 = 100UL * 1024 * 1024 * 1024;
    void *ptr5 = malloc(big5);
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

    return 0;
}
