/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Maurus Cuelenaere
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

#ifndef __INCLUDE_CHINACHIP_H_
#define __INCLUDE_CHINACHIP_H_

#ifdef __cplusplus
extern "C" {
#endif

enum cc_error {
    E_OK,
    E_OPEN_FIRMWARE,
    E_OPEN_BOOTLOADER,
    E_MEMALLOC,
    E_LOAD_FIRMWARE,
    E_INVALID_FILE,
    E_NO_CCPMP,
    E_OPEN_BACKUP,
    E_WRITE_BACKUP,
    E_LOAD_BOOTLOADER,
    E_GET_TIME,
    E_OPEN_OUTFILE,
    E_WRITE_OUTFILE,
};

enum cc_error chinachip_patch(const char* firmware, const char* bootloader,
                    const char* output, const char* ccpmp_backup);

#ifdef __cplusplus
}
#endif

#endif /* __INCLUDE_CHINACHIP_H_ */
