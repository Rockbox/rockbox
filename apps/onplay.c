/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "lcd.h"
#include "dir.h"
#include "file.h"
#include "mpeg.h"
#include "menu.h"
#include "lang.h"
#include "playlist.h"
#include "button.h"
#include "kernel.h"
#include "keyboard.h"

static char* selected_file = NULL;
static bool reload_dir = false;

static bool queue_file(void)
{
    queue_add(selected_file);
    return false;
}

static bool delete_file(void)
{
    bool exit = false;

    lcd_clear_display();
    lcd_puts(0,0,str(LANG_REALLY_DELETE));
    lcd_puts_scroll(0,1,selected_file);

    while (!exit) {
        int btn = button_get(true);
        switch (btn) {
        case BUTTON_PLAY:
        case BUTTON_PLAY | BUTTON_REL:
            if (!remove(selected_file)) {
                reload_dir = true;
                lcd_clear_display();
                lcd_puts_scroll(0,0,selected_file);
                lcd_puts(0,1,str(LANG_DELETED));
                lcd_update();
                sleep(HZ);
                exit = true;
            }
            break;

        default:
            /* ignore button releases */
            if (!(btn & BUTTON_REL))
                exit = true;
            break;
        }
    }
    return false;
}

static bool rename_file(void)
{
    char newname[MAX_PATH];
    char* ptr = strrchr(selected_file, '/') + 1;
    int pathlen = (ptr - selected_file);
    strncpy(newname, selected_file, sizeof newname);
    if (!kbd_input(newname + pathlen, (sizeof newname)-pathlen)) {
        if (!strlen(selected_file+pathlen) ||
            (rename(selected_file, newname) < 0)) {
            lcd_clear_display();
            lcd_puts(0,0,str(LANG_RENAME));
            lcd_puts(0,1,str(LANG_FAILED));
            lcd_update();
            sleep(HZ*2);
        }
        else
            reload_dir = true;
    }

    return false;
}

extern int d_1;
extern int d_2;

static void xingupdate(int percent)
{
    char buf[32];

    snprintf(buf, 32, "%d%%", percent);
    lcd_puts(0, 3, buf);
    snprintf(buf, 32, "%x", d_1);
    lcd_puts(0, 4, buf);
    snprintf(buf, 32, "%x", d_2);
    lcd_puts(0, 5, buf);
    lcd_update();
}

static bool vbr_fix(void)
{
    char buf[32];
    unsigned long start_tick;
    unsigned long end_tick;

    lcd_clear_display();
    lcd_puts(0, 0, selected_file);
    lcd_update();

    start_tick = current_tick;
    mpeg_create_xing_header(selected_file, xingupdate);
    end_tick = current_tick;

    snprintf(buf, 32, "%d ticks", (int)(end_tick - start_tick));
    lcd_puts(0, 1, buf);
    snprintf(buf, 32, "%d seconds", (int)(end_tick - start_tick)/HZ);
    lcd_puts(0, 2, buf);
    lcd_update();

    return false;
}

int onplay(char* file, int attr)
{
    struct menu_items menu[5]; /* increase this if you add entries! */
    int m, i=0, result;

    selected_file = file;
    
    if (mpeg_status() & MPEG_STATUS_PLAY)
        menu[i++] = (struct menu_items) { str(LANG_QUEUE), queue_file };

    if (!(attr & ATTR_DIRECTORY))
        menu[i++] = (struct menu_items) { str(LANG_DELETE), delete_file };

    menu[i++] = (struct menu_items) { str(LANG_RENAME), rename_file };
    menu[i++] = (struct menu_items) { "VBRfix", vbr_fix };

    /* DIY menu handling, since we want to exit after selection */
    m = menu_init( menu, i );
    result = menu_show(m);
    if (result >= 0)
        menu[result].function();
    menu_exit(m);

    return reload_dir;
}
