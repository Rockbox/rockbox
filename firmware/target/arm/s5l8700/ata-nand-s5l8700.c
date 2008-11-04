/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Dave Chapman
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
#include "ata.h"
#include "ata-target.h"
#include "nand_idle_notify.h"
#include "system.h"
#include <string.h>
#include "thread.h"
#include "led.h"
#include "disk.h"
#include "panic.h"
#include "usb.h"

/* for compatibility */
int nand_spinup_time = 0;

long last_disk_activity = -1;

/** static, private data **/ 
static bool initialized = false;

static long next_yield = 0;
#define MIN_YIELD_PERIOD 2000

/* API Functions */

void nand_led(bool onoff)
{
    led(onoff);
}

int nand_read_sectors(IF_MV2(int drive,) unsigned long start, int incount,
                     void* inbuf)
{

}

int nand_write_sectors(IF_MV2(int drive,) unsigned long start, int count,
                      const void* outbuf)
{
}

void nand_spindown(int seconds)
{
    (void)seconds;
}

bool nand_disk_is_active(void)
{
    return 0;
}

void nand_sleep(void)
{
}

void nand_spin(void)
{
}

int nand_soft_reset(void)
{
    return 0;
}

void nand_enable(bool on)
{
}

int nand_init(void)
{
}
