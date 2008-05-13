#ifndef ZXCONFIG_H
#define ZXCONFIG_H

#include <plugin.h>
extern int load_tap;
extern bool clear_kbd;
extern bool exit_requested;
extern const struct plugin_api* rb;
extern void press_key(int c);
extern long video_frames;
extern long start_time;
extern int intkeys[5];

#define ZX_WIDTH 256
#define ZX_HEIGHT 192

#define SETTINGS_MIN_VERSION 2
#define SETTINGS_VERSION 2

/* undef not to use greyscale lib */
#if !defined HAVE_LCD_COLOR
#define USE_GREY
#define USE_BUFFERED_GREY
#endif


#define Z80C

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
/* seems not to have effect on arm targets */
#undef PROCP
/*#define PROCP*/

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

#endif
