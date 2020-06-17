#include "string.h"
#include <stdarg.h>

struct
{
    uint8_t x;
    uint8_t y;
} cursor = {0, 0};

uint64_t strlen(char *str)
{
    uint64_t len = 0;
    while (*str++)
        len++;
    return len;
}

int strcmp(const char *s1, const char *s2)
{
    while (*s1 != 0 && *s1 == *s2)
        ++s1, ++s2;
    return *s1 - *s2;
}

void memset(void *addr, uint8_t val, uint64_t size)
{
    for (uint64_t i = 0; i < size; ++i)
    {
        *((char *)addr + i) = val;
    }
}

void *memcpy(void *dest, const void *src, uint64_t n)
{
    char *dest_c = (char *)dest;
    const char *src_c = (char *)src;

    for (uint64_t i = 0; i < n; ++i)
    {
        dest_c[i] = src_c[i];
    }

    return dest;
}

void *memmove(void *dest, const void *src, uint64_t n)
{
    // Possibly undefined behaviour
    if (src > dest)
    {
        return memcpy(dest, src, n);
    }
    else
    {
        char *dest_c = (char *)dest;
        const char *src_c = (char *)src;

        // We can't do >= 0 because unsigned
        for (uint64_t i = n; i > 0; --i)
        {
            dest_c[i - 1] = src_c[i - 1];
        }

        return dest;
    }
}

static void put_char_at(char chr, uint8_t color, uint8_t x, uint8_t y)
{
    unsigned char *videomem_start = (unsigned char *)(0xffffff8000000000 + 0xb8000);
    uint16_t offset = 160 * y + 2 * x;

    videomem_start[offset] = (unsigned char)chr;
    videomem_start[offset + 1] = color;
}

static void put_char(char chr)
{
    switch (chr)
    {
    case '\n':
        ++cursor.y;
        cursor.x = 0;
        break;
    default:
        put_char_at(chr, 0x07, cursor.x, cursor.y);
        ++cursor.x;
    }
}

void printk(char *str)
{
    unsigned char *videomem_start = (unsigned char *)(0xffffff8000000000 + 0xb8000);

    if (cursor.y >= 24)
    {
        memmove(videomem_start, videomem_start + 160, 160 * 24);
    }

    auto len = strlen(str);
    for (auto i = 0; i < len; i++)
    {
        put_char(str[i]);
    }
}

void printk_hex(uint64_t hex)
{
    static int iters = 0;
    char hex_str[] = "0x0000000000000000";

    for (int i = 17; i >= 2; --i)
    {
        unsigned char nibble = hex & 0xF;
        char hex_chr = '0' + nibble;
        if (nibble > 9)
        {
            hex_chr += 7;
        }
        hex_str[i] = hex_chr;
        hex /= 16;
    }
    printk(hex_str);

    ++iters;
}

// void printf(char *str, ...)
// {
//     va_list arg;
//     va_start(arg, str);
//     int zero, cnt;

//     while(*str)
//     {
//         if (*str == '%')
//         {
//             str++;
//             zero = cnt = 0;
//             if (*str <= '9' && *str >= '0')
//             {
//                 zero = !(*str - '0'), cnt = 0;
//                 for (;*str <= '9' && *str >= 0;str++)
//                 {
//                     cnt *= 10;
//                     cnt += *str - '0';
//                 }
//             }
//             if (*str == 'd')
//                 puti(va_arg(arg, int), cnt, zero);
//             else if (*str == 'c')
//                 put_char((char)va_arg(arg, int));
//             else if (*str == 's')
//                 puts((char *)va_arg(arg, char*));
//             else if (*str == 'x')
//                 putx(va_arg(arg, long), 0, cnt, zero);
//             else if (*str == 'X')
//                 putx(va_arg(arg, long), 1, cnt, zero);
//             else if (*str == 'l' && *(str + 1) == 'd')
//             {
//                 str++;
//                 putl(va_arg(arg, long), cnt, zero);
//             }
//             else if (*str == 'u')
//             {
//                 str++;
//                 if (*str == 'd')
//                     putui(va_arg(arg, int), cnt, zero);
//                 else if (*str == 'x')
//                     putux(va_arg(arg, long), 0, cnt, zero);
//                 else if (*str == 'X')
//                     putux(va_arg(arg, long), 1, cnt, zero);
//                 else if (*str == 'l' && *(str + 1) == 'd')
//                 {
//                     str++;
//                     putl(va_arg(arg, long), cnt, zero);
//                 }
//             }
//             else
//                 put_char(*str);
//         }
//         else if (*str == '\n')
//         {
//             Pos.YPosition += Pos.YCharSize;
//             Pos.XPosition = 0;
//             Pos.YPosition %= Pos.YResolution;
//         }
//         else if (*str == '\t')
//         {
//             char tmp[50];
//             int i, cnt = 4 - Pos.XPosition / Pos.XCharSize % 4;
//             for (i = 0;i < cnt; i++)
//                 tmp[i] = ' ';
//             tmp[i] = '\0';
//             puts(tmp);
//         }
//         else if (*str == '\b')
//             TranslateX(-1);
//         else
//             put_char(*str);
//         str++;
//     }
//     va_end(arg);
// }