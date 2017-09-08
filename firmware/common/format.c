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
#include "config.h"
#include "system.h"
#include "stdint.h"
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include "format.h"

#define FMT_HH_SUPPORT 0    /* signed/unsigned char formatting */
#define FMT_H_SUPPORT  0    /* signed/unsigned short formatting */
#define FMT_L_SUPPORT  1    /* signed/unsigned long formatting */
#define FMT_LL_SUPPORT 0    /* signed/unsigned long long formatting */
#define FMT_Z_SUPPORT  1    /* size_t/ssize_t formatting */

#define FMT_O_SUPPORT  0    /* octal radix formatting */
#define FMT_P_SUPPORT  1    /* pointer formatting */

/* chars and shorts are formatted as ints */

/* avoid defining redundant functions if two or more types can use the same */
#define LEN_d       0
#define LEN_u       0

/* l */
#if LONG_MIN == INT_MIN && LONG_MAX == INT_MAX
#define LEN_ld      LEN_d
#define format_ld   format_d
#else /* unique */
#define LEN_ld      (LEN_d+1)
#endif /* LONG_ */

#if ULONG_MAX == UINT_MAX
#define LEN_lu      LEN_u
#define format_lu   format_u
#else /* unique */
#define LEN_lu      (LEN_u+1)
#endif /* ULONG_ */

/* ll */
#if LLONG_MIN == INT_MIN && LLONG_MAX == INT_MAX
#define LEN_lld     LEN_d
#define format_lld  format_d
#elif LLONG_MIN == LONG_MIN && LLONG_MAX == LONG_MAX
#define LEN_lld     LEN_ld
#define format_lld  format_ld
#else /* unique */
#define LEN_lld     (LEN_ld+1)
#endif /* LLONG_ */

#if ULLONG_MAX == UINT_MAX
#define LEN_llu     LEN_u
#define format_llu  format_u
#elif ULLONG_MAX == ULONG_MAX
#define LEN_llu     LEN_lu
#define format_llu  format_lu
#else   /* unique */
#define LEN_llu     (LEN_lu+1)
#endif /* ULLONG_ */

#define FMT_LENMOD_DEFAULT
enum fmt_lenmod {
    LENMOD_NONE,    /* default */
#if FMT_HH_SUPPORT
    LENMOD_hh,      /* char */
#endif
#if FMT_H_SUPPORT
    LENMOD_h,       /* short */
#endif
#if FMT_L_SUPPORT
    LENMOD_l,       /* long */
#endif
#if FMT_LL_SUPPORT
    LENMOD_ll,      /* long long */
#endif
    LENMOD_SGN,
};

#define LENMOD_select(size) \
    ({  int __lenmod;                                     \
        if ((size) == sizeof (unsigned int)) {            \
            __lenmod = LENMOD_NONE;                       \
        }                                                 \
        else if ((size) == sizeof (unsigned long)) {      \
            IF_FMT_L(__lenmod = LENMOD_l;)                \
        }                                                 \
        else if ((size) == sizeof (unsigned long long)) { \
            IF_FMT_LL(__lenmod = LENMOD_ll;)              \
        }                                                 \
        __lenmod; })

#if FMT_Z_SUPPORT
#define LENMOD_z LENMOD_select(sizeof (size_t))
#endif

#if FMT_P_SUPPORT
#define LENMOD_p LENMOD_select(sizeof (uintptr_t))
#endif

enum fmt_radix {
    RADIX_NONE,
#if FMT_O_SUPPORT
    RADIX_8,
#endif
    RADIX_10,
    RADIX_16,
};

struct fmt_spec {
    bool left;       /* field data aligns to left side */
    bool zero;       /* pad field with leading zeros (numbers) */
    int  signchar;   /* ' ' or '+' prefix for positive signed conversion */
    int  pfxlen;     /* length of prefix applied to field */
    int  width;      /* field width */
    int  precision;  /* precision specifier */
    int  radix;
    int  len;        /* length of field data */
    int  xchar;
    char buf[32];    /* work buffer */
};

#define FMT_SPEC_BUFEND(spec) \
    (&(spec)->buf[sizeof ((spec)->buf)])

#define FMT_SPEC_LEN(spec, p) \
    ((int)(FMT_SPEC_BUFEND(spec) - (const char *)(p)))

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

#define LENMOD_CALL(lenmod, spec, val) \
    ({  const char *__buf;                   \
        switch (lenmod) {                    \
        case LENMOD_NONE:                    \
            __buf = format_u((spec), (val)); \
            break;                           \
        case LENMOD_NONE + LENMOD_SGN:       \
            __buf = format_d((spec), (val)); \
            break;                           \
        IF_FMT_L(                            \
        case LENMOD_l:                       \
            __buf = format_lu(spec, (val));  \
            break;                           \
        case LENMOD_l + LENMOD_SGN:          \
            __buf = format_ld(spec, (val));  \
            break;                           \
        )                                    \
        IF_FMT_LL(                           \
        case LENMOD_ll:                      \
            __buf = format_llu(spec, (val)); \
            break;                           \
        case LENMOD_ll + LENMOD_SGN:         \
            __buf = format_lld(spec, (val)); \
            break;                           \
        )                                    \
        }                                    \
        __buf; })

#define CONVERT_RADIX_10_sgn(spec, p, val, type) \
    do {                                  \
        if (val || spec->precision) {     \
            unsigned type __v = ABS(val); \
            do {                          \
                *--p = (__v % 10) + '0';  \
                __v /= 10;                \
            } while (__v);                \
        }                                 \
        if (val < 0) {                    \
            p[-1] = '-';                  \
        }                                 \
        else if (spec->signchar) {        \
            p[-1] = spec->signchar;       \
        }                                 \
        else {                            \
            spec->pfxlen = 0;             \
            break;                        \
        }                                 \
        spec->pfxlen = 1;                 \
    } while (0)

#if FMT_O_SUPPORT
#define CONVERT_RADIX_8(spec, p, val) \
    do {                                  \
        if (val || spec->precision) {     \
            typeof (val) __v = (val);     \
            do {                          \
                *--p = (__v % 010) + '0'; \
                __v /= 010;               \
            } while (__v);                \
        }                                 \
        if (spec->pfxlen) {               \
            p[-1] = '0';                  \
        }                                 \
    } while (0)
#endif /* FMT_O_SUPPORT */

#define CONVERT_RADIX_10(spec, p, val) \
    do {                                 \
        if (val || spec->precision) {    \
            typeof (val) __v = (val);    \
            do {                         \
                *--p = (__v % 10) + '0'; \
                __v /= 10;               \
            } while (__v);               \
        }                                \
        spec->pfxlen = 0;                \
    } while (0)

#define CONVERT_RADIX_16(spec, p, val) \
    do {                                           \
        if (val || spec->precision) {              \
            const int __x = spec->xchar;           \
            const int __h = __x - 'X' - 0xA + 'A'; \
            typeof (val) __v = (val);              \
            do {                                   \
                unsigned int __d = __v % 0x10;     \
                *--p = __d < 0xA ?                 \
                    __d + '0' : __d + __h;         \
                __v /= 0x10;                       \
            } while (__v);                         \
            if (val && spec->pfxlen) {             \
                spec->pfxlen = 2;                  \
                p[-1] = __x;                       \
                p[-2] = '0';                       \
                break;                             \
            }                                      \
        }                                          \
        spec->pfxlen = 0;                          \
    } while (0)

static const char * parse_number_spec(const char *fmt, int ch, int *out)
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

static const char * format_d(struct fmt_spec *spec, int val)
{
    char *buf = FMT_SPEC_BUFEND(spec);
    CONVERT_RADIX_10_sgn(spec, buf, val, int);
    return buf;
}

static const char * format_u(struct fmt_spec *spec, unsigned int val)
{
    char *buf = FMT_SPEC_BUFEND(spec);

    switch (spec->radix)
    {
#if FMT_O_SUPPORT
    case RADIX_8:
        CONVERT_RADIX_8(spec, buf, val);
        break;
#endif /* FMT_O_SUPPORT */
    case RADIX_10:
        CONVERT_RADIX_10(spec, buf, val);
        break;
    case RADIX_16:
        CONVERT_RADIX_16(spec, buf, val);
        break;
    }

    return buf;
}

#ifndef format_ld
static const char * format_ld(struct fmt_spec *spec, long val)
{
    char *buf = FMT_SPEC_BUFEND(spec);
    CONVERT_RADIX_10_sgn(spec, buf, val, long);
    return buf;
}
#endif /* format_ld */

#ifndef format_lu
static const char * format_lu(struct fmt_spec *spec, unsigned long val)
{
    char *buf = FMT_SPEC_BUFEND(spec);

    switch (spec->radix)
    {
#if FMT_O_SUPPORT
    case RADIX_8:
        CONVERT_RADIX_8(spec, buf, val);
        break;
#endif /* FMT_O_SUPPORT */
    case RADIX_10:
        CONVERT_RADIX_10(spec, buf, val);
        break;
    case RADIX_16:
        CONVERT_RADIX_16(spec, buf, val);
        break;
    }

    return buf;
}
#endif /* format_lu */

#if FMT_LL_SUPPORT
#ifndef format_lld
static const char * format_lld(struct fmt_spec *spec, long long val)
{
    char *buf = FMT_SPEC_BUFEND(spec);
    CONVERT_RADIX_10_sgn(spec, buf, val, long long):
    return buf;
}
#endif /* format_lld */

#ifndef format_llu
static const char * format_llu(struct fmt_spec *spec, unsigned long long val)
{
    char *buf = FMT_SPEC_BUFEND(spec);

    switch (spec->radix)
    {
#if FMT_O_SUPPORT
    case RADIX_8:
        CONVERT_RADIX_8(spec, buf, val);
        break;
#endif /* FMT_O_SUPPORT */
    case RADIX_10:
        CONVERT_RADIX_10(spec, buf, val);
        break;
    case RADIX_16:
        CONVERT_RADIX_16(spec, buf, val);
        break;
    }

    return buf;
}
#endif /* format_llu */
#endif /* FMT_LL_SUPPORT */

static const char * format_c(struct fmt_spec *spec, unsigned char c)
{
    char *buf = FMT_SPEC_BUFEND(spec);
    *--buf = c;
    spec->len = 1;
    return buf;
}

static const char * format_s(struct fmt_spec *spec, const char *str)
{
    spec->len = strlen(str);

    if (spec->precision >= 0 && spec->precision < spec->len) {
        spec->len = spec->precision;
    }

    return str;
}

#if FMT_P_SUPPORT
static const char * format_p(struct fmt_spec *spec, const void *p, int ch)
{
    if (p) {
        spec->pfxlen = 1;
        spec->xchar = ch - 'P' + 'X';
        spec->radix = RADIX_16;
        p = LENMOD_CALL(LENMOD_p, spec, (uintptr_t)p);
        spec->len = FMT_SPEC_LEN(spec, p);
        return p;
    }
    else {
        spec->radix = RADIX_NONE;
        spec->len = 5;
        return "(nil)";
    }
}
#endif /* FMT_P_SUPPORT */

void vuprintf(
    format_push_cb_t push, /* call 'push()' for each output letter */
    void *userp,
    const char *fmt,
    va_list ap)
{
    bool ok = true;
    struct fmt_spec spec;
    const char *fmt_start;
    const char *buf;
    int ch;

    while (ok && (ch = *fmt++) != '\0') {
        if (LIKELY(ch != '%')) {
            ok = push(userp, ch);
            continue;
        }

        /* save start point in case it's bad */
        fmt_start = fmt;

        ch = *fmt++;
        if (ch == '%') {
            fmt_start = fmt;
            goto no_fmt;
        }

        /* set to defaults */
        spec.left      = false;
        spec.zero      = false;
        spec.signchar  = '\0';
        spec.pfxlen    = 0;
        spec.width     = 0;
        spec.precision = -1;

        /*** flags ***/
        while (1) {
            switch (ch)
            {
            case ' ': case '+':
                /* '+' overrides ' ' */
                if (ch > spec.signchar) {
                    spec.signchar = ch;
                }
                break;
            case '#':
                /* indicate; formatter updates with actual length */
                spec.pfxlen = 1;
                break;
            case '-':
                /* '-' overrides '0' */
                spec.left = true;
                spec.zero = false;
                break;
            case '0':
                spec.zero = !spec.left;
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
            spec.width = va_arg(ap, int);
            if (spec.width < 0) {
                /* negative width is width with implied '-' */
                spec.width = -spec.width;
                spec.left = true;
            }

            ch = *fmt++;
            break;
        case '1' ... '9':
            /* fixed width */
            fmt = parse_number_spec(fmt, ch, &spec.width);
            ch = *fmt++;
            break;
        }

        /* read precision */
        if (ch == '.') {
            ch = *fmt++;

            switch (ch)
            {
            case '*':
                /* variable precision; negative precision is ignored */
                spec.precision = va_arg (ap, int);
                ch = *fmt++;
                break;
            case '0' ... '9':
                /* fixed precision */
                fmt = parse_number_spec(fmt, ch, &spec.precision);
                ch = *fmt++;
                break;
            }
        }

        /*** length modifier ***/
        unsigned int lenmod = LENMOD_NONE;

        switch (ch)
        {
    #if FMT_H_SUPPORT || FMT_HH_SUPPORT
        case 'h':
        #if FMT_HH_SUPPORT
            if (*fmt == 'h') {
                fmt++;
                lenmod = LENMOD_hh;
                break;
            }
        #endif
        #if FMT_H_SUPPORT
            lenmod = LENMOD_h;
            break;
        #endif
            goto no_fmt;
    #endif /*  FMT_H_SUPPORT || FMT_HH_SUPPORT */
    #if FMT_L_SUPPORT || FMT_LL_SUPPORT
        case 'l':
        #if FMT_LL_SUPPORT
            if (*fmt == 'l') {
                fmt++;
                lenmod = LENMOD_ll;
                break;
            }
        #endif /* FMT_LL_SUPPORT */
        #if FMT_L_SUPPORT
            lenmod = LENMOD_l;
            break;
        #endif
            goto no_fmt;
    #endif /* FMT_L_SUPPORT || FMT_LL_SUPPORT */
    #if FMT_Z_SUPPORT
        case 'z':
            lenmod = LENMOD_z;
            break;
    #endif /* FMT_Z_SUPPORT */
        }

        if (lenmod != LENMOD_NONE) {
            ch = *fmt++;
        }

        /*** radix ***/
        switch (ch)
        {
            /** non-numeric **/
        default:
            spec.radix = RADIX_NONE;

            switch (ch)
            {
            case 'c':
                buf = format_c(&spec, va_arg(ap, int));
                break;
            case 's':
                if (lenmod == LENMOD_NONE) {
                    buf = format_s(&spec, va_arg(ap, const char *));
                    break;
                }
            default:
                goto no_fmt;
            }

            goto push_fmt;

            /** non-integer **/
        #if FMT_P_SUPPORT
        case 'p': case 'P':
            buf = format_p(&spec, va_arg(ap, void *), ch);
            goto push_fmt;
        #endif /* FMT_P_SUPPORT */

            /** integer **/
        case 'd': case 'i':
            spec.radix = RADIX_10;
            lenmod += LENMOD_SGN;
            break;
        #if FMT_O_SUPPORT
        case 'o':
            spec.radix = RADIX_8;
            break;
        #endif /* FMT_O_SUPPORT */
        case 'u':
            spec.radix = RADIX_10;
            break;
        case 'x': case 'X':
            spec.radix = RADIX_16;
            spec.xchar = ch;
            break;
        }

        switch (lenmod)
        {
        default:
        case LENMOD_NONE:
            buf = format_u(&spec, va_arg(ap, unsigned int));
            break;
        case LENMOD_NONE + LENMOD_SGN:
            buf = format_d(&spec, va_arg(ap, int));
            break;
    #if FMT_HH_SUPPORT
        case LENMOD_hh:
            buf = format_u(&spec, (unsigned char)va_arg(ap, int));
            break;
        case LENMOD_hh + LENMOD_SGN:
            buf = format_d(&spec, (signed char)va_arg(ap, int));
            break;
    #endif /* FMT_HH_SUPPORT */
    #if FMT_H_SUPPORT
        case LENMOD_h:
            buf = format_u(&spec, (unsigned short)va_arg(ap, int));
            break;
        case LENMOD_h + LENMOD_SGN:
            buf = format_d(&spec, (signed short)va_arg(ap, int));
            break;
    #endif /* FMT_H_SUPPORT */
    #if FMT_L_SUPPORT
        case LENMOD_l:
            buf = format_lu(&spec, va_arg(ap, unsigned long));
            break;
        case LENMOD_l + LENMOD_SGN:
            buf = format_ld(&spec, va_arg(ap, long));
            break;
    #endif /* FMT_L_SUPPORT */
    #if FMT_LL_SUPPORT
        case LENMOD_ll:
            buf = format_llu(&spec, va_arg(ap, unsigned long long));
            break;
        case LENMOD_ll + LENMOD_SGN:
            buf = format_lld(&spec, va_arg(ap, long long));
            break;
    #endif /* FMT_LL_SUPPORT */
        }

        spec.len = FMT_SPEC_LEN(&spec, buf);

    push_fmt:
        /* calculate width and precision padding */
        if (spec.radix == RADIX_NONE) {
            spec.pfxlen = 0;
            spec.precision = 0;
        }
        else if (spec.precision >= 0) {
            spec.precision -= spec.len;
        }
        else if (spec.zero) {
            spec.precision = spec.width - spec.len - spec.pfxlen;
        }

        if (spec.width > 0) {
            spec.width -= spec.len + spec.pfxlen + MAX(spec.precision, 0);
        }

        /* start padding on left or do it afterwards if left-aligned */
        if (spec.left) {
            goto push_left_align;
        }

        /* push all the stuff */
        while (1) {
            /* push padding spaces */
            while (ok && spec.width > 0) {
                ok = push(userp, ' ');
                spec.width--;
            }

            if (spec.left) {
                break;
            }

        push_left_align:
            /* push prefix */
            while (ok && spec.pfxlen > 0) {
                ok = push(userp, buf[-spec.pfxlen]);
                spec.pfxlen--;
            }

            /* push zero padding */
            while (ok && spec.precision > 0) {
                ok = push(userp, '0');
                spec.precision--;
            }

            /* push field */
            while (ok && spec.len > 0) {
                ok = push(userp, *buf++);
                spec.len--;
            }

            if (!spec.left) {
                break;
            }
        }

        if (0) {
            /* invalid formatter or '%' */
        no_fmt:
            ok = push(userp, '%');
            fmt = fmt_start;
        }
    }
}
