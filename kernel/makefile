kernel: main.cpp lib/print.S lib/print.h lib/stdint.h
	nasm -f elf -o lib/print.o lib/print.S
	g++ -g -m32 -I lib/kernel -c -o main.o main.cpp -fno-builtin -fno-exceptions -fno-rtti -Wno-write-strings 
	ld -m elf_i386 -Ttext 0xc0001500 -e main -o kernel.bin main.o lib/print.o
