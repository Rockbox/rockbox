
#ifndef RBCONFIG_H
#define RBCONFIG_H

#include <plugin.h>

extern const struct plugin_api *rb;

typedef uint32_t        UINT32;
typedef uint16_t        UINT16;

typedef uint8_t         BYTE;
typedef int8_t          SIGNED_BYTE;
typedef unsigned int    ADDRESS;
typedef unsigned long   CLOCK;


/* ====================================================================== */

#define VERSION_MAIN   "1.7"

// TODO: move to a different place, e.g. keyboard.c
struct rb_button {
    int joy1_left;
    int joy1_right;
    int joy1_up;
    int joy1_down;
    int joy1_trigger;
    int joy2_left;
    int joy2_right;
    int joy2_up;
    int joy2_down;
    int joy2_trigger;
    int console_select;
    int console_reset;
    /* TODO: implement other controllers (paddle) */
};
extern struct rb_button input;

// -----------------------------------------------------
// Stuff for optimizing etc.

/* this should be set if the target is fast, i.e. is capable of emulating
 * full 60fps. Without this define it is not possible to reduce the frame rate! */
#ifdef SIMULATOR
#   define IS_FAST_TARGET
#endif

/* enable/disable inlining. */
#define INLINE      inline

/* the cold attribute informs the compiler that the function should be optimized
 * for size rather than speed. */
#define COLD_ATTR   __attribute__((cold))


// To be defined
// global optimizations off macro. undefine if needed
/* #define NEVER_OPTIMIZE */
// maximum debugging/verbosity/...
/* #define SOMETHINGSOMETHING */


// helpfull macros
#define ARRAY_LEN(array)     (sizeof(array) / sizeof(array[0]))


#endif
