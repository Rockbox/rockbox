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
#include "ata_mmc.h"
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
    if (on)
    {
        mmc_enable_int_flash_clock(!mmc_detect());

        if (!(HW_MASK & MMC_CLOCK_POLARITY))
            and_b(~0x20, &PBDRH); /* old circuit needs SCK1 = low while on USB */
        or_b(0x20, &PADRL); /* enable USB */
        and_b(~0x08, &PADRL); /* assert card detect */
    }
    else
    {
        if (!(HW_MASK & MMC_CLOCK_POLARITY))
            or_b(0x20, &PBDRH); /* reset SCK1 = high for old circuit */
        and_b(~0x20, &PADRL); /* disable USB */
        or_b(0x08, &PADRL); /* deassert card detect */
    }
}

void usb_init_device(void)
{
    PACR2 &= ~0x04C0;     /* use PA3 (card detect) and PA5 (USB enabled) as GPIO */
    and_b(~0x20, &PADRL); /* disable USB */
    or_b(0x08, &PADRL);   /* deassert card detect */
    or_b(0x28, &PAIORL);  /* output for USB enable and card detect */
}
