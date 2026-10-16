/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2026 Mauricio G.
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

#ifndef _SYS_KERNEL_HOOK_H
#define _SYS_KERNEL_HOOK_H

#include <pspctrl.h>
#include <pspimpose_driver.h>
/* #include <pspdisplay_kernel.h> */

void sys_init_kernel_hook(void);

/* kernel display driver */
int sys_display_enable(void);
int sys_display_disable(void);
void sys_display_get_brightness(int *level);
void sys_display_set_brightness(int level);

/* kernel impose driver */
int sys_impose_get_param(SceImposeParam param);
int sys_impose_set_param(SceImposeParam param, int value);

/* kernel audio driver */
int sys_audio_set_frequency(int frequency);

/* kernel controller driver */
int sys_ctrl_peek_buffer_positive(SceCtrlData *pad_data, int count);

#endif /* _SYS_KERNEL_HOOK_H */