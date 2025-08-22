/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2009 by Rafaël Carré
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
#include "sdmmc.h"
#include "storage.h"

void sd_sleep(void)
{
}

void sd_spin(void)
{
}

void sd_spindown(int seconds)
{
    (void)seconds;
}

#ifdef STORAGE_GET_INFO
void sd_get_info(IF_MD(int drive,) struct storage_info *info)
{
#ifndef HAVE_MULTIDRIVE
    const int drive=0;
#endif

    tCardInfo *card = card_get_info_target(drive);

    info->sector_size=card->blocksize;
    info->num_sectors=card->numblocks;
    info->vendor="Rockbox";
#if CONFIG_STORAGE == STORAGE_SD
    info->product = (drive==0) ? "Internal Storage" : "SD Card Slot";
#else   /* Internal storage is not SD */
    info->product = "SD Card Slot";
#endif
    info->revision="0.00";
}
#endif
