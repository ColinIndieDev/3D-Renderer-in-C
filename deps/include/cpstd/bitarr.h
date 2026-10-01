#pragma once

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    size_t bits;
} bit_header_t;

#define bit_header(arr) ((bit_header_t *)(arr) - 1)
#define bit_arr_bits(arr) ((arr) ? bit_header(arr)->bits : 0)

uint8_t *bit_arr_init(size_t bits);
void bit_arr_destroy(uint8_t *arr);

void bit_arr_set(uint8_t *arr, size_t i);
void bit_arr_clear(uint8_t *arr, size_t i);
uint8_t bit_arr_get(const uint8_t *arr, size_t i);
