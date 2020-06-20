#include "interrupt.h"
#include "lib/string.h"
#include "gdt.h"
#include "memory.h"
#include "lib/printk.h"

enum IDT_Descriptor_Type
{
    INTERRUPT = 0x8E,
    TRAP = 0x8F,
    SYSTEM = 0xEF
};

static inline void load_idt(struct IDTR *idt_r)
{
    __asm__("lidt %0" ::"m"(*idt_r));
}

extern "C"
{
    void isr0();
    void isr1();
    void isr2();
    void isr3();
    void isr4();
    void isr5();
    void isr6();
    void isr7();
    void isr8();
    void isr9();
    void isr10();
    void isr11();
    void isr12();
    void isr13();
    void isr14();
    void isr15();
    void isr16();
    void isr17();
    void isr18();
    void isr19();
}

#define CONVERT_ISR_ADDR(i) Phy_To_Virt((void *)(&isr##i))

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

#define set_trap_gate(n, ist) _set_gate(TRAP, n, ist, CONVERT_ISR_ADDR(n));
#define set_intr_gate(n, ist) _set_gate(INTERRUPT, n, ist, CONVERT_ISR_ADDR(n));
#define set_system_gate(n, ist) _set_gate(SYSTEM, n, ist, CONVERT_ISR_ADDR(n));

extern "C" void handle_devide_error(uint64_t rsp, uint64_t error_code)
{
    printk("rsp: %x", rsp);
    printk("divide error\n");
    while (1)
        ;
}

extern "C" void handle_debug_error(uint64_t rsp, uint64_t error_code)
{
    printk("rsp: %x", rsp);
    printk("debug error\n");
    while (1)
        ;
}

extern "C" void handle_mni_error(uint64_t rsp, uint64_t error_code)
{
    printk("rsp: %x", rsp);
    printk("mni error");
    while (1)
        ;
}

extern "C" void handle_nmi_error(uint64_t rsp, uint64_t error_code)
{
    printk("rsp: %x", rsp);
    printk("\n");
    printk("nmi error");
    while (1)
        ;
}
extern "C" void handle_int3_error(uint64_t rsp, uint64_t error_code)
{
    printk("rsp: %x", rsp);
    printk("\n");
    printk("int3 error");
    while (1)
        ;
}

extern "C" void handle_tss_error(uint64_t rsp, uint64_t error_code)
{
    printk("rsp: %x", rsp);
    printk("\n");
    printk("tss error");
    while (1)
        ;
}
extern "C" void handle_page_fault_error(uint64_t rsp, uint64_t error_code)
{
    printk("rsp: %x", rsp);
    printk("\n");
    printk("page_fault error");
    while (1)
        ;
}

void idt_init()
{
    set_intr_gate(0, 1);
    set_intr_gate(1, 1);
    set_intr_gate(2, 1);
    set_intr_gate(3, 1);
    set_intr_gate(4, 1);
    set_intr_gate(5, 1);
    set_intr_gate(6, 1);
    set_intr_gate(7, 1);
    set_intr_gate(8, 1);
    set_intr_gate(9, 1);
    set_intr_gate(10, 1);
    set_intr_gate(11, 1);
    set_intr_gate(12, 1);
    set_intr_gate(13, 1);
    set_intr_gate(14, 1);
    set_intr_gate(15, 1);
    set_intr_gate(16, 1);
    set_intr_gate(17, 1);
    set_intr_gate(18, 1);
    set_intr_gate(19, 1);

    load_idt(&idtr);
    printk("idt_init\n");
}

extern "C" void isr_handler(uint64_t isr_number, uint64_t error_code)
{
    printk("isr: %d error_code: %d\n", isr_number, error_code);
}