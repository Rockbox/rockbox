OUTPUT_FORMAT(elf32-sh)

MEMORY
{
	/* the boot ROM uses IRAM at 400-430, stay away and start at 500 */
	IRAM : ORIGIN = 0x0FFFF500, LENGTH = 0xA00
	/* and leave some room for stack at the end */
}

SECTIONS
{
	.startvector :
	{
		*(.startvector)
		. = ALIGN(0x4);
	} > IRAM

	.text :
	{	 
		*(.text)	
		*(.icode)
		. = ALIGN(0x4);
	} > IRAM
	
	.data :
	{
		*(.data)
	} > IRAM

	.bss :
	{
	   *(.bss)
	} > IRAM
}
