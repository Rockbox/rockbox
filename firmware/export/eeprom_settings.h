/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Miika Pekkarinen
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

#ifndef _EEPROM_SETTINGS_H_
#define _EEPROM_SETTINGS_H_

#include <stdbool.h>
#include "inttypes.h"

#define EEPROM_SETTINGS_VERSION   0x24c01002
#define EEPROM_SETTINGS_BL_MINVER 7

enum boot_methods {
    BOOT_DISK = 0,
    BOOT_RAM,
    BOOT_ROM,
    BOOT_RECOVERY,
};

struct eeprom_settings
{
    long version;       /* Settings version number */
    bool initialized;   /* Is eeprom_settings ready to be used */
    bool disk_clean;    /* Is disk intact from last reboot */
    uint8_t bootmethod; /* The default boot method. */
    uint8_t bl_version; /* Installed bootloader version */
    
    long reserved;      /* A few reserved bits for the future. */
    
    /* This must be the last entry */
    uint32_t checksum;  /* Checksum of this structure */
};

extern struct eeprom_settings firmware_settings;

bool eeprom_settings_init(void);
bool eeprom_settings_store(void);

#endif

