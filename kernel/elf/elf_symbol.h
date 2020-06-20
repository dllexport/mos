#pragma oncel
#include "../lib/stdint.h"
#include "elf.h"

struct elf_symbol_t {
	Elf64_Sym *symtab;
	uint32_t symtabsz;
	const char *strtab;
	uint32_t strtabsz;
};