#pragma once

#include "stdint.h"

extern "C" {
    void printk(char *str);
    void printk_hex(uint64_t hex);
    void printkf(char *str, ...);
    uint64_t strlen(char *str);
    int strcmp(const char *s1, const char *s2);
    void memset(void *addr, uint8_t val, uint64_t size);    
    void *memcpy(void *dest, const void *src, uint64_t n);
    void *memmove(void *dest, const void *src, uint64_t n);
}
