/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 by Simon Rothen, Udo Schläpfer.
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

#include "usb-dx50.h"


void usb_enable_adb( void )
{
    system( "setprop persist.sys.usb.config adb" );
    system( "setprop persist.usb.debug 1" );
}


void usb_enable_mass_storage( void )
{
    system( "setprop persist.sys.usb.config mass_storage" );
    system( "setprop persist.usb.debug 0" );
}


void set_usb_mode( int mode )
{
    switch( mode )
    {
        case USB_MODE_MASS_STORAGE:
        {
            usb_enable_mass_storage();
            break;
        }

        case USB_MODE_CHARGE_ONLY: /* Work around. */
        case USB_MODE_ADB:
        {
            usb_enable_adb();
            break;
        }
    }
}