#include "interrupt.h"
#include "gdt.h"
#include "memory.h"
#include "memory_flag.h"
#include "task.h"
#include "lib/printk.h"
#include "lib/debug.h"
#include "timer.h"
extern "C" void start_kernel()
{
   printk("mos kernel startup...\n");
   debug_init();
   memory_init();

   gdt_init();
   idt_init();

   timer_init(100);

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
}
