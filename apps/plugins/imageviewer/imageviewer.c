/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * user intereface of image viewers (jpeg, png, etc.)
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
#include <lib/helper.h>
#include <lib/configfile.h>
#include "imageviewer.h"

PLUGIN_HEADER

#ifdef USEGSLIB
GREY_INFO_STRUCT
#endif

/* Headings */
#define DIR_PREV  1
#define DIR_NEXT -1
#define DIR_NONE  0

/******************************* Globals ***********************************/

bool slideshow_enabled = false;   /* run slideshow */
bool running_slideshow = false;   /* loading image because of slideshw */
#ifdef DISK_SPINDOWN
bool immediate_ata_off = false;   /* power down disk after loading */
#endif
#if PLUGIN_BUFFER_SIZE >= MIN_MEM
/* are we using the plugin buffer or the audio buffer? */
bool plug_buf = true;
#endif

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

/* jpeg use this */
struct imgview_settings settings =
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

#if LCD_DEPTH > 1
static fb_data* old_backdrop;
#endif

/**************** begin Application ********************/


/************************* Globals ***************************/

#if defined(HAVE_LCD_COLOR) && defined(JPEG_VIEWER)
static fb_data rgb_linebuf[LCD_WIDTH];  /* Line buffer for scrolling when
                                           DITHER_DIFFUSION is set        */
#endif

/* my memory pool (from the mp3 buffer) */
static char print[32]; /* use a common snprintf() buffer */
/* the remaining free part of the buffer for loaded+resized images */
static unsigned char* buf;
static ssize_t buf_size;

static int ds, ds_min, ds_max; /* downscaling and limits */
static struct image_info image_info;

static struct tree_context *tree;

/* the current full file name */
static char np_file[MAX_PATH];
static int curfile = 0, direction = DIR_NONE, entries = 0;

/* list of the supported image files */
static char **file_pt;

/************************* Implementation ***************************/

/*Read directory contents for scrolling. */
static void get_pic_list(void)
{
    int i;
    struct entry *dircache;
    char *pname;
    tree = rb->tree_get_context();
    dircache = tree->dircache;

    file_pt = (char **) buf;

    /* Remove path and leave only the name.*/
    pname = rb->strrchr(np_file,'/');
    pname++;

    for (i = 0; i < tree->filesindir; i++)
    {
        if (!(dircache[i].attr & ATTR_DIRECTORY)
            && img_ext(rb->strrchr(dircache[i].name,'.')))
        {
            file_pt[entries] = dircache[i].name;
            /* Set Selected File. */
            if (!rb->strcmp(file_pt[entries], pname))
                curfile = entries;
            entries++;
        }
    }

    buf += (entries * sizeof(char**));
    buf_size -= (entries * sizeof(char**));
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

    if (rb->strlen(tree->currdir) > 1)
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
static void cleanup(void *parameter)
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

#if defined(HAVE_LCD_COLOR) && defined(JPEG_VIEWER)
static bool set_option_grayscale(void)
{
    bool gray = settings.jpeg_colour_mode == COLOURMODE_GRAY;
    rb->set_bool("Grayscale", &gray);
    settings.jpeg_colour_mode = gray ? COLOURMODE_GRAY : COLOURMODE_COLOUR;
    return false;
}

static bool set_option_dithering(void)
{
    static const struct opt_items dithering[DITHER_NUM_MODES] = {
        [DITHER_NONE]      = { "Off",       -1 },
        [DITHER_ORDERED]   = { "Ordered",   -1 },
        [DITHER_DIFFUSION] = { "Diffusion", -1 },
    };

    rb->set_option("Dithering", &settings.jpeg_dither_mode, INT,
                   dithering, DITHER_NUM_MODES, NULL);
    return false;
}

MENUITEM_FUNCTION(grayscale_item, 0, "Greyscale",
                  set_option_grayscale, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(dithering_item, 0, "Dithering",
                  set_option_dithering, NULL, NULL, Icon_NOICON);
MAKE_MENU(display_menu, "Display Options", NULL, Icon_NOICON,
            &grayscale_item, &dithering_item);

static void display_options(void)
{
    rb->do_menu(&display_menu, NULL, NULL, false);
}
#endif /* defined(HAVE_LCD_COLOR) && defined(JPEG_VIEWER) */

static int show_menu(void) /* return 1 to quit */
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
    int result;

    enum menu_id
    {
        MIID_RETURN = 0,
        MIID_TOGGLE_SS_MODE,
        MIID_CHANGE_SS_MODE,
#if PLUGIN_BUFFER_SIZE >= MIN_MEM
        MIID_SHOW_PLAYBACK_MENU,
#endif
#if defined(HAVE_LCD_COLOR) && defined(JPEG_VIEWER)
        MIID_DISPLAY_OPTIONS,
#endif
        MIID_QUIT,
    };

    MENUITEM_STRINGLIST(menu, MENU_TITLE, NULL,
                        "Return", "Toggle Slideshow Mode",
                        "Change Slideshow Time",
#if PLUGIN_BUFFER_SIZE >= MIN_MEM
                        "Show Playback Menu",
#endif
#if defined(HAVE_LCD_COLOR) && defined(JPEG_VIEWER)
                        "Display Options",
#endif
                        "Quit");

    static const struct opt_items slideshow[2] = {
        { "Disable", -1 },
        { "Enable", -1 },
    };

    result=rb->do_menu(&menu, NULL, NULL, false);

    switch (result)
    {
        case MIID_RETURN:
            break;
        case MIID_TOGGLE_SS_MODE:
            rb->set_option("Toggle Slideshow", &slideshow_enabled, INT,
                           slideshow , 2, NULL);
            break;
        case MIID_CHANGE_SS_MODE:
            rb->set_int("Slideshow Time", "s", UNIT_SEC,
                        &settings.ss_timeout, NULL, 1,
                        SS_MIN_TIMEOUT, SS_MAX_TIMEOUT, NULL);
            break;

#if PLUGIN_BUFFER_SIZE >= MIN_MEM
        case MIID_SHOW_PLAYBACK_MENU:
            if (plug_buf)
            {
                playback_control(NULL);
            }
            else
            {
                rb->splash(HZ, "Cannot restart playback");
            }
            break;
#endif
#if defined(HAVE_LCD_COLOR) && defined(JPEG_VIEWER)
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
    immediate_ata_off = false;
    rb->storage_spindown(rb->global_settings->disk_spindown);

    if (slideshow_enabled)
    {
        if(settings.ss_timeout < 10)
        {
            /* slideshow times < 10s keep disk spinning */
            rb->storage_spindown(0);
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
    return 0;
}

/* Pan the viewing window right - move image to the left and fill in
   the right-hand side */
static void pan_view_right(struct image_info *info)
{
    int move;

    move = MIN(HSCROLL, info->width - info->x - LCD_WIDTH);
    if (move > 0)
    {
        MYXLCD(scroll_left)(move); /* scroll left */
        info->x += move;
        draw_image_rect(info, LCD_WIDTH - move, 0, move, info->height-info->y);
        MYLCD_UPDATE();
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
        MYXLCD(scroll_right)(move); /* scroll right */
        info->x -= move;
        draw_image_rect(info, 0, 0, move, info->height-info->y);
        MYLCD_UPDATE();
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
        MYXLCD(scroll_down)(move); /* scroll down */
        info->y -= move;
#if defined(HAVE_LCD_COLOR) && defined(JPEG_VIEWER)
        if (settings.jpeg_dither_mode == DITHER_DIFFUSION)
        {
            /* Draw over the band at the top of the last update
               caused by lack of error history on line zero. */
            move = MIN(move + 1, info->y + info->height);
        }
#endif
        draw_image_rect(info, 0, 0, info->width-info->x, move);
        MYLCD_UPDATE();
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
        MYXLCD(scroll_up)(move); /* scroll up */
        info->y += move;
#if defined(HAVE_LCD_COLOR) && defined(JPEG_VIEWER)
        if (settings.jpeg_dither_mode == DITHER_DIFFUSION)
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

        draw_image_rect(info, 0, LCD_HEIGHT - move, info->width-info->x, move);

#if defined(HAVE_LCD_COLOR) && defined(JPEG_VIEWER)
        if (settings.jpeg_dither_mode == DITHER_DIFFUSION)
        {
            /* Cover the first row drawn with previous image data. */
            rb->memcpy(rb->lcd_framebuffer + (LCD_HEIGHT - move)*LCD_WIDTH,
                        rgb_linebuf, LCD_WIDTH*sizeof (fb_data));
            info->y++;
        }
#endif
        MYLCD_UPDATE();
    }
}

/* interactively scroll around the image */
static int scroll_bmp(struct image_info *info)
{
    int button;
    int lastbutton = 0;

    while (true)
    {
        if (slideshow_enabled)
            button = rb->button_get_w_tmo(settings.ss_timeout * HZ);
        else
            button = rb->button_get(true);

        running_slideshow = false;

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
            if (!slideshow_enabled)
                break;
            running_slideshow = true;
            if (entries > 1)
                return change_filename(DIR_NEXT);
            break;

#ifdef IMGVIEW_SLIDE_SHOW
        case IMGVIEW_SLIDE_SHOW:
            slideshow_enabled = !slideshow_enabled;
            running_slideshow = slideshow_enabled;
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
#ifdef USEGSLIB
            grey_show(false); /* switch off greyscale overlay */
#endif
            if (show_menu() == 1)
                return PLUGIN_OK;

#ifdef USEGSLIB
            grey_show(true); /* switch on greyscale overlay */
#else
            draw_image_rect(info, 0, 0,
                            info->width-info->x, info->height-info->y);
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

/* callback updating a progress meter while image decoding */
void cb_progress(int current, int total)
{
    rb->yield(); /* be nice to the other threads */
    if(!running_slideshow)
    {
        rb->gui_scrollbar_draw(rb->screens[SCREEN_MAIN],
                            0, LCD_HEIGHT-8, LCD_WIDTH, 8,
                            total, 0, current, HORIZONTAL);
        rb->lcd_update_rect(0, LCD_HEIGHT-8, LCD_WIDTH, 8);
    }
#ifndef USEGSLIB
    else
    {
        /* in slideshow mode, keep gui interference to a minimum */
        rb->gui_scrollbar_draw(rb->screens[SCREEN_MAIN],
                            0, LCD_HEIGHT-4, LCD_WIDTH, 4,
                            total, 0, current, HORIZONTAL);
        rb->lcd_update_rect(0, LCD_HEIGHT-4, LCD_WIDTH, 4);
    }
#endif
}

/* how far can we zoom in without running out of memory */
static int min_downscale(int bufsize)
{
    int downscale = 8;

    if (img_mem(8) > bufsize)
        return 0; /* error, too large, even 1:8 doesn't fit */

    while (downscale > 1 && img_mem(downscale/2) <= bufsize)
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

#if LCD_DEPTH > 1
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(LCD_BLACK);
    rb->lcd_set_backdrop(NULL);
#endif
    rb->lcd_clear_display();

    rb->memset(info, 0, sizeof(*info));
    remaining = buf_size;

    if (rb->button_get(false) == IMGVIEW_MENU)
        status = PLUGIN_ABORT;
    else
        status = load_image(filename, info, buf, &remaining);

    if (status == PLUGIN_OUTOFMEM)
    {
#if PLUGIN_BUFFER_SIZE >= MIN_MEM
        if(plug_buf)
        {
            rb->lcd_setfont(FONT_SYSFIXED);
            rb->lcd_clear_display();
            rb->snprintf(print,sizeof(print),"%s:",rb->strrchr(filename,'/')+1);
            rb->lcd_puts(0,0,print);
            rb->lcd_puts(0,1,"Not enough plugin memory!");
            rb->lcd_puts(0,2,"Zoom In: Stop playback.");
            if(entries>1)
                rb->lcd_puts(0,3,"Left/Right: Skip File.");
            rb->lcd_puts(0,4,"Show Menu: Quit.");
            rb->lcd_update();
            rb->lcd_setfont(FONT_UI);

            rb->button_clear_queue();

            while (1)
            {
                int button = rb->button_get(true);
                switch(button)
                {
                    case IMGVIEW_ZOOM_IN:
                        plug_buf = false;
                        buf = rb->plugin_get_audio_buffer((size_t *)&buf_size);
                        /*try again this file, now using the audio buffer */
                        return PLUGIN_OTHER;
#ifdef IMGVIEW_RC_MENU
                    case IMGVIEW_RC_MENU:
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
        rb->splash(HZ, "aborted");
        return PLUGIN_OK;
    }

    ds_max = max_downscale(info);       /* check display constraint */
    ds_min = min_downscale(remaining);  /* check memory constraint */
    if (ds_min == 0)
    {
#if UNSCALED_IS_AVAILABLE
        /* Can not resize the image but original one is available, so use it. */
        ds_min = ds_max = 1;
#else
        /* not enough memory to decode image. */
        rb->splash(HZ, "too large");
        file_pt[curfile] = NULL;
        return change_filename(direction);
#endif
    }
    else if (ds_max < ds_min)
        ds_max = ds_min;

    ds = ds_max; /* initialize setting */
    cx = info->x_size/ds/2; /* center the view */
    cy = info->y_size/ds/2;

    do  /* loop the image prepare and decoding when zoomed */
    {
        status = get_image(info, ds); /* decode or fetch from cache */
        if (status == PLUGIN_ERROR)
        {
            file_pt[curfile] = NULL;
            return change_filename(direction);
        }

        set_view(info, cx, cy);

        if(!running_slideshow)
        {
            rb->snprintf(print, sizeof(print), "showing %dx%d",
                info->width, info->height);
            rb->lcd_puts(0, 3, print);
            rb->lcd_update();
        }

        MYLCD(clear_display)();
        draw_image_rect(info, 0, 0,
                        info->width-info->x, info->height-info->y);
        MYLCD_UPDATE();

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
#if UNSCALED_IS_AVAILABLE
                if (ds > 1)
#else
                if (ds > ds_min)
#endif
                {
#if UNSCALED_IS_AVAILABLE
                    /* if 1/1 is always available, jump ds from ds_min to 1. */
                    int zoom = (ds == ds_min)? ds_min: 2;
#else
                    const int zoom = 2;
#endif
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
#if UNSCALED_IS_AVAILABLE
                    /* if ds is 1 and ds_min is > 1, jump ds to ds_min. */
                    int zoom = (ds < ds_min)? ds_min: 2;
#else
                    const int zoom = 2;
#endif
                    ds *= zoom; /* increase downscaling to zoom out */
                    get_view(info, &cx, &cy);
                    cx /= zoom; /* prepare the position in the new image */
                    cy /= zoom;
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

enum plugin_status plugin_start(const void* parameter)
{
    int condition;
#ifdef USEGSLIB
    long greysize; /* helper */
#endif
#if LCD_DEPTH > 1
    old_backdrop = rb->lcd_get_backdrop();
#endif

    if(!parameter) return PLUGIN_ERROR;

#if PLUGIN_BUFFER_SIZE >= MIN_MEM
    buf = rb->plugin_get_buffer((size_t *)&buf_size);
#else
    buf = rb->plugin_get_audio_buffer((size_t *)&buf_size);
#endif

    rb->strcpy(np_file, parameter);
    get_pic_list();

    if(!entries) return PLUGIN_ERROR;

#if PLUGIN_BUFFER_SIZE >= MIN_MEM
    if(!rb->audio_status())
    {
        plug_buf = false;
        buf = rb->plugin_get_audio_buffer((size_t *)&buf_size);
    }
#endif

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

    /* should be ok to just load settings since the plugin itself has
       just been loaded from disk and the drive should be spinning */
    configfile_load(IMGVIEW_CONFIGFILE, config,
                    ARRAYLEN(config), IMGVIEW_SETTINGS_MINVERSION);
    rb->memcpy(&old_settings, &settings, sizeof (settings));

    /* Turn off backlight timeout */
    backlight_force_on(); /* backlight control in lib/helper.c */

    do
    {
        condition = load_and_show(np_file, &image_info);
    } while (condition != PLUGIN_OK && condition != PLUGIN_USB_CONNECTED
            && condition != PLUGIN_ERROR);

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
    backlight_use_settings(); /* backlight control in lib/helper.c */

#ifdef USEGSLIB
    grey_release(); /* deinitialize */
#endif

    return condition;
}
