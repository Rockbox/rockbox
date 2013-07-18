ENTRY(start)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(rk27xx/crt0.o)

#define DRAMORIG 0x60000000
#define DRAMSIZE (16 * 0x100000)
#define DRAM_END_ADDRESS (DRAMORIG + DRAMSIZE)

#define IRAMORIG 0x00000000
#define IRAMSIZE 4K

MEMORY
{
    DRAM  : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
    IRAM  : ORIGIN = IRAMORIG, LENGTH = IRAMSIZE
}

SECTIONS
{
  .relocstart (NOLOAD) : {
    _relocstart = .;
  } > DRAM

  .text : {
    oc_codestart = .;
    *(.init.text)
    *(.text*)
    *(.icode*)
    *(.glue_7*)
  } > DRAM

  .intvect : {
    _intvectstart = . ;
    KEEP(*(.intvect))
    _intvectend = . ;
  } > IRAM AT > DRAM
  _intvectcopy = LOADADDR(.intvect) ;

  .rodata : {
    *(.rodata*)
    *(.irodata*)
    . = ALIGN(0x4);
  } > DRAM

  .data : {
    *(.data*)
    *(.idata*)
    . = ALIGN(0x4);
  } > DRAM

  .relocend (NOLOAD) : {
    _relocend = .;
  } > DRAM

  .stack (NOLOAD) :
  {
     *(.stack)
     oc_stackstart = .;
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
     oc_stackend = .;
  } > DRAM

  .bss (NOLOAD) : {
     _edata = .;
     *(.bss*);
     *(.ibss);
     *(COMMON);
    . = ALIGN(0x4);
     _end = .;
    oc_codeend = .;
    oc_bufferstart = .;
  } > DRAM

  .dramend DRAM_END_ADDRESS (NOLOAD) : {
    oc_bufferend = .;
  } > DRAM
}
