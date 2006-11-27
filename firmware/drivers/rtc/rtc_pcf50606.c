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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
    int oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
    
    rc = pcf50606_read_multiple(0x0a, buf, 7);

    set_irq_level(oldlevel);
    return rc;
}

int rtc_write_datetime(unsigned char* buf) {
    int rc;
    int oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
    
    rc = pcf50606_write_multiple(0x0a, buf, 7);

    set_irq_level(oldlevel);

    return rc;
}

