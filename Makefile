GCC_FLAGS = -std=gnu23 -O3 -march=native -Wextra -Werror -Wall -Wno-gnu-folding-constant -g
THREADS_MAX ?= 8

all: build

build: fmt virt_phys virt_phys-static align noalloc.so pagemalloc.so

virt_phys: virt_phys.c
	gcc $(GCC_FLAGS) virt_phys.c -o build/virt_phys

virt_phys-static: virt_phys.c noalloc.c
	gcc $(GCC_FLAGS) virt_phys.c noalloc.c -o build/virt_phys_static

align: align.c
	gcc $(GCC_FLAGS) align.c -o build/align

noalloc.so: noalloc.c
	gcc $(GCC_FLAGS) -shared -fPIC -o build/noalloc.so noalloc.c

pagemalloc.so: pagemalloc.c
	gcc $(GCC_FLAGS) -shared -fPIC -o build/pagemalloc.so pagemalloc.c

run: run-virt_phys run-noalloc run-static run-pagemalloc run-mimalloc run-align

run-virt_phys: build
	@echo
	@echo "--- system allocator ---"
	@THREADS_MAX=$(THREADS_MAX) ./build/virt_phys

run-noalloc: build
	@echo
	@echo "--- aborting allocator (LD_PRELOAD) ---"
	-@THREADS_MAX=$(THREADS_MAX) LD_PRELOAD=./build/noalloc.so ./build/virt_phys

run-static: build
	@echo
	@echo "--- aborting allocator (static) ---"
	-@THREADS_MAX=$(THREADS_MAX) ./build/virt_phys_static

run-pagemalloc: build
	@echo
	@echo "--- pagemalloc (mmap-only allocator) ---"
	-@THREADS_MAX=$(THREADS_MAX) LD_PRELOAD=./build/pagemalloc.so ./build/virt_phys

run-mimalloc: build
	@echo
	@echo "--- mimalloc (LD_PRELOAD) ---"
	-@THREADS_MAX=$(THREADS_MAX) LD_PRELOAD=/usr/lib64/libmimalloc.so.2 ./build/virt_phys

run-align: build
	@echo
	@echo "--- malloc alignment ---"
	@./build/align

fmt:
	clang-format -i *.c *.h 2>/dev/null || true
