#ifndef _CONFIG_TREMOR_H
#define _CONFIG_TREMOR_H

#include "codeclib.h" 

#ifdef CPU_ARM
#define _ARM_ASSEM_
#endif

#ifndef BYTE_ORDER
#ifdef ROCKBOX_BIG_ENDIAN
#define BIG_ENDIAN 1
#define LITTLE_ENDIAN 0
#define BYTE_ORDER BIG_ENDIAN
#else
#define BYTE_ORDER LITTLE_ENDIAN
#define LITTLE_ENDIAN 1
#define BIG_ENDIAN 0
#endif
#endif

#ifndef ICODE_ATTR_TREMOR_NOT_MDCT
#define ICODE_ATTR_TREMOR_NOT_MDCT ICODE_ATTR
#endif

/* Enable special handling of buffers in faster ram, not usefull when no
   such different ram is available. There are 3 different memory configurations
   * No special iram, uses double buffers to avoid copying data
   * Small special iram, copies buffers to run hottest processing in iram
   * Large iram, uses double buffers in iram */
#ifdef USE_IRAM
#define TREMOR_USE_IRAM

/* Define CPU of large IRAM (PP5022/5024, MCF5250)     */
#if (CONFIG_CPU == PP5022) || (CONFIG_CPU == PP5024) || defined(CPU_S5L870X) || (CONFIG_CPU == MCF5250)
/* PCM_BUFFER    : 32768 byte (4096*2*4 or 2048*4*4)   *
 * WINDOW_LOOKUP : 9216 Byte (256*4 + 2048*4)          *
 * TOTAL         : 41984                               */
#define IRAM_IBSS_SIZE 41984

/* Define CPU of Normal IRAM (96KB)                    */
#else
/* floor and double residue buffer : 24576 Byte (2048/2*4*2*3)  *
 * WINDOW_LOOKUP                   : 4608 Byte (128*4 + 1024*4) *
 * TOTAL                           : 29184                      */
#define IRAM_IBSS_SIZE 29184
#endif
#endif

/* max 2 channels  */
#define CHANNELS 2

// #define _LOW_ACCURACY_

#endif /*  _CONFIG_TREMOR_H */
