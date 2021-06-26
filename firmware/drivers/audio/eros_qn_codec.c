/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Andrew Ryabinin, Dana Conrad
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

#include "system.h"
#include "eros_qn_codec.h"
#include "config.h"
#include "audio.h"
#include "audiohw.h"
#include "settings.h"
#include "pcm_sw_volume.h"

static long int vol_l_hw = 0;
static long int vol_r_hw = 0;

void pcm5102_set_outputs(void)
{
    audiohw_set_volume(vol_l_hw, vol_r_hw);
}

/* this makes less sense here than it does in the audiohw-*.c file,
 * but we need access to settings.h */
void audiohw_set_volume(int vol_l, int vol_r)
{
    int l, r;

    vol_l_hw = vol_l;
    vol_r_hw = vol_r;

    l = vol_l;
    r = vol_r;

#if (defined(HAVE_HEADPHONE_DETECTION) && defined(HAVE_LINEOUT_DETECTION))
    /* make sure headphones aren't present - don't want to
     * blow out our eardrums cranking it to full */
    if (lineout_inserted() && !headphones_inserted())
    {
        l = r = global_settings.volume_limit * 10;
    }
    else
    {
        l = vol_l;
        r = vol_r;
    }
#endif

    pcm_set_master_volume(l, r);
}
