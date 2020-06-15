#pragma once
#include "lib/stdint.h"

static uint64_t tss[13];

struct NO_ALIGNMENT TSS_PTR {
    uint16_t limit;
    void* idt_address;
};

static TSS_PTR tss_ptr {104, tss};

extern "C" void init_tss();