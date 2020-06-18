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

struct NO_ALIGNMENT TSS_STRUCT
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
    uint64_t reserved3;
    uint16_t reserved4;
    uint16_t io_map_base_addr;
};

class TSS
{
private:
    inline static TSS_STRUCT tss_table;

public:
    static TSS_STRUCT &Get()
    {
        return tss_table;
    }
};

extern "C" void tss_init();
void set_tss(uint64_t rsp0, uint64_t rsp1, uint64_t rsp2, uint64_t ist1, uint64_t ist2, uint64_t ist3,
                        uint64_t ist4, uint64_t ist5, uint64_t ist6, uint64_t ist7);
void set_tss(TSS_STRUCT &new_tss);
