/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
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
#include <stdbool.h>
#include "adc.h"
#include "kernel.h"
#include "system.h"
#include "power.h"
#include "usb.h"
#include "backlight-target.h"

#if CONFIG_TUNER

static bool powered = false;

bool tuner_power(bool status)
{
    bool old_status = powered;

    powered = status;
    if (status)
    {
        and_b(~0x04, &PADRL); /* drive PA2 low for tuner enable */
        sleep(1); /* let the voltage settle */
    }
    else
        or_b(0x04, &PADRL); /* drive PA2 high for tuner disable */
    return old_status;
}

#endif /* #if CONFIG_TUNER */

void power_init(void)
{
    PBCR2 &= ~0x0c00;    /* GPIO for PB5 */
    or_b(0x20, &PBIORL);
    or_b(0x20, &PBDRL);  /* hold power */
#ifndef HAVE_BACKLIGHT
    /* Disable backlight on backlight-modded Ondios when running
     * a standard build (always on otherwise). */
    PACR1 &= ~0x3000;    /* Set PA14 (backlight control) to GPIO */
    and_b(~0x40, &PADRH); /* drive it low */
    or_b(0x40, &PAIORH); /* ..and output */
#endif
    PACR2 &= ~0x0030;  /* GPIO for PA2 */
    or_b(0x04, &PADRL); /* drive PA2 high for tuner disable */
    or_b(0x04, &PAIORL); /* output for PA2 */
}

void power_off(void)
{
    set_irq_level(HIGHEST_IRQ_LEVEL);
#ifdef HAVE_BACKLIGHT
    /* Switch off the light on backlight-modded Ondios */
    _backlight_off();
#endif
    and_b(~0x20, &PBDRL);
    or_b(0x20, &PBIORL);
    while(1)
        yield();
}
