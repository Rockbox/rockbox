/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: eq_menu.c 12179 2007-02-01 23:08:15Z amiconn $
 *
 * Copyright (C) 2006 Dan Everton, 2009 Carsten Schreiter, 2015 Chiwen Chang
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

#include "config.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "eq_menu.h"
#include "system.h"
#include "kernel.h"
#include "lcd.h"
#include "menu.h"
#include "action.h"
#include "mp3_playback.h"
#include "settings.h"
#include "screens.h"
#include "icons.h"
#include "font.h"
#include "lang.h"
#include "talk.h"
#include "misc.h"
#include "sound.h"
#include "dsp_proc_settings.h"
#include "tree.h"
#include "screen_access.h"
#include "keyboard.h"
#include "gui/scrollbar.h"
#include "menu_common.h"
#include "viewport.h"
#include "stdlib.h"
#include "exported_menus.h"

/*
 * Utility functions
 */

const char* eq_q_format(char* buffer, size_t buffer_size, int value, const char* unit)
{
    snprintf(buffer, buffer_size, "%d.%d %s", value / EQ_USER_DIVISOR,
         value % EQ_USER_DIVISOR, unit);
    return buffer;
}

const char* eq_precut_format(char* buffer, size_t buffer_size, int value, const char* unit)
{
    snprintf(buffer, buffer_size, "%s%d.%d %s", value == 0 ? " " : "-",
        value / EQ_USER_DIVISOR, value % EQ_USER_DIVISOR, unit);
    return buffer;
}

/*
 * Settings functions
 */
static void eq_apply(void)
{
    dsp_eq_enable(global_settings.eq_enabled);
    dsp_set_eq_precut(global_settings.eq_precut);
    /* Update all bands */
    for(int i = 0; i < EQ_NUM_BANDS; i++) {
        dsp_set_eq_coefs(i, &global_settings.eq_band_settings[i]);
    }
}

static int eq_setting_callback(int action, const struct menu_item_ex *this_item)
{
    switch (action)
    {
        case ACTION_ENTER_MENUITEM:
            action = lowlatency_callback(action, this_item);
            break;
        case ACTION_EXIT_MENUITEM:
            eq_apply();
            action = lowlatency_callback(action, this_item);
            break;
    }

    return action;
}
MENUITEM_SETTING(eq_enable, &global_settings.eq_enabled, eq_setting_callback);
MENUITEM_SETTING(eq_precut, &global_settings.eq_precut, eq_setting_callback);

MENUITEM_SETTING(cutoff_0, &global_settings.eq_band_settings[0].cutoff, eq_setting_callback);
MENUITEM_SETTING(cutoff_1, &global_settings.eq_band_settings[1].cutoff, eq_setting_callback);
MENUITEM_SETTING(cutoff_2, &global_settings.eq_band_settings[2].cutoff, eq_setting_callback);
MENUITEM_SETTING(cutoff_3, &global_settings.eq_band_settings[3].cutoff, eq_setting_callback);
MENUITEM_SETTING(cutoff_4, &global_settings.eq_band_settings[4].cutoff, eq_setting_callback);
MENUITEM_SETTING(cutoff_5, &global_settings.eq_band_settings[5].cutoff, eq_setting_callback);
MENUITEM_SETTING(cutoff_6, &global_settings.eq_band_settings[6].cutoff, eq_setting_callback);
MENUITEM_SETTING(cutoff_7, &global_settings.eq_band_settings[7].cutoff, eq_setting_callback);
MENUITEM_SETTING(cutoff_8, &global_settings.eq_band_settings[8].cutoff, eq_setting_callback);
MENUITEM_SETTING(cutoff_9, &global_settings.eq_band_settings[9].cutoff, eq_setting_callback);

MENUITEM_SETTING(q_0, &global_settings.eq_band_settings[0].q, eq_setting_callback);
MENUITEM_SETTING(q_1, &global_settings.eq_band_settings[1].q, eq_setting_callback);
MENUITEM_SETTING(q_2, &global_settings.eq_band_settings[2].q, eq_setting_callback);
MENUITEM_SETTING(q_3, &global_settings.eq_band_settings[3].q, eq_setting_callback);
MENUITEM_SETTING(q_4, &global_settings.eq_band_settings[4].q, eq_setting_callback);
MENUITEM_SETTING(q_5, &global_settings.eq_band_settings[5].q, eq_setting_callback);
MENUITEM_SETTING(q_6, &global_settings.eq_band_settings[6].q, eq_setting_callback);
MENUITEM_SETTING(q_7, &global_settings.eq_band_settings[7].q, eq_setting_callback);
MENUITEM_SETTING(q_8, &global_settings.eq_band_settings[8].q, eq_setting_callback);
MENUITEM_SETTING(q_9, &global_settings.eq_band_settings[9].q, eq_setting_callback);

MENUITEM_SETTING(gain_0, &global_settings.eq_band_settings[0].gain, eq_setting_callback);
MENUITEM_SETTING(gain_1, &global_settings.eq_band_settings[1].gain, eq_setting_callback);
MENUITEM_SETTING(gain_2, &global_settings.eq_band_settings[2].gain, eq_setting_callback);
MENUITEM_SETTING(gain_3, &global_settings.eq_band_settings[3].gain, eq_setting_callback);
MENUITEM_SETTING(gain_4, &global_settings.eq_band_settings[4].gain, eq_setting_callback);
MENUITEM_SETTING(gain_5, &global_settings.eq_band_settings[5].gain, eq_setting_callback);
MENUITEM_SETTING(gain_6, &global_settings.eq_band_settings[6].gain, eq_setting_callback);
MENUITEM_SETTING(gain_7, &global_settings.eq_band_settings[7].gain, eq_setting_callback);
MENUITEM_SETTING(gain_8, &global_settings.eq_band_settings[8].gain, eq_setting_callback);
MENUITEM_SETTING(gain_9, &global_settings.eq_band_settings[9].gain, eq_setting_callback);

static char* gainitem_get_name(int selected_item, void * data, char *buffer)
{
    (void)selected_item;
    int *setting = (int*)data;
    snprintf(buffer, MAX_PATH, str(LANG_EQUALIZER_GAIN_ITEM), *setting);
    return buffer;
}

static int gainitem_speak_item(int selected_item, void * data)
{
    (void)selected_item;
    int *setting = (int*)data;
    talk_number(*setting, false);
    talk_id(LANG_EQUALIZER_GAIN_ITEM, true);
    return 0;
}

static int do_option(void * param)
{
    const struct menu_item_ex *setting = (const struct menu_item_ex*)param;
    lowlatency_callback(ACTION_ENTER_MENUITEM, setting);
    do_setting_from_menu(setting, NULL);
    eq_apply();
    lowlatency_callback(ACTION_EXIT_MENUITEM, setting);
    return 0;
}

MENUITEM_FUNCTION_DYNTEXT(gain_item_0, MENU_FUNC_USEPARAM,
                          do_option, (void*)&gain_0,
                          gainitem_get_name, gainitem_speak_item,
                          &global_settings.eq_band_settings[0].cutoff, NULL, Icon_NOICON);
MENUITEM_FUNCTION_DYNTEXT(gain_item_1, MENU_FUNC_USEPARAM,
                          do_option, (void*)&gain_1,
                          gainitem_get_name, gainitem_speak_item,
                          &global_settings.eq_band_settings[1].cutoff, NULL, Icon_NOICON);
MENUITEM_FUNCTION_DYNTEXT(gain_item_2, MENU_FUNC_USEPARAM,
                          do_option, (void*)&gain_2,
                          gainitem_get_name, gainitem_speak_item,
                          &global_settings.eq_band_settings[2].cutoff, NULL, Icon_NOICON);
MENUITEM_FUNCTION_DYNTEXT(gain_item_3, MENU_FUNC_USEPARAM,
                          do_option, (void*)&gain_3,
                          gainitem_get_name, gainitem_speak_item,
                          &global_settings.eq_band_settings[3].cutoff, NULL, Icon_NOICON);
MENUITEM_FUNCTION_DYNTEXT(gain_item_4, MENU_FUNC_USEPARAM,
                          do_option, (void*)&gain_4,
                          gainitem_get_name, gainitem_speak_item,
                          &global_settings.eq_band_settings[4].cutoff, NULL, Icon_NOICON);
MENUITEM_FUNCTION_DYNTEXT(gain_item_5, MENU_FUNC_USEPARAM,
                          do_option, (void*)&gain_5,
                          gainitem_get_name, gainitem_speak_item,
                          &global_settings.eq_band_settings[5].cutoff, NULL, Icon_NOICON);
MENUITEM_FUNCTION_DYNTEXT(gain_item_6, MENU_FUNC_USEPARAM,
                          do_option, (void*)&gain_6,
                          gainitem_get_name, gainitem_speak_item,
                          &global_settings.eq_band_settings[6].cutoff, NULL, Icon_NOICON);
MENUITEM_FUNCTION_DYNTEXT(gain_item_7, MENU_FUNC_USEPARAM,
                          do_option, (void*)&gain_7,
                          gainitem_get_name, gainitem_speak_item,
                          &global_settings.eq_band_settings[7].cutoff, NULL, Icon_NOICON);
MENUITEM_FUNCTION_DYNTEXT(gain_item_8, MENU_FUNC_USEPARAM,
                          do_option, (void*)&gain_8,
                          gainitem_get_name, gainitem_speak_item,
                          &global_settings.eq_band_settings[8].cutoff, NULL, Icon_NOICON);
MENUITEM_FUNCTION_DYNTEXT(gain_item_9, MENU_FUNC_USEPARAM,
                          do_option, (void*)&gain_9,
                          gainitem_get_name, gainitem_speak_item,
                          &global_settings.eq_band_settings[9].cutoff, NULL, Icon_NOICON);

MAKE_MENU(gain_menu, ID2P(LANG_EQUALIZER_GAIN), NULL, Icon_NOICON, &gain_item_0,
            &gain_item_1, &gain_item_2, &gain_item_3, &gain_item_4,&gain_item_5,&gain_item_6,
            &gain_item_7,&gain_item_8,&gain_item_9);

static const struct menu_item_ex *band_items[EQ_NUM_BANDS-2][3] = {
    { &cutoff_1, &q_1, &gain_1 },
    { &cutoff_2, &q_2, &gain_2 },
    { &cutoff_3, &q_3, &gain_3 },
    { &cutoff_4, &q_4, &gain_4 },
    { &cutoff_5, &q_5, &gain_5 },
    { &cutoff_6, &q_6, &gain_6 },
    { &cutoff_7, &q_7, &gain_7 },
    { &cutoff_8, &q_8, &gain_8 }
};


/* variables used to store buttonbar, band and slider position */
struct button_data{
    int x;
    int width;
};

struct button_bar{
    struct button_data ok;
    struct button_data cancel;
    struct button_data edit_mode;
    struct button_data inc;
    struct button_data dec;
    short height;
    short y;
};

struct eq_screen_data{
    struct eq_slider_data{
        short x;
        short y;
        short height;
        short length;
    }slider;
    struct eq_txt_data{
        short x;
        short y[EQ_NUM_BANDS];
        short height;
        short width;
    }band;
};

static char* centerband_get_name(int selected_item, void * data, char *buffer)
{
    (void)selected_item;
    int band = (intptr_t)data;
    snprintf(buffer, MAX_PATH, str(LANG_EQUALIZER_BAND_PEAK), band);
    return buffer;
}

static int centerband_speak_item(int selected_item, void * data)
{
    (void)selected_item;
    int band = (intptr_t)data;
    talk_id(LANG_EQUALIZER_BAND_PEAK, false);
    talk_number(band, true);
    return 0;
}

static int do_center_band_menu(void* param)
{
    int band = (intptr_t)param;
    struct menu_item_ex menu;
    struct menu_callback_with_desc cb_and_desc;
    char desc[MAX_PATH];

    cb_and_desc.menu_callback = NULL;
    snprintf(desc, MAX_PATH, str(LANG_EQUALIZER_BAND_PEAK), band);
    cb_and_desc.desc = desc;
    cb_and_desc.icon_id = Icon_EQ;
    menu.flags = MT_MENU|(3<<MENU_COUNT_SHIFT)|MENU_HAS_DESC;
    menu.submenus = band_items[band-1];
    menu.callback_and_desc = &cb_and_desc;
    do_menu(&menu, NULL, NULL, false);
    return 0;
}

MAKE_MENU(band_0_menu, ID2P(LANG_EQUALIZER_BAND_LOW_SHELF), NULL,
            Icon_EQ, &cutoff_0, &q_0, &gain_0);
MENUITEM_FUNCTION_DYNTEXT(band_1_menu, MENU_FUNC_USEPARAM,
                          do_center_band_menu, (void*)1,
                          centerband_get_name, centerband_speak_item,
                          (void*)1, NULL, Icon_EQ);
MENUITEM_FUNCTION_DYNTEXT(band_2_menu, MENU_FUNC_USEPARAM,
                          do_center_band_menu, (void*)2,
                          centerband_get_name, centerband_speak_item,
                          (void*)2, NULL, Icon_EQ);
MENUITEM_FUNCTION_DYNTEXT(band_3_menu, MENU_FUNC_USEPARAM,
                          do_center_band_menu, (void*)3,
                          centerband_get_name, centerband_speak_item,
                          (void*)3, NULL, Icon_EQ);
MENUITEM_FUNCTION_DYNTEXT(band_4_menu, MENU_FUNC_USEPARAM,
                          do_center_band_menu, (void*)4,
                          centerband_get_name, centerband_speak_item,
                          (void*)4, NULL, Icon_EQ);
MENUITEM_FUNCTION_DYNTEXT(band_5_menu, MENU_FUNC_USEPARAM,
                          do_center_band_menu, (void*)5,
                          centerband_get_name, centerband_speak_item,
                          (void*)5, NULL, Icon_EQ);
MENUITEM_FUNCTION_DYNTEXT(band_6_menu, MENU_FUNC_USEPARAM,
                          do_center_band_menu, (void*)6,
                          centerband_get_name, centerband_speak_item,
                          (void*)6, NULL, Icon_EQ);
MENUITEM_FUNCTION_DYNTEXT(band_7_menu, MENU_FUNC_USEPARAM,
                          do_center_band_menu, (void*)7,
                          centerband_get_name, centerband_speak_item,
                          (void*)7, NULL, Icon_EQ);
MENUITEM_FUNCTION_DYNTEXT(band_8_menu, MENU_FUNC_USEPARAM,
                          do_center_band_menu, (void*)8,
                          centerband_get_name, centerband_speak_item,
                          (void*)8, NULL, Icon_EQ);
MAKE_MENU(band_9_menu, ID2P(LANG_EQUALIZER_BAND_HIGH_SHELF), NULL,
            Icon_EQ, &cutoff_9, &q_9, &gain_9);

MAKE_MENU(advanced_eq_menu_, ID2P(LANG_EQUALIZER_ADVANCED), NULL, Icon_EQ,
            &band_0_menu, &band_1_menu, &band_2_menu, &band_3_menu, &band_4_menu,
            &band_5_menu, &band_6_menu, &band_7_menu, &band_8_menu, &band_9_menu);


enum eq_slider_mode {
    GAIN,
    CUTOFF,
    Q,
};

static void save_eq_settings(int *eq_settings)
{
    int i;
    int *setting = &global_settings.eq_band_settings[0].cutoff;
    for (i = 0; i < EQ_NUM_BANDS * 3; i ++)
    {
        eq_settings[i] = setting[i];
    }
    return;
}

static void restore_eq_settings(int *eq_settings)
{
    int i;
    int *setting = &global_settings.eq_band_settings[0].cutoff;
    for (i = 0; i < EQ_NUM_BANDS * 3; i++)
    {
        setting[i] =  eq_settings[i];
    }
    for (i = 0; i < EQ_NUM_BANDS; i++)
    {
        dsp_set_eq_coefs(i, &global_settings.eq_band_settings[i]);
    }
    return;
}

/* left and right border the eq screen */
#define EQ_OUTER_MARGIN 14
/* to let reach the max setting shortly before slider end for better control */
#define SLIDER_TOUCH_MARGIN 5
#define BUTTON_BAR_MARGIN_BOTTOM 7
#define SLIDER_WIDTH 20

static void draw_slider_bar(struct screen * screen, int x, int y,
                        int width, int height, int value,
                        int min, int max)
{
    int inner_x, inner_y, inner_wd, inner_ht;
    int start, size;
    short edge_reached = 0;
#ifdef HAVE_LCD_COLOR
    int infill;
#endif

#define LEFT_EDGE  1
#define RIGHT_EDGE 2

    inner_x  = x + 1;
    inner_y  = y + 1;
    inner_wd = width  - 2;
    inner_ht = height - 2;

    size = SLIDER_WIDTH;
    start = inner_x + ((value - min) * inner_wd / (max - min)) - SLIDER_WIDTH / 2;

    if (start <= inner_x)
    {
        size = start + SLIDER_WIDTH - inner_x;
        start = inner_x;
        edge_reached = LEFT_EDGE;
    }

    if (start > inner_x + inner_wd - SLIDER_WIDTH)
    {
        size = inner_x + inner_wd - start;
        edge_reached = RIGHT_EDGE;
    }

    /* draw box */
#ifdef HAVE_LCD_COLOR
    /* must avoid corners if case of (flags & FOREGROUND) */
    screen->hline(inner_x, x + inner_wd, y);
    screen->hline(inner_x, x + inner_wd, y + height - 1);
    screen->vline(x, inner_y, y + inner_ht);
    screen->vline(x + width - 1, inner_y, y + inner_ht);
#else
    screen->drawrect(x, y, width, height);
#endif

    screen->set_drawmode(DRMODE_SOLID | DRMODE_INVERSEVID);

#ifdef HAVE_LCD_COLOR
    infill = (screen->depth > 1 ? INNER_FILL_MASK : INNER_FILL);

#endif
    /* clear corner pixels */
    screen->drawpixel(x, y);
    screen->drawpixel(x + width - 1, y);
    screen->drawpixel(x, y + height - 1);
    screen->drawpixel(x + width - 1, y + height - 1);

#ifdef HAVE_LCD_COLOR
    if (infill != INNER_BGFILL)
        infill = INNER_FILL;

    if (infill == INNER_FILL)
#endif
    {
        /* clear pixels in progress bar */
        screen->fillrect(inner_x, inner_y, inner_wd, inner_ht);
    }

    screen->set_drawmode(DRMODE_SOLID);

#ifdef HAVE_LCD_COLOR
    if (infill == INNER_BGFILL)
    {
        /* fill inner area with current background color */
        unsigned fg = screen->get_foreground();
        screen->set_foreground(screen->get_background());
        screen->fillrect(inner_x, inner_y, inner_wd, inner_ht);
        screen->set_foreground(fg);
    }
#endif
    /* draw slider */
    screen->fillrect(start, inner_y, size, inner_ht);

    /* clear corner pixels of slider */
    screen->set_drawmode(DRMODE_SOLID | DRMODE_INVERSEVID);
    if (edge_reached != LEFT_EDGE)
    {
        screen->drawpixel(start, inner_y);
        screen->drawpixel(start, inner_y + inner_ht - 1);
    }
    if (edge_reached != RIGHT_EDGE)
    {
        screen->drawpixel(start + size - 1, inner_y);
        screen->drawpixel(start + size - 1, inner_y + inner_ht - 1);
    }
    screen->set_drawmode(DRMODE_SOLID);
}

/* Draw's all the EQ sliders. */
static void draw_eq_sliders(struct screen * screen,
                            struct viewport *vp,
                            struct eq_screen_data *eq_data,
                            int current_band,
                            enum eq_slider_mode mode)
{
    int band, w, h, gain, q, cutoff;
    int *setting = &global_settings.eq_band_settings[0].cutoff;
    char buf[26];
    int value, min_item, max_item;
    int x1, x2, y1;

    screen->set_viewport(vp);

    for (band = 0; band < EQ_NUM_BANDS; band ++) {
        cutoff = *setting++;
        q = *setting++;
        gain = *setting++;

        switch(mode) {
        case Q:
            value = q;
            min_item = EQ_Q_MIN;
            max_item = EQ_Q_MAX;
            break;
        case CUTOFF:
            value = cutoff;
            min_item = EQ_CUTOFF_MIN;
            max_item = EQ_CUTOFF_MAX;
            break;
        case GAIN:
        default:
            value = gain;
            min_item = EQ_GAIN_MIN;
            max_item = EQ_GAIN_MAX;
            break;
        }

        /* Start two pixels in, one for border, one for margin */
        x1 = (int)eq_data->band.x + 2;
        y1 = (int)eq_data->band.y[band] + 2;

        /* Print out the band label */
        if (band == 0) {
            screen->putsxy(x1, y1, "LS: ");
            screen->getstringsize("LS:", &w, &h);
        } else if (band == EQ_NUM_BANDS - 1) {
            screen->putsxy(x1, y1, "HS: ");
            screen->getstringsize("HS:", &w, &h);
        } else {
            snprintf(buf, sizeof(buf),  "PK%d:", band);
            screen->putsxy(x1, y1, buf);
            screen->getstringsize(buf, &w, &h);
        }

        screen->getstringsize("A", &w, &h);
        x1 += 5*w; /* 4 chars for label + 1 space = 5 */

        /* Print out gain part of status line (left justify after label) */
        if (mode == GAIN && band == current_band)
            screen->set_drawmode(DRMODE_SOLID | DRMODE_INVERSEVID);
        else
            screen->set_drawmode(DRMODE_SOLID);

        snprintf(buf, sizeof(buf), "%s%2d.%d%s", gain < 0 ? "-" : " ",
            abs(gain) / EQ_USER_DIVISOR, abs(gain) % EQ_USER_DIVISOR,"dB");
        screen->putsxy(x1, y1, buf);
        screen->getstringsize(buf, &w, &h);

        x1 += w;

        /* Print out Q part of status line (right justify) */
        if (mode == Q && band == current_band)
            screen->set_drawmode(DRMODE_SOLID | DRMODE_INVERSEVID);
        else
            screen->set_drawmode(DRMODE_SOLID);

        snprintf(buf, sizeof(buf), "%d.%d%s", q / EQ_USER_DIVISOR,
                 q % EQ_USER_DIVISOR, " Q");
        screen->getstringsize(buf, &w, &h);
        x2 = eq_data->band.x +  eq_data->band.width - w - 2;
        screen->putsxy(x2, y1, buf);

        /* Print out cutoff part of status line (center between gain & Q) */
        if (mode == CUTOFF && band == current_band)
            screen->set_drawmode(DRMODE_SOLID | DRMODE_INVERSEVID);
        else
            screen->set_drawmode(DRMODE_SOLID);

        snprintf(buf, sizeof(buf),  "%5d%s", cutoff, "Hz");
        screen->getstringsize(buf, &w, &h);
        x1 = x1 + (x2 - x1 - w)/2;
        screen->putsxy(x1, y1, buf);

        screen->set_drawmode(DRMODE_SOLID);

        /* Draw horizontal slider */
        draw_slider_bar(screen, eq_data->band.x, eq_data->band.y[band] + eq_data->slider.y,
                           eq_data->band.width, eq_data->slider.height, value,
                           min_item, max_item);
    }
    return;
}

/* draws a rectangle around a button */
static void draw_button (struct screen *screen, char *buf,
                         int x, int y, int width, int height)
{
    int txt_w, txt_h;

    /* draw button */
    screen->drawrect(x, y, width, height);

    /* clear corner pixels of button */
    screen->set_drawmode(DRMODE_SOLID | DRMODE_INVERSEVID);
    screen->drawpixel(x, y);
    screen->drawpixel(x + width - 1, y);
    screen->drawpixel(x, y + height - 1);
    screen->drawpixel(x + width -1, y + height -1);
    screen->set_drawmode(DRMODE_SOLID);

    /* draw button text */
    screen->getstringsize(buf, &txt_w, &txt_h);
    screen->putsxy(x + (width - txt_w)/2 , (height - txt_h)/2, buf);

}

/* draws the button bar at the top of the screen */
static void draw_button_bar(struct screen *screen,
                            struct viewport *vp,
                            enum eq_slider_mode mode,
                            struct button_bar *btn_bar)
{

    char buf[24];

    screen->set_viewport(vp);
    screen->set_drawmode(DRMODE_SOLID);

    /* Edit mode button */
    if (mode == CUTOFF) {
        /* cutoff */
        snprintf(buf, sizeof(buf), "Mode: %s %s",
                         str(LANG_SYSFONT_EQUALIZER_BAND_CUTOFF), "(Hz)");
    }
    else if (mode == GAIN) {
        /* gain */
        snprintf(buf, sizeof(buf), "Mode: %s %s",
                 str(LANG_SYSFONT_GAIN), "(dB)");
    }
    else {
            /* Q */
        snprintf(buf, sizeof(buf), "Mode: %s %s",
                 str(LANG_EQUALIZER_BAND_Q), "");
    }
    draw_button(screen, buf, btn_bar->edit_mode.x, 0,
                        btn_bar->edit_mode.width, btn_bar->height);

    /* Cancel button */
    snprintf(buf, sizeof(buf), str(LANG_KBD_CANCEL), NULL, NULL);
    draw_button(screen, buf, btn_bar->cancel.x, 0,
                        btn_bar->cancel.width, btn_bar->height);

    /* Ok button */
    snprintf(buf, sizeof(buf), str(LANG_KBD_OK), NULL, NULL);
    draw_button(screen, buf, btn_bar->ok.x, 0,
                        btn_bar->ok.width, btn_bar->height);
    /*inc button*/
    snprintf(buf, sizeof(buf), " + ");
    draw_button(screen, buf, btn_bar->inc.x, btn_bar->height,
                        btn_bar->inc.width, vp->height - btn_bar->height);
    /*dec button*/
    snprintf(buf, sizeof(buf), " - ");
    draw_button(screen, buf, btn_bar->dec.x, btn_bar->height,
                        btn_bar->dec.width, vp->height - btn_bar->height);
    return;
}

#ifdef HAVE_REMOTE_LCD
void draw_remote_screen(struct screen * screen,
                        struct viewport *vp,
                        int current_band,
                        enum eq_slider_mode mode,
                        int setting)
{
    int x, h, y;
    char buf[10];

    screen->set_viewport(vp);

    screen->getstringsize("PK  ", &x, &h);
    y = (vp->height - h) / 2;

    if (current_band == 0) {
        screen->putsxy(2, y, "LS ");
    } else if (current_band == EQ_NUM_BANDS - 1) {
        screen->putsxy(2, y, "HS ");
    } else {
        snprintf(buf, sizeof(buf),  "PK%d", current_band);
        screen->putsxy(2, y, buf);
    }

    switch(mode) {
        case Q:
            snprintf (buf, sizeof(buf), "-Q: %d.%d", setting / EQ_USER_DIVISOR,
                 setting % EQ_USER_DIVISOR);
            break;
        case CUTOFF:
            snprintf (buf, sizeof(buf), "-C: %5d", setting);
            break;
        case GAIN:
        default:
            snprintf (buf, sizeof(buf), "-G: %s%2d.%d", setting < 0 ? "-" : " ",
                abs(setting) / EQ_USER_DIVISOR, abs(setting) % EQ_USER_DIVISOR);
            break;
        }
    screen->putsxy(x, y, buf);
}
#endif

/* calculates buttonbar, band and slider postionions and sizes */
static bool init_eq_screen(struct screen * screen,
                           struct viewport *vp,
                           struct button_bar *btn_bar,
                           struct eq_screen_data  *eq_data)
{
    int w, w1, h, i;
    char buf[24];

    btn_bar->y = 0;
    /* check if the button bar fits in the viewport*/

    /* Edit mode button */
    /* don't know which setting is the longest in each language, therefore */
    /* check all 3 settings*/
    snprintf(buf, sizeof(buf),"Mode: %s %s",
                         str(LANG_SYSFONT_EQUALIZER_BAND_CUTOFF), "(Hz)");
    screen->getstringsize(buf, &w1, &h);
    w = w1;

    snprintf(buf, sizeof(buf), "Mode: %s %s",
                                        str(LANG_SYSFONT_GAIN), "(dB)");
    screen->getstringsize(buf, &w1, &h);
    if (w1 > w) {
        w = w1;
    }

    snprintf(buf, sizeof(buf), "Mode: %s %s",
                                        str(LANG_EQUALIZER_BAND_Q), "");
    screen->getstringsize(buf, &w1, &h);
    if (w1 > w) {
        w = w1;
    }
    btn_bar->edit_mode.width = w + 10;
    btn_bar->edit_mode.x = vp->width - btn_bar->edit_mode.width;

    /* Cancel button */
    snprintf(buf, sizeof(buf), str(LANG_KBD_CANCEL), NULL, NULL);
    screen->getstringsize(buf, &w, &h);
    btn_bar->cancel.width = w + 10;
    btn_bar->cancel.x = btn_bar->cancel.width + 5;

    /* Ok button */
    snprintf(buf, sizeof(buf), str(LANG_KBD_OK), NULL, NULL);
    screen->getstringsize(buf, &w, &h);
    btn_bar->ok.x = 0;
    btn_bar->ok.width = btn_bar->cancel.width;

    /*dec button*/
    snprintf(buf, sizeof(buf), " - ");
    screen->getstringsize(buf, &w, &h);
    btn_bar->dec.x = 0;
    btn_bar->dec.width = btn_bar->cancel.width;

    /*inc button*/
    snprintf(buf, sizeof(buf), " + ");
    screen->getstringsize(buf, &w, &h);
    btn_bar->inc.width = btn_bar->cancel.width;
    btn_bar->inc.x = vp->width - btn_bar->inc.width;


    screen->getstringsize("A", &w, &h);
    if (w * 20 > vp->width ||
        (btn_bar->cancel.width + btn_bar->ok.width +
         btn_bar->edit_mode.width + 10 ) > vp->width)
    {
        return false; /* buttonbar or 3 settings do not fit on the screen */
    }

    /* determine the height of the button bar */
    if (h <= 16) {
        btn_bar->height = 20;
    }
    else {
        btn_bar->height = h + 4;
    }

    /* calculate band postion and size */
    eq_data->band.x = btn_bar->cancel.width;
    eq_data->band.width = vp->width - btn_bar->cancel.width * 2;
    eq_data->band.height = (vp->height - 1 - btn_bar->height -
                BUTTON_BAR_MARGIN_BOTTOM) / 10;
    for (i=0; i < 10; i++)
    {
        eq_data->band.y[i] = btn_bar->height + BUTTON_BAR_MARGIN_BOTTOM + i *
                                                eq_data->band.height;
    }

    /* calculate slider postion within a band and size */
    eq_data->slider.x = SLIDER_TOUCH_MARGIN;
    eq_data->slider.length = vp->width - 2 * SLIDER_TOUCH_MARGIN;
    eq_data->slider.y = h + 3 + 1;
    eq_data->slider.height = eq_data->band.height - eq_data->slider.y;

    if (eq_data->slider.height > 30)
    {
        eq_data->slider.height = 30;
    }
    else if (eq_data->slider.height < 15)
    {
        return false; /* sliders are too small */
    }
    return true;
}


/* Provides a graphical means of editing the EQ settings */
bool eq_menu_graphical(void)
{
    bool exit_request = false;
    bool result = true;
    bool has_changed = false;
    int button;
    int *setting;
    int current_band = 0;
    int step, min, max;
    enum eq_slider_mode mode = GAIN;
    //int barsenabled = viewportmanager_set_statusbar(VP_SB_HIDE_ALL);
    int eq_settings[30];
    struct button_bar btn_bar;
    struct eq_screen_data eq_data;
    struct viewport vp [NB_SCREENS];

    touchscreen_set_mode(TOUCHSCREEN_POINT);
    save_eq_settings(eq_settings);

    FOR_NB_SCREENS(i) {
        screens[i].set_viewport(NULL);
        viewport_set_defaults(&vp[i], i);
        screens[i].set_viewport(&vp[i]);
        screens[i].clear_viewport();
        screens[i].update_viewport();
    }

    vp[SCREEN_MAIN].x = EQ_OUTER_MARGIN;
    vp[SCREEN_MAIN].width -= 2 * EQ_OUTER_MARGIN;
    vp[SCREEN_MAIN].y += EQ_OUTER_MARGIN;
    vp[SCREEN_MAIN].height -= EQ_OUTER_MARGIN;
    vp[SCREEN_MAIN].font = FONT_UI;

#ifdef HAVE_REMOTE_LCD
    vp[SCREEN_REMOTE].font = FONT_SYSFIXED;
#endif

    /* Select the font. Use the theme font if possible, use Sysfont if the */
    /* buttonbar or EQ do not fit on the screen. Additionally buttonbar    */
    /* band and slider coordinates are calculated once.                    */
    if (!init_eq_screen(&screens[SCREEN_MAIN], &vp[SCREEN_MAIN], &btn_bar, &eq_data))
    {
#if (CONFIG_PLATFORM & PLATFORM_ANDROID)
        vp[SCREEN_MAIN].font = FONT_UI;
#else
        vp[SCREEN_MAIN].font = FONT_SYSFIXED;
#endif
        init_eq_screen(&screens[SCREEN_MAIN], &vp[SCREEN_MAIN], &btn_bar, &eq_data);
    }

    while (!exit_request) {

        FOR_NB_SCREENS(i) {
            screens[i].clear_viewport();
        }

        /* Set pointer to the band data currently editable */
        if (mode == GAIN) {
            /* gain */
            setting = &global_settings.eq_band_settings[0].gain;
            setting += current_band * 3;

            step = EQ_GAIN_STEP;
            min = EQ_GAIN_MIN;
            max = EQ_GAIN_MAX;

        } else if (mode == CUTOFF) {
            /* cutoff */
            setting = &global_settings.eq_band_settings[0].cutoff;
            setting += current_band * 3;

            step = EQ_CUTOFF_STEP;
            min = EQ_CUTOFF_MIN;
            max = EQ_CUTOFF_MAX;

        } else {
            /* Q */
            setting = &global_settings.eq_band_settings[0].q;
            setting += current_band * 3;

            step = EQ_Q_STEP;
            min = EQ_Q_MIN;
            max = EQ_Q_MAX;
        }

        /* Draw button bar and equalizer bands */
        draw_button_bar(&screens[SCREEN_MAIN], &vp[SCREEN_MAIN], mode, &btn_bar);
        draw_eq_sliders(&screens[SCREEN_MAIN], &vp[SCREEN_MAIN],
                                                &eq_data, current_band, mode);
#ifdef HAVE_REMOTE_LCD
        draw_remote_screen(&screens[SCREEN_REMOTE], &vp[SCREEN_REMOTE],
                                                 current_band, mode, *setting);
#endif
        FOR_NB_SCREENS(i) {
            screens[i].set_viewport(&vp[i]);
            screens[i].update_viewport();
        }

        /* wait here until a button or the touchscreen is pressed. no need to */
        /* to redraw if there is no action                                    */
        do
        {
            button = get_action(CONTEXT_SETTINGS_EQ,TIMEOUT_BLOCK);
        }
        while (button == BUTTON_NONE);

#define EQ_SLIDER_TCONST 2

        static bool slider_change = false;

        if ((button == ACTION_TOUCHSCREEN &&
            touchscreen_get_mode() == TOUCHSCREEN_POINT) ||
            slider_change)
        {
            bool slider_touched = false;
            short selected_band = current_band;
            int button_touchscreen;
            short touch_x = 0, touch_y = 0;
            int target_value = 0;

            button_touchscreen = action_get_touchscreen_press(&touch_x, &touch_y);

            if (viewport_point_within_vp(&vp[SCREEN_MAIN],
                                         (int)touch_x, (int)touch_y))
            {   /* convert touch into VP coordinates */
                touch_x -= vp[SCREEN_MAIN].x;
                touch_y -= vp[SCREEN_MAIN].y;

                if (touch_y >= btn_bar.y &&
                    touch_y <= btn_bar.y + btn_bar.height &&
                    touch_x >= eq_data.slider.x &&
                    touch_x <= eq_data.slider.x + eq_data.slider.length &&
                    !slider_change)
                {
                    if (touch_x >= btn_bar.ok.x &&
                            touch_x <= btn_bar.ok.x + btn_bar.ok.width)
                    {
                        button = ACTION_STD_CANCEL;
                        slider_change = false;
                    }
                    else if (touch_x >= btn_bar.cancel.x &&
                             touch_x <= btn_bar.cancel.x + btn_bar.cancel.width &&
                             (button_touchscreen & BUTTON_REL))
                    {
                        restore_eq_settings(eq_settings);
                        button = ACTION_STD_CANCEL;
                        slider_change = false;
                    }
                    else if (touch_x >= btn_bar.edit_mode.x &&
                             touch_x <= btn_bar.edit_mode.x + btn_bar.edit_mode.width &&
                             (button_touchscreen & BUTTON_REL))
                    {
                        button = ACTION_STD_OK;
                        /* change Edit Mode */
                    }
                    else
                    {
                        button = BUTTON_NONE;
                    }
                }
                else if (touch_x >= btn_bar.dec.x &&
                         touch_x <= btn_bar.dec.x + btn_bar.dec.width)
                {
                    if  (button_touchscreen & BUTTON_REL)
                        button = BUTTON_NONE;
                    else
                        button = ACTION_SETTINGS_DECREPEAT;
                    /* dec button */
                }
                else if (touch_x >= btn_bar.inc.x &&
                         touch_x <= btn_bar.inc.x + btn_bar.inc.width)
                {
                    if  (button_touchscreen & BUTTON_REL)
                        button = BUTTON_NONE;
                    else
                        button = ACTION_SETTINGS_INCREPEAT;
                    /* inc button */
                }
                else
                {
                    button = BUTTON_NONE;
                    selected_band = 0;
                    if (touch_x >= eq_data.band.x &&
                        touch_y >= eq_data.band.y[0] + eq_data.slider.y)
                    {
                        do {  /* find out which item and band has been touched */
                            if ( touch_y >= eq_data.band.y[selected_band] +
                                                        eq_data.slider.y  &&
                                (touch_y <= (eq_data.band.y[selected_band] +
                                    eq_data.slider.y + eq_data.slider.height)))
                            {
                                slider_touched = true;
                            }
                            else
                            {
                                selected_band++;
                            }
                        } while (selected_band < EQ_NUM_BANDS && !slider_touched);
                    }
                }

                if (slider_touched || slider_change)
                {
                    if (!(slider_change))
                    {
                        if (current_band != selected_band)
                        {
                            current_band = selected_band;
                        }
                        else if (slider_touched) {
                            slider_change = true;
                        }
                    }

                    if (slider_change)
                    {
                        if (mode == GAIN) {
                            target_value = max * 2 * (touch_x - eq_data.slider.x -
                                eq_data.slider.length/2) / eq_data.slider.length;
                        }
                        else if (mode == CUTOFF) {
                            if (current_band == 1)
                            {
                                step = step * 2;
                            }
                            else if (current_band > 1 && current_band < EQ_NUM_BANDS - 1)
                            {
                                step = step * 5;
                            }
                            else if (current_band == EQ_NUM_BANDS - 1)
                            {
                                step = step * 10;
                            }
                            target_value = min + ((max-min) * (touch_x -
                                    eq_data.slider.x) / eq_data.slider.length);
                        }
                        else if (mode == Q) {
                            target_value = min + ((max-min) * (touch_x -
                                    eq_data.slider.x) / eq_data.slider.length);
                        }

                        target_value = (target_value / step) * step;

                        if (mode != Q)
                        {
                            if (abs(target_value) % step >= step / 2)
                            {
                                if (target_value >= 0)
                                {
                                    target_value += step;
                                }
                                else
                                {
                                    target_value -= step;
                                }
                            }
                        }

                        if (target_value < min) {
                            target_value = min;
                        }
                        else if (target_value > max) {
                            target_value = max;
                        }

                        /* make the slider movement a little smoother */
                        *(setting) = (*(setting) + (target_value - *(setting)) /
                                    EQ_SLIDER_TCONST + (target_value - *(setting)) %
                                    EQ_SLIDER_TCONST);
                        has_changed = true;
                    }
                }

                if (slider_change && (button_touchscreen & BUTTON_REL)) {
                    slider_change = false;
                    *(setting) = target_value;
                }
            }
            else {
                button = BUTTON_NONE;
            }
        }

        switch (button) {
        case ACTION_SETTINGS_DEC:
        case ACTION_SETTINGS_DECREPEAT:
            *(setting) -= step;
            has_changed = true;
            if (*(setting) < min)
                *(setting) = min;
            break;

        case ACTION_SETTINGS_INC:
        case ACTION_SETTINGS_INCREPEAT:
            *(setting) += step;
            has_changed = true;
            if (*(setting) > max)
                *(setting) = max;
            break;

        case ACTION_STD_PREV:
        case ACTION_STD_PREVREPEAT:
            current_band--;
            if (current_band < 0)
                current_band = EQ_NUM_BANDS - 1; /* wrap around */
            break;

        case ACTION_STD_NEXT:
        case ACTION_STD_NEXTREPEAT:
            current_band++;
            if (current_band > EQ_NUM_BANDS - 1)
                current_band = 0; /* wrap around */
            break;

        case ACTION_STD_OK:
            mode++;
            if (mode > Q)
                mode = GAIN; /* wrap around */
            break;

        case ACTION_STD_CANCEL:
            exit_request = true;
            result = false;
            touchscreen_set_mode(global_settings.touch_mode);
            break;
        default:
            if(default_event_handler(button) == SYS_USB_CONNECTED) {
                exit_request = true;
                result = true;
            }
            break;
        }

        /* Update the filter if the user changed something */
        if (has_changed) {
            dsp_set_eq_coefs(current_band,
                &global_settings.eq_band_settings[current_band]);
            has_changed = false;
        }
    }

    /* Reset screen settings */
    FOR_NB_SCREENS(i) {
        screens[i].setfont(FONT_UI);
        screens[i].clear_display();
        screens[i].set_viewport(NULL);
    }
    //viewportmanager_set_statusbar(barsenabled);
    return result;
}

static bool eq_save_preset(void)
{
    /* make sure that the eq is enabled for setting saving */
    bool enabled = global_settings.eq_enabled;
    global_settings.eq_enabled = true;

    bool result = settings_save_config(SETTINGS_SAVE_EQPRESET);

    global_settings.eq_enabled = enabled;

    return result;
}

/* Allows browsing of preset files */
static struct browse_folder_info eqs = { EQS_DIR, SHOW_CFG };

MENUITEM_FUNCTION(eq_graphical, 0, ID2P(LANG_EQUALIZER_GRAPHICAL),
                    (int(*)(void))eq_menu_graphical, NULL, lowlatency_callback,
                    Icon_EQ);
MENUITEM_FUNCTION(eq_save, 0, ID2P(LANG_EQUALIZER_SAVE),
                    (int(*)(void))eq_save_preset, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(eq_browse, MENU_FUNC_USEPARAM, ID2P(LANG_EQUALIZER_BROWSE),
                    browse_folder, (void*)&eqs, lowlatency_callback,
                    Icon_NOICON);

MAKE_MENU(equalizer_menu, ID2P(LANG_EQUALIZER), NULL, Icon_EQ,
        &eq_enable, &eq_graphical, &eq_precut, &gain_menu,
        &advanced_eq_menu_, &eq_save, &eq_browse);

