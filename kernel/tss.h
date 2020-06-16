#pragma once
#include "lib/stdint.h"

// place it in gdt
struct NO_ALIGNMENT GDT_TSS
{
    uint16_t limit;
    uint8_t low[3];
    uint16_t attr;
    uint8_t high[5];
    uint32_t reserved;
};

struct NO_ALIGNMENT TSS_TABLE
{
    uint32_t reserved1;
    uint64_t rsp0 = 0xffff800000007c00;
    uint64_t rsp1 = 0xffff800000007c00;
    uint64_t rsp2 = 0xffff800000007c00;
    uint64_t reserved2;
    uint64_t ist1 = 0xffff800000007c00;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint32_t reserved3[3];
};

class TSS
{
private:
    inline static TSS_TABLE tss_table;
public:
    static TSS_TABLE& Get() {
        return tss_table;
    }
};

extern "C" void tss_init();