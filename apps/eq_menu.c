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
#include "button.h"
#include "mp3_playback.h"
#include "settings.h"
#include "statusbar.h"
#include "screens.h"
#include "icons.h"
#include "font.h"
#include "widgets.h"
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

/* Key definitions */
#if (CONFIG_KEYPAD == IRIVER_H100_PAD || \
     CONFIG_KEYPAD == IRIVER_H300_PAD)

#define EQ_BTN_MODIFIER     BUTTON_ON
#define EQ_BTN_DECREMENT    BUTTON_LEFT
#define EQ_BTN_INCREMENT    BUTTON_RIGHT
#define EQ_BTN_NEXT_BAND    BUTTON_DOWN
#define EQ_BTN_PREV_BAND    BUTTON_UP
#define EQ_BTN_CHANGE_MODE  BUTTON_SELECT
#define EQ_BTN_EXIT         BUTTON_OFF

#elif (CONFIG_KEYPAD == IPOD_4G_PAD)

#define EQ_BTN_DECREMENT    BUTTON_SCROLL_BACK
#define EQ_BTN_INCREMENT    BUTTON_SCROLL_FWD
#define EQ_BTN_NEXT_BAND    BUTTON_RIGHT
#define EQ_BTN_PREV_BAND    BUTTON_LEFT
#define EQ_BTN_CHANGE_MODE  BUTTON_SELECT
#define EQ_BTN_EXIT         BUTTON_MENU

#elif CONFIG_KEYPAD == IAUDIO_X5_PAD

#define EQ_BTN_DECREMENT    BUTTON_LEFT
#define EQ_BTN_INCREMENT    BUTTON_RIGHT
#define EQ_BTN_NEXT_BAND    BUTTON_DOWN
#define EQ_BTN_PREV_BAND    BUTTON_UP
#define EQ_BTN_CHANGE_MODE  BUTTON_REC
#define EQ_BTN_EXIT         BUTTON_MENU

#elif (CONFIG_KEYPAD == IRIVER_IFP7XX_PAD)

#define EQ_BTN_DECREMENT    BUTTON_LEFT
#define EQ_BTN_INCREMENT    BUTTON_RIGHT
#define EQ_BTN_NEXT_BAND    BUTTON_DOWN
#define EQ_BTN_PREV_BAND    BUTTON_UP
#define EQ_BTN_CHANGE_MODE  BUTTON_SELECT
#define EQ_BTN_EXIT         BUTTON_PLAY

#endif


#define EQ_CUTOFF_MIN        20
#define EQ_CUTOFF_MAX     22040
#define EQ_CUTOFF_STEP       10
#define EQ_CUTOFF_FAST_STEP 100
#define EQ_GAIN_MIN       (-240)
#define EQ_GAIN_MAX         240
#define EQ_GAIN_STEP          1
#define EQ_GAIN_FAST_STEP    10
#define EQ_Q_MIN              5
#define EQ_Q_MAX             64
#define EQ_Q_STEP             1
#define EQ_Q_FAST_STEP       10

#define EQ_USER_DIVISOR      10

static bool eq_enabled(void)
{
    int i;
    
    bool result = set_bool(str(LANG_EQUALIZER_ENABLED),
        &global_settings.eq_enabled);

    /* Update all bands */
    for(i = 0; i < 5; i++) {
        dsp_eq_update_data(global_settings.eq_enabled, i);
    }

    return result;
}

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

/* Possibly dodgy way of simplifying the code a bit. */
#define eq_make_gain_label(buf, bufsize, frequency) snprintf((buf), \
    (bufsize), str(LANG_EQUALIZER_GAIN_ITEM), (frequency))

#define eq_set_center(band) \
static bool eq_set_band ## band ## _center(void) \
{ \
    bool result = set_int(str(LANG_EQUALIZER_BAND_CENTER), "Hertz", UNIT_HERTZ, \
        &global_settings.eq_band ## band ## _cutoff, NULL, \
        EQ_CUTOFF_STEP, EQ_CUTOFF_MIN, EQ_CUTOFF_MAX, NULL); \
    dsp_eq_update_data(global_settings.eq_enabled, band); \
    return result; \
}
    
#define eq_set_cutoff(band) \
static bool eq_set_band ## band ## _cutoff(void) \
{ \
    bool result = set_int(str(LANG_EQUALIZER_BAND_CUTOFF), "Hertz", UNIT_HERTZ, \
        &global_settings.eq_band ## band ## _cutoff, NULL, \
        EQ_CUTOFF_STEP, EQ_CUTOFF_MIN, EQ_CUTOFF_MAX, NULL); \
    dsp_eq_update_data(global_settings.eq_enabled, band); \
    return result; \
}

#define eq_set_q(band) \
static bool eq_set_band ## band ## _q(void) \
{ \
    bool result = set_int(str(LANG_EQUALIZER_BAND_Q), "Q", UNIT_INT, \
        &global_settings.eq_band ## band ## _q, NULL, \
        EQ_Q_STEP, EQ_Q_MIN, EQ_Q_MAX, eq_q_format); \
    dsp_eq_update_data(global_settings.eq_enabled, band); \
    return result; \
}

#define eq_set_gain(band) \
static bool eq_set_band ## band ## _gain(void) \
{ \
    bool result = set_int("Band " #band, str(LANG_UNIT_DB), UNIT_DB, \
        &global_settings.eq_band ## band ## _gain, NULL, \
        EQ_GAIN_STEP, EQ_GAIN_MIN, EQ_GAIN_MAX, eq_gain_format); \
    dsp_eq_update_data(global_settings.eq_enabled, band); \
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
    char gain_label[5][24];
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

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
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

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
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

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
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

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
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

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
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

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
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

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);

    return result;
}

#define SLIDER_KNOB_HEIGHT  6
#define SLIDER_KNOB_WIDTH 4

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

/* Draw a slider */
static void draw_slider(const struct screen * screen, int x, int y,
                           int width, int steps, int current_step)
{
    int knob_x = ((width * 100 / steps) * current_step) / 100
        + (SLIDER_KNOB_WIDTH / 2);

    /* Draw groove */
    screen->fillrect(x, y + 2, width, 2);

    /* Draw knob */
    screen->fillrect(x + knob_x, y, SLIDER_KNOB_WIDTH, SLIDER_KNOB_HEIGHT);
}

/* Draw the UI for a whole EQ band */
static int draw_eq_slider(const struct screen * screen, int x, int y,
    int width, int cutoff, int q, int gain, bool selected,
    enum eq_slider_mode mode, enum eq_type type)
{
    char buf[26];
    const char separator[2] = " ";
    int steps = EQ_GAIN_MAX - EQ_GAIN_MIN;
    int abs_q = abs(q);
    int abs_gain = abs(gain);
    int current_x, total_height, separator_width, separator_height;
    int w, h;

    /* Start two pixels in, one for border, one for margin */
    current_x = x + 2;

    /* Figure out how large our separator string is */
    screen->getstringsize(separator, &separator_width, &separator_height);
    
    /* Total height includes margins, text, and line selector */
    total_height = separator_height + SLIDER_KNOB_HEIGHT + 2 + 3;
    
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
    snprintf(buf, sizeof(buf), "%d.%d Q", abs_q / EQ_USER_DIVISOR,
        abs_q % EQ_USER_DIVISOR);

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

    /* Draw horizontal slider */
    draw_slider(screen, x + 3, y + h + 3, width - 6, steps,
        abs(EQ_GAIN_MIN) + gain);

    return total_height;
}

/* Draw's all the EQ sliders. Returns the total height of the sliders drawn */
static int draw_eq_sliders(int current_band, enum eq_slider_mode mode)
{
    int i, gain, q, cutoff;
    int height = 2; /* Two pixel margin */
    int slider_width = screens[SCREEN_MAIN].width - 4; /* two pixel margin on each side */
    int *setting = &global_settings.eq_band0_cutoff;
    enum eq_type type;

    for( i = 0; i < 5; ++i) {
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
            slider_width, cutoff, q, gain, i == current_band, mode, type);

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

    screens[SCREEN_MAIN].setfont(FONT_SYSFIXED);
    screens[SCREEN_MAIN].clear_display();
    
    /* Start off editing gain on the first band */
    mode = GAIN;
    current_type = LOW_SHELF;
    current_band = 0;
    
    while (!exit_request) {
        /* Clear the screen. The drawing routines expect this */
        screens[SCREEN_MAIN].clear_display();
        
        /* Draw equalizer band details */
        y = draw_eq_sliders(current_band, mode);

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
            
            snprintf(buf, sizeof(buf), str(LANG_EQUALIZER_EDIT_MODE),
                str(LANG_EQUALIZER_BAND_GAIN));
            
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

            snprintf(buf, sizeof(buf), str(LANG_EQUALIZER_EDIT_MODE),
                str(LANG_EQUALIZER_BAND_CUTOFF));
            
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
            
            snprintf(buf, sizeof(buf), str(LANG_EQUALIZER_EDIT_MODE),
                str(LANG_EQUALIZER_BAND_Q));
            
            screens[SCREEN_MAIN].putsxy(2, y, buf);
        }

        screens[SCREEN_MAIN].update();
        
        button = button_get(true);

        switch (button) {
        case EQ_BTN_DECREMENT:
        case EQ_BTN_DECREMENT | BUTTON_REPEAT:
            *(setting) -= step;
            has_changed = true;
            if (*(setting) < min)
                *(setting) = min;
            break;

        case EQ_BTN_INCREMENT:
        case EQ_BTN_INCREMENT | BUTTON_REPEAT:
            *(setting) += step;
            has_changed = true;
            if (*(setting) > max)
                *(setting) = max;
            break;

#ifdef EQ_BTN_MODIFIER
        case EQ_BTN_MODIFIER | EQ_BTN_INCREMENT:
        case EQ_BTN_MODIFIER | EQ_BTN_INCREMENT | BUTTON_REPEAT:
            *(setting) += fast_step;
            has_changed = true;
            if (*(setting) > max)
                *(setting) = max;
            break;

        case EQ_BTN_MODIFIER | EQ_BTN_DECREMENT:
        case EQ_BTN_MODIFIER | EQ_BTN_DECREMENT | BUTTON_REPEAT:
            *(setting) -= fast_step;
            has_changed = true;
            if (*(setting) < min)
                *(setting) = min;
            break;
#endif

        case EQ_BTN_PREV_BAND:
        case EQ_BTN_PREV_BAND | BUTTON_REPEAT:
            current_band--;
            if (current_band < 0)
                current_band = 4; /* wrap around */
            break;

        case EQ_BTN_NEXT_BAND:
        case EQ_BTN_NEXT_BAND | BUTTON_REPEAT:
            current_band++;
            if (current_band > 4)
                current_band = 0; /* wrap around */
            break;

        case EQ_BTN_CHANGE_MODE:
        case EQ_BTN_CHANGE_MODE | BUTTON_REPEAT:
            mode++;
            if (mode > Q)
                mode = GAIN; /* wrap around */
            break;

        case EQ_BTN_EXIT:
        case EQ_BTN_EXIT | BUTTON_REPEAT:
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
            dsp_eq_update_data(global_settings.eq_enabled, current_band);
            has_changed = false;
        }
    }

    /* Reset screen settings */
    screens[SCREEN_MAIN].setfont(FONT_UI);
    screens[SCREEN_MAIN].clear_display();
    
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

    create_numbered_filename(filename, EQS_DIR, "eq", ".cfg", 2);

    /* allow user to modify filename */
    while (true) {
        if (!kbd_input(filename, sizeof filename)) {
            fd = creat(filename,0);
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
    
    setting = &global_settings.eq_band0_cutoff;
    
    for(i = 0; i < 5; ++i) {
        fdprintf(fd, "eq band %d cutoff: %d\r\n", i, *setting++);
        fdprintf(fd, "eq band %d q: %d\r\n", i, *setting++);
        fdprintf(fd, "eq band %d gain: %d\r\n", i, *setting++);
    }

    close(fd);

    gui_syncsplash(HZ, true, (unsigned char *)"%s %s", str(LANG_SETTINGS_SAVED1),
           str(LANG_SETTINGS_SAVED2));

    return true;
}

/* Allows browsing of preset files */
bool eq_browse_presets(void)
{
    return rockbox_browse(EQS_DIR, SHOW_CFG);
}

/* Full equalizer menu */
bool eq_menu(void)
{
    int m;
    bool result;
    static const struct menu_item items[] = {
        { ID2P(LANG_EQUALIZER_ENABLED), eq_enabled },
        { ID2P(LANG_EQUALIZER_GRAPHICAL), eq_menu_graphical },
        { ID2P(LANG_EQUALIZER_GAIN), eq_gain_menu },
        { ID2P(LANG_EQUALIZER_ADVANCED), eq_advanced_menu },
        { ID2P(LANG_EQUALIZER_SAVE), eq_save_preset },
        { ID2P(LANG_EQUALIZER_BROWSE), eq_browse_presets },
    };

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);

    return result;
}

