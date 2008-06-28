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

#include "eeprom_settings.h"
#include "eeprom_24cxx.h"
#include "crc32.h"

#include "system.h"
#include "string.h"
#include "logf.h"

struct eeprom_settings firmware_settings;

static bool reset_config(void)
{
    memset(&firmware_settings, 0, sizeof(struct eeprom_settings));
#ifdef BOOTLOADER
    /* Don't reset settings if we are inside bootloader. */
    firmware_settings.initialized = false;
#else
    firmware_settings.version = EEPROM_SETTINGS_VERSION;
    firmware_settings.initialized = true;
    firmware_settings.bootmethod = BOOT_RECOVERY;
    firmware_settings.bl_version = 0;
#endif
    
    return firmware_settings.initialized;
}

bool eeprom_settings_init(void)
{
    int ret;
    uint32_t sum;
    
    eeprom_24cxx_init();
    
    /* Check if player has been flashed. */
    if (detect_original_firmware())
    {
        memset(&firmware_settings, 0, sizeof(struct eeprom_settings));
        firmware_settings.initialized = false;
        logf("Rockbox in flash is required");
        return false;
    }
    
    ret = eeprom_24cxx_read(0, &firmware_settings, 
                            sizeof(struct eeprom_settings));

    if (ret < 0)
    {
        memset(&firmware_settings, 0, sizeof(struct eeprom_settings));
        firmware_settings.initialized = false;
        return false;
    }
    
    sum = crc_32(&firmware_settings, sizeof(struct eeprom_settings)-4,
                 0xffffffff);
    
    logf("BL version: %d", firmware_settings.bl_version);
    if (firmware_settings.version != EEPROM_SETTINGS_VERSION)
    {
        logf("Version mismatch");
        return reset_config();
    }
    
    if (firmware_settings.checksum != sum)
    {
        logf("Checksum mismatch");
        return reset_config();
    }
    
#ifndef BOOTLOADER
    if (firmware_settings.bl_version < EEPROM_SETTINGS_BL_MINVER)
    {
        logf("Too old bootloader: %d", firmware_settings.bl_version);
        return reset_config();
    }
#endif
    
    return true;
}

bool eeprom_settings_store(void)
{
    int ret;
    uint32_t sum;
    
    if (!firmware_settings.initialized || detect_original_firmware())
    {
        logf("Rockbox in flash is required");
        return false;
    }
    
    /* Update the checksum. */
    sum = crc_32(&firmware_settings, sizeof(struct eeprom_settings)-4,
                 0xffffffff);
    firmware_settings.checksum = sum;
    ret = eeprom_24cxx_write(0, &firmware_settings,
                             sizeof(struct eeprom_settings));
    
    if (ret < 0)
        firmware_settings.initialized = false;
    
    return ret;
}

