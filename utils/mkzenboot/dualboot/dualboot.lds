ENTRY(start)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)

MEMORY
{
    /* keep this consistent with the address in mkzenboot.c */
#if IMX233_SUBTARGET == 3700
    RAM : ORIGIN = 0x41000000, LENGTH = 0x8000
#elif IMX233_SUBTARGET == 3600
    RAM : ORIGIN = 0x61000000, LENGTH = 0x8000
#else
#error define me
#endif
}

SECTIONS
{
    .text :
    {
        *(.start*)
        *(.text*)
        . = ALIGN(4);
        LONG(0x1ceb00da)
        of_vector = .;
        . += 4;
        rb_vector = .;
        . += 4;
        boot_arg = .;
        . += 4;
    } > RAM
}
