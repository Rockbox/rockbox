/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2003 Linus Nielsen Feltzing
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
#include <stdlib.h>
#include "settings.h"
#include "general.h"
#include "radio.h"
#include "tuner.h"
#include "file.h"
#include "string-extra.h"
#include "misc.h"
#include "filefuncs.h"
#include "lang.h"
#include "action.h"
#include "list.h"
#include "splash.h"
#include "menu.h"
#include "yesno.h"
#include "keyboard.h"
#include "talk.h"
#include "filetree.h"
#include "dir.h"
#include "presets.h"

static int curr_preset = -1;

extern int curr_freq; /* from radio.c.. naughty but meh */
extern int radio_mode;
int snap_freq_to_grid(int freq);
void remember_frequency(void);
void talk_freq(int freq, bool enqueue);

#define MAX_PRESETS 64
static bool presets_loaded = false;
static bool presets_changed = false;
static struct fmstation presets[MAX_PRESETS];

static char filepreset[MAX_PATH]; /* preset filename variable */

static int num_presets = 0; /* The number of presets in the preset list */

bool yesno_pop(const char* text); /* radio.c */

int radio_current_preset(void)
{
    return curr_preset;
}
int radio_preset_count(void)
{
    return num_presets;
}
const struct fmstation *radio_get_preset(int preset)
{
    return &presets[preset];
}

bool presets_have_changed(void)
{
    return presets_changed;
}


/* Find a matching preset to freq */
int preset_find(int freq)
{
    int i;
    if(num_presets < 1)
        return -1;
    for(i = 0;i < MAX_PRESETS;i++)
    {
        if(freq == presets[i].frequency)
            return i;
    }

    return -1;
}

/* Return the closest preset encountered in the search direction with
   wraparound. */
static int find_closest_preset(int freq, int direction)
{
    int i;
    int lowpreset = 0;
    int highpreset = 0;
    int closest = -1;

    if (direction == 0) /* direction == 0 isn't really used */
        return 0;

    for (i = 0; i < num_presets; i++)
    {
        int f = presets[i].frequency;
        if (f == freq)
            return i; /* Exact match = stop */
        
        /* remember the highest and lowest presets for wraparound */
        if (f < presets[lowpreset].frequency)
            lowpreset = i;
        if (f > presets[highpreset].frequency)
            highpreset = i;

        /* find the closest preset in the given direction */
        if (direction > 0 && f > freq)
        {
            if (closest < 0 || f < presets[closest].frequency)
                closest = i;
        }
        else if (direction < 0 && f < freq)
        {
            if (closest < 0 || f > presets[closest].frequency)
                closest = i;
        }
    }

    if (closest < 0)
    {
        /* no presets in the given direction */
        /* wrap around depending on direction */
        if (direction < 0)
            closest = highpreset;
        else
            closest = lowpreset;
    }

    return closest;
}

void preset_next(int direction)
{
    if (num_presets < 1)
        return;

    if (curr_preset == -1)
        curr_preset = find_closest_preset(curr_freq, direction);
    else
        curr_preset = (curr_preset + direction + num_presets) % num_presets;

    /* Must stay on the current grid for the region */
    curr_freq = snap_freq_to_grid(presets[curr_preset].frequency);

    tuner_set(RADIO_FREQUENCY, curr_freq);
    remember_frequency();
}

void preset_set_current(int preset)
{
    curr_preset = preset;
}

/* Speak a preset by number or by spelling its name, depending on settings. */
void preset_talk(int preset, bool fallback, bool enqueue)
{
    if (global_settings.talk_file == 1) /* number */
        talk_number(preset + 1, enqueue);
    else
    { /* spell */
        if(presets[preset].name[0])
            talk_spell(presets[preset].name, enqueue);
        else if(fallback)
             talk_freq(presets[preset].frequency, enqueue);
    }
}


void radio_save_presets(void)
{
    int fd;
    int i;

    fd = creat(filepreset, 0666);
    if(fd >= 0)
    {
        for(i = 0;i < num_presets;i++)
        {
            fdprintf(fd, "%d:%s\n", presets[i].frequency, presets[i].name);
        }
        close(fd);

        if(!strncasecmp(FMPRESET_PATH, filepreset, strlen(FMPRESET_PATH)))
            set_file(filepreset, global_settings.fmr_file, MAX_FILENAME);
        presets_changed = false;
    }
    else
    {
        splash(HZ, ID2P(LANG_FM_PRESET_SAVE_FAILED));
    }
}

void radio_load_presets(char *filename)
{
    int fd;
    int rc;
    char buf[128];
    char *freq;
    char *name;
    bool done = false;
    int f;

    memset(presets, 0, sizeof(presets));
    num_presets = 0;

    /* No Preset in configuration. */
    if(filename[0] == '\0')
    {
        filepreset[0] = '\0';
        return;
    }
    /* Temporary preset, loaded until player shuts down. */
    else if(filename[0] == '/') 
        strlcpy(filepreset, filename, sizeof(filepreset));
    /* Preset from default directory. */
    else
        snprintf(filepreset, sizeof(filepreset), "%s/%s.fmr",
            FMPRESET_PATH, filename);

    fd = open_utf8(filepreset, O_RDONLY);
    if(fd >= 0)
    {
        while(!done && num_presets < MAX_PRESETS)
        {
            rc = read_line(fd, buf, 128);
            if(rc > 0)
            {
                if(settings_parseline(buf, &freq, &name))
                {
                    f = atoi(freq);
                    if(f) /* For backwards compatibility */
                    {
                        struct fmstation * const fms = &presets[num_presets];
                        fms->frequency = f;
                        strlcpy(fms->name, name, MAX_FMPRESET_LEN+1);
                        num_presets++;
                    }
                }
            }
            else
                done = true;
        }
        close(fd);
    }
    else /* invalid file name? */
        filepreset[0] = '\0';

    presets_loaded = num_presets > 0;
    presets_changed = false;
}

const char* radio_get_preset_name(int preset)
{
    if (preset < num_presets)
        return presets[preset].name;
    return NULL;
}

int handle_radio_add_preset(void)
{
    char buf[MAX_FMPRESET_LEN + 1];

    if(num_presets < MAX_PRESETS)
    {
        buf[0] = '\0';

        if (!kbd_input(buf, MAX_FMPRESET_LEN + 1))
        {
            struct fmstation * const fms = &presets[num_presets];
            strcpy(fms->name, buf);
            fms->frequency = curr_freq;
            num_presets++;
            presets_changed = true;
            presets_loaded = num_presets > 0;
            return true;
        }
    }
    else
    {
        splash(HZ, ID2P(LANG_FM_NO_FREE_PRESETS));
    }
    return false;
}

/* needed to know which preset we are edit/delete-ing */
static int selected_preset = -1;
static int radio_edit_preset(void)
{
    char buf[MAX_FMPRESET_LEN + 1];

    if (num_presets > 0)
    {
        struct fmstation * const fms = &presets[selected_preset];

        strcpy(buf, fms->name);

        if (!kbd_input(buf, MAX_FMPRESET_LEN + 1))
        {
            strcpy(fms->name, buf);
            presets_changed = true;
        }
    }

    return 1;
}

static int radio_delete_preset(void)
{
    if (num_presets > 0)
    {
        struct fmstation * const fms = &presets[selected_preset];

        if (selected_preset >= --num_presets)
            selected_preset = num_presets - 1;

        memmove(fms, fms + 1, (uintptr_t)(fms + num_presets) -
                              (uintptr_t)fms);

        if (curr_preset >= num_presets)
            --curr_preset;
    }

     /* Don't ask to save when all presets are deleted. */
    presets_changed = num_presets > 0;

    if (!presets_changed)
    {
        /* The preset list will be cleared, switch to Scan Mode. */
        radio_mode = RADIO_SCAN_MODE;
        curr_preset = -1;
        presets_loaded = false;
    }

    return 1;
}

int preset_list_load(void)
{
    char selected[MAX_PATH];
    struct browse_context browse;
    snprintf(selected, sizeof(selected), "%s.%s", global_settings.fmr_file, "fmr");
    browse_context_init(&browse, SHOW_FMR, 0,
                        str(LANG_FM_PRESET_LOAD), NOICON,
                        FMPRESET_PATH, selected);
    return !rockbox_browse(&browse);
}

int preset_list_save(void)
{
    if(num_presets > 0)
    { 
        bool bad_file_name = true;

        if(!dir_exists(FMPRESET_PATH)) /* Check if there is preset folder */
            mkdir(FMPRESET_PATH);

        create_numbered_filename(filepreset, FMPRESET_PATH, "preset",
                                 ".fmr", 2 IF_CNFN_NUM_(, NULL));

        while(bad_file_name)
        {
            if(!kbd_input(filepreset, sizeof(filepreset)))
            {
                /* check the name: max MAX_FILENAME (20) chars */
                char* p2;
                char* p1;
                int len;
                p1 = strrchr(filepreset, '/');
                p2 = p1;
                while((p1) && (*p2) && (*p2 != '.'))
                    p2++;
                len = (int)(p2-p1) - 1;
                if((!p1) || (len > MAX_FILENAME) || (len == 0))
                {
                    /* no slash, too long or too short */
                    splash(HZ, ID2P(LANG_INVALID_FILENAME));
                }
                else
                {
                    /* add correct extension (easier to always write)
                       at this point, p2 points to 0 or the extension dot */
                    *p2 = '\0';
                    strcat(filepreset,".fmr");
                    bad_file_name = false;
                    radio_save_presets();
                }
            }
            else
            {
                /* user aborted */
                return false;
            }
        }
    }
    else
        splash(HZ, ID2P(LANG_FM_NO_PRESETS));

    return true;
}

int preset_list_clear(void)
{
    /* Clear all the preset entries */
    memset(presets, 0, sizeof (presets));

    num_presets = 0;
    presets_loaded = false;
    /* The preset list will be cleared switch to Scan Mode. */
    radio_mode = RADIO_SCAN_MODE;
    curr_preset = -1;
    presets_changed = false; /* Don't ask to save when clearing the list. */

    return true;
}

MENUITEM_FUNCTION(radio_edit_preset_item, MENU_FUNC_CHECK_RETVAL, 
                    ID2P(LANG_FM_EDIT_PRESET), 
                    radio_edit_preset, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(radio_delete_preset_item, MENU_FUNC_CHECK_RETVAL,
                    ID2P(LANG_FM_DELETE_PRESET), 
                    radio_delete_preset, NULL, NULL, Icon_NOICON);
static int radio_preset_callback(int action,
                                 const struct menu_item_ex *this_item)
{
    if (action == ACTION_STD_OK)
        action = ACTION_EXIT_AFTER_THIS_MENUITEM;
    return action;
    (void)this_item;
}
MAKE_MENU(handle_radio_preset_menu, ID2P(LANG_PRESET),
            radio_preset_callback, Icon_NOICON, &radio_edit_preset_item, 
            &radio_delete_preset_item);
/* present a list of preset stations */
static const char* presets_get_name(int selected_item, void *data,
                                    char *buffer, size_t buffer_len)
{
    (void)data;
    struct fmstation *p = &presets[selected_item];
    if(p->name[0])
        return p->name;
    int freq = p->frequency / 10000;
    int frac = freq % 100;
    freq /= 100;
    snprintf(buffer, buffer_len,
             str(LANG_FM_DEFAULT_PRESET_NAME), freq, frac);
    return buffer;
}

static int presets_speak_name(int selected_item, void * data)
{
    (void)data;
    preset_talk(selected_item, true, false);
    return 0;
}

int handle_radio_presets(void)
{
    struct gui_synclist lists;
    int result = 0;
    int action = ACTION_NONE;
#ifdef HAVE_BUTTONBAR
    struct gui_buttonbar buttonbar;
#endif

    if(presets_loaded == false)
        return result;

#ifdef HAVE_BUTTONBAR
    gui_buttonbar_init(&buttonbar);
    gui_buttonbar_set_display(&buttonbar, &(screens[SCREEN_MAIN]) );
    gui_buttonbar_set(&buttonbar, str(LANG_FM_BUTTONBAR_ADD),
                                 str(LANG_FM_BUTTONBAR_EXIT),
                                 str(LANG_FM_BUTTONBAR_ACTION));
    gui_buttonbar_draw(&buttonbar);
#endif
    gui_synclist_init(&lists, presets_get_name, NULL, false, 1, NULL);
    gui_synclist_set_title(&lists, str(LANG_PRESET), NOICON);
    gui_synclist_set_icon_callback(&lists, NULL);
    if(global_settings.talk_file)
        gui_synclist_set_voice_callback(&lists, presets_speak_name);
    gui_synclist_set_nb_items(&lists, num_presets);
    gui_synclist_select_item(&lists, curr_preset<0 ? 0 : curr_preset);
    gui_synclist_speak_item(&lists);

    while (result == 0)
    {
        gui_synclist_draw(&lists);
        list_do_action(CONTEXT_STD, TIMEOUT_BLOCK,
                       &lists, &action, LIST_WRAP_UNLESS_HELD);
        switch (action)
        {
            case ACTION_STD_MENU:
                if (handle_radio_add_preset())
                {
                    gui_synclist_set_nb_items(&lists, num_presets);
                    gui_synclist_select_item(&lists, num_presets - 1);
        }
                break;
            case ACTION_STD_CANCEL:
                result = 1;
                break;
            case ACTION_STD_OK:
                curr_preset = gui_synclist_get_sel_pos(&lists);
                curr_freq = presets[curr_preset].frequency;
                next_station(0);
                remember_frequency();
                result = 1;
                break;
            case ACTION_F3:
            case ACTION_STD_CONTEXT:
                selected_preset = gui_synclist_get_sel_pos(&lists);
                do_menu(&handle_radio_preset_menu, NULL, NULL, false);
                gui_synclist_set_nb_items(&lists, num_presets);
                gui_synclist_select_item(&lists, selected_preset);
                gui_synclist_speak_item(&lists);
                break;
            default:
                if(default_event_handler(action) == SYS_USB_CONNECTED)
                    result = 2;
        }
    }
    return result - 1;
}


int presets_scan(void *viewports)
{
    bool do_scan = true;
    int i;
    struct viewport *vp = (struct viewport *)viewports;

    FOR_NB_SCREENS(i)
        screens[i].set_viewport(vp?&vp[i]:NULL);
    if(num_presets > 0) /* Do that to avoid 2 questions. */
        do_scan = yesno_pop(ID2P(LANG_FM_CLEAR_PRESETS));

    if(do_scan)
    {
        const struct fm_region_data * const fmr =
            &fm_region_data[global_settings.fm_region];

        curr_freq = fmr->freq_min;
        num_presets = 0;
        memset(presets, 0, sizeof(presets));

        tuner_set(RADIO_MUTE, 1);

        while(curr_freq <= fmr->freq_max)
        {
            int freq, frac;
            if(num_presets >= MAX_PRESETS || action_userabort(TIMEOUT_NOBLOCK))
                break;

            freq = curr_freq / 10000;
            frac = freq % 100;
            freq /= 100;

            splashf(0, str(LANG_FM_SCANNING), freq, frac);

            if(tuner_set(RADIO_SCAN_FREQUENCY, curr_freq))
            {
                /* add preset */
                presets[num_presets].name[0] = '\0';
                presets[num_presets].frequency = curr_freq;
                num_presets++;
            }

            curr_freq += fmr->freq_step;
        }

        if (get_radio_status() == FMRADIO_PLAYING)
            tuner_set(RADIO_MUTE, 0);

        presets_changed = true;

        FOR_NB_SCREENS(i)
        {
            screens[i].clear_viewport();
            screens[i].update_viewport();
        }

        if(num_presets > 0)
        {
            curr_freq = presets[0].frequency;
            radio_mode = RADIO_PRESET_MODE;
            presets_loaded = true;
            next_station(0);
        }
        else
        {
            /* Wrap it to beginning or we'll be past end of band */
            presets_loaded = false;
            next_station(1);
        }
    }
    return true;
}


void presets_save(void)
{
    if(filepreset[0] == '\0')
        preset_list_save();
    else
        radio_save_presets();
}

#ifdef HAVE_LCD_BITMAP
static inline void draw_vertical_line_mark(struct screen * screen,
                                           int x, int y, int h)
{
    screen->set_drawmode(DRMODE_COMPLEMENT);
    screen->vline(x, y, y+h-1);
}

/* draw the preset markers for a track of length "tracklen",
   between (x,y) and (x+w,y) */
void presets_draw_markers(struct screen *screen,
                          int x, int y, int w, int h)
{
    int i,xi;
    const struct fm_region_data *region_data =
            &(fm_region_data[global_settings.fm_region]);
    int len = region_data->freq_max - region_data->freq_min;
    for (i=0; i < radio_preset_count(); i++)
    {
        int freq = radio_get_preset(i)->frequency;
        int diff = freq - region_data->freq_min;
        xi = x + (w * diff)/len;
        draw_vertical_line_mark(screen, xi, y, h);
    }
}
#endif
