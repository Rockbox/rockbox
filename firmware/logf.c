/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Daniel Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/*
 * logf() logs 16 bytes in a circular buffer. Each logged string is space-
 * padded for easier and faster output on screen. Just output 16 lines on each
 * line. 16 bytes fit nicely on the iRiver remote LCD (128 pixels with an 8
 * pixels font).
 */

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <sprintf.h>
#include <stdbool.h>
#include "config.h"

#define MAX_LOGF_LINES 1000
#define MAX_LOGF_DATASIZE (16*MAX_LOGF_LINES)

unsigned char logfbuffer[MAX_LOGF_DATASIZE];
unsigned char *logfend=&logfbuffer[MAX_LOGF_DATASIZE];
unsigned char *logfptr=&logfbuffer[0];
bool logfwrap;

void logf(const char *format, ...)
{
    int len;
    va_list ap;
    va_start(ap, format);

    if(logfptr >= logfend) {
        /* wrap */
        logfwrap = true;
        logfptr = &logfbuffer[0];
    }
    len = vsnprintf(logfptr, 16, format, ap);
    va_end(ap);
    if(len < 16)
        /* pad with spaces up to the 16 byte border */
        memset(logfptr+len, ' ', 16-len);

    logfptr += 16; /* leave it where we write the next time */
}
