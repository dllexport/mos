kernel: main.cpp lib/stdint.h lib/string.c lib/string.h
	nasm -f elf64 -o lib/no_gnu_property.o lib/no_gnu_property.asm
	nasm -f elf64 -o interrupt_asm.o interrupt.asm
	g++ -g -m64 -c -o lib/string.o lib/string.c -fno-builtin -fno-exceptions -fno-rtti -Wno-write-strings -fno-stack-protector
	g++ -g -m64 -c -o interrupt.o interrupt.cpp -fno-builtin -fno-exceptions -fno-rtti -Wno-write-strings 
	g++ -g -m64 -c -std=c++17 -o gdt.o gdt.cpp -fno-builtin -fno-exceptions -fno-rtti -Wno-write-strings 
	g++ -g -m64 -c -std=c++17 -o tss.o tss.cpp -fno-builtin -fno-exceptions -fno-rtti -Wno-write-strings 
	g++ -g -m64 -c -std=c++17 -o memory.o memory.cpp -fno-builtin -fno-exceptions -fno-rtti -Wno-write-strings 
	g++ -g -m64 -c -o main.o main.cpp -fno-builtin -fno-exceptions -fno-rtti -Wno-write-strings 
	ld -m elf_x86_64 -Ttext 0x1500 -n -o kernel.bin main.o memory.o gdt.o tss.o interrupt.o interrupt_asm.o lib/string.o lib/no_gnu_property.o
