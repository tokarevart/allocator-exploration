GCC_FLAGS = -std=gnu23 -O3 -march=native -Wextra -Werror -Wall -Wno-gnu-folding-constant -g
THREADS_MAX ?= 8

all: build

build: fmt bench noalloc-static align noalloc.so pagemalloc.so

bench: bench.c
	gcc $(GCC_FLAGS) bench.c -o build/bench

noalloc-static: bench.c noalloc.c
	gcc $(GCC_FLAGS) bench.c noalloc.c -o build/noalloc_static

align: align.c
	gcc $(GCC_FLAGS) align.c -o build/align

noalloc.so: noalloc.c
	gcc $(GCC_FLAGS) -shared -fPIC -o build/noalloc.so noalloc.c

pagemalloc.so: pagemalloc.c
	gcc $(GCC_FLAGS) -shared -fPIC -o build/pagemalloc.so pagemalloc.c

run: run-sys run-noalloc run-noalloc-static run-pagemalloc run-mimalloc run-align

run-sys: build
	@echo
	@echo "--- system allocator ---"
	@THREADS_MAX=$(THREADS_MAX) ./build/bench

run-noalloc: build
	@echo
	@echo "--- aborting allocator (LD_PRELOAD) ---"
	-@THREADS_MAX=$(THREADS_MAX) LD_PRELOAD=./build/noalloc.so ./build/bench

run-noalloc-static: build
	@echo
	@echo "--- aborting allocator (static) ---"
	-@THREADS_MAX=$(THREADS_MAX) ./build/noalloc_static

run-pagemalloc: build
	@echo
	@echo "--- pagemalloc (mmap-only allocator) ---"
	-@THREADS_MAX=$(THREADS_MAX) LD_PRELOAD=./build/pagemalloc.so ./build/bench

run-mimalloc: build
	@echo
	@echo "--- mimalloc (LD_PRELOAD) ---"
	-@THREADS_MAX=$(THREADS_MAX) LD_PRELOAD=/usr/lib64/libmimalloc.so.2 ./build/bench

run-align: build
	@echo
	@echo "--- malloc alignment ---"
	@./build/align

fmt:
	clang-format -i *.c *.h 2>/dev/null || true
