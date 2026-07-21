#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int
main(void) {
    size_t sizes[] = {1, 2, 3, 7, 15, 16, 17, 31, 32, 33, 63, 64, 65, 128, 256, 512, 1024};
    size_t n = sizeof(sizes) / sizeof(sizes[0]);

    printf("%-12s  %-18s  %-8s  %s\n", "size", "ptr", "binary", "aligned for");

    for (size_t i = 0; i < n; i++) {
        void *p = malloc(sizes[i]);
        uintptr_t addr = (uintptr_t)p;
        size_t align = addr & -addr;
        printf("%-12zu  0x%016lx  ", sizes[i], (unsigned long)addr);
        for (int b = 7; b >= 0; b--) {
            printf("%c", (addr >> b) & 1 ? '1' : '0');
        }
        printf("  %zu bytes\n", align);
        free(p);
    }

    size_t min_align = SIZE_MAX;
    for (size_t sz = 1; sz <= 1000; sz++) {
        void *p = malloc(sz);
        uintptr_t addr = (uintptr_t)p;
        size_t align = addr & -addr;
        if (align < min_align) {
            min_align = align;
        }
        free(p);
    }
    printf("\nmin alignment across 1-1000 byte allocs: %zu bytes\n", min_align);

    printf("\n--- u64 read/write speed by offset ---\n");
    constexpr size_t buf_elems = 16 * 1024 / sizeof(uint64_t);
    uint64_t *buf = aligned_alloc(32, buf_elems * sizeof(uint64_t) + 32);
    for (size_t i = 0; i < buf_elems; i++) {
        buf[i] = (uint64_t)i;
    }
    size_t iters = 32000000;
    size_t data_mb = iters * 16 * sizeof(uint64_t) / (1024 * 1024);

    // warm up the cache
    volatile uint64_t sink = 0;
    uint64_t *base = (uint64_t *)((char *)buf);
    for (size_t i = 0; i < iters; i++) {
        sink += base[i % buf_elems];
    }

    printf("  offset   read (MB/s)   read (ms)   write (MB/s)   write (ms)\n");
    size_t test_offsets[] = {0, 4};
    for (size_t oi = 0; oi < 2; oi++) {
        size_t off = test_offsets[oi];
        struct timespec t0, t1;

        clock_gettime(CLOCK_MONOTONIC, &t0);
        uint64_t s0 = 0, s1 = 0, s2 = 0, s3 = 0;
        uint64_t s4 = 0, s5 = 0, s6 = 0, s7 = 0;
        uint64_t s8 = 0, s9 = 0, sa = 0, sb = 0;
        uint64_t sc = 0, sd = 0, se = 0, sf = 0;
        uint64_t *rp = (uint64_t *)((char *)buf + off);
        for (size_t i = 0; i < iters; i++) {
            size_t base = (i * 16) % buf_elems;
            s0 += rp[(base + 0) % buf_elems];
            s1 += rp[(base + 1) % buf_elems];
            s2 += rp[(base + 2) % buf_elems];
            s3 += rp[(base + 3) % buf_elems];
            s4 += rp[(base + 4) % buf_elems];
            s5 += rp[(base + 5) % buf_elems];
            s6 += rp[(base + 6) % buf_elems];
            s7 += rp[(base + 7) % buf_elems];
            s8 += rp[(base + 8) % buf_elems];
            s9 += rp[(base + 9) % buf_elems];
            sa += rp[(base + 10) % buf_elems];
            sb += rp[(base + 11) % buf_elems];
            sc += rp[(base + 12) % buf_elems];
            sd += rp[(base + 13) % buf_elems];
            se += rp[(base + 14) % buf_elems];
            sf += rp[(base + 15) % buf_elems];
        }
        sink = s0 + s1 + s2 + s3 + s4 + s5 + s6 + s7 + s8 + s9 + sa + sb + sc + sd + se + sf;
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double read_ms = (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6;

        clock_gettime(CLOCK_MONOTONIC, &t0);
        uint64_t *wp = (uint64_t *)((char *)buf + off);
        for (size_t i = 0; i < iters; i++) {
            size_t base = (i * 16) % buf_elems;
            wp[(base + 0) % buf_elems] = (uint64_t)i;
            wp[(base + 1) % buf_elems] = (uint64_t)i;
            wp[(base + 2) % buf_elems] = (uint64_t)i;
            wp[(base + 3) % buf_elems] = (uint64_t)i;
            wp[(base + 4) % buf_elems] = (uint64_t)i;
            wp[(base + 5) % buf_elems] = (uint64_t)i;
            wp[(base + 6) % buf_elems] = (uint64_t)i;
            wp[(base + 7) % buf_elems] = (uint64_t)i;
            wp[(base + 8) % buf_elems] = (uint64_t)i;
            wp[(base + 9) % buf_elems] = (uint64_t)i;
            wp[(base + 10) % buf_elems] = (uint64_t)i;
            wp[(base + 11) % buf_elems] = (uint64_t)i;
            wp[(base + 12) % buf_elems] = (uint64_t)i;
            wp[(base + 13) % buf_elems] = (uint64_t)i;
            wp[(base + 14) % buf_elems] = (uint64_t)i;
            wp[(base + 15) % buf_elems] = (uint64_t)i;
        }
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double write_ms = (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6;

        printf(
            "  %3zu        %7.1f      %8.1f        %7.1f      %8.1f%s\n",
            off,
            data_mb / read_ms * 1000.0,
            read_ms,
            data_mb / write_ms * 1000.0,
            write_ms,
            off == 0 ? "  <- aligned" : ""
        );
    }
    free(buf);

    /* Sequential stride-8: no modulo, inner loop reads buf_elems sequentially */
    printf("--- sequential stride-8 (no modulo) ---\n");
    uint64_t *seq_buf = aligned_alloc(32, buf_elems * sizeof(uint64_t) + 32);
    for (size_t i = 0; i < buf_elems; i++) {
        seq_buf[i] = (uint64_t)i;
    }
    size_t seq_chunks = 15625;
    size_t seq_iters = seq_chunks * buf_elems;
    size_t seq_data_mb = seq_iters * sizeof(uint64_t) / (1024 * 1024);

    uint64_t seq_sink = 0;
    uint64_t *srp = (uint64_t *)((char *)seq_buf);
    for (size_t c = 0; c < seq_chunks; c++) {
#pragma GCC unroll 8
        for (size_t i = 0; i < buf_elems; i++) {
            seq_sink += srp[i];
        }
    }
    (void)seq_sink;

    printf("\n  offset   read (MB/s)   read (ms)   write (MB/s)   write (ms)\n");
    size_t seq_test_offsets[] = {0, 8};
    for (size_t oi = 0; oi < 2; oi++) {
        size_t off = seq_test_offsets[oi];
        struct timespec t0, t1;

        clock_gettime(CLOCK_MONOTONIC, &t0);
        uint64_t seq_sink = 0;
        uint64_t *srp = (uint64_t *)((char *)seq_buf + off);
        for (size_t c = 0; c < seq_chunks; c++) {
#pragma GCC unroll 8
            for (size_t i = 0; i < buf_elems; i++) {
                seq_sink += srp[i];
            }
        }
        volatile uint64_t tmp = seq_sink;
        (void)tmp;
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double read_ms = (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6;

        clock_gettime(CLOCK_MONOTONIC, &t0);
        uint64_t *swp = (uint64_t *)((char *)seq_buf + off);
        for (size_t c = 0; c < seq_chunks; c++) {
            for (size_t i = 0; i < buf_elems; i++) {
                swp[i] = (uint64_t)(c * buf_elems + i);
            }
        }
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double write_ms = (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6;

        printf(
            "  %3zu        %7.1f      %8.1f        %7.1f      %8.1f%s\n",
            off,
            seq_data_mb / read_ms * 1000.0,
            read_ms,
            seq_data_mb / write_ms * 1000.0,
            write_ms,
            off == 0 ? "  <- aligned" : ""
        );
    }
    free(seq_buf);

    /* AVX2: 32-byte aligned, 4 x __m256i accumulators (128 bytes/iter) */
    printf("\n--- avx2 read/write speed (32-byte aligned) ---\n");
    uint64_t *avx_buf = aligned_alloc(32, buf_elems * sizeof(uint64_t) + 32);
    for (size_t i = 0; i < buf_elems; i++) {
        avx_buf[i] = (uint64_t)i;
    }

    printf("  offset   read (MB/s)   read (ms)   write (MB/s)   write (ms)\n");
    size_t avx_test_offsets[] = {0, 4, 8};
    for (size_t oi = 0; oi < 3; oi++) {
        size_t off = avx_test_offsets[oi];
        struct timespec t0, t1;

        clock_gettime(CLOCK_MONOTONIC, &t0);
        __m256i a0 = _mm256_setzero_si256();
        __m256i a1 = _mm256_setzero_si256();
        __m256i a2 = _mm256_setzero_si256();
        __m256i a3 = _mm256_setzero_si256();
        uint64_t *arp = (uint64_t *)((char *)avx_buf + off);
        if (off == 0) {
            for (size_t i = 0; i < iters; i++) {
                size_t base = (i * 16) % buf_elems;
                a0 = _mm256_add_epi64(
                    a0, _mm256_load_si256((__m256i *)(arp + (base + 0) % buf_elems))
                );
                a1 = _mm256_add_epi64(
                    a1, _mm256_load_si256((__m256i *)(arp + (base + 4) % buf_elems))
                );
                a2 = _mm256_add_epi64(
                    a2, _mm256_load_si256((__m256i *)(arp + (base + 8) % buf_elems))
                );
                a3 = _mm256_add_epi64(
                    a3, _mm256_load_si256((__m256i *)(arp + (base + 12) % buf_elems))
                );
            }
        } else {
            for (size_t i = 0; i < iters; i++) {
                size_t base = (i * 16) % buf_elems;
                a0 = _mm256_add_epi64(
                    a0, _mm256_loadu_si256((__m256i *)(arp + (base + 0) % buf_elems))
                );
                a1 = _mm256_add_epi64(
                    a1, _mm256_loadu_si256((__m256i *)(arp + (base + 4) % buf_elems))
                );
                a2 = _mm256_add_epi64(
                    a2, _mm256_loadu_si256((__m256i *)(arp + (base + 8) % buf_elems))
                );
                a3 = _mm256_add_epi64(
                    a3, _mm256_loadu_si256((__m256i *)(arp + (base + 12) % buf_elems))
                );
            }
        }
        __m256i sum = _mm256_add_epi64(_mm256_add_epi64(a0, a1), _mm256_add_epi64(a2, a3));
        uint64_t tmp[4];
        _mm256_storeu_si256((__m256i *)tmp, sum);
        sink = tmp[0] + tmp[1] + tmp[2] + tmp[3];
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double read_ms = (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6;

        clock_gettime(CLOCK_MONOTONIC, &t0);
        uint64_t *awp = (uint64_t *)((char *)avx_buf + off);
        if (off == 0) {
            for (size_t i = 0; i < iters; i++) {
                size_t base = (i * 16) % buf_elems;
                _mm256_store_si256(
                    (__m256i *)(awp + (base + 0) % buf_elems), _mm256_set1_epi64x((long)i)
                );
                _mm256_store_si256(
                    (__m256i *)(awp + (base + 4) % buf_elems), _mm256_set1_epi64x((long)i)
                );
                _mm256_store_si256(
                    (__m256i *)(awp + (base + 8) % buf_elems), _mm256_set1_epi64x((long)i)
                );
                _mm256_store_si256(
                    (__m256i *)(awp + (base + 12) % buf_elems), _mm256_set1_epi64x((long)i)
                );
            }
        } else {
            for (size_t i = 0; i < iters; i++) {
                size_t base = (i * 16) % buf_elems;
                _mm256_storeu_si256(
                    (__m256i *)(awp + (base + 0) % buf_elems), _mm256_set1_epi64x((long)i)
                );
                _mm256_storeu_si256(
                    (__m256i *)(awp + (base + 4) % buf_elems), _mm256_set1_epi64x((long)i)
                );
                _mm256_storeu_si256(
                    (__m256i *)(awp + (base + 8) % buf_elems), _mm256_set1_epi64x((long)i)
                );
                _mm256_storeu_si256(
                    (__m256i *)(awp + (base + 12) % buf_elems), _mm256_set1_epi64x((long)i)
                );
            }
        }
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double write_ms = (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6;

        printf(
            "  %3zu        %7.1f      %8.1f        %7.1f      %8.1f%s\n",
            off,
            data_mb / read_ms * 1000.0,
            read_ms,
            data_mb / write_ms * 1000.0,
            write_ms,
            off == 0 ? "  <- aligned" : ""
        );
    }
    free(avx_buf);

    return 0;
}
