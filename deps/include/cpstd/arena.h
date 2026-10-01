#pragma once

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    size_t capacity;
    size_t pos;
} mem_arena;

#define ALIGN_POW2(n, p)                                                       \
    (((size_t)(n) + ((size_t)(p) - 1)) & (~((size_t)(p) - 1)))
#define ARENA_BASE_POS (sizeof(mem_arena))
#define ARENA_ALIGN (sizeof(void *))

#define KiB(b) ((b) * 1024)
#define MiB(b) ((b) * 1024 * 1024)
#define GiB(b) ((b) * 1024 * 1024 * 1024)

mem_arena *arena_create(size_t capacity);
void arena_destroy(mem_arena *arena);
void *arena_alloc(mem_arena *arena, size_t size);
void *arena_calloc(mem_arena *arena, size_t size);
void arena_free(mem_arena *arena, size_t size);
void arena_free_to(mem_arena *arena, size_t pos);
void arena_clear(mem_arena *arena);
