/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * user intereface of image viewer
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

/*
 * TODO:
 *   - check magick value in file header to determine image type.
 */
#include "plugin.h"
#include <lib/playback_control.h>
#include <lib/helper.h>
#include <lib/configfile.h>
#include "imageviewer.h"
#include "imageviewer_button.h"
#include "image_decoder.h"


#ifdef USEGSLIB
GREY_INFO_STRUCT
#endif

/* Headings */
#define DIR_PREV  1
#define DIR_NEXT -1
#define DIR_NONE  0

/******************************* Globals ***********************************/

/* Persistent configuration */
#define IMGVIEW_CONFIGFILE          "imageviewer.cfg"
#define IMGVIEW_SETTINGS_MINVERSION 1
#define IMGVIEW_SETTINGS_VERSION    2

/* Slideshow times */
#define SS_MIN_TIMEOUT      1
#define SS_MAX_TIMEOUT      20
#define SS_DEFAULT_TIMEOUT  5

#ifdef HAVE_LCD_COLOR
/* needed for value of settings */
#include "jpeg/yuv2rgb.h"
#endif

static struct imgview_settings settings =
{
#ifdef HAVE_LCD_COLOR
    COLOURMODE_COLOUR,
    DITHER_NONE,
#endif
    SS_DEFAULT_TIMEOUT
};
static struct imgview_settings old_settings;

static struct configdata config[] =
{
#ifdef HAVE_LCD_COLOR
    { TYPE_ENUM, 0, COLOUR_NUM_MODES, { .int_p = &settings.jpeg_colour_mode },
        "Colour Mode", (char *[]){ "Colour", "Grayscale" } },
    { TYPE_ENUM, 0, DITHER_NUM_MODES, { .int_p = &settings.jpeg_dither_mode },
        "Dither Mode", (char *[]){ "None", "Ordered", "Diffusion" } },
#endif
    { TYPE_INT, SS_MIN_TIMEOUT, SS_MAX_TIMEOUT,
        { .int_p = &settings.ss_timeout }, "Slideshow Time", NULL },
};

static void cb_progress(int current, int total);

static struct imgdec_api iv_api = {
    .settings = &settings,
    .slideshow_enabled = false,
    .running_slideshow = false,
#ifdef DISK_SPINDOWN
    .immediate_ata_off = false,
#endif
#ifdef USE_PLUG_BUF
    .plug_buf = true,
#endif

    .cb_progress = cb_progress,

#ifdef USEGSLIB
    .gray_bitmap_part = myxlcd_ub_(gray_bitmap_part),
#endif
};

/**************** begin Application ********************/


/************************* Globals ***************************/

#ifdef HAVE_LCD_COLOR
static fb_data rgb_linebuf[LCD_WIDTH];  /* Line buffer for scrolling when
                                           DITHER_DIFFUSION is set        */
#endif

/* buffer to load image decoder */
static unsigned char* decoder_buf;
static size_t decoder_buf_size;
/* the remaining free part of the buffer for loaded+resized images */
static unsigned char* buf;
static size_t buf_size;

static int ds, ds_min, ds_max; /* downscaling and limits */
static struct image_info image_info;

/* the current full file name */
static char np_file[MAX_PATH];
static int curfile = -1, direction = DIR_NEXT, entries = 0;

/* list of the supported image files */
static char **file_pt;

static const struct image_decoder *imgdec = NULL;
static enum image_type image_type = IMAGE_UNKNOWN;

/************************* Implementation ***************************/

/* Read directory contents for scrolling. */
static void get_pic_list(void)
{
    struct tree_context *tree = rb->tree_get_context();
    struct entry *dircache = rb->tree_get_entries(tree);
    int i;
    char *pname;

    file_pt = (char **) buf;

    /* Remove path and leave only the name.*/
    pname = rb->strrchr(np_file,'/');
    pname++;

    for (i = 0; i < tree->filesindir && buf_size > sizeof(char**); i++)
    {
        /* Add all files. Non-image files will be filtered out while loading. */
        if (!(dircache[i].attr & ATTR_DIRECTORY))
        {
            file_pt[entries] = dircache[i].name;
            /* Set Selected File. */
            if (!rb->strcmp(file_pt[entries], pname))
                curfile = entries;
            entries++;

            buf += (sizeof(char**));
            buf_size -= (sizeof(char**));
        }
    }
}

static int change_filename(int direct)
{
    bool file_erased = (file_pt[curfile] == NULL);
    direction = direct;

    curfile += (direct == DIR_PREV? entries - 1: 1);
    if (curfile >= entries)
        curfile -= entries;

    if (file_erased)
    {
        /* remove 'erased' file names from list. */
        int count, i;
        for (count = i = 0; i < entries; i++)
        {
            if (curfile == i)
                curfile = count;
            if (file_pt[i] != NULL)
                file_pt[count++] = file_pt[i];
        }
        entries = count;
    }

    if (entries == 0)
    {
        rb->splash(HZ, "No supported files");
        return PLUGIN_ERROR;
    }

    rb->strcpy(rb->strrchr(np_file, '/')+1, file_pt[curfile]);

    return PLUGIN_OTHER;
}

/* switch off overlay, for handling SYS_ events */
static void cleanup(void *parameter)
{
    (void)parameter;
#ifdef USEGSLIB
    grey_show(false);
#endif
}

#ifdef HAVE_LCD_COLOR
static bool set_option_grayscale(void)
{
    bool gray = settings.jpeg_colour_mode == COLOURMODE_GRAY;
    rb->set_bool("Grayscale (Jpeg)", &gray);
    settings.jpeg_colour_mode = gray ? COLOURMODE_GRAY : COLOURMODE_COLOUR;
    return false;
}

static bool set_option_dithering(void)
{
    static const struct opt_items dithering[DITHER_NUM_MODES] = {
        [DITHER_NONE]      = { STR(LANG_OFF) },
        [DITHER_ORDERED]   = { STR(LANG_ORDERED) },
        [DITHER_DIFFUSION] = { STR(LANG_DIFFUSION) },
    };

    rb->set_option(rb->str(LANG_DITHERING), &settings.jpeg_dither_mode, INT,
                   dithering, DITHER_NUM_MODES, NULL);
    return false;
}

MENUITEM_FUNCTION(grayscale_item, 0, ID2P(LANG_GRAYSCALE),
                  set_option_grayscale, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(dithering_item, 0, ID2P(LANG_DITHERING),
                  set_option_dithering, NULL, NULL, Icon_NOICON);
MAKE_MENU(display_menu, "Display Options", NULL, Icon_NOICON,
            &grayscale_item, &dithering_item);

static void display_options(void)
{
    rb->do_menu(&display_menu, NULL, NULL, false);
}
#endif /* HAVE_LCD_COLOR */

static int show_menu(void) /* return 1 to quit */
{
    int result;

    enum menu_id
    {
        MIID_RETURN = 0,
        MIID_TOGGLE_SS_MODE,
        MIID_CHANGE_SS_MODE,
#ifdef USE_PLUG_BUF
        MIID_SHOW_PLAYBACK_MENU,
#endif
#ifdef HAVE_LCD_COLOR
        MIID_DISPLAY_OPTIONS,
#endif
        MIID_QUIT,
    };

    MENUITEM_STRINGLIST(menu, "Image Viewer Menu", NULL,
                        ID2P(LANG_RETURN),
                        ID2P(LANG_SLIDESHOW_MODE),
                        ID2P(LANG_SLIDESHOW_TIME),
#ifdef USE_PLUG_BUF
                        ID2P(LANG_PLAYBACK_CONTROL),
#endif
#ifdef HAVE_LCD_COLOR
                        ID2P(LANG_MENU_DISPLAY_OPTIONS),
#endif
                        ID2P(LANG_MENU_QUIT));

    static const struct opt_items slideshow[2] = {
        { STR(LANG_OFF) },
        { STR(LANG_ON) },
    };

    result=rb->do_menu(&menu, NULL, NULL, false);

    switch (result)
    {
        case MIID_RETURN:
            break;
        case MIID_TOGGLE_SS_MODE:
            rb->set_option(rb->str(LANG_SLIDESHOW_MODE), &iv_api.slideshow_enabled, BOOL,
                           slideshow , 2, NULL);
            break;
        case MIID_CHANGE_SS_MODE:
            rb->set_int(rb->str(LANG_SLIDESHOW_TIME), "s", UNIT_SEC,
                        &settings.ss_timeout, NULL, 1,
                        SS_MIN_TIMEOUT, SS_MAX_TIMEOUT, NULL);
            break;

#ifdef USE_PLUG_BUF
        case MIID_SHOW_PLAYBACK_MENU:
            if (iv_api.plug_buf)
            {
                playback_control(NULL);
            }
            else
            {
                rb->splash(HZ, ID2P(LANG_CANNOT_RESTART_PLAYBACK));
            }
            break;
#endif
#ifdef HAVE_LCD_COLOR
        case MIID_DISPLAY_OPTIONS:
            display_options();
            break;
#endif
        case MIID_QUIT:
            return 1;
            break;
    }

#ifdef DISK_SPINDOWN
    /* change ata spindown time based on slideshow time setting */
    iv_api.immediate_ata_off = false;
    rb->storage_spindown(rb->global_settings->disk_spindown);

    if (iv_api.slideshow_enabled)
    {
        if(settings.ss_timeout < 10)
        {
            /* slideshow times < 10s keep disk spinning */
            rb->storage_spindown(0);
        }
        else if (!rb->mp3_is_playing())
        {
            /* slideshow times > 10s and not playing: ata_off after load */
            iv_api.immediate_ata_off = true;
        }
    }
#endif
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(LCD_BLACK);
#endif
    rb->lcd_clear_display();
    return 0;
}

#ifdef USE_PLUG_BUF
static int ask_and_get_audio_buffer(const char *filename)
{
    int button;
#if defined(IMGVIEW_ZOOM_PRE)
    int lastbutton = BUTTON_NONE;
#endif
    rb->lcd_setfont(FONT_SYSFIXED);
    rb->lcd_clear_display();
    rb->lcd_puts(0, 0, rb->strrchr(filename,'/')+1);
    rb->lcd_puts(0, 1, "Not enough plugin memory!");
    rb->lcd_puts(0, 2, "Zoom In: Stop playback.");
    if(entries > 1)
        rb->lcd_puts(0, 3, "Left/Right: Skip File.");
    rb->lcd_puts(0, 4, "Show Menu: Quit.");
    rb->lcd_update();
    rb->lcd_setfont(FONT_UI);

    rb->button_clear_queue();

    while (1)
    {
        if (iv_api.slideshow_enabled)
            button = rb->button_get_w_tmo(settings.ss_timeout * HZ);
        else
            button = rb->button_get(true);

        switch(button)
        {
            case IMGVIEW_ZOOM_IN:
#ifdef IMGVIEW_ZOOM_PRE
                if (lastbutton != IMGVIEW_ZOOM_PRE)
                    break;
#endif
                iv_api.plug_buf = false;
                buf = rb->plugin_get_audio_buffer(&buf_size);
                /*try again this file, now using the audio buffer */
                return PLUGIN_OTHER;
#ifdef IMGVIEW_RC_MENU
            case IMGVIEW_RC_MENU:
#endif
#ifdef IMGVIEW_QUIT
            case IMGVIEW_QUIT:
#endif
            case IMGVIEW_MENU:
                return PLUGIN_OK;

            case IMGVIEW_LEFT:
                if(entries>1)
                {
                    rb->lcd_clear_display();
                    return change_filename(DIR_PREV);
                }
                break;

            case IMGVIEW_RIGHT:
                if(entries>1)
                {
                    rb->lcd_clear_display();
                    return change_filename(DIR_NEXT);
                }
                break;
            case BUTTON_NONE:
                if(entries>1)
                {
                    rb->lcd_clear_display();
                    return change_filename(direction);
                }
                break;

            default:
                if(rb->default_event_handler_ex(button, cleanup, NULL)
                        == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
        }
#if defined(IMGVIEW_ZOOM_PRE)
        if (button != BUTTON_NONE)
            lastbutton = button;
#endif
    }
}
#endif /* USE_PLUG_BUF */

/* callback updating a progress meter while image decoding */
static void cb_progress(int current, int total)
{
    rb->yield(); /* be nice to the other threads */
#ifndef USEGSLIB
    /* in slideshow mode, keep gui interference to a minimum */
    const int size = (!iv_api.running_slideshow ? 8 : 4);
#else
    const int size = 8;
    if(!iv_api.running_slideshow)
#endif
    {
        rb->gui_scrollbar_draw(rb->screens[SCREEN_MAIN],
                            0, LCD_HEIGHT-size, LCD_WIDTH, size,
                            total, 0, current, HORIZONTAL);
        rb->lcd_update_rect(0, LCD_HEIGHT-size, LCD_WIDTH, size);
    }
}

#define VSCROLL (LCD_HEIGHT/8)
#define HSCROLL (LCD_WIDTH/10)

/* Pan the viewing window right - move image to the left and fill in
   the right-hand side */
static void pan_view_right(struct image_info *info)
{
    int move;

    move = MIN(HSCROLL, info->width - info->x - LCD_WIDTH);
    if (move > 0)
    {
        mylcd_ub_scroll_left(move); /* scroll left */
        info->x += move;
        imgdec->draw_image_rect(info, LCD_WIDTH - move, 0,
                                move, info->height-info->y);
        mylcd_ub_update();
    }
}

/* Pan the viewing window left - move image to the right and fill in
   the left-hand side */
static void pan_view_left(struct image_info *info)
{
    int move;

    move = MIN(HSCROLL, info->x);
    if (move > 0)
    {
        mylcd_ub_scroll_right(move); /* scroll right */
        info->x -= move;
        imgdec->draw_image_rect(info, 0, 0, move, info->height-info->y);
        mylcd_ub_update();
    }
}

/* Pan the viewing window up - move image down and fill in
   the top */
static void pan_view_up(struct image_info *info)
{
    int move;

    move = MIN(VSCROLL, info->y);
    if (move > 0)
    {
        mylcd_ub_scroll_down(move); /* scroll down */
        info->y -= move;
#ifdef HAVE_LCD_COLOR
        if (image_type == IMAGE_JPEG
         && settings.jpeg_dither_mode == DITHER_DIFFUSION)
        {
            /* Draw over the band at the top of the last update
               caused by lack of error history on line zero. */
            move = MIN(move + 1, info->y + info->height);
        }
#endif
        imgdec->draw_image_rect(info, 0, 0, info->width-info->x, move);
        mylcd_ub_update();
    }
}

/* Pan the viewing window down - move image up and fill in
   the bottom */
static void pan_view_down(struct image_info *info)
{
    int move;

    move = MIN(VSCROLL, info->height - info->y - LCD_HEIGHT);
    if (move > 0)
    {
        mylcd_ub_scroll_up(move); /* scroll up */
        info->y += move;
#ifdef HAVE_LCD_COLOR
        if (image_type == IMAGE_JPEG
         && settings.jpeg_dither_mode == DITHER_DIFFUSION)
        {
            /* Save the line that was on the last line of the display
               and draw one extra line above then recover the line with
               image data that had an error history when it was drawn.
             */
            move++, info->y--;
            rb->memcpy(rgb_linebuf,
                    rb->lcd_framebuffer + (LCD_HEIGHT - move)*LCD_WIDTH,
                    LCD_WIDTH*sizeof (fb_data));
        }
#endif

        imgdec->draw_image_rect(info, 0, LCD_HEIGHT - move,
                                info->width-info->x, move);

#ifdef HAVE_LCD_COLOR
        if (image_type == IMAGE_JPEG
         && settings.jpeg_dither_mode == DITHER_DIFFUSION)
        {
            /* Cover the first row drawn with previous image data. */
            rb->memcpy(rb->lcd_framebuffer + (LCD_HEIGHT - move)*LCD_WIDTH,
                        rgb_linebuf, LCD_WIDTH*sizeof (fb_data));
            info->y++;
        }
#endif
        mylcd_ub_update();
    }
}

/* interactively scroll around the image */
static int scroll_bmp(struct image_info *info)
{
    static long ss_timeout = 0;

    int button;
#if defined(IMGVIEW_ZOOM_PRE) || defined(IMGVIEW_MENU_PRE) \
    || defined(IMGVIEW_SLIDE_SHOW_PRE)
    int lastbutton = BUTTON_NONE;
#endif

    if (!ss_timeout && iv_api.slideshow_enabled)
        ss_timeout = *rb->current_tick + settings.ss_timeout * HZ;

    while (true)
    {
        if (iv_api.slideshow_enabled)
        {
            if (info->frames_count > 1 && info->delay &&
                settings.ss_timeout * HZ > info->delay)
            {
                /* animated content and delay between subsequent frames
                 * is shorter then slideshow delay
                 */
                button = rb->button_get_w_tmo(info->delay);
            }
            else
                button = rb->button_get_w_tmo(settings.ss_timeout * HZ);
        }
        else
        {
            if (info->frames_count > 1 && info->delay)
                button = rb->button_get_w_tmo(info->delay);
            else
                button = rb->button_get(true);
        }

        iv_api.running_slideshow = false;

        switch(button)
        {
        case IMGVIEW_LEFT:
            if (entries > 1 && info->width <= LCD_WIDTH
                            && info->height <= LCD_HEIGHT)
                return change_filename(DIR_PREV);
        case IMGVIEW_LEFT | BUTTON_REPEAT:
            pan_view_left(info);
            break;

        case IMGVIEW_RIGHT:
            if (entries > 1 && info->width <= LCD_WIDTH
                            && info->height <= LCD_HEIGHT)
                return change_filename(DIR_NEXT);
        case IMGVIEW_RIGHT | BUTTON_REPEAT:
            pan_view_right(info);
            break;

        case IMGVIEW_UP:
        case IMGVIEW_UP | BUTTON_REPEAT:
            pan_view_up(info);
            break;

        case IMGVIEW_DOWN:
        case IMGVIEW_DOWN | BUTTON_REPEAT:
            pan_view_down(info);
            break;

        case BUTTON_NONE:
            if (iv_api.slideshow_enabled && entries > 1)
            {
                if (info->frames_count > 1)
                {
                    /* animations */
                    if (TIME_AFTER(*rb->current_tick, ss_timeout))
                    {
                        iv_api.running_slideshow = true;
                        ss_timeout = 0;
                        return change_filename(DIR_NEXT);
                    }
                    else
                        return NEXT_FRAME;
                }
                else
                {
                    /* still picture */
                    iv_api.running_slideshow = true;
                    return change_filename(DIR_NEXT);
                }
            }
            else
                return NEXT_FRAME;

            break;

#ifdef IMGVIEW_SLIDE_SHOW
        case IMGVIEW_SLIDE_SHOW:
#ifdef IMGVIEW_SLIDE_SHOW_PRE
            if (lastbutton != IMGVIEW_SLIDE_SHOW_PRE)
                break;
#endif
#ifdef IMGVIEW_SLIDE_SHOW2
        case IMGVIEW_SLIDE_SHOW2:
#endif
            iv_api.slideshow_enabled = !iv_api.slideshow_enabled;
            break;
#endif

#ifdef IMGVIEW_NEXT_REPEAT
        case IMGVIEW_NEXT_REPEAT:
#endif
        case IMGVIEW_NEXT:
            if (entries > 1)
                return change_filename(DIR_NEXT);
            break;

#ifdef IMGVIEW_PREVIOUS_REPEAT
        case IMGVIEW_PREVIOUS_REPEAT:
#endif
        case IMGVIEW_PREVIOUS:
            if (entries > 1)
                return change_filename(DIR_PREV);
            break;

        case IMGVIEW_ZOOM_IN:
#ifdef IMGVIEW_ZOOM_PRE
            if (lastbutton != IMGVIEW_ZOOM_PRE)
                break;
#endif
            return ZOOM_IN;
            break;

        case IMGVIEW_ZOOM_OUT:
#ifdef IMGVIEW_ZOOM_PRE
            if (lastbutton != IMGVIEW_ZOOM_PRE)
                break;
#endif
            return ZOOM_OUT;
            break;

#ifdef IMGVIEW_RC_MENU
        case IMGVIEW_RC_MENU:
#endif
        case IMGVIEW_MENU:
#ifdef IMGVIEW_MENU_PRE
            if (lastbutton != IMGVIEW_MENU_PRE)
                break;
#endif
#ifdef USEGSLIB
            grey_show(false); /* switch off greyscale overlay */
#endif
            if (show_menu() == 1)
                return PLUGIN_OK;

#ifdef USEGSLIB
            grey_show(true); /* switch on greyscale overlay */
#else
            imgdec->draw_image_rect(info, 0, 0,
                            info->width-info->x, info->height-info->y);
            mylcd_ub_update();
#endif
            break;

#ifdef IMGVIEW_QUIT
            case IMGVIEW_QUIT:
            return PLUGIN_OK;
            break;
#endif

        default:
            if (rb->default_event_handler_ex(button, cleanup, NULL)
                == SYS_USB_CONNECTED)
                return PLUGIN_USB_CONNECTED;
            break;

        } /* switch */
#if defined(IMGVIEW_ZOOM_PRE) || defined(IMGVIEW_MENU_PRE) || defined(IMGVIEW_SLIDE_SHOW_PRE)
        if (button != BUTTON_NONE)
            lastbutton = button;
#endif
    } /* while (true) */
}

/********************* main function *************************/

/* how far can we zoom in without running out of memory */
static int min_downscale(int bufsize)
{
    int downscale = 8;

    if (imgdec->img_mem(8) > bufsize)
        return 0; /* error, too large, even 1:8 doesn't fit */

    while (downscale > 1 && imgdec->img_mem(downscale/2) <= bufsize)
        downscale /= 2;

    return downscale;
}

/* how far can we zoom out, to fit image into the LCD */
static int max_downscale(struct image_info *info)
{
    int downscale = 1;

    while (downscale < 8 && (info->x_size/downscale > LCD_WIDTH
                          || info->y_size/downscale > LCD_HEIGHT))
    {
        downscale *= 2;
    }

    return downscale;
}

/* set the view to the given center point, limit if necessary */
static void set_view(struct image_info *info, int cx, int cy)
{
    int x, y;

    /* plain center to available width/height */
    x = cx - MIN(LCD_WIDTH, info->width) / 2;
    y = cy - MIN(LCD_HEIGHT, info->height) / 2;

    /* limit against upper image size */
    x = MIN(info->width - LCD_WIDTH, x);
    y = MIN(info->height - LCD_HEIGHT, y);

    /* limit against negative side */
    x = MAX(0, x);
    y = MAX(0, y);

    info->x = x; /* set the values */
    info->y = y;
}

/* calculate the view center based on the bitmap position */
static void get_view(struct image_info *info, int *p_cx, int *p_cy)
{
    *p_cx = info->x + MIN(LCD_WIDTH, info->width) / 2;
    *p_cy = info->y + MIN(LCD_HEIGHT, info->height) / 2;
}

/* load, decode, display the image */
static int load_and_show(char* filename, struct image_info *info)
{
    int status;
    int cx, cy;
    ssize_t remaining;

    rb->lcd_clear_display();

    /* suppress warning while running slideshow */
    status = get_image_type(filename, iv_api.running_slideshow);
    if (status == IMAGE_UNKNOWN) {
        /* file isn't supported image file, skip this. */
        file_pt[curfile] = NULL;
        return change_filename(direction);
    }
    if (image_type != status) /* type of image is changed, load decoder. */
    {
        struct loader_info loader_info = {
            status, &iv_api, decoder_buf, decoder_buf_size,
        };
        image_type = status;
        imgdec = load_decoder(&loader_info);
        if (imgdec == NULL)
        {
            /* something is wrong */
            return PLUGIN_ERROR;
        }
#ifdef USE_PLUG_BUF
        if(iv_api.plug_buf)
        {
            buf = loader_info.buffer;
            buf_size = loader_info.size;
        }
#endif
    }
    rb->memset(info, 0, sizeof(*info));
    remaining = buf_size;

    if (rb->button_get(false) == IMGVIEW_MENU)
        status = PLUGIN_ABORT;
    else
        status = imgdec->load_image(filename, info, buf, &remaining);

    if (status == PLUGIN_OUTOFMEM)
    {
#ifdef USE_PLUG_BUF
        if(iv_api.plug_buf)
        {
            return ask_and_get_audio_buffer(filename);
        }
        else
#endif
        {
            rb->splash(HZ, "Out of Memory");
            file_pt[curfile] = NULL;
            return change_filename(direction);
        }
    }
    else if (status == PLUGIN_ERROR)
    {
        file_pt[curfile] = NULL;
        return change_filename(direction);
    }
    else if (status == PLUGIN_ABORT) {
        rb->splash(HZ, "Aborted");
        return PLUGIN_OK;
    }

    ds_max = max_downscale(info);       /* check display constraint */
    ds_min = min_downscale(remaining);  /* check memory constraint */
    if (ds_min == 0)
    {
        if (imgdec->unscaled_avail)
        {
            /* Can not resize the image but original one is available, so use it. */
            ds_min = ds_max = 1;
        }
        else
#ifdef USE_PLUG_BUF
        if (iv_api.plug_buf)
        {
            return ask_and_get_audio_buffer(filename);
        }
        else
#endif
        {
            rb->splash(HZ, "Too large");
            file_pt[curfile] = NULL;
            return change_filename(direction);
        }
    }
    else if (ds_max < ds_min)
        ds_max = ds_min;

    ds = ds_max; /* initialize setting */
    cx = info->x_size/ds/2; /* center the view */
    cy = info->y_size/ds/2;

    /* used to loop through subimages in animated gifs */
    int frame = 0;
    do  /* loop the image prepare and decoding when zoomed */
    {
        status = imgdec->get_image(info, frame, ds); /* decode or fetch from cache */
        if (status == PLUGIN_ERROR)
        {
            file_pt[curfile] = NULL;
            return change_filename(direction);
        }

        set_view(info, cx, cy);

        if(!iv_api.running_slideshow && (info->frames_count == 1))
        {
            rb->lcd_putsf(0, 3, "showing %dx%d", info->width, info->height);
            rb->lcd_update();
        }

        mylcd_ub_clear_display();
        imgdec->draw_image_rect(info, 0, 0,
                        info->width-info->x, info->height-info->y);
        mylcd_ub_update();

#ifdef USEGSLIB
        grey_show(true); /* switch on greyscale overlay */
#endif

        /* drawing is now finished, play around with scrolling
         * until you press OFF or connect USB
         */
        while (1)
        {
            status = scroll_bmp(info);

            if (status == ZOOM_IN)
            {
                if (ds > ds_min || (imgdec->unscaled_avail && ds > 1))
                {
                    /* if 1/1 is always available, jump ds from ds_min to 1. */
                    int zoom = (ds == ds_min)? ds_min: 2;
                    ds /= zoom; /* reduce downscaling to zoom in */
                    get_view(info, &cx, &cy);
                    cx *= zoom; /* prepare the position in the new image */
                    cy *= zoom;
                }
                else
                    continue;
            }

            if (status == ZOOM_OUT)
            {
                if (ds < ds_max)
                {
                    /* if ds is 1 and ds_min is > 1, jump ds to ds_min. */
                    int zoom = (ds < ds_min)? ds_min: 2;
                    ds *= zoom; /* increase downscaling to zoom out */
                    get_view(info, &cx, &cy);
                    cx /= zoom; /* prepare the position in the new image */
                    cy /= zoom;
                }
                else
                    continue;
            }

            /* next frame in animated content */
            if (status == NEXT_FRAME)
                frame = (frame + 1)%info->frames_count;

            break;
        }

        rb->lcd_clear_display();
    }
    while (status > PLUGIN_OTHER);
#ifdef USEGSLIB
    grey_show(false); /* switch off overlay */
    rb->lcd_update();
#endif
    return status;
}

/******************** Plugin entry point *********************/

enum plugin_status plugin_start(const void* parameter)
{
    int condition;
#ifdef USEGSLIB
    long greysize; /* helper */
#endif

    if(!parameter) return PLUGIN_ERROR;

    rb->strcpy(np_file, parameter);
    if (get_image_type(np_file, false) == IMAGE_UNKNOWN)
    {
        rb->splash(HZ*2, "Unsupported file");
        return PLUGIN_ERROR;
    }

#ifdef USE_PLUG_BUF
    buf = rb->plugin_get_buffer(&buf_size);
#else
    decoder_buf = rb->plugin_get_buffer(&decoder_buf_size);
    buf = rb->plugin_get_audio_buffer(&buf_size);
#endif

    get_pic_list();

    if(!entries) return PLUGIN_ERROR;

#ifdef USEGSLIB
    if (!grey_init(buf, buf_size, GREY_ON_COP,
                   LCD_WIDTH, LCD_HEIGHT, &greysize))
    {
        rb->splash(HZ, "grey buf error");
        return PLUGIN_ERROR;
    }
    buf += greysize;
    buf_size -= greysize;
#endif

#ifdef USE_PLUG_BUF
    decoder_buf = buf;
    decoder_buf_size = buf_size;
    if(!rb->audio_status())
    {
        iv_api.plug_buf = false;
        buf = rb->plugin_get_audio_buffer(&buf_size);
    }
#endif

    /* should be ok to just load settings since the plugin itself has
       just been loaded from disk and the drive should be spinning */
    configfile_load(IMGVIEW_CONFIGFILE, config,
                    ARRAYLEN(config), IMGVIEW_SETTINGS_MINVERSION);
    rb->memcpy(&old_settings, &settings, sizeof (settings));

    /* Turn off backlight timeout */
    backlight_ignore_timeout();

#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(LCD_BLACK);
#endif

    do
    {
        condition = load_and_show(np_file, &image_info);
    } while (condition >= PLUGIN_OTHER);
    release_decoder();

    if (rb->memcmp(&settings, &old_settings, sizeof (settings)))
    {
        /* Just in case drive has to spin, keep it from looking locked */
        rb->splash(0, "Saving Settings");
        configfile_save(IMGVIEW_CONFIGFILE, config,
                        ARRAYLEN(config), IMGVIEW_SETTINGS_VERSION);
    }

#ifdef DISK_SPINDOWN
    /* set back ata spindown time in case we changed it */
    rb->storage_spindown(rb->global_settings->disk_spindown);
#endif

    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings();

#ifdef USEGSLIB
    grey_release(); /* deinitialize */
#endif

    return condition;
}
