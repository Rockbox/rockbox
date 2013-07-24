/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Marcin Bukat
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
#include "cpu.h"
#include "string.h"
#include "usb.h"
#include "usb_drv.h"
#include "usb_core.h"
#include "system.h"
#include "system-target.h"

int usb_status = USB_EXTRACTED;

void usb_init_device(void)
{
    /* enable UDC interrupt */
    INTC_IMR |= (1<<16);
    INTC_IECR |= (1<<16);

    EN_INT = EN_SUSP_INTR   |  /* Enable Suspend Interrupt */
             EN_RESUME_INTR |  /* Enable Resume Interrupt */
             EN_USBRST_INTR |  /* Enable USB Reset Interrupt */
             EN_OUT0_INTR   |  /* Enable OUT Token receive Interrupt EP0 */
             EN_IN0_INTR    |  /* Enable IN Token transmits Interrupt EP0 */
             EN_SETUP_INTR;    /* Enable SETUP Packet Receive Interrupt */

    /* configure INTCON */
    INTCON = UDC_INTHIGH_ACT |  /* interrupt high active */
             UDC_INTEN;         /* enable EP0 interrupts */
}

void usb_attach(void)
{
    usb_enable(true);
}

void usb_enable(bool on)
{
    if(on)
        usb_core_init();
    else
        usb_core_exit();
}
