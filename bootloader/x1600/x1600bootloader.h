/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
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

#ifndef __X1600BOOTLOADER_H__
#define __X1600BOOTLOADER_H__

#include "config.h"
#include "button.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* POWER and NEXT use GPIOs; PLAY and the volume keys use the SADC ladder.
 * Initialise SADC before reading buttons. */

/* Keymap, chosen to fit the R1's five keys */
#define BL_RECOVERY         BUTTON_NEXT     /* also the BootROM USB strap */
#define BL_UP               BUTTON_VOL_UP
#define BL_DOWN             BUTTON_VOL_DOWN
#define BL_SELECT           BUTTON_PLAY
#define BL_QUIT             BUTTON_POWER
#define BL_UP_NAME          "VOL+"
#define BL_DOWN_NAME        "VOL-"
#define BL_SELECT_NAME      "PLAY"
#define BL_QUIT_NAME        "POWER"

/* No screenshot key: the X1000 bootloader binds one, the R1 has no spare key */

/* Original-firmware boot is not implemented. The vendor uses an A/B slot
 * flag in mtd5. */
#define BOOTBACKUP_FILE     "/hibyr1-boot.bin"

/* gui.c -- panel bring-up.  The screen primitives live in
 * bootloader/ingenic/gui-ingenic.c, shared with the X1000. */
void init_lcd(void);
void gui_shutdown(void);

/* utils.c -- buttons, prompts and card presence */
/* Returns SYS_USB_DISCONNECTED after USB mode; callers must redraw. */
int  get_recovery_button(int timeout);
void wait_btn_release(void);
int  wait_btn_press(void);
bool confirm(const char* what, const char* consequence);
bool card_available(void);

int  load_rockbox(const char* filename, size_t* sizep);

/* Optional host-preloaded image at 32 MiB, with its header one page below.
 * Read through KSEG1 because the host writes directly to memory. */
#define BL_PRELOAD_HDR    0xa1fff000u
#define BL_PRELOAD_IMAGE  0xa2000000u
#define BL_PRELOAD_MAGIC  0x5230424Fu   /* "R0BO" little-endian-ish tag */

/* boot.c -- load Rockbox and hand control to it */
int  bl_preloaded_size(void);
void boot_rockbox(void);

/* recovery.c -- install / backup / restore */
void recovery_menu(void);

#endif /* __X1600BOOTLOADER_H__ */
