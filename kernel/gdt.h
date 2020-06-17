#pragma once
#include "lib/stdint.h"
#include "memory.h"
#define CODE_SEG 0x0008
#define DATA_SEG 0x0010
#define TSS_SEG 0x0028

static uint64_t gdt_table[8] = {
    0x0000000000000000,
    0x0020980000000000,
    0x0000920000000000,
    0x0020f80000000000,
    0x0000f20000000000
};

struct NO_ALIGNMENT GDTP {
    uint16_t limit;
    void* gdt_address;
};

// we must translate gdt_table address to virtual 
// direct mapping 0x00000... -> 0x00000 will be deleted later
static GDTP gdt_ptr = {uint16_t(64 - 1), Phy_To_Virt(gdt_table)};

extern "C" void gdt_init();
