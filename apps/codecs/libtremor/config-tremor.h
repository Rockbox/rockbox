#ifndef _CONFIG_TREMOR_H
#define _CONFIG_TREMOR_H

#include "codeclib.h" 

#ifdef CPU_ARM
#define _ARM_ASSEM_
#endif

#ifdef ROCKBOX_BIG_ENDIAN
#define BIG_ENDIAN 1
#define LITTLE_ENDIAN 0
#define BYTE_ORDER BIG_ENDIAN
#else
#define BYTE_ORDER LITTLE_ENDIAN
#define LITTLE_ENDIAN 1
#define BIG_ENDIAN 0
#endif

#ifndef ICODE_ATTR_TREMOR_MDCT
#define ICODE_ATTR_TREMOR_MDCT ICODE_ATTR
#endif

/* Workaround for gcc bug where all static functions are called with short 
   calls */
#if !defined(ICODE_ATTR_TREMOR_NOT_MDCT) && (CONFIG_CPU==S5L8701)
#define STATICIRAM_NOT_MDCT 
#else
#define STATICIRAM_NOT_MDCT static
#endif

#ifndef ICODE_ATTR_TREMOR_NOT_MDCT
#define ICODE_ATTR_TREMOR_NOT_MDCT ICODE_ATTR
#endif

/* Define CPU of large IRAM (MCF5250)                  */
#if (CONFIG_CPU == MCF5250)
/* PCM_BUFFER    : 32768 Byte (4096*2*4)               *
 * WINDOW_LOOKUP : 4608 Byte (128*4 + 1024*4)          *
 * TOTAL         : 37376                               */
#define IRAM_IBSS_SIZE 37376

/* Define CPU of large IRAM (PP5022/5024)              */
#elif (CONFIG_CPU == PP5022) || (CONFIG_CPU == PP5024) || defined(CPU_S5L870X)
/* PCM_BUFFER    : 32768 byte (4096*2*4 or 2048*4*4)   *
 * WINDOW_LOOKUP : 9216 Byte (256*4 + 2048*4)          *
 * TOTAL         : 41984                               */
#define IRAM_IBSS_SIZE 41984

/* Define CPU of Normal IRAM (96KB) (and SIM also)     */
#else
/* PCM_BUFFER    : 16384 Byte (2048*2*4)               *
 * WINDOW_LOOKUP : 4608 Byte (128*4 + 1024*4)          *
 * TOTAL         : 20992                               */
#define IRAM_IBSS_SIZE 20992
#endif

/* max 2 channels  */
#define CHANNELS 2

// #define _LOW_ACCURACY_

#endif /*  _CONFIG_TREMOR_H */
