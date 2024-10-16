/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Dan Everton
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
#include "list.h"
#include "menu.h"
#include "action.h"
#include "settings.h"
#include "rbpaths.h"
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
#include "exported_menus.h"
#include "pcmbuf.h"
#include "option_select.h"
#include "string-extra.h"

static void eq_apply(void);

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

void eq_enabled_option_callback(bool enabled)
{
    (void)enabled;
    eq_apply();
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

static int eq_setting_callback(int action,
                               const struct menu_item_ex *this_item,
                               struct gui_synclist *this_list)
{
    (void)this_list;
    switch (action)
    {
        case ACTION_ENTER_MENUITEM:
            action = lowlatency_callback(action, this_item, NULL);
            break;
        case ACTION_EXIT_MENUITEM:
            eq_apply();
            action = lowlatency_callback(action, this_item, NULL);
            break;
    }

    return action;
}
MENUITEM_SETTING(eq_enable, &global_settings.eq_enabled, eq_setting_callback);
MENUITEM_SETTING(eq_precut, &global_settings.eq_precut, eq_setting_callback);

static char* gainitem_get_name(int selected_item, void *data, char *buffer, size_t len)
{
    (void)data;
    snprintf(buffer, len, str(LANG_EQUALIZER_GAIN_ITEM),
            global_settings.eq_band_settings[selected_item].cutoff);

    return buffer;
}

static int gainitem_speak_item(int selected_item, void *data)
{
    (void)data;
    talk_number(global_settings.eq_band_settings[selected_item].cutoff, false);
    talk_id(LANG_EQUALIZER_GAIN_ITEM, true);
    return 0;
}

static enum themable_icons gainitem_get_icon(int selected_item, void * data)
{
    (void)selected_item;
    (void)data;

    return Icon_Menu_functioncall;
}

static const char* db_format(char* buffer, size_t buffer_size, int value,
                      const char* unit)
{
    int v = abs(value);

    snprintf(buffer, buffer_size, "%s%d.%d %s", value < 0 ? "-" : "",
             v / 10, v % 10, unit);
    return buffer;
}

static int32_t get_dec_talkid(int value, int unit)
{
    return TALK_ID_DECIMAL(value, 1, unit);
}

static const struct int_setting gain_int_setting = {
    .option_callback = NULL,
    .unit = UNIT_DB,
    .step = EQ_GAIN_STEP,
    .min = EQ_GAIN_MIN,
    .max = EQ_GAIN_MAX,
    .formatter = db_format,
    .get_talk_id = get_dec_talkid,
};

static const struct int_setting q_int_setting = {
    .option_callback = NULL,
    .unit = UNIT_INT,
    .step = EQ_Q_STEP,
    .min = EQ_Q_MIN,
    .max = EQ_Q_MAX,
    .formatter = eq_q_format,
    .get_talk_id = get_dec_talkid,
};

static const struct int_setting cutoff_int_setting = {
    .option_callback = NULL,
    .unit = UNIT_HERTZ,
    .step = EQ_CUTOFF_STEP,
    .min = EQ_CUTOFF_MIN,
    .max = EQ_CUTOFF_MAX,
    .formatter = NULL,
    .get_talk_id = NULL,
};

static int simplelist_action_callback(int action, struct gui_synclist *lists)
{
    (void)lists;
    if (action == ACTION_STD_OK)
        return ACTION_STD_CANCEL;
    return action;
}

static int eq_do_simple_menu(void * param)
{
    (void)param;
    struct simplelist_info info;
    struct settings_list setting;
    char title[MAX_PATH];

    simplelist_info_init(&info, str(LANG_EQUALIZER_GAIN), EQ_NUM_BANDS, NULL);
    info.get_name = (list_get_name*)gainitem_get_name;
    info.get_talk = gainitem_speak_item;
    info.get_icon = gainitem_get_icon;
    info.action_callback = simplelist_action_callback;
    info.selection = -1;
    info.title_icon = Icon_Submenu;
    setting.flags = F_BANFROMQS|F_INT_SETTING|F_T_INT|F_NO_WRAP;
    setting.lang_id = LANG_GAIN;
    setting.default_val.int_ = 0;
    setting.int_setting = &gain_int_setting;

    while (true)
    {
        simplelist_show_list(&info);
        if (info.selection < 0)
            break;
        pcmbuf_set_low_latency(true);
        setting.setting = &global_settings.eq_band_settings[info.selection].gain;
        option_screen(&setting, NULL, false,
                gainitem_get_name(info.selection, NULL, title, MAX_PATH));
        eq_apply();
        pcmbuf_set_low_latency(false);
    }
    return 0;
}
MENUITEM_FUNCTION(gain_menu, 0, ID2P(LANG_EQUALIZER_GAIN),
	              eq_do_simple_menu, NULL, Icon_Submenu);

static void selection_to_banditem(int selection, int expanded_band, int *band, int *item)
{
    int diff = selection - expanded_band;

    if (expanded_band < 0 || diff < 0)
    {
        *item = 0;
        *band = selection;
    }
    else if (diff < 4)
    {
        *item = selection - expanded_band;
        *band = expanded_band;
    }
    else
    {
        *item = 0;
        *band = expanded_band + diff - 3;
    }
}

static char *advancedmenu_item_get_name(int selected_item, void *data, char *buffer, size_t len)
{
    int band;
    int item;
    int lang = -1;

    selection_to_banditem(selected_item, *(intptr_t*)data, &band, &item);

    switch (item)
    {
    case 0: /* Band title */
        if (band == 0)
            return str(LANG_EQUALIZER_BAND_LOW_SHELF);
        else if (band == EQ_NUM_BANDS - 1)
            return str(LANG_EQUALIZER_BAND_HIGH_SHELF);
        else
        {
            snprintf(buffer, len, str(LANG_EQUALIZER_BAND_PEAK), band);
            return buffer;
        }
        break;
    case 1: /* cutoff */
        if (band == 0)
            lang = LANG_EQUALIZER_BAND_CUTOFF;
        else if (band == EQ_NUM_BANDS - 1)
            lang = LANG_EQUALIZER_BAND_CUTOFF;
        else
            lang = LANG_EQUALIZER_BAND_CENTER;
        break;
    case 2: /* Q */
        lang = LANG_EQUALIZER_BAND_Q;
        break;
    case 3: /* Gain */
        lang = LANG_GAIN;
        break;
    }

    if(lang < 0)
        buffer[0] = 0;
    else {
        buffer[0] = '\t';
        strmemccpy(&buffer[1], str(lang), len - 1);
    }

    return buffer;
}

static int advancedmenu_speak_item(int selected_item, void *data)
{
    (void)data;
    int band;
    int item;
    int lang = -1;

    selection_to_banditem(selected_item, *(intptr_t*)data, &band, &item);

    switch (item)
    {
    case 0: /* Band title */
        if (band == 0)
            lang = LANG_EQUALIZER_BAND_LOW_SHELF;
        else if (band == EQ_NUM_BANDS - 1)
            lang = LANG_EQUALIZER_BAND_HIGH_SHELF;
        else
        {
            talk_id(LANG_EQUALIZER_BAND_PEAK, false);
            talk_number(band, true);
            return -1;
        }
        break;
    case 1: /* cutoff */
        if (band == 0)
            lang = LANG_EQUALIZER_BAND_CUTOFF;
        else if (band == EQ_NUM_BANDS - 1)
            lang = LANG_EQUALIZER_BAND_CUTOFF;
        else
            lang = LANG_EQUALIZER_BAND_CENTER;
        break;
    case 2: /* Q */
        lang = LANG_EQUALIZER_BAND_Q;
        break;
    case 3: /* Gain */
        lang = LANG_GAIN;
        break;
    }
    talk_id(lang, true);
    return -1;
}

static enum themable_icons advancedmenu_get_icon(int selected_item, void * data)
{
    (void)data;
    int band;
    int item;

    selection_to_banditem(selected_item, *(intptr_t*)data, &band, &item);

    if (item == 0)
        return Icon_Submenu;
    else
        return Icon_Menu_setting;
}
extern struct eq_band_setting eq_defaults[EQ_NUM_BANDS];

static int eq_do_advanced_menu(void * param)
{
    (void)param;
    struct simplelist_info info;
    struct settings_list setting;
    char title[MAX_PATH];
    int band, item;
    intptr_t selected_band = -1;

    simplelist_info_init(&info, str(LANG_EQUALIZER_ADVANCED),
            EQ_NUM_BANDS, &selected_band);
    info.get_name = (list_get_name*)advancedmenu_item_get_name;
    info.get_talk = advancedmenu_speak_item;
    info.get_icon = advancedmenu_get_icon;
    info.action_callback = simplelist_action_callback;
    info.selection = -1;
    info.title_icon = Icon_EQ;
    setting.flags = F_BANFROMQS|F_INT_SETTING|F_T_INT|F_NO_WRAP;

    while (true)
    {
        simplelist_show_list(&info);
        if (info.selection < 0)
            break;
        selection_to_banditem(info.selection, selected_band, &band, &item);
        switch (item)
        {
            case 0: /* title, do nothing */
            {
                int extra;
                if (selected_band == band)
                {
                    extra = 0;
                    selected_band = -1;
                }
                else
                {
                    extra = 3;
                    selected_band = band;
                }
                info.selection = band;
                info.count = EQ_NUM_BANDS + extra;
                continue;
            }
            case 1: /* cutoff */
                if (band == 0 || band == EQ_NUM_BANDS - 1)
                    setting.lang_id = LANG_EQUALIZER_BAND_CUTOFF;
                else
                    setting.lang_id = LANG_EQUALIZER_BAND_CENTER;
                setting.default_val.int_ = eq_defaults[band].cutoff;
                setting.int_setting = &cutoff_int_setting;
                setting.setting = &global_settings.eq_band_settings[band].cutoff;
                break;
            case 2: /* Q */
                setting.lang_id = LANG_EQUALIZER_BAND_Q;
                setting.default_val.int_ = eq_defaults[band].q;
                setting.int_setting = &q_int_setting;
                setting.setting = &global_settings.eq_band_settings[band].q;
                break;
            case 3: /* Gain */
                setting.lang_id = LANG_GAIN;
                setting.default_val.int_ = eq_defaults[band].gain;
                setting.int_setting = &gain_int_setting;
                setting.setting = &global_settings.eq_band_settings[band].gain;
                break;
        }
        pcmbuf_set_low_latency(true);
        advancedmenu_item_get_name(info.selection, &selected_band, title, MAX_PATH);

        option_screen(&setting, NULL, false, title[0] == '\t' ? &title[1] : title);
        eq_apply();
        pcmbuf_set_low_latency(false);
    }
    return 0;
}
MENUITEM_FUNCTION(advanced_menu, 0, ID2P(LANG_EQUALIZER_ADVANCED),
                  eq_do_advanced_menu, NULL, Icon_EQ);

enum eq_slider_mode {
    GAIN,
    CUTOFF,
    Q,
};

enum eq_type {
    LOW_SHELF,
    PEAK,
    HIGH_SHELF
};

/* Size of just the slider/srollbar */
#define SCROLLBAR_SIZE 6

/* Draw the UI for a whole EQ band */
static int draw_eq_slider(struct screen * screen, int x, int y,
    int width, int cutoff, int q, int gain, bool selected,
    enum eq_slider_mode mode, int band)
{
    char buf[26];
    int steps, min_item, max_item;
    int abs_gain = abs(gain);
    int x1, x2, y1, total_height;
    int w, h;

    switch(mode) {
    case Q:
        steps = EQ_Q_MAX - EQ_Q_MIN;
        min_item = q - EQ_Q_STEP - EQ_Q_MIN;
        max_item = q + EQ_Q_STEP - EQ_Q_MIN;
        break;
    case CUTOFF:
        steps = EQ_CUTOFF_MAX - EQ_CUTOFF_MIN;
        min_item = cutoff - EQ_CUTOFF_FAST_STEP * 2;
        max_item = cutoff + EQ_CUTOFF_FAST_STEP * 2;
        break;
    case GAIN:
    default:
        steps = EQ_GAIN_MAX - EQ_GAIN_MIN;
        min_item = abs(EQ_GAIN_MIN) + gain - EQ_GAIN_STEP * 5;
        max_item = abs(EQ_GAIN_MIN) + gain + EQ_GAIN_STEP * 5;
        break;
    }

    /* Start two pixels in, one for border, one for margin */
    x1 = x + 2;
    y1 = y + 2;

    /* Print out the band label */
    if (band == 0) {
        screen->putsxy(x1, y1, "LS: ");
        /*screen->getstringsize("LS:", &w, &h); UNUSED*/
    } else if (band == EQ_NUM_BANDS - 1) {
        screen->putsxy(x1, y1, "HS: ");
        /*screen->getstringsize("HS:", &w, &h); UNUSED*/
    } else {
        snprintf(buf, sizeof(buf),  "PK%d:", band);
        screen->putsxy(x1, y1, buf);
        /*screen->getstringsize(buf, &w, &h); UNUSED*/
    }

    w = screen->getstringsize("A", NULL, &h);

    x1 += 5*w; /* 4 chars for label + 1 space = 5 */

    /* Print out gain part of status line (left justify after label) */
    if (mode == GAIN && selected)
        screen->set_drawmode(DRMODE_SOLID | DRMODE_INVERSEVID);
    else
        screen->set_drawmode(DRMODE_SOLID);

    snprintf(buf, sizeof(buf), "%s%2d.%d%s", gain < 0 ? "-" : " ",
        abs_gain / EQ_USER_DIVISOR, abs_gain % EQ_USER_DIVISOR,
        screen->lcdwidth >= 160 ? "dB" : "");
    screen->putsxy(x1, y1, buf);
    w = screen->getstringsize(buf, NULL, NULL);
    x1 += w;

    /* Print out Q part of status line (right justify) */
    if (mode == Q && selected)
        screen->set_drawmode(DRMODE_SOLID | DRMODE_INVERSEVID);
    else
        screen->set_drawmode(DRMODE_SOLID);

    snprintf(buf, sizeof(buf), "%d.%d%s", q / EQ_USER_DIVISOR,
             q % EQ_USER_DIVISOR, screen->lcdwidth >= 160 ? " Q" : "");
    w = screen->getstringsize(buf, NULL, NULL);
    x2 = x + width - w - 2;
    screen->putsxy(x2, y1, buf);

    /* Print out cutoff part of status line (center between gain & Q) */
    if (mode == CUTOFF && selected)
        screen->set_drawmode(DRMODE_SOLID | DRMODE_INVERSEVID);
    else
        screen->set_drawmode(DRMODE_SOLID);

    snprintf(buf, sizeof(buf),  "%5d%s", cutoff,
             screen->lcdwidth >= 160 ? "Hz" : "");
    w = screen->getstringsize(buf, NULL, NULL);
    x1 = x1 + (x2 - x1 - w)/2;
    screen->putsxy(x1, y1, buf);

    /* Draw selection box */
    total_height = 3 + h + 1 + SCROLLBAR_SIZE + 3;
    screen->set_drawmode(DRMODE_SOLID);
    if (selected) {
        screen->drawrect(x, y, width, total_height);
    }

    /* Draw horizontal slider. Reuse scrollbar for this */
    gui_scrollbar_draw(screen, x + 3, y1 + h + 1, width - 6, SCROLLBAR_SIZE,
                       steps, min_item, max_item, HORIZONTAL);

    return total_height;
}

/* Draw's all the EQ sliders. Returns the total height of the sliders drawn */
static void draw_eq_sliders(struct screen * screen, int x, int y,
                            int nb_eq_sliders, int start_item,
                            int current_band, enum eq_slider_mode mode)
{
    int height = y;

    start_item = MIN(start_item, EQ_NUM_BANDS - nb_eq_sliders);

    for (int i = 0; i < EQ_NUM_BANDS; i++) {
        struct eq_band_setting *setting = &global_settings.eq_band_settings[i];
        int cutoff = setting->cutoff;
        int q      = setting->q;
        int gain   = setting->gain;

        if (i == start_item + nb_eq_sliders)
            break;

        if (i >= start_item) {
            height += draw_eq_slider(screen, x, height, screen->lcdwidth - x - 1,
                                     cutoff, q, gain, i == current_band, mode,
                                     i);
            /* add a margin */
            height++;
        }
    }

    if (nb_eq_sliders != EQ_NUM_BANDS)
        gui_scrollbar_draw(screen, 0, y, SCROLLBAR_SIZE - 1,
                           screen->lcdheight - y, EQ_NUM_BANDS,
                           start_item, start_item + nb_eq_sliders,
                           VERTICAL);
    return;
}

/* Provides a graphical means of editing the EQ settings */
int eq_menu_graphical(void)
{
    bool exit_request = false;
    bool result = true;
    bool has_changed = false;
    int button;
    int *setting;
    int current_band, x, y, step, fast_step, min, max;
    enum eq_slider_mode mode;
    int h, height, start_item, nb_eq_sliders[NB_SCREENS];
    FOR_NB_SCREENS(i)
        viewportmanager_theme_enable(i, false, NULL);


    FOR_NB_SCREENS(i) {
        screens[i].set_viewport(NULL);
        screens[i].setfont(FONT_SYSFIXED);
        screens[i].clear_display();

        /* Figure out how many sliders can be drawn on the screen */
        h = screens[i].getcharheight();

        /* Total height includes margins (1), text, slider, and line selector (1) */
        height = 3 + h + 1 + SCROLLBAR_SIZE + 3;
        nb_eq_sliders[i] = screens[i].lcdheight / height;

        /* Make sure the "Edit Mode" text fits too */
        height = nb_eq_sliders[i]*height + h + 2;
        if (height > screens[i].lcdheight)
            nb_eq_sliders[i]--;

        if (nb_eq_sliders[i] > EQ_NUM_BANDS)
            nb_eq_sliders[i] = EQ_NUM_BANDS;
    }

    y = h + 1;

    /* Start off editing gain on the first band */
    mode = GAIN;
    current_band = 0;

    while (!exit_request) {
        FOR_NB_SCREENS(i)
        {
            screens[i].clear_display();

            /* Set pointer to the band data currently editable */
            if (mode == GAIN) {
                /* gain */
                setting = &global_settings.eq_band_settings[current_band].gain;

                step = EQ_GAIN_STEP;
                fast_step = EQ_GAIN_FAST_STEP;
                min = EQ_GAIN_MIN;
                max = EQ_GAIN_MAX;

                screens[i].putsxyf(0, 0, str(LANG_SYSFONT_EQUALIZER_EDIT_MODE),
                         str(LANG_GAIN), "(dB)");
            } else if (mode == CUTOFF) {
                /* cutoff */
                setting = &global_settings.eq_band_settings[current_band].cutoff;

                step = EQ_CUTOFF_STEP;
                fast_step = EQ_CUTOFF_FAST_STEP;
                min = EQ_CUTOFF_MIN;
                max = EQ_CUTOFF_MAX;

                screens[i].putsxyf(0, 0, str(LANG_SYSFONT_EQUALIZER_EDIT_MODE),
                         str(LANG_SYSFONT_EQUALIZER_BAND_CUTOFF), "(Hz)");
            } else {
                /* Q */
                setting = &global_settings.eq_band_settings[current_band].q;

                step = EQ_Q_STEP;
                fast_step = EQ_Q_FAST_STEP;
                min = EQ_Q_MIN;
                max = EQ_Q_MAX;

                screens[i].putsxyf(0, 0, str(LANG_SYSFONT_EQUALIZER_EDIT_MODE),
                         str(LANG_EQUALIZER_BAND_Q), "");
            }

            /* Draw scrollbar if needed */
            if (nb_eq_sliders[i] != EQ_NUM_BANDS)
            {
                if (current_band == 0) {
                    start_item = 0;
                } else if (current_band == EQ_NUM_BANDS - 1) {
                    start_item = EQ_NUM_BANDS - nb_eq_sliders[i];
                } else {
                    start_item = current_band - 1;
                }
                x = SCROLLBAR_SIZE;
            } else {
                x = 1;
                start_item = 0;
            }
            /* Draw equalizer band details */
            draw_eq_sliders(&screens[i], x, y, nb_eq_sliders[i], start_item,
                            current_band, mode);

            screens[i].update();
        }

        button = get_action(CONTEXT_SETTINGS_EQ,TIMEOUT_BLOCK);

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

        case ACTION_SETTINGS_INCBIGSTEP:
            *(setting) += fast_step;
            has_changed = true;
            if (*(setting) > max)
                *(setting) = max;
            break;

        case ACTION_SETTINGS_DECBIGSTEP:
            *(setting) -= fast_step;
            has_changed = true;
            if (*(setting) < min)
                *(setting) = min;
            break;

        case ACTION_STD_PREV:
        case ACTION_STD_PREVREPEAT:
            current_band--;
            if (current_band < 0)
                current_band = EQ_NUM_BANDS - 1; /* wrap around */
            break;

        case ACTION_STD_NEXT:
        case ACTION_STD_NEXTREPEAT:
            current_band = (current_band + 1) % EQ_NUM_BANDS;
            break;

        case ACTION_STD_OK:
            mode++;
            if (mode > Q)
                mode = GAIN; /* wrap around */
            break;

        case ACTION_STD_CANCEL:
            exit_request = true;
            result = false;
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
    FOR_NB_SCREENS(i)
    {
        screens[i].setfont(FONT_UI);
        screens[i].clear_display();
        screens[i].set_viewport(NULL);
        viewportmanager_theme_undo(i, false);
    }
    return (result) ? 1 : 0;
}

static int eq_save_preset(void)
{
    /* make sure that the eq is enabled for setting saving */
    bool enabled = global_settings.eq_enabled;
    global_settings.eq_enabled = true;

    bool result = settings_save_config(SETTINGS_SAVE_EQPRESET);

    global_settings.eq_enabled = enabled;

    return (result) ? 1 : 0;
}
/* Allows browsing of preset files */
static struct browse_folder_info eqs = { EQS_DIR, SHOW_CFG };

static void eq_reset_defaults(void)
{
    for (int i = 0; i < EQ_NUM_BANDS; i++) {
        global_settings.eq_band_settings[i].cutoff = eq_defaults[i].cutoff;
        global_settings.eq_band_settings[i].q = eq_defaults[i].q;
        global_settings.eq_band_settings[i].gain = eq_defaults[i].gain;
    }
    eq_apply();
}

MENUITEM_FUNCTION(eq_graphical, 0, ID2P(LANG_EQUALIZER_GRAPHICAL),
                  eq_menu_graphical, lowlatency_callback, Icon_EQ);
MENUITEM_FUNCTION(eq_save, 0, ID2P(LANG_EQUALIZER_SAVE),
                  eq_save_preset, NULL, Icon_NOICON);
MENUITEM_FUNCTION(eq_reset, 0, ID2P(LANG_RESET_EQUALIZER),
                  eq_reset_defaults, NULL, Icon_NOICON);
MENUITEM_FUNCTION_W_PARAM(eq_browse, 0, ID2P(LANG_EQUALIZER_BROWSE),
                          browse_folder, (void*)&eqs,
                          lowlatency_callback, Icon_NOICON);

MAKE_MENU(equalizer_menu, ID2P(LANG_EQUALIZER), NULL, Icon_EQ,
        &eq_enable, &eq_graphical, &eq_precut, &gain_menu,
        &advanced_menu, &eq_save, &eq_browse, &eq_reset);
