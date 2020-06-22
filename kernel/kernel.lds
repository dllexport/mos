OUTPUT_FORMAT("elf64-x86-64","elf64-x86-64","elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(start_kernel)
SECTIONS
{
	. = 0xffffff8000100000;
	.text :
	{
		_kernel_text_start = .;
		*(.text)
		_kernel_text_end = .;
	}
	. = ALIGN(8);
	.data :
	{
		_kernel_data_start = .;
		*(.data)
		
		_kernel_data_end = .;
	}
}