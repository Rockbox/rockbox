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
#ifndef _PLUGIN_H_
#define _PLUGIN_H_

/* instruct simulator code to not redefine any symbols when compiling plugins.
   (the PLUGIN macro is defined in apps/plugins/Makefile) */
#ifdef PLUGIN
#define NO_REDEFINES_PLEASE
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "dir.h"
#include "kernel.h"
#include "button.h"
#include "font.h"
#include "system.h"
#include "lcd.h"

/* increase this every time the api struct changes */
#define PLUGIN_API_VERSION 3

/* plugin return codes */
enum plugin_status {
    PLUGIN_OK = 0,
    PLUGIN_USB_CONNECTED,

    PLUGIN_WRONG_API_VERSION = -1,
    PLUGIN_WRONG_MODEL = -2,
    PLUGIN_ERROR = -3,
};

/* different (incompatible) plugin models */
enum model {
    PLAYER,
    RECORDER
};

#ifdef HAVE_LCD_CHARCELLS
#define MODEL PLAYER
#else
#define MODEL RECORDER
#endif

/* compatibility test macro */
#define TEST_PLUGIN_API(_api_) \
do { \
 int _rc_ = _api_->plugin_test(PLUGIN_API_VERSION, MODEL); \
 if (_rc_<0) \
     return _rc_; \
} while(0)

struct plugin_api {
    /* these two fields must always be first, to ensure
       TEST_PLUGIN_API will always work */
    int version;
    int (*plugin_test)(int api_version, int model);

    /* lcd */
    void (*lcd_clear_display)(void);
    void (*lcd_puts)(int x, int y, unsigned char *string);
    void (*lcd_puts_scroll)(int x, int y, unsigned char* string);
    void (*lcd_stop_scroll)(void);
#ifdef HAVE_LCD_CHARCELLS
    void (*lcd_define_pattern)(int which,char *pattern);
#else
    void (*lcd_putsxy)(int x, int y, unsigned char *string);
    void (*lcd_bitmap)(unsigned char *src, int x, int y,
                       int nx, int ny, bool clear);
    void (*lcd_drawline)(int x1, int y1, int x2, int y2);
    void (*lcd_clearline)(int x1, int y1, int x2, int y2);
    void (*lcd_drawpixel)(int x, int y);
    void (*lcd_clearpixel)(int x, int y);
    void (*lcd_setfont)(int font);
    void (*lcd_clearrect)(int x, int y, int nx, int ny);
    void (*lcd_fillrect)(int x, int y, int nx, int ny);
    void (*lcd_drawrect)(int x, int y, int nx, int ny);
    void (*lcd_invertrect)(int x, int y, int nx, int ny);
    int  (*lcd_getstringsize)(unsigned char *str, int *w, int *h);
    void (*lcd_update)(void);
    void (*lcd_update_rect)(int x, int y, int width, int height);
    void (*progressbar)(int x, int y, int width, int height,
                        int percent, int direction);
    void (*slidebar)(int x, int y, int width, int height,
                     int percent, int direction);
    void (*scrollbar)(int x, int y, int width, int height, int items,
                      int min_shown, int max_shown, int orientation);
#ifndef SIMULATOR
    void (*lcd_roll)(int pixels);
#endif
#endif

    /* button */
    int (*button_get)(bool block);
    int (*button_get_w_tmo)(int ticks);

    /* file */
    int (*open)(const char* pathname, int flags);
    int (*close)(int fd);
    int (*read)(int fd, void* buf, int count);
    int (*lseek)(int fd, int offset, int whence);
    int (*creat)(const char *pathname, int mode);
    int (*write)(int fd, void* buf, int count);
    int (*remove)(const char* pathname);
    int (*rename)(const char* path, const char* newname);
    int (*ftruncate)(int fd, unsigned int size);
    int (*filesize)(int fd);
    int (*fprintf)(int fd, const char *fmt, ...);
    int (*read_line)(int fd, char* buffer, int buffer_size);
    
    /* dir */
    DIR* (*opendir)(char* name);
    int (*closedir)(DIR* dir);
    struct dirent* (*readdir)(DIR* dir);

    /* kernel */
    void (*sleep)(int ticks);
    void (*usb_screen)(void);
    long* current_tick;

    /* strings and memory */
    int    (*snprintf)(char *buf, size_t size, const char *fmt, ...);
    char*  (*strcpy)(char *dst, const char *src);
    size_t (*strlen)(const char *str);
    void*  (*memset)(void *dst, int c, size_t length);
    void*  (*memcpy)(void *out, const void *in, size_t n);

    /* sound */
#ifndef SIMULATOR
#ifdef HAVE_MAS3587F
    int (*mas_codec_readreg)(int reg);
#endif
#endif

    /* misc */
    void (*srand)(unsigned int seed);
    int  (*rand)(void);
    void (*splash)(int ticks, int keymask, bool center, char *fmt, ...);
    void (*qsort)(void *base, size_t nmemb, size_t size,
                  int(*compar)(const void *, const void *));
    int (*kbd_input)(char* buffer, int buflen);
};

/* defined by the plugin loader (plugin.c) */
int plugin_load(char* plugin, void* parameter);

/* defined by the plugin */
enum plugin_status plugin_start(struct plugin_api* rockbox, void* parameter)
    __attribute__ ((section (".entry")));

#endif
