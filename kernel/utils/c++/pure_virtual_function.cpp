#include "../../lib/debug.h"

extern "C" void __cxa_pure_virtual()
{
    panic("pure function call");
}