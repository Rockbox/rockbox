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
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include "stdint.h"
#include "gcc_extensions.h"
#include "vuprintf.h"

#define FMT_H_SUPPORT  0    /* signed/unsigned short formatting */
#define FMT_HH_SUPPORT 1    /* signed/unsigned char formatting */
#define FMT_L_SUPPORT  1    /* signed/unsigned long formatting */
#define FMT_LL_SUPPORT 0    /* signed/unsigned long long formatting */
#define FMT_Z_SUPPORT  1    /* size_t/ssize_t formatting */

#define FMT_N_SUPPORT  0    /* bytes output so far ('int *' argument) */
#define FMT_O_SUPPORT  0    /* octal radix formatting */
#define FMT_P_SUPPORT  1    /* pointer formatting */

/* avoid defining redundant functions if two or more types can use the same
 * something not getting a macro means it gets assigned its own value and
 * formatter */

/* l */
#if LONG_MIN == INT_MIN && LONG_MAX == INT_MAX
#define val_ld          val_d
#define format_ld       format_d
#define branch_fmt_ld   branch_fmt_d
#elif !FMT_L_SUPPORT    /* unique */
#define val_ld
#endif /* LONG_ */

#if ULONG_MAX == UINT_MAX
#define val_lu          val_u
#define format_lu       format_u
#define branch_fmt_lu   branch_fmt_u
#elif !FMT_L_SUPPORT    /* unique */
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
#elif !FMT_LL_SUPPORT   /* unique */
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
#elif !FMT_LL_SUPPORT   /* unique */
#define val_llu
#endif /* ULLONG_ */

/* some macros to have conditional work inside macros */
#if FMT_L_SUPPORT
#define IF_FMT_L(...) __VA_ARGS__
#else
#define IF_FMT_L(...)
#endif

#if FMT_LL_SUPPORT
#define IF_FMT_LL(...) __VA_ARGS__
#else
#define IF_FMT_LL(...)
#endif

#if FMT_O_SUPPORT
#define IF_FMT_O(...) __VA_ARGS__
#else
#define IF_FMT_O(...)
#endif

#define LENMOD_select(type, signd) \
    ({  int __lenmod;                                            \
        if (sizeof (type) == sizeof (unsigned int)) {            \
            __lenmod = LENMOD_NONE;                              \
        }                                                        \
        else if (sizeof (type) == sizeof (unsigned long)) {      \
            IF_FMT_L(__lenmod = LENMOD_l;)                       \
        }                                                        \
        else if (sizeof (type) == sizeof (unsigned long long)) { \
            IF_FMT_LL(__lenmod = LENMOD_ll;)                     \
        }                                                        \
        __lenmod; })

#define LENMOD_NONE '\0'

#if FMT_H_SUPPORT
#define LENMOD_h    'h'
#endif
#if FMT_HH_SUPPORT
#define LENMOD_hh   ('h'| ('h' << CHAR_BIT))
#endif
#if FMT_L_SUPPORT
#define LENMOD_l    'l'
#endif
#if FMT_LL_SUPPORT
#define LENMOD_ll   ('l' | ('l' << CHAR_BIT))
#endif
#if FMT_Z_SUPPORT
#define LENMOD_z    LENMOD_select(size_t, false)
#endif
#if FMT_P_SUPPORT
#define LENMOD_p    LENMOD_select(uintptr_t, false)
#endif

struct fmt_buf {
    const char *fmt_start; /* first character of formatter */
    int length;            /* length of formatted text (non-numeric)
                              or prefix (numeric) */
    char buf[24];          /* work buffer */
    char bufend[1];
};

#define LENMOD_INT_CALL(val, fmt_buf, lenmod, x, sign) \
    ({  const char *__buf;                          \
        switch (lenmod) {                           \
        default:                                    \
            __buf = (sign) ?                        \
                format_d((val), (fmt_buf), (x)) :   \
                format_u((val), (fmt_buf), (x));    \
            break;                                  \
        IF_FMT_L(                                   \
        case LENMOD_l:                              \
            __buf = (sign) ?                        \
                format_ld((val), (fmt_buf), (x)) :  \
                format_lu((val), (fmt_buf), (x));   \
            break;                                  \
        )                                           \
        IF_FMT_LL(                                  \
        case LENMOD_ll:                             \
            __buf = (sign) ?                        \
                format_lld((val), (fmt_buf), (x)) : \
                format_llu((val), (fmt_buf), (x));  \
            break;                                  \
        )                                           \
        }                                           \
        __buf; })

#define CONVERT_RADIX_10_SIGN(val, fmt_buf, p, signchar, type) \
    do {                                        \
        if (val) {                              \
            unsigned type v;                    \
                                                \
            if (val < 0) {                      \
                v = -val;                       \
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
    IF_FMT_O( case 'o':                         \
        CONVERT_RADIX_8(val, fmt_buf, p);       \
        break; )                                \
    case 'u':                                   \
        CONVERT_RADIX_10(val, fmt_buf, p);      \
        break;                                  \
    default:                                    \
        CONVERT_RADIX_16(val, fmt_buf, p, x);   \
        break;                                  \
    }

/* %d %i */
static const char * format_d(int val,
                             struct fmt_buf *fmt_buf,
                             int signchar)
{
    char *p = fmt_buf->bufend;
    CONVERT_RADIX_10_SIGN(val, fmt_buf, p, signchar, int);
    return p;
}

/* %o %u %x %X */
static const char * format_u(unsigned int val,
                             struct fmt_buf *fmt_buf,
                             int radixchar)
{
    char *p = fmt_buf->bufend;
    CONVERT_RADIX_NOSIGN(val, fmt_buf, p, radixchar);
    return p;
}

#if FMT_L_SUPPORT
#ifndef format_ld
/* %ld %li */
static const char * format_ld(long val,
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
static const char * format_lu(unsigned long val,
                              struct fmt_buf *fmt_buf,
                              int radixchar)
{
    char *p = fmt_buf->bufend;
    CONVERT_RADIX_NOSIGN(val, fmt_buf, p, radixchar);
    return p;
}
#endif /* format_lu */
#endif /* FMT_L_SUPPORT */

#if FMT_LL_SUPPORT
#ifndef format_lld
/* %lld %lli */
static const char * format_lld(long long val,
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
static const char * format_llu(unsigned long long val,
                               struct fmt_buf *fmt_buf,
                               int radixchar)
{
    char *p = fmt_buf->bufend;
    CONVERT_RADIX_NOSIGN(val, fmt_buf, p, radixchar);
    return p;
}
#endif /* format_llu */
#endif /* FMT_LL_SUPPORT */

/* %c */
static const char * format_c(unsigned char c,
                             struct fmt_buf *fmt_buf,
                             int lenmod)
{
    if (lenmod != LENMOD_NONE) {
        return NULL; /* no wchar_t support for now */
    }

    fmt_buf->length = 1;

    char *p = fmt_buf->bufend;
    *--p = c;
    return p;
}

/* %s */
static const char * format_s(const char *str,
                             struct fmt_buf *fmt_buf,
                             int lenmod,
                             int precision)
{
    if (lenmod != LENMOD_NONE) {
        return NULL; /* no wchar_t support for now */
    }

    /* string length may be specified by precision instead of NULL-
       terminated however, don't go past a \0 if one is there */
    size_t len = precision >= 0 ? (size_t)precision : (size_t)-1;
    const char *nil = memchr(str, 0x00, len);
    if (nil) {
        len = nil - str;
    }

    fmt_buf->length = len;
    return str;
}

#if FMT_N_SUPPORT
/* %n */
static bool format_n(void *np,
                     int lenmod,
                     int count)
{
    if (lenmod != LENMOD_NONE) {
        return false; /* int only! */
    }

    *(int *)np = count;
    return true;
}
#endif /* FMT_N_SUPPORT */

#if FMT_P_SUPPORT
/* %p %P */
static const char * format_p(const void *p,
                             struct fmt_buf *fmt_buf,
                             int radixchar,
                             bool *numericp)
{
    if (p) {
        /* format as %#x or %#X */
        *numericp = true;
        radixchar -= 'P' - 'X';
        fmt_buf->length = 2;
        return LENMOD_INT_CALL((uintptr_t)p, fmt_buf, LENMOD_p,
                               radixchar, false);
    }
    else {
        /* format as %s */
        fmt_buf->length = 5;
        return "(nil)";
    }
}
#endif /* FMT_P_SUPPORT */

/* parse fixed width or precision field */
static const char * parse_number_spec(const char *fmt,
                                      int ch,
                                      int *out)
{
    int i = 0;

    while (1) {
        i = i * 10 + ch - '0';

        ch = *fmt;

        if (ch < '0' || ch > '9') {
            break;
        }

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
#endif /* val_id */
#ifndef val_lu
    unsigned long val_lu;
#endif /* val_lu */
#ifndef val_lld
    unsigned long long val_lld;
#endif /* val_lld */
#ifndef val_llu
    unsigned long long val_llu;
#endif /* val_llu */

    struct fmt_buf fmt_buf;
    fmt_buf.bufend[0] = '0';

    while ((ch = *fmt++) != '\0') {
        if (LIKELY(ch != '%' || (ch = *fmt++) == '%')) {
            int rc = push(userp, ch);
            count += rc >= 0;
            if (rc <= 0) {
                goto done;
            }

            continue;
        }

        /* set to defaults */
        fmt_buf.fmt_start = fmt;

        int signchar = 0;
        int width = 0;
        int lenmod = LENMOD_NONE;
        int length = 0;
        int pfxlen = 0;
        bool numeric = false;
        int alignchar = '0' + 1;
        int precision = -1;
        const char *buf = NULL;

        /*** flags ***/
        while (1) {
            switch (ch)
            {
            case ' ':
            case '+':
                /* '+' overrides ' ' */
                if (ch > signchar) {
                    signchar = ch;
                }
                break;
            case '-':
            case '0':
                /* '-' overrides '0' */
                if (ch < alignchar) {
                    alignchar = ch;
                }
                break;
            case '#':
                /* indicate; formatter updates with actual length */
                pfxlen = 1;
                break;
            default:
                goto flags_done;
            }

            ch = *fmt++;
        }
   flags_done:

        /*** width ***/
        switch (ch)
        {
        case '*':
            /* variable width */
            width = va_arg(ap, int);
            if (width < 0) {
                /* negative width is width with implied '-' */
                width = -width;
                alignchar = '-';
            }

            ch = *fmt++;
            break;
        case '1' ... '9':
            /* fixed width */
            fmt = parse_number_spec(fmt, ch, &width);
            ch = *fmt++;
            break;
        }

        /*** precision ***/
        if (ch == '.') {
            ch = *fmt++;

            switch (ch)
            {
            case '*':
                /* variable precision; negative precision is ignored */
                precision = va_arg (ap, int);
                ch = *fmt++;
                break;
            case '0' ... '9':
                /* fixed precision */
                fmt = parse_number_spec(fmt, ch, &precision);
                ch = *fmt++;
                break;
            }
        }

        /*** length modifier ***/
        switch (ch)
        {
        #if FMT_H_SUPPORT || FMT_HH_SUPPORT
        case 'h':
            ch = *fmt++;
        #if FMT_HH_SUPPORT
            if (ch == 'h') {
                ch = *fmt++;
                lenmod = LENMOD_hh;
                break;
            }
        #endif
        #if FMT_H_SUPPORT
            lenmod = LENMOD_h;
        #else
            ch = 0;
        #endif
            break;
        #endif /*  FMT_H_SUPPORT || FMT_HH_SUPPORT */
        #if FMT_L_SUPPORT || FMT_LL_SUPPORT
        case 'l':
            ch = *fmt++;
        #if FMT_LL_SUPPORT
            if (ch == 'l') {
                ch = *fmt++;
                lenmod = LENMOD_ll;
                break;
            }
        #endif
        #if FMT_L_SUPPORT
            lenmod = LENMOD_l;
        #else
            ch = 0;
        #endif
            break;
        #endif /* FMT_L_SUPPORT || FMT_LL_SUPPORT */
        #if FMT_Z_SUPPORT
        case 'z':
            lenmod = LENMOD_z;
            ch = *fmt++;
            break;
        #endif /* FMT_Z_SUPPORT */
        }

        /*** radix ***/
        switch (ch)
        {
            /** non-numeric **/
        case 'c':
            buf = format_c(va_arg(ap, int),  &fmt_buf, lenmod);
            break;
        case 's':
            buf = format_s(va_arg(ap, const char *), &fmt_buf,
                           lenmod, precision);
            break;
        #if FMT_N_SUPPORT
        case 'n':
            if (format_n(va_arg(ap, void *), lenmod, count)) {
                continue;
            }

            /* format literally */
            break;
        #endif /* FMT_N_SUPPORT */

            /** non-integer **/
        #if FMT_P_SUPPORT
        case 'p':
        case 'P':
            buf = format_p(va_arg(ap, void *), &fmt_buf, ch, &numeric);
            break;
        #endif /* FMT_P_SUPPORT */

            /** signed integer **/
        case 'd':
        case 'i':
            numeric = true;
            fmt_buf.length = pfxlen;

            switch (lenmod)
            {
            default:
                val_d = va_arg(ap, signed int);
                goto branch_fmt_d;
            #if FMT_H_SUPPORT
            case LENMOD_h:
                val_d = (signed short)va_arg(ap, int);
                goto branch_fmt_d;
            #endif /* FMT_H_SUPPORT */
            #if FMT_HH_SUPPORT
            case LENMOD_hh:
                val_d = (signed char)va_arg(ap, int);
                goto branch_fmt_d;
            #endif /* FMT_HH_SUPPORT */
            #if FMT_L_SUPPORT
            case LENMOD_l:
                val_ld = va_arg(ap, signed long);
                goto branch_fmt_ld;
            #endif /* FMT_L_SUPPORT */
            #if FMT_LL_SUPPORT
            case LENMOD_ll:
                val_lld = va_arg(ap, signed long long);
                goto branch_fmt_lld;
            #endif /* FMT_LL_SUPPORT */
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

            break;

            /** unsigned integer **/
        #if FMT_O_SUPPORT
        case 'o':
        #endif
        case 'u':
        case 'x':
        case 'X':
            numeric = true;
            fmt_buf.length = pfxlen;

            switch (lenmod)
            {
            default:
                val_u = va_arg(ap, unsigned int);
                goto branch_fmt_u;
            #if FMT_H_SUPPORT
            case LENMOD_h:
                val_u = (unsigned short)va_arg(ap, int);
                goto branch_fmt_u;
            #endif /* FMT_H_SUPPORT */
            #if FMT_HH_SUPPORT
            case LENMOD_hh:
                val_u = (unsigned char)va_arg(ap, int);
                goto branch_fmt_u;
            #endif /* FMT_HH_SUPPORT */
            #if FMT_L_SUPPORT
            case LENMOD_l:
                val_lu = va_arg(ap, unsigned long);
                goto branch_fmt_lu;
            #endif /* FMT_L_SUPPORT */
            #if FMT_LL_SUPPORT
            case LENMOD_ll:
                val_llu = va_arg(ap, unsigned long long);
                goto branch_fmt_llu;
            #endif /* FMT_LL_SUPPORT */
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

            break;
        }

        if (!buf) {
            /* format not accepted; print it literally */
            buf = fmt_buf.fmt_start - 2;
            fmt_buf.length = fmt - buf;
            width = 0;
            numeric = false;
        }

        /* precision padding */
        if (numeric) {
            /* numeric formats into fmt_buf.buf */
            pfxlen = fmt_buf.length;
            length = fmt_buf.bufend - buf;

            if (precision >= 0) {
                /* explicit precision */
                precision -= length;
            }
            else {
                if (!length) {
                    length = 1;
                }

                if (alignchar == '0') {
                    /* width zero-fill */
                    precision = width - length - pfxlen;
                }
            }
        }
        else {
            /* non-numeric: supress prefix and precision; keep length and
               width */
            pfxlen = precision = 0;
            length = fmt_buf.length;
        }

        /* width padding */
        if (width > 0) {
            /* pad with spaces whatever remains after prefix, length and
               precision are accounted for */
            width -= length + pfxlen;

            if (precision > 0) {
                /* precision will take some */
                width -= precision;
            }

            if (width < 0) {
                /* formatting wider than width */
                width = 0;
            }
            else if (alignchar == '-') {
                /* left-align; pad right side */
                width = -width;
            }
        }

        /* push all the stuff */
        while (1) {
            if (width > 0) {            /* left padding */
                ch = ' ';
                width--;
            }
            else if (pfxlen > 0) {      /* prefix */
                ch = buf[-pfxlen];
                pfxlen--;
            }
            else if (precision > 0) {   /* 0-padding */
                ch = '0';
                precision--;
            }
            else if (length > 0) {      /* field */
                ch = *buf++;
                length--;
            }
            else if (width < 0) {       /* right padding */
                ch = ' ';
                width++;
            }
            else {
                break;
            }

            int rc = push(userp, ch);
            count += rc >= 0;
            if (rc <= 0) {
                goto done;
            }
        }
    }

done:
    return count;
}
