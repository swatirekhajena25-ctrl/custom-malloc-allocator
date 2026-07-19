# Custom Memory Allocator (malloc / free) in C

A from-scratch implementation of dynamic memory allocation in C,
replicating the core mechanics behind `malloc()` and `free()` using
a linked free-list, block splitting, and coalescing to reduce
fragmentation.

## Overview

This allocator manages a heap region requested from the OS via
`sbrk()`, tracking every allocated and free block using an
in-memory linked list of block headers. It implements the two
core techniques real allocators use to use memory efficiently:

- **Splitting** — when a free block is much larger than requested,
  it's split into two: one sized exactly for the request, and a
  smaller free block left over for future use.
- **Coalescing** — when a block is freed, it's merged with
  adjacent free blocks (if any) to prevent the heap from
  fragmenting into many small, unusable gaps.

## Design

| Component | Purpose |
|---|---|
| `block_header_t` | Metadata stored just before every allocated region: size, free/used flag, and pointers to the next/previous block in the list |
| `request_space()` | Asks the OS for more memory via `sbrk()` only when no existing free block can satisfy a request |
| `find_free_block()` | First-fit search through the free list for a block large enough to reuse |
| `split_block()` | Divides an oversized free block so leftover space isn't wasted |
| `coalesce()` | Merges a freshly freed block with its free neighbors, reducing fragmentation |
| `ALIGN()` macro | Rounds every allocation up to the nearest 8-byte boundary, matching how real-world allocators align memory for CPU efficiency |

## Build & Run

Requires `gcc` (standard on any Linux environment, including GitHub
Codespaces).

```bash
gcc allocator.c -o allocator
./allocator
```

The included `main()` runs a demonstration: allocating several
blocks, freeing some, printing the free list at each step, and
showing a new allocation reuse the coalesced space instead of
requesting fresh memory from the OS.

## What I learned / debugged

- **Why alignment matters** — unaligned memory access can be slower
  (or on some architectures, illegal), so real allocators always
  round allocation sizes up to a fixed boundary (commonly 8 or 16
  bytes) rather than returning the exact byte count requested.
- **Why coalescing must check both neighbors** — a freed block can
  potentially merge with the block *after* it and the block
  *before* it; missing either case leaves avoidable fragmentation.
- **The header-before-payload trick** — `my_free()` receives only a
  pointer to the user's data, not the block header. Metadata is
  recovered by pointer arithmetic (`(block_header_t *)ptr - 1`),
  which is exactly how production allocators locate their bookkeeping
  data without needing a separate lookup structure.

## Possible extensions

- Best-fit or worst-fit block selection instead of first-fit
- Thread safety (mutex-protected free list) for multi-threaded use
- `realloc()` support
- Segregated free lists (separate lists per size class) for faster
  lookup at scale