OUTPUT_FORMAT(elf32-sh)
INPUT(minimon.o)

MEMORY
{
	IRAM : ORIGIN = 0x0FFFF000, LENGTH = 0x500
}

SECTIONS
{
	.startvector :
	{
		*(.startvector)    
		. = ALIGN(0x4);
	} > IRAM

	.got :
	{
	   *(.got)
	} > IRAM

	.got.plt :
	{
	   *(.got.plt)
	} > IRAM

	.rela.got :
	{
	   *(.rela.got)
	} > IRAM

	.text :    
	{	 
		. = ALIGN(0x200);
		*(.entry)
		*(.text)	
		. = ALIGN(0x4);
	} > IRAM
	
	.data :
	{
		*(.data)
	} > IRAM

	.rodata :
	{
		*(.rodata)
		. = ALIGN(0x4);
	} > IRAM

	.bss :
	{
	   *(.bss)
	} > IRAM

	.stack :
	{
	   *(.stack)
	} > IRAM
}
