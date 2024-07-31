#ifndef ARENA_H_
#define ARENA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARENA_DEFAULT_CAPACITY (8 * 1024)
#define ARENA_CMP(size) ((size) < ARENA_DEFAULT_CAPACITY ? ARENA_DEFAULT_CAPACITY : (size))

typedef struct Region Region;

struct Region {
    size_t alloc_pos;
    size_t capacity;
    Region *next;
    char *data;
};

Region *region_create(size_t capacity);

typedef struct {
    Region *head;
    Region *tail;
} Arena;

void arena_dump(Arena *arena);
void arena_free(Arena *arena);
void arena_reset(Arena *arena);

void *arena_alloc(Arena *arena, size_t size);
void *arena_alloc_aligned(Arena *arena, size_t size, size_t alignment);
void *arena_realloc(Arena *arena, void *old_ptr, size_t old_size, size_t new_size);

#define arena_da_append(a, da, item, c) \
    do { \
        if ((da)->count + 1 >= (da)->capacity) { \
            size_t old_size = (da)->capacity * sizeof(*(da)->items); \
            size_t new_size = (da)->capacity == 0 ? (c) : (da)->capacity * 2; \
            new_size *= sizeof(*(da)->items); \
            (da)->items = arena_realloc((a), (da)->items, old_size, new_size); \
            (da)->capacity = new_size; \
        } \
        (da)->items[(da)->count++] = (item); \
    } while(0)

#endif // ARENA_H_
