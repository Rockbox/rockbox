/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Jens Arnold
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include "cpu.h"
#include "system.h"
#include "backlight.h"
#include "backlight-target.h"

bool _backlight_init(void)
{
    and_l(~0x00000008, &GPIO1_OUT);
    or_l(0x00000008, &GPIO1_ENABLE);
    or_l(0x00000008, &GPIO1_FUNCTION);
    return true; /* Backlight always ON after boot. */
}

void _backlight_on(void)
{
    and_l(~0x00000008, &GPIO1_OUT);
}

void _backlight_off(void)
{
    or_l(0x00000008, &GPIO1_OUT);
}
