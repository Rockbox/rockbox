/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr & Nick Robinson
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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "serial.h"

void dprintf(const char * str, ... )
{
    char dprintfbuff[256];
    char * ptr;

    va_list ap;
    va_start(ap, str);

    ptr = dprintfbuff;
    vsnprintf(ptr,sizeof(dprintfbuff),str,ap);
    va_end(ap);

    serial_tx((unsigned char *)ptr);
}

void serial_tx(const unsigned char * buf)
{
    /*Tx*/
    for(;;) {
        if(tx_rdy()) {
            if(*buf == '\0')
                return;
            if(*buf == '\n')
                tx_writec('\r');
            tx_writec(*buf);
            buf++;
        }
    }
}
