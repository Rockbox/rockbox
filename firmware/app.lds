#include "config.h"

ENTRY(start)
#ifdef CPU_COLDFIRE
OUTPUT_FORMAT(elf32-m68k)
#elif CONFIG_CPU == TCC730
OUTPUT_FORMAT(elf32-calmrisc16)
#else
OUTPUT_FORMAT(elf32-sh)
#endif
INPUT(crt0.o)

#if CONFIG_CPU == TCC730
MEMORY
{
   FIRMWARE : ORIGIN = LOADADDRESS, LENGTH = 256K
   FLASH (RX) : ORIGIN = 0x000000, LENGTH = 1024K
   ICMEM (RX) : ORIGIN = 0x3FC000, LENGTH = 32K
   IMEM1 : ORIGIN = 0x200000, LENGTH = 32K
   IMEM2 : ORIGIN = 0x210000, LENGTH = 16K
   IMEM3 : ORIGIN = 0x220000, LENGTH = 32K
   IMEM4 : ORIGIN = 0x230000, LENGTH = 16K

   DRAM : ORIGIN = 0x000000, LENGTH = 0x3F0000
}

SECTIONS
{
.text LOADADDRESS : {
	*(.init.text)
	*(.text) 
	}> FLASH  AT> FIRMWARE 

.icode 0x3FC040: {
	*(.vectors)
	*(.icode)
	}> ICMEM  AT> FIRMWARE

/* We start at 0x2000, to avoid overwriting Archos' loader datasegment.
 * If it turns out that we won't call back the loader, this can be set to 0.
 */
.bss 0x2000 : {
	*(.bss)
	. = ALIGN(2);
	}> DRAM	

/* Data is put after BSS, to have all strings addresses > VIRT_PTR + VIRT_SIZE.
   Strings are in rodata, so what we really assert is (.rodata > VIRT_PTR + VIRT_SIZE)
   See settings.h for details */
.data ALIGN(2): { 
	*(.data) 
	. = ALIGN(2);
	*(.rodata) 
	. = ALIGN(2);
	*(.rodata.str1.2)
	. = ALIGN(2);
	}> DRAM AT> FIRMWARE


.stack ALIGN(2) : {	
   . = . + 0x2000;
}> DRAM

.idata 0x200000: {
	*(.idata)
}> IMEM1

.idata2 0x220000: {
	*(.idata2)
}> IMEM3

_codesize = SIZEOF(.text);

_icodecopy = LOADADDR(.icode);
_icodestart = ADDR(.icode);
_icodesize = SIZEOF(.icode);

_datacopy = LOADADDR(.data);
_datastart = ADDR(.data);
_datasize = SIZEOF(.data);


_bssstart = ADDR(.bss);
_bsssize = (SIZEOF(.bss) + 1) & ~ 1;

_stackbegin = ADDR(.stack);
_stackend = ADDR(.stack) + SIZEOF(.stack);

_idatastart = ADDR(.idata);

/* FIXME: Where to put audio buffer? */

_audiobuffer = 0;
_audiobufend = 0;

/* Plugins are not supported on the Gmini*/

_pluginbuf = 0;

}

#else

#define PLUGINSIZE PLUGIN_BUFFER_SIZE
#define CODECSIZE CODEC_SIZE


#ifdef DEBUG
#define STUBOFFSET 0x10000
#else
#define STUBOFFSET 0
#endif

#define DRAMSIZE (MEMORYSIZE * 0x100000) - PLUGINSIZE - STUBOFFSET - CODECSIZE

#if defined(IRIVER_H100_SERIES) || defined(IRIVER_H300)
#define DRAMORIG 0x31000000 + STUBOFFSET
#define IRAMORIG 0x10000000
#define IRAMSIZE 0xc000
#else
#define DRAMORIG 0x09000000 + STUBOFFSET
#define IRAMORIG 0x0f000000
#define IRAMSIZE 0x1000
#endif

/* End of the audio buffer, where the codec buffer starts */
#define ENDAUDIOADDR  (DRAMORIG + DRAMSIZE)

/* Where the codec buffer ends, and the plugin buffer starts */
#define ENDADDR (ENDAUDIOADDR + CODECSIZE)

MEMORY
{
   DRAM : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
   IRAM : ORIGIN = IRAMORIG, LENGTH = IRAMSIZE
}

SECTIONS
{
    .vectors :
    {
        loadaddress = .;
        _loadaddress = .;
        KEEP(*(.resetvectors));
        *(.resetvectors);
        KEEP(*(.vectors));
        *(.vectors);
    } > DRAM

    .text :
    {
        . = ALIGN(0x200);
        *(.init.text)
        *(.text*)
        . = ALIGN(0x4);
    } > DRAM

    .rodata :
    {
        *(.rodata)  /* problems without this, dunno why */
        *(.rodata*)
        *(.rodata.str1.1)
        *(.rodata.str1.4)
        . = ALIGN(0x4);

        /* Pseudo-allocate the copies of the data sections */
        _datacopy = .;
    } > DRAM

    /* TRICK ALERT! For RAM execution, we put the .data section at the
       same load address as the copy. Thus, we don't waste extra RAM
       when we don't actually need the copy.  */
    .data : AT ( _datacopy )
    {
        _datastart = .;
        *(.data*)
        . = ALIGN(0x4);
        _dataend  = .;
        _iramcopy = .;
    } > DRAM

    /DISCARD/ : 
    {
        *(.eh_frame)
    }

    .iram IRAMORIG : AT ( _iramcopy)
    {
        _iramstart = .;
        *(.icode)
        *(.idata)
        _iramend = .;
    } > IRAM

#ifdef CPU_COLDFIRE
    .stack :
    {
       *(.stack)
       stackbegin = .;
       . += 0x2000;
       stackend = .;
    } > IRAM
#else
    /* TRICK ALERT! We want 0x2000 bytes of stack, but we set the section
       size smaller, and allow the stack to grow into the .iram copy */
    .stack ADDR(.data) + SIZEOF(.data) + SIZEOF(.iram):
    {
       *(.stack)
       _stackbegin = . - SIZEOF(.iram);
       . += 0x2000 - SIZEOF(.iram);
       _stackend = .;
    } > DRAM
#endif

#ifdef CPU_COLDFIRE
    .bss ADDR(.data) + SIZEOF(.data) + SIZEOF(.iram):
#else
    .bss :
#endif
    {
       _edata = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(0x4);
       _end = .;
    } > DRAM

    .audiobuf ALIGN(4) :
    {
        _audiobuffer = .;
        audiobuffer = .;
    } > DRAM

    .audiobufend ENDAUDIOADDR:
    {
        audiobufend = .;
        _audiobufend = .;
    } > DRAM

    .codec ENDAUDIOADDR:
    {
        codecbuf = .;
        _codecbuf = .;
    }

    .plugin ENDADDR:
    {
        _pluginbuf = .;
        pluginbuf = .;
    }
}

#endif
