#include "memory.h"
#include "lib/string.h"
#include "memory_flag.h"
void page_init(Page *page, uint64_t flags);

void memory_init()
{
    using e820_desc = E820Descriptor;
    auto p = (struct E820 *)0xffffff800000090b;
    uint64_t total_physical_memory = 0;
    auto e820s = e820_desc::Get();
    auto valid_e820_count = 0;
    for (int i = 0; i < 32; i++)
    {
        if (p->type == 1)
        {
            // 记录可用区域信息
            e820s[valid_e820_count].address = p->address;
            printk("find zone: ");
            printk_hex(p->address);
            printk("\n");
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
    e820_desc::SetE820Length(valid_e820_count);

    uint64_t pages_count = 0;
    for (int i = 0; i < e820_desc::GetE820Length(); ++i)
    {
        uint64_t start = PAGE_4K_ALIGN(e820s[i].address);
        uint64_t end = ((e820s[i].address + e820s[i].length) >> PAGE_4K_SHIFT) << PAGE_4K_SHIFT;
        if (end <= start)
            continue;
        pages_count += (end - start) >> PAGE_4K_SHIFT;
    }
    e820_desc::SetAvailable4kPageCount(pages_count);

    auto &memory_desc = MemoryDescriptor::GetInstance();

    memory_desc.mem_bitmap = reinterpret_cast<uint8_t *>(KETNEL_MEM_BITMAP_OFFSET);
    memory_desc.mem_bitmap_size = pages_count / 8;

    memset(memory_desc.mem_bitmap, 0, memory_desc.mem_bitmap_size);

    memory_desc.pages = reinterpret_cast<Page *>(memory_desc.mem_bitmap + memory_desc.mem_bitmap_size);
    memory_desc.pages_count = pages_count;
    memory_desc.pages_size = pages_count * sizeof(Page);
    memset(memory_desc.pages, 0x0, sizeof(Page) * pages_count);

    memory_desc.zones = reinterpret_cast<Zone *>(memory_desc.mem_bitmap + memory_desc.mem_bitmap_size + memory_desc.pages_size);
    memory_desc.zones_count = valid_e820_count;
    memory_desc.zones_size = valid_e820_count * sizeof(Zone);
    memset(memory_desc.zones, 0x0, sizeof(Zone) * valid_e820_count);

    for (int i = 0; i < e820_desc::GetE820Length(); ++i)
    {

        uint64_t start = PAGE_4K_ALIGN(e820s[i].address);
        uint64_t end = ((e820s[i].address + e820s[i].length) >> PAGE_4K_SHIFT) << PAGE_4K_SHIFT;
        if (end <= start)
            continue;

        auto &z = memory_desc.zones[i];
        z.zone_start_address = start;
        z.zone_end_address = end;
        z.page_used = 0;
        z.page_free = (end - start) >> PAGE_4K_SHIFT;
        z.total_reference_count = 0;
        z.attribute = 0;

        z.page_length = (end - start) >> PAGE_4K_SHIFT;
        z.page_group = memory_desc.pages;

        for (int j = 0; j < z.page_length; ++j)
        {
            auto pages = memory_desc.pages + memory_desc.zones[i - 1].page_length + j;
            pages->zones = &z;
            pages->physical_address = start + PAGE_4K_SIZE * j;
            pages->attributes = 0;
            pages->reference_count = 0;
            pages->age = 0;
            auto which_page = pages->physical_address / PAGE_4K_SIZE;
            memory_desc.bitmap_set(which_page, 1);
        }
    }

    auto end_page_addr = PAGE_4K_ALIGN(memory_desc.EndAddress());
    auto page_used_count = (end_page_addr - KETNEL_PAGE_OFFSET) / PAGE_4K_SIZE;

    for (int i = 0; i < page_used_count; ++i)
    {
        page_init(&memory_desc.pages[i], PG_PTable_Maped | PG_Kernel_Init | PG_Active | PG_Kernel);
    }

    auto cr3 = Get_CR3();
   *(uint64_t*)cr3 = 0UL;
   flush_tlb();

}

void page_init(Page *page, uint64_t flags)
{
    if (!page->attributes)
    {
        auto &memory_desc = MemoryDescriptor::GetInstance();
        auto page_offset = page - memory_desc.pages;
        memory_desc.bitmap_set(page_offset, 1);
        page->attributes = flags;
        page->reference_count++;
        page->zones->page_used++;
        page->zones->page_free--;
        page->zones->total_reference_count++;
    }
    else if ((page->attributes & PG_Referenced) || (page->attributes & PG_K_Share_To_U) || (flags & PG_Referenced) || (flags & PG_K_Share_To_U))
    {
        page->attributes |= flags;
        page->reference_count++;
        page->zones->total_reference_count++;
    }
    else
    {
        auto &memory_desc = MemoryDescriptor::GetInstance();
        auto page_offset = page - memory_desc.pages;
        memory_desc.bitmap_set(page_offset, 1);
        page->attributes |= flags;
    }
}