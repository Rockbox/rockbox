// Library configuration. Modify this file as necessary.

#ifndef BLARGG_CONFIG_H
#define BLARGG_CONFIG_H

// Uncomment to enable platform-specific optimizations
//#define BLARGG_NONPORTABLE 1

// Uncomment if automatic byte-order determination doesn't work
#ifdef ROCKBOX_BIG_ENDIAN
	#define BLARGG_BIG_ENDIAN 1
#endif

// Uncomment if you get errors in the bool section of blargg_common.h
#define BLARGG_COMPILER_HAS_BOOL 1

// Uncomment to use fast gb apu implementation
// #define GB_APU_FAST 1

// Uncomment to remove agb emulation support
// #define GB_APU_NO_AGB 1

// Uncomment to emulate only nes apu
// #define NSF_EMU_APU_ONLY 1

// Uncomment to remove vrc7 apu support
// #define NSF_EMU_NO_VRC7 1

// Uncomment to remove fmopl apu support
// #define KSS_EMU_NO_FMOPL 1

// To handle undefined reference to assert
#define NDEBUG 1

// Use standard config.h if present
#define HAVE_CONFIG_H 1

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#endif
