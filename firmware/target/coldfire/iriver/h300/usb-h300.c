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
#include <stdbool.h>
#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "usb.h"

void usb_init_device(void)
{
    or_l(0x00000080, &GPIO1_FUNCTION); /* GPIO39 is the USB detect input */
    /* ISD300 3.3V ON */
    or_l(8,&GPIO1_FUNCTION);
    or_l(8,&GPIO1_OUT);
    or_l(8,&GPIO1_ENABLE);

    /* Tristate the SCK/SDA to the ISD300 config EEPROM */
    and_l(~0x03000000, &GPIO_ENABLE);
    or_l(0x03000000, &GPIO_FUNCTION);
}

int usb_detect(void)
{
    return (GPIO1_READ & 0x80) ? USB_INSERTED : USB_EXTRACTED;
}

void usb_enable(bool on)
{
    if(on)
    {
        /* Power on the Cypress chip */
        and_l(~0x00000008,&GPIO1_OUT);
        sleep(2);
    }
    else
    {
        /* Power off the Cypress chip */
        or_l(0x00000008,&GPIO1_OUT);
    }
}
