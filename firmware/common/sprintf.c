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

/*
 * Minimal printf and snprintf formatting functions
 *
 * These support %c %s %d and %x
 * Field width and zero-padding flag only
 */

#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

#include "file.h" /* for write(), used in fprintf() */
#include "sprintf.h" /* to allow the simulator magic */

static const char hexdigit[] = "0123456789ABCDEF";

static int format(
    /* call 'push()' for each output letter */
    int (*push)(void *userp, unsigned char data),
    void *userp,
    const char *fmt,
    va_list ap)
{
    char *str;
    char tmpbuf[12], pad;
    int ch, width, val, sign, precision;
    long lval, lsign;
    unsigned int uval;
    unsigned long ulval;
    bool ok = true;

    tmpbuf[sizeof tmpbuf - 1] = '\0';

    while ((ch = *fmt++) != '\0' && ok)
    {
    if (ch == '%')
    {
        ch = *fmt++;
        pad = ' ';
        if (ch == '0')
        pad = '0';

        width = 0;
        while (ch >= '0' && ch <= '9')
        {
        width = 10*width + ch - '0';
        ch = *fmt++;
        }
        
        precision = 0;
        if(ch == '.')
        {
            ch = *fmt++;
            while (ch >= '0' && ch <= '9')
            {
                precision = 10*precision + ch - '0';
                ch = *fmt++;
            }
        } else {
            precision = INT_MAX;
        }

        str = tmpbuf + sizeof tmpbuf - 1;
        switch (ch)
        {
        case 'c':
            *--str = va_arg (ap, int);
            break;

        case 's':
            str = va_arg (ap, char*);
            break;

        case 'd':
            val = sign = va_arg (ap, int);
            if (val < 0)
            val = -val;
            do
            {
            *--str = (val % 10) + '0';
            val /= 10;
            }
            while (val > 0);
            if (sign < 0)
            *--str = '-';
            break;

        case 'u':
            uval = va_arg(ap, unsigned int);
            do
            {
            *--str = (uval % 10) + '0';
            uval /= 10;
            }
            while (uval > 0);
            break;

        case 'x':
        case 'X':
            uval = va_arg (ap, int);
            do
            {
            *--str = hexdigit[uval & 0xf];
            uval >>= 4;
            }
            while (uval);
            break;

        case 'l':
            ch = *fmt++;
            switch(ch) {
                case 'x':
                case 'X':
                    ulval = va_arg (ap, long);
                    do
                    {
                        *--str = hexdigit[ulval & 0xf];
                        ulval >>= 4;
                    }
                    while (ulval);
                    break;
                case 'd':
                    lval = lsign = va_arg (ap, long);
                    if (lval < 0)
                        lval = -lval;
                    do
                    {
                        *--str = (lval % 10) + '0';
                        lval /= 10;
                    }
                    while (lval > 0);
                    if (lsign < 0)
                        *--str = '-';
                    break;

                case 'u':
                    ulval = va_arg(ap, unsigned long);
                    do
                    {
                        *--str = (ulval % 10) + '0';
                        ulval /= 10;
                    }
                    while (ulval > 0);
                    break;

                default:
                    *--str = 'l';
                    *--str = ch;
            }

            break;

        default:
            *--str = ch;
            break;
        }

        if (width > 0)
        {
        width -= strlen (str);
        while (width-- > 0 && ok)
            ok=push(userp, pad);
        }
        while (*str != '\0' && ok && precision--)
                ok=push(userp, *str++);
    }
    else
        ok=push(userp, ch);
    }
    return ok; /* true means good */
}

#if !defined(SIMULATOR) || !defined(linux)
/* ALSA library requires a more advanced snprintf, so let's not
   override it in simulator for Linux.  Note that Cygwin requires
   our snprintf or it produces garbled output after a while. */

struct for_snprintf {
    unsigned char *ptr; /* where to store it */
    int bytes; /* amount already stored */
    int max;   /* max amount to store */
};

static int sprfunc(void *ptr, unsigned char letter)
{
    struct for_snprintf *pr = (struct for_snprintf *)ptr;
    if(pr->bytes < pr->max) {
        *pr->ptr = letter;
        pr->ptr++;
        pr->bytes++;
        return true;
    }
    return false; /* filled buffer */
}


int snprintf(char *buf, size_t size, const char *fmt, ...)
{
    bool ok;
    va_list ap;
    struct for_snprintf pr;

    pr.ptr = (unsigned char *)buf;
    pr.bytes = 0;
    pr.max = size;

    va_start(ap, fmt);
    ok = format(sprfunc, &pr, fmt, ap);
    va_end(ap);

    /* make sure it ends with a trailing zero */
    pr.ptr[(pr.bytes < pr.max) ? 0 : -1] = '\0';
    
    return pr.bytes;
}

int vsnprintf(char *buf, int size, const char *fmt, va_list ap)
{
    bool ok;
    struct for_snprintf pr;

    pr.ptr = (unsigned char *)buf;
    pr.bytes = 0;
    pr.max = size;

    ok = format(sprfunc, &pr, fmt, ap);

    /* make sure it ends with a trailing zero */
    pr.ptr[(pr.bytes < pr.max) ? 0 : -1] = '\0';
    
    return pr.bytes;
}

#endif /* Linux SIMULATOR */

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
    bool ok;
    va_list ap;
    struct for_fprintf fpr;

    fpr.fd=fd;
    fpr.bytes=0;

    va_start(ap, fmt);
    ok = format(fprfunc, &fpr, fmt, ap);
    va_end(ap);

    return fpr.bytes; /* return 0 on error */
}

