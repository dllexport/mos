#include "lib/string.h"
#include "interrupt.h"
#include "gdt.h"
#include "memory.h"
#include "memory_flag.h"
#include "task.h"
#include "lib/printk.h"
#include "lib/debug.h"
extern "C" void start_kernel();

static inline void load_gdt(struct GDTP *p)
{
   __asm__("lgdt %0" ::"m"(*p));
}
void start_kernel()
{
   debug_init();
   using namespace Kernel::VGA;
   console_write_color("Hello, OS kernel!\n", Color::rc_black, Color::rc_white);
   memory_init();

   gdt_init();
   idt_init();

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
