#pragma once
#include "lib/stdint.h"

extern "C"
{
    void timer_callback();

    void timer_init(uint32_t frequency);
}