#pragma once
#include "lib/stdint.h"

#define PAGE_GDT_SHIFT	39
#define PAGE_4K_SHIFT	12

#define PAGE_4K_SIZE	(1UL << PAGE_4K_SHIFT)

#define PAGE_4K_MASK	(~ (PAGE_4K_SIZE - 1))

#define PAGE_4K_ALIGN(addr)	(((unsigned long)(addr) + PAGE_4K_SIZE - 1) & PAGE_4K_MASK)


struct NO_ALIGNMENT E820
{
    uint64_t address;
    uint64_t length;
    uint32_t type;
};

class MemoryDescriptor
{
private:
    inline static E820 e820[32];
    inline static uint32_t length;
public:
    static auto& Get() {
        return e820;
    }
    static const uint32_t& GetLength() {
        return length;
    }
    static void SetLength(uint32_t len) {
        MemoryDescriptor::length = len;
    }
};
extern "C" void memory_init();