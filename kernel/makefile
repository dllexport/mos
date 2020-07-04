
CXXFLAGS = --target=x86_64-elf -mno-sse -fPIE -g -mcmodel=kernel -fno-common -fno-builtin -ffreestanding -m64 -std=gnu++17 -fno-exceptions -fno-rtti -fno-threadsafe-statics -nostdlib 
COMPILER = clang++-10
BUILD_DIR = build

kernel:
	
	nasm -f elf64 -o build/no_gnu_property.o lib/no_gnu_property.asm
	nasm -f elf64 -o build/interrupt_irq.o interrupt_irq.asm
	nasm -f elf64 -o build/interrupt_asm.o interrupt.asm
	nasm -f elf64 -o build/task_asm.o task.asm

	$(COMPILER) $(CXXFLAGS) -c -o build/string.o lib/string.c 
	$(COMPILER) $(CXXFLAGS) -c -o build/page_fault.o interrupt/page_fault.cpp
	$(COMPILER) $(CXXFLAGS) -c -o build/interrupt.o interrupt.cpp 
	$(COMPILER) $(CXXFLAGS) -c -o build/gdt.o gdt.cpp 
	$(COMPILER) $(CXXFLAGS) -c -o build/tss.o tss.cpp
	$(COMPILER) $(CXXFLAGS) -c -o build/memory.o memory.cpp
	$(COMPILER) $(CXXFLAGS) -c -o build/buddy_system.o buddy_system.cpp
	$(COMPILER) $(CXXFLAGS) -c -o build/task.o task.cpp
	$(COMPILER) $(CXXFLAGS) -c -o build/printk.o lib/printk.cpp
	$(COMPILER) $(CXXFLAGS) -c -o build/debug.o lib/debug.cpp
	$(COMPILER) $(CXXFLAGS) -c -o build/timer.o timer.cpp
	$(COMPILER) $(CXXFLAGS) -c -o build/main.o main.cpp

	ld -m elf_x86_64 -e start_kernel -T kernel.lds -n -o kernel.bin $(BUILD_DIR)/main.o $(BUILD_DIR)/timer.o $(BUILD_DIR)/debug.o $(BUILD_DIR)/page_fault.o $(BUILD_DIR)/printk.o $(BUILD_DIR)/task.o $(BUILD_DIR)/task_asm.o $(BUILD_DIR)/memory.o $(BUILD_DIR)/buddy_system.o $(BUILD_DIR)/gdt.o $(BUILD_DIR)/tss.o $(BUILD_DIR)/interrupt.o $(BUILD_DIR)/interrupt_irq.o $(BUILD_DIR)/interrupt_asm.o $(BUILD_DIR)/string.o $(BUILD_DIR)/no_gnu_property.o