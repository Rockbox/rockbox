/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 Franklin Wei, Benjamin Brown
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

/* TODO: */
/* vertical stride support (as of Dec. 2014, only the M:Robe 500 has a color,
   vertical stride LCD) */

/* monochrome/grayscale support (many grayscale targets have vertical strides,
   so get that working first!) */

#include "plugin.h"

#include "lib/display_text.h"
#include "lib/helper.h"
#include "lib/pluginlib_actions.h"
#include "lib/pluginlib_bmp.h"
#include "lib/pluginlib_exit.h"
#include "lib/keymaps.h"

#include "sys.h"
#include "parts.h"
#include "engine.h"

static struct System* save_sys;
static fb_data *lcd_fb = NULL;

static bool sys_save_settings(struct System* sys)
{
    File f;
    file_create(&f, false);
    if(!file_open(&f, SETTINGS_FILE, sys->e->_saveDir, "wb"))
    {
        return false;
    }
    file_write(&f, &sys->settings, sizeof(sys->settings));
    file_close(&f);
    return true;
}

static void sys_rotate_keymap(struct System* sys)
{
    switch(sys->settings.rotation_option)
    {
    case 0:
        sys->keymap.up = BTN_UP;
        sys->keymap.down = BTN_DOWN;
        sys->keymap.left = BTN_LEFT;
        sys->keymap.right = BTN_RIGHT;
#ifdef BTN_HAVE_DIAGONAL
        sys->keymap.upleft = BTN_UP_LEFT;
        sys->keymap.upright = BTN_UP_RIGHT;
        sys->keymap.downleft = BTN_DOWN_RIGHT;
        sys->keymap.downright = BTN_DOWN_LEFT;
#endif
        break;
    case 1:
        sys->keymap.up = BTN_RIGHT;
        sys->keymap.down = BTN_LEFT;
        sys->keymap.left = BTN_UP;
        sys->keymap.right = BTN_DOWN;
#ifdef BTN_HAVE_DIAGONAL
        sys->keymap.upleft = BTN_UP_RIGHT;
        sys->keymap.upright = BTN_DOWN_RIGHT;
        sys->keymap.downleft = BTN_UP_LEFT;
        sys->keymap.downright = BTN_DOWN_LEFT;
#endif
        break;
    case 2:
        sys->keymap.up = BTN_LEFT;
        sys->keymap.down = BTN_RIGHT;
        sys->keymap.left = BTN_DOWN;
        sys->keymap.right = BTN_UP;
#ifdef BTN_HAVE_DIAGONAL
        sys->keymap.upleft = BTN_DOWN_LEFT;
        sys->keymap.upright = BTN_UP_LEFT;
        sys->keymap.downleft = BTN_DOWN_RIGHT;
        sys->keymap.downright = BTN_DOWN_LEFT;
#endif
        break;
    default:
        error("sys_rotate_keymap: fall-through!");
    }
}

static bool sys_load_settings(struct System* sys)
{
    File f;
    file_create(&f, false);
    if(!file_open(&f, SETTINGS_FILE, sys->e->_saveDir, "rb"))
    {
        return false;
    }
    file_read(&f, &sys->settings, sizeof(sys->settings));
    file_close(&f);
    sys_rotate_keymap(sys);
    return true;
}

void exit_handler(void)
{
    sys_save_settings(save_sys);
    sys_stopAudio(save_sys);
    rb->timer_unregister();
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
#ifdef HAVE_BACKLIGHT
    backlight_use_settings();
#endif
}

static bool sys_do_help(void)
{
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(LCD_BLACK);
#endif
    rb->lcd_setfont(FONT_UI);
    char *help_text[] = {
        "XWorld", "",
        "XWorld", "is", "a", "port", "of", "a", "bytecode", "interpreter", "for", "`Another", "World',", "a", "cinematic", "adventure", "game", "by", "Eric", "Chahi.",
        "",
        "",
        "Level", "Codes:", "",
        "Level", "1:", "LDKD", "",
        "Level", "2:", "HTDC", "",
        "Level", "3:", "CLLD", "",
        "Level", "4:", "LBKG", "",
        "Level", "5:", "XDDJ", "",
        "Level", "6:", "FXLC", "",
        "Level", "7:", "KRFK", "",
        "Level", "8:", "KFLB", "",
        "Level", "9:", "DDRX", "",
        "Level", "10:", "BFLX", "",
        "Level", "11:", "BRTD", "",
        "Level", "12:", "TFBB", "",
        "Level", "13:", "TXHF", "",
        "Level", "14:", "CKJL", "",
        "Level", "15:", "LFCK", "",
    };
    struct style_text style[] = {
        { 0, TEXT_CENTER | TEXT_UNDERLINE },
    };

    return display_text(ARRAYLEN(help_text), help_text, style, NULL, true);
}

static const struct opt_items scaling_settings[3] = {
    { "Disabled", -1 },
    { "Fast"    , -1 },
#ifdef HAVE_LCD_COLOR
    { "Good"    , -1 }
#endif
};

static const struct opt_items rotation_settings[3] = {
    { "Disabled"        , -1 },
    { "Clockwise"       , -1 },
    { "Counterclockwise", -1 }
};

static void do_video_settings(struct System* sys)
{
    MENUITEM_STRINGLIST(menu, "Video Settings", NULL,
                        "Negative",
#ifdef SYS_MOTION_BLUR
                        "Motion Blur",
#endif
                        "Scaling",
                        "Rotation",
                        "Show FPS",
                        "Zoom on Code",
                        "Back");
    int sel = 0;
    while(1)
    {
        switch(rb->do_menu(&menu, &sel, NULL, false))
        {
        case 0:
            rb->set_bool("Negative", &sys->settings.negative_enabled);
            break;
#ifdef SYS_MOTION_BLUR
        case 1:
            rb->set_bool("Motion Blur", &sys->settings.blur);
            break;
#endif
#ifndef SYS_MOTION_BLUR
        case 1:
#else
        case 2:
#endif
            rb->set_option("Scaling", &sys->settings.scaling_quality, INT, scaling_settings,
#ifdef HAVE_LCD_COLOR
                           3
#else
                           2
#endif
                           , NULL);
            if(sys->settings.scaling_quality &&
               sys->settings.zoom)
            {
                rb->splash(HZ*2, "Zoom automatically disabled.");
                sys->settings.zoom = false;
            }
            break;
#ifndef SYS_MOTION_BLUR
        case 2:
#else
        case 3:
#endif
            rb->set_option("Rotation", &sys->settings.rotation_option, INT, rotation_settings, 3, NULL);
            if(sys->settings.rotation_option &&
               sys->settings.zoom)
            {
                rb->splash(HZ*2, "Zoom automatically disabled.");
                sys->settings.zoom = false;
            }
            sys_rotate_keymap(sys);
            break;
#ifndef SYS_MOTION_BLUR
        case 3:
#else
        case 4:
#endif
            rb->set_bool("Show FPS", &sys->settings.showfps);
            break;
#ifndef SYS_MOTION_BLUR
        case 4:
#else
        case 5:
#endif
            rb->set_bool("Zoom on Code", &sys->settings.zoom);
            /* zoom only works with scaling and rotation disabled */
            if(sys->settings.zoom && (sys->settings.scaling_quality || sys->settings.rotation_option))
            {
                rb->splash(HZ*2, "Scaling and rotation automatically disabled.");
                sys->settings.scaling_quality = 0;
                sys->settings.rotation_option = 0;
            }
            break;
        default:
            sys_save_settings(sys);
            return;
        }
    }
}

#define MAX_SOUNDBUF_SIZE 256

static void do_sound_settings(struct System* sys)
{
    MENUITEM_STRINGLIST(menu, "Sound Settings", NULL,
                        "Enabled",
                        "Buffer Level",
                        "Volume",
                        "Back",
                       );
    int sel = 0;
    while(1)
    {
        switch(rb->do_menu(&menu, &sel, NULL, false))
        {
        case 0:
            rb->set_bool("Enabled", &sys->settings.sound_enabled);
            break;
        case 1:
            rb->set_int("Buffer Level", "samples", UNIT_INT, &sys->settings.sound_bufsize, NULL, 16, 16, MAX_SOUNDBUF_SIZE, NULL);
            break;
        case 2:
        {
            const struct settings_list* vol =
                rb->find_setting(&rb->global_settings->volume, NULL);
            rb->option_screen((struct settings_list*)vol, NULL, false, "Volume");
            break;
        }
        case 3:
        default:
            sys_save_settings(sys);
            return;
        }
    }
}

static void sys_reset_settings(struct System* sys)
{
    sys->settings.negative_enabled = false;
    sys->settings.rotation_option = 0;   // off
    sys->settings.scaling_quality = 1;   // fast
    sys->settings.sound_enabled = true;
    sys->settings.sound_bufsize = 32;    /* keep this low */
    sys->settings.showfps = false;
    sys->settings.zoom = false;
    sys->settings.blur = false;
    sys_rotate_keymap(sys);
}

static struct System* mainmenu_sysptr;

static int mainmenu_cb(int action,
                       const struct menu_item_ex *this_item,
                       struct gui_synclist *this_list)
{
    (void)this_list;
    int idx = ((intptr_t)this_item);
    if(action == ACTION_REQUEST_MENUITEM && !mainmenu_sysptr->loaded && (idx == 0 || idx == 8 || idx == 10))
        return ACTION_EXIT_MENUITEM;
    return action;
}

static AudioCallback audio_callback = NULL;
static void get_more(const void** start, size_t* size);
static void* audio_param;
static struct System* audio_sys;

/************************************** MAIN MENU ***************************************/
/* called after game init */

void sys_menu(struct System* sys)
{
    sys_stopAudio(sys);

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    /* boost for load */
    rb->cpu_boost(true);
#endif

    rb->splash(0, "Loading...");
    sys->loaded = engine_loadGameState(sys->e, 0);

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif

    rb->lcd_update();

    mainmenu_sysptr = sys;
    MENUITEM_STRINGLIST(menu, "XWorld Menu", mainmenu_cb,
                        "Resume Game",          /* 0  */
                        "Start New Game",       /* 1  */
                        "Video Settings",       /* 2  */
                        "Sound Settings",       /* 3  */
                        "Fast Mode",            /* 4  */
                        "Help",                 /* 5  */
                        "Reset Settings",       /* 6  */
                        "Load",                 /* 7  */
                        "Save",                 /* 8  */
                        "Quit without Saving",  /* 9  */
                        "Save and Quit");       /* 10 */
    bool quit = false;
    while(!quit)
    {
        switch(rb->do_menu(&menu, NULL, NULL, false))
        {
        case 0:
            quit = true;
            break;
        case 1:
            vm_initForPart(&sys->e->vm, GAME_PART_FIRST); // This game part is the protection screen
            quit = true;
            break;
        case 2:
            do_video_settings(sys);
            break;
        case 3:
            do_sound_settings(sys);
            break;
        case 4:
            rb->set_bool("Fast Mode", &sys->e->vm._fastMode);
            sys_save_settings(sys);
            break;
        case 5:
            sys_do_help();
            break;
        case 6:
            sys_reset_settings(sys);
            sys_save_settings(sys);
            break;
        case 7:
            rb->splash(0, "Loading...");
            sys->loaded = engine_loadGameState(sys->e, 0);
            rb->lcd_update();
            break;
        case 8:
            sys->e->_stateSlot = 0;
            rb->splash(0, "Saving...");
            engine_saveGameState(sys->e, sys->e->_stateSlot, "quicksave");
            rb->lcd_update();
            break;
        case 9:
            engine_deleteGameState(sys->e, 0);
            exit(PLUGIN_OK);
            break;
        case 10:
            /* saves are NOT deleted on loading */
            exit(PLUGIN_OK);
            break;
        default:
            break;
        }
    }

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    /* boost for game */
    rb->cpu_boost(true);
#endif

    sys_startAudio(sys, audio_callback, audio_param);
}

void sys_init(struct System* sys, const char* title)
{
    (void) title;
#ifdef HAVE_BACKLIGHT
    backlight_ignore_timeout();
#endif
    rb_atexit(exit_handler);
    save_sys = sys;
    rb->memset(&sys->input, 0, sizeof(sys->input));
    sys->mutex_bitfield = ~0;
    if(!sys_load_settings(sys))
    {
        sys_reset_settings(sys);
    }
    struct viewport *vp_main = rb->lcd_set_viewport(NULL);
    lcd_fb = vp_main->buffer->fb_ptr;
}

void sys_destroy(struct System* sys)
{
    (void) sys;
    rb->splash(HZ, "Exiting...");
}

void sys_setPalette(struct System* sys, uint8_t start, uint8_t n, const uint8_t *buf)
{
    for(int i = start; i < start + n; ++i)
    {
        uint8_t c[3];
        for (int j = 0; j < 3; j++) {
            uint8_t col = buf[i * 3 + j];
            c[j] = (col << 2) | (col & 3);
        }
#if (LCD_DEPTH > 24)
        sys->palette[i] = (fb_data) {
            c[2], c[1], c[0], 255
        };
#elif (LCD_DEPTH > 16) && (LCD_DEPTH <= 24)
        sys->palette[i] = (fb_data) {
            c[2], c[1], c[0]
        };
#elif defined(HAVE_LCD_COLOR)
        sys->palette[i] = FB_RGBPACK(c[0], c[1], c[2]);
#elif LCD_DEPTH > 1
        sys->palette[i] = LCD_BRIGHTNESS((c[0] + c[1] + c[2]) / 3);
#endif
    }
}

/****************************** THE MAIN DRAWING METHOD ********************************/

void sys_copyRect(struct System* sys, uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *buf, uint32_t pitch)
{
    static int last_timestamp = 1;

    /* get the address of the temporary framebuffer that has been allocated in the audiobuf */
    fb_data* framebuffer = (fb_data*) sys->e->res._memPtrStart + MEM_BLOCK_SIZE + (4 * VID_PAGE_SIZE);
    buf += y * pitch + x;

    /************************ BLIT THE TEMPORARY FRAMEBUFFER ***********************/

    if(sys->settings.rotation_option)
    {
        /* clockwise */
        if(sys->settings.rotation_option == 1)
        {
            while (h--) {
                /* one byte gives two pixels */
                for (int i = 0; i < w / 2; ++i) {
                    uint8_t pix1 = *(buf + i) >> 4;
                    uint8_t pix2 = *(buf + i) & 0xF;
#if defined(LCD_STRIDEFORMAT) && (LCD_STRIDEFORMAT == VERTICAL_STRIDE)
                    framebuffer[( (h * 2)    ) * 320 + i] = sys->palette[pix1];
                    framebuffer[( (h * 2) + 1) * 320 + i] = sys->palette[pix2];
#else
                    framebuffer[( (i * 2)    ) * 200 + h] = sys->palette[pix1];
                    framebuffer[( (i * 2) + 1) * 200 + h] = sys->palette[pix2];
#endif
                }
                buf += pitch;
            }
        }
        /* counterclockwise */
        else
        {
            while (h--) {
                /* one byte gives two pixels */
                for (int i = 0; i < w / 2; ++i) {
                    uint8_t pix1 = *(buf + i) >> 4;
                    uint8_t pix2 = *(buf + i) & 0xF;
#if defined(LCD_STRIDEFORMAT) && (LCD_STRIDEFORMAT == VERTICAL_STRIDE)
                    framebuffer[(200 - h * 2    ) * 320 + ( 320 - i )] = sys->palette[pix1];
                    framebuffer[(200 - h * 2 - 1) * 320 + ( 320 - i )] = sys->palette[pix2];
#else
                    framebuffer[(320 - i * 2    ) * 200 + ( 200 - h )] = sys->palette[pix1];
                    framebuffer[(320 - i * 2 - 1) * 200 + ( 200 - h )] = sys->palette[pix2];
#endif
                }
                buf += pitch;
            }
        }
    }
    /* no rotation */
    else
    {
        int next = 0;
#if defined(LCD_STRIDEFORMAT) && (LCD_STRIDEFORMAT == VERTICAL_STRIDE)
        for(int x = 0; x < w / 2; ++x)
        {
            for(int y = 0; y < h; ++y)
            {
                uint8_t pix1 = buf[ y * w + x ] >>  4;
                uint8_t pix2 = buf[ y * w + x ] & 0xF;
                framebuffer[(x + 0)*h + y] = sys->palette[pix1];
                framebuffer[(x + 1)*h + y] = sys->palette[pix2];
            }
        }
#else
        while (h--) {
            /* one byte gives two pixels */
            for (int i = 0; i < w / 2; ++i) {
                uint8_t pix1 = *(buf + i) >> 4;
                uint8_t pix2 = *(buf + i) & 0xF;
                framebuffer[next] = sys->palette[pix1];
                ++next;
                framebuffer[next] = sys->palette[pix2];
                ++next;
            }
            buf += pitch;
        }
#endif
    }

    /*************************** NOW SCALE IT! ***************************/

    if(sys->settings.scaling_quality)
    {
        struct bitmap in_bmp;
        if(sys->settings.rotation_option)
        {
#if defined(LCD_STRIDEFORMAT) && (LCD_STRIDEFORMAT == VERTICAL_STRIDE)
            in_bmp.width = 320;
            in_bmp.height = 200;
#else
            in_bmp.width = 200;
            in_bmp.height = 320;
#endif
        }
        else
        {
#if defined(LCD_STRIDEFORMAT) && (LCD_STRIDEFORMAT == VERTICAL_STRIDE)
            in_bmp.width = 200;
            in_bmp.height = 320;
#else
            in_bmp.width = 320;
            in_bmp.height = 200;
#endif
        }
        in_bmp.data = (unsigned char*) framebuffer;
        struct bitmap out_bmp;
        out_bmp.width = LCD_WIDTH;
        out_bmp.height = LCD_HEIGHT;
        out_bmp.data = (unsigned char*) lcd_fb;

#ifdef HAVE_LCD_COLOR
        if(sys->settings.scaling_quality == 1)
#endif
            simple_resize_bitmap(&in_bmp, &out_bmp);
#ifdef HAVE_LCD_COLOR
        else
            smooth_resize_bitmap(&in_bmp, &out_bmp);
#endif
    }
    else
    {
#if defined(LCD_STRIDEFORMAT) && (LCD_STRIDEFORMAT == VERTICAL_STRIDE)
        for(int x = 0; x < 320; ++x)
        {
            for(int y = 0; y < 200; ++y)
            {
                rb->lcd_set_foreground(framebuffer[x * 200 + y]);
                rb->lcd_drawpixel(x, y);
            }
        }
#else
        if(sys->settings.zoom)
        {
            rb->lcd_bitmap_part(framebuffer, CODE_X, CODE_Y, STRIDE(SCREEN_MAIN, 320, 200), 0, 0, 320 - CODE_X, 200 - CODE_Y);
        }
        else
        {
            if(sys->settings.rotation_option)
                rb->lcd_bitmap(framebuffer, 0, 0, 200, 320);
            else
                rb->lcd_bitmap(framebuffer, 0, 0, 320, 200);
        }
#endif
    }

    /************************************* APPLY FILTERS ******************************************/

    if(sys->settings.negative_enabled)
    {
        for(int y = 0; y < LCD_HEIGHT; ++y)
        {
            for(int x = 0; x < LCD_WIDTH; ++x)
            {
#ifdef HAVE_LCD_COLOR
                int r, g, b;
                fb_data pix = lcd_fb[y * LCD_WIDTH + x];
#if (LCD_DEPTH > 24)
                r = 0xff - pix.r;
                g = 0xff - pix.g;
                b = 0xff - pix.b;
                lcd_fb[y * LCD_WIDTH + x] = (fb_data) { b, g, r, 255 };
#elif (LCD_DEPTH == 24)
                r = 0xff - pix.r;
                g = 0xff - pix.g;
                b = 0xff - pix.b;
                lcd_fb[y * LCD_WIDTH + x] = (fb_data) { b, g, r };
#else
                r = RGB_UNPACK_RED  (pix);
                g = RGB_UNPACK_GREEN(pix);
                b = RGB_UNPACK_BLUE (pix);
                lcd_fb[y * LCD_WIDTH + x] = LCD_RGBPACK(0xff - r, 0xff - g, 0xff - b);
#endif
#else
                lcd_fb[y * LCD_WIDTH + x] = LCD_BRIGHTNESS(0xff - lcd_fb[y * LCD_WIDTH + x]);
#endif
            }
        }
    }

#ifdef SYS_MOTION_BLUR
    if(sys->settings.blur)
    {
        static fb_data *prev_frames = NULL;
        static fb_data *orig_fb = NULL;
        static int prev_baseidx = 0; /* circular buffer */
        if(!prev_frames)
        {
            prev_frames = sys_get_buffer(sys, sizeof(fb_data) * LCD_WIDTH * LCD_HEIGHT * BLUR_FRAMES);
            orig_fb = sys_get_buffer(sys, sizeof(fb_data) * LCD_WIDTH * LCD_HEIGHT);

            rb->memset(prev_frames, 0, sizeof(fb_data) * LCD_WIDTH * LCD_HEIGHT * BLUR_FRAMES);
        }
        if(prev_frames && orig_fb)
        {

            rb->memcpy(orig_fb, lcd_fb, sizeof(fb_data) * LCD_WIDTH * LCD_HEIGHT);
            /* fancy useless slow motion blur */
            for(int y = 0; y < LCD_HEIGHT; ++y)
            {
                for(int x = 0; x < LCD_WIDTH; ++x)
                {
                    int r, g, b;
                    fb_data pix = lcd_fb[y * LCD_WIDTH + x];
                    r = RGB_UNPACK_RED  (pix);
                    g = RGB_UNPACK_GREEN(pix);
                    b = RGB_UNPACK_BLUE (pix);
                    r *= BLUR_FRAMES + 1;
                    g *= BLUR_FRAMES + 1;
                    b *= BLUR_FRAMES + 1;
                    for(int i = 0; i < BLUR_FRAMES; ++i)
                    {
                        fb_data prev_pix = prev_frames[ (prev_baseidx + i * (LCD_WIDTH * LCD_HEIGHT)) % (LCD_WIDTH * LCD_HEIGHT * BLUR_FRAMES) + y * LCD_WIDTH + x];
                        r += (BLUR_FRAMES-i) * RGB_UNPACK_RED(prev_pix);
                        g += (BLUR_FRAMES-i) * RGB_UNPACK_GREEN(prev_pix);
                        b += (BLUR_FRAMES-i) * RGB_UNPACK_BLUE(prev_pix);
                    }
                    r /= (BLUR_FRAMES + 1) / 2 * (1 + BLUR_FRAMES + 1);
                    g /= (BLUR_FRAMES + 1) / 2 * (1 + BLUR_FRAMES + 1);
                    b /= (BLUR_FRAMES + 1) / 2 * (1 + BLUR_FRAMES + 1);
                    lcd_fb[y * LCD_WIDTH + x] = LCD_RGBPACK(r, g, b);
                }
            }
            prev_baseidx -= LCD_WIDTH * LCD_HEIGHT;
            if(prev_baseidx < 0)
                prev_baseidx += BLUR_FRAMES * LCD_WIDTH * LCD_HEIGHT;
            rb->memcpy(prev_frames + prev_baseidx, orig_fb, sizeof(fb_data) * LCD_WIDTH * LCD_HEIGHT);
        }
    }
#endif

    /*********************** SHOW FPS *************************/

    int current_time = sys_getTimeStamp(sys);
    if(sys->settings.showfps)
    {
        rb->lcd_set_foreground(LCD_BLACK);
        rb->lcd_set_background(LCD_WHITE);
        /* use 1000 and not HZ here because getTimeStamp is in milliseconds */
        rb->lcd_putsf(0, 0, "FPS: %d", 1000 / ((current_time - last_timestamp == 0) ? 1 : current_time - last_timestamp));
    }
    rb->lcd_update();
    last_timestamp = sys_getTimeStamp(sys);
}

static void do_pause_menu(struct System* sys)
{
    sys_stopAudio(sys);

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif

    int sel = 0;
    MENUITEM_STRINGLIST(menu, "XWorld Menu", NULL,
                        "Resume Game",         /* 0 */
                        "Start New Game",      /* 1 */
                        "Video Settings",      /* 2 */
                        "Sound Settings",      /* 3 */
                        "Fast Mode",           /* 4 */
                        "Enter Code",          /* 5 */
                        "Help",                /* 6 */
                        "Reset Settings",      /* 7 */
                        "Load",                /* 8 */
                        "Save",                /* 9 */
                        "Quit");               /* 10 */

    bool quit = false;
    while(!quit)
    {
        switch(rb->do_menu(&menu, &sel, NULL, false))
        {
        case 0:
            quit = true;
            break;
        case 1:
            vm_initForPart(&sys->e->vm, GAME_PART_FIRST);
            quit = true;
            break;
        case 2:
            do_video_settings(sys);
            break;
        case 3:
            do_sound_settings(sys);
            break;
        case 4:
            rb->set_bool("Fast Mode", &sys->e->vm._fastMode);
            sys_save_settings(sys);
            break;
        case 5:
            sys->input.code = true;
            quit = true;
            break;
        case 6:
            sys_do_help();
            break;
        case 7:
            sys_reset_settings(sys);
            sys_save_settings(sys);
            break;
        case 8:
            rb->splash(0, "Loading...");
            sys->loaded = engine_loadGameState(sys->e, 0);
            rb->lcd_update();
            quit = true;
            break;
        case 9:
            sys->e->_stateSlot = 0;
            rb->splash(0, "Saving...");
            engine_saveGameState(sys->e, sys->e->_stateSlot, "quicksave");
            rb->lcd_update();
            break;
        case 10:
            exit(PLUGIN_OK);
            break;
        }
    }

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif

    sys_startAudio(sys, audio_callback, audio_param);
}

void sys_processEvents(struct System* sys)
{
    int btn = rb->button_status();
    rb->button_clear_queue();

    static int oldbuttonstate = 0;

    debug(DBG_SYS, "button is 0x%08x", btn);

    /* exit early if we can */
    if(btn == oldbuttonstate)
    {
        return;
    }

    /* handle special keys first */
    switch(btn)
    {
    case BTN_PAUSE:
        do_pause_menu(sys);
        sys->input.dirMask = 0;
        sys->input.button = false;
        /* exit early to avoid unwanted button presses being detected */
        return;
    default:
        exit_on_usb(btn);
        break;
    }

    /* Ignore some buttons that cause errant input */

    btn &= ~BUTTON_REDRAW;

#if (CONFIG_KEYPAD == IPOD_4G_PAD) || \
    (CONFIG_KEYPAD == IPOD_3G_PAD) || \
    (CONFIG_KEYPAD == IPOD_1G2G_PAD)
    if(btn & 0x80000000)
        return;
#endif

#if (CONFIG_KEYPAD == SANSA_E200_PAD)
    if(btn == (BUTTON_SCROLL_FWD || BUTTON_SCROLL_BACK))
        return;
#endif

#if (CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD)
    if(btn == (BUTTON_SELECT))
        return;
#endif

    /* copied from doom which was copied from rockboy... */
    unsigned released = ~btn & oldbuttonstate;
    unsigned pressed = btn & ~oldbuttonstate;
    oldbuttonstate = btn;

    if(released)
    {
        if(released & BTN_FIRE)
            sys->input.button = false;
        if(released & sys->keymap.up)
            sys->input.dirMask &= ~DIR_UP;
        if(released & sys->keymap.down)
            sys->input.dirMask &= ~DIR_DOWN;
        if(released & sys->keymap.left)
            sys->input.dirMask &= ~DIR_LEFT;
        if(released & sys->keymap.right)
            sys->input.dirMask &= ~DIR_RIGHT;
#ifdef BTN_DOWN_LEFT
        if(released & sys->keymap.downleft)
            sys->input.dirMask &= ~(DIR_DOWN | DIR_LEFT);
#endif
#ifdef BTN_DOWN_RIGHT
        if(released & sys->keymap.downright)
            sys->input.dirMask &= ~(DIR_DOWN | DIR_RIGHT);
#endif
#ifdef BTN_UP_LEFT
        if(released & sys->keymap.upleft)
            sys->input.dirMask &= ~(DIR_UP | DIR_LEFT);
#endif
#ifdef BTN_UP_RIGHT
        if(released & sys->keymap.upright)
            sys->input.dirMask &= ~(DIR_UP | DIR_RIGHT);
#endif
    }

    if(pressed)
    {
        if(pressed & BTN_FIRE)
            sys->input.button = true;
        if(pressed & sys->keymap.up)
            sys->input.dirMask |= DIR_UP;
        if(pressed & sys->keymap.down)
            sys->input.dirMask |= DIR_DOWN;
        if(pressed & sys->keymap.left)
            sys->input.dirMask |= DIR_LEFT;
        if(pressed & sys->keymap.right)
            sys->input.dirMask |= DIR_RIGHT;
#ifdef BTN_DOWN_LEFT
        if(pressed & sys->keymap.downleft)
            sys->input.dirMask |= (DIR_DOWN | DIR_LEFT);
#endif
#ifdef BTN_DOWN_RIGHT
        if(pressed & sys->keymap.downright)
            sys->input.dirMask |= (DIR_DOWN | DIR_RIGHT);
#endif
#ifdef BTN_UP_LEFT
        if(pressed & sys->keymap.upleft)
            sys->input.dirMask |= (DIR_UP | DIR_LEFT);
#endif
#ifdef BTN_UP_RIGHT
        if(pressed & sys->keymap.upright)
            sys->input.dirMask |= (DIR_UP | DIR_RIGHT);
#endif
    }

#if 0
    /* handle releases */
    if(btn & BUTTON_REL)
    {
        debug(DBG_SYS, "button_rel");
        btn &= ~BUTTON_REL;

        if(btn & BTN_FIRE)
            sys->input.button = false;
        if(btn & sys->keymap.up)
            sys->input.dirMask &= ~DIR_UP;
        if(btn & sys->keymap.down)
            sys->input.dirMask &= ~DIR_DOWN;
        if(btn & sys->keymap.left)
            sys->input.dirMask &= ~DIR_LEFT;
        if(btn & sys->keymap.right)
            sys->input.dirMask &= ~DIR_RIGHT;
#ifdef BTN_DOWN_LEFT
        if(btn & sys->keymap.downleft)
            sys->input.dirMask &= ~(DIR_DOWN | DIR_LEFT);
#endif
#ifdef BTN_DOWN_RIGHT
        if(btn & sys->keymap.downright)
            sys->input.dirMask &= ~(DIR_DOWN | DIR_RIGHT);
#endif
#ifdef BTN_UP_LEFT
        if(btn & sys->keymap.upleft)
            sys->input.dirMask &= ~(DIR_UP | DIR_LEFT);
#endif
#ifdef BTN_UP_RIGHT
        if(btn & sys->keymap.upright)
            sys->input.dirMask &= ~(DIR_UP | DIR_RIGHT);
#endif
    }
    /* then handle presses */
    else
    {
        if(btn & BTN_FIRE)
            sys->input.button = true;
        if(btn & sys->keymap.up)
            sys->input.dirMask |= DIR_UP;
        if(btn & sys->keymap.down)
            sys->input.dirMask |= DIR_DOWN;
        if(btn & sys->keymap.left)
            sys->input.dirMask |= DIR_LEFT;
        if(btn & sys->keymap.right)
            sys->input.dirMask |= DIR_RIGHT;
#ifdef BTN_DOWN_LEFT
        if(btn & sys->keymap.downleft)
            sys->input.dirMask |= (DIR_DOWN | DIR_LEFT);
#endif
#ifdef BTN_DOWN_RIGHT
        if(btn & sys->keymap.downright)
            sys->input.dirMask |= (DIR_DOWN | DIR_RIGHT);
#endif
#ifdef BTN_UP_LEFT
        if(btn & sys->keymap.upleft)
            sys->input.dirMask |= (DIR_UP | DIR_LEFT);
#endif
#ifdef BTN_UP_RIGHT
        if(btn & sys->keymap.upright)
            sys->input.dirMask |= (DIR_UP | DIR_RIGHT);
#endif
    }
    debug(DBG_SYS, "dirMask is 0x%02x", sys->input.dirMask);
    debug(DBG_SYS, "button is %s", sys->input.button == true ? "true" : "false");
#endif
}

void sys_sleep(struct System* sys, uint32_t duration)
{
    (void) sys;
    /* duration is in ms */
    rb->sleep(duration / (1000/HZ));
}

uint32_t sys_getTimeStamp(struct System* sys)
{
    (void) sys;
    return (uint32_t) (*rb->current_tick * (1000/HZ));
}

/* game provides us mono samples, we need stereo */
static int16_t rb_soundbuf[MAX_SOUNDBUF_SIZE * 2];
static int8_t temp_soundbuf[MAX_SOUNDBUF_SIZE];

static void get_more(const void** start, size_t* size)
{
    if(audio_sys->settings.sound_enabled && audio_callback)
    {
        audio_callback(audio_param, temp_soundbuf, audio_sys->settings.sound_bufsize);

        /* convert xworld format (signed 8-bit) to rockbox format (stereo signed 16-bit) */
        for(int i = 0; i < audio_sys->settings.sound_bufsize; ++i)
        {
            rb_soundbuf[2*i] = rb_soundbuf[2*i+1] = temp_soundbuf[i] * 256;
        }
    }
    else
    {
        rb->memset(rb_soundbuf, 0, audio_sys->settings.sound_bufsize * 2 * sizeof(int16_t));
    }
    *start = rb_soundbuf;
    *size = audio_sys->settings.sound_bufsize * 2 * sizeof(int16_t);
}

void sys_startAudio(struct System* sys, AudioCallback callback, void *param)
{
    (void) sys;
    audio_callback = callback;
    audio_param = param;
    audio_sys = sys;

    rb->pcm_play_data(get_more, NULL, NULL, 0);
}

void sys_stopAudio(struct System* sys)
{
    (void) sys;
    rb->pcm_play_stop();
}

uint32_t sys_getOutputSampleRate(struct System* sys)
{
    (void) sys;
    return rb->mixer_get_frequency();
}

/* pretty non-reentrant here, but who cares? it's not like someone
   would want to run two instances of the game on Rockbox! :D */

static uint32_t cur_delay;
static void* cur_param;
static TimerCallback user_callback;
static void timer_callback(void)
{
    debug(DBG_SYS, "timer callback with delay of %d ms callback 0x%08x param 0x%08x", cur_delay, timer_callback, cur_param);
    uint32_t ret = user_callback(cur_delay, cur_param);
    if(ret != cur_delay)
    {
        cur_delay = ret;
        rb->timer_register(1, NULL, TIMER_FREQ / 1000 * ret, timer_callback IF_COP(, CPU));
    }
}

void *sys_addTimer(struct System* sys, uint32_t delay, TimerCallback callback, void *param)
{
    (void) sys;
    debug(DBG_SYS, "registering timer with delay of %d ms callback 0x%08x param 0x%08x", delay, callback, param);
    user_callback = callback;
    cur_delay = delay;
    cur_param = param;
    rb->timer_register(1, NULL, TIMER_FREQ / 1000 * delay, timer_callback IF_COP(, CPU));
    return NULL;
}

void sys_removeTimer(struct System* sys, void *timerId)
{
    (void) sys;
    (void) timerId;
    /* there's only one timer per game instance, so ignore both parameters */
    rb->timer_unregister();
}

void *sys_createMutex(struct System* sys)
{
    if(!sys)
        error("sys is NULL!");

    debug(DBG_SYS, "allocating mutex");

    /* this bitfield works as follows: bit set = free, unset = in use */
    for(int i = 0; i < MAX_MUTEXES; ++i)
    {
        /* check that the corresponding bit is 1 (free) */
        if(sys->mutex_bitfield & (1 << i))
        {
            rb->mutex_init(sys->mutex_memory + i);
            sys->mutex_bitfield &= ~(1 << i);
            return sys->mutex_memory + i;
        }
    }
    warning("Out of mutexes!");
    return NULL;
}

void sys_destroyMutex(struct System* sys, void *mutex)
{
    int mutex_number = ((char*)mutex - (char*)sys->mutex_memory) / sizeof(struct mutex); /* pointer arithmetic! check for bugs! */
    sys->mutex_bitfield |= 1 << mutex_number;
}

void sys_lockMutex(struct System* sys, void *mutex)
{
    (void) sys;
    rb->mutex_lock((struct mutex*) mutex);
}

void sys_unlockMutex(struct System* sys, void *mutex)
{
    (void) sys;
    rb->mutex_unlock((struct mutex*) mutex);
}

uint8_t* getOffScreenFramebuffer(struct System* sys)
{
    (void) sys;
    return NULL;
}

void *sys_get_buffer(struct System* sys, size_t sz)
{
    if((signed int)sys->bytes_left - (signed int)sz >= 0)
    {
        void* ret = sys->membuf;
        rb->memset(ret, 0, sz);
        sys->membuf = (char*)(sys->membuf) + sz;
        sys->bytes_left -= sz;
        return ret;
    }
    else
    {
        error("sys_get_buffer: out of memory!");
        return NULL;
    }
}

void MutexStack(struct MutexStack_t* s, struct System *stub, void *mutex)
{
    s->sys = stub;
    s->_mutex = mutex;
    /* FW 2017-2-12: disabled; no blocking ops in IRQ context! */
    /*sys_lockMutex(s->sys, s->_mutex);*/
}

void MutexStack_destroy(struct MutexStack_t* s)
{
    (void) s;
    /*sys_unlockMutex(s->sys, s->_mutex);*/
}
