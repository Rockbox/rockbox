/***************************************************************************
 *             __________               __   ___.                  
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \  
 *                     \/            \/     \/    \/            \/ 
 * $Id$
 *
 * Copyright (C) 2008 by Dave Chapman
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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "config.h"
#include "gwps.h"
#include "checkwps.h"

#define MIN(x,y) ((x) > (y) ? (y) : (x))

bool debug_wps = true;
int wps_verbose_level = 0;

int errno;

/* static endianness conversion */
#define SWAP_16(x) ((typeof(x))(unsigned short)(((unsigned short)(x) >> 8) | \
                                                ((unsigned short)(x) << 8)))

#define SWAP_32(x) ((typeof(x))(unsigned long)( ((unsigned long)(x) >> 24) | \
                                               (((unsigned long)(x) & 0xff0000ul) >> 8) | \
                                               (((unsigned long)(x) & 0xff00ul) << 8) | \
                                                ((unsigned long)(x) << 24)))
unsigned short letoh16(unsigned short x)
{
    unsigned short n = 0x1234;
    unsigned char* ch = (unsigned char*)&n;

    if (*ch == 0x34)
    {
        /* Little-endian */
        return x;
    } else {
        return SWAP_16(x);
    }
}

unsigned int htole32(unsigned int x)
{
    unsigned short n = 0x1234;
    unsigned char* ch = (unsigned char*)&n;

    if (*ch == 0x34)
    {
        /* Little-endian */
        return x;
    } else {
        return SWAP_32(x);
    }
}

int read_line(int fd, char* buffer, int buffer_size)
{
    int count = 0;
    int num_read = 0;
    
    errno = 0;

    while (count < buffer_size)
    {
        unsigned char c;

        if (1 != read(fd, &c, 1))
            break;
        
        num_read++;
            
        if ( c == '\n' )
            break;

        if ( c == '\r' )
            continue;

        buffer[count++] = c;
    }

    buffer[MIN(count, buffer_size - 1)] = 0;

    return errno ? -1 : num_read;
}

bool load_wps_backdrop(const char* filename)
{
    return true;
}

bool load_remote_wps_backdrop(const char* filename)
{
    return true;
}

static char pluginbuf[PLUGIN_BUFFER_SIZE];

static unsigned dummy_func2(void)
{
    return 0;
}

void* plugin_get_buffer(size_t *buffer_size)
{
    *buffer_size = PLUGIN_BUFFER_SIZE;
    return pluginbuf;
}
struct user_settings global_settings = {
    .statusbar = true,
#ifdef HAVE_LCD_COLOR
    .bg_color = LCD_DEFAULT_BG,
    .fg_color = LCD_DEFAULT_FG,
#endif
};
    
int getwidth(void) { return LCD_WIDTH; }
int getheight(void) { return LCD_HEIGHT; }
#ifdef HAVE_REMOTE_LCD
int remote_getwidth(void) { return LCD_REMOTE_WIDTH; }
int remote_getheight(void) { return LCD_REMOTE_HEIGHT; }
#endif

struct screen screens[NB_SCREENS] =
{
    {
        .screen_type=SCREEN_MAIN,
        .lcdwidth=LCD_WIDTH,
        .lcdheight=LCD_HEIGHT,
        .depth=LCD_DEPTH,
#ifdef HAVE_LCD_COLOR
        .is_color=true,
#else
        .is_color=false,
#endif
        .getwidth = getwidth,
        .getheight = getheight,
#if LCD_DEPTH > 1
        .get_foreground=dummy_func2,
        .get_background=dummy_func2,
#endif
    },
#ifdef HAVE_REMOTE_LCD
    {
        .screen_type=SCREEN_REMOTE,
        .lcdwidth=LCD_REMOTE_WIDTH,
        .lcdheight=LCD_REMOTE_HEIGHT,
        .depth=LCD_REMOTE_DEPTH,
        .is_color=false,/* No color remotes yet */
        .getwidth = remote_getwidth,
        .getheight = remote_getheight,
#if LCD_REMOTE_DEPTH > 1
        .get_foreground=dummy_func2,
        .get_background=dummy_func2,
#endif
    }
#endif
};

#ifdef HAVE_LCD_BITMAP
void screen_clear_area(struct screen * display, int xstart, int ystart,
                       int width, int height)
{
    display->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    display->fillrect(xstart, ystart, width, height);
    display->set_drawmode(DRMODE_SOLID);
}
#endif


int main(int argc, char **argv)
{
    int res;
    int fd;
    int filearg = 1;

    struct wps_data wps;

    if (argc < 2) {
        printf("Usage: checkwps [OPTIONS] filename.wps\n");
        printf("\nOPTIONS:\n");
        printf("\t-v\tverbose\n");
        printf("\t-vv\tmore verbose\n");
        printf("\t-vvv\tvery verbose\n");
        return 1;
    }

    if (argv[1][0] == '-') {
        filearg++;
        int i = 1;
        while (argv[1][i] && argv[1][i] == 'v') {
            i++;
            wps_verbose_level++;
        }
    }

    fd = open(argv[filearg], O_RDONLY);
    if (fd < 0) {
      printf("Failed to open %s\n",argv[1]);
      return 2;
    }
    close(fd);

    res = wps_data_load(&wps, &screens[0], argv[filearg], true);

    if (!res) {
      printf("WPS parsing failure\n");
      return 3;
    }

    printf("WPS parsed OK\n");
    return 0;
}

