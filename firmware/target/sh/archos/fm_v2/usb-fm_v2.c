/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Jens Arnold
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include <stdbool.h>
#include "adc.h"
#include "cpu.h"
#include "hwcompat.h"
#include "system.h"
#include "usb.h"

int usb_detect(void)
{
    return (adc_read(ADC_USB_POWER) <= 512) ? USB_INSERTED : USB_EXTRACTED;
}

void usb_enable(bool on)
{
    if(HW_MASK & USB_ACTIVE_HIGH)
        on = !on;

    if(on)
        and_b(~0x04, &PADRH); /* enable USB */
    else
        or_b(0x04, &PADRH);
}

void usb_init_device(void)
{
    usb_enable(false);
    or_b(0x04, &PAIORH);
}
