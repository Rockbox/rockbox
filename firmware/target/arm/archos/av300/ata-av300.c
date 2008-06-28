/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: ata-pp5020.c 10521 2006-08-11 08:35:27Z bger $
 *
 * Target-specific ATA functions for AV3xx (TMS320DSC25)
 *
 * Based on code from the ArchOpen project - http://www.archopen.org
 * Adapted for Rockbox in January 2007
 *
 * Original file: 
 *   lib/target/arch_AV3XX/ata.c
 *
 *   AvLo - linav project
 *   Copyright (c) 2005 by Christophe THOMAS (oxygen77 at free.fr)
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

#include <stdbool.h>
#include "system.h"
#include "ata-target.h"

void ata_reset() 
{
    /* arch_ata_reset_HD(void) */
    cpld_set_port_2(CPLD_HD_RESET);
    cpld_clear_port_2(CPLD_HD_RESET);
}

void ata_enable(bool on)
{
    /* TODO: Implement ata_enable() */
    (void)on;
}

bool ata_is_coldstart()
{
    /* TODO: Implement coldstart variable */
    return true;
}

void ata_device_init()
{
    /* Set CF/HD selection to HD */
    cpld_select(CPLD_HD_CF,CPLD_SEL_HD);
}
