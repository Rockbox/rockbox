/**
 * @file bitstream.h
 * bitstream api header.
 */

#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <inttypes.h>
#include "ffmpeg_get_bits.h"

#ifndef BUILD_STANDALONE
  #include "platform.h"
#else
  #include <stdio.h>
  #define IBSS_ATTR
  #define ICONST_ATTR
  #define ICODE_ATTR
#endif

#if (CONFIG_CPU == MCF5250) || (CONFIG_CPU == PP5022) || \
    (CONFIG_CPU == PP5024) || (CONFIG_CPU == S5L8702)
#define ICODE_ATTR_FLAC ICODE_ATTR
#define IBSS_ATTR_FLAC  IBSS_ATTR
/* Enough IRAM to move additional data to it. */
#define IBSS_ATTR_FLAC_LARGE_IRAM   IBSS_ATTR
#define IBSS_ATTR_FLAC_XLARGE_IRAM
#define IBSS_ATTR_FLAC_XXLARGE_IRAM

#elif (CONFIG_CPU == S5L8700) || (CONFIG_CPU == S5L8701)
#define ICODE_ATTR_FLAC ICODE_ATTR
#define IBSS_ATTR_FLAC  IBSS_ATTR
/* Enough IRAM to move even more additional data to it. */
#define IBSS_ATTR_FLAC_LARGE_IRAM   IBSS_ATTR
#define IBSS_ATTR_FLAC_XLARGE_IRAM  IBSS_ATTR
#define IBSS_ATTR_FLAC_XXLARGE_IRAM

#else
#define ICODE_ATTR_FLAC ICODE_ATTR
#define IBSS_ATTR_FLAC  IBSS_ATTR
/* Not enough IRAM available. */
#define IBSS_ATTR_FLAC_LARGE_IRAM
#define IBSS_ATTR_FLAC_XLARGE_IRAM
#define IBSS_ATTR_FLAC_XXLARGE_IRAM
#endif

/* Endian conversion routines for standalone compilation */
#ifdef BUILD_STANDALONE
    #ifdef BUILD_BIGENDIAN
        #define betoh32(x) (x)
        #define letoh32(x) swap32(x)
    #else
        #define letoh32(x) (x)
        #define betoh32(x) swap32(x)
    #endif

    /* Taken from rockbox/firmware/export/system.h */

    static inline unsigned short swap16(unsigned short value)
        /*
          result[15..8] = value[ 7..0];
          result[ 7..0] = value[15..8];
        */
    {
        return (value >> 8) | (value << 8);
    }

    static inline unsigned long swap32(unsigned long value)
        /*
          result[31..24] = value[ 7.. 0];
          result[23..16] = value[15.. 8];
          result[15.. 8] = value[23..16];
          result[ 7.. 0] = value[31..24];
        */
    {
        unsigned long hi = swap16(value >> 16);
        unsigned long lo = swap16(value & 0xffff);
        return (lo << 16) | hi;
    }
#endif

#endif /* BITSTREAM_H */
