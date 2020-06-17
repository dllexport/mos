#pragma once
#include "lib/stdint.h"

// the virtual memory that kernel start, mapped to physical 0x0
constexpr uint64_t PAGE_OFFSET = 0xffff800000000000;

constexpr uint16_t ENTRIES_PER_PAGE = 512;

// 4k page needs 12 bits offset 
constexpr uint16_t PAGE_4K_SHIFT = 12;
 
// 4k page size is 4096 bytes
constexpr uint16_t PAGE_4K_SIZE = (1UL << PAGE_4K_SHIFT);

// use to mask the low 12 bits
constexpr uint64_t PAGE_4K_MASK = (~ (PAGE_4K_SIZE - 1));

// round up the addr to 4k boundary
#define PAGE_4K_ALIGN(addr)	(((unsigned long)(addr) + PAGE_4K_SIZE - 1) & PAGE_4K_MASK)


#define Virt_To_Phy(addr)	((uint64_t)(addr) - PAGE_OFFSET)
#define Phy_To_Virt(addr)	((uint64_t*)((uint64_t)(addr) + PAGE_OFFSET))

#define Virt_To_2M_Page(kaddr)	(memory_management_struct.pages_struct + (Virt_To_Phy(kaddr) >> PAGE_2M_SHIFT))
#define Phy_to_2M_Page(kaddr)	(memory_management_struct.pages_struct + ((unsigned long)(kaddr) >> PAGE_2M_SHIFT))

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