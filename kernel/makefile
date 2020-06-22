
CXXFLAGS = -g -mcmodel=large -m64 -std=gnu++17 -fno-builtin -fno-exceptions -fno-rtti -fno-threadsafe-statics -Wno-write-strings -fno-stack-protector
COMPILER = g++
kernel: main.cpp lib/stdint.h lib/string.c lib/string.h
	
	nasm -f elf64 -o lib/no_gnu_property.o lib/no_gnu_property.asm
	nasm -f elf64 -o interrupt_irq.o interrupt_irq.asm
	nasm -f elf64 -o interrupt_asm.o interrupt.asm
	nasm -f elf64 -o task_asm.o task.asm

	$(COMPILER) $(CXXFLAGS) -c -o lib/string.o lib/string.c 
	$(COMPILER) $(CXXFLAGS) -c -o interrupt.o interrupt.cpp 
	$(COMPILER) $(CXXFLAGS) -c -o gdt.o gdt.cpp 
	$(COMPILER) $(CXXFLAGS) -c -o tss.o tss.cpp
	$(COMPILER) $(CXXFLAGS) -c -o memory.o memory.cpp
	$(COMPILER) $(CXXFLAGS) -c -o task.o task.cpp
	$(COMPILER) $(CXXFLAGS) -c -o lib/printk.o lib/printk.cpp
	$(COMPILER) $(CXXFLAGS) -c -o lib/debug.o lib/debug.cpp
	$(COMPILER) $(CXXFLAGS) -c -o timer.o timer.cpp
	$(COMPILER) $(CXXFLAGS) -c -o main.o main.cpp

	ld -m elf_x86_64 -e start_kernel -Ttext 0x100000 -n -o kernel.bin main.o timer.o lib/debug.o lib/printk.o task.o task_asm.o memory.o gdt.o tss.o interrupt.o interrupt_irq.o interrupt_asm.o lib/string.o lib/no_gnu_property.o
