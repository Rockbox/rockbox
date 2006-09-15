#ifndef ZXCONFIG_H
#define ZXCONFIG_H

#include <plugin.h>
extern int load_tap;
extern bool clear_kbd;
extern bool exit_requested;
extern struct plugin_api* rb;
extern void press_key(int c);
extern long video_frames;
extern long start_time;
extern int intkeys[5];

#define ZX_WIDTH 256
#define ZX_HEIGHT 192

#define SETTINGS_MIN_VERSION 2
#define SETTINGS_VERSION 2

/* undef not to use grayscale lib */
#if !defined HAVE_LCD_COLOR && LCD_PIXELFORMAT != HORIZONTAL_PACKING
#define USE_GRAY
#define USE_BUFFERED_GRAY
#endif


#define Z80C
#define MULTIUSER 0
#define DATADIR "."

/* Always define this for the spectrum emulator. */
#define SPECT_MEM 1

/* Define if sound driver is available. */
#if CONFIG_CODEC == SWCODEC && !defined SIMULATOR
#define HAVE_SOUND
#endif

/* Define this to use the inline intel assembly sections */
#undef I386_ASM

/* Define this to use an alternative way of passing the z80 processor
   data to the z80 instruction emulation functions. May make emulation
   faster on some machines, but not on intel, and sparc. */
#undef PROCP /* seems not to have effect on arm targets */
/* #define PROCP */

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if your processor stores words with the most significant
   byte first (like Motorola and SPARC, unlike Intel and VAX).  */
/* #undef WORDS_BIGENDIAN */

#endif
