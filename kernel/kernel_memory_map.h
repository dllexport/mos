#pragma once
#include "lib/stdint.h"

// get their address
extern "C" uint8_t _kernel_text_start;
extern "C" uint8_t _kernel_text_end;
extern "C" uint8_t _kernel_data_start;
extern "C" uint8_t _kernel_data_end;
extern "C" uint8_t _kernel_rodata_start;
extern "C" uint8_t _kernel_rodata_end;
extern "C" uint8_t _kernel_bss_start;
extern "C" uint8_t _kernel_bss_end;
extern "C" uint8_t _kernel_end;
