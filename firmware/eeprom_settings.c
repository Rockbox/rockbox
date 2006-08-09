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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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

static void reset_config(void)
{
    memset(&firmware_settings, 0, sizeof(struct eeprom_settings));
    firmware_settings.version = EEPROM_SETTINGS_VERSION;
    firmware_settings.initialized = true;
    firmware_settings.boot_disk = false;
    firmware_settings.bl_version = 0;
}

bool eeprom_settings_init(void)
{
    int ret;
    uint32_t sum;
    
    eeprom_24cxx_init();
    
    /* Check if player has been flashed. */
    if (!detect_flashed_rockbox())
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
        reset_config();
        return true;
    }
    
    if (firmware_settings.checksum != sum)
    {
        logf("Checksum mismatch");
        reset_config();
        return true;
    }
    
#ifndef BOOTLOADER
    if (firmware_settings.bl_version < EEPROM_SETTINGS_BL_MINVER)
    {
        logf("Too old bootloader: %d", firmware_settings.bl_version);
        reset_config();
        return true;
    }
#endif
    
    return true;
}

bool eeprom_settings_store(void)
{
    int ret;
    uint32_t sum;
    
    if (!firmware_settings.initialized || !detect_flashed_rockbox())
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

