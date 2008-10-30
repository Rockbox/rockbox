/* This is for the variant without boot ROM,
   where the flash ROM is mirrored to address zero */

OUTPUT_FORMAT(elf32-sh)

MEMORY
{
	IRAM : ORIGIN = 0x0FFFF000, LENGTH = 0x1000
	FLASH : ORIGIN = 0x00000000, LENGTH = 0x40000
}

SECTIONS
{
	.vectors :
	{
		*(.vectors)
		. = ALIGN(0x200);
	} > FLASH

	.startup :
	{
		*(.startup)
		. = ALIGN(0x4);
		_begin_iramcopy = .;
	} > FLASH

	.text : AT ( _begin_iramcopy )
	{	 
		_begin_text = .;
		*(.text)	
		*(.icode)
		. = ALIGN(0x4);
		_end_text = .;
	} > IRAM
	
	.data : AT ( _end_text )
	{
		_begin_data = .;
		*(.data)
		. = ALIGN(0x4);
		_end_data = .;
	} > IRAM

	.bss : AT ( _end_data )
	{
		_begin_bss = .;
		*(.bss)
		. = ALIGN(0x4);
		_end_bss = .;
	} > IRAM

	.stack :
	{
		_begin_stack = .;
		*(.stack)
		. = ALIGN(0x1000);
		_end_stack = .;
	} > IRAM

	/* size of the program (without vectors) */
	_total_size = SIZEOF(.startup) + SIZEOF(.text) + SIZEOF(.data);
}
