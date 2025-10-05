# my_alloc

An alignment-aware custom memory allocator implementing `malloc()`, `free()`, `calloc()`, and `realloc()` from scratch.

## What is this?

This is a custom memory allocator built using `sbrk()` to understand how dynamic memory allocation works under the hood. It implements the core memory management functions with proper alignment, metadata tracking, and free block reuse.

## Quick Start

compile and test run:

```bash
gcc my_alloc.c -o my_alloc

./my_alloc
```

## How it works

Each memory block has metadata stored right before the user pointer:

```
Heap: [metadata][user data][metadata][user data]...
               ↑                     ↑
          returned ptr         returned ptr
```

The allocator maintains a linked list of blocks and reuses freed blocks when possible.

## Implementation

- **First-fit strategy**: Scans the free list and reuses the first block that fits
- **Metadata per block**: Tracks size, aligned size, and free status
- **Alignment**: Rounds all allocations to 8-byte boundaries (configurable)
- **No coalescing**: Freed blocks are marked but not merged (yet)

## API

```c
void* my_alloc(size_t size);
void  freee(void* ptr);
void* call_oc(size_t nelem, size_t elsize);
void* reall_oc(void* ptr, size_t size);
```

## Limitations

- Not thread-safe, currently
- No block splitting or coalescing
- Memory is never returned to the OS
- First-fit can cause fragmentation

## TODO

- [ ] Block splitting for better memory utilization
- [ ] Coalescing adjacent free blocks
- [ ] Segregated free lists
- [ ] Thread safety (mutexes)
- [ ] Better fit strategies (best-fit, next-fit)


A complete guide to the development can be found in `my_log.md` in this repo.
