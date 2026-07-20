GCC_FLAGS = -std=gnu23 -Wextra -Werror -Wall -Wno-gnu-folding-constant -g

all: build

build: fmt
	gcc $(GCC_FLAGS) memtest.c -o memtest

run: build
	./memtest

fmt:
	clang-format -i *.c *.h 2>/dev/null || true
