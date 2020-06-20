#pragma once
#include "stdint.h"
#include "attribute_macros.h"

// write value to port (1 byte)
ALWAYS_INLINE void outb(uint16_t port, uint8_t value)
{
    asm volatile("outb %1, %0"
                 :
                 : "dN"(port), "a"(value));
}

ALWAYS_INLINE uint8_t inb(uint16_t port)
{
    uint8_t ret;

    asm volatile ("inb %1, %0" : "=a" (ret) : "dN" (port));

    return ret;
}

ALWAYS_INLINE uint16_t inw(uint16_t port)
{
    uint16_t ret;

    asm volatile ("inw %1, %0" : "=a" (ret) : "dN" (port));

    return ret;
}