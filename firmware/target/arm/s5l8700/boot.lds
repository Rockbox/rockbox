#include "config.h"

ENTRY(start)
#ifdef ROCKBOX_LITTLE_ENDIAN
OUTPUT_FORMAT(elf32-littlearm)
#else
OUTPUT_FORMAT(elf32-bigarm)
#endif
OUTPUT_ARCH(arm)
STARTUP(target/arm/s5l8700/crt0.o)

#define DRAMORIG 0x08000000
#define DRAMSIZE (MEMORYSIZE * 0x100000)

#define IRAMORIG 0x22000000
#if CONFIG_CPU==S5L8701
#define IRAMSIZE 176K
#else
#define IRAMSIZE 256K
#endif

#ifdef MEIZU_M6SL
#define DFULOADADDR IRAMORIG
#else
#define DFULOADADDR (IRAMORIG+0x20000)
#endif

/* This is not available in all versions of the S5L8700 */
#define FLASHORIG 0x24000000
#define FLASHSIZE 1M

MEMORY
{
    DRAM  : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
    IRAM  : ORIGIN = IRAMORIG, LENGTH = IRAMSIZE
    FLASH : ORIGIN = FLASHORIG, LENGTH = FLASHSIZE
}

#ifdef IPOD_NANO2G
#define LOAD_AREA IRAM
#else
#define LOAD_AREA FLASH
#endif

SECTIONS
{
  .intvect : {
    _intvectstart = . ;
    *(.intvect)
    _intvectend = _newstart ;  
  } >IRAM AT> LOAD_AREA
  _intvectcopy = LOADADDR(.intvect) ;

  .text : {
    *(.init.text)
    *(.text*)
    *(.glue_7*)
  } > LOAD_AREA

  .rodata : {
    *(.rodata*)
    . = ALIGN(0x4);
  } > LOAD_AREA

  .data : {
    _datastart = . ;
    *(.irodata)
    *(.icode)
    *(.idata)
    *(.data*)
    *(.ncdata*);
    . = ALIGN(0x4);
    _dataend = . ;
   } > IRAM AT> LOAD_AREA
   _datacopy = LOADADDR(.data) ;

  .stack :
  {
     *(.stack)
     _stackbegin = .;
     stackbegin = .;
     . += 0x2000;
     _stackend = .;
     stackend = .;
     _irqstackbegin = .;
     . += 0x400;
     _irqstackend = .;
     _fiqstackbegin = .;
     . += 0x400;
     _fiqstackend = .;
  } > IRAM

  . = DRAMORIG;
#ifdef IPOD_NANO2G
  /* The bss section is too large for IRAM - we just move it 12MB into the
     DRAM */
  . += (12*1024*1024) {
#endif
  .bss :{
     _edata = .;
     *(.bss*);
     *(.ibss);
     *(.ncbss*);
     *(COMMON);
    . = ALIGN(0x4);
     _end = .;
#ifdef IPOD_NANO2G
  } > DRAM
#else /* other targets don't have DRAM set up yet */
  } > IRAM
#endif  
}
