/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Karl Kurbjun
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
#include "kernel.h"
#include "thread.h"
#include "system.h"
#include "power.h"
#include "panic.h"
#include "pcf50606.h"
#include "ata.h"
#include "ata-target.h"
#include "backlight-target.h"

/* ARESET on C7C68300 and RESET on ATA interface (Active Low) */
#define ATA_RESET_ENABLE  (IO_GIO_BITCLR0 = 1 << 10)
#define ATA_RESET_DISABLE (IO_GIO_BITSET0 = 1 << 10)

void ata_reset(void)
{
    ATA_RESET_ENABLE;
    sleep(1); /* > 25us */
    ATA_RESET_DISABLE;
    sleep(1); /* > 2ms */
}

/* This function is called before enabling the USB bus */
void ata_enable(bool on)
{
    (void) on;
    return;
}

bool ata_is_coldstart(void)
{
    return true;
}

void ata_device_init(void)
{
    /* ATA reset */
    /*  10: output, non-inverted, no-irq, falling edge, no-chat, normal */
    dm320_set_io(10, false, false, false, false, false, 0x00);
    ATA_RESET_DISABLE; /* Set the pin to disable an active low reset */
    
    /* ATA INT (currently unused) */
    /*  11: input ,     inverted,    irq,     any edge, no-chat, normal */
    dm320_set_io(11, true, true, true, true, false, 0x00);
}

