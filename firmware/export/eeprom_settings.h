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

#ifndef _EEPROM_SETTINGS_H_
#define _EEPROM_SETTINGS_H_

#include <stdbool.h>
#include "inttypes.h"

#define EEPROM_SETTINGS_VERSION   0x24c01001
#define EEPROM_SETTINGS_BL_MINVER 7

struct eeprom_settings
{
    long version;       /* Settings version number */
    bool initialized;   /* Is eeprom_settings ready to be used */
    bool disk_clean;    /* Is disk intact from last reboot */
    bool boot_disk;     /* Load firmware from disk (default=FLASH) */
    uint8_t bl_version; /* Installed bootloader version */
    
    /* This must be the last entry */
    uint32_t checksum;  /* Checksum of this structure */
};

extern struct eeprom_settings firmware_settings;

bool detect_flashed_rockbox(void);
bool eeprom_settings_init(void);
bool eeprom_settings_store(void);

#endif

