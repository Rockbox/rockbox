/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021-2022 Aidan MacDonald
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

#ifndef __X1000BOOTLOADER_H__
#define __X1000BOOTLOADER_H__

#include "config.h"

#if defined(FIIO_M3K)
# define BL_RECOVERY        BUTTON_VOL_UP
# define BL_UP              BUTTON_VOL_UP
# define BL_DOWN            BUTTON_VOL_DOWN
# define BL_SELECT          BUTTON_PLAY
# define BL_QUIT            BUTTON_POWER
# define BL_UP_NAME         "VOL+"
# define BL_DOWN_NAME       "VOL-"
# define BL_SELECT_NAME     "PLAY"
# define BL_QUIT_NAME       "POWER"
# define BOOTBACKUP_FILE    "/fiiom3k-boot.bin"
#elif defined(SHANLING_Q1)
# define BL_RECOVERY        BUTTON_NEXT
# define BL_UP              BUTTON_PREV
# define BL_DOWN            BUTTON_NEXT
# define BL_SELECT          BUTTON_PLAY
# define BL_QUIT            BUTTON_POWER
# define BL_UP_NAME         "PREV"
# define BL_DOWN_NAME       "NEXT"
# define BL_SELECT_NAME     "PLAY"
# define BL_QUIT_NAME       "POWER"
# define BOOTBACKUP_FILE    "/shanlingq1-boot.bin"
#elif defined(EROS_QN)
# define BL_RECOVERY        BUTTON_VOL_UP
# define BL_UP              BUTTON_SCROLL_BACK
# define BL_DOWN            BUTTON_SCROLL_FWD
# define BL_SELECT          BUTTON_PLAY
# define BL_QUIT            BUTTON_POWER
# define BL_UP_NAME         "Up"
# define BL_DOWN_NAME       "Scroll Down"
# define BL_SELECT_NAME     "PLAY"
# define BL_QUIT_NAME       "POWER"
# define BOOTBACKUP_FILE    "/erosqnative-boot.bin"
#else
# error "Missing keymap!"
#endif

/*
 * GUI stuff
 */

void clearscreen(void);
void putversion(void);
void putcenter_y(int y, const char* msg);
void putcenter_line(int line, const char* msg);
void splash2(long delay, const char* msg, const char* msg2);
void splash(long delay, const char* msg);
int get_button(int timeout);
void init_lcd(void);

void gui_shutdown(void);

#endif /* __X1000BOOTLOADER_H__ */
