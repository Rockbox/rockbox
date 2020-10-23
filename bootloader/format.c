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
 * Copyright (C) 2017 by William Wilgus
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


#include <stdarg.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>
#include "file.h"
#include "format.h"

static const char hexdigit[] = "0123456789ABCDEF";

/* smaller compressed binary without inline but 18% slower */
#define FMT_DECL static inline

FMT_DECL int fmt_width_precision(int *ch, const char **fmt, char **str, va_list *ap)
{
    int value = 0;
    (void) str;
    (void) ap;

    while (*ch >= '0' && *ch <= '9')
    {
        value = 10 * value + (*ch - '0');
        *ch = *(*fmt)++;
    }
    return value;
}

FMT_DECL int fmt_integer_signed(int *ch, const char **fmt, char **str, va_list *ap)
{
    int val, rem, sign;
    (void) ch;
    (void) fmt;

    val = sign = va_arg(*ap, int);
    if (val < 0)
        val = -val;
    do {
        rem = val % 10;
        val /= 10;
        *--(*str) = rem + '0';

    } while (val > 0);

    if (sign < 0)
        *--(*str) = '-';
    return 0;
}

FMT_DECL int fmt_integer_unsigned(int *ch, const char **fmt, char **str, va_list *ap)
{
    unsigned int uval, urem;
    (void) ch;
    (void) fmt;

    uval = va_arg(*ap, unsigned int);
    do {
        urem = uval % 10;
        uval /= 10;
        *--(*str) = urem + '0';
    } while (uval > 0);
    return 0;
}

FMT_DECL int fmt_long(int *ch, const char **fmt, char **str, va_list *ap)
{
    int pad = 0;
    long lval, lrem, lsign = 0;
    unsigned long ulval, ulrem;
    char ch_l = *ch;

    *ch = *(*fmt)++;
    if (*ch == 'd') {
        lval = lsign = va_arg(*ap, long);

        if (lval < 0)
            lval = -lval;
        do {
            lrem = lval % 10;
            lval /= 10;
            *--(*str) = lrem + '0';
        } while (lval > 0);

        if (lsign < 0)
            *--(*str) = '-';
    }
    else if (*ch == 'u') {
        ulval = va_arg(*ap, unsigned long);
        do {
            ulrem = ulval % 10;
            ulval /= 10;
            *--(*str) = ulrem + '0';
        } while (ulval > 0);
    }
    else if (*ch == 'x' || *ch == 'X') {
        pad++;
        ulval = va_arg(*ap, long);
        do {
            *--(*str) = hexdigit[ulval & 0xf];
            ulval >>= 4;
        } while (ulval > 0);
    }
    else {
        *--(*str) = ch_l;
        *--(*str) = *ch;
    }
    return pad;
}

FMT_DECL int fmt_character(int *ch, const char **fmt, char **str, va_list *ap)
{
    (void) ch;
    (void) fmt;

    *--(*str) = va_arg(*ap, int);
    return 0;
}

FMT_DECL int fmt_string(int *ch, const char **fmt, char **str, va_list *ap)
{
    (void) ch;
    (void) fmt;

    *str = va_arg (*ap, char*);
    return 0;
}

FMT_DECL int fmt_hex_unsigned(int *ch, const char **fmt, char **str, va_list *ap)
{
    unsigned int uval;
    (void) ch;
    (void) fmt;

    uval = va_arg(*ap, int);
    do {
        *--(*str) = hexdigit[uval & 0xf];
        uval >>= 4;
    } while (uval > 0);
    return 1;
}

FMT_DECL int fmt_pointer(int *ch, const char **fmt, char **str, va_list *ap)
{
    int pad = fmt_hex_unsigned(ch, fmt, str, ap);
    /* for pointers prepend 0x and act like 'X' */
    *--(*str) = 'x';
    *--(*str) = '0';
    return pad;
}

FMT_DECL int fmt_sizet(int *ch, const char **fmt, char **str, va_list *ap)
{
    size_t uszval, uszrem;
    ssize_t szval, szrem, szsign;
    char ch_z = *ch;
    *ch = *(*fmt)++;

    if (*ch == 'd') {
        szval = szsign = va_arg(*ap, ssize_t);
        if (szval < 0)
            szval = -szval;
        do {
            szrem = szval % 10;
            szval /= 10;
            *--(*str) = szrem + '0';
        } while (szval > 0);

        if (szsign < 0)
            *--(*str) = '-';
    }
    else if (*ch == 'u') {
        uszval = va_arg(*ap, size_t);
        do {
            uszrem = uszval % 10;
            uszval /= 10;
            *--(*str) = uszrem + '0';
        } while (uszval > 0);
    }
    else {
        *--(*str) = ch_z;
        *--(*str) = *ch;
    }
    return 0;
}

static inline int fmt_next_char(int *ch, const char **fmt, char **str, va_list *ap)
{
    (void) fmt;
    (void) ap;

    *--(*str) = *ch;
    return 0;
}



void format(
    /* call 'push()' for each output letter */
    int (*push)(void *userp, unsigned char data),
    void *userp,
    const char *fmt,
    va_list ap)
{
    bool ok = true;
    char *str;
    char tmpbuf[12], pad;
    int ch, width, precision, padded;


    ch = *fmt++;
    tmpbuf[sizeof tmpbuf - 1] = '\0';

    do
    {
        if (ch == '%')
        {
            str = tmpbuf + sizeof tmpbuf - 1;
            ch = *fmt++;
            padded = (ch == '0' ? 1 : 0);
            width = fmt_width_precision(&ch, &fmt, &str, &ap);

            precision = INT_MAX;
            if(ch == '.')
            {
                ch = *fmt++;
                precision = fmt_width_precision(&ch, &fmt, &str, &ap);
            }

            if (ch == 'd')
                fmt_integer_signed(&ch, &fmt, &str, &ap);
            else if (ch == 'u')
                fmt_integer_unsigned(&ch, &fmt, &str, &ap);
            else if (ch == 'l')
                padded += fmt_long(&ch, &fmt, &str, &ap);
            else if (ch == 'c')
                fmt_character(&ch, &fmt, &str, &ap);
            else if (ch == 's')
                fmt_string(&ch, &fmt, &str, &ap);
            else if (ch == 'x' || ch == 'X')
                padded += fmt_hex_unsigned(&ch, &fmt, &str, &ap);
            else if (ch == 'p' || ch == 'P')
                padded += fmt_pointer(&ch, &fmt, &str, &ap);
#if 0
            else if (ch == 'z')
                fmt_sizet(&ch, &fmt, &str, &ap);
#endif
            else
                fmt_next_char(&ch, &fmt, &str, &ap);

            width -= strlen (str);
            if (width > 0)
            {
                pad = (padded ? '0' : ' ');
                while (width-- > 0 && ok)
                    ok=push(userp, pad);
            }
            while(*str != '\0' && ok && precision--)
                    ok=push(userp, *str++);
        }
        else
            ok=push(userp, ch);

    } while ((ch = *fmt++) != '\0' && ok);
}

struct for_fprintf {
    int fd;    /* where to store it */
    int bytes; /* amount stored */
};

static int fprfunc(void *pr, unsigned char letter)
{
    struct for_fprintf *fpr  = (struct for_fprintf *)pr;
    int rc = write(fpr->fd, &letter, 1);

    if(rc > 0) {
        fpr->bytes++; /* count them */
        return true;  /* we are ok */
    }

    return false; /* failure */
}


int fdprintf(int fd, const char *fmt, ...)
{
    va_list ap;
    struct for_fprintf fpr;

    fpr.fd=fd;
    fpr.bytes=0;

    va_start(ap, fmt);
    format(fprfunc, &fpr, fmt, ap);
    va_end(ap);

    return fpr.bytes; /* return 0 on error */
}

void vuprintf(int (*push)(void *userp, unsigned char data), void *userp, const char *fmt, va_list ap)
{
    format(push, userp, fmt, ap);
}
