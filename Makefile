GCC_FLAGS = -std=gnu23 -Wextra -Werror -Wall -Wno-gnu-folding-constant -g

all: build

build: fmt virt_phys virt_phys-static align noalloc.so

virt_phys: virt_phys.c
	gcc $(GCC_FLAGS) virt_phys.c -o virt_phys

virt_phys-static: virt_phys.c noalloc.c
	gcc $(GCC_FLAGS) virt_phys.c noalloc.c -o virt_phys_static

align: align.c
	gcc $(GCC_FLAGS) align.c -o align

noalloc.so: noalloc.c
	gcc -shared -fPIC -o noalloc.so noalloc.c

run: run-virt_phys run-noalloc run-static run-align

run-virt_phys: build
	@echo
	@echo "--- virtual vs physical memory ---"
	@./virt_phys

run-noalloc: build
	@echo
	@echo "--- aborting allocator (LD_PRELOAD) ---"
	-@LD_PRELOAD=./noalloc.so ./virt_phys

run-static: build
	@echo
	@echo "--- aborting allocator (static) ---"
	-@./virt_phys_static

run-align: build
	@echo
	@echo "--- malloc alignment ---"
	@./align

fmt:
	clang-format -i *.c *.h 2>/dev/null || true
