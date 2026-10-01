#pragma once

#include <stddef.h>

typedef struct {
    char *cstr;
    size_t size;
} string;

#define STRING_FMT "%.*s"
#define STRING_ARG(str) (int)(str)->size, (str)->cstr

#define STRING_INITIALIZER string_create("")

#define string_cstr(str) ((str)->cstr)
#define string_size(str) ((str)->size)

string *string_create(const char *cstr);
string *string_create_fmt(const char *fmt, ...);
void string_destroy(string *str);
void string_cpy(string *dest, string *src);
void string_cset(string *str, size_t i, char c);
char string_cget(string *str, size_t i);
int string_cmp(string *a, string *b);
void string_fmt(string *str, const char *fmt, ...);

typedef struct {
    const char *cstr;
    size_t size;
} string_view;

void string_view_create(string_view *view, const char *cstr);
