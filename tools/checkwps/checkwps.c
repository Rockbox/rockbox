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
#include <string.h>
#include "config.h"
#include "checkwps.h"
#include "resize.h"
#include "wps.h"
#include "wps_internals.h"
#include "settings.h"

bool debug_wps = true;
int wps_verbose_level = 0;

int errno;

const struct settings_list *settings;
const int nb_settings = 0;

/* static endianness conversion */
#define SWAP_16(x) ((typeof(x))(unsigned short)(((unsigned short)(x) >> 8) | \
                                                ((unsigned short)(x) << 8)))

#define SWAP_32(x) ((typeof(x))(unsigned long)( ((unsigned long)(x) >> 24) | \
                                               (((unsigned long)(x) & 0xff0000ul) >> 8) | \
                                               (((unsigned long)(x) & 0xff00ul) << 8) | \
                                                ((unsigned long)(x) << 24)))

#ifndef letoh16
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
#endif

#ifndef letoh32
unsigned short letoh32(unsigned short x)
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
#endif

#ifndef htole32
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
#endif

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

int recalc_dimension(struct dim *dst, struct dim *src)
{
    return 0;
}

int resize_on_load(struct bitmap *bm, bool dither,
                   struct dim *src, struct rowset *tmp_row,
                   unsigned char *buf, unsigned int len,
                   const struct custom_format *cformat,
                   IF_PIX_FMT(int format_index,)
                   struct img_part* (*store_part)(void *args),
                   void *args)
{
    return 0;
}

int audio_status(void)
{
    return 0;
}

struct mp3entry* audio_current_track(void)
{
    return NULL;
}

void audio_stop(void)
{
}

void audio_play(long offset)
{
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
        .backdrop_load=backdrop_load,
    },
#ifdef HAVE_REMOTE_LCD
    {
        .screen_type=SCREEN_REMOTE,
        .lcdwidth=LCD_REMOTE_WIDTH,
        .lcdheight=LCD_REMOTE_HEIGHT,
        .depth=LCD_REMOTE_DEPTH,
        .is_color=false,/* No color remotes yet */
        .getwidth=remote_getwidth,
        .getheight=remote_getheight,
#if LCD_REMOTE_DEPTH > 1
        .get_foreground=dummy_func2,
        .get_background=dummy_func2,
#endif
        .backdrop_load=backdrop_load,
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

/* From skin_display.c */
void skin_data_init(struct wps_data *wps_data)
{
#ifdef HAVE_LCD_BITMAP
    wps_data->wps_sb_tag = false;
    wps_data->show_sb_on_wps = false;
    wps_data->peak_meter_enabled = false;
    wps_data->images = NULL;
    wps_data->progressbars = NULL;
    /* progress bars */
#else /* HAVE_LCD_CHARCELLS */
    int i;
    for (i = 0; i < 8; i++)
    {
        wps_data->wps_progress_pat[i] = 0;
    }
    wps_data->full_line_progressbar = false;
#endif
    wps_data->button_time_volume = 0;
    wps_data->wps_loaded = false;
}

#ifdef HAVE_LCD_BITMAP
struct gui_img* find_image(char label, struct wps_data *data)
{
    struct skin_token_list *list = data->images;
    while (list)
    {
        struct gui_img *img = (struct gui_img *)list->token->value.data;
        if (img->label == label)
            return img;
        list = list->next;
    }
    return NULL;
}   
#endif 

struct skin_viewport* find_viewport(char label, struct wps_data *data)
{
    struct skin_token_list *list = data->viewports;
    while (list)
    {
        struct skin_viewport *vp = (struct skin_viewport *)list->token->value.data;
        if (vp->label == label)
            return vp;
        list = list->next;
    }
    return NULL;
}

int skin_last_token_index(struct wps_data *data, int line, int subline)
{
    int first_subline_idx = data->lines[line].first_subline_idx;
    int idx = first_subline_idx + subline;
    if (idx < data->num_sublines - 1)
    {
        /* This subline ends where the next begins */
        return data->sublines[idx+1].first_token_idx - 1;
    }
    else
    {
        /* The last subline goes to the end */
        return data->num_tokens - 1;
    }
}

/* From viewport.c & misc.h */
#define LIST_VALUE_PARSED(setvals, position) ((setvals) & BIT_N(position))

/*some short cuts for fg/bg/line selector handling */
#ifdef HAVE_LCD_COLOR
#define LINE_SEL_FROM_SETTINGS(vp) \
    do { \
        vp->lss_pattern = global_settings.lss_color; \
        vp->lse_pattern = global_settings.lse_color; \
        vp->lst_pattern = global_settings.lst_color; \
    } while (0)
#define FG_FALLBACK global_settings.fg_color
#define BG_FALLBACK global_settings.bg_color
#else
/* mono/greyscale doesn't have most of the above */
#define LINE_SEL_FROM_SETTINGS(vp)
#define FG_FALLBACK LCD_DEFAULT_FG
#define BG_FALLBACK LCD_DEFAULT_BG
#endif

#ifdef HAVE_LCD_COLOR
#define ARG_STRING(_depth) ((_depth) == 2 ? "dddddgg":"dddddcc")
#else
#define ARG_STRING(_depth) "dddddgg"
#endif

extern const char* parse_list(const char *fmt, uint32_t *set_vals,
                       const char sep, const char* str, ...);

const char* viewport_parse_viewport(struct viewport *vp,
                                    enum screen_type screen,
                                    const char *bufptr,
                                    const char separator)
{
    /* parse the list to the viewport struct */
    const char *ptr = bufptr;
    int depth;
    uint32_t set = 0;

    enum {
        PL_X = 0,
        PL_Y,
        PL_WIDTH,
        PL_HEIGHT,
        PL_FONT,
        PL_FG,
        PL_BG,
    };

    /* Work out the depth of this display */
    depth = screens[screen].depth;
#if (LCD_DEPTH == 1) || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH == 1)
    if (depth == 1)
    {
        if (!(ptr = parse_list("ddddd", &set, separator, ptr,
                    &vp->x, &vp->y, &vp->width, &vp->height, &vp->font)))
            return NULL;
    }
    else
#endif
#if (LCD_DEPTH > 1) || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)
    if (depth >= 2)
    {
        if (!(ptr = parse_list(ARG_STRING(depth), &set, separator, ptr,
                    &vp->x, &vp->y, &vp->width, &vp->height, &vp->font,
                    &vp->fg_pattern,&vp->bg_pattern)))
            return NULL;
    }
    else
#endif
    {}
#undef ARG_STRING

    /* X and Y *must* be set */
    if (!LIST_VALUE_PARSED(set, PL_X) || !LIST_VALUE_PARSED(set, PL_Y))
        return NULL;
    
    /* fix defaults */
    if (!LIST_VALUE_PARSED(set, PL_WIDTH))
        vp->width = screens[screen].lcdwidth - vp->x;
    if (!LIST_VALUE_PARSED(set, PL_HEIGHT))
        vp->height = screens[screen].lcdheight - vp->y;

#if (LCD_DEPTH > 1) || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)
    if (!LIST_VALUE_PARSED(set, PL_FG))
        vp->fg_pattern = FG_FALLBACK;
    if (!LIST_VALUE_PARSED(set, PL_BG))
        vp->bg_pattern = BG_FALLBACK;
#endif /* LCD_DEPTH > 1 || LCD_REMOTE_DEPTH > 1 */

    LINE_SEL_FROM_SETTINGS(vp);

    /* Validate the viewport dimensions - we know that the numbers are
       non-negative integers, ignore bars and assume the viewport takes them
       * into account */
    if ((vp->x >= screens[screen].lcdwidth) ||
        ((vp->x + vp->width) > screens[screen].lcdwidth) ||
        (vp->y >= screens[screen].lcdheight) ||
        ((vp->y + vp->height) > screens[screen].lcdheight))
    {
        return NULL;
    }

    /* Default to using the user font if the font was an invalid number or '-'*/
    if (((vp->font != FONT_SYSFIXED) && (vp->font != FONT_UI))
            || !LIST_VALUE_PARSED(set, PL_FONT)
            )
        vp->font = FONT_UI;

    /* Set the defaults for fields not user-specified */
    vp->drawmode = DRMODE_SOLID;

    return ptr;
}

int main(int argc, char **argv)
{
    int res;
    int filearg = 1;

    struct wps_data wps;
    struct screen* wps_screen = &screens[SCREEN_MAIN];

    /* No arguments -> print the help text
     * Also print the help text upon -h or --help */
    if( (argc < 2) ||
        strcmp(argv[1],"-h") == 0 ||
        strcmp(argv[1],"--help") == 0 )
    {
        printf("Usage: checkwps [OPTIONS] filename.wps [filename2.wps]...\n");
        printf("\nOPTIONS:\n");
        printf("\t-v\t\tverbose\n");
        printf("\t-vv\t\tmore verbose\n");
        printf("\t-vvv\t\tvery verbose\n");
        printf("\t-h,\t--help\tshow this message\n");
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

    skin_buffer_init();

    /* Go through every wps that was thrown at us, error out at the first
     * flawed wps */
    while (argv[filearg]) {
        printf("Checking %s...\n", argv[filearg]);
#ifdef HAVE_REMOTE_LCD
        if(strcmp(&argv[filearg][strlen(argv[filearg])-4], "rwps") == 0)
        {
            wps_screen = &screens[SCREEN_REMOTE];
            wps.remote_wps = true;
        }
        else
        {
            wps_screen = &screens[SCREEN_MAIN];
            wps.remote_wps = false;
        }
#endif

        res = skin_data_load(&wps, wps_screen, argv[filearg], true);

        if (!res) {
            printf("WPS parsing failure\n");
            return 3;
        }

        printf("WPS parsed OK\n\n");
        filearg++;
    }
    return 0;
}
