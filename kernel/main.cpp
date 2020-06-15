#include "lib/string.h"

int start_kernel();

extern "C" void _start() {
   start_kernel();
}

int start_kernel() {
   // put_int(0);
   printk("asdf");
   // put_int(9);
   // put_char('\n');
   // put_int(0x00021a3f);
   // put_char('\n');
   // put_int(0x12345678);
   // put_char('\n');
   // put_int(0x00000000);
   while(1);
   return 0;
}
