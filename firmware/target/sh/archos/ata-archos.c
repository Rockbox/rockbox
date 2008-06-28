/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Jens Arnold
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
#include <stdbool.h>
#include "kernel.h"
#include "system.h"
#include "ata-target.h"
#include "hwcompat.h"

#define ATA_CONTROL1    ((volatile unsigned char*)0x06200206)
#define ATA_CONTROL2    ((volatile unsigned char*)0x06200306)

volatile unsigned char* ata_control;

void ata_reset(void)
{
    /* state HRR0 */
    and_b(~0x02, &PADRH); /* assert _RESET */
    sleep(1); /* > 25us */

    /* state HRR1 */
    or_b(0x02, &PADRH); /* negate _RESET */
    sleep(1); /* > 2ms */
}

void ata_enable(bool on)
{
    if(on)
        and_b(~0x80, &PADRL); /* enable ATA */
    else
        or_b(0x80, &PADRL); /* disable ATA */

    or_b(0x80, &PAIORL);
}

void ata_device_init(void)
{
    or_b(0x02, &PAIORH); /* output for ATA reset */
    or_b(0x02, &PADRH);  /* release ATA reset */
    PACR2 &= 0xBFFF; /* GPIO function for PA7 (IDE enable) */
 
    if (HW_MASK & ATA_ADDRESS_200)
        ata_control = ATA_CONTROL1;
    else
        ata_control = ATA_CONTROL2;
}

bool ata_is_coldstart(void)
{
    return (PACR2 & 0x4000) != 0;
}
