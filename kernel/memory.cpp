#include "memory.h"
#include "lib/string.h"
void memory_init()
{
    using desc = MemoryDescriptor;
    auto p = (struct E820 *)0x90b;
    uint64_t total_physical_memory = 0;
    auto e820s = desc::Get();
    for (int i = 0; i < 32; i++)
    {
        if (p->type == 1)
        {
            // 记录可用区域信息
            e820s[i].address = p->address;
            printk_hex(p->address);
            printk("\n");
            e820s[i].length = p->length;
            printk_hex(p->length);
            printk("\n");
            e820s[i].type = p->type;
        }
        else if (p->type > 4 || p->type < 1)
        {
            desc::SetLength(i);
            break;
        }
        p++;
    }
}