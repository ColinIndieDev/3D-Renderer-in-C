#pragma once

#include <stddef.h>

typedef struct {
    size_t capacity;
    size_t size;
    size_t head;
    size_t tail;
} q_header_t;

#define q_header(queue) ((queue) ? ((q_header_t *)(queue) - 1) : NULL)
#define queue_size(queue) ((queue) ? q_header(queue)->size : 0)
#define queue_empty(queue) (q_header(queue)->size == 0)

#define queue_init(q, capacity) queue_init_impl(capacity, sizeof(*(q)))
void *queue_init_impl(size_t capacity, size_t element_size);
void queue_destroy(void *q);

void *queue_resize_impl(void *q, size_t element_size);

#define queue_push(q, val)                                                     \
    do {                                                                       \
        assert(q);                                                             \
        q_header_t *header = q_header(q);                                      \
        if (header->size >= header->capacity) {                                \
            (q) = queue_resize_impl(q, sizeof(*(q)));                          \
            header = q_header(q);                                              \
        }                                                                      \
        (q)[header->head] = (val);                                             \
        header->head = (header->head + 1) % header->capacity;                  \
        header->size++;                                                        \
    } while (0)

#define queue_pop(q)                                                           \
    ({                                                                         \
        assert(q);                                                             \
        q_header_t *header = q_header(q);                                      \
        assert(header->size > 0);                                              \
        header->size--;                                                        \
        __auto_type val = (q)[header->tail];                                   \
        header->tail = (header->tail + 1) % header->capacity;                  \
        val;                                                                   \
    })

#define queue_front(q)                                                         \
    ({                                                                         \
        assert(q);                                                             \
        q_header_t *header = q_header(q);                                      \
        assert(header->size != 0);                                             \
        size_t idx =                                                           \
            header->head == 0 ? header->capacity - 1 : header->head - 1;       \
        &(q)[idx];                                                             \
    })

#define queue_back(q)                                                          \
    ({                                                                         \
        assert(q);                                                             \
        q_header_t *header = q_header(q);                                      \
        assert(header->size != 0);                                             \
        &(q)[header->tail];                                                    \
    })
