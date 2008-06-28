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
