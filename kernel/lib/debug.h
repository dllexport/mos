#pragma once
#include "stdint.h"

extern "C"
{
    void debug_init();
    void panic(const char *msg);
}

#define assert(x, info)  \
    do                   \
    {                    \
        if (!(x))        \
        {                \
            panic(info); \
        }                \
    } while (0)

// 编译期间静态检测
#define static_assert(x) \
    switch (x)           \
    {                    \
    case 0:              \
    case (x):;           \
    }
