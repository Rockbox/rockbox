/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2026 Aidan MacDonald
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
#ifndef __SYSTEM_ECHOPLAYER_H__
#define __SYSTEM_ECHOPLAYER_H__

enum echoplayer_rtcout_mode
{
    ECHOPLAYER_RTCOUT_DISABLED,
    ECHOPLAYER_RTCOUT_REBOOT,
};

enum echoplayer_boot_reason
{
    ECHOPLAYER_BOOT_REASON_NORMAL,
    ECHOPLAYER_BOOT_REASON_SW_POWEROFF,
    ECHOPLAYER_BOOT_REASON_SW_REBOOT,
};

void echoplayer_set_rtcout_mode(enum echoplayer_rtcout_mode mode);

extern enum echoplayer_boot_reason echoplayer_boot_reason;

#endif /* __SYSTEM_ECHOPLAYER_H__ */
