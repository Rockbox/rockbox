OUTPUT_FORMAT(elf32-sh)
INPUT(minimon.o)

MEMORY
{
	DRAM : ORIGIN = 0x09000000, LENGTH = 0x200000
}

SECTIONS
{
	.startvector :
	{
		*(.startvector)    
		. = ALIGN(0x4);
	} > DRAM

	.got :
	{
	   *(.got)
	} > DRAM

	.got.plt :
	{
	   *(.got.plt)
	} > DRAM

	.rela.got :
	{
	   *(.rela.got)
	} > DRAM

	.text :    
	{	 
		. = ALIGN(0x200);
		*(.entry)
		*(.text)	
		. = ALIGN(0x4);
	} > DRAM
	
	.data :
	{
		*(.data)
	} > DRAM

	.rodata :
	{
		*(.rodata)
		. = ALIGN(0x4);
	} > DRAM

	.bss :
	{
	   *(.bss)
	} > DRAM

	.stack :
	{
	   *(.stack)
	} > DRAM
}
