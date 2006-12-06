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
#ifdef HAVE_WM8758
#include "wm8758.h"
#endif

/* Various user interface limits and sizes */
#define EQ_CUTOFF_MIN        20
#define EQ_CUTOFF_MAX     22040
#define EQ_CUTOFF_STEP       10
#define EQ_CUTOFF_FAST_STEP 100
#define EQ_GAIN_MIN       (-240)
#define EQ_GAIN_MAX         240
#define EQ_GAIN_STEP          5
#define EQ_GAIN_FAST_STEP    10
#define EQ_Q_MIN              5
#define EQ_Q_MAX             64
#define EQ_Q_STEP             1
#define EQ_Q_FAST_STEP       10

#define EQ_USER_DIVISOR      10

/*
 * Utility functions
 */

static void eq_gain_format(char* buffer, int buffer_size, int value, const char* unit)
{
    int v = abs(value);

    snprintf(buffer, buffer_size, "%s%d.%d %s", value < 0 ? "-" : "",
        v / EQ_USER_DIVISOR, v % EQ_USER_DIVISOR, unit);
}

static void eq_q_format(char* buffer, int buffer_size, int value, const char* unit)
{
    snprintf(buffer, buffer_size, "%d.%d %s", value / EQ_USER_DIVISOR, value % EQ_USER_DIVISOR, unit);
}

static void eq_precut_format(char* buffer, int buffer_size, int value, const char* unit)
{
    snprintf(buffer, buffer_size, "%s%d.%d %s", value == 0 ? " " : "-",
        value / EQ_USER_DIVISOR, value % EQ_USER_DIVISOR, unit);
}

/*
 * Settings functions
 */

static bool eq_enabled(void)
{
    int i;

    bool result = set_bool(str(LANG_EQUALIZER_ENABLED),
        &global_settings.eq_enabled);

    dsp_set_eq(global_settings.eq_enabled); 

    dsp_set_eq_precut(global_settings.eq_precut);

    /* Update all bands */
    for(i = 0; i < 5; i++) {
        dsp_set_eq_coefs(i);
    }

    return result;
}

static bool eq_precut(void)
{
    bool result = set_int(str(LANG_EQUALIZER_PRECUT), str(LANG_UNIT_DB), 
        UNIT_DB, &global_settings.eq_precut, dsp_set_eq_precut, 5, 0, 240, 
        eq_precut_format);

    return result;
}

/* Possibly dodgy way of simplifying the code a bit. */
#define eq_make_gain_label(buf, bufsize, frequency) snprintf((buf), \
    (bufsize), str(LANG_EQUALIZER_GAIN_ITEM), (frequency))

#define eq_set_center(band) \
static bool eq_set_band ## band ## _center(void) \
{ \
    bool result = set_int(str(LANG_EQUALIZER_BAND_CENTER), "Hertz", \
        UNIT_HERTZ, &global_settings.eq_band ## band ## _cutoff, NULL, \
        EQ_CUTOFF_STEP, EQ_CUTOFF_MIN, EQ_CUTOFF_MAX, NULL); \
    dsp_set_eq_coefs(band); \
    return result; \
}
    
#define eq_set_cutoff(band) \
static bool eq_set_band ## band ## _cutoff(void) \
{ \
    bool result = set_int(str(LANG_EQUALIZER_BAND_CUTOFF), "Hertz", \
        UNIT_HERTZ, &global_settings.eq_band ## band ## _cutoff, NULL, \
        EQ_CUTOFF_STEP, EQ_CUTOFF_MIN, EQ_CUTOFF_MAX, NULL); \
    dsp_set_eq_coefs(band); \
    return result; \
}

#define eq_set_q(band) \
static bool eq_set_band ## band ## _q(void) \
{ \
    bool result = set_int(str(LANG_EQUALIZER_BAND_Q), "Q", UNIT_INT, \
        &global_settings.eq_band ## band ## _q, NULL, \
        EQ_Q_STEP, EQ_Q_MIN, EQ_Q_MAX, eq_q_format); \
    dsp_set_eq_coefs(band); \
    return result; \
}

#define eq_set_gain(band) \
static bool eq_set_band ## band ## _gain(void) \
{ \
    bool result = set_int("Band " #band, str(LANG_UNIT_DB), UNIT_DB, \
        &global_settings.eq_band ## band ## _gain, NULL, \
        EQ_GAIN_STEP, EQ_GAIN_MIN, EQ_GAIN_MAX, eq_gain_format); \
    dsp_set_eq_coefs(band); \
    return result; \
}

eq_set_cutoff(0);
eq_set_center(1);
eq_set_center(2);
eq_set_center(3);
eq_set_cutoff(4);

eq_set_q(0);
eq_set_q(1);
eq_set_q(2);
eq_set_q(3);
eq_set_q(4);

eq_set_gain(0);
eq_set_gain(1);
eq_set_gain(2);
eq_set_gain(3);
eq_set_gain(4);

static bool eq_gain_menu(void)
{
    int m, i;
    int *setting;
    bool result;
    char gain_label[5][32];
    static struct menu_item items[5] = {
        { NULL, eq_set_band0_gain },
        { NULL, eq_set_band1_gain },
        { NULL, eq_set_band2_gain },
        { NULL, eq_set_band3_gain },
        { NULL, eq_set_band4_gain },
    };

    setting = &global_settings.eq_band0_cutoff;
    
    /* Construct menu labels */
    for(i = 0; i < 5; i++) {
        eq_make_gain_label(gain_label[i], sizeof(gain_label[i]),
            *setting);
        items[i].desc = gain_label[i];
        
        /* Skip to next band */
        setting += 3;
    }

    m = menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);

    return result;
}

static bool eq_set_band0(void)
{
    int m;
    bool result;
    static const struct menu_item items[] = {
        { ID2P(LANG_EQUALIZER_BAND_CUTOFF), eq_set_band0_cutoff },
        { ID2P(LANG_EQUALIZER_BAND_Q), eq_set_band0_q },
        { ID2P(LANG_EQUALIZER_BAND_GAIN), eq_set_band0_gain },
    };

    m = menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);

    return result;
}

static bool eq_set_band1(void)
{
    int m;
    bool result;
    static const struct menu_item items[] = {
        { ID2P(LANG_EQUALIZER_BAND_CENTER), eq_set_band1_center },
        { ID2P(LANG_EQUALIZER_BAND_Q), eq_set_band1_q },
        { ID2P(LANG_EQUALIZER_BAND_GAIN), eq_set_band1_gain },
    };

    m = menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);

    return result;
}

static bool eq_set_band2(void)
{
    int m;
    bool result;
    static const struct menu_item items[] = {
        { ID2P(LANG_EQUALIZER_BAND_CENTER), eq_set_band2_center },
        { ID2P(LANG_EQUALIZER_BAND_Q), eq_set_band2_q },
        { ID2P(LANG_EQUALIZER_BAND_GAIN), eq_set_band2_gain },
    };

    m = menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);

    return result;
}

static bool eq_set_band3(void)
{
    int m;
    bool result;
    static const struct menu_item items[] = {
        { ID2P(LANG_EQUALIZER_BAND_CENTER), eq_set_band3_center },
        { ID2P(LANG_EQUALIZER_BAND_Q), eq_set_band3_q },
        { ID2P(LANG_EQUALIZER_BAND_GAIN), eq_set_band3_gain },
    };

    m = menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);

    return result;
}

static bool eq_set_band4(void)
{
    int m;
    bool result;
    static const struct menu_item items[] = {
        { ID2P(LANG_EQUALIZER_BAND_CUTOFF), eq_set_band4_cutoff },
        { ID2P(LANG_EQUALIZER_BAND_Q), eq_set_band4_q },
        { ID2P(LANG_EQUALIZER_BAND_GAIN), eq_set_band4_gain },
    };

    m = menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);

    return result;
}

static bool eq_advanced_menu(void)
{
    int m, i;
    bool result;
    char peak_band_label[3][32];
    static struct menu_item items[] = {
        { ID2P(LANG_EQUALIZER_BAND_LOW_SHELF), eq_set_band0 },
        { NULL, eq_set_band1 },
        { NULL, eq_set_band2 },
        { NULL, eq_set_band3 },
        { ID2P(LANG_EQUALIZER_BAND_HIGH_SHELF), eq_set_band4 },
    };

    /* Construct menu labels */
    for(i = 1; i < 4; i++) {
        snprintf(peak_band_label[i-1], sizeof(peak_band_label[i-1]),
            str(LANG_EQUALIZER_BAND_PEAK), i);
        items[i].desc = peak_band_label[i-1];
    }

    m = menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);

    return result;
}

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
            screen->putsxy(current_x, y + 2, str(LANG_EQUALIZER_BAND_GAIN));
            screen->getstringsize(str(LANG_EQUALIZER_BAND_GAIN), &w, &h);
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
                str(LANG_SYSFONT_EQUALIZER_BAND_GAIN));
            
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

    action_signalscreenchange();
    /* Reset screen settings */
    FOR_NB_SCREENS(i) {
        screens[i].setfont(FONT_UI);
        screens[i].clear_display();
    }
    return result;
}

/* Preset saver.
 * TODO: Can the settings system be used to do this instead?
 */
static bool eq_save_preset(void)
{
    int fd, i;
    char filename[MAX_PATH];
    int *setting;

    create_numbered_filename(filename, EQS_DIR, "eq", ".cfg", 2
        IF_CNFN_NUM_(, NULL));

    /* allow user to modify filename */
    while (true) {
        if (!kbd_input(filename, sizeof filename)) {
            fd = creat(filename, O_WRONLY);
            if (fd < 0)
                gui_syncsplash(HZ, true, str(LANG_FAILED));
            else
                break;
        }
        else {
            gui_syncsplash(HZ, true, str(LANG_MENU_SETTING_CANCEL));
            return false;
        }
    }

    /* TODO: Should we really do this? */
    fdprintf(fd, "eq enabled: on\r\n");
    fdprintf(fd, "eq precut: %d\r\n", global_settings.eq_precut);
    
    setting = &global_settings.eq_band0_cutoff;
    
    for(i = 0; i < 5; ++i) {
        fdprintf(fd, "eq band %d cutoff: %d\r\n", i, *setting++);
        fdprintf(fd, "eq band %d q: %d\r\n", i, *setting++);
        fdprintf(fd, "eq band %d gain: %d\r\n", i, *setting++);
    }

    close(fd);

    gui_syncsplash(HZ, true, str(LANG_SETTINGS_SAVED));

    return true;
}

/* Allows browsing of preset files */
bool eq_browse_presets(void)
{
    return rockbox_browse(EQS_DIR, SHOW_CFG);
}

#ifdef HAVE_WM8758

/* WM8758 equalizer supports -12 to +12 dB gain in 1 dB increments. */
#define EQ_HW_GAIN_STEP 1
#define EQ_HW_GAIN_MIN -12
#define EQ_HW_GAIN_MAX 12

static const struct opt_items BANDWIDTH_NAMES[] = {
    { STR(LANG_EQUALIZER_HARDWARE_BANDWIDTH_NARROW) },
    { STR(LANG_EQUALIZER_HARDWARE_BANDWIDTH_WIDE) },
};

static const int BANDWIDTH_NAMES_SIZE = sizeof(BANDWIDTH_NAMES) /
                                        sizeof(*BANDWIDTH_NAMES);

static void eq_hw_gain_format(char* buffer, int buffer_size, int value,
                                    const char* unit)
{
    snprintf(buffer, buffer_size, "%d %s", value, unit);
}

static bool eq_hw_set_band0_cutoff(void)
{
    static const struct opt_items names[] = {
        { (unsigned char *)"80 Hz", TALK_ID(80, UNIT_HERTZ) },
        { (unsigned char *)"105 Hz", TALK_ID(105, UNIT_HERTZ) },
        { (unsigned char *)"135 Hz", TALK_ID(135, UNIT_HERTZ) },
        { (unsigned char *)"175 Hz", TALK_ID(175, UNIT_HERTZ) },
    };

    bool result = set_option(str(LANG_EQUALIZER_BANDWIDTH),
        &global_settings.eq_hw_band0_cutoff, INT, names,
        sizeof(names) / sizeof(*names), NULL);

#ifndef SIMULATOR
    audiohw_set_equalizer_band(0, global_settings.eq_hw_band0_cutoff, 0,
                                global_settings.eq_hw_band0_gain);
#endif

    return result;
}

static bool eq_hw_set_band0_gain(void) 
{
    bool result = set_int(str(LANG_EQUALIZER_BAND_GAIN), str(LANG_UNIT_DB), UNIT_DB,
        &global_settings.eq_hw_band0_gain, NULL,
        EQ_HW_GAIN_STEP, EQ_HW_GAIN_MIN, EQ_HW_GAIN_MAX,
        eq_hw_gain_format);
        
#ifndef SIMULATOR
    audiohw_set_equalizer_band(0, global_settings.eq_hw_band0_cutoff, 0,
                                global_settings.eq_hw_band0_gain);
#endif

    return result;
}

static bool eq_hw_set_band1_center(void)
{
    static const struct opt_items names[] = {
        { (unsigned char *)"230 Hz", TALK_ID(230, UNIT_HERTZ) },
        { (unsigned char *)"300 Hz", TALK_ID(300, UNIT_HERTZ) },
        { (unsigned char *)"385 Hz", TALK_ID(385, UNIT_HERTZ) },
        { (unsigned char *)"500 Hz", TALK_ID(500, UNIT_HERTZ) },
    };
    
    bool result = set_option(str(LANG_EQUALIZER_BAND_CENTER),
        &global_settings.eq_hw_band1_center, INT, names,
        sizeof(names) / sizeof(*names), NULL);

#ifndef SIMULATOR
    audiohw_set_equalizer_band(1, global_settings.eq_hw_band1_center,
                                global_settings.eq_hw_band1_bandwidth,
                                global_settings.eq_hw_band1_gain);
#endif

    return result;
}

static bool eq_hw_set_band1_bandwidth(void)
{
    bool result = set_option(str(LANG_EQUALIZER_BANDWIDTH),
        &global_settings.eq_hw_band1_bandwidth, INT, BANDWIDTH_NAMES,
        BANDWIDTH_NAMES_SIZE, NULL);

#ifndef SIMULATOR
    audiohw_set_equalizer_band(1, global_settings.eq_hw_band1_center,
                                global_settings.eq_hw_band1_bandwidth,
                                global_settings.eq_hw_band1_gain);
#endif

    return result;
}

static bool eq_hw_set_band1_gain(void) 
{
    bool result = set_int(str(LANG_EQUALIZER_BAND_GAIN), str(LANG_UNIT_DB), UNIT_DB,
        &global_settings.eq_hw_band1_gain, NULL,
        EQ_HW_GAIN_STEP, EQ_HW_GAIN_MIN, EQ_HW_GAIN_MAX,
        eq_hw_gain_format);
        
#ifndef SIMULATOR
    audiohw_set_equalizer_band(1, global_settings.eq_hw_band1_center,
                                global_settings.eq_hw_band1_bandwidth,
                                global_settings.eq_hw_band1_gain);
#endif

    return result;
}

static bool eq_hw_set_band2_center(void)
{
    static const struct opt_items names[] = {
        { (unsigned char *)"650 Hz", TALK_ID(650, UNIT_HERTZ) },
        { (unsigned char *)"850 Hz", TALK_ID(850, UNIT_HERTZ) },
        { (unsigned char *)"1.1 kHz", TALK_ID(1100, UNIT_HERTZ) },
        { (unsigned char *)"1.4 kHz", TALK_ID(1400, UNIT_HERTZ) },
    };
    
    bool result = set_option(str(LANG_EQUALIZER_BAND_CENTER),
        &global_settings.eq_hw_band2_center, INT, names,
        sizeof(names) / sizeof(*names), NULL);

#ifndef SIMULATOR
    audiohw_set_equalizer_band(2, global_settings.eq_hw_band2_center,
                                global_settings.eq_hw_band2_bandwidth,
                                global_settings.eq_hw_band2_gain);
#endif

    return result;
}

static bool eq_hw_set_band2_bandwidth(void)
{   
    bool result = set_option(str(LANG_EQUALIZER_BANDWIDTH),
        &global_settings.eq_hw_band2_bandwidth, INT, BANDWIDTH_NAMES,
        BANDWIDTH_NAMES_SIZE, NULL);

#ifndef SIMULATOR
    audiohw_set_equalizer_band(2, global_settings.eq_hw_band2_center,
                                global_settings.eq_hw_band2_bandwidth,
                                global_settings.eq_hw_band2_gain);
#endif

    return result;
}

static bool eq_hw_set_band2_gain(void) 
{
    bool result = set_int(str(LANG_EQUALIZER_BAND_GAIN), str(LANG_UNIT_DB), UNIT_DB,
        &global_settings.eq_hw_band2_gain, NULL,
        EQ_HW_GAIN_STEP, EQ_HW_GAIN_MIN, EQ_HW_GAIN_MAX,
        eq_hw_gain_format);
        
#ifndef SIMULATOR
    audiohw_set_equalizer_band(2, global_settings.eq_hw_band2_center,
                                global_settings.eq_hw_band2_bandwidth,
                                global_settings.eq_hw_band2_gain);
#endif

    return result;
}

static bool eq_hw_set_band3_center(void)
{
    static const struct opt_items names[] = {
        { (unsigned char *)"1.8 kHz", TALK_ID(1800, UNIT_HERTZ) },
        { (unsigned char *)"2.4 kHz", TALK_ID(2400, UNIT_HERTZ) },
        { (unsigned char *)"3.2 kHz", TALK_ID(3200, UNIT_HERTZ) },
        { (unsigned char *)"4.1 kHz", TALK_ID(4100, UNIT_HERTZ) },
    };
    
    bool result = set_option(str(LANG_EQUALIZER_BAND_CENTER),
        &global_settings.eq_hw_band3_center, INT, names,
        sizeof(names) / sizeof(*names), NULL);

#ifndef SIMULATOR
    audiohw_set_equalizer_band(3, global_settings.eq_hw_band3_center,
                                global_settings.eq_hw_band3_bandwidth,
                                global_settings.eq_hw_band3_gain);
#endif

    return result;
}

static bool eq_hw_set_band3_bandwidth(void)
{
    bool result = set_option(str(LANG_EQUALIZER_BANDWIDTH),
        &global_settings.eq_hw_band3_bandwidth, INT, BANDWIDTH_NAMES,
        BANDWIDTH_NAMES_SIZE, NULL);

#ifndef SIMULATOR
    audiohw_set_equalizer_band(3, global_settings.eq_hw_band3_center,
                                global_settings.eq_hw_band3_bandwidth,
                                global_settings.eq_hw_band3_gain);
#endif

    return result;
}

static bool eq_hw_set_band3_gain(void) 
{
    bool result = set_int(str(LANG_EQUALIZER_BAND_GAIN), str(LANG_UNIT_DB), UNIT_DB,
        &global_settings.eq_hw_band3_gain, NULL,
        EQ_HW_GAIN_STEP, EQ_HW_GAIN_MIN, EQ_HW_GAIN_MAX,
        eq_hw_gain_format);
        
#ifndef SIMULATOR
    audiohw_set_equalizer_band(3, global_settings.eq_hw_band3_center,
                                global_settings.eq_hw_band3_bandwidth,
                                global_settings.eq_hw_band3_gain);
#endif

    return result;
}

static bool eq_hw_set_band4_cutoff(void)
{
    static const struct opt_items names[] = {
        { (unsigned char *)"5.3 kHz", TALK_ID(5300, UNIT_HERTZ) },
        { (unsigned char *)"6.9 kHz", TALK_ID(6900, UNIT_HERTZ) },
        { (unsigned char *)"9.0 kHz", TALK_ID(9000, UNIT_HERTZ) },
        { (unsigned char *)"11.7 kHz", TALK_ID(11700, UNIT_HERTZ) },
    };
    
    bool result = set_option(str(LANG_EQUALIZER_BAND_CUTOFF),
        &global_settings.eq_hw_band4_cutoff, INT, names,
        sizeof(names) / sizeof(*names), NULL);

#ifndef SIMULATOR
    audiohw_set_equalizer_band(4, global_settings.eq_hw_band4_cutoff, 0,
                                global_settings.eq_hw_band4_gain);
#endif

    return result;
}

static bool eq_hw_set_band4_gain(void) 
{
    bool result = set_int(str(LANG_EQUALIZER_BAND_GAIN), str(LANG_UNIT_DB), UNIT_DB,
        &global_settings.eq_hw_band4_gain, NULL,
        EQ_HW_GAIN_STEP, EQ_HW_GAIN_MIN, EQ_HW_GAIN_MAX,
        eq_hw_gain_format);
        
#ifndef SIMULATOR
    audiohw_set_equalizer_band(4, global_settings.eq_hw_band4_cutoff, 0,
                                global_settings.eq_hw_band4_gain);
#endif

    return result;
}

void eq_hw_enable(bool enable)
{
#ifdef SIMULATOR
    (void) enable;
#else
    if (enable) {
        audiohw_set_equalizer_band(0, global_settings.eq_hw_band0_cutoff,
                                   0, global_settings.eq_hw_band0_gain);
        audiohw_set_equalizer_band(1, global_settings.eq_hw_band1_center,
                                   global_settings.eq_hw_band1_bandwidth,
                                   global_settings.eq_hw_band1_gain);
        audiohw_set_equalizer_band(2, global_settings.eq_hw_band2_center,
                                   global_settings.eq_hw_band2_bandwidth,
                                   global_settings.eq_hw_band2_gain);
        audiohw_set_equalizer_band(3, global_settings.eq_hw_band3_center,
                                   global_settings.eq_hw_band3_bandwidth,
                                   global_settings.eq_hw_band3_gain);
        audiohw_set_equalizer_band(4, global_settings.eq_hw_band4_cutoff,
                                   0, global_settings.eq_hw_band4_gain);
    } else {
        audiohw_set_equalizer_band(0, global_settings.eq_hw_band0_cutoff, 0, 0);
        audiohw_set_equalizer_band(1, global_settings.eq_hw_band1_center,
                                   global_settings.eq_hw_band1_bandwidth, 0);
        audiohw_set_equalizer_band(2, global_settings.eq_hw_band2_center,
                                   global_settings.eq_hw_band2_bandwidth, 0);
        audiohw_set_equalizer_band(3, global_settings.eq_hw_band3_center,
                                   global_settings.eq_hw_band3_bandwidth, 0);
        audiohw_set_equalizer_band(4, global_settings.eq_hw_band4_cutoff, 0, 0);
    }
#endif
}

static bool eq_hw_enabled(void)
{
    bool result = set_bool(str(LANG_EQUALIZER_HARDWARE_ENABLED),
        &global_settings.eq_hw_enabled);

    eq_hw_enable(global_settings.eq_hw_enabled);

    return result;
}

static bool eq_hw_set_band0(void)
{
    int m;
    bool result;
    static const struct menu_item items[] = {
        { ID2P(LANG_EQUALIZER_BAND_CUTOFF), eq_hw_set_band0_cutoff },
        { ID2P(LANG_EQUALIZER_BAND_GAIN), eq_hw_set_band0_gain },
    };

    m = menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);

    return result;
}

static bool eq_hw_set_band1(void)
{
    int m;
    bool result;
    static const struct menu_item items[] = {
        { ID2P(LANG_EQUALIZER_BAND_CENTER), eq_hw_set_band1_center },
        { ID2P(LANG_EQUALIZER_BANDWIDTH), eq_hw_set_band1_bandwidth },
        { ID2P(LANG_EQUALIZER_BAND_GAIN), eq_hw_set_band1_gain },
    };

    m = menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);

    return result;
}

static bool eq_hw_set_band2(void)
{
    int m;
    bool result;
    static const struct menu_item items[] = {
        { ID2P(LANG_EQUALIZER_BAND_CENTER), eq_hw_set_band2_center },
        { ID2P(LANG_EQUALIZER_BANDWIDTH), eq_hw_set_band2_bandwidth },
        { ID2P(LANG_EQUALIZER_BAND_GAIN), eq_hw_set_band2_gain }, 
    };

    m = menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);

    return result;
}

static bool eq_hw_set_band3(void)
{
    int m;
    bool result;
    static const struct menu_item items[] = {
        { ID2P(LANG_EQUALIZER_BAND_CENTER), eq_hw_set_band3_center },
        { ID2P(LANG_EQUALIZER_BANDWIDTH), eq_hw_set_band3_bandwidth },
        { ID2P(LANG_EQUALIZER_BAND_GAIN), eq_hw_set_band3_gain },
    };

    m = menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);

    return result;
}

static bool eq_hw_set_band4(void)
{
    int m;
    bool result;
    static const struct menu_item items[] = {
        { ID2P(LANG_EQUALIZER_BAND_CUTOFF), eq_hw_set_band4_cutoff },
        { ID2P(LANG_EQUALIZER_BAND_GAIN), eq_hw_set_band4_gain },
    };

    m = menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);

    return result;
}

bool eq_hw_menu(void)
{
    int m;
    bool result;
    static const struct menu_item items[] = {
        { ID2P(LANG_EQUALIZER_HARDWARE_ENABLED), eq_hw_enabled },
        { ID2P(LANG_EQUALIZER_BAND_LOW_SHELF), eq_hw_set_band0 },
        { "Peak Filter 1", eq_hw_set_band1 },
        { "Peak Filter 2", eq_hw_set_band2 },
        { "Peak Filter 3", eq_hw_set_band3 },        
        { ID2P(LANG_EQUALIZER_BAND_HIGH_SHELF), eq_hw_set_band4 },
    };

    m = menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);

    return result;
}
#endif

/* Full equalizer menu */
bool eq_menu(void)
{
    int m;
    bool result;
    static const struct menu_item items[] = {
        { ID2P(LANG_EQUALIZER_ENABLED), eq_enabled },
        { ID2P(LANG_EQUALIZER_GRAPHICAL), eq_menu_graphical },
        { ID2P(LANG_EQUALIZER_PRECUT), eq_precut },
        { ID2P(LANG_EQUALIZER_GAIN), eq_gain_menu },
        { ID2P(LANG_EQUALIZER_ADVANCED), eq_advanced_menu },
        { ID2P(LANG_EQUALIZER_SAVE), eq_save_preset },
        { ID2P(LANG_EQUALIZER_BROWSE), eq_browse_presets },
    };

    m = menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);

    return result;
}

