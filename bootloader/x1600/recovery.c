/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
 * Copyright (C) 2021-2022 Aidan MacDonald
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 ****************************************************************************/

/* The recovery menu: install, backup and restore.  Mirrors
 * bootloader/x1000/recovery.c + install.c. */

#include "x1600bootloader.h"
#include "ingenic/gui-ingenic.h"
#include "kernel.h"
#include "installer-ingenic.h"
#include "lcd.h"
#include "font.h"

/* Minimal recovery menu.
 *
 * The X1000 splits this between recovery.c and install.c because its menu
 * nests; the R1 has three fixed actions, so they live here together.
 *
 * RESTORE matters as much as INSTALL: installing erases the vendor SPL, and
 * without a way to put it back the only recovery is BootROM USB on a host.
 */
enum {
    RA_INSTALL = 0,
    RA_BACKUP,
    RA_RESTORE,
    RA_EXIT,
    RA_COUNT,
};

static const char* const recovery_items[RA_COUNT] = {
    "Install bootloader",
    "Backup bootloader",
    "Restore bootloader",
    "Exit",
};

static void run_action(int which)
{
    int rc;

    wait_btn_release();
    if(!card_available())
        return;

    switch(which) {
    case RA_INSTALL:
        if(!confirm("Install bootloader", "ERASES the vendor SPL."))
            return;
        if(!card_available())
            return;
        rc = install_bootloader("/bootloader." BOOTFILE_EXT);
        break;
    case RA_BACKUP:
        /* Non-destructive: writes a file, touches no flash. */
        rc = backup_bootloader(BOOTBACKUP_FILE);
        break;
    case RA_RESTORE:
        if(!confirm("Restore bootloader", "OVERWRITES flash from file."))
            return;
        if(!card_available())
            return;
        rc = restore_bootloader(BOOTBACKUP_FILE);
        break;
    default:
        return;
    }

    do {
        wait_btn_release();
        lcd_clear_display();
        lcd_putsf(0, 0, "%s", recovery_items[which]);
        lcd_putsf(0, 1, "%s (%d)", installer_strerror(rc), rc);
        lcd_putsf(0, 3, "%s to continue", BL_QUIT_NAME);
        lcd_update();
    } while(wait_btn_press() != BL_QUIT);
}

static struct bl_list recmenu_list;

/* Same "=> item" idiom as bootloader/x1000/recovery.c, so both players draw
 * the recovery menu identically. */
static void recmenu_draw_item(const struct bl_listitem* item)
{
    const char* fmt = (item->index == item->list->selected_item)
                    ? "=> %s" : "   %s";
    lcd_putsxyf(item->x, item->y, fmt, recovery_items[item->index]);
}

void recovery_menu(void)
{
    struct viewport vp = {
        .x = 0, .y = SYSFONT_HEIGHT,
        .width = LCD_WIDTH,
        .height = LCD_HEIGHT - SYSFONT_HEIGHT*5,
    };

    init_lcd();
    lcd_init_viewport(&vp);

    struct bl_list* list = &recmenu_list;
    gui_list_init(list, &vp);
    list->draw_item = recmenu_draw_item;
    list->num_items = RA_COUNT;
    gui_list_select(list, 0);
    wait_btn_release();

    while(1) {
        clearscreen();
        putcenter_y(0, "Rockbox recovery menu");

        int ypos = LCD_HEIGHT - 4*SYSFONT_HEIGHT;
        put_help_line(ypos, 0, BL_DOWN_NAME "/" BL_UP_NAME, "move cursor");
        put_help_line(ypos, 1, BL_SELECT_NAME, "select item");
        put_help_line(ypos, 2, BL_QUIT_NAME, "exit");

        gui_list_draw(list);
        lcd_update();

        int btn = get_recovery_button(TIMEOUT_BLOCK);

        if(btn == BL_UP)
            gui_list_scroll(list, -1);
        else if(btn == BL_DOWN)
            gui_list_scroll(list, 1);
        else if(btn == BL_SELECT) {
            if(list->selected_item == RA_EXIT)
                return;
            run_action(list->selected_item);
        } else if(btn == BL_QUIT)
            return;
    }
}
