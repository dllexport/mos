#include "printk.h"
#include "port_ops.h"
#include "string.h"
#include "../spinlock.h"

static Spinlock printk_spinlock;

namespace Kernel::VGA
{
        // VGA 的显示缓冲的起点是 0xB8000
        static uint16_t *video_memory = (uint16_t *)(0xffffff8000000000 + 0xb8000);

        // 屏幕"光标"的坐标
        static uint8_t cursor_x = 0;
        static uint8_t cursor_y = 0;

        static void move_cursor()
        {
                // 屏幕是 80 字节宽
                uint16_t cursorLocation = cursor_y * 80 + cursor_x;

                // 在这里用到的两个内部寄存器的编号为14与15，分别表示光标位置
                // 的高8位与低8位。

                outb(0x3D4, 14);                  // 告诉 VGA 我们要设置光标的高字节
                outb(0x3D5, cursorLocation >> 8); // 发送高 8 位
                outb(0x3D4, 15);                  // 告诉 VGA 我们要设置光标的低字节
                outb(0x3D5, cursorLocation);      // 发送低 8 位
        }

        void console_clear()
        {
                uint8_t attribute_byte = (0 << 4) | (15 & 0x0F);
                uint16_t blank = 0x20 | (attribute_byte << 8);

                int i;
                for (i = 0; i < 80 * 25; i++)
                {
                        video_memory[i] = blank;
                }

                cursor_x = 0;
                cursor_y = 0;
                move_cursor();
        }

        static void scroll()
        {
                // attribute_byte 被构造出一个黑底白字的描述格式
                uint8_t attribute_byte = (0 << 4) | (15 & 0x0F);
                uint16_t blank = 0x20 | (attribute_byte << 8); // space 是 0x20

                // cursor_y 到 25 的时候，就该换行了
                if (cursor_y >= 25)
                {
                        // 将所有行的显示数据复制到上一行，第一行永远消失了...
                        int i;

                        for (i = 0 * 80; i < 24 * 80; i++)
                        {
                                video_memory[i] = video_memory[i + 80];
                        }

                        // 最后的一行数据现在填充空格，不显示任何字符
                        for (i = 24 * 80; i < 25 * 80; i++)
                        {
                                video_memory[i] = blank;
                        }

                        // 向上移动了一行，所以 cursor_y 现在是 24
                        cursor_y = 24;
                }
        }

        void console_putc_color(char c, Color back, Color fore)
        {
                uint8_t back_color = (uint8_t)back;
                uint8_t fore_color = (uint8_t)fore;

                uint8_t attribute_byte = (back_color << 4) | (fore_color & 0x0F);
                uint16_t attribute = attribute_byte << 8;

                // 0x08 是退格键的 ASCII 码
                // 0x09 是tab 键的 ASCII 码
                if (c == 0x08 && cursor_x)
                {
                        cursor_x--;
                }
                else if (c == 0x09)
                {
                        cursor_x = (cursor_x + 8) & ~(8 - 1);
                }
                else if (c == '\r')
                {
                        cursor_x = 0;
                }
                else if (c == '\n')
                {
                        cursor_x = 0;
                        cursor_y++;
                }
                else if (c >= ' ')
                {
                        video_memory[cursor_y * 80 + cursor_x] = c | attribute;
                        cursor_x++;
                }

                // 每 80 个字符一行，满80就必须换行了
                if (cursor_x >= 80)
                {
                        cursor_x = 0;
                        cursor_y++;
                }

                // 如果需要的话滚动屏幕显示
                scroll();

                // 移动硬件的输入光标
                move_cursor();
        }

        void console_write(char *cstr)
        {
                while (*cstr)
                {
                        console_putc_color(*cstr++, Color::rc_black, Color::rc_white);
                }
        }

        void console_write_color(char *cstr, Color back, Color fore)
        {
                while (*cstr)
                {
                        console_putc_color(*cstr++, back, fore);
                }
        }
} // namespace Kernel::VGA

static int vsprintf(char *buff, const char *format, va_list args);

#define printk_body                       \
        static char buff[1024];           \
        va_list args;                     \
        int i;                            \
        va_start(args, format);           \
        i = vsprintf(buff, format, args); \
        va_end(args);                     \
        buff[i] = '\0';                   \
        Kernel::VGA::console_write(buff);

void printk_with_spinlock_cli(const char *format, ...)
{
        asm volatile("cli");
        LockGuard<Spinlock> lg(printk_spinlock);
        printk_body;
        asm volatile("sti");
}

void putchar(char ch)
{
        Kernel::VGA::console_putc_color(ch);
}

void putvalue(char *val)
{
        while (*(val++) != '\0')
        {
                Kernel::VGA::console_putc_color(*val);
        }
}
char *int2hexstr(uint64_t val, int &end_index)
{
        static const auto hex_table = "0123456789abcdef";
        static char buff[32];
        int i = 0;
        while (val != 0)
        {
                auto hex = val % 16;
                val = val / 16;
                buff[i++] = hex_table[hex];
        }
        end_index = i;
        return buff;
}

char *int2str(uint64_t val, int &end_index)
{
        static char buff[32];
        int i = 0;
        while (val != 0)
        {
                auto hex = val % 10;
                val = val / 10;
                buff[i++] = hex + '0';
        }
        end_index = i;
        return buff;
}

void putvalue(uint64_t val)
{
        int end_idx;
        auto buff = int2str(val, end_idx);
        for (int i = end_idx; i >= 0; --i)
        {
                Kernel::VGA::console_putc_color(buff[i]);
        }
}

void putvalue(int64_t val)
{
        if (val < 0)
        {
                Kernel::VGA::console_putc_color('-');
                putvalue(uint64_t(-val));
                return;
        }

        putvalue(uint64_t(val));
}

void putvalue(void *val)
{
        int end_idx;
        auto buff = int2hexstr(uint64_t(val), end_idx);

        Kernel::VGA::console_putc_color('0');
        Kernel::VGA::console_putc_color('x');
        auto padding = 16 - end_idx;
        while (padding--)
        {
                Kernel::VGA::console_putc_color('0');
        }
        for (int i = end_idx; i >= 0; --i)
        {
                Kernel::VGA::console_putc_color(buff[i]);
        }
}

void printk(const char *format, ...)
{
        asm volatile("pushf");
        asm volatile("cli");
        static const auto peek = [](char *current, uint64_t count = 1) {
                return current[count];
        };

        va_list args;
        va_start(args, format);
        auto pformat = const_cast<char *>(format);
        char ch = *format;
        while (ch)
        {
                if (ch == '%')
                {
                        switch (peek(pformat))
                        {
                        // int64_t
                        case 'd':
                        {
                                ++pformat;
                                putvalue((int64_t)4);
                                break;
                        }
                        case 's':
                        {
                                ++pformat;
                                putvalue(va_arg(args, char *));
                                break;
                        }
                        case 'u':
                        {
                                ++pformat;
                                putvalue(va_arg(args, uint64_t));
                                break;
                        }
                        // case 'f':
                        // {
                        //         ++format;
                        //         putvalue(va_arg(args, double));
                        //         break;
                        // }
                        case 'x':
                        case 'p':
                        {
                                ++pformat;
                                putvalue(va_arg(args, void *));
                                break;
                        }
                        default:
                        {
                                // output '%'
                                putchar(ch);
                        }
                        }
                }
                else
                {
                        // output ch
                        putchar(ch);
                }
                ch = *(++pformat);
        }
        va_end(args);

        asm volatile("popf");
}

void printk_while(const char *format, ...)
{
        // 避免频繁创建临时变量，内核的栈很宝贵
        printk_body;
        while (1)
                ;
}

#define is_digit(c) ((c) >= '0' && (c) <= '9')

static int skip_atoi(const char **s)
{
        int i = 0;

        while (is_digit(**s))
        {
                i = i * 10 + *((*s)++) - '0';
        }

        return i;
}

#define ZEROPAD 1  // pad with zero
#define SIGN 2     // unsigned/signed long
#define PLUS 4     // show plus
#define SPACE 8    // space if plus
#define LEFT 16    // left justified
#define SPECIAL 32 // 0x
#define SMALL 64   // use 'abcdef' instead of 'ABCDEF'

#define do_div(n, base) ({ \
                int __res; \
                __asm__("divq %4":"=a" (n),"=d" (__res):"0" (n),"1" (0),"r" (base)); \
                __res; })

static char *number(char *str, uint64_t num, uint64_t base, int size, int precision, int type)
{
        char c, sign, tmp[36];
        const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        int i;

        if (type & SMALL)
        {
                digits = "0123456789abcdefghijklmnopqrstuvwxyz";
        }
        if (type & LEFT)
        {
                type &= ~ZEROPAD;
        }
        if (base < 2 || base > 36)
        {
                return 0;
        }

        c = (type & ZEROPAD) ? '0' : ' ';

        if (type & SIGN && num < 0)
        {
                sign = '-';
                num = -num;
        }
        else
        {
                sign = (type & PLUS) ? '+' : ((type & SPACE) ? ' ' : 0);
        }

        if (sign)
        {
                size--;
        }
        if (type & SPECIAL)
        {
                if (base == 16)
                {
                        size -= 2;
                }
                else if (base == 8)
                {
                        size--;
                }
        }
        i = 0;
        if (num == 0)
        {
                tmp[i++] = '0';
        }
        else
        {
                while (num != 0)
                {
                        tmp[i++] = digits[do_div(num, base)];
                }
                tmp[i++] = 'x';
                tmp[i++] = '0';
        }

        if (i > precision)
        {
                precision = i;
        }
        size -= precision;

        if (!(type & (ZEROPAD + LEFT)))
        {
                while (size-- > 0)
                {
                        *str++ = ' ';
                }
        }
        if (sign)
        {
                *str++ = sign;
        }
        if (type & SPECIAL)
        {
                if (base == 8)
                {
                        *str++ = '0';
                }
                else if (base == 16)
                {
                        *str++ = '0';
                        *str++ = digits[33];
                }
        }
        if (!(type & LEFT))
        {
                while (size-- > 0)
                {
                        *str++ = c;
                }
        }
        while (i < precision--)
        {
                *str++ = '0';
        }
        while (i-- > 0)
        {
                *str++ = tmp[i];
        }
        while (size-- > 0)
        {
                *str++ = ' ';
        }

        return str;
}

static int vsprintf(char *buff, const char *format, va_list args)
{
        int len;
        int i;
        char *str;
        char *s;
        int *ip;
        int flags;       // flags to number()
        int field_width; // width of output field
        int precision;   // min. # of digits for integers; max number of chars for from string

        for (str = buff; *format; ++format)
        {
                if (*format != '%')
                {
                        *str++ = *format;
                        continue;
                }
                flags = 0;
        repeat:
                ++format; // this also skips first '%'
                switch (*format)
                {
                case '-':
                        flags |= LEFT;
                        goto repeat;
                case '+':
                        flags |= PLUS;
                        goto repeat;
                case ' ':
                        flags |= SPACE;
                        goto repeat;
                case '#':
                        flags |= SPECIAL;
                        goto repeat;
                case '0':
                        flags |= ZEROPAD;
                        goto repeat;
                }

                // get field width
                field_width = -1;
                if (is_digit(*format))
                {
                        field_width = skip_atoi(&format);
                }
                else if (*format == '*')
                {
                        // it's the next argument
                        field_width = va_arg(args, int);
                        if (field_width < 0)
                        {
                                field_width = -field_width;
                                flags |= LEFT;
                        }
                }

                // get the precision
                precision = -1;
                if (*format == '.')
                {
                        ++format;
                        if (is_digit(*format))
                        {
                                precision = skip_atoi(&format);
                        }
                        else if (*format == '*')
                        {
                                // it's the next argument
                                precision = va_arg(args, int);
                        }
                        if (precision < 0)
                        {
                                precision = 0;
                        }
                }

                // get the conversion qualifier
                //int qualifier = -1;   // 'h', 'l', or 'L' for integer fields
                if (*format == 'h' || *format == 'l' || *format == 'L')
                {
                        //qualifier = *format;
                        ++format;
                }

                switch (*format)
                {
                case 'c':
                        if (!(flags & LEFT))
                        {
                                while (--field_width > 0)
                                {
                                        *str++ = ' ';
                                }
                        }
                        *str++ = (unsigned char)va_arg(args, int);
                        while (--field_width > 0)
                        {
                                *str++ = ' ';
                        }
                        break;

                case 's':
                        s = va_arg(args, char *);
                        len = strlen(s);
                        if (precision < 0)
                        {
                                precision = len;
                        }
                        else if (len > precision)
                        {
                                len = precision;
                        }

                        if (!(flags & LEFT))
                        {
                                while (len < field_width--)
                                {
                                        *str++ = ' ';
                                }
                        }
                        for (i = 0; i < len; ++i)
                        {
                                *str++ = *s++;
                        }
                        while (len < field_width--)
                        {
                                *str++ = ' ';
                        }
                        break;

                case 'o':
                        str = number(str, va_arg(args, unsigned long), 8,
                                     field_width, precision, flags);
                        break;

                case 'p':
                        if (field_width == -1)
                        {
                                field_width = 8;
                                flags |= ZEROPAD;
                        }
                        flags |= SMALL;
                        str = number(str, (unsigned long)va_arg(args, void *), 16,
                                     field_width, precision, flags);
                        break;

                case 'x':
                        flags |= SMALL;
                case 'X':
                        str = number(str, va_arg(args, unsigned long), 16,
                                     field_width, precision, flags);
                        break;

                case 'd':
                case 'i':
                        flags |= SIGN;
                case 'u':
                        str = number(str, va_arg(args, unsigned long), 10,
                                     field_width, precision, flags);
                        break;
                case 'b':
                        str = number(str, va_arg(args, unsigned long), 2,
                                     field_width, precision, flags);
                        break;

                case 'n':
                        ip = va_arg(args, int *);
                        *ip = (str - buff);
                        break;

                default:
                        if (*format != '%')
                                *str++ = '%';
                        if (*format)
                        {
                                *str++ = *format;
                        }
                        else
                        {
                                --format;
                        }
                        break;
                }
        }
        *str = '\0';

        return (str - buff);
}