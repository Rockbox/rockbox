/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Miscellaneous helper API declarations
 *
 * Copyright (c) 2007 Michael Sevakis
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef MPEG_MISC_H
#define MPEG_MISC_H

/* Miscellaneous helpers */
#ifndef ALIGNED_ATTR
#define ALIGNED_ATTR(x) __attribute__((aligned(x)))
#endif

/* Generic states for when things are too simple to care about naming them */
enum state_enum
{
    state0 = 0,
    state1,
    state2,
    state3,
    state4,
    state5,
    state6,
    state7,
    state8,
    state9,
};

/* Macros for comparing memory bytes to a series of constant bytes in an
   efficient manner - evaluate to true if corresponding bytes match */
#if defined (CPU_ARM)
/* ARM must load 32-bit values at addres % 4 == 0 offsets but this data
   isn't aligned nescessarily, so just byte compare */
#define CMP_3_CONST(_a, _b) \
    ({  int _x;                             \
        asm volatile (                      \
            "ldrb   %[x], [%[a], #0]  \n"   \
            "eors   %[x], %[x], %[b0] \n"   \
            "ldreqb %[x], [%[a], #1]  \n"   \
            "eoreqs %[x], %[x], %[b1] \n"   \
            "ldreqb %[x], [%[a], #2]  \n"   \
            "eoreqs %[x], %[x], %[b2] \n"   \
            : [x]"=&r"(_x)                  \
            : [a]"r"(_a),                   \
              [b0]"i"(((_b) >> 24) & 0xff), \
              [b1]"i"(((_b) >> 16) & 0xff), \
              [b2]"i"(((_b) >>  8) & 0xff)  \
        );                                  \
        _x == 0; })

#define CMP_4_CONST(_a, _b) \
    ({  int _x;                             \
        asm volatile (                      \
            "ldrb   %[x], [%[a], #0]  \n"   \
            "eors   %[x], %[x], %[b0] \n"   \
            "ldreqb %[x], [%[a], #1]  \n"   \
            "eoreqs %[x], %[x], %[b1] \n"   \
            "ldreqb %[x], [%[a], #2]  \n"   \
            "eoreqs %[x], %[x], %[b2] \n"   \
            "ldreqb %[x], [%[a], #3]  \n"   \
            "eoreqs %[x], %[x], %[b3] \n"   \
            : [x]"=&r"(_x)                  \
            : [a]"r"(_a),                   \
              [b0]"i"(((_b) >> 24) & 0xff), \
              [b1]"i"(((_b) >> 16) & 0xff), \
              [b2]"i"(((_b) >>  8) & 0xff), \
              [b3]"i"(((_b)      ) & 0xff)  \
        );                                  \
        _x == 0; })

#elif defined (CPU_COLDFIRE)
/* Coldfire can just load a 32 bit value at any offset but ASM is not the
   best way to integrate this with the C code */
#define CMP_3_CONST(a, b) \
    (((*(uint32_t *)(a) >> 8) == ((uint32_t)(b) >> 8)))

#define CMP_4_CONST(a, b) \
    ((*(uint32_t *)(a) == (b)))

#else
/* Don't know what this is - use bytewise comparisons */
#define CMP_3_CONST(a, b) \
    (( ((a)[0] ^ (((b) >> 24) & 0xff)) | \
       ((a)[1] ^ (((b) >> 16) & 0xff)) | \
       ((a)[2] ^ (((b) >>  8) & 0xff)) ) == 0)

#define CMP_4_CONST(a, b) \
    (( ((a)[0] ^ (((b) >> 24) & 0xff)) | \
       ((a)[1] ^ (((b) >> 16) & 0xff)) | \
       ((a)[2] ^ (((b) >>  8) & 0xff)) | \
       ((a)[3] ^ (((b)      ) & 0xff)) ) == 0)
#endif /* CPU_* */


/** Streams **/

/* Convert PTS/DTS ticks to our clock ticks */
#define TS_TO_TICKS(pts) ((uint64_t)CLOCK_RATE*(pts) / TS_SECOND)
/* Convert our clock ticks to PTS/DTS ticks */
#define TICKS_TO_TS(ts)  ((uint64_t)TS_SECOND*(ts) / CLOCK_RATE)
/* Convert timecode ticks to our clock ticks */
#define TC_TO_TICKS(stamp) ((uint64_t)CLOCK_RATE*(stamp) / TC_SECOND)
/* Convert our clock ticks to timecode ticks */
#define TICKS_TO_TC(stamp) ((uint64_t)TC_SECOND*(stamp) / CLOCK_RATE)
/* Convert timecode ticks to timestamp ticks */
#define TC_TO_TS(stamp) ((stamp) / 600)

/*
 * S = start position, E = end position
 *
 * pos:
 *     initialize to search start position (S)
 *
 * len:
 *     initialize to = ABS(S-E)
 *     scanning = remaining bytes in scan direction
 *
 * dir:
 *     scan direction; >= 0 == forward, < 0 == reverse
 *
 * margin:
 *     amount of data to right of cursor - initialize by stream_scan_normalize
 *
 * data:
 *     Extra data used/returned by the function implemented
 *
 * Forward scan:
 *     S      pos         E
 *     |       *<-margin->| dir->
 *     |       |<--len--->|
 *
 * Reverse scan:
 *     E      pos         S
 *     |<-len->*<-margin->| <-dir
 *     |       |          |
 */
struct stream_scan
{
    off_t   pos;    /* Initial scan position (file offset) */
    ssize_t len;    /* Maximum length of scan */
    off_t   dir;    /* Direction - >= 0; forward, < 0 backward */
    ssize_t margin; /* Used by function to track margin between position and data end */
    intptr_t data;  /* */
};

#define SSCAN_REVERSE (-1)
#define SSCAN_FORWARD 1

/* Ensures direction is -1 or 1 and margin is properly initialized */
void stream_scan_normalize(struct stream_scan *sk);

/* Moves a scan cursor. If amount is positive, the increment is in the scan
 * direction, otherwise opposite the scan direction */
void stream_scan_offset(struct stream_scan *sk, off_t by);

/** Time helpers **/
struct hms
{
    unsigned int hrs;
    unsigned int min;
    unsigned int sec;
    unsigned int frac;
};

void ts_to_hms(uint32_t ts, struct hms *hms);
void hms_format(char *buf, size_t bufsize, struct hms *hms);

/** Maths **/

/* Moving average */
#define AVERAGE(var, x, count) \
    ({ typeof (count) _c = (count);   \
       ((var) * (_c-1) + (x)) / (_c); })

/* Multiply two unsigned 32-bit integers yielding a 64-bit result and
 * divide by another unsigned 32-bit integer to yield a 32-bit result.
 * Rounds to nearest with saturation. */
uint32_t muldiv_uint32(uint32_t multiplicand,
                       uint32_t multiplier,
                       uint32_t divisor);

#endif /* MPEG_MISC_H */
