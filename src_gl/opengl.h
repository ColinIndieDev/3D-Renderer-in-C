#pragma once

#include <stdint.h>

char *shader_read_file(const char *path);

void init_gl(uint32_t width, uint32_t height);
void render_gl(uint32_t width, uint32_t height, uint8_t *pixels);
void resize_gl(uint32_t width, uint32_t height);
