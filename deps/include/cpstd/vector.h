#pragma once

#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    size_t size;
    size_t capacity;
} vec_header_t;

#define vec_header(arr) ((arr) ? ((vec_header_t *)(arr) - 1) : NULL)
#define vec_size(arr) ((arr) ? vec_header(arr)->size : 0)
#define vec_empty(vec) (vec_header(vec)->size == 0)

#define vec_init(arr, capacity) vec_init_impl(sizeof(*(arr)), capacity)
void *vec_init_impl(size_t element_size, size_t capacity);
void vec_destroy(void *arr);
void vec_clear(void *arr);

#define vec_push(arr, x)                                                       \
    do {                                                                       \
        __auto_type tmp = (x);                                                 \
        (arr) = vec_push_impl((arr), &tmp, sizeof(*(arr)));                    \
    } while (0)
void *vec_push_impl(void *arr, void *val, size_t element_size);
#define vec_push_front(arr, x)                                                 \
    do {                                                                       \
        __auto_type tmp = (x);                                                 \
        (arr) = vec_push_front_impl((arr), &tmp, sizeof(*(arr)));              \
    } while (0)
void *vec_push_front_impl(void *arr, void *val, size_t element_size);
void vec_pop(void *arr);
#define vec_pop_front(arr) vec_pop_front_impl((arr), sizeof(*(arr)))
void vec_pop_front_impl(void *arr, size_t element_size);
#define vec_delete(arr, i) vec_delete_impl((arr), (i), sizeof(*(arr)))
void vec_delete_impl(void *arr, size_t i, size_t element_size);

void *vec_front(void *arr);
#define vec_back(arr) vec_back_impl((arr), sizeof(*(arr)))
void *vec_back_impl(void *arr, size_t element_size);

void *vec_begin(void *arr);
#define vec_end(arr) vec_end_impl((arr), sizeof(*(arr)))
void *vec_end_impl(void *arr, size_t element_size);

#define foreach_vec(it, arr)                                                   \
    for (__auto_type(it) = (arr); (it) != vec_end(arr); ++(it))

#define vec_erase_if(it, arr, cond)                                            \
    do {                                                                       \
        size_t write_idx = 0;                                                  \
        for (__auto_type(it) = (arr); (it) != vec_end(arr); ++(it)) {          \
            if (!(cond)) {                                                     \
                (arr)[write_idx++] = *(it);                                    \
            }                                                                  \
        }                                                                      \
        vec_header(arr)->size = write_idx;                                     \
    } while (0)
