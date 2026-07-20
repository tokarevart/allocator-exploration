GCC_FLAGS = -std=gnu23 -Wextra -Werror -Wall -Wno-gnu-folding-constant -g

all: build

build: fmt memtest.c
	gcc $(GCC_FLAGS) memtest.c -o memtest

run: build
	./memtest

noalloc.so: noalloc.c
	gcc -shared -fPIC -o noalloc.so noalloc.c

run-noalloc: build noalloc.so
	LD_PRELOAD=./noalloc.so ./memtest

run-static: fmt memtest.c noalloc.c
	gcc $(GCC_FLAGS) memtest.c noalloc.c -o memtest_static
	./memtest_static

fmt:
	clang-format -i *.c *.h 2>/dev/null || true
