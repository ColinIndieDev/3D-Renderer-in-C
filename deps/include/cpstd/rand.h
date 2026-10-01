#pragma once

#include <stddef.h>

typedef struct {
    size_t state;
    size_t inc;
} pcg_state;

#define PERCENT(n) ((n) * 0.01f)

void pcg_rand_seed();
unsigned int pcg_rand();
float pcg_randf();
float pcg_randf_range(float min, float max);
int pcg_rand_range(int min, int max);
int pcg_rand_percent(float probability);
