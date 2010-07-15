/**
 * @file bitstream.h
 * bitstream api header.
 */

#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <inttypes.h>
#include "ffmpeg_get_bits.h"

#ifndef BUILD_STANDALONE
  #include <config.h>
  #include <system.h>
#else
  #include <stdio.h>
  #define IBSS_ATTR
  #define ICONST_ATTR
  #define ICODE_ATTR
#endif

#ifndef ICODE_ATTR_FLAC
#define ICODE_ATTR_FLAC ICODE_ATTR
#endif

#ifndef IBSS_ATTR_FLAC_DECODED0
#define IBSS_ATTR_FLAC_DECODED0 IBSS_ATTR
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
