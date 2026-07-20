#include <stdio.h>
#include <stdlib.h>

void *
malloc(size_t size) {
    (void)size;
    fprintf(stderr, "malloc(%zu) called — aborting\n", size);
    abort();
}

void *
calloc(size_t nmemb, size_t size) {
    (void)nmemb;
    (void)size;
    fprintf(stderr, "calloc(%zu, %zu) called — aborting\n", nmemb, size);
    abort();
}

void *
realloc(void *ptr, size_t size) {
    (void)ptr;
    (void)size;
    fprintf(stderr, "realloc(%p, %zu) called — aborting\n", ptr, size);
    abort();
}

void
free(void *ptr) {
    (void)ptr;
}
