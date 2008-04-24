/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include "system.h"
#include "kernel.h"
#include "usb.h"
#include "usb-target.h"
#include "usb_drv.h"
#include "usb_core.h"
#include "isp1583.h"

#define printf

bool usb_drv_connected(void)
{
    return button_usb_connected();
}

int usb_detect(void)
{
    if(button_usb_connected())
        return USB_INSERTED;
    else
        return USB_EXTRACTED;
}

void usb_init_device(void)
{
    return;
}

void usb_enable(bool on)
{
    if(on)
        usb_core_init();
    else
         usb_core_exit();
}

void GIO7(void)
{
#ifdef DEBUG
    //printf("GIO7 interrupt... [%d]", current_tick);
#endif
	usb_drv_int();
    
	IO_INTC_IRQ1 = INTR_IRQ1_EXT7;
}
