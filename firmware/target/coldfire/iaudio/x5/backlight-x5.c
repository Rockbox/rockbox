/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "cpu.h"
#include "system.h"
#include "backlight.h"
#include "pcf50606.h"

void __backlight_on(void)
{
    int level = set_irq_level(HIGHEST_IRQ_LEVEL);
    pcf50606_write(0x38, 0x30); /* Backlight ON */
    set_irq_level(level);
}

void __backlight_off(void)
{
    int level = set_irq_level(HIGHEST_IRQ_LEVEL);
    pcf50606_write(0x38, 0x70); /* Backlight OFF */
    set_irq_level(level);
}

