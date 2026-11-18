/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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
#ifndef __SYSSTORAGE_PSP_H__
#define __SYSSTORAGE_PSP_H__

/* PSP only: ownership of the Memory Stick, see sys_storage_psp.c */
void sys_storage_detach(void);        /* at storage init: Rockbox owns the card */
void sys_storage_window_open(void);   /* ms0: back briefly, Rockbox file ops held */
void sys_storage_window_close(void);
void sys_storage_reattach(void);      /* at exit */

#endif /* __SYSSTORAGE_PSP_H__ */
