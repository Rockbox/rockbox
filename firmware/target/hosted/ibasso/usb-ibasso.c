/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 by Ilia Sergachev: Initial Rockbox port to iBasso DX50
 * Copyright (C) 2014 by Mario Basister: iBasso DX90 port
 * Copyright (C) 2014 by Simon Rothen: Initial Rockbox repository submission, additional features
 * Copyright (C) 2014 by Udo Schläpfer: Code clean up, additional features
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

#include <stdlib.h>

#include "config.h"
#include "debug.h"

#include "usb.h"
#include "debug-ibasso.h"
#include "usb-ibasso.h"

static void usb_enable_adb(void)
{
    TRACE;

    if(system(NULL))
    {
        system("setprop persist.sys.usb.config adb");
        system("setprop persist.usb.debug 1");
        return;
    }

    DEBUGF("ERROR %s: No command processor available.", __func__);
}

static void usb_enable_mass_storage(void)
{
    TRACE;

    if(system(NULL))
    {
        system("setprop persist.sys.usb.config mass_storage");
        system("setprop persist.usb.debug 0");
        return;
    }

    DEBUGF("ERROR %s: No command processor available.", __func__);
}

/* Default at boot not known. */
static int _last_usb_mode = -1;

void ibasso_set_usb_mode(int mode)
{
    DEBUGF("DEBUG %s: _last_usb_mode: %d, mode: %d.", __func__, _last_usb_mode, mode);

    if(_last_usb_mode != mode)
    {
        switch(mode)
        {
            case USB_MODE_MASS_STORAGE:
            {
                _last_usb_mode = mode;
                usb_enable_mass_storage();
                break;
            }

            case USB_MODE_CHARGE: /* Work around. */
            case USB_MODE_ADB:
            {
                _last_usb_mode = mode;
                usb_enable_adb();
                break;
            }
        }
    }
}

bool usb_powered_only(void)
{
  return false;
}
