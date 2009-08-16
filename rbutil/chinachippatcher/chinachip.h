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

int chinachip_patch(const char* firmware, const char* bootloader,
                    const char* output, const char* ccpmp_backup,
                    void (*info)(void*, char*, ...),
                    void (*err)(void*, char*, ...),
                    void* userdata);

#ifdef __cplusplus
}
#endif

#endif /* __INCLUDE_CHINACHIP_H_ */
