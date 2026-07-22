# allocator-exploration

Demos and benchmarks of malloc & virtual memory.

## Programs

| File | What it does |
|------|-------------|
| `virt_phys.c` | Allocates memory in 5 phases (1B → 1GB) and prints VmSize/VmRSS from `/proc/self/status` with per-alloc cost |
| `align.c` | Prints malloc pointer addresses and minimum alignment across 1–1000 byte allocations |
| `noalloc.c` | Aborting malloc/calloc/realloc/free — use via `LD_PRELOAD` or linked statically |
| `pagemalloc.c` | mmap-only allocator with 16-byte header, `mremap` for realloc — use via `LD_PRELOAD` |

## Build & Run

```sh
make            # build all
make run        # run all demos (virt_phys with glibc, noalloc, pagemalloc, mimalloc, align)
```

Requires: gcc with `-std=gnu23`, `clang-format` (optional, for `make fmt`), `mimalloc` system library (optional, for `run-mimalloc`).

## Allocator comparison

`make run` executes `virt_phys` under five allocators:

1. **glibc** (default) — standard allocator
2. **noalloc** (`LD_PRELOAD`) — aborts on any allocation, shows which phase needs memory
3. **pagemalloc** (`LD_PRELOAD`) — every `malloc` is a `mmap`, every `free` is `munmap`
4. **mimalloc** (`LD_PRELOAD`) — fast allocator with thread-local heaps, aggressive page return
5. **Static noalloc** — `noalloc.c` linked directly into the binary

## Alignment

`align.c` prints the actual alignment of `malloc` pointers for various sizes and reports the minimum alignment observed across 1–1000 byte allocations. On glibc x86-64, the minimum is typically 16 bytes.
