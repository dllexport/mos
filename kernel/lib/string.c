#include "string.h"

struct {
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

void *memcpy(void * dest, const void * src, uint64_t n)
{
	char * dest_c = (char*)dest;
	const char * src_c = (char*)src;

	for (uint64_t i = 0; i < n; ++i) {
		dest_c[i] = src_c[i];
	}

	return dest;
}

void *memmove(void *dest, const void *src, uint64_t n)
{
	// Possibly undefined behaviour
	if (src > dest) {
		return memcpy(dest, src, n);
	} else {
		char *dest_c = (char*)dest;
		const char *src_c = (char*)src;

		// We can't do >= 0 because unsigned
		for (uint64_t i = n; i > 0; --i) {
			dest_c[i-1] = src_c[i-1];
		}

		return dest;
	}
}


static void putchar_at(char chr, uint8_t color, uint8_t x, uint8_t y)
{
	unsigned char *videomem_start = (unsigned char *) 0xb8000;
	uint16_t offset = 160*y + 2*x;

	videomem_start[offset] = (unsigned char) chr;
	videomem_start[offset + 1] = color;
}

static void put_char(char chr)
{
	switch (chr) {
	case '\n':
		++cursor.y;
		cursor.x = 0;
		break;
	default:
		putchar_at(chr, 0x07, cursor.x, cursor.y);
		++cursor.x;
	}
}

void printk(char *str)
{
	unsigned char *videomem_start = (unsigned char *) 0xb8000;

	if (cursor.y >= 24) {
		memmove(videomem_start, videomem_start+160, 160*24);
	}

	auto len = strlen(str);
	for (auto i = 0; i < len; i++) {
		put_char(str[i]);
	}
}