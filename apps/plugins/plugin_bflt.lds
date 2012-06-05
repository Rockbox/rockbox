#include "config.h"
#include "cpu.h"

/* These output formats should be in the config-files */

#ifdef CPU_COLDFIRE
OUTPUT_FORMAT(elf32-m68k)
#elif defined(CPU_ARM)
OUTPUT_FORMAT(elf32-littlearm)
#elif defined(CPU_SH)
OUTPUT_FORMAT(elf32-sh)
#elif defined(CPU_MIPS)
OUTPUT_FORMAT(elf32-littlemips)
#else
/* We can have an #error here we don't use this file when build sims! */
#error Unknown CPU architecture
#endif

ENTRY (__header)

MEMORY {
	flatmem : ORIGIN = 0x0, LENGTH = 0xffffffff
}

#if 0
PHDRS {
	text PT_LOAD ;
	data PT_LOAD ;
}
#endif

SECTIONS {

	.text 0x0 : {
		/* . = . + 4; this was for pic libs */
		. = ALIGN(0x4) ;
		_stext = . ;
		/* rockbox specific plugin header section */
		*(.header)
		*(.text)
		*(.text.*)
#if (PLUGIN_IRAMSIZE == 0)
		*(.icode)
#endif
		*(.glue_7t)
		*(.glue_7)
		*(.gnu.warning)
		*(.stub)
		*(.gnu.linkonce.t*)
		*(.jcr)
		KEEP (*(.init))
		KEEP (*(.fini))

W_RODAT		*(.rodata)
W_RODAT		*(.rodata1)
W_RODAT		*(.rodata.*)
#if (PLUGIN_IRAMSIZE == 0)
W_RODAT		*(.irodata)
#endif
W_RODAT		*(.gnu.linkonce.r*)

		/* This is special code area at the end of the normal
		   text section.  It contains a small lookup table at
		   the start followed by the code pointed to by entries
		   in the lookup table.  */
		. = ALIGN (4) ;
		PROVIDE(__ctbp = .);
		*(.call_table_data)
		*(.call_table_text)
		. = ALIGN(0x20) ;
		_etext = . ;
	} > flatmem

	.icode : {
		*(.icode*)
	} > flatmem

	.data : {
		. = ALIGN(0x4) ;
		_sdata = . ;
		__data_start = . ;
		data_start = . ;
	/*	*(.got.plt) */
	/*	*(.got)     */
	/*	FILL(0) ;   */
	/*	. = ALIGN(0x20) ; */
	/*	LONG(-1)          */
	/*	. = ALIGN(0x20) ; */
R_RODAT		*(.rodata)
R_RODAT		*(.rodata1)
R_RODAT		*(.rodata.*)
#if (PLUGIN_IRAMSIZE == 0)
R_RODAT		*(.irodata)
#endif
R_RODAT		*(.gnu.linkonce.r*)
		*(.data)
		*(.data1)
		*(.data.*)
#if (PLUGIN_IRAMSIZE == 0)
		*(.idata)
#endif
		*(.gnu.linkonce.d*)

		/* Microblaze has .sdata and .sbss (small bss).  They must
		   be contiguous, so please don't move any of this. JW */
		_ssrw = . ;
		*(.sdata)
		*(.sdata.*)
		*(.sbss)			/* Don't move this! */
		*(.gnu.linkonce.sb*)
		_essrw = . ;

		_ssrw_size = _essrw - _ssrw;
		PROVIDE(_SDA_BASE_ = _ssrw + (_ssrw_size / 2));

		*(.gnu.linkonce.s.*)
		*(__libc_atexit)
		*(__libc_subinit)
		*(__libc_subfreeres)

		/* microblaze-specific read-only small data area
		   and associated locating symbols */
		_ssro = . ;
		*(.sdata2)
		_essro = . ;
		_ssro_size = _essro - _ssro;
		PROVIDE(_SDA2_BASE_ = _ssro + (_ssro_size / 2));
		. = ALIGN(4) ;
TOR:		__CTOR_LIST__ = .;
TOR:		LONG((__CTOR_END__ - __CTOR_LIST__) / 4 - 2)
SINGLE_LINK:	/* gcc uses crtbegin.o to find the start of
SINGLE_LINK:	   the constructors, so we make sure it is
SINGLE_LINK:	   first.  Because this is a wildcard, it
SINGLE_LINK:	   doesn't matter if the user does not
SINGLE_LINK:	   actually link against crtbegin.o; the
SINGLE_LINK:	   linker won't look for a file to match a
SINGLE_LINK:	   wildcard.  The wildcard also means that it
SINGLE_LINK:	   doesn't matter which directory crtbegin.o
SINGLE_LINK:	   is in.  */
SINGLE_LINK:	KEEP (*crtbegin*.o(.ctors))
SINGLE_LINK:	/* We don't want to include the .ctor section from
SINGLE_LINK:	   from the crtend.o file until after the sorted ctors.
SINGLE_LINK:	   The .ctor section from the crtend file contains the
SINGLE_LINK:	   end of ctors marker and it must be last */
SINGLE_LINK:	KEEP (*(EXCLUDE_FILE (*crtend*.o ) .ctors))
SINGLE_LINK:	KEEP (*(SORT(.ctors.*)))
		KEEP (*(.ctors))
TOR:		LONG(0)
TOR:		__CTOR_END__ = .;
TOR:		__DTOR_LIST__ = .;
TOR:		LONG((__DTOR_END__ - __DTOR_LIST__) / 4 - 2)
SINGLE_LINK:	KEEP (*crtbegin*.o(.dtors))
SINGLE_LINK:	KEEP (*(EXCLUDE_FILE (*crtend*.o ) .dtors))
SINGLE_LINK:	KEEP (*(SORT(.dtors.*)))
		KEEP (*(.dtors))
TOR:		LONG(0)
TOR:		__DTOR_END__ = .;

		PROVIDE (__preinit_array_start = .);
		KEEP (*(.preinit_array))
		PROVIDE (__preinit_array_end = .);
		PROVIDE (__init_array_start = .);
		KEEP (*(SORT(.init_array.*)))
		KEEP (*(.init_array))
		PROVIDE (__init_array_end = .);
		PROVIDE (__fini_array_start = .);
		KEEP (*(.fini_array))
		KEEP (*(SORT(.fini_array.*)))
		PROVIDE (__fini_array_end = .);
	} > flatmem

#ifdef NOCACHE_BASE
        /* We make sure this section is never empty
         * otherwise ld marks this section as NOLOAD
         * with section size not 0 due to ALIGN
         * directives. ld skips this section size
         * when calculating LMA of subsequent sections
         * but does not garbage collect it because
         * SIZEOF != 0. Because section does exist
         * in intermediate ELF, elf2flt takes its size
         * into account and produces wrong LMA in
         * subsequent sections.
         */
	.ncdata (. + NOCACHE_BASE) : {
	. = ALIGN(CACHEALIGN_SIZE);
		*(.ncdata*)
		LONG(-1);
	. = ALIGN(CACHEALIGN_SIZE);
	} AT > flatmem
#endif

#if (PLUGIN_IRAMSIZE != 0)
	.idata : {
		*(.irodata*)
		*(.idata*)
	} > flatmem
#endif

	.note.ABI-tag : { *(.note.ABI-tag) } > flatmem
	.eh_frame_hdr : { *(.eh_frame_hdr) } > flatmem
	.eh_frame : { KEEP(*(.eh_frame)) } > flatmem
	.gcc_except_table : {
		KEEP(*(.gcc_except_table))
		KEEP(*(.gcc_except_table.*))
	} > flatmem


	. = ALIGN(CACHEALIGN_SIZE) ;
	_edata = . ;

	.bss (NOLOAD) : {
		. = ALIGN(0x4) ;
		_sbss = ALIGN(0x4) ;
		__bss_start = . ;
		*(.dynsbss)
		*(.sbss)
		*(.sbss.*)
		*(.scommon)
		*(.dynbss)
		*(.bss)
		*(.bss.*)
		*(.bss*)
		*(.gnu.linkonce.b*)
#if (PLUGIN_IRAMSIZE == 0)
		*(.ibss)
#endif
		*(COMMON)
		. = ALIGN(0x10) ;
		_ebss = . ;
		_end = . ;
		end = . ;
	} > flatmem

#ifdef NOCACHE_BASE
	.ncbss . + NOCACHE_BASE (NOLOAD) : {
		. = ALIGN(CACHEALIGN_SIZE);
		*(.ncbss*)
		. = ALIGN(CACHEALIGN_SIZE);
	} AT> flatmem
#endif

#if (PLUGIN_IRAMSIZE != 0)
	.ibss (NOLOAD) : {
		*(.ibss)
	} > flatmem
#endif
	.stack : {
		. = ALIGN(0x4);
		_stack_start = .;
	}

	.junk 0 : { *(.rel*) *(.rela*) }
	/DISCARD/ : { *(.note.GNU-stack) }
	/* Stabs debugging sections.    */
	.stab 0 : { *(.stab) }
	.stabstr 0 : { *(.stabstr) }
	.stab.excl 0 : { *(.stab.excl) }
	.stab.exclstr 0 : { *(.stab.exclstr) }
	.stab.index 0 : { *(.stab.index) }
	.stab.indexstr 0 : { *(.stab.indexstr) }
	.comment 0 : { *(.comment) }
	/* DWARF debug sections.
	   Symbols in the DWARF debugging sections are relative to the beginning
	   of the section so we begin them at 0.  */
	/* DWARF 1 */
	.debug          0 : { *(.debug) }
	.line           0 : { *(.line) }
	/* GNU DWARF 1 extensions */
	.debug_srcinfo  0 : { *(.debug_srcinfo) }
	.debug_sfnames  0 : { *(.debug_sfnames) }
	/* DWARF 1.1 and DWARF 2 */
	.debug_aranges  0 : { *(.debug_aranges) }
	.debug_pubnames 0 : { *(.debug_pubnames) }
	/* DWARF 2 */
	.debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
	.debug_abbrev   0 : { *(.debug_abbrev) }
	.debug_line     0 : { *(.debug_line) }
	.debug_frame    0 : { *(.debug_frame) }
	.debug_str      0 : { *(.debug_str) }
	.debug_loc      0 : { *(.debug_loc) }
	.debug_macinfo  0 : { *(.debug_macinfo) }
	/* SGI/MIPS DWARF 2 extensions */
	.debug_weaknames 0 : { *(.debug_weaknames) }
	.debug_funcnames 0 : { *(.debug_funcnames) }
	.debug_typenames 0 : { *(.debug_typenames) }
	.debug_varnames  0 : { *(.debug_varnames) }
}
