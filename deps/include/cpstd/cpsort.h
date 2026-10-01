#pragma once

#include "cpbase.h"

#define INSERTION_THRESHOLD 16

static i32 log2_floor(i32 n) {
    i32 result = 0;
    while (n > 1) {
        n >>= 1;
        result++;
    }
    return result;
}

#define SORT_ALGORITHM_DEF(type, name)                                         \
    void name##_insertion_sort(type *a, u32 size) {                            \
        for (u32 i = 1; i < size; i++) {                                       \
            type key = a[i];                                                   \
            i32 j = (i32)i - 1;                                                \
            while (j >= 0 && a[j] > key) {                                     \
                a[j + 1] = a[j];                                               \
                j -= 1;                                                        \
            }                                                                  \
            a[j + 1] = key;                                                    \
        }                                                                      \
    }                                                                          \
    void name##_swap(type *x, type *y) {                                       \
        type tmp = *x;                                                         \
        *x = *y;                                                               \
        *y = tmp;                                                              \
    }                                                                          \
    void name##_heapify(type *a, i32 n, i32 i) {                               \
        i32 largest = i;                                                       \
        i32 l = (2 * i) + 1;                                                   \
        i32 r = (2 * i) + 2;                                                   \
        if (l < n && a[l] > a[largest]) {                                      \
            largest = l;                                                       \
        }                                                                      \
        if (r < n && a[r] > a[largest]) {                                      \
            largest = r;                                                       \
        }                                                                      \
        if (largest != i) {                                                    \
            name##_swap(&a[i], &a[largest]);                                   \
            name##_heapify(a, n, largest);                                     \
        }                                                                      \
    }                                                                          \
    void name##_heap_sort(type *a, u32 size) {                                 \
        for (i32 i = ((i32)size / 2) - 1; i >= 0; i--) {                       \
            name##_heapify(a, (i32)size, i);                                   \
        }                                                                      \
        for (i32 i = (i32)size - 1; i > 0; i--) {                              \
            name##_swap(&a[0], &a[i]);                                         \
            name##_heapify(a, i, 0);                                           \
        }                                                                      \
    }                                                                          \
    i32 name##_partition(type *a, i32 low, i32 high) {                         \
        type pivot = a[high];                                                  \
        i32 i = low - 1;                                                       \
        for (i32 j = low; j < high; j++) {                                     \
            if (a[j] <= pivot) {                                               \
                i++;                                                           \
                name##_swap(&a[i], &a[j]);                                     \
            }                                                                  \
        }                                                                      \
        name##_swap(&a[i + 1], &a[high]);                                      \
        return i + 1;                                                          \
    }                                                                          \
    void name##_quick_sort(type *a, i32 low, i32 high) {                       \
        if (low < high) {                                                      \
            i32 pi = name##_partition(a, low, high);                           \
            name##_quick_sort(a, low, pi - 1);                                 \
            name##_quick_sort(a, pi + 1, high);                                \
        }                                                                      \
    }                                                                          \
    void name##_introsort(type *a, i32 low, i32 high, i32 depth_limit) {       \
        i32 size = high - low + 1;                                             \
        if (size <= INSERTION_THRESHOLD) {                                     \
            name##_insertion_sort(a + low, size);                              \
            return;                                                            \
        }                                                                      \
        if (depth_limit == 0) {                                                \
            name##_heap_sort(a + low, size);                                   \
            return;                                                            \
        }                                                                      \
        i32 pivot = name##_partition(a, low, high);                            \
        name##_introsort(a, low, pivot - 1, depth_limit - 1);                  \
        name##_introsort(a, pivot + 1, high, depth_limit - 1);                 \
    }                                                                          \
    void name##_sort(type *a, i32 size) {                                      \
        if (size <= 1) {                                                       \
            return;                                                            \
        }                                                                      \
        i32 depth_limit = 2 * log2_floor(size);                                \
        name##_introsort(a, 0, size - 1, depth_limit);                         \
    }
