/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Gary Czvitkovicz
 * Copyright (C) 2017 by Michael A. Sevakis
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
#include <sys/types.h>
#include <limits.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include "system.h"
#include "vuprintf.h"
#include "ap_int.h"

#ifndef BOOTLOADER

/* Turn everything on if we have enough RAM. */
#if MEMORYSIZE >= 8
# define FMT_LENMOD (0xffffffff)
# define FMT_RADIX  (0xffffffff)
#endif

#endif

/* these are the defaults if no other preference is given */
#ifndef FMT_LENMOD
#define FMT_LENMOD      (FMT_LENMOD_l  | \
                         FMT_LENMOD_z)
#endif /* FMT_LENMOD */

#ifndef FMT_RADIX
#define FMT_RADIX       (FMT_RADIX_c | \
                         FMT_RADIX_d | \
                         FMT_RADIX_p | \
                         FMT_RADIX_s | \
                         FMT_RADIX_u | \
                         FMT_RADIX_x)
#endif /* FMT_RADIX */

/** Length modifier and radix flags **/

/* compulsory length modifiers: NONE
 * however a compatible 'l' or 'll' must be defined if another requires it */
#define FMT_LENMOD_h    0x001  /* signed/unsigned short (%h<radix>) */
#define FMT_LENMOD_hh   0x002  /* signed/unsigned char (%hh<radix>) */
#define FMT_LENMOD_j    0x004  /* intmax_t/uintmax_t (%j<radix>) */
#define FMT_LENMOD_l    0x008  /* signed/unsigned long (%l<radix>) */
#define FMT_LENMOD_ll   0x010  /* signed/unsigned long long (%ll<radix>) */
#define FMT_LENMOD_t    0x020  /* signed/unsigned ptrdiff_t (%t<radix>) */
#define FMT_LENMOD_z    0x040  /* size_t/ssize_t (%z<radix>) */
#if 0
#define FMT_LENMOD_L    0x080  /* long double (instead of double) */
#else
#define FMT_LENMOD_L    0x000
#endif

/* compulsory radixes: c, d, i, u, s */
#define FMT_RADIX_c     0x001  /* single character (%c) */
#define FMT_RADIX_d     0x002  /* signed integer type, decimal (%d %i) */
#define FMT_RADIX_n     0x004  /* bytes output so far (%n) */
#define FMT_RADIX_o     0x008  /* unsigned integer type, octal (%o) */
#define FMT_RADIX_p     0x010  /* pointer (%p %P) */
#define FMT_RADIX_s     0x020  /* string (%s) */
#define FMT_RADIX_u     0x040  /* unsigned integer type, decimal (%u) */
#define FMT_RADIX_x     0x080  /* unsigned integer type, hex (%x %X) */
#define FMT_RADIX_a     0x100  /* hex floating point "[-]0xh.hhhhp±d" */
#define FMT_RADIX_e     0x200  /* floating point with exponent "[-]d.ddde±dd" */
#define FMT_RADIX_f     0x400  /* floating point "[-]ddd.ddd" */
#define FMT_RADIX_g     0x800  /* floating point exponent or decimal depending
                                  upon value and precision */

/* TODO: 'a' 'A' */
#define FMT_RADIX_floats (FMT_RADIX_e|FMT_RADIX_f|FMT_RADIX_g)

#if (FMT_RADIX & FMT_RADIX_floats)
/* Assumes IEEE 754 double-precision, native-endian; replace to parse and init
   for some other format */
#define parse_double            parse_ieee754_double
#define init_double_chunks      init_ieee754_double_chunks
#define format_double_int10     format_ap_int10
#define format_double_frac10    format_ap_frac10
#endif

/* avoid defining redundant functions if two or more types can use the same
 * something not getting a macro means it gets assigned its own value and
 * formatter */

/* l */
#if LONG_MIN == INT_MIN && LONG_MAX == INT_MAX
#define val_ld          val_d
#define format_ld       format_d
#define branch_fmt_ld   branch_fmt_d
#elif !(FMT_LENMOD & FMT_LENMOD_l) /* unique */
#define val_ld
#endif /* LONG_ */

#if ULONG_MAX == UINT_MAX
#define val_lu          val_u
#define format_lu       format_u
#define branch_fmt_lu   branch_fmt_u
#elif !(FMT_LENMOD & FMT_LENMOD_l) /* unique */
#define val_lu
#endif /* ULONG_ */

/* ll */
#if LLONG_MIN == INT_MIN && LLONG_MAX == INT_MAX
#define val_lld         val_d
#define format_lld      format_d
#define branch_fmt_lld  branch_fmt_d
#elif LLONG_MIN == LONG_MIN && LLONG_MAX == LONG_MAX
#define val_lld         val_ld
#define format_lld      format_ld
#define branch_fmt_lld  branch_fmt_ld
#elif !(FMT_LENMOD & FMT_LENMOD_ll) /* unique */
#define val_lld
#endif /* LLONG_ */

#if ULLONG_MAX == UINT_MAX
#define val_llu         val_u
#define format_llu      format_u
#define branch_fmt_llu  branch_fmt_u
#elif ULLONG_MAX == ULONG_MAX
#define val_llu         val_lu
#define format_llu      format_lu
#define branch_fmt_llu  branch_fmt_lu
#elif !(FMT_LENMOD & FMT_LENMOD_ll) /* unique */
#define val_llu
#endif /* ULLONG_ */

/* char/short parameter type promotions */
#define SCHAR_INT_ARG   int
#define UCHAR_INT_ARG   int
#define SSHRT_INT_ARG   int
#if USHRT_MAX == UINT_MAX
#define USHRT_INT_ARG   unsigned int
#else
#define USHRT_INT_ARG   int
#endif

/* some macros to have conditional work inside macros */
#if (FMT_LENMOD & FMT_LENMOD_l)
#define IF_FMT_LENMOD_l(...)    __VA_ARGS__
#else
#define IF_FMT_LENMOD_l(...)
#endif

#if (FMT_LENMOD & FMT_LENMOD_ll)
#define IF_FMT_LENMOD_ll(...)   __VA_ARGS__
#else
#define IF_FMT_LENMOD_ll(...)
#endif

#if (FMT_RADIX & FMT_RADIX_o)
#define IF_FMT_RADIX_o(...)     __VA_ARGS__
#else
#define IF_FMT_RADIX_o(...)
#endif

#if (FMT_RADIX & FMT_RADIX_x)
#define IF_FMT_RADIX_x(...)     __VA_ARGS__
#else
#define IF_FMT_RADIX_x(...)
#endif

/* synthesize multicharacter constant */
#define LENMOD2(cv, ch) \
    (((cv) << CHAR_BIT) | (ch))

#define LENMOD_NONE 0

#if (FMT_LENMOD & FMT_LENMOD_h)
#define LENMOD_h    'h'
#endif
#if (FMT_LENMOD & FMT_LENMOD_hh)
#define LENMOD_hh   LENMOD2('h', 'h') /* 'hh' */
#endif
#if (FMT_LENMOD & FMT_LENMOD_j)
#define LENMOD_j    'j'
#endif
#if (FMT_LENMOD & FMT_LENMOD_l)
#define LENMOD_l    'l'
#endif
#if (FMT_LENMOD & FMT_LENMOD_ll)
#undef FMT_MAX_L
#define LENMOD_ll   LENMOD2('l', 'l') /* 'll' */
#endif
#if (FMT_LENMOD & FMT_LENMOD_t)
#define LENMOD_t    't'
#endif
#if (FMT_LENMOD & FMT_LENMOD_z)
#define LENMOD_z    'z'
#endif

/* select type-compatible length modifier
 * (a bit hacky; it should be range-based) */
#define LENMOD_INTCOMPAT_SEL(type, signd) \
    ({  int __lenmod;                                                 \
        size_t __size = sizeof (type);                                \
        if (__size == ((signd) ? sizeof (int) :                       \
                                 sizeof (unsigned int))) {            \
            __lenmod = LENMOD_NONE;                                   \
        }                                                             \
        else if (__size == ((signd) ? sizeof (long) :                 \
                                      sizeof (unsigned long))) {      \
            IF_FMT_LENMOD_l(__lenmod = LENMOD_l;)                     \
        }                                                             \
        else if (__size == ((signd) ? sizeof (long long) :            \
                                      sizeof (unsigned long long))) { \
            IF_FMT_LENMOD_ll(__lenmod = LENMOD_ll;)                   \
        }                                                             \
        __lenmod; })

/* call formatting function for the compatible integer type */
#define LENMOD_INTCOMPAT_CALL(inteqv, val, fmt_buf, x, signd) \
    ({  const char *__buf;                          \
        switch (inteqv) {                           \
        case LENMOD_NONE:                           \
            __buf = (signd) ?                       \
                format_d((val), (fmt_buf), (x)) :   \
                format_u((val), (fmt_buf), (x));    \
            break;                                  \
        IF_FMT_LENMOD_l(                            \
        case LENMOD_l:                              \
            __buf = (signd) ?                       \
                format_ld((val), (fmt_buf), (x)) :  \
                format_lu((val), (fmt_buf), (x));   \
            break;                                  \
        )                                           \
        IF_FMT_LENMOD_ll(                           \
        case LENMOD_ll:                             \
            __buf = (signd) ?                       \
                format_lld((val), (fmt_buf), (x)) : \
                format_llu((val), (fmt_buf), (x));  \
            break;                                  \
        )                                           \
        }                                           \
        __buf;                                      \
    })

/* execute formatting branch for the compatible integer type */
#define LENMOD_INTCOMPAT_BRANCH(inteqv, val, signd)  \
    ({  switch (inteqv) {                       \
        case LENMOD_NONE:                       \
            if (signd) {                        \
                val_d = (val);                  \
                goto branch_fmt_d;              \
            }                                   \
            else {                              \
                val_u = (val);                  \
                goto branch_fmt_u;              \
            }                                   \
        IF_FMT_LENMOD_l(                        \
        case LENMOD_l:                          \
            if (signd) {                        \
                val_ld = (val);                 \
                goto branch_fmt_ld;             \
            }                                   \
            else {                              \
                val_lu = (val);                 \
                goto branch_fmt_lu;             \
            }                                   \
        )                                       \
        IF_FMT_LENMOD_ll(                       \
        case LENMOD_ll:                         \
            if (signd) {                        \
                val_lld = (val);                \
                goto branch_fmt_lld;            \
            }                                   \
            else {                              \
                val_llu = (val);                \
                goto branch_fmt_llu;            \
            }                                   \
        )                                       \
        }                                       \
    })

#define CONVERT_RADIX_10_SIGN(val, fmt_buf, p, signchar, type) \
    do {                                        \
        if (val) {                              \
            unsigned type v;                    \
                                                \
            if (val < 0) {                      \
                v = (typeof (v))-(val + 1) + 1; \
                signchar = '-';                 \
            }                                   \
            else {                              \
                v = val;                        \
            }                                   \
                                                \
            do {                                \
                *--p = (v % 10) + '0';          \
                v /= 10;                        \
            } while (v);                        \
        }                                       \
                                                \
        if (signchar) {                         \
            p[-1] = signchar;                   \
            fmt_buf->length = 1;                \
            break;                              \
        }                                       \
                                                \
        fmt_buf->length = 0;                    \
    } while (0)

#define CONVERT_RADIX_8(val, fmt_buf, p) \
    do {                                        \
        if (val) {                              \
            typeof (val) v = val;               \
                                                \
            do {                                \
                *--p = (v % 010) + '0';         \
                v /= 010;                       \
            } while (v);                        \
        }                                       \
                                                \
        if (fmt_buf->length) {                  \
            *--p = '0';                         \
            fmt_buf->length = 0;                \
        }                                       \
    } while (0)

#define CONVERT_RADIX_10(val, fmt_buf, p) \
    do {                                        \
        if (val) {                              \
            typeof (val) v = val;               \
                                                \
            do {                                \
                *--p = (v % 10) + '0';          \
                v /= 10;                        \
            } while (v);                        \
        }                                       \
                                                \
        fmt_buf->length = 0;                    \
    } while (0)

#define CONVERT_RADIX_16(val, fmt_buf, p, x) \
    do {                                        \
        if (val) {                              \
            const int h = x - 'X' - 0xA         \
                            + 'A' - '0';        \
            typeof (val) v = val;               \
                                                \
            do {                                \
                unsigned int d = v % 0x10;      \
                if (d >= 0xA) {                 \
                    d += h;                     \
                }                               \
                *--p = d + '0';                 \
                v /= 0x10;                      \
            } while (v);                        \
                                                \
            if (fmt_buf->length) {              \
                p[-1] = x;                      \
                p[-2] = '0';                    \
                fmt_buf->length = 2;            \
                break;                          \
            }                                   \
        }                                       \
                                                \
        fmt_buf->length = 0;                    \
    } while (0)

#define CONVERT_RADIX_NOSIGN(val, fmt_buf, p, x) \
    switch (x)                                  \
    {                                           \
    IF_FMT_RADIX_o( case 'o':                   \
        CONVERT_RADIX_8(val, fmt_buf, p);       \
        break; )                                \
    case 'u':                                   \
        CONVERT_RADIX_10(val, fmt_buf, p);      \
        break;                                  \
    IF_FMT_RADIX_x( default:                    \
        CONVERT_RADIX_16(val, fmt_buf, p, x);   \
        break; )                                \
    }

struct fmt_buf {
    const char *fmt_start; /* second character of formatter after '%' */
    size_t length;         /* length of formatted text (non-numeric)
                              or prefix (numeric) */
    char buf[24];          /* work buffer */
    char bufend[1];        /* buffer end marker and guard '0' */
#if (FMT_RADIX & FMT_RADIX_floats)
    int lenmod;
    int radixchar;
    int signchar;
    int alignchar;
    int width;
    int precision;
    char *p;
#endif
};

#define PUSHCHAR(ch) \
    ({  int __rc = push(userp, (ch)); \
        count += __rc >= 0;           \
        if (__rc <= 0) {              \
            goto done;                \
        } })

/* %d %i */
static inline const char * format_d(int val,
                                    struct fmt_buf *fmt_buf,
                                    int signchar)
{
    char *p = fmt_buf->bufend;
    CONVERT_RADIX_10_SIGN(val, fmt_buf, p, signchar, int);
    return p;
}

/* %o %u %x %X */
static inline const char * format_u(unsigned int val,
                                    struct fmt_buf *fmt_buf,
                                    int radixchar)
{
    char *p = fmt_buf->bufend;
    CONVERT_RADIX_NOSIGN(val, fmt_buf, p, radixchar);
    return p;
}

#if (FMT_LENMOD & FMT_LENMOD_l)
#ifndef format_ld
/* %ld %li */
static inline const char * format_ld(long val,
                                     struct fmt_buf *fmt_buf,
                                     int signchar)
{
    char *p = fmt_buf->bufend;
    CONVERT_RADIX_10_SIGN(val, fmt_buf, p, signchar, long);
    return p;
}
#endif /* format_ld */

#ifndef format_lu
/* %lo %lu %lx %lX */
static inline const char * format_lu(unsigned long val,
                                     struct fmt_buf *fmt_buf,
                                     int radixchar)
{
    char *p = fmt_buf->bufend;
    CONVERT_RADIX_NOSIGN(val, fmt_buf, p, radixchar);
    return p;
}
#endif /* format_lu */
#endif /* FMT_LENMOD_l */

#if (FMT_LENMOD & FMT_LENMOD_ll)
#ifndef format_lld
/* %lld %lli */
static inline const char * format_lld(long long val,
                                      struct fmt_buf *fmt_buf,
                                      int signchar)
{
    char *p = fmt_buf->bufend;
    CONVERT_RADIX_10_SIGN(val, fmt_buf, p, signchar, long long);
    return p;
}
#endif /* format_lld */

#ifndef format_llu
/* %llo %llu %llx %llX */
static inline const char * format_llu(unsigned long long val,
                                      struct fmt_buf *fmt_buf,
                                      int radixchar)
{
    char *p = fmt_buf->bufend;
    CONVERT_RADIX_NOSIGN(val, fmt_buf, p, radixchar);
    return p;
}
#endif /* format_llu */
#endif /* FMT_LENMOD_ll */

/* %c */
static inline const char * format_c(int c,
                                    struct fmt_buf *fmt_buf,
                                    int lenmod)
{
    if (lenmod != LENMOD_NONE) {
        return NULL; /* wchar_t support for now */
    }

    char *p = fmt_buf->bufend;
    fmt_buf->length = 1;
    *--p = (unsigned char)c;
    return p;
}

/* %s */
static inline const char * format_s(const void *str,
                                    struct fmt_buf *fmt_buf,
                                    int precision,
                                    int lenmod)
{
    if (lenmod != LENMOD_NONE) {
        return NULL; /* wchar_t support for now */
    }

    const char *s = str;
    size_t len;
    /* string length may be specified by precision instead of \0-
       terminated; however, don't go past a \0 if one is there */
    if (precision >= 0) {
        const char *nil = memchr(s, '\0', (size_t) precision);

        if (nil != NULL && (nil - s) < precision)
            len = nil - s;
        else
            len = precision;
    } 
    else
        len = strlen(s);

    fmt_buf->length = len;
    return s;
}

#if (FMT_RADIX & FMT_RADIX_n)
/* %n */
static inline bool format_n(void *np,
                            int count,
                            int lenmod)
{
    if (lenmod != LENMOD_NONE) {
        return false; /* int only for now */
    }

    *(int *)np = count;
    return true;
}
#endif /* FMT_RADIX_n */

#if (FMT_RADIX & FMT_RADIX_p)
/* %p %P */
static inline const char * format_p(const void *p,
                                    struct fmt_buf *fmt_buf,
                                    int radixchar,
                                    bool *numericp)
{
    if (p) {
        /* format as %#x or %#X */
        *numericp = true;
        radixchar -= 'P' - 'X';
        fmt_buf->length = 2;
        return LENMOD_INTCOMPAT_CALL(LENMOD_INTCOMPAT_SEL(uintptr_t, false),
                                     (uintptr_t)p, fmt_buf, radixchar, false);
    }
    else {
        /* format as %s */
        fmt_buf->length = 5;
        return "(nil)";
    }
}
#endif /* FMT_RADIX_p */

#if (FMT_RADIX & FMT_RADIX_floats)
/* find out how many uint32_t chunks need to be allocated, if any
 * if none are needed, finish the init for the number here */
static long parse_ieee754_double(double f,
                                 struct ap_int *ia,
                                 struct ap_int *fa,
                                 struct fmt_buf *fmt_buf)
{
    long rc = 0;

    union {
        double   f;
        uint64_t f64;
    } u = { .f = f };

    int e = ((int)(u.f64 >> 52) & 0x7ff) - 1023; /* -1023..1024 */
    uint64_t mantissa = u.f64 & 0x000fffffffffffffull;

    if (u.f64 >> 63) {
        fmt_buf->signchar = '-';
    }

    if (LIKELY(e >= -8 && e <= 63)) { /* -8 to +63 */
        /* integer, fraction and manipulations fit in uint64_t */
        mantissa |= 0x0010000000000000ull;
        ia->numchunks = 0;
        ia->shift     = 0;
        fa->numchunks = 0;

        if (e < 0) {            /* -8 to -1 - fraction */
            long fracbits = 52 - e;
            /* int - none */
            ia->len   = 0;
            ia->val   = 0;
            /* frac */
            fa->len   = fracbits - __builtin_ctzll(mantissa);
            fa->shift = fracbits;
            fa->val   = mantissa;
        }
        else if (e <= 51) {     /* 0 to +51 - integer|fraction */
            long fracbits = 52 - e;
            /* int */
            ia->len   = base10exp(e) + 2; /* go up + possibly 1 longer */
            ia->val   = mantissa >> fracbits;
            /* frac */
            fa->shift = fracbits;
            fa->val   = mantissa ^ (ia->val << fracbits);
            fa->len   = fa->val ? fracbits - __builtin_ctzll(mantissa) : 0;
        }
        else {                  /* +52 to +63 - integer */
            /* int */
            ia->len   = base10exp(e) + 2;
            ia->val   = mantissa << (e - 52);
            /* frac - none */
            fa->len   = 0;
            fa->shift = 0;
            fa->val   = 0;
        }
    }
    else if (e < 0) {         /* -1023 to -9 - fraction */
        /* int - none */
        ia->numchunks = 0;
        ia->len       = 0;
        ia->shift     = 0;
        ia->val       = 0;
        /* frac - left-justify on bit 31 of the chunk of the MSb */
        if (e >= -1022) {       /* normal */
            mantissa |= 0x0010000000000000ull;
        }
        else {                  /* subnormal (including zero) */
            e = -1022;
        }

        if (mantissa) {
            long fracbits = 52 - e;
            fa->len       = fracbits - __builtin_ctzll(mantissa);
            fa->shift     = 31 - ((51 - e) % 32);
            fa->val       = mantissa;
            fa->basechunk = (fa->shift + 52) / 32;
            fa->numchunks = (51 - e + fa->shift) / 32 + 1;
            rc = fa->numchunks;
        }
        else {                  /* zero */
            fa->numchunks = 0;
            fa->len       = 0;
            fa->shift     = 0;
            fa->val       = 0;
        }
    }
    else if (e <= 1023) {       /* +64 to +1023 - integer */
        /* int - right-justify on bit 0 of the first chunk */
        ia->val       = mantissa | 0x0010000000000000ull;
        ia->len       = base10exp(e) + 2;
        ia->shift     = (e - 52) % 32;
        ia->basechunk = e / 32;
        ia->numchunks = ia->basechunk + 1;
        rc = ia->numchunks;
        /* frac - none */
        fa->numchunks = 0;
        fa->len       = 0;
        fa->shift     = 0;
        fa->val       = 0;
    }
    else {                      /* +1024: INF, NAN */
        rc = -1 - !!mantissa;
    }

    return rc;
}

/* construct the arbitrary-precision value in the provided allocation */
static void init_ieee754_double_chunks(struct ap_int *a,
                                       uint32_t *a_chunks)
{
    long basechunk = a->basechunk;
    long shift = a->shift;
    uint64_t val = a->val;

    a->chunks = a_chunks;

    memset(a_chunks, 0, a->numchunks*sizeof (uint32_t));

    if (shift < 12) {
        a_chunks[basechunk - 1] = val << shift;
        a_chunks[basechunk - 0] = val >> (32 - shift);
    }
    else {
        a_chunks[basechunk - 2] = val << shift;
        a_chunks[basechunk - 1] = val >> (32 - shift);
        a_chunks[basechunk - 0] = val >> (64 - shift);
    }
}

/* format inf, nan strings */
static void format_inf_nan(struct fmt_buf *fmt_buf, long type)
{
    /* certain special values */
    static const char text[2][2][3] =
    {
        { { 'I', 'N', 'F' }, { 'i', 'n', 'f' } },
        { { 'N', 'A', 'N' }, { 'n', 'a', 'n' } },
    };

    char *p = fmt_buf->buf;
    fmt_buf->p = p;
    fmt_buf->length = 3;

    /* they also have a sign */
    if (fmt_buf->signchar) {
        *p++ = fmt_buf->signchar;
        fmt_buf->length++;
    }

    memcpy(p, &text[type][(fmt_buf->radixchar >> 5) & 0x1], 3);
}

/* %e %E %f %F %g %G */
static int format_double_radix(double f,
                               struct fmt_buf *fmt_buf,
                               vuprintf_push_cb push,
                               void *userp)
{
    struct ap_int ia, fa;
    long rc = parse_double(f, &ia, &fa, fmt_buf);

    if (UNLIKELY(rc < 0)) {
        format_inf_nan(fmt_buf, -rc - 1);
        return 0;
    }

    int count = 0;

    /* default precision is 6 for all formats */
    int prec_rem = fmt_buf->precision < 0 ? 6 : fmt_buf->precision;

    int exp = exp;
    int explen = 0;

    switch (fmt_buf->radixchar & 0x3)
    {
    case 3: /* %g, %G */
        fmt_buf->precision = prec_rem;
        if (prec_rem) {
            prec_rem--;
        }
        /* fallthrough */
    case 1: /* %e, %E */
        explen = 2;
        break;
    default:
        break;
    }

    if (rc > 0 && ia.numchunks > 0) {
        /* large integer required */
        init_double_chunks(&ia, alloca(rc*sizeof(*ia.chunks)));
        rc = 0;
    }

    const int bufoffs = 6; /* log rollover + round rollover + leading zeros (%g) */
    long f_prec = MIN(fa.len, prec_rem + 1);
    char buf[bufoffs + ia.len + f_prec + 1];
    char *p_last  = &buf[bufoffs + ia.len];
    char *p_dec   = p_last;
    char *p_first = format_double_int10(&ia, p_last);

    if (explen) {
        if (!ia.val && fa.len) {
            p_first = p_last = &buf[bufoffs];
            f_prec = -f_prec - 1; /* no lead zeros */
        }
        else { /* handles 0e+0 too */
            exp = ia.len - 1;

            if (exp) {
                prec_rem -= exp;

                if (prec_rem < 0) {
                    p_last += prec_rem + 1;
                    f_prec = 0;
                }
                else {
                    f_prec = MIN(fa.len, prec_rem + 1);
                }
            }
        }

        p_dec = p_first + 1;
    }

    if (f_prec) {
        if (rc > 0) {
            /* large integer required */
            init_double_chunks(&fa, alloca(rc*sizeof(*fa.chunks)));
        }

        p_last = format_double_frac10(&fa, p_last, f_prec);

        if (f_prec < 0) {
            f_prec = -f_prec - 1;
            exp = f_prec - fa.len;
        }

        prec_rem -= f_prec;
    }

    if (prec_rem < 0) {
        prec_rem = 0;
        p_last--;

        if (round_number_string10(p_last, p_last - p_first)) {
            /* carried left */
            p_first--;

            if (explen) {
                /* slide everything left by 1 */
                exp++;
                p_dec--;
                p_last--;
            }
        }
    }

    if (explen) {
        if ((fmt_buf->radixchar & 0x3) == 0x3) { /* g, G */
            /* 'g' is some weird crap */
            /* now that the final exponent is known and everything rounded,
               it is possible to decide whether to format similarly to
               'e' or 'f' */
            if (fmt_buf->precision > exp && exp >= -4) { /* P > X >= -4 */
                if (exp >= 0) {
                    /* integer digits will be in the buffer */
                    p_dec = p_first + exp + 1;
                }
                else {
                    /* we didn't keep leading zeros and need to regenerate
                       them; space was reserved just in case */
                    p_first = memset(p_dec + exp - 1, '0', -exp);
                    p_dec = p_first + 1;
                }

                /* suppress exponent */
                explen = 0;
            }

            if (!fmt_buf->length) {
                /* strip any trailing zeros from the fraction */
                while (p_last > p_dec && p_last[-1] == '0') {
                    p_last--;
                }

                /* suppress trailing precision fill */
                prec_rem = 0;
            }
        }

        if (explen) {
            /* build exponent string: 'e±dd' */
            char *p = fmt_buf->bufend;
            int signchar = '+';

            if (exp < 0) {
                signchar = '-';
                exp = -exp;
            }

            while (exp || explen < 4) {
                *--p = exp % 10 + '0';
                exp /= 10;
                explen++;
            }

            *--p = signchar;
            *--p = fmt_buf->radixchar & ~0x2;
        }
    }

    int width = fmt_buf->width;
    int point = p_last > p_dec || prec_rem || fmt_buf->length;
    int length = p_last - p_first + !!fmt_buf->signchar + point + explen;

    if (width) {
        if (width - length <= prec_rem) {
            width = 0;
        }
        else {
            width -= length + prec_rem;
        }
    }

    rc = -1;

    /* left padding */
    if (fmt_buf->alignchar > '0') {
        /* space-padded width -- before sign */
        while (width > 0) {
            PUSHCHAR(' ');
            width--;
        }
    }

    if (fmt_buf->signchar) {
        PUSHCHAR(fmt_buf->signchar);
    }

    if (fmt_buf->alignchar == '0') {
        /* zero-padded width -- after sign */
        while (width > 0) {
            PUSHCHAR('0');
            width--;
        }
    }

    /* integer part */
    while (p_first < p_dec) {
        PUSHCHAR(*p_first++);
    }

    /* decimal point */
    if (point) {
        PUSHCHAR('.');
    }

    /* fractional part */
    while (p_first < p_last) {
        PUSHCHAR(*p_first++);
    }

    /* precision 0-padding */
    while (prec_rem > 0) {
        PUSHCHAR('0');
        prec_rem--;
    }

    /* exponent */
    if (explen > 0) {
        char *p = fmt_buf->bufend;
        while (explen > 0) {
            PUSHCHAR(p[-explen--]);
        }
    }

    /* right padding */
    while (width > 0) {
        PUSHCHAR(' ');
        width--;
    }

    rc = 1;
done:
    fmt_buf->length = count;
    return rc;
}
#endif /* FMT_RADIX_floats */

/* parse fixed width or precision field */
static const char * parse_number_spec(const char *fmt,
                                      int ch,
                                      int *out)
{
    int i = ch - '0';

    while (1) {
        ch = *fmt - '0';

        if (ch < 0 || ch > 9 || i > INT_MAX / 10 ||
            (i == INT_MAX / 10 && ch > INT_MAX % 10)) {
            break;
        }

        i = i * 10 + ch;
        fmt++;
    }

    *out = i;
    return fmt;
}

int vuprintf(vuprintf_push_cb push, /* call 'push()' for each output letter */
             void *userp,
             const char *fmt,
             va_list ap)
{
    int count = 0;
    int ch;

    /* macrofied identifiers share a variable with another */
    unsigned int val_d;
    unsigned int val_u;
    #ifndef val_ld
    unsigned long val_ld;
    #endif
    #ifndef val_lu
    unsigned long val_lu;
    #endif
    #ifndef val_lld
    unsigned long long val_lld;
    #endif
    #ifndef val_llu
    unsigned long long val_llu;
    #endif

    struct fmt_buf fmt_buf;
    fmt_buf.bufend[0] = '0';

    while (1) {
        while (1) {
            if ((ch = *fmt++) == '\0') {
                goto done;
            }

            if (ch == '%' && (ch = *fmt++) != '%') {
                break;
            }

            PUSHCHAR(ch);
        }

        /* set to defaults */
        fmt_buf.fmt_start = fmt;

        int signchar = 0;
        unsigned int width = 0;
        int lenmod = LENMOD_NONE;
        size_t length = 0;
        size_t pfxlen = 0;
        bool numeric = false;
        int alignchar = '0' + 1;
        int precision = -1;
        const char *buf = NULL;

        /*** flags ***/
        while (1) {
            switch (ch)
            {
            case ' ':   /* <space> before non-negative value (signed conversion) */
            case '+':   /* '+' before non-negative value (signed conversion) */
                /* '+' overrides ' ' */
                if (ch > signchar) {
                    signchar = ch;
                }
                break;
            case '-':   /* left-justify in field */
            case '0':   /* zero-pad to fill field */
                /* '-' overrides '0' */
                if (ch < alignchar) {
                    alignchar = ch;
                }
                break;
            case '#':   /* number prefix (nonzero %o:'0' %x/%X:'0x') */
                /* indicate; formatter updates with actual length */
                pfxlen = 1;
                break;
            #if 0
            case '\'':  /* digit grouping (non-monetary) */
                break;
            #endif
            default:
                goto flags_done;
            }

            ch = *fmt++;
        }
   flags_done:

        /*** width ***/
        if (ch == '*') {
            /* variable width */
            int w = va_arg(ap, int);
            if (w < 0) {
                /* negative width is width with implied '-' */
                width = (unsigned int)-(w + 1) + 1;
                alignchar = '-';
            }
            else {
                width = w;
            }

            ch = *fmt++;
        }
        else if (ch >= '1' && ch <= '9') {
            /* fixed width */
            fmt = parse_number_spec(fmt, ch, &width);
            ch = *fmt++;
        }

        /*** precision ***/
        if (ch == '.') {
            ch = *fmt++;

            if (ch == '*') {
                /* variable precision; negative precision is ignored */
                precision = va_arg (ap, int);
                ch = *fmt++;
            }
            else if (ch >= '0' && ch <= '9') {
                /* fixed precision */
                fmt = parse_number_spec(fmt, ch, &precision);
                ch = *fmt++;
            }
        }

        /*** length modifier ***/
        #if FMT_LENMOD
        switch (ch)
        {
        #if (FMT_LENMOD & (FMT_LENMOD_h | FMT_LENMOD_hh))
        case 'h':
        #endif
        #if (FMT_LENMOD & FMT_LENMOD_j)
        case 'j':
        #endif
        #if (FMT_LENMOD & (FMT_LENMOD_l | FMT_LENMOD_ll))
        case 'l':
        #endif
        #if (FMT_LENMOD & FMT_LENMOD_t)
        case 't':
        #endif
        #if (FMT_LENMOD & FMT_LENMOD_z)
        case 'z':
        #endif
        #if (FMT_LENMOD & FMT_LENMOD_L)
        case 'L':
        #endif
            lenmod = ch;
            ch = *fmt++;
        #if (FMT_LENMOD & (FMT_LENMOD_hh | FMT_LENMOD_ll))
            /* doesn't matter if jj, tt or zz happen; they will be rejected
               by the radix handler */
            if (ch == lenmod) {
                lenmod = LENMOD2(lenmod, ch);
                ch = *fmt++;
            }
        #endif
        }
        #endif /* FMT_LENMOD */

        /*** radix ***/
        switch (ch)
        {
            /** non-numeric **/
        case 'c':
            buf = format_c(va_arg(ap, int),  &fmt_buf, lenmod);
            break;
        #if (FMT_RADIX & FMT_RADIX_n)
        case 'n':
            if (format_n(va_arg(ap, void *), count, lenmod)) {
                continue; /* no output */
            }
            break;
        #endif
        case 's':
            buf = format_s(va_arg(ap, const void *), &fmt_buf,
                           precision, lenmod);
            break;

            /** non-integer **/
        #if (FMT_RADIX & FMT_RADIX_p)
        case 'p':
        case 'P':
            buf = format_p(va_arg(ap, void *), &fmt_buf, ch,
                           &numeric);
            break;
        #endif

        #if (FMT_RADIX & FMT_RADIX_floats)
        /* any floats gets all of them (except with 'L' and %a, %A for now) */
        case 'e':
        case 'E':
        case 'f':
        case 'F':
        case 'g':
        case 'G':
            /* LENMOD_L isn't supported for now and will be rejected automatically */

            /* floating point has very different spec interpretations to other
               formats and requires special handling */
            fmt_buf.length    = pfxlen;
            fmt_buf.lenmod    = lenmod;
            fmt_buf.radixchar = ch;
            fmt_buf.signchar  = signchar;
            fmt_buf.alignchar = alignchar;
            fmt_buf.width     = width;
            fmt_buf.precision = precision;

            ch = format_double_radix(va_arg(ap, double), &fmt_buf, push, userp);
            if (ch) {
                count += fmt_buf.length;
                if (ch > 0) {
                    continue;
                }

                goto done;
            }

            buf = fmt_buf.p;
            break;
        #endif

            /** signed integer **/
        case 'd':
        case 'i':
            fmt_buf.length = pfxlen;

            switch (lenmod)
            {
            case LENMOD_NONE:
                val_d = va_arg(ap, signed int);
                goto branch_fmt_d;
            #if (FMT_LENMOD & FMT_LENMOD_h)
            case LENMOD_h:
                val_d = (signed short)va_arg(ap, SSHRT_INT_ARG);
                goto branch_fmt_d;
            #endif
            #if (FMT_LENMOD & FMT_LENMOD_hh)
            case LENMOD_hh:
                val_d = (signed char)va_arg(ap, SCHAR_INT_ARG);
                goto branch_fmt_d;
            #endif
            #if (FMT_LENMOD & FMT_LENMOD_j)
            case LENMOD_j:
                LENMOD_INTCOMPAT_BRANCH(LENMOD_INTCOMPAT_SEL(intmax_t, true),
                                        va_arg(ap, intmax_t), true);
            #endif
            #if (FMT_LENMOD & FMT_LENMOD_l)
            case LENMOD_l:
                val_ld = va_arg(ap, signed long);
                goto branch_fmt_ld;
            #endif
            #if (FMT_LENMOD & FMT_LENMOD_ll)
            case LENMOD_ll:
                val_lld = va_arg(ap, signed long long);
                goto branch_fmt_lld;
            #endif
            #if (FMT_LENMOD & FMT_LENMOD_t)
            case LENMOD_t:
                LENMOD_INTCOMPAT_BRANCH(LENMOD_INTCOMPAT_SEL(ptrdiff_t, true),
                                        va_arg(ap, ptrdiff_t), true);
            #endif
            #if (FMT_LENMOD & FMT_LENMOD_z)
            case LENMOD_z:
                LENMOD_INTCOMPAT_BRANCH(LENMOD_INTCOMPAT_SEL(ssize_t, true),
                                        va_arg(ap, ssize_t), true);
            #endif
            }

            /* macrofied labels share a formatter with another */
            if (0) {
            branch_fmt_d:
                buf = format_d(val_d, &fmt_buf, signchar);
            } else if (0) {
            #ifndef val_ld
            branch_fmt_ld:
                buf = format_ld(val_ld, &fmt_buf, signchar);
            #endif
            } else if (0) {
            #ifndef val_lld
            branch_fmt_lld:
                buf = format_lld(val_lld, &fmt_buf, signchar);
            #endif
            }

            numeric = true;
            break;

            /** unsigned integer **/
        #if (FMT_RADIX & FMT_RADIX_o)
        case 'o':
        #endif
        case 'u':
        #if (FMT_RADIX & FMT_RADIX_x)
        case 'x':
        case 'X':
        #endif
            fmt_buf.length = pfxlen;

            switch (lenmod)
            {
            case LENMOD_NONE:
                val_u = va_arg(ap, unsigned int);
                goto branch_fmt_u;
            #if (FMT_LENMOD & FMT_LENMOD_h)
            case LENMOD_h:
                val_u = (unsigned short)va_arg(ap, USHRT_INT_ARG);
                goto branch_fmt_u;
            #endif
            #if (FMT_LENMOD & FMT_LENMOD_hh)
            case LENMOD_hh:
                val_u = (unsigned char)va_arg(ap, UCHAR_INT_ARG);
                goto branch_fmt_u;
            #endif
            #if (FMT_LENMOD & FMT_LENMOD_j)
            case LENMOD_j:
                LENMOD_INTCOMPAT_BRANCH(LENMOD_INTCOMPAT_SEL(uintmax_t, false),
                                        va_arg(ap, uintmax_t), false);
            #endif
            #if (FMT_LENMOD & FMT_LENMOD_l)
            case LENMOD_l:
                val_lu = va_arg(ap, unsigned long);
                goto branch_fmt_lu;
            #endif
            #if (FMT_LENMOD & FMT_LENMOD_ll)
            case LENMOD_ll:
                val_llu = va_arg(ap, unsigned long long);
                goto branch_fmt_llu;
            #endif
            #if (FMT_LENMOD & (FMT_LENMOD_t | FMT_LENMOD_z))
            /* format "uptrdiff_t" as size_t (unless it becomes standard) */
            #if (FMT_LENMOD & FMT_LENMOD_t)
            case LENMOD_t:
            #endif
            #if (FMT_LENMOD & FMT_LENMOD_z)
            case LENMOD_z:
            #endif
                LENMOD_INTCOMPAT_BRANCH(LENMOD_INTCOMPAT_SEL(size_t, false),
                                        va_arg(ap, size_t), false);
            #endif
            }

            /* macrofied labels share a formatter with another */
            if (0) {
            branch_fmt_u:
                buf = format_u(val_u, &fmt_buf, ch);
            } else if (0) {
            #ifndef val_lu
            branch_fmt_lu:
                buf = format_lu(val_lu, &fmt_buf, ch);
            #endif
            } else if (0) {
            #ifndef val_llu
            branch_fmt_llu:
                buf = format_llu(val_llu, &fmt_buf, ch);
            #endif
            }

            numeric = true;
            break;
        }

        if (buf) {
            /** padding **/
            if (numeric) {
                /* numeric formats into fmt_buf.buf */
                pfxlen = fmt_buf.length;
                length = fmt_buf.bufend - buf;

                size_t size = pfxlen + length;

                if (precision >= 0) {
                    /* explicit precision */
                    precision -= (int)length;

                    if (precision > 0) {
                        size += precision;
                    }

                    width -= MIN(width, size);
                }
                else {
                    /* default precision */
                    if (!length) {
                        length = 1;
                        size++;
                    }

                    width -= MIN(width, size);

                    if (alignchar == '0') {
                        /* width zero-fill */
                        precision = width;
                        width = 0;
                    }
                }
            }
            else {
                /* non-numeric: supress prefix and precision; keep length and
                   width */
                pfxlen = 0;
                precision = 0;
                length = fmt_buf.length;
                width -= MIN(width, length);
            }
        }
        else {
            /* format not accepted; print it literally */
            buf = fmt_buf.fmt_start - 2;
            length = fmt - buf;
            width = 0;
            pfxlen = 0;
            precision = 0;
        }

        /** push all the stuff **/

        if (alignchar != '-') {
            /* left padding */
            while (width > 0) {
                PUSHCHAR(' ');
                width--;
            }
        }

        /* prefix */
        while (pfxlen > 0) {
            PUSHCHAR(buf[-pfxlen]);
            pfxlen--;
        }

        /* 0-padding */
        while (precision > 0) {
            PUSHCHAR('0');
            precision--;
        }

        /* field */
        while (length > 0) {
            PUSHCHAR(*buf++);
            length--;
        }

        /* right padding */
        while (width > 0) {
            PUSHCHAR(' ');
            width--;
        }
    }

done:
    return count;
}
