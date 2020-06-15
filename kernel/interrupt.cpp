#include "interrupt.h"
#include "lib/string.h"
#include "gdt.h"

static void ignore_int() {
    printk("ignore_int\n");
}

void idt_setup() {
    uint64_t handler = reinterpret_cast<uint64_t>(&ignore_int);
    for (int i = 0; i < 32; ++i) {
        auto& id = idt[i];
        id.offset_low = (uint16_t)handler;
        id.selector = CODE_SEG;
        id.istack = 0;
        id.type_attribute = 0x8E;
        id.offset_mid = (uint16_t)(handler >> 16);
        id.offset_high = (uint32_t)(handler >> 32);
        id.zero = 0;
    }
    
    printk("idt_setup\n");
}