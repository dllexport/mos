#include "gdt.h"
#include "lib/string.h"

static inline void load_gdt (struct GDTP *p)
{
    __asm__ ("lgdt %0" :: "m"(*p));
}

void gdt_init() {
    load_gdt(&gdt_ptr);
    printk("gdt_init\n");
}