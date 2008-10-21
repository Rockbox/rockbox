/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* JPEG image viewer
* (This is a real mess if it has to be coded in one single C file)
*
* File scrolling addition (C) 2005 Alexander Spyridakis
* Copyright (C) 2004 Jörg Hohensohn aka [IDC]Dragon
* Heavily borrowed from the IJG implementation (C) Thomas G. Lane
* Small & fast downscaling IDCT (C) 2002 by Guido Vollbeding  JPEGclub.org
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

#include "plugin.h"
#include <lib/playback_control.h>
#include <lib/oldmenuapi.h>
#include <lib/helper.h>
#include <lib/configfile.h>

#include <lib/grey.h>
#include <lib/xlcd.h>

#include "jpeg.h"
#include "jpeg_decoder.h"

PLUGIN_HEADER

#ifdef HAVE_LCD_COLOR
#include "yuv2rgb.h"
#endif

/* different graphics libraries */
#if LCD_DEPTH < 8
#define USEGSLIB
GREY_INFO_STRUCT
#define MYLCD(fn) grey_ub_ ## fn
#define MYLCD_UPDATE()
#define MYXLCD(fn) grey_ub_ ## fn
#else
#define MYLCD(fn) rb->lcd_ ## fn
#define MYLCD_UPDATE() rb->lcd_update();
#define MYXLCD(fn) xlcd_ ## fn
#endif

#define MAX_X_SIZE LCD_WIDTH*8

/* Min memory allowing us to use the plugin buffer
 * and thus not stopping the music
 * *Very* rough estimation:
 * Max 10 000 dir entries * 4bytes/entry (char **) = 40000 bytes
 * + 20k code size = 60 000
 * + 50k min for jpeg = 120 000
 */
#define MIN_MEM 120000

/* Headings */
#define DIR_PREV  1
#define DIR_NEXT -1
#define DIR_NONE  0

#define PLUGIN_OTHER 10 /* State code for output with return. */

/******************************* Globals ***********************************/

const struct plugin_api* rb;
MEM_FUNCTION_WRAPPERS(rb);

static int slideshow_enabled = false;   /* run slideshow */
static int running_slideshow = false;   /* loading image because of slideshw */
#ifndef SIMULATOR
static int immediate_ata_off = false;   /* power down disk after loading */
#endif

#ifdef HAVE_LCD_COLOR
fb_data rgb_linebuf[LCD_WIDTH];  /* Line buffer for scrolling when
                                    DITHER_DIFFUSION is set                */
#endif


/* Persistent configuration */
#define JPEG_CONFIGFILE             "jpeg.cfg"
#define JPEG_SETTINGS_MINVERSION    1
#define JPEG_SETTINGS_VERSION       2

/* Slideshow times */
#define SS_MIN_TIMEOUT 1
#define SS_MAX_TIMEOUT 20
#define SS_DEFAULT_TIMEOUT 5

struct jpeg_settings
{
#ifdef HAVE_LCD_COLOR
    int colour_mode;
    int dither_mode;
#endif
    int ss_timeout;
};

static struct jpeg_settings jpeg_settings =
    { 
#ifdef HAVE_LCD_COLOR
      COLOURMODE_COLOUR, 
      DITHER_NONE,
#endif
      SS_DEFAULT_TIMEOUT
    };
static struct jpeg_settings old_settings;

static struct configdata jpeg_config[] =
{
#ifdef HAVE_LCD_COLOR
   { TYPE_ENUM, 0, COLOUR_NUM_MODES, &jpeg_settings.colour_mode,
     "Colour Mode", (char *[]){ "Colour", "Grayscale" }, NULL },
   { TYPE_ENUM, 0, DITHER_NUM_MODES, &jpeg_settings.dither_mode,
     "Dither Mode", (char *[]){ "None", "Ordered", "Diffusion" }, NULL },
#endif
   { TYPE_INT, SS_MIN_TIMEOUT, SS_MAX_TIMEOUT, &jpeg_settings.ss_timeout,
     "Slideshow Time", NULL, NULL},
};

#if LCD_DEPTH > 1
fb_data* old_backdrop;
#endif

/**************** begin Application ********************/


/************************* Types ***************************/

struct t_disp
{
#ifdef HAVE_LCD_COLOR
    unsigned char* bitmap[3]; /* Y, Cr, Cb */
    int csub_x, csub_y;
#else
    unsigned char* bitmap[1]; /* Y only */
#endif
    int width;
    int height;
    int stride;
    int x, y;
};

/************************* Globals ***************************/

/* decompressed image in the possible sizes (1,2,4,8), wasting the other */
struct t_disp disp[9];

/* my memory pool (from the mp3 buffer) */
char print[32]; /* use a common snprintf() buffer */
unsigned char* buf; /* up to here currently used by image(s) */

/* the remaining free part of the buffer for compressed+uncompressed images */
unsigned char* buf_images;

ssize_t buf_size, buf_images_size;
/* the root of the images, hereafter are decompresed ones */
unsigned char* buf_root;
int root_size;

int ds, ds_min, ds_max; /* downscaling and limits */
static struct jpeg jpg; /* too large for stack */

static struct tree_context *tree;

/* the current full file name */
static char np_file[MAX_PATH];
int curfile = 0, direction = DIR_NONE, entries = 0;

/* list of the jpeg files */
char **file_pt;
/* are we using the plugin buffer or the audio buffer? */
bool plug_buf = false;


/************************* Implementation ***************************/

/* support function for qsort() */
static int compare(const void* p1, const void* p2)
{
    return rb->strcasecmp(*((char **)p1), *((char **)p2));
}

bool jpg_ext(const char ext[])
{
    if(!ext)
        return false;
    if(!rb->strcasecmp(ext,".jpg") ||
       !rb->strcasecmp(ext,".jpe") ||
       !rb->strcasecmp(ext,".jpeg"))
            return true;
    else
            return false;
}

/*Read directory contents for scrolling. */
void get_pic_list(void)
{
    int i;
    long int str_len = 0;
    char *pname;
    tree = rb->tree_get_context();

#if PLUGIN_BUFFER_SIZE >= MIN_MEM
    file_pt = rb->plugin_get_buffer((size_t *)&buf_size);
#else
    file_pt = rb->plugin_get_audio_buffer((size_t *)&buf_size);
#endif

    for(i = 0; i < tree->filesindir; i++)
    {
        if(jpg_ext(rb->strrchr(&tree->name_buffer[str_len],'.')))
            file_pt[entries++] = &tree->name_buffer[str_len];

        str_len += rb->strlen(&tree->name_buffer[str_len]) + 1;
    }

    rb->qsort(file_pt, entries, sizeof(char**), compare);

    /* Remove path and leave only the name.*/
    pname = rb->strrchr(np_file,'/');
    pname++;

    /* Find Selected File. */
    for(i = 0; i < entries; i++)
        if(!rb->strcmp(file_pt[i], pname))
            curfile = i;
}

int change_filename(int direct)
{
    int count = 0;
    direction = direct;

    if(direct == DIR_PREV)
    {
        do
        {
            count++;
            if(curfile == 0)
                curfile = entries - 1;
            else
                curfile--;
        }while(file_pt[curfile] == '\0' && count < entries);
        /* we "erase" the file name if  we encounter
         * a non-supported file, so skip it now */
    }
    else /* DIR_NEXT/DIR_NONE */
    {
        do
        {
            count++;
            if(curfile == entries - 1)
                curfile = 0;
            else
                curfile++;
        }while(file_pt[curfile] == '\0' && count < entries);
    }

    if(count == entries && file_pt[curfile] == '\0')
    {
        rb->splash(HZ, "No supported files");
        return PLUGIN_ERROR;
    }
    if(rb->strlen(tree->currdir) > 1)
    {
        rb->strcpy(np_file, tree->currdir);
        rb->strcat(np_file, "/");
    }
    else
        rb->strcpy(np_file, tree->currdir);

    rb->strcat(np_file, file_pt[curfile]);

    return PLUGIN_OTHER;
}

/* switch off overlay, for handling SYS_ events */
void cleanup(void *parameter)
{
    (void)parameter;
#ifdef USEGSLIB
    grey_show(false);
#endif
}

#define VSCROLL (LCD_HEIGHT/8)
#define HSCROLL (LCD_WIDTH/10)

#define ZOOM_IN  100 /* return codes for below function */
#define ZOOM_OUT 101

#ifdef HAVE_LCD_COLOR
bool set_option_grayscale(void)
{
    bool gray = jpeg_settings.colour_mode == COLOURMODE_GRAY;
    rb->set_bool("Grayscale", &gray);
    jpeg_settings.colour_mode = gray ? COLOURMODE_GRAY : COLOURMODE_COLOUR;
    return false;
}

bool set_option_dithering(void)
{
    static const struct opt_items dithering[DITHER_NUM_MODES] = {
        [DITHER_NONE]      = { "Off",       -1 },
        [DITHER_ORDERED]   = { "Ordered",   -1 },
        [DITHER_DIFFUSION] = { "Diffusion", -1 },
    };

    rb->set_option("Dithering", &jpeg_settings.dither_mode, INT,
                   dithering, DITHER_NUM_MODES, NULL);
    return false;
}

static void display_options(void)
{
    static const struct menu_item items[] = {
        { "Grayscale", set_option_grayscale },
        { "Dithering", set_option_dithering },
    };

    int m = menu_init(rb, items, ARRAYLEN(items),
                          NULL, NULL, NULL, NULL);
    menu_run(m);
    menu_exit(m);
}
#endif /* HAVE_LCD_COLOR */

int show_menu(void) /* return 1 to quit */
{
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(old_backdrop);
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(rb->global_settings->fg_color);
    rb->lcd_set_background(rb->global_settings->bg_color);
#else
    rb->lcd_set_foreground(LCD_BLACK);
    rb->lcd_set_background(LCD_WHITE);
#endif
#endif
    int m;
    int result;

    enum menu_id
    {
        MIID_QUIT = 0,
        MIID_TOGGLE_SS_MODE,
        MIID_CHANGE_SS_MODE,
#if PLUGIN_BUFFER_SIZE >= MIN_MEM
        MIID_SHOW_PLAYBACK_MENU,
#endif
#ifdef HAVE_LCD_COLOR
        MIID_DISPLAY_OPTIONS,
#endif
        MIID_RETURN,
    };

    static const struct menu_item items[] = {
        [MIID_QUIT] =
            { "Quit", NULL },
        [MIID_TOGGLE_SS_MODE] =
            { "Toggle Slideshow Mode", NULL },
        [MIID_CHANGE_SS_MODE] =
            { "Change Slideshow Time", NULL },
#if PLUGIN_BUFFER_SIZE >= MIN_MEM
        [MIID_SHOW_PLAYBACK_MENU] =
            { "Show Playback Menu", NULL },
#endif
#ifdef HAVE_LCD_COLOR
        [MIID_DISPLAY_OPTIONS] =
            { "Display Options", NULL },
#endif
        [MIID_RETURN] =
            { "Return", NULL },
    };

    static const struct opt_items slideshow[2] = {
        { "Disable", -1 },
        { "Enable", -1 },
    };

    m = menu_init(rb, items, sizeof(items) / sizeof(*items),
                      NULL, NULL, NULL, NULL);
    result=menu_show(m);

    switch (result)
    {
        case MIID_QUIT:
            menu_exit(m);
            return 1;
            break;
        case MIID_TOGGLE_SS_MODE:
            rb->set_option("Toggle Slideshow", &slideshow_enabled, INT,
                           slideshow , 2, NULL);
            break;
        case MIID_CHANGE_SS_MODE:
            rb->set_int("Slideshow Time", "s", UNIT_SEC,
                        &jpeg_settings.ss_timeout, NULL, 1,
                        SS_MIN_TIMEOUT, SS_MAX_TIMEOUT, NULL);
            break;

#if PLUGIN_BUFFER_SIZE >= MIN_MEM
        case MIID_SHOW_PLAYBACK_MENU:
            if (plug_buf)
            {
                playback_control(rb, NULL);
            }
            else
            {
                rb->splash(HZ, "Cannot restart playback");
            }
            break;
#endif
#ifdef HAVE_LCD_COLOR
        case MIID_DISPLAY_OPTIONS:
            display_options();
            break;
#endif
        case MIID_RETURN:
            break;
    }

#if !defined(SIMULATOR) && defined(HAVE_DISK_STORAGE)
    /* change ata spindown time based on slideshow time setting */
    immediate_ata_off = false;
    rb->ata_spindown(rb->global_settings->disk_spindown);

    if (slideshow_enabled)
    {
        if(jpeg_settings.ss_timeout < 10)
        {
            /* slideshow times < 10s keep disk spinning */
            rb->ata_spindown(0);
        }
        else if (!rb->mp3_is_playing())
        {
            /* slideshow times > 10s and not playing: ata_off after load */
            immediate_ata_off = true;
        }
    }
#endif
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(LCD_BLACK);
#endif
    rb->lcd_clear_display();
    menu_exit(m);
    return 0;
}
/* interactively scroll around the image */
int scroll_bmp(struct t_disp* pdisp)
{
    int lastbutton = 0;

    while (true)
    {
        int button;
        int move;

        if (slideshow_enabled)
            button = rb->button_get_w_tmo(jpeg_settings.ss_timeout * HZ);
        else button = rb->button_get(true);

        running_slideshow = false;

        switch(button)
        {
        case JPEG_LEFT:
            if (!(ds < ds_max) && entries > 0 && jpg.x_size <= MAX_X_SIZE)
                return change_filename(DIR_PREV);
        case JPEG_LEFT | BUTTON_REPEAT:
            move = MIN(HSCROLL, pdisp->x);
            if (move > 0)
            {
                MYXLCD(scroll_right)(move); /* scroll right */
                pdisp->x -= move;
#ifdef HAVE_LCD_COLOR
                yuv_bitmap_part(
                    pdisp->bitmap, pdisp->csub_x, pdisp->csub_y,
                    pdisp->x, pdisp->y, pdisp->stride,
                    0, MAX(0, (LCD_HEIGHT-pdisp->height)/2), /* x, y */
                    move, MIN(LCD_HEIGHT, pdisp->height),    /* w, h */
                    jpeg_settings.colour_mode, jpeg_settings.dither_mode);
#else
                MYXLCD(gray_bitmap_part)(
                    pdisp->bitmap[0], pdisp->x, pdisp->y, pdisp->stride,
                    0, MAX(0, (LCD_HEIGHT-pdisp->height)/2), /* x, y */
                    move, MIN(LCD_HEIGHT, pdisp->height));   /* w, h */
#endif
                MYLCD_UPDATE();
            }
            break;

        case JPEG_RIGHT:
            if (!(ds < ds_max) && entries > 0 && jpg.x_size <= MAX_X_SIZE)
                return change_filename(DIR_NEXT);
        case JPEG_RIGHT | BUTTON_REPEAT:
            move = MIN(HSCROLL, pdisp->width - pdisp->x - LCD_WIDTH);
            if (move > 0)
            {
                MYXLCD(scroll_left)(move); /* scroll left */
                pdisp->x += move;
#ifdef HAVE_LCD_COLOR
                yuv_bitmap_part(
                    pdisp->bitmap, pdisp->csub_x, pdisp->csub_y,
                    pdisp->x + LCD_WIDTH - move, pdisp->y, pdisp->stride,
                    LCD_WIDTH - move, MAX(0, (LCD_HEIGHT-pdisp->height)/2), /* x, y */
                    move, MIN(LCD_HEIGHT, pdisp->height),    /* w, h */
                    jpeg_settings.colour_mode, jpeg_settings.dither_mode);
#else
                MYXLCD(gray_bitmap_part)(
                    pdisp->bitmap[0], pdisp->x + LCD_WIDTH - move,
                    pdisp->y, pdisp->stride,
                    LCD_WIDTH - move, MAX(0, (LCD_HEIGHT-pdisp->height)/2), /* x, y */
                    move, MIN(LCD_HEIGHT, pdisp->height));   /* w, h */
#endif
                MYLCD_UPDATE();
            }
            break;

        case JPEG_UP:
        case JPEG_UP | BUTTON_REPEAT:
            move = MIN(VSCROLL, pdisp->y);
            if (move > 0)
            {
                MYXLCD(scroll_down)(move); /* scroll down */
                pdisp->y -= move;
#ifdef HAVE_LCD_COLOR
                if (jpeg_settings.dither_mode == DITHER_DIFFUSION)
                {
                    /* Draw over the band at the top of the last update
                       caused by lack of error history on line zero. */
                    move = MIN(move + 1, pdisp->y + pdisp->height);
                }

                yuv_bitmap_part(
                    pdisp->bitmap, pdisp->csub_x, pdisp->csub_y,
                    pdisp->x, pdisp->y, pdisp->stride,
                    MAX(0, (LCD_WIDTH-pdisp->width)/2), 0,   /* x, y */
                    MIN(LCD_WIDTH, pdisp->width), move,      /* w, h */
                    jpeg_settings.colour_mode, jpeg_settings.dither_mode);
#else
                MYXLCD(gray_bitmap_part)(
                    pdisp->bitmap[0], pdisp->x, pdisp->y, pdisp->stride,
                    MAX(0, (LCD_WIDTH-pdisp->width)/2), 0,   /* x, y */
                    MIN(LCD_WIDTH, pdisp->width), move);     /* w, h */
#endif
                MYLCD_UPDATE();
            }
            break;

        case JPEG_DOWN:
        case JPEG_DOWN | BUTTON_REPEAT:
            move = MIN(VSCROLL, pdisp->height - pdisp->y - LCD_HEIGHT);
            if (move > 0)
            {
                MYXLCD(scroll_up)(move); /* scroll up */
                pdisp->y += move;
#ifdef HAVE_LCD_COLOR
                if (jpeg_settings.dither_mode == DITHER_DIFFUSION)
                {
                    /* Save the line that was on the last line of the display
                       and draw one extra line above then recover the line with
                       image data that had an error history when it was drawn.
                     */
                    move++, pdisp->y--;
                    rb->memcpy(rgb_linebuf,
                           rb->lcd_framebuffer + (LCD_HEIGHT - move)*LCD_WIDTH,
                           LCD_WIDTH*sizeof (fb_data));
                }

                yuv_bitmap_part(
                    pdisp->bitmap, pdisp->csub_x, pdisp->csub_y, pdisp->x,
                    pdisp->y + LCD_HEIGHT - move, pdisp->stride,
                    MAX(0, (LCD_WIDTH-pdisp->width)/2), LCD_HEIGHT - move, /* x, y */
                    MIN(LCD_WIDTH, pdisp->width), move,     /* w, h */
                    jpeg_settings.colour_mode, jpeg_settings.dither_mode);

                if (jpeg_settings.dither_mode == DITHER_DIFFUSION)
                {
                    /* Cover the first row drawn with previous image data. */
                    rb->memcpy(rb->lcd_framebuffer + (LCD_HEIGHT - move)*LCD_WIDTH,
                           rgb_linebuf,
                           LCD_WIDTH*sizeof (fb_data));
                    pdisp->y++;
                }
#else
                MYXLCD(gray_bitmap_part)(
                    pdisp->bitmap[0], pdisp->x,
                    pdisp->y + LCD_HEIGHT - move, pdisp->stride,
                    MAX(0, (LCD_WIDTH-pdisp->width)/2), LCD_HEIGHT - move, /* x, y */
                    MIN(LCD_WIDTH, pdisp->width), move);     /* w, h */
#endif
                MYLCD_UPDATE();
            }
            break;
        case BUTTON_NONE:
            if (!slideshow_enabled)
                break;
            running_slideshow = true;
            if (entries > 0)
                return change_filename(DIR_NEXT);
            break;

#ifdef JPEG_SLIDE_SHOW
        case JPEG_SLIDE_SHOW:
            slideshow_enabled = !slideshow_enabled;
            running_slideshow = slideshow_enabled;
            break;
#endif

#ifdef JPEG_NEXT_REPEAT
        case JPEG_NEXT_REPEAT:
#endif
        case JPEG_NEXT:
            if (entries > 0)
                return change_filename(DIR_NEXT);
            break;

#ifdef JPEG_PREVIOUS_REPEAT
        case JPEG_PREVIOUS_REPEAT:
#endif
        case JPEG_PREVIOUS:
            if (entries > 0)
                return change_filename(DIR_PREV);
            break;

        case JPEG_ZOOM_IN:
#ifdef JPEG_ZOOM_PRE
            if (lastbutton != JPEG_ZOOM_PRE)
                break;
#endif
            return ZOOM_IN;
            break;

        case JPEG_ZOOM_OUT:
#ifdef JPEG_ZOOM_PRE
            if (lastbutton != JPEG_ZOOM_PRE)
                break;
#endif
            return ZOOM_OUT;
            break;
#ifdef JPEG_RC_MENU
        case JPEG_RC_MENU:
#endif
        case JPEG_MENU:
#ifdef USEGSLIB
            grey_show(false); /* switch off greyscale overlay */
#endif
            if (show_menu() == 1)
                return PLUGIN_OK;

#ifdef USEGSLIB
            grey_show(true); /* switch on greyscale overlay */
#else
            yuv_bitmap_part(
                pdisp->bitmap, pdisp->csub_x, pdisp->csub_y,
                pdisp->x, pdisp->y, pdisp->stride,
                MAX(0, (LCD_WIDTH - pdisp->width) / 2),
                MAX(0, (LCD_HEIGHT - pdisp->height) / 2),
                MIN(LCD_WIDTH, pdisp->width),
                MIN(LCD_HEIGHT, pdisp->height),
                jpeg_settings.colour_mode, jpeg_settings.dither_mode);
            MYLCD_UPDATE();
#endif
            break;
        default:
            if (rb->default_event_handler_ex(button, cleanup, NULL)
                == SYS_USB_CONNECTED)
                return PLUGIN_USB_CONNECTED;
            break;

        } /* switch */

        if (button != BUTTON_NONE)
            lastbutton = button;
    } /* while (true) */
}

/********************* main function *************************/

/* callback updating a progress meter while JPEG decoding */
void cb_progess(int current, int total)
{
    rb->yield(); /* be nice to the other threads */
    if(!running_slideshow)
    {
        rb->gui_scrollbar_draw(rb->screens[SCREEN_MAIN],0, LCD_HEIGHT-8, LCD_WIDTH, 8, total, 0,
                      current, HORIZONTAL);
        rb->lcd_update_rect(0, LCD_HEIGHT-8, LCD_WIDTH, 8);
    }
#ifndef USEGSLIB
    else
    {
        /* in slideshow mode, keep gui interference to a minimum */
        rb->gui_scrollbar_draw(rb->screens[SCREEN_MAIN],0, LCD_HEIGHT-4, LCD_WIDTH, 4, total, 0,
                      current, HORIZONTAL);
        rb->lcd_update_rect(0, LCD_HEIGHT-4, LCD_WIDTH, 4);
    }
#endif
}

int jpegmem(struct jpeg *p_jpg, int ds)
{
    int size;

    size = (p_jpg->x_phys/ds/p_jpg->subsample_x[0])
         * (p_jpg->y_phys/ds/p_jpg->subsample_y[0]);
#ifdef HAVE_LCD_COLOR
    if (p_jpg->blocks > 1) /* colour, add requirements for chroma */
    {
        size += (p_jpg->x_phys/ds/p_jpg->subsample_x[1])
              * (p_jpg->y_phys/ds/p_jpg->subsample_y[1]);
        size += (p_jpg->x_phys/ds/p_jpg->subsample_x[2])
              * (p_jpg->y_phys/ds/p_jpg->subsample_y[2]);
    }
#endif
    return size;
}

/* how far can we zoom in without running out of memory */
int min_downscale(struct jpeg *p_jpg, int bufsize)
{
    int downscale = 8;

    if (jpegmem(p_jpg, 8) > bufsize)
        return 0; /* error, too large, even 1:8 doesn't fit */

    while (downscale > 1 && jpegmem(p_jpg, downscale/2) <= bufsize)
        downscale /= 2;

    return downscale;
}


/* how far can we zoom out, to fit image into the LCD */
int max_downscale(struct jpeg *p_jpg)
{
    int downscale = 1;

    while (downscale < 8 && (p_jpg->x_size > LCD_WIDTH*downscale
                          || p_jpg->y_size > LCD_HEIGHT*downscale))
    {
        downscale *= 2;
    }

    return downscale;
}


/* return decoded or cached image */
struct t_disp* get_image(struct jpeg* p_jpg, int ds)
{
    int w, h; /* used to center output */
    int size; /* decompressed image size */
    long time; /* measured ticks */
    int status;

    struct t_disp* p_disp = &disp[ds]; /* short cut */

    if (p_disp->bitmap[0] != NULL)
    {
        return p_disp; /* we still have it */
    }

    /* assign image buffer */

     /* physical size needed for decoding */
    size = jpegmem(p_jpg, ds);
    if (buf_size <= size)
    {   /* have to discard the current */
        int i;
        for (i=1; i<=8; i++)
            disp[i].bitmap[0] = NULL; /* invalidate all bitmaps */
        buf = buf_root; /* start again from the beginning of the buffer */
        buf_size = root_size;
    }

#ifdef HAVE_LCD_COLOR
    if (p_jpg->blocks > 1) /* colour jpeg */
    {
        int i;

        for (i = 1; i < 3; i++)
        {
            size = (p_jpg->x_phys / ds / p_jpg->subsample_x[i])
                 * (p_jpg->y_phys / ds / p_jpg->subsample_y[i]);
            p_disp->bitmap[i] = buf;
            buf += size;
            buf_size -= size;
        }
        p_disp->csub_x = p_jpg->subsample_x[1];
        p_disp->csub_y = p_jpg->subsample_y[1];
    }
    else
    {
        p_disp->csub_x = p_disp->csub_y = 0;
        p_disp->bitmap[1] = p_disp->bitmap[2] = buf;
    }
#endif
    /* size may be less when decoded (if height is not block aligned) */
    size = (p_jpg->x_phys/ds) * (p_jpg->y_size / ds);
    p_disp->bitmap[0] = buf;
    buf += size;
    buf_size -= size;

    if(!running_slideshow)
    {
        rb->snprintf(print, sizeof(print), "decoding %d*%d",
            p_jpg->x_size/ds, p_jpg->y_size/ds);
        rb->lcd_puts(0, 3, print);
        rb->lcd_update();
    }

    /* update image properties */
    p_disp->width = p_jpg->x_size / ds;
    p_disp->stride = p_jpg->x_phys / ds; /* use physical size for stride */
    p_disp->height = p_jpg->y_size / ds;

    /* the actual decoding */
    time = *rb->current_tick;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
    status = jpeg_decode(p_jpg, p_disp->bitmap, ds, cb_progess);
    rb->cpu_boost(false);
#else
    status = jpeg_decode(p_jpg, p_disp->bitmap, ds, cb_progess);
#endif
    if (status)
    {
        rb->splashf(HZ, "decode error %d", status);
        file_pt[curfile] = '\0';
        return NULL;
    }
    time = *rb->current_tick - time;

    if(!running_slideshow)
    {
        rb->snprintf(print, sizeof(print), " %ld.%02ld sec ", time/HZ, time%HZ);
        rb->lcd_getstringsize(print, &w, &h); /* centered in progress bar */
        rb->lcd_putsxy((LCD_WIDTH - w)/2, LCD_HEIGHT - h, print);
        rb->lcd_update();
    }

    return p_disp;
}


/* set the view to the given center point, limit if necessary */
void set_view (struct t_disp* p_disp, int cx, int cy)
{
    int x, y;

    /* plain center to available width/height */
    x = cx - MIN(LCD_WIDTH, p_disp->width) / 2;
    y = cy - MIN(LCD_HEIGHT, p_disp->height) / 2;

    /* limit against upper image size */
    x = MIN(p_disp->width - LCD_WIDTH, x);
    y = MIN(p_disp->height - LCD_HEIGHT, y);

    /* limit against negative side */
    x = MAX(0, x);
    y = MAX(0, y);

    p_disp->x = x; /* set the values */
    p_disp->y = y;
}


/* calculate the view center based on the bitmap position */
void get_view(struct t_disp* p_disp, int* p_cx, int* p_cy)
{
    *p_cx = p_disp->x + MIN(LCD_WIDTH, p_disp->width) / 2;
    *p_cy = p_disp->y + MIN(LCD_HEIGHT, p_disp->height) / 2;
}


/* load, decode, display the image */
int load_and_show(char* filename)
{
    int fd;
    int filesize;
    unsigned char* buf_jpeg; /* compressed JPEG image */
    int status;
    struct t_disp* p_disp; /* currenly displayed image */
    int cx, cy; /* view center */

    fd = rb->open(filename, O_RDONLY);
    if (fd < 0)
    {
        rb->snprintf(print,sizeof(print),"err opening %s:%d",filename,fd);
        rb->splash(HZ, print);
        return PLUGIN_ERROR;
    }
    filesize = rb->filesize(fd);
    rb->memset(&disp, 0, sizeof(disp));

    buf = buf_images + filesize;
    buf_size = buf_images_size - filesize;
    /* allocate JPEG buffer */
    buf_jpeg = buf_images;

    buf_root = buf; /* we can start the decompressed images behind it */
    root_size = buf_size;

    if (buf_size <= 0)
    {
#if PLUGIN_BUFFER_SIZE >= MIN_MEM
        if(plug_buf)
        {
            rb->close(fd);
            rb->lcd_setfont(FONT_SYSFIXED);
            rb->lcd_clear_display();
            rb->snprintf(print,sizeof(print),"%s:",rb->strrchr(filename,'/')+1);
            rb->lcd_puts(0,0,print);
            rb->lcd_puts(0,1,"Not enough plugin memory!");
            rb->lcd_puts(0,2,"Zoom In: Stop playback.");
            if(entries>1)
                rb->lcd_puts(0,3,"Left/Right: Skip File.");
            rb->lcd_puts(0,4,"Off: Quit.");
            rb->lcd_update();
            rb->lcd_setfont(FONT_UI);

            rb->button_clear_queue();

            while (1)
            {
                int button = rb->button_get(true);
                switch(button)
                {
                    case JPEG_ZOOM_IN:
                        plug_buf = false;
                        buf_images = rb->plugin_get_audio_buffer(
                                        (size_t *)&buf_images_size);
                        /*try again this file, now using the audio buffer */
                        return PLUGIN_OTHER;
#ifdef JPEG_RC_MENU
                    case JPEG_RC_MENU:
#endif
                    case JPEG_MENU:
                        return PLUGIN_OK;

                    case JPEG_LEFT:
                        if(entries>1)
                        {
                            rb->lcd_clear_display();
                            return change_filename(DIR_PREV);
                        }
                        break;

                    case JPEG_RIGHT:
                        if(entries>1)
                        {
                            rb->lcd_clear_display();
                            return change_filename(DIR_NEXT);
                        }
                        break;
                    default:
                         if(rb->default_event_handler_ex(button, cleanup, NULL)
                                == SYS_USB_CONNECTED)
                              return PLUGIN_USB_CONNECTED;

                }
            }
        }
        else
#endif
        {
            rb->splash(HZ, "Out of Memory");
            rb->close(fd);
            return PLUGIN_ERROR;
        }
    }

    if(!running_slideshow)
    {
#if LCD_DEPTH > 1
        rb->lcd_set_foreground(LCD_WHITE);
        rb->lcd_set_background(LCD_BLACK);
        rb->lcd_set_backdrop(NULL);
#endif

        rb->lcd_clear_display();
        rb->snprintf(print, sizeof(print), "%s:", rb->strrchr(filename,'/')+1);
        rb->lcd_puts(0, 0, print);
        rb->lcd_update();

        rb->snprintf(print, sizeof(print), "loading %d bytes", filesize);
        rb->lcd_puts(0, 1, print);
        rb->lcd_update();
    }

    rb->read(fd, buf_jpeg, filesize);
    rb->close(fd);

    if(!running_slideshow)
    {
        rb->snprintf(print, sizeof(print), "decoding markers");
        rb->lcd_puts(0, 2, print);
        rb->lcd_update();
    }
#ifndef SIMULATOR
    else if(immediate_ata_off)
    {
        /* running slideshow and time is long enough: power down disk */
        rb->ata_sleep();
    }
#endif

    rb->memset(&jpg, 0, sizeof(jpg)); /* clear info struct */
    /* process markers, unstuffing */
    status = process_markers(buf_jpeg, filesize, &jpg);

    if (status < 0 || (status & (DQT | SOF0)) != (DQT | SOF0))
    {   /* bad format or minimum components not contained */
        rb->splashf(HZ, "unsupported %d", status);
        file_pt[curfile] = '\0';
        return change_filename(direction);
    }

    if (!(status & DHT)) /* if no Huffman table present: */
        default_huff_tbl(&jpg); /* use default */
    build_lut(&jpg); /* derive Huffman and other lookup-tables */

    if(!running_slideshow)
    {
        rb->snprintf(print, sizeof(print), "image %dx%d", jpg.x_size, jpg.y_size);
        rb->lcd_puts(0, 2, print);
        rb->lcd_update();
    }
    ds_max = max_downscale(&jpg);            /* check display constraint */
    ds_min = min_downscale(&jpg, buf_size);  /* check memory constraint */
    if (ds_min == 0)
    {
        rb->splash(HZ, "too large");
        file_pt[curfile] = '\0';
        return change_filename(direction);
    }

    ds = ds_max; /* initials setting */
    cx = jpg.x_size/ds/2; /* center the view */
    cy = jpg.y_size/ds/2;

    do  /* loop the image prepare and decoding when zoomed */
    {
        p_disp = get_image(&jpg, ds); /* decode or fetch from cache */
        if (p_disp == NULL)
            return change_filename(direction);

        set_view(p_disp, cx, cy);

        if(!running_slideshow)
        {
            rb->snprintf(print, sizeof(print), "showing %dx%d",
                p_disp->width, p_disp->height);
            rb->lcd_puts(0, 3, print);
            rb->lcd_update();
        }
        MYLCD(clear_display)();
#ifdef HAVE_LCD_COLOR
        yuv_bitmap_part(
            p_disp->bitmap, p_disp->csub_x, p_disp->csub_y,
            p_disp->x, p_disp->y, p_disp->stride,
            MAX(0, (LCD_WIDTH - p_disp->width) / 2),
            MAX(0, (LCD_HEIGHT - p_disp->height) / 2),
            MIN(LCD_WIDTH, p_disp->width),
            MIN(LCD_HEIGHT, p_disp->height),
            jpeg_settings.colour_mode, jpeg_settings.dither_mode);
#else
        MYXLCD(gray_bitmap_part)(
            p_disp->bitmap[0], p_disp->x, p_disp->y, p_disp->stride,
            MAX(0, (LCD_WIDTH - p_disp->width) / 2),
            MAX(0, (LCD_HEIGHT - p_disp->height) / 2),
            MIN(LCD_WIDTH, p_disp->width),
            MIN(LCD_HEIGHT, p_disp->height));
#endif
        MYLCD_UPDATE();

#ifdef USEGSLIB
        grey_show(true); /* switch on greyscale overlay */
#endif

        /* drawing is now finished, play around with scrolling
         * until you press OFF or connect USB
         */
        while (1)
        {
            status = scroll_bmp(p_disp);
            if (status == ZOOM_IN)
            {
                if (ds > ds_min)
                {
                    ds /= 2; /* reduce downscaling to zoom in */
                    get_view(p_disp, &cx, &cy);
                    cx *= 2; /* prepare the position in the new image */
                    cy *= 2;
                }
                else
                    continue;
            }

            if (status == ZOOM_OUT)
            {
                if (ds < ds_max)
                {
                    ds *= 2; /* increase downscaling to zoom out */
                    get_view(p_disp, &cx, &cy);
                    cx /= 2; /* prepare the position in the new image */
                    cy /= 2;
                }
                else
                    continue;
            }
            break;
        }

#ifdef USEGSLIB
        grey_show(false); /* switch off overlay */
#endif
        rb->lcd_clear_display();
    }
    while (status != PLUGIN_OK && status != PLUGIN_USB_CONNECTED
                                       && status != PLUGIN_OTHER);
#ifdef USEGSLIB
    rb->lcd_update();
#endif
    return status;
}

/******************** Plugin entry point *********************/

enum plugin_status plugin_start(const struct plugin_api* api, const void* parameter)
{
    rb = api;

    int condition;
#ifdef USEGSLIB
    long greysize; /* helper */
#endif
#if LCD_DEPTH > 1
    old_backdrop = rb->lcd_get_backdrop();
#endif

    if(!parameter) return PLUGIN_ERROR;

    rb->strcpy(np_file, parameter);
    get_pic_list();

    if(!entries) return PLUGIN_ERROR;

#if (PLUGIN_BUFFER_SIZE >= MIN_MEM) && !defined(SIMULATOR)
    if(rb->audio_status())
    {
        buf = rb->plugin_get_buffer((size_t *)&buf_size) +
             (entries * sizeof(char**));
        buf_size -= (entries * sizeof(char**));
        plug_buf = true;
    }
    else
        buf = rb->plugin_get_audio_buffer((size_t *)&buf_size);
#else
    buf = rb->plugin_get_audio_buffer(&buf_size) +
               (entries * sizeof(char**));
    buf_size -= (entries * sizeof(char**));
#endif

#ifdef USEGSLIB
    if (!grey_init(rb, buf, buf_size, GREY_ON_COP,
                   LCD_WIDTH, LCD_HEIGHT, &greysize))
    {
        rb->splash(HZ, "grey buf error");
        return PLUGIN_ERROR;
    }
    buf += greysize;
    buf_size -= greysize;
#else
    xlcd_init(rb);
#endif

    /* should be ok to just load settings since a parameter is present
       here and the drive should be spinning */
    configfile_init(rb);
    configfile_load(JPEG_CONFIGFILE, jpeg_config,
                    ARRAYLEN(jpeg_config), JPEG_SETTINGS_MINVERSION);
    old_settings = jpeg_settings;

    buf_images = buf; buf_images_size = buf_size;

    /* Turn off backlight timeout */
    backlight_force_on(rb); /* backlight control in lib/helper.c */

    do
    {
        condition = load_and_show(np_file);
    }while (condition != PLUGIN_OK && condition != PLUGIN_USB_CONNECTED
                                          && condition != PLUGIN_ERROR);

    if (rb->memcmp(&jpeg_settings, &old_settings, sizeof (jpeg_settings)))
    {
        /* Just in case drive has to spin, keep it from looking locked */
        rb->splash(0, "Saving Settings");
        configfile_save(JPEG_CONFIGFILE, jpeg_config,
                        ARRAYLEN(jpeg_config), JPEG_SETTINGS_VERSION);
    }

#if !defined(SIMULATOR) && defined(HAVE_DISK_STORAGE)
    /* set back ata spindown time in case we changed it */
    rb->ata_spindown(rb->global_settings->disk_spindown);
#endif

    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings(rb); /* backlight control in lib/helper.c */

#ifdef USEGSLIB
    grey_release(); /* deinitialize */
#endif

    return condition;
}
