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
# define BL_SCREENSHOT      BUTTON_MENU
# define BL_UP_NAME         "VOL+"
# define BL_DOWN_NAME       "VOL-"
# define BL_SELECT_NAME     "PLAY"
# define BL_QUIT_NAME       "POWER"
# define BL_SCREENSHOT_NAME "MENU"
# define BOOTBACKUP_FILE    "/fiiom3k-boot.bin"
# define OF_PLAYER_NAME     "FiiO player"
# define OF_PLAYER_ADDR     0x20000
# define OF_PLAYER_LENGTH   (4 * 1024 * 1024)
/* WARNING: The length of kernel arguments cannot exceed 99 bytes on the M3K
 * due to an Ingenic kernel bug: plat_mem_setup() calls ddr_param_change() and
 * that function tries to copy the command line to an 100-char buffer without
 * any bounds checking. Overflowing the buffer typically leads to disaster.
 * It seems ddr_param_change() is not present on all Ingenic kernels and the
 * bug may not affect the Q1. */
# define OF_PLAYER_ARGS     OF_RECOVERY_ARGS \
    " init=/linuxrc ubi.mtd=3 root=ubi0:rootfs ubi.mtd=4 rootfstype=ubifs rw"
# define OF_PLAYER_BTN      BUTTON_PLAY
# define OF_RECOVERY_NAME   "FiiO recovery"
# define OF_RECOVERY_ADDR   0x420000
# define OF_RECOVERY_LENGTH (5 * 1024 * 1024)
# define OF_RECOVERY_ARGS \
    "mem=64M console=ttyS2"
# define OF_RECOVERY_BTN    (BUTTON_PLAY|BUTTON_VOL_UP)
#elif defined(SHANLING_Q1)
# define BL_RECOVERY        BUTTON_NEXT
# define BL_UP              BUTTON_PREV
# define BL_DOWN            BUTTON_NEXT
# define BL_SELECT          BUTTON_PLAY
# define BL_QUIT            BUTTON_POWER
# define BL_SCREENSHOT      BUTTON_TOPLEFT
# define BL_UP_NAME         "PREV"
# define BL_DOWN_NAME       "NEXT"
# define BL_SELECT_NAME     "PLAY"
# define BL_QUIT_NAME       "POWER"
# define BL_SCREENSHOT_NAME "TOPLEFT"
# define BOOTBACKUP_FILE    "/shanlingq1-boot.bin"
# define OF_PLAYER_NAME     "Shanling player"
# define OF_PLAYER_ADDR     0x140000
# define OF_PLAYER_LENGTH   (8 * 1024 * 1024)
# define OF_PLAYER_ARGS     OF_RECOVERY_ARGS \
    " init=/linuxrc ubi.mtd=5 root=ubi0:rootfs ubi.mtd=6 rootfstype=ubifs rw"
# define OF_PLAYER_BTN      BUTTON_PREV
# define OF_RECOVERY_NAME   "Shanling recovery"
# define OF_RECOVERY_ADDR   0x940000
# define OF_RECOVERY_LENGTH (10 * 1024 * 1024)
# define OF_RECOVERY_ARGS \
    "mem=64M@0x0 no_console_suspend console=ttyS2,115200n8 lpj=5009408 ip=off"
# define OF_RECOVERY_BTN    (BUTTON_PREV|BUTTON_NEXT)
#elif defined(EROS_QN)
# define BL_RECOVERY        BUTTON_VOL_UP
# define BL_UP              BUTTON_SCROLL_BACK
# define BL_DOWN            BUTTON_SCROLL_FWD
# define BL_SELECT          BUTTON_PLAY
# define BL_QUIT            BUTTON_POWER
# define BL_SCREENSHOT      BUTTON_MENU
# define BL_UP_NAME         "Up"
# define BL_DOWN_NAME       "Scroll Down"
# define BL_SELECT_NAME     "PLAY"
# define BL_QUIT_NAME       "POWER"
# define BL_SCREENSHOT_NAME "MENU"
# define BOOTBACKUP_FILE    "/erosqnative-boot.bin"
# define OF_PLAYER_NAME     "Aigo Player"
# define OF_PLAYER_ADDR     0x300000
# define OF_PLAYER_LENGTH   (6 * 1024 * 1024)
# define OF_PLAYER_ARGS     OF_RECOVERY_ARGS \
    " init=/linuxrc ubi.mtd=4 root=ubi0:rootfs ubi.mtd=5 rootfstype=ubifs \
sn_no=00000000000000000000000000000000 bt_mac=xxxxxxxxxxxx wifi_mac=xxxxxxxxxxxx rw"
# define OF_PLAYER_BTN      BUTTON_PLAY
/* Note: OF Recovery boots, but is otherwise untested. */
//# define OF_RECOVERY_NAME   "Aigo Recovery"
//# define OF_RECOVERY_ADDR   0x900000
//# define OF_RECOVERY_LENGTH (7 * 1024 * 1024)
# define OF_RECOVERY_ARGS \
    "console=ttyS2,115200n8 mem=32M@0x0 no_console_suspend lpj=5009408 ip=off"
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
void splashf(long delay, const char* msg, ...);
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
void boot_of_player(void);
void boot_of_recovery(void);
void boot_linux(void);
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
void screenshot(void);
void screenshot_enable(void);

int load_rockbox(const char* filename, size_t* sizep);
int load_uimage_file(const char* filename,
                     struct uimage_header* uh, size_t* sizep);
int load_uimage_flash(uint32_t addr, uint32_t length,
                      struct uimage_header* uh, size_t* sizep);

int dump_flash(int fd, uint32_t addr, uint32_t length);
int dump_flash_file(const char* file, uint32_t addr, uint32_t length);
void dump_of_player(void);
void dump_of_recovery(void);
void dump_entire_flash(void);
void show_flash_info(void);

void recovery_menu(void) __attribute__((noreturn));

#endif /* __X1000BOOTLOADER_H__ */
