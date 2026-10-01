#pragma once

#include <assert.h>
#include <stdint.h>
#include <string.h>

typedef enum : uint8_t {
    HS_EMPTY = 0,
    HS_OCCUPIED,
    HS_TOMBSTONE
} hs_entry_state_t;

typedef struct {
    uint8_t *flag;
    size_t size;
    size_t capacity;
} hs_header_t;

#define hs_header(hs) ((hs_header_t *)(hs) - 1)
#define hs_size(hs) ((hs) ? hs_header(hs)->size : 0)
#define hs_empty(hs) (hs_header(hs)->size == 0)

#define hs_init(hs, capacity) hs_init_impl(sizeof(*(hs)), capacity)

void *hs_init_impl(size_t element_size, size_t capacity);
void hs_destroy(void *hs);

static size_t hs_hash(void *key, size_t key_size) {
    assert(key);
    size_t hash = 14695981039346656037ULL;
    uint8_t *bytes = (uint8_t *)key;
    for (size_t i = 0; i < key_size; i++) {
        hash ^= bytes[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

static size_t hs_string_to_key(const char *str) {
    assert(str);
    size_t hash = 0xcbf29ce484222325ULL;
    while (*str) {
        hash ^= (unsigned char)*str++;
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

static size_t hs_probe_impl(void *hs, void *key, size_t key_size, size_t element_size) {
    assert(hs);
    assert(key);
    hs_header_t *header = hs_header(hs);
    size_t hash = hs_hash(key, key_size);
    size_t idx = hash % header->capacity;
    size_t first_tomb = (size_t)-1;
    char *data = (char *)hs;
    while (1) {
        if (header->flag[idx] == HS_EMPTY) {
            return first_tomb != (size_t)-1 ? first_tomb : idx;
        }
        if (header->flag[idx] == HS_TOMBSTONE) {
            if (first_tomb == (size_t)-1) {
                first_tomb = idx;
            }
        } else if (header->flag[idx] == HS_OCCUPIED) {
            void *cur_key = data + (idx * element_size);
            if (memcmp(cur_key, key, key_size) == 0) {
                return idx;
            }
        }
        idx = (idx + 1) % header->capacity;
    }
}

void *hs_resize_impl(void *hs, size_t element_size, size_t key_size);

void *hs_begin(void *hs);

#define hs_end(hs) hs_end_impl(hs, sizeof(*(hs)))
void *hs_end_impl(void *hs, size_t element_size);

#define hs_it_exist(hs, it) hs_it_exist_impl(hs, it, sizeof(*(hs)))

int hs_it_exist_impl(void *hs, void *it, size_t element_size);

#define hs_put(hs, key_val)                                                     \
    do {                                                                        \
        hs_header_t *header = hs_header(hs);                                    \
        __auto_type k = (key_val);                                              \
        if ((float)(header->size + 1) / header->capacity > 0.7f) {              \
            (hs) = hs_resize_impl((hs), sizeof(*(hs)), sizeof(*(hs)));          \
            header = hs_header(hs);                                             \
        }                                                                       \
        size_t idx = hs_probe_impl((hs), &k, sizeof(*(hs)), sizeof(*(hs)));     \
        if (header->flag[idx] != HS_OCCUPIED) {                                 \
            header->size++;                                                     \
            header->flag[idx] = HS_OCCUPIED;                                    \
            (hs)[idx] = k;                                                      \
        }                                                                       \
    } while (0)
#define hs_get(hs, key_val)                                                     \
    ({                                                                          \
        __auto_type k = (key_val);                                              \
        size_t idx = hs_probe_impl((hs), &k, sizeof(*(hs)), sizeof(*(hs)));     \
        (hs_header(hs)->flag[idx] == HS_OCCUPIED) ? &((hs)[idx]) : NULL;        \
    })
#define hs_remove(hs, key_val)                                                  \
    do {                                                                        \
        __auto_type k = (key_val);                                              \
        size_t idx = hs_probe_impl((hs), &k, sizeof(*(hs)), sizeof(*(hs)));     \
        hs_header_t *header = hs_header(hs);                                    \
        if (header->flag[idx] == HS_OCCUPIED) {                                 \
            header->flag[idx] = HS_TOMBSTONE;                                   \
            header->size--;                                                     \
        }                                                                       \
    } while (0)

#define foreach_hs(type, it, hashset) for (type *(it) = hs_begin(hashset); (it) != hs_end_impl(hashset, sizeof(*(hashset))); (it)++)
