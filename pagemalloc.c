#define _GNU_SOURCE
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

constexpr size_t HEADER_SIZE = 16;

void *
malloc(size_t size) {
    int prot = PROT_READ | PROT_WRITE;
    int flags = MAP_PRIVATE | MAP_ANONYMOUS;
    void *p = mmap(NULL, size + HEADER_SIZE, prot, flags, -1, 0);
    if (p == MAP_FAILED) {
        return NULL;
    }
    *(size_t *)p = size;
    return (char *)p + HEADER_SIZE;
}

void
free(void *ptr) {
    if (!ptr) {
        return;
    }
    void *base = (char *)ptr - HEADER_SIZE;
    size_t size = *(size_t *)base;
    munmap(base, size + HEADER_SIZE);
}

void *
calloc(size_t nmemb, size_t size) {
    if (nmemb && size > SIZE_MAX / nmemb) {
        return NULL;
    }
    return malloc(nmemb * size);
}

void *
realloc(void *ptr, size_t new_size) {
    if (!ptr) {
        return malloc(new_size);
    }
    if (new_size == 0) {
        free(ptr);
        return NULL;
    }
    size_t old_size = *(size_t *)((char *)ptr - HEADER_SIZE);
    void *new_ptr = mremap(
        (char *)ptr - HEADER_SIZE, old_size + HEADER_SIZE, new_size + HEADER_SIZE, MREMAP_MAYMOVE
    );
    if (new_ptr == MAP_FAILED) {
        return NULL;
    }
    *(size_t *)new_ptr = new_size;
    return (char *)new_ptr + HEADER_SIZE;
}
