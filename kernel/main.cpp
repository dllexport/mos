#include "lib/print.h"

class TestClass {
public:
   TestClass() {
      fun();
   }
   ~TestClass() {
      fun();
   }
   void fun() {
      put_str("I am kernel\n");
   }
};

int main() {
   {
         TestClass t;
   }
   put_int(0);
   put_char('\n');
   put_int(9);
   put_char('\n');
   put_int(0x00021a3f);
   put_char('\n');
   put_int(0x12345678);
   put_char('\n');
   put_int(0x00000000);
   while(1);
   return 0;
}

