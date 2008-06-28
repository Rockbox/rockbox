/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing, Uwe Freese, Laurent Baum
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
#include "i2c.h"
#include "rtc.h"
#include "kernel.h"
#include "system.h"
#include "pcf50606.h"
#include <stdbool.h>

void rtc_init(void)
{
}

int rtc_read_datetime(unsigned char* buf) {
    int rc;
    int oldlevel = disable_irq_save();
    
    rc = pcf50606_read_multiple(0x0a, buf, 7);

    restore_irq(oldlevel);
    return rc;
}

int rtc_write_datetime(unsigned char* buf) {
    int rc;
    int oldlevel = disable_irq_save();
    
    rc = pcf50606_write_multiple(0x0a, buf, 7);

    restore_irq(oldlevel);

    return rc;
}

