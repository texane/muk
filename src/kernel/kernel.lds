SECTIONS
{
	. = 0x200000;
	g_kernel_start = .;
	.text : { *(.text) }
	.data : { *(.data) }
	.rodata : { *(.rodata) }
	.bss : { *(.bss) }
	g_kernel_end = .;
}