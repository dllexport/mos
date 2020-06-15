#include "interrupt.h"
#include "lib/string.h"
#include "gdt.h"

extern "C" void ignore_int();

static inline void load_idt (struct IDTR *idt_r)
{
    __asm__ ("lidt %0" :: "m"(*idt_r));
}

void idt_init() {
    uint64_t handler = reinterpret_cast<uint64_t>(&ignore_int);
    for (int i = 0; i < idt_count; ++i) {
        auto& id = idt[i];
        id.offset_low = (uint16_t)handler;
        id.selector = CODE_SEG;
        id.istack = 0;
        id.type_attribute = 0x8E;
        id.offset_mid = (uint16_t)(handler >> 16);
        id.offset_high = (uint32_t)(handler >> 32);
        id.zero = 0;
    }
    load_idt(&idtr);
    printk("idt_init\n");
}