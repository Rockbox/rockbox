/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2008 Rafaël Carré
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

#include "usb.h"
#include "power.h"
#include "as3525.h"
#include <stdbool.h>

void usb_enable(bool on)
{
    (void)on;
}

void usb_init_device(void)
{
    /* TODO */
}

int usb_detect(void)
{
#if defined(SANSA_CLIP)
    return !GPIOA_PIN(6);
#elif defined(SANSA_FUZE) || defined(SANSA_E200V2)
    return !GPIOA_PIN(3);
#elif defined(SANSA_C200V2)
    return !GPIOA_PIN(1);
#else
    /* TODO: Implement USB detection */
    return USB_EXTRACTED;
#endif
}
