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
#include "stdint.h"
#include "gcc_extensions.h"
#include "vuprintf.h"

#define FMT_H_SUPPORT  0    /* signed/unsigned short formatting */
#define FMT_HH_SUPPORT 0    /* signed/unsigned char formatting */
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

struct fmt_spec {
    int  align;        /* 0-pad ('0') or left-align flag ('-')  */
    int  signchar;     /* positive radix-10 sign character (' ') or ('+') */
    int  pfxlen;       /* offset of prefix from formatted field (negative) */
    int  width;        /* field width specifier */
    int  precision;    /* precision specifier */
    int  radixchar;    /* radix specifier */
    int  len;          /* length of field data */
    char buf[24];      /* work buffer (largest int length plus any prefix) */
    const char *start; /* first character of formatter */
};

#define FMT_SPEC_BUFEND(spec) \
    (&(spec)->buf[sizeof ((spec)->buf)])

#define FMT_SPEC_LEN(spec, p) \
    ((int)(FMT_SPEC_BUFEND(spec) - (const char *)(p)))

#define LENMOD_CALL(lenmod, spec, val, sign) \
    ({  const char *__buf;                      \
        switch (lenmod) {                       \
        default:                                \
            __buf = (sign) ?                    \
                format_d((spec), (val)) :       \
                format_u((spec), (val));        \
            break;                              \
        IF_FMT_L(                               \
        case LENMOD_l:                          \
            __buf = (sign) ?                    \
                format_ld((spec), (val)) :      \
                format_lu((spec), (val));       \
            break;                              \
        )                                       \
        IF_FMT_LL(                              \
        case LENMOD_ll:                         \
            __buf = (sign) ?                    \
                format_lld((spec), (val)) :     \
                format_llu((spec), (val));      \
            break;                              \
        )                                       \
        }                                       \
        __buf; })

#define CONVERT_RADIX_10_SIGN(spec, p, val, type) \
    do {                                        \
        if (val || spec->precision) {           \
            unsigned type v;                    \
                                                \
            if (val < 0) {                      \
                v = -val;                       \
                spec->signchar = '-';           \
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
        if (spec->signchar) {                   \
            p[-1] = spec->signchar;             \
            spec->pfxlen = -1;                  \
        }                                       \
        else {                                  \
            spec->pfxlen = 0;                   \
        }                                       \
    } while (0)

#define CONVERT_RADIX_8(spec, p, val) \
    do {                                        \
        if (val || spec->precision) {           \
            typeof (val) v = val;               \
                                                \
            do {                                \
                *--p = (v % 010) + '0';         \
                v /= 010;                       \
            } while (v);                        \
        }                                       \
                                                \
        if (spec->pfxlen) {                     \
            p[-1] = '0';                        \
        }                                       \
    } while (0)

#define CONVERT_RADIX_10(spec, p, val) \
    do {                                        \
        if (val || spec->precision) {           \
            typeof (val) v = val;               \
                                                \
            do {                                \
                *--p = (v % 10) + '0';          \
                v /= 10;                        \
            } while (v);                        \
        }                                       \
                                                \
        spec->pfxlen = 0;                       \
    } while (0)

#define CONVERT_RADIX_16(spec, p, val) \
    do {                                        \
        if (val || spec->precision) {           \
            const int x = spec->radixchar;      \
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
            if (val && spec->pfxlen) {          \
                spec->pfxlen = -2;              \
                p[-1] = x;                      \
                p[-2] = '0';                    \
                break;                          \
            }                                   \
        }                                       \
                                                \
        spec->pfxlen = 0;                       \
    } while (0)

#define CONVERT_RADIX_NOSIGN(spec, p, val) \
    switch (spec->radixchar)                    \
    {                                           \
    IF_FMT_O( case 'o':                         \
        CONVERT_RADIX_8(spec, p, val);          \
        break; )                                \
    case 'u':                                   \
        CONVERT_RADIX_10(spec, p, val);         \
        break;                                  \
    default:                                    \
        CONVERT_RADIX_16(spec, p, val);         \
        break;                                  \
    }

/* %d %i */
static const char * format_d(struct fmt_spec *spec,
                             int val)
{
    char *buf = FMT_SPEC_BUFEND(spec);
    CONVERT_RADIX_10_SIGN(spec, buf, val, int);
    return buf;
}

/* %o %u %x %X */
static const char * format_u(struct fmt_spec *spec,
                             unsigned int val)
{
    char *buf = FMT_SPEC_BUFEND(spec);
    CONVERT_RADIX_NOSIGN(spec, buf, val);
    return buf;
}

#if FMT_L_SUPPORT
#ifndef format_ld
/* %ld %li */
static const char * format_ld(struct fmt_spec *spec,
                              long val)
{
    char *buf = FMT_SPEC_BUFEND(spec);
    CONVERT_RADIX_10_SIGN(spec, buf, val, long);
    return buf;
}
#endif /* format_ld */

#ifndef format_lu
/* %lo %lu %lx %lX */
static const char * format_lu(struct fmt_spec *spec,
                              unsigned long val)
{
    char *buf = FMT_SPEC_BUFEND(spec);
    CONVERT_RADIX_NOSIGN(spec, buf, val);
    return buf;
}
#endif /* format_lu */
#endif /* FMT_L_SUPPORT */

#if FMT_LL_SUPPORT
#ifndef format_lld
/* %lld %lli */
static const char * format_lld(struct fmt_spec *spec,
                               long long val)
{
    char *buf = FMT_SPEC_BUFEND(spec);
    CONVERT_RADIX_10_SIGN(spec, buf, val, long long);
    return buf;
}
#endif /* format_lld */

#ifndef format_llu
/* %llo %llu %llx %llX */
static const char * format_llu(struct fmt_spec *spec,
                               unsigned long long val)
{
    char *buf = FMT_SPEC_BUFEND(spec);
    CONVERT_RADIX_NOSIGN(spec, buf, val);
    return buf;
}
#endif /* format_llu */
#endif /* FMT_LL_SUPPORT */

/* %c */
static const char * format_c(struct fmt_spec *spec,
                             unsigned char c)
{
    if (spec->len != 'c') {
        return NULL; /* no wchar_t support for now */
    }

    char *buf = FMT_SPEC_BUFEND(spec);
    *--buf = c;
    spec->len = 1;
    return buf;
}

/* %s */
static const char * format_s(struct fmt_spec *spec,
                             const char *str)
{
    if (spec->len != 's') {
        return NULL; /* no wchar_t support for now */
    }

    spec->len = strlen(str);

    if (spec->precision >= 0 && spec->precision < spec->len) {
        spec->len = spec->precision;
    }

    return str;
}

#if FMT_N_SUPPORT
/* %n */
static const char * format_n(struct fmt_spec *spec,
                             int *np,
                             int count)
{
    if (spec->len != 'n') {
        return NULL;  /* int only */
    }

    *np = count;

    /* collapse field completely; return something not NULL */
    spec->len =
    spec->width = 0;
    return "";
}
#endif /* FMT_N_SUPPORT */

#if FMT_P_SUPPORT
/* %p %P */
static const char * format_p(struct fmt_spec *spec,
                             const void *p,
                             int radixchar)
{
    if (p) {
        /* format as %#x or %#X */
        spec->pfxlen = -1;
        spec->radixchar = radixchar - 'P' + 'X';
        p = LENMOD_CALL(LENMOD_p, spec, (uintptr_t)p, false);
        spec->len = FMT_SPEC_LEN(spec, p);
        return p;
    }
    else {
        /* format as %s; other valid %s formatting in effect */
        spec->len = 5;
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

int vuprintf(
    vuprintf_push_cb push, /* call 'push()' for each output letter */
    void *userp,
    const char *fmt,
    va_list ap)
{
    int count = 0;
    struct fmt_spec spec;
    const char *buf;
    int ch;

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
        spec.signchar  =
        spec.pfxlen    =
        spec.width     =
        spec.radixchar = 0;
        spec.align     = '0' + 1;
        spec.precision = -1;
        spec.start     = fmt;

        /*** flags ***/
        while (1) {
            switch (ch)
            {
            case ' ':
            case '+':
                /* '+' overrides ' ' */
                if (ch > spec.signchar) {
                    spec.signchar = ch;
                }
                break;
            case '-':
            case '0':
                /* '-' overrides '0' */
                if (ch < spec.align) {
                    spec.align = ch;
                }
                break;
            case '#':
                /* indicate; formatter updates with actual -length */
                spec.pfxlen = -1;
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
                spec.align = '-';
            }

            ch = *fmt++;
            break;
        case '1' ... '9':
            /* fixed width */
            fmt = parse_number_spec(fmt, ch, &spec.width);
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
        spec.len = ch;

        switch (ch)
        {
        #if FMT_H_SUPPORT || FMT_HH_SUPPORT
        case 'h':
            ch = *fmt++;
        #if FMT_HH_SUPPORT
            if (ch == 'h') {
                spec.len = LENMOD_hh;
                ch = *fmt++;
                break;
            }
        #endif
        #if !FMT_H_SUPPORT
            /* have hh but not h */
            ch = 0;
        #endif
            break;
        #endif /*  FMT_H_SUPPORT || FMT_HH_SUPPORT */
        #if FMT_L_SUPPORT || FMT_LL_SUPPORT
        case 'l':
            ch = *fmt++;
        #if FMT_LL_SUPPORT
            if (ch == 'l') {
                spec.len = LENMOD_ll;
                ch = *fmt++;
                break;
            }
        #endif
        #if !FMT_L_SUPPORT
            /* have ll but not l */
            ch = 0;
        #endif
            break;
        #endif /* FMT_L_SUPPORT || FMT_LL_SUPPORT */
        #if FMT_Z_SUPPORT
        case 'z':
            spec.len = LENMOD_z;
            ch = *fmt++;
            break;
        #endif /* FMT_Z_SUPPORT */
        }

        /*** radix ***/
        switch (ch)
        {
            /** non-numeric **/
        case 'c':
            buf = format_c(&spec, va_arg(ap, int));
            break;
        case 's':
            buf = format_s(&spec, va_arg(ap, const char *));
            break;
        #if FMT_N_SUPPORT
        case 'n':
            buf = format_n(&spec, va_arg(ap, int *), count);
            break;
        #endif /* FMT_N_SUPPORT */

            /** non-integer **/
        #if FMT_P_SUPPORT
        case 'p':
        case 'P':
            buf = format_p(&spec, va_arg(ap, void *), ch);
            break;
        #endif /* FMT_P_SUPPORT */

            /** signed integer **/
        case 'd':
        case 'i':;
            /* macrofied identifiers share a variable with another */
            unsigned int val_d;
        #ifndef val_ld
            unsigned long val_ld;
        #endif
        #ifndef val_lld
            unsigned long long val_lld;
        #endif

            spec.radixchar = ch;

            switch (spec.len)
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
                buf = format_d(&spec, val_d);
            } else if (0) {
            #ifndef val_ld
            branch_fmt_ld:
                buf = format_ld(&spec, val_ld);
            #endif
            } else if (0) {
            #ifndef val_lld
            branch_fmt_lld:
                buf = format_lld(&spec, val_lld);
            #endif
            }

            spec.len = FMT_SPEC_LEN(&spec, buf);
            break;

            /** unsigned integer **/
        #if FMT_O_SUPPORT
        case 'o':
        #endif
        case 'u':
        case 'x':
        case 'X':;
            /* macrofied identifiers share a variable with another */
            unsigned int val_u;
        #ifndef val_lu
            unsigned long val_lu;
        #endif
        #ifndef val_llu
            unsigned long long val_llu;
        #endif

            spec.radixchar = ch;

            switch (spec.len)
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
                buf = format_u(&spec, val_u);
            } else if (0) {
            #ifndef val_lu
            branch_fmt_lu:
                buf = format_lu(&spec, val_lu);
            #endif
            } else if (0) {
            #ifndef val_llu
            branch_fmt_llu:
                buf = format_llu(&spec, val_llu);
            #endif
            }

            spec.len = FMT_SPEC_LEN(&spec, buf);
            break;

        default:
            buf = NULL;
            break;
        }

        if (UNLIKELY(!buf)) {
            /* format not accepted; print it literally */
            buf = spec.start - 2;
            spec.len = fmt - buf;
            spec.width =
            spec.radixchar = 0;
        }

        /* calculate width and precision padding */
        if (!spec.radixchar) {
            /* non-numeric: supress prefix and precision; keep len and width */
            spec.pfxlen =
            spec.precision = 0;
        }
        else if (spec.precision >= 0) {
            /* explicit precision */
            spec.precision -= spec.len;
        }
        else if (spec.align == '0') {
            /* field zero-fill as precision */
            spec.precision = spec.width - spec.len + spec.pfxlen;
        }

        if (spec.width > 0) {
            spec.width -= spec.len - spec.pfxlen;

            if (spec.precision > 0) {
                spec.width -= spec.precision;
            }

            if (spec.width < 0) {
                spec.width = 0;
            }
            else if (spec.align == '-') {
                spec.width = -spec.width;
            }
        }

        /* push all the stuff */
        while (1) {
            if (spec.width > 0) {          /* left padding */
                ch = ' ';
                spec.width--;
            }
            else if (spec.pfxlen < 0) {    /* prefix */
                ch = buf[spec.pfxlen];
                spec.pfxlen++;
            }
            else if (spec.precision > 0) { /* 0-padding */
                ch = '0';
                spec.precision--;
            }
            else if (spec.len > 0) {       /* field */
                ch = *buf++;
                spec.len--;
            }
            else if (spec.width < 0) {     /* right padding */
                ch = ' ';
                spec.width++;
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
