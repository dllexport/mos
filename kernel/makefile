kernel: main.cpp lib/stdint.h lib/string.c lib/string.h
	nasm -f elf64 -o lib/no_gnu_property.o lib/no_gnu_property.asm
	g++ -g -m64 -c -o lib/string.o lib/string.c -fno-builtin -fno-exceptions -fno-rtti -Wno-write-strings 
	g++ -g -m64 -c -o main.o main.cpp -fno-builtin -fno-exceptions -fno-rtti -Wno-write-strings 
	ld -m elf_x86_64 -Ttext 0x1500 -n -o kernel.bin main.o lib/string.o lib/no_gnu_property.o
