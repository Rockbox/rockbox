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

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "button.h"
#include "lcd.h"
#include "dir.h"
#include "file.h"
#include "kernel.h"
#include "sprintf.h"
#include "screens.h"
#include "misc.h"
#include "mas.h"
#include "plugin.h"

#include <windows.h>

static int plugin_test(int api_version, int model);

static struct plugin_api rockbox_api = {
    PLUGIN_API_VERSION,

    plugin_test,
    
    /* lcd */
    lcd_clear_display,
    lcd_puts,
    lcd_puts_scroll,
    lcd_stop_scroll,
#ifdef HAVE_LCD_CHARCELLS
    lcd_define_pattern,
#else
    lcd_putsxy,
    lcd_bitmap,
    lcd_drawline,
    lcd_clearline,
    lcd_drawpixel,
    lcd_clearpixel,
    lcd_setfont,
    lcd_clearrect,
    lcd_fillrect,
    lcd_drawrect,
    lcd_invertrect,
    lcd_getstringsize,
    lcd_update,
    lcd_update_rect,
#endif

    /* button */
    button_get,
    button_get_w_tmo,

    /* file */
    open,
    close,
    read,
    lseek,
    creat,
    write,
    remove,
    rename,
    NULL, /* ftruncate */
    win32_filesize,
    fprintf,
    read_line,

    /* dir */
    opendir,
    closedir,
    readdir,

    /* kernel */
    sleep,
    usb_screen,
    &current_tick,

    /* strings and memory */
    snprintf,
    strcpy,
    strlen,
    memset,
    memcpy,
    
    /* misc */
    srand,
    rand,
    splash,
    qsort,
};

typedef enum plugin_status (*plugin_fn)(struct plugin_api* api, void* param);

int plugin_load(char* plugin, void* parameter)
{
    plugin_fn plugin_start;
    int rc;
    char buf[64];
    void* pd;

    lcd_clear_display();
#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(0,0);
    lcd_update();
#endif

    pd = LoadLibrary(plugin);
    if (!pd) {
        snprintf(buf, sizeof buf, "Can't open %s", plugin);
        splash(HZ*2, 0, true, buf);
        return -1;
    }

    plugin_start = (plugin_fn)GetProcAddress(pd, "plugin_start");
    if (!plugin_start) {
        splash(HZ*2, 0, true, "Can't find entry point");
        FreeLibrary(pd);
        return -1;
    }

    rc = plugin_start(&rockbox_api, parameter);

    FreeLibrary(pd);
    
    return rc;
}

int plugin_test(int api_version, int model)
{
    if (api_version != PLUGIN_API_VERSION)
        return PLUGIN_WRONG_API_VERSION;

    if (model != MODEL)
        return PLUGIN_WRONG_MODEL;

    return PLUGIN_OK;
}
