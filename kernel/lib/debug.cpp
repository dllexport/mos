#include "debug.h"
#include "../elf/elf_symbol.h"
#include "printk.h"
#include "../memory.h"

// the rbp when we jump to kernel_start
constexpr uint64_t start_kernel_base = 0x70000 + KETNEL_PAGE_OFFSET;

static elf_symbol_t elf_symbol;

// elf_addr: the addr where the entire kernel is loaded
static int get_elf_section_of_symbol_table(uint64_t elf_addr)
{
    auto elf_header = (Elf64_Ehdr *)elf_addr;
    auto sheader = (Elf64_Shdr *)((uint8_t *)elf_addr + elf_header->e_shoff);
    //printk_raw("e_shoff %d\n", elf_header->e_shoff);
    auto seg_count = elf_header->e_shnum;
    //    printk_raw("e_shnum %d\n", elf_header->e_shnum);
    //printk_raw("e_shentsize %d\n", elf_header->e_shentsize);

    auto find_count = 0;
    for (int i = 0; i < seg_count && find_count < 2; ++i)
    {
        if (sheader[i].sh_type == SHT_SYMTAB)
        {
            // printk_raw("find SHT_SYMTAB at: %d\n", sheader[i].sh_offset);
            elf_symbol.symtab = (Elf64_Sym *)((uint8_t *)elf_header + sheader[i].sh_offset);
            elf_symbol.symtabsz = sheader[i].sh_size;
            find_count++;
        }
        if (sheader[i].sh_type == SHT_STRTAB)
        {
            //printk_raw("find SHT_SYMTAB at: %d\n", sheader[i].sh_offset);
            elf_symbol.strtab = (const char *)((uint8_t *)elf_header + sheader[i].sh_offset);
            elf_symbol.strtabsz = sheader[i].sh_size;
            find_count++;
        }
    }

    return find_count == 2;
}

void debug_init()
{
    printk_raw("debug_init\n");
    get_elf_section_of_symbol_table(0xffffff8000000000 + 0x70000);
}

static const char *elf_lookup_symbol(uint64_t addr)
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

    return "unknown symbol";
}

static void print_stack_trace()
{
    uint64_t *rbp, *rip;

    // load current rbp
    asm volatile("mov %%rbp, %0"
                 : "=r"(rbp));

    // we keep poping stack until reaching start_kernel_base
    for (int i = 0; i < 10 && *rbp != start_kernel_base; ++i)
    // while (*rbp != start_kernel_base)
    {
        // rip is above the rbp because
        // call instruction will push rip and
        // the function will exec
        // push rbp;
        // mov rsp, rbp;
        rip = rbp + 1;
        // rip points to the stack which it's value *rip is the return address
        // the symbol in elf is mapped at the start of 0x1500
        // but we've shifted the address to kernel space
        // and we now executing with virtual address like 0xffff800000001500
        // so translation from *rip to it's original is needed
        auto symbol = elf_lookup_symbol(*rip);
        printk_raw("   [%x] %s\n", *rip, symbol);
        // rbp points to the current position of rbp on stack
        // it's value *rbp points to the previous one on stack
        // we make rbp points to the previous one on stack
        rbp = (uint64_t *)*rbp;
    }
}

void panic(const char *msg)
{
    asm volatile("cli");
    printk_raw("*** System panic: %s\n", msg);
    print_stack_trace();
    printk_raw("***\n");

    // 致命错误发生后打印栈信息后停止在这里
    while (1)
        ;
}