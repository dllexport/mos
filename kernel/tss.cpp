#include "tss.h"
#include "lib/stdint.h"
#include "lib/string.h"
#include "memory.h"
void set_tss(uint64_t rsp0, uint64_t rsp1, uint64_t rsp2, uint64_t ist1, uint64_t ist2, uint64_t ist3,
             uint64_t ist4, uint64_t ist5, uint64_t ist6, uint64_t ist7)
{
    auto& tss = TSS::Get();
    tss.rsp0 = rsp0;
    tss.rsp1 = rsp1;
    tss.rsp2 = rsp2;
    tss.ist1 = ist1;
    tss.ist2 = ist2;
    tss.ist3 = ist3;
    tss.ist4 = ist4;
    tss.ist5 = ist5;
    tss.ist6 = ist6;
    tss.ist7 = ist7;
}

void tss_init()
{
    auto addr = (uint64_t)Phy_To_Virt(0x0000000000007c00);
    set_tss(addr, addr, addr,
            addr, addr, addr,
            addr, addr, addr,
            addr);
}