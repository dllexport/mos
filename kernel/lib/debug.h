#pragma once
#include "stdint.h"
extern "C"
{
    void debug_init();
    
    const char *elf_lookup_symbol(uint32_t addr);

    void panic(const char *msg);

}