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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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

static const char hexdigit[] = "0123456789ABCDEF";

int vsnprintf (char *buf, int size, const char *fmt, va_list ap)
{
    char *bp = buf;
    char *end = buf + size - 1;

    char *str;
    char tmpbuf[12], pad;
    int ch, width, val, sign;

    tmpbuf[sizeof tmpbuf - 1] = '\0';

    while ((ch = *fmt++) != '\0' && bp < end)
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

		case 'x':
		case 'X':
		    val = va_arg (ap, int);
		    do
		    {
			*--str = hexdigit[val & 0xf];
			val >>= 4;
		    }
		    while (val > 0);
		    break;

		default:
		    *--str = ch;
		    break;
	    }

	    if (width > 0)
	    {
		width -= strlen (str);
		while (width-- > 0 && buf < end)
		    *bp++ = pad;
	    }
	    while (*str != '\0' && buf < end)
		*bp++ = *str++;
	}
	else
	    *bp++ = ch;
    }

    *bp++ = '\0';
    return bp - buf - 1;
}

int snprintf (char *buf, int size, const char *fmt, ...)
{
    int n;
    va_list ap;

    va_start (ap, fmt);
    n = vsnprintf (buf, size, fmt, ap);
    va_end (ap);

    return n;
}
