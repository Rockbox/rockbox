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


#ifndef _USB_DX50_H_
#define _USB_DX50_H_


/* Supported usb modes. */
enum
{
    USB_MODE_MASS_STORAGE = 0,
    USB_MODE_CHARGE_ONLY,
    USB_MODE_ADB
};

/*
    Set the usb mode
    mode: Either USB_MODE_MASS_STORAGE, USB_MODE_CHARGE_ONLY or USB_MODE_ADB.
*/
void set_usb_mode( int mode );


#endif
