/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Ulrich Pegelow
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

#include "system.h"
#include "kernel.h"
#include "logf.h"
#include "m5636.h"
#include "m5636-target.h"

/* Disclaimer: This code had to be developed without any documentation for the
               M5636 USBOTG chip, due to the restrictive information policy of
               its manufacturer.
               The development is solely based on reverse engineering.
               Malfunctioning (with the risk of possible damage to the
               hardware) can not be fully excluded.
               USE THIS CODE AT YOUR OWN RISK!
*/

/* Init: This currently just puts the M5636 into sleep mode */

void m5636_init(void)
{
    m5636_device_init();

    M5636_4068 |= 0x0003; /* ???? */
    M5636_4068 |= 0x0080; /* ???? */
    M5636_4078 |= 0x0001; /* ???? */
}

