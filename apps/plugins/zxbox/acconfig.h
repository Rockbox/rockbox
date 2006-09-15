
/* Define this on i386 architectures if the C compiler generates symbols
   beginning with underscores, eg. on old aout versions of Linux */
#undef AOUT_FORMAT

/* Define this to enable running in background on the Linux console.
   Works only with SVGALIB 1.2.11 or newer */
#undef RUN_IN_BACKGROUND

/* Define this if Xlib has the MITSHM extension */
#undef HAVE_MITSHM

/* Define this if program can query MITSHM extension */
#undef HAVE_SHMQUERY

/* Define this if you have the readline library */
#undef HAVE_READLINE

/* Define this to use the C version of the program insead of the 
   i386 assembly. Define this on non intel machines */
#undef Z80C

/* Always define this for the spectrum emulator. */
#undef SPECT_MEM

/* Define if sound driver is available. */
#undef HAVE_SOUND

/* Define if sound driver is Open Sound System (OSS) */
#undef OSS_SOUND

/* Define if sound driver is SUN */
#undef SUN_SOUND

/* Define this to use the inline intel assembly sections */
#undef I386_ASM

/* Define this to use an alternative way of passing the z80 processor
   data to the z80 instruction emulation functions. May make emulation
   faster on some machines, but not on intel, and sparc. */
#undef PROCP

@TOP@
