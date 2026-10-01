#pragma once

#include <assert.h>

#include "cpbase.h"
#include <stdlib.h>

#define PRIORITY_QUEUE_DEF(type, name)                                         \
    typedef struct {                                                           \
        type val;                                                              \
        f32 priority;                                                          \
    } name##_node;                                                             \
    typedef struct {                                                           \
        name##_node *data;                                                     \
        u32 size;                                                              \
        u32 capacity;                                                          \
    } name;                                                                    \
    void name##_init(name *q, u32 capacity) {                                  \
        assert(q != NULLPTR);                                                  \
        assert(capacity > 0);                                                  \
        q->data = malloc(capacity * sizeof(name##_node));                      \
        q->capacity = capacity;                                                \
        q->size = 0;                                                           \
    }                                                                          \
    void name##_destroy(name *q) {                                             \
        assert(q != NULLPTR);                                                  \
        free(q->data);                                                         \
        q->size = 0;                                                           \
        q->capacity = 0;                                                       \
    }                                                                          \
    void name##_heapify_up(name *q, u32 i) {                                   \
        assert(q != NULLPTR);                                                  \
        while (i > 0) {                                                        \
            u32 parent = (i - 1) / 2;                                          \
            if (q->data[parent].priority >= q->data[i].priority) {             \
                break;                                                         \
            }                                                                  \
            name##_node temp = q->data[parent];                                \
            q->data[parent] = q->data[i];                                      \
            q->data[i] = temp;                                                 \
            i = parent;                                                        \
        }                                                                      \
    }                                                                          \
    void name##_heapify_down(name *q, u32 i) {                                 \
        assert(q != NULLPTR);                                                  \
        while (true) {                                                         \
            u32 left = (2 * i) + 1;                                            \
            u32 right = (2 * i) + 2;                                           \
            u32 smallest = i;                                                  \
            if (left < q->size &&                                              \
                q->data[left].priority > q->data[smallest].priority) {         \
                smallest = left;                                               \
            }                                                                  \
            if (right < q->size &&                                             \
                q->data[right].priority > q->data[smallest].priority) {        \
                smallest = right;                                              \
            }                                                                  \
            if (smallest == i) {                                               \
                break;                                                         \
            }                                                                  \
            name##_node temp = q->data[i];                                     \
            q->data[i] = q->data[smallest];                                    \
            q->data[smallest] = temp;                                          \
            i = smallest;                                                      \
        }                                                                      \
    }                                                                          \
    void name##_push(name *q, type val, f32 priority) {                        \
        assert(q != NULLPTR);                                                  \
        if (q->size >= q->capacity) {                                          \
            name##_node *new_data = cp_realloc(                                \
                q->data, (u64)q->capacity * 2 * sizeof(name##_node));          \
            if (new_data != NULL) {                                            \
                q->data = new_data;                                            \
                q->capacity *= 2;                                              \
            }                                                                  \
        }                                                                      \
        q->data[q->size] = (name##_node){val, priority};                       \
        name##_heapify_up(q, q->size);                                         \
        q->size++;                                                             \
    }                                                                          \
    void name##_pop(name *q, type *val) {                                      \
        assert(q != NULLPTR);                                                  \
        assert(q->size > 0);                                                   \
        *val = q->data[0].val;                                                 \
        q->size--;                                                             \
        q->data[0] = q->data[q->size];                                         \
        name##_heapify_down(q, 0);                                             \
    }                                                                          \
    b8 name##_empty(name *q) {                                                 \
        assert(q != NULLPTR);                                                  \
        return q->size <= 0;                                                   \
    }
