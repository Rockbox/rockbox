/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: eq_menu.c 12179 2007-02-01 23:08:15Z amiconn $
 *
 * Copyright (C) 2006 Dan Everton
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
#include "statusbar.h"
#include "screens.h"
#include "icons.h"
#include "font.h"
#include "lang.h"
#include "sprintf.h"
#include "talk.h"
#include "misc.h"
#include "sound.h"
#include "splash.h"
#include "dsp.h"
#include "tree.h"
#include "talk.h"
#include "screen_access.h"
#include "keyboard.h"
#include "gui/scrollbar.h"
#include "eq_menu.h"

/*
 * Utility functions
 */

void eq_q_format(char* buffer, size_t buffer_size, int value, const char* unit)
{
    snprintf(buffer, buffer_size, "%d.%d %s", value / EQ_USER_DIVISOR, value % EQ_USER_DIVISOR, unit);
}

void eq_precut_format(char* buffer, size_t buffer_size, int value, const char* unit)
{
    snprintf(buffer, buffer_size, "%s%d.%d %s", value == 0 ? " " : "-",
        value / EQ_USER_DIVISOR, value % EQ_USER_DIVISOR, unit);
}

/*
 * Settings functions
 */
void eq_apply(void)
{
    int i;
    dsp_set_eq(global_settings.eq_enabled); 
    dsp_set_eq_precut(global_settings.eq_precut);    
    /* Update all bands */
    for(i = 0; i < 5; i++) {
        dsp_set_eq_coefs(i);
    }
}
int enable_callback(int action, const struct menu_item_ex *this_item)
{
    (void)this_item;
    if (action == ACTION_EXIT_MENUITEM)
        eq_apply();
    return action;
}
MENUITEM_SETTING(eq_enable, &global_settings.eq_enabled, enable_callback);
MENUITEM_SETTING(eq_precut, &global_settings.eq_precut, enable_callback);

int dsp_set_coefs_callback(int action, const struct menu_item_ex *this_item)
{
    (void)this_item;
    if (action == ACTION_EXIT_MENUITEM)
    {
        /* for now, set every band... figure out a better way later */
        int i=0;
        for (i=0; i<5; i++)
            dsp_set_eq_coefs(i);
    }
    return action;
}

char* gainitem_get_name(int selected_item, void * data, char *buffer)
{
    (void)selected_item;
    int *setting = (int*)data;
    snprintf(buffer, MAX_PATH, str(LANG_EQUALIZER_GAIN_ITEM), *setting);
    return buffer;
}
int gainitem_speak_item(int selected_item, void * data)
{
    (void)selected_item;
    int *setting = (int*)data;
    talk_number(*setting, false);
    talk_id(LANG_EQUALIZER_GAIN_ITEM, true);
    return 0;
}

int do_option(void* param)
{
    const struct menu_item_ex *setting = (const struct menu_item_ex*)param;
    do_setting_from_menu(setting);
    eq_apply();
    return 0;
}

MENUITEM_SETTING(cutoff_0, &global_settings.eq_band0_cutoff, dsp_set_coefs_callback);
MENUITEM_SETTING(cutoff_1, &global_settings.eq_band1_cutoff, dsp_set_coefs_callback);
MENUITEM_SETTING(cutoff_2, &global_settings.eq_band2_cutoff, dsp_set_coefs_callback);
MENUITEM_SETTING(cutoff_3, &global_settings.eq_band3_cutoff, dsp_set_coefs_callback);
MENUITEM_SETTING(cutoff_4, &global_settings.eq_band4_cutoff, dsp_set_coefs_callback);

MENUITEM_SETTING(q_0, &global_settings.eq_band0_q, dsp_set_coefs_callback);
MENUITEM_SETTING(q_1, &global_settings.eq_band1_q, dsp_set_coefs_callback);
MENUITEM_SETTING(q_2, &global_settings.eq_band2_q, dsp_set_coefs_callback);
MENUITEM_SETTING(q_3, &global_settings.eq_band3_q, dsp_set_coefs_callback);
MENUITEM_SETTING(q_4, &global_settings.eq_band4_q, dsp_set_coefs_callback);

MENUITEM_SETTING(gain_0, &global_settings.eq_band0_gain, dsp_set_coefs_callback);
MENUITEM_SETTING(gain_1, &global_settings.eq_band1_gain, dsp_set_coefs_callback);
MENUITEM_SETTING(gain_2, &global_settings.eq_band2_gain, dsp_set_coefs_callback);
MENUITEM_SETTING(gain_3, &global_settings.eq_band3_gain, dsp_set_coefs_callback);
MENUITEM_SETTING(gain_4, &global_settings.eq_band4_gain, dsp_set_coefs_callback);

MENUITEM_FUNCTION_DYNTEXT(gain_item_0, MENU_FUNC_USEPARAM,
                          do_option, (void*)&gain_0, 
                          gainitem_get_name, gainitem_speak_item,
                          &global_settings.eq_band0_cutoff, NULL, Icon_NOICON);
MENUITEM_FUNCTION_DYNTEXT(gain_item_1, MENU_FUNC_USEPARAM,
                          do_option, (void*)&gain_1,
                          gainitem_get_name, gainitem_speak_item,
                          &global_settings.eq_band1_cutoff, NULL, Icon_NOICON);
MENUITEM_FUNCTION_DYNTEXT(gain_item_2, MENU_FUNC_USEPARAM,
                          do_option, (void*)&gain_2,
                          gainitem_get_name, gainitem_speak_item,
                          &global_settings.eq_band2_cutoff, NULL, Icon_NOICON);
MENUITEM_FUNCTION_DYNTEXT(gain_item_3, MENU_FUNC_USEPARAM,
                          do_option, (void*)&gain_3,
                          gainitem_get_name, gainitem_speak_item,
                          &global_settings.eq_band3_cutoff, NULL, Icon_NOICON);
MENUITEM_FUNCTION_DYNTEXT(gain_item_4, MENU_FUNC_USEPARAM,
                          do_option, (void*)&gain_4,
                          gainitem_get_name, gainitem_speak_item,
                          &global_settings.eq_band4_cutoff, NULL, Icon_NOICON);
                                    
MAKE_MENU(gain_menu, ID2P(LANG_EQUALIZER_GAIN), NULL, Icon_NOICON, &gain_item_0, 
            &gain_item_1, &gain_item_2, &gain_item_3, &gain_item_4);

static const struct menu_item_ex *band_items[3][3] = {
    { &cutoff_1, &q_1, &gain_1 },
    { &cutoff_2, &q_2, &gain_2 },
    { &cutoff_3, &q_3, &gain_3 }
};
char* centerband_get_name(int selected_item, void * data, char *buffer)
{
    (void)selected_item;
    int band = (intptr_t)data;
    snprintf(buffer, MAX_PATH, str(LANG_EQUALIZER_BAND_PEAK), band);
    return buffer;
}
int centerband_speak_item(int selected_item, void * data)
{
    (void)selected_item;
    int band = (intptr_t)data;
    talk_id(LANG_EQUALIZER_BAND_PEAK, false);
    talk_number(band, true);
    return 0;
}
int do_center_band_menu(void* param)
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
    do_menu(&menu, NULL);
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
MAKE_MENU(band_4_menu, ID2P(LANG_EQUALIZER_BAND_HIGH_SHELF), NULL, 
            Icon_EQ, &cutoff_4, &q_4, &gain_4);

MAKE_MENU(advanced_eq_menu_, ID2P(LANG_EQUALIZER_ADVANCED), NULL, Icon_EQ,
            &band_0_menu, &band_1_menu, &band_2_menu, &band_3_menu, &band_4_menu);


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

/* Draw the UI for a whole EQ band */
static int draw_eq_slider(struct screen * screen, int x, int y,
    int width, int cutoff, int q, int gain, bool selected,
    enum eq_slider_mode mode, enum eq_type type)
{
    char buf[26];
    const char separator[2] = " ";
    int steps, min_item, max_item;
    int abs_gain = abs(gain);
    int current_x, total_height, separator_width, separator_height;
    int w, h;
    const int slider_height = 6;  

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
    current_x = x + 2;

    /* Figure out how large our separator string is */
    screen->getstringsize(separator, &separator_width, &separator_height);

    /* Total height includes margins, text, and line selector */
    total_height = separator_height + slider_height + 2 + 3;

    /* Print out the band label */
    if (type == LOW_SHELF) {
        screen->putsxy(current_x, y + 2, "LS:");
        screen->getstringsize("LS:", &w, &h);
    } else if (type == HIGH_SHELF) {
        screen->putsxy(current_x, y + 2, "HS:");
        screen->getstringsize("HS:", &w, &h);
    } else {
        screen->putsxy(current_x, y + 2, "PK:");
        screen->getstringsize("PK:", &w, &h);
    }
    current_x += w;

    /* Print separator */
    screen->set_drawmode(DRMODE_SOLID);
    screen->putsxy(current_x, y + 2, separator);
    current_x += separator_width;
#if NB_SCREENS > 1
    if (screen->screen_type == SCREEN_REMOTE) {
        if (mode == GAIN) {
            screen->putsxy(current_x, y + 2, str(LANG_GAIN));
            screen->getstringsize(str(LANG_GAIN), &w, &h);
        } else if (mode == CUTOFF) {
            screen->putsxy(current_x, y + 2, str(LANG_EQUALIZER_BAND_CUTOFF));
            screen->getstringsize(str(LANG_EQUALIZER_BAND_CUTOFF), &w, &h);
        } else {
            screen->putsxy(current_x, y + 2, str(LANG_EQUALIZER_BAND_Q));
            screen->getstringsize(str(LANG_EQUALIZER_BAND_Q), &w, &h);
        }

        /* Draw horizontal slider. Reuse scrollbar for this */
        gui_scrollbar_draw(screen, x + 3, y + h + 3, width - 6, slider_height, steps,
                           min_item, max_item, HORIZONTAL);
    
        /* Print out cutoff part */
        snprintf(buf, sizeof(buf), "%sGain     %s%2d.%ddB",mode==GAIN?" > ":"   ", gain < 0 ? "-" : " ",
                 abs_gain / EQ_USER_DIVISOR, abs_gain % EQ_USER_DIVISOR);
        screen->getstringsize(buf, &w, &h);
        y = 3*h;
        screen->putsxy(0, y, buf);
        /* Print out cutoff part */
        snprintf(buf, sizeof(buf),  "%sCutoff   %5dHz",mode==CUTOFF?" > ":"   ", cutoff);
        y += h;
        screen->putsxy(0, y, buf);
        snprintf(buf, sizeof(buf), "%sQ setting %d.%d Q",mode==Q?" > ":"   ", q / EQ_USER_DIVISOR,
                 q % EQ_USER_DIVISOR);
        y += h;
        screen->putsxy(0, y, buf);
        return y;
    }
#endif
    
    /* Print out gain part of status line */
    snprintf(buf, sizeof(buf), "%s%2d.%ddB", gain < 0 ? "-" : " ",
        abs_gain / EQ_USER_DIVISOR, abs_gain % EQ_USER_DIVISOR);

    if (mode == GAIN && selected)
        screen->set_drawmode(DRMODE_SOLID | DRMODE_INVERSEVID);

    screen->putsxy(current_x, y + 2, buf);
    screen->getstringsize(buf, &w, &h);
    current_x += w;

    /* Print separator */
    screen->set_drawmode(DRMODE_SOLID);
    screen->putsxy(current_x, y + 2, separator);
    current_x += separator_width;

    /* Print out cutoff part of status line */
    snprintf(buf, sizeof(buf),  "%5dHz", cutoff);

    if (mode == CUTOFF && selected)
        screen->set_drawmode(DRMODE_SOLID | DRMODE_INVERSEVID);

    screen->putsxy(current_x, y + 2, buf);
    screen->getstringsize(buf, &w, &h);
    current_x += w;

    /* Print separator */
    screen->set_drawmode(DRMODE_SOLID);
    screen->putsxy(current_x, y + 2, separator);
    current_x += separator_width;

    /* Print out Q part of status line */
    snprintf(buf, sizeof(buf), "%d.%d Q", q / EQ_USER_DIVISOR,
        q % EQ_USER_DIVISOR);

    if (mode == Q && selected)
        screen->set_drawmode(DRMODE_SOLID | DRMODE_INVERSEVID);

    screen->putsxy(current_x, y + 2, buf);
    screen->getstringsize(buf, &w, &h);
    current_x += w;

    screen->set_drawmode(DRMODE_SOLID);

    /* Draw selection box */
    if (selected) {
        screen->drawrect(x, y, width, total_height);
    }

    /* Draw horizontal slider. Reuse scrollbar for this */
    gui_scrollbar_draw(screen, x + 3, y + h + 3, width - 6, slider_height, steps,
        min_item, max_item, HORIZONTAL);

    return total_height;
}

/* Draw's all the EQ sliders. Returns the total height of the sliders drawn */
static int draw_eq_sliders(int current_band, enum eq_slider_mode mode)
{
    int i, gain, q, cutoff;
    int height = 2; /* Two pixel margin */
    int slider_width[NB_SCREENS];
    int *setting = &global_settings.eq_band0_cutoff;
    enum eq_type type;

    FOR_NB_SCREENS(i)
        slider_width[i] = screens[i].width - 4; /* two pixel margin on each side */  
    
    for (i=0; i<5; i++) {
        cutoff = *setting++;
        q = *setting++;
        gain = *setting++;

        if (i == 0) {
            type = LOW_SHELF;
        } else if (i == 4) {
            type = HIGH_SHELF;
        } else {
            type = PEAK;
        }
        height += draw_eq_slider(&(screens[SCREEN_MAIN]), 2, height,
                      slider_width[SCREEN_MAIN], cutoff, q, gain, 
                      i == current_band, mode, type);
#if NB_SCREENS > 1
        if (i == current_band)
            draw_eq_slider(&(screens[SCREEN_REMOTE]), 2, 0,
                      slider_width[SCREEN_REMOTE], cutoff, q, gain,1, mode, type);
#endif
        /* add a margin */
        height += 2;
    }

    return height;
}

/* Provides a graphical means of editing the EQ settings */
bool eq_menu_graphical(void)
{
    bool exit_request = false;
    bool result = true;
    bool has_changed = false;
    int button;
    int *setting;
    int current_band, y, step, fast_step, min, max, voice_unit;
    enum eq_slider_mode mode;
    enum eq_type current_type;
    char buf[24];
    int i;

    FOR_NB_SCREENS(i) {
        screens[i].setfont(FONT_SYSFIXED);
        screens[i].clear_display();
    }
    
    /* Start off editing gain on the first band */
    mode = GAIN;
    current_type = LOW_SHELF;
    current_band = 0;
    
    while (!exit_request) {
        
        FOR_NB_SCREENS(i) {
            /* Clear the screen. The drawing routines expect this */
            screens[i].clear_display();        
            /* Draw equalizer band details */
            y = draw_eq_sliders(current_band, mode);
        }

        /* Set pointer to the band data currently editable */
        if (mode == GAIN) {
            /* gain */
            setting = &global_settings.eq_band0_gain;
            setting += current_band * 3;

            step = EQ_GAIN_STEP;
            fast_step = EQ_GAIN_FAST_STEP;
            min = EQ_GAIN_MIN;
            max = EQ_GAIN_MAX;
            voice_unit = UNIT_DB;
            
            snprintf(buf, sizeof(buf), str(LANG_SYSFONT_EQUALIZER_EDIT_MODE),
                str(LANG_SYSFONT_GAIN));
            
            screens[SCREEN_MAIN].putsxy(2, y, buf);
        } else if (mode == CUTOFF) {
            /* cutoff */
            setting = &global_settings.eq_band0_cutoff;
            setting += current_band * 3;

            step = EQ_CUTOFF_STEP;
            fast_step = EQ_CUTOFF_FAST_STEP;
            min = EQ_CUTOFF_MIN;
            max = EQ_CUTOFF_MAX;
            voice_unit = UNIT_HERTZ;

            snprintf(buf, sizeof(buf), str(LANG_SYSFONT_EQUALIZER_EDIT_MODE),
                str(LANG_SYSFONT_EQUALIZER_BAND_CUTOFF));
            
            screens[SCREEN_MAIN].putsxy(2, y, buf);
        } else {
            /* Q */
            setting = &global_settings.eq_band0_q;
            setting += current_band * 3;

            step = EQ_Q_STEP;
            fast_step = EQ_Q_FAST_STEP;
            min = EQ_Q_MIN;
            max = EQ_Q_MAX;
            voice_unit = UNIT_INT;
            
            snprintf(buf, sizeof(buf), str(LANG_SYSFONT_EQUALIZER_EDIT_MODE),
                str(LANG_EQUALIZER_BAND_Q));
            
            screens[SCREEN_MAIN].putsxy(2, y, buf);
        }

        FOR_NB_SCREENS(i) {
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
                current_band = 4; /* wrap around */
            break;

        case ACTION_STD_NEXT:
        case ACTION_STD_NEXTREPEAT:
            current_band++;
            if (current_band > 4)
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
            dsp_set_eq_coefs(current_band);
            has_changed = false;
        }
    }

    /* Reset screen settings */
    FOR_NB_SCREENS(i) {
        screens[i].setfont(FONT_UI);
        screens[i].clear_display();
    }
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
bool eq_browse_presets(void)
{
    return rockbox_browse(EQS_DIR, SHOW_CFG);
}

MENUITEM_FUNCTION(eq_graphical, 0, ID2P(LANG_EQUALIZER_GRAPHICAL),
                    (int(*)(void))eq_menu_graphical, NULL, NULL, 
                    Icon_EQ);
MENUITEM_FUNCTION(eq_save, 0, ID2P(LANG_EQUALIZER_SAVE),
                    (int(*)(void))eq_save_preset, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(eq_browse, 0, ID2P(LANG_EQUALIZER_BROWSE),
                    (int(*)(void))eq_browse_presets, NULL, NULL, Icon_NOICON);

int soundmenu_callback(int action,const struct menu_item_ex *this_item);
MAKE_MENU(equalizer_menu, ID2P(LANG_EQUALIZER), soundmenu_callback, Icon_EQ,
        &eq_enable, &eq_graphical, &eq_precut, &gain_menu, 
        &advanced_eq_menu_, &eq_save, &eq_browse);

