/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by wavey@wavey.org
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include "panic.h"
#include "lcd.h"
#include "debug.h"

char panic_buf[128];

/*
 * "Dude. This is pretty fucked-up, right here." 
 */
void panicf( char *fmt, ...)
{
    va_list ap;
    
    va_start( ap, fmt );
    vsnprintf( panic_buf, sizeof(panic_buf), fmt, ap );
    va_end( ap );

    lcd_puts(0,0,panic_buf);
    DEBUGF(panic_buf);
    while(1);
}
