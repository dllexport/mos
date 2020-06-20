#include "debug.h"
#include "../elf/elf_symbol.h"
#include "printk.h"
#include "../memory.h"

constexpr uint64_t start_kernel_base = 0x9f000 + KETNEL_PAGE_OFFSET;

static elf_symbol_t elf_symbol;

static int get_elf_section_of_symbol_table(uint64_t elf_addr)
{
    auto elf_header = (Elf64_Ehdr *)elf_addr;
    auto sheader = (Elf64_Shdr *)((uint8_t *)elf_addr + elf_header->e_shoff);
    printk("e_shoff %d\n", elf_header->e_shoff);
    auto seg_count = elf_header->e_shnum;
    printk("e_shnum %d\n", elf_header->e_shnum);
    printk("e_shentsize %d\n", elf_header->e_shentsize);

    auto find_count = 0;
    for (int i = 0; i < seg_count && find_count < 2; ++i)
    {
        if (sheader[i].sh_type == SHT_SYMTAB)
        {
            printk("find SHT_SYMTAB at: %d\n", sheader[i].sh_offset);
            elf_symbol.symtab = (Elf64_Sym *)((uint8_t *)elf_header + sheader[i].sh_offset);
            elf_symbol.symtabsz = sheader[i].sh_size;
            find_count++;
        }
        if (sheader[i].sh_type == SHT_STRTAB)
        {
            printk("find SHT_SYMTAB at: %d\n", sheader[i].sh_offset);
            elf_symbol.strtab = (const char *)((uint8_t *)elf_header + sheader[i].sh_offset);
            elf_symbol.strtabsz = sheader[i].sh_size;
            find_count++;
        }
    }

    return find_count == 2;
}

void debug_init()
{
    get_elf_section_of_symbol_table(0xffffff8000000000 + 0x70000);
}

const char *elf_lookup_symbol(uint64_t addr)
{
    for (int i = 0; i < (elf_symbol.symtabsz); i++)
    {
        if (ELF64_ST_TYPE(elf_symbol.symtab[i].st_info) != 0x2)
        {
            continue;
        }

        uint64_t size = elf_symbol.symtab[i].st_size;
        uint64_t start = elf_symbol.symtab[i].st_value;
        uint64_t end = start + size;
        
        // 通过函数调用地址查到函数的名字
        if ((addr >= start) && (addr < end))
        {
            return (const char *)((uint8_t *)elf_symbol.strtab + elf_symbol.symtab[i].st_name);
        }
    }

    return nullptr;
}

static void print_stack_trace()
{
    uint64_t *rbp, *rip;

    asm volatile("mov %%rbp, %0"
                 : "=r"(rbp));

    while (*rbp != start_kernel_base)
    {
        rip = rbp + 1;
        printk("   [0x%x] %s\n", *rip, elf_lookup_symbol((uint64_t)Virt_To_Phy(*rip)));
        rbp = (uint64_t *)*rbp;
    }
}

void panic(const char *msg)
{
    printk("*** System panic: %s\n", msg);
    print_stack_trace();
    printk("***\n");

    // 致命错误发生后打印栈信息后停止在这里
    while (1)
        ;
}