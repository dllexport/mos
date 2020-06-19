#include "lib/string.h"
#include "interrupt.h"
#include "gdt.h"
#include "memory.h"
#include "memory_flag.h"
#include "task.h"

int start_kernel();

extern "C" void _start()
{
   start_kernel();
}
static inline void load_gdt(struct GDTP *p)
{
   __asm__("lgdt %0" ::"m"(*p));
}
int start_kernel()
{
   memory_init();
   gdt_init();
   idt_init();
   
   printk("this is mos\n");
   task_init();
   // put_int(9);
   // put_char('\n');
   // put_int(0x00021a3f);
   // put_char('\n');
   // put_int(0x12345678);
   // put_char('\n');
   // put_int(0x00000000);
   while (1)
      ;
   return 0;
}
