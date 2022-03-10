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
#include "lcd.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

struct uimage_header;

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

struct bl_listitem {
    struct bl_list* list;

    int index;
    int x, y, width, height;
};

struct bl_list {
    struct viewport* vp;

    int num_items;
    int selected_item;
    int top_item;
    int item_height;

    void(*draw_item)(const struct bl_listitem* item);
};

void clearscreen(void);
void putversion(void);
void putcenter_y(int y, const char* msg);
void putcenter_line(int line, const char* msg);
void splash2(long delay, const char* msg, const char* msg2);
void splash(long delay, const char* msg);
int get_button(int timeout);
void init_lcd(void);

void gui_shutdown(void);

void gui_list_init(struct bl_list* list, struct viewport* vp);
void gui_list_draw(struct bl_list* list);
void gui_list_select(struct bl_list* list, int item_index);
void gui_list_scroll(struct bl_list* list, int delta);

/*
 * Installer
 */

void bootloader_install(void);
void bootloader_backup(void);
void bootloader_restore(void);

/*
 * Boot code
 */

void boot_rockbox(void);
void shutdown(void);
void reboot(void);

/*
 * Misc
 */

enum {
    DISK_PRESENT = 0,
    DISK_ABSENT = -1,
    DISK_CANCELED = -2,
};

int check_disk(bool wait);
void usb_mode(void);

int load_rockbox(const char* filename, size_t* sizep);
int load_uimage_file(const char* filename,
                     struct uimage_header* uh, size_t* sizep);
int load_uimage_flash(uint32_t addr, uint32_t length,
                      struct uimage_header* uh, size_t* sizep);

void recovery_menu(void) __attribute__((noreturn));

#endif /* __X1000BOOTLOADER_H__ */
