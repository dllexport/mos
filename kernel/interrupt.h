#pragma once
#include "lib/stdint.h"

constexpr uint8_t idt_count = 24;

struct NO_ALIGNMENT IDT_Descriptor
{
    uint16_t offset_low;    // offset bits 0..15
    uint16_t selector;      // a code segment selector in GDT or LDT
    uint8_t istack;         // bits 0..2 holds Interrupt Stack Table offset, rest of bits zero.
    uint8_t type_attribute; // type and attributes
    uint16_t offset_mid;    // offset bits 16..31
    uint32_t offset_high;   // offset bits 32..63
    uint32_t zero;          // reserved
};

static IDT_Descriptor idt[idt_count];

struct NO_ALIGNMENT IDTR {
    uint16_t limit;
    void* idt_address;
};

static IDTR idtr = {uint16_t(idt_count * 16 - 1), idt};

extern "C" void idt_init();