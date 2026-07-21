#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

    return 0;
}
