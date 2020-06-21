#pragma once
#include "lib/stdint.h"
#include "lib/string.h"

// the virtual memory that kernel start, mapped to physical 0x0
constexpr uint64_t PAGE_OFFSET = 0xffffff8000000000;
constexpr uint64_t KETNEL_PAGE_OFFSET = PAGE_OFFSET;                         // alias
constexpr uint64_t KETNEL_MEM_BITMAP_OFFSET = KETNEL_PAGE_OFFSET + 0x200000; // alias

constexpr uint16_t ENTRIES_PER_PAGE = 512;

// 4k page needs 12 bits offset
constexpr uint16_t PAGE_4K_SHIFT = 12;

// 4k page size is 4096 bytes
constexpr uint16_t PAGE_4K_SIZE = (1UL << PAGE_4K_SHIFT);

// use to mask the low 12 bits
constexpr uint64_t PAGE_4K_MASK = (~(PAGE_4K_SIZE - 1));

// round up the addr to 4k boundary
#define PAGE_4K_ALIGN(addr) (((uint64_t)(addr) + PAGE_4K_SIZE - 1) & PAGE_4K_MASK)

#define Virt_To_Phy(addr) ((uint8_t *)(addr)-PAGE_OFFSET)
#define Phy_To_Virt(addr) ((uint8_t *)((uint8_t *)(addr) + PAGE_OFFSET))

#define Virt_To_2M_Page(kaddr) (memory_management_struct.pages_struct + (Virt_To_Phy(kaddr) >> PAGE_2M_SHIFT))
#define Phy_to_2M_Page(kaddr) (memory_management_struct.pages_struct + ((unsigned long)(kaddr) >> PAGE_2M_SHIFT))

#define flush_tlb()               \
    do                            \
    {                             \
        unsigned long tmpreg;     \
        __asm__ __volatile__(     \
            "movq	%%cr3,	%0	\n\t" \
            "movq	%0,	%%cr3	\n\t" \
            : "=r"(tmpreg)        \
            :                     \
            : "memory");          \
    } while (0)

inline uint64_t Get_CR3()
{
    void *addr;
    __asm__ __volatile__(
        "movq	%%cr3,	%0	\n\t"
        : "=r"(addr)
        :
        : "memory");
    return reinterpret_cast<uint64_t>(addr);
}

struct NO_ALIGNMENT E820
{
    uint64_t address;
    uint64_t length;
    uint32_t type;
};

struct Page;

struct Zone
{
    Page *page_group;
    uint64_t page_length;
    uint64_t zone_start_address;
    uint64_t zone_end_address;
    uint64_t attribute;

    uint64_t page_used;
    uint64_t page_free;
    uint64_t total_reference_count;
};

struct Page
{
    Zone *zones;
    uint64_t physical_address;
    uint32_t reference_count;
    uint16_t attributes;
    uint16_t age;
};

struct MemoryDescriptor
{
    // bitmap for whole physical memory
    uint8_t *mem_bitmap;
    // bitmap size in bytes
    uint64_t mem_bitmap_size;

    // test if the number bit_idx bit is set
    bool bitmap_test(uint32_t bit_idx)
    {
        uint32_t byte_index = bit_idx / 8; // 位图中bit_idx所在的byte索引
        uint32_t bit_offset = bit_idx % 8; // 该bit在 1byte中的偏移量

        return this->mem_bitmap[byte_index] & (1 << bit_offset);
    }

    // alloc size physical memory, contiguous
    int bitmap_alloc(uint32_t size)
    {
        uint32_t idx_byte = 0;

        while ((0xff == this->mem_bitmap[idx_byte]) && (idx_byte < this->mem_bitmap_size))
        { // 判断该字节是否全部为1, 且位图大小必须 小于 要申请的空间
            // 代表该字节已全部占用，继续向下字节查找
            ++idx_byte;
        }

        // ASSERT(idx_byte < btmp->btmp_bytes_len);

        // 内存池中找不到可用空间
        if (idx_byte == this->mem_bitmap_size)
        {
            return -1;
        }

        // 在idx_byte中有空闲位后，对该byte进行逐bit比对，直到有连续的bits=cnt
        int idx_bit = 0;
        while ((uint8_t)(1 << idx_bit) & this->mem_bitmap[idx_byte])
        { // 找到idx_byte中为0的bit所在的位置
            ++idx_bit;
        }

        int bit_idx_start = idx_byte * 8 + idx_bit; // 空闲位在位图中的bit偏移量

        if (size == 1)
        {
            return bit_idx_start;
        }

        // 位图中还剩余的bits数
        uint32_t bit_left = (this->mem_bitmap_size * 8 - bit_idx_start);

        uint32_t next_bit = bit_idx_start + 1;
        uint32_t count = 1; // 记录找到的空闲位数量

        bit_idx_start = -1;
        // 在剩余的空间中继续查找，直到有连续的bits=cnt
        while (bit_left-- > 0)
        {
            if (!this->bitmap_test(next_bit))
            {
                ++count;
            }
            else
            {
                count = 0;
            }

            if (count == size)
            {
                bit_idx_start = next_bit - size + 1;
            }

            ++next_bit;
        }

        return bit_idx_start;
    }

    // set the number bit_idx bit to value
    void bitmap_set(uint32_t bit_idx, int8_t value)
    {
        // ASSERT(value == 0 || value == 1);

        uint32_t byte_idx = bit_idx / 8;
        uint32_t bit_odd = bit_idx % 8;

        if (value)
        {
            this->mem_bitmap[byte_idx] |= (1 << bit_odd);
        }
        else
        {
            this->mem_bitmap[byte_idx] &= ~(1 << bit_odd);
        }
    }

    // page entry for the entire physical memory
    Page *pages;
    // the whole size in bytes that pages takes, contiguous
    uint64_t pages_size;
    // same as pages_size / sizeof(Page)
    uint64_t pages_count;

    // zone entry for the entire physical memory
    Zone *zones;
    // the whole size in bytes that pages takes, contiguous
    uint64_t zones_size;
    // same as zones_size / sizeof(Zone)
    uint64_t zones_count;

    static MemoryDescriptor &GetInstance()
    {
        static MemoryDescriptor instance;
        return instance;
    }

    uint64_t EndAddress()
    {
        return (uint64_t) & this->zones[this->zones_count];
    }
};

extern "C"
{
    void memory_init();
    Page *alloc_pages(uint32_t page_count, uint64_t page_flags);
}