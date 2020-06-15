#pragma once

#include "stdint.h"

extern "C" {
    void printk(char *str);

    uint64_t strlen(char *str);
    int strcmp(const char *s1, const char *s2);

    void *memcpy(void *dest, const void *src, uint64_t n);
    void *memmove(void *dest, const void *src, uint64_t n);
}
