#include "config.h"

ENTRY(start)
#ifdef ROCKBOX_LITTLE_ENDIAN
OUTPUT_FORMAT(elf32-littlearm)
#else
OUTPUT_FORMAT(elf32-bigarm)
#endif
OUTPUT_ARCH(arm)
STARTUP(target/arm/rk27xx/crt0.o)

#define PLUGINSIZE PLUGIN_BUFFER_SIZE
#define CODECSIZE CODEC_SIZE

#define DRAMORIG 0x60000000
#define DRAMSIZE (MEMORYSIZE * 0x100000) - PLUGINSIZE - CODECSIZE

#define IRAMORIG 0x00000000
#define IRAMSIZE 4K

/* End of the audio buffer, where the codec buffer starts */
#define ENDAUDIOADDR  (DRAMORIG + DRAMSIZE)
#define CODECORIG (ENDAUDIOADDR)

/* Where the codec buffer ends, and the plugin buffer starts */
#define ENDADDR (ENDAUDIOADDR + CODECSIZE)

MEMORY
{
    DRAM  : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
    IRAM  : ORIGIN = IRAMORIG, LENGTH = IRAMSIZE
}

SECTIONS
{
  loadaddress = DRAMORIG;

  .intvect : {
    _intvectstart = . ;
    KEEP(*(.intvect))
    _intvectend = . ;  
  } > IRAM AT > DRAM
  _intvectcopy = LOADADDR(.intvect);

  .text : {
    *(.init.text)
    *(.text*)
    *(.icode*)
    *(.glue_7*)
  } > DRAM

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

  .stack (NOLOAD) :
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
  } > DRAM

  .bss (NOLOAD) : {
     _edata = .;
     *(.bss*);
     *(.ibss);
     *(COMMON);
    . = ALIGN(0x4);
     _end = .;
  } > DRAM

  .audiobuf (NOLOAD) :
  {
      . = ALIGN(4);
      _audiobuffer = .;
      audiobuffer = .;
  } > DRAM

  .audiobufend ENDAUDIOADDR (NOLOAD) :
  {
      audiobufend = .;
      _audiobufend = .;
  } > DRAM

  .codec CODECORIG (NOLOAD) :
  {
      codecbuf = .;
      _codecbuf = .;
  } > DRAM

  .plugin ENDADDR (NOLOAD) :
  {
      _pluginbuf = .;
      pluginbuf = .;
  }

}
