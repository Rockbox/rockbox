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
#include "lang.h"
#include "keyboard.h"

#ifdef HAVE_LCD_BITMAP
#include "widgets.h"
#endif

#ifdef SIMULATOR
  #include <debug.h>
  #ifdef WIN32
    #include "plugin-win32.h"
    #define PREFIX(_x_) _x_
  #else
    #include <dlfcn.h>
    #define PREFIX(_x_) x11_ ## _x_
  #endif
#else
#define PREFIX(_x_) _x_
#endif

static int plugin_test(int api_version, int model, int memsize);

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
    progressbar,
    slidebar,
    scrollbar,
#ifndef SIMULATOR
    lcd_roll,
#endif
#endif

    /* button */
    button_get,
    button_get_w_tmo,

    /* file */
    PREFIX(open),
    PREFIX(close),
    read,
    lseek,
    PREFIX(creat),
    write,
    remove,
    rename,
    ftruncate,
    PREFIX(filesize),
    fprintf,
    read_line,

    /* dir */
    PREFIX(opendir),
    PREFIX(closedir),
    PREFIX(readdir),

    /* kernel */
    PREFIX(sleep),
    usb_screen,
    &current_tick,

    /* strings and memory */
    snprintf,
    strcpy,
    strlen,
    memset,
    memcpy,

    /* sound */
#ifndef SIMULATOR
#ifdef HAVE_MAS3587F
    mas_codec_readreg,
#endif
#endif
    
    /* misc */
    srand,
    rand,
    splash,
    qsort,
    kbd_input,
};

int plugin_load(char* plugin, void* parameter)
{
    enum plugin_status (*plugin_start)(struct plugin_api* api, void* param);
    int rc;
    char buf[64];
#ifdef SIMULATOR
    void* pd;
    char path[256];
#else
    extern unsigned char pluginbuf[];
    int fd;
#endif

    lcd_clear_display();
#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(0,0);
    lcd_update();
#endif
#ifdef SIMULATOR
#ifdef WIN32
    snprintf(path, sizeof path, "%s", plugin);
#else
    snprintf(path, sizeof path, "archos%s", plugin);
#endif
    pd = dlopen(path, RTLD_NOW);
    if (!pd) {
        snprintf(buf, sizeof buf, "Can't open %s", plugin);
        splash(HZ*2, 0, true, buf);
        DEBUGF("dlopen(%s): %s\n",path,dlerror());
        dlclose(pd);
        return -1;
    }

    plugin_start = dlsym(pd, "plugin_start");
    if (!plugin_start) {
        plugin_start = dlsym(pd, "_plugin_start");
        if (!plugin_start) {
            splash(HZ*2, 0, true, "Can't find entry point");
            dlclose(pd);
            return -1;
        }
    }
#else
    fd = open(plugin, O_RDONLY);
    if (fd < 0) {
        snprintf(buf, sizeof buf, str(LANG_PLUGIN_CANT_OPEN), plugin);
        splash(HZ*2, 0, true, buf);
        return fd;
    }

    plugin_start = (void*)&pluginbuf;
    rc = read(fd, plugin_start, 0x8000);
    close(fd);
    if (rc < 0) {
        /* read error */
        snprintf(buf, sizeof buf, str(LANG_READ_FAILED), plugin);
        splash(HZ*2, 0, true, buf);
        return -1;
    }
    if (rc == 0) {
        /* loaded a 0-byte plugin, implying it's not for this model */
        splash(HZ*2, 0, true, str(LANG_PLUGIN_WRONG_MODEL));
        return -1;
    }
#endif

    rc = plugin_start(&rockbox_api, parameter);
    switch (rc) {
        case PLUGIN_OK:
            break;

        case PLUGIN_USB_CONNECTED:
            return PLUGIN_USB_CONNECTED;

        case PLUGIN_WRONG_API_VERSION:
            splash(HZ*2, 0, true, str(LANG_PLUGIN_WRONG_VERSION));
            break;

        case PLUGIN_WRONG_MODEL:
            splash(HZ*2, 0, true, str(LANG_PLUGIN_WRONG_MODEL));
            break;

        default:
            splash(HZ*2, 0, true, str(LANG_PLUGIN_ERROR));
            break;
    }

#ifdef SIMULATOR
    dlclose(pd);
#endif
    
    return PLUGIN_OK;
}

int plugin_test(int api_version, int model, int memsize)
{
    if (api_version != PLUGIN_API_VERSION)
        return PLUGIN_WRONG_API_VERSION;

    if (model != MODEL)
        return PLUGIN_WRONG_MODEL;

    if (memsize != MEM)
        return PLUGIN_WRONG_MODEL;
    
    return PLUGIN_OK;
}
