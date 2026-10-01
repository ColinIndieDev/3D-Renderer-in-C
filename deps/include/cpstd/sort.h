#pragma once 

#include <stddef.h>

void insertion_sort(void *a, size_t s, size_t n, int (*cmp)(const void *, const void *));
void heap_sort(void *a, size_t s, size_t n, int (*cmp)(const void *, const void *));
void quick_sort(void *a, size_t s, size_t n, int (*cmp)(const void *, const void *));
void sort(void *a, size_t s, size_t n, int (*cmp)(const void *, const void *));
