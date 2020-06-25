#include "memory.h"
#include "lib/string.h"
#include "lib/printk.h"
#include "lib/debug.h"
#include "memory_flag.h"
#include "kernel_memory_map.h"
#include "buddy_system.h"

void set_page_attributes(Page *page, uint64_t flags);
Page *alloc_pages(uint8_t buddy_index, uint32_t pages_count, uint64_t page_flags);

static E820 e820s[16];
static uint32_t e820_length;
static uint64_t total_available_4k_page_length;

void memory_init()
{
    auto cr3 = Get_CR3();
    *(uint64_t *)cr3 = 0UL;
    flush_tlb();
    printk("kernel text_start %p\n", &_kernel_text_start);
    printk("kernel text_ends %p\n", &_kernel_text_start);
    printk("kernel data_start %p\n", &_kernel_data_start);
    printk("kernel data_ends %p\n", &_kernel_data_end);
    printk("kernel rodata_start %p\n", &_kernel_rodata_start);
    printk("kernel rodata_ends %p\n", &_kernel_rodata_end);
    printk("kernel bss_start %p\n", &_kernel_bss_start);
    printk("kernel bss_end %p\n", &_kernel_bss_end);
    printk("kernel end %p\n", &_kernel_end);
    // printk_while("kernel end round %p\n", PAGE_4K_ALIGN(&_kernel_end));
    auto p = (struct E820 *)0xffffff800000090b;
    uint64_t total_physical_memory = 0;
    auto valid_e820_count = 0;
    for (int i = 0; i < 16; i++)
    {
        if (p->type == 1)
        {
            // 记录可用区域信息
            e820s[valid_e820_count].address = p->address;
            e820s[valid_e820_count].length = p->length;

            e820s[valid_e820_count].type = p->type;
            valid_e820_count += 1;
        }
        else if (p->type > 4 || p->type < 1)
        {
            break;
        }
        p++;
    }
    e820_length = valid_e820_count;

    uint64_t pages_count = 0;
    for (int i = 0; i < e820_length; ++i)
    {
        uint64_t start = PAGE_4K_ALIGN(e820s[i].address);
        uint64_t end = ((e820s[i].address + e820s[i].length) >> PAGE_4K_SHIFT) << PAGE_4K_SHIFT;
        printk("zone %d start: %p end: %p\n", i, start, end);
        if (end <= start)
            continue;

        pages_count += (end - start) >> PAGE_4K_SHIFT;
    }
    total_available_4k_page_length = pages_count;

    auto &memory_desc = MemoryDescriptor::GetInstance();

    auto buddies_start_address = KETNEL_MEM_BUDDY_SYSTEM_OFFSET;

    memory_desc.buddys_count = valid_e820_count;
    // memory_desc.zones_size = valid_e820_count * sizeof(Zone);
    // memset(memory_desc.zones, 0x0, sizeof(Zone) * valid_e820_count);

    for (int i = 0; i < e820_length; ++i)
    {
        uint64_t start = PAGE_4K_ALIGN(e820s[i].address);
        uint64_t end = ((e820s[i].address + e820s[i].length) >> PAGE_4K_SHIFT) << PAGE_4K_SHIFT;
        if (end <= start)
            continue;
        auto &z = memory_desc.buddys[i];
        z = reinterpret_cast<BuddySystem *>(i == 0 ? buddies_start_address : buddies_start_address + memory_desc.buddys[i - 1]->BuddySystemSize());
        z->physical_start_address = start;
        z->physical_end_address = end;
        z->attribute = 0;
        z->total_reference_count = 0;
        z->Construct((end - start) >> PAGE_4K_SHIFT);
        printk("z.pages_free %d\n", z->FreePagesCount());
        buddies_start_address += z->BuddySystemSize();
        printk("buddysystem %d start %p, size: %d bytes\n", i, z, z->BuddySystemSize());
    }

    auto buddies_end_address = buddies_start_address;
    auto pages_start_address = buddies_end_address;
    // init pages for each buddies
    for (int i = 0; i < memory_desc.buddys_count; ++i)
    {
        auto z = memory_desc.buddys[i];
        auto &pages = z->pages;
        pages = reinterpret_cast<Page *>(i == 0 ? pages_start_address : pages_start_address + memory_desc.buddys[i - 1]->PageSize());
        printk("zone: %d's pages start at %p size: %d\n", i, pages, z->FreePagesCount());
        for (int j = 0; j < z->FreePagesCount(); ++j)
        {
            pages[j].buddy = z;
            pages[j].physical_address = z->physical_start_address + PAGE_4K_SIZE * j;
            pages[j].attributes = 0;
            pages[j].reference_count = 0;
            pages[j].age = 0;
        }
    }
    auto page_used_count = (PAGE_4K_ALIGN(&_kernel_end) - (uint64_t)&_kernel_text_start) / PAGE_4K_SIZE;
    printk("kernel_page_used: %d\n", page_used_count);

    for (int i = 0; i < memory_desc.buddys_count && i <= 1; ++i)
    {
        auto z = memory_desc.buddys[i];
        auto flags = PG_PTable_Maped | PG_Kernel_Init | PG_Active | PG_Kernel;
        if (z->physical_start_address == 0x0)
        {
            // for 0-1M memory we reseved all space
            alloc_pages(BUDDY_ZONE_LOW_INDEX, z->FreePagesCount(), flags);
        }
        else
        {
            // for kernel memory we reseved page_used_count
            alloc_pages(BUDDY_ZONE_NORMAL_INDEX, page_used_count, flags);
        }
    }
}

void set_page_attributes(Page *page, uint64_t flags)
{
    if (!page->attributes)
    {
        page->attributes = flags;
        page->reference_count++;
        page->buddy->total_reference_count++;
    }
    else if ((page->attributes & PG_Referenced) || (page->attributes & PG_K_Share_To_U) || (flags & PG_Referenced) || (flags & PG_K_Share_To_U))
    {
        page->attributes |= flags;
        page->reference_count++;
        page->buddy->total_reference_count++;
    }
    else
    {
        page->attributes |= flags;
    }
}

Page *alloc_pages(uint8_t buddy_index, uint32_t pages_count, uint64_t page_flags)
{
    auto &memory_desc = MemoryDescriptor::GetInstance();
    auto &z = memory_desc.buddys[buddy_index];

    auto allocated_index = z->AllocPages(pages_count);

    if (allocated_index < 0)
        return nullptr;
    auto allocated_pages = &z->GetPages()[allocated_index];

    printk("alloc zone: %d page idx: %d start at: %p physical: %p count: %d end: %p\n", buddy_index, allocated_index, allocated_pages, allocated_pages->physical_address, pages_count, allocated_pages->physical_address + PAGE_4K_SIZE * pages_count);

    for (int64_t i = 0; i < pages_count; ++i)
    {
        set_page_attributes(&allocated_pages[i], page_flags);
    }
    return allocated_pages;
}
