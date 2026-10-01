#pragma once

#include <assert.h>
#include <stdint.h>
#include <string.h>

typedef enum : uint8_t {
    HASH_EMPTY = 0,
    HASH_OCCUPIED,
    HASH_TOMBSTONE
} hm_entry_state_t;

typedef struct {
    uint8_t *flag;
    size_t size;
    size_t capacity;
} hm_header_t;

#define HM_STRING_KEY size_t

#define hm_header(hm) ((hm_header_t *)(hm) - 1)
#define hm_size(hm) ((hm) ? hm_header(hm)->size : 0)
#define hm_empty(hm) (hm_header(hm)->size == 0)

#define hm_init(hm, capacity) hm_init_impl(sizeof(*(hm)), capacity)
void *hm_init_impl(size_t element_size, size_t capacity);
void hm_destroy(void *hm);

static size_t hm_hash(void *key, size_t key_size) {
    assert(key);
    size_t hash = 14695981039346656037ULL;
    uint8_t *bytes = (uint8_t *)key;
    for (size_t i = 0; i < key_size; i++) {
        hash ^= bytes[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

static size_t hm_string_to_key(char *str) {
    size_t hash = 0xcbf29ce484222325ULL;
    while (*str) {
        hash ^= (unsigned char)*str++;
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

static size_t hm_probe_impl(void *hm, void *key, size_t key_size,
                            size_t element_size) {
    assert(hm);
    assert(key);
    hm_header_t *header = hm_header(hm);
    size_t hash = hm_hash(key, key_size);
    size_t idx = hash % header->capacity;
    size_t first_tomb = (size_t)-1;

    char *data = (char *)hm;

    while (1) {
        if (header->flag[idx] == HASH_EMPTY) {
            return first_tomb != (size_t)-1 ? first_tomb : idx;
        }
        if (header->flag[idx] == HASH_TOMBSTONE) {
            if (first_tomb == (size_t)-1) {
                first_tomb = idx;
            }
        } else if (header->flag[idx] == HASH_OCCUPIED) {
            void *cur_key = data + (idx * element_size);
            if (memcmp(cur_key, key, key_size) == 0) {
                return idx;
            }
        }
        idx = (idx + 1) % header->capacity;
    }
}

void *hm_resize_impl(void *hm, size_t element_size, size_t key_size);

void *hm_begin(void *hm);
#define hm_end(hm) hm_end_impl(hm, sizeof(*(hm)))
void *hm_end_impl(void *hm, size_t element_size);

#define hm_it_exist(hm, it) hm_it_exist_impl(hm, it, sizeof(*(hm)))
int hm_it_exist_impl(void *hm, void *it, size_t element_size);

#define hm_put(hm, key_val, val_val)                                           \
    do {                                                                       \
        hm_header_t *header = hm_header(hm);                                   \
        if ((float)(header->size + 1) / header->capacity > 0.7f) {             \
            (hm) = hm_resize_impl(hm, sizeof(*(hm)), sizeof((hm)->key));       \
            header = hm_header(hm);                                            \
        }                                                                      \
        __auto_type k = (key_val);                                             \
        size_t idx = hm_probe_impl(hm, &k, sizeof((hm)->key), sizeof(*(hm)));  \
        if (header->flag[idx] != HASH_OCCUPIED) {                              \
            header->size++;                                                    \
            header->flag[idx] = HASH_OCCUPIED;                                 \
        }                                                                      \
        (hm)[idx].key = k;                                                     \
        (hm)[idx].value = (val_val);                                           \
    } while (0)
#define hm_get(hm, key_val)                                                    \
    ({                                                                         \
        __auto_type k = (key_val);                                             \
        size_t idx = hm_probe_impl(hm, &k, sizeof((hm)->key), sizeof(*(hm)));  \
        (hm_header(hm)->flag[idx] == HASH_OCCUPIED) ? &((hm)[idx].value)       \
                                                    : NULL;                    \
    })
#define hm_get_element(hm, key_val)                                            \
    ({                                                                         \
        __auto_type k = (key_val);                                             \
        size_t idx = hm_probe_impl(hm, &k, sizeof((hm)->key), sizeof(*(hm)));  \
        (hm_header(hm)->flag[idx] == HASH_OCCUPIED) ? &((hm)[idx]) : NULL;     \
    })
#define hm_remove(hm, key_val)                                                 \
    do {                                                                       \
        __auto_type k = (key_val);                                             \
        size_t idx = hm_probe_impl(hm, &k, sizeof((hm)->key), sizeof(*(hm)));  \
        hm_header_t *header = hm_header(hm);                                   \
        if (header->flag[idx] == HASH_OCCUPIED) {                              \
            header->flag[idx] = HASH_TOMBSTONE;                                \
            header->size--;                                                    \
        }                                                                      \
    } while (0)

#define foreach_hm(type, it, hashm)                                            \
    for (type * (it) = hm_begin(hashm);                                        \
         (it) != hm_end_impl(hashm, sizeof(*(hashm))); (it)++)

#define HM_CREATE_ENTRY(key_t, val_t, name)                                    \
    typedef struct {                                                           \
        key_t key;                                                             \
        val_t value;                                                           \
    }(name);
