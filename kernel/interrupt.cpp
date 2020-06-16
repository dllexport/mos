#include "interrupt.h"
#include "lib/string.h"
#include "gdt.h"

enum IDT_Descriptor_Type
{
    INTERRUPT = 0x8E,
    TRAP = 0x8F,
    SYSTEM = 0xEF
};

extern "C" void ignore_int();

static inline void load_idt(struct IDTR *idt_r)
{
    __asm__("lidt %0" ::"m"(*idt_r));
}

static inline void _set_gate(IDT_Descriptor_Type type, unsigned int n, unsigned char ist, void *addr)
{
    uint64_t handler = reinterpret_cast<uint64_t>(addr);
    auto &id = idt[n];
    id.offset_low = (uint16_t)handler;
    id.selector = CODE_SEG;
    id.istack = 1;
    id.type_attribute = type;
    id.offset_mid = (uint16_t)(handler >> 16);
    id.offset_high = (uint32_t)(handler >> 32);
    id.zero = 0;
}

static inline void set_intr_gate(unsigned int n, unsigned char ist, void *addr)
{
    _set_gate(INTERRUPT, n, ist, addr);
}

static inline void set_trap_gate(unsigned int n, unsigned char ist, void *addr)
{
    _set_gate(TRAP, n, ist, addr);
}

static inline void set_system_gate(unsigned int n, unsigned char ist, void *addr)
{
    _set_gate(SYSTEM, n, ist, addr);
}

extern "C" void handle_devide_error(uint64_t rsp, uint64_t error_code)
{
    printk_hex(rsp);
    printk("\n");
    printk("divide error");
    while (1);
}

extern "C" void handle_debug_error(uint64_t rsp, uint64_t error_code)
{
    printk_hex(rsp);
    printk("\n");
    printk("debug error");
    while (1);
}

extern "C" void handle_mni_error(uint64_t rsp, uint64_t error_code)
{
    printk_hex(rsp);
    printk("\n");
    printk("mni error");
    while (1);
}

extern "C" void handle_nmi_error(uint64_t rsp, uint64_t error_code)
{
    printk_hex(rsp);
    printk("\n");
    printk("nmi error");
    while (1);
}
extern "C" void handle_int3_error(uint64_t rsp, uint64_t error_code)
{
    printk_hex(rsp);
    printk("\n");
    printk("int3 error");
    while (1);
}

extern "C" void handle_tss_error(uint64_t rsp, uint64_t error_code)
{
    printk_hex(rsp);
    printk("\n");
    printk("tss error");
    while (1);
}
extern "C" void handle_page_fault_error(uint64_t rsp, uint64_t error_code)
{
    printk_hex(rsp);
    printk("\n");
    printk("page_fault error");
    while (1);
}
extern "C" void divide_error();
extern "C" void debug_error();
extern "C" void nmi_error();
extern "C" void int3_error();
extern "C" void page_fault_error();

void idt_init()
{
    set_trap_gate(0, 1, reinterpret_cast<void *>(&divide_error));
    set_trap_gate(1, 1, reinterpret_cast<void *>(&debug_error));
    set_intr_gate(2, 1, reinterpret_cast<void *>(&nmi_error));
    set_system_gate(3, 1, reinterpret_cast<void *>(&int3_error));
    set_trap_gate(14, 1, reinterpret_cast<void *>(&page_fault_error));
    load_idt(&idtr);
    printk("idt_init\n");
}