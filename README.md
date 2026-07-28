# allocator-exploration

Demos and benchmarks of malloc & virtual memory.

## Programs

| File | What it does |
|------|-------------|
| `bench.c` | Multi-threaded allocator benchmark: runs allocation phases (1B → 1GB) across 1..THREADS_MAX threads (doubling), measures elapsed time, per-alloc cost, and VmSize/VmRSS from `/proc/self/status` |
| `align.c` | Prints malloc pointer addresses and minimum alignment across 1–1000 byte allocations |
| `noalloc.c` | Aborting malloc/calloc/realloc/free — use via `LD_PRELOAD` or linked statically |
| `pagemalloc.c` | mmap-only allocator with 16-byte header, `mremap` for realloc — use via `LD_PRELOAD` |

## Build & Run

```sh
make                    # build all
make run                # run all demos
make run-sys            # run only bench with system allocator
```

Requires: gcc with `-std=gnu23` and C11 threads support, `clang-format` (optional, for `make fmt`), `mimalloc` system library (optional, for `run-mimalloc`).

## Environment variables

| Variable | Default | Description |
|----------|---------|-------------|
| `THREADS_MAX` | `8` | Max thread count (must be power of 2). Benchmarks run at 1, 2, 4, ..., THREADS_MAX threads |
| `THREAD_STATS` | unset | Set to show per-thread timing breakdown and `[avg]` aggregate |

```sh
THREADS_MAX=4 make run-sys                 # override thread count
THREAD_STATS=1 make run-sys                # show per-thread stats
THREADS_MAX=16 THREAD_STATS=1 make run     # both
```

## Allocator comparison

`make run` executes `bench` under six allocators:

1. **sys** (default) — system/glibc allocator
2. **noalloc** (`LD_PRELOAD`) — aborts on any allocation, shows which phase needs memory
3. **noalloc-static** — `noalloc.c` linked directly into the binary
4. **pagemalloc** (`LD_PRELOAD`) — every `malloc` is a `mmap`, every `free` is `munmap`
5. **mimalloc** (`LD_PRELOAD`) — fast allocator with thread-local heaps, aggressive page return

## Alignment

`align.c` prints the actual alignment of `malloc` pointers for various sizes and reports the minimum alignment observed across 1–1000 byte allocations. On glibc x86-64, the minimum is typically 16 bytes.
