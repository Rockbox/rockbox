/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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
#include "rbpaths.h"
#include "general.h"
#include "radio.h"
#include "tuner.h"
#include "file.h"
#include "string-extra.h"
#include "misc.h"
#include "pathfuncs.h"
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
#include "core_alloc.h"

static int curr_preset = -1;

int snap_freq_to_grid(int freq);
void remember_frequency(void);

#define FREQ_DISP_DIVISOR (10000)
#define MAX_PRESETS 64
static bool presets_loaded = false;
static bool presets_changed = false;
static int preset_handle = 0;
static int num_presets = 0; /* The number of presets in the preset list */

struct fmstation
{
    int frequency; /* In Hz */
    char name[MAX_FMPRESET_LEN+1];
};
struct fmstation_buf /* Allocd on first use of presets */
{
    struct fmstation fmstation[MAX_PRESETS];
    char filepreset[MAX_PATH];
};


static struct fmstation_buf *presets_get(void)
{
    if (preset_handle <= 0)
    {
        preset_handle = core_alloc(sizeof(struct fmstation_buf));
    }
    if (preset_handle > 0)
    {
        return core_get_data_pinned(preset_handle);
    }
    return NULL;
}

static void presets_put(struct fmstation_buf *presets)
{
    if (presets != NULL)
    {
        core_put_data_pinned(presets);
    }
}

int radio_current_preset(void)
{
    return curr_preset;
}
int radio_preset_count(void)
{
    return num_presets;
}

bool presets_have_changed(void)
{
    return presets_changed;
}


/* Find a matching preset to freq */
int preset_find(int freq)
{
    int ret = -1;
    int i;
    if(num_presets < 1)
        return -1;
    struct fmstation_buf *presets = presets_get();
    if (presets != NULL)
    {
        for(i = 0;i < MAX_PRESETS;i++)
        {
            if(freq == presets->fmstation[i].frequency)
            {
                ret = i;
                break;
            }
        }
        presets_put(presets);
    }
    return ret;
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

    struct fmstation_buf *presets = presets_get();
    if (presets != NULL)
    {
        for (i = 0; i < num_presets; i++)
        {
            int f = presets->fmstation[i].frequency;
            if (f == freq)
            {
                closest = i; /* Exact match = stop */
                break;
            }
            /* remember the highest and lowest presets for wraparound */
            if (f < presets->fmstation[lowpreset].frequency)
                lowpreset = i;
            if (f > presets->fmstation[highpreset].frequency)
                highpreset = i;

            /* find the closest preset in the given direction */
            if (direction > 0 && f > freq)
            {
                if (closest < 0 || f < presets->fmstation[closest].frequency)
                    closest = i;
            }
            else if (direction < 0 && f < freq)
            {
                if (closest < 0 || f > presets->fmstation[closest].frequency)
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
        presets_put(presets);
    }
    return closest;
}

void preset_next(int direction)
{
    if (num_presets < 1)
        return;

    struct fmstation_buf *presets = presets_get();
    if (presets != NULL)
    {
        int curr_freq = radio_get_current_frequency();

        if (curr_preset == -1)
            curr_preset = find_closest_preset(curr_freq, direction);
        else
            curr_preset = (curr_preset + direction + num_presets) % num_presets;

        /* Must stay on the current grid for the region */
        curr_freq = snap_freq_to_grid(presets->fmstation[curr_preset].frequency);
        radio_set_current_frequency(curr_freq);
        tuner_set(RADIO_FREQUENCY, curr_freq);
        remember_frequency();
        presets_put(presets);
    }
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
        struct fmstation_buf *presets = presets_get();
        if (presets != NULL)
        {
            if(presets->fmstation[preset].name[0])
                talk_spell(presets->fmstation[preset].name, enqueue);
            else if(fallback)
                talk_value_decimal(presets->fmstation[preset].frequency, UNIT_INT, 6, enqueue);
            presets_put(presets);
        }
    }
}

void radio_save_presets(void)
{
    int fd;
    int i;

    struct fmstation_buf *presets = presets_get();
    if (presets != NULL)
    {
        char *filepreset = presets->filepreset;
        fd = creat(filepreset, 0666);
        if(fd >= 0)
        {
            for(i = 0;i < num_presets;i++)
            {
                fdprintf(fd, "%d:%s\n", presets->fmstation[i].frequency, presets->fmstation[i].name);
            }
            close(fd);

            if (strcasestr(filepreset, FMPRESET_PATH))
                set_file(filepreset, global_settings.fmr_file);
            presets_changed = false;
        }
        else
        {
            splash(HZ, ID2P(LANG_FM_PRESET_SAVE_FAILED));
        }
        presets_put(presets);
    }
}

void radio_load_presets(const char *filename)
{
    int fd;
    int rc;
    char buf[128];
    char *freq;
    char *name;
    bool done = false;
    int f;

    num_presets = 0;
    struct fmstation_buf *presets = presets_get();
    if (presets == NULL)
        return;

    memset(presets->fmstation, 0, sizeof(presets->fmstation));

    char *filepreset = presets->filepreset;

    /* No Preset in configuration. */
    if(filename[0] == '\0' || filename[0] == '-')
    {
        filepreset[0] = '\0';
        presets_put(presets);
        return;
    }

    splash(0, ID2P(LANG_WAIT));

    if(filename[0] != '/') /* Preset within radio screen */
    {
        snprintf(filepreset, MAX_PATH, "%s/%s.fmr",
                                         FMPRESET_PATH, filename);;
    }
    else
    {
        strmemccpy(filepreset, filename, MAX_PATH);
    }

    /* Preset inside the default folder? */
    if (strcasestr(filepreset, FMPRESET_PATH))
        set_file(filepreset, global_settings.fmr_file);
    /* else Temporary preset, loaded until player shuts down. */

    fd = open_utf8(filepreset, O_RDONLY);
    if(fd >= 0)
    {
        while(!done && num_presets < MAX_PRESETS)
        {
            rc = read_line(fd, buf, sizeof(buf));
            if(rc > 0)
            {
                if(settings_parseline(buf, &freq, &name))
                {
                    f = atoi(freq);
                    if(f) /* For backwards compatibility */
                    {
                        struct fmstation * const fms = &presets->fmstation[num_presets];
                        fms->frequency = f;
                        strmemccpy(fms->name, name, MAX_FMPRESET_LEN+1);
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
    presets_put(presets);
}

int radio_get_preset_freq(int preset)
{
    int freq = -1;
    struct fmstation_buf *presets = presets_get();
    if (presets != NULL)
    {
        if (preset < num_presets)
            freq = presets->fmstation[preset].frequency;

        presets_put(presets);
    }
    return freq;
}

int handle_radio_add_preset(void)
{
    int ret = 0;
    char buf[MAX_FMPRESET_LEN + 1];

    if(preset_handle > 0 && num_presets < MAX_PRESETS)
    {
        struct fmstation_buf *presets = presets_get();
        if (presets != NULL)
        {
            buf[0] = '\0';
            if (!kbd_input(buf, sizeof(buf), NULL))
            {
                struct fmstation * const fms = &presets->fmstation[num_presets];
                strcpy(fms->name, buf);
                fms->frequency = radio_get_current_frequency();
                num_presets++;
                presets_changed = true;
                presets_loaded = num_presets > 0;
                ret = 1;
            }
            presets_put(presets);
        }
    }
    else
    {
        splash(HZ, ID2P(LANG_FM_NO_FREE_PRESETS));
    }

    return ret;
}

/* needed to know which preset we are edit/delete-ing */
static int selected_preset = -1;
static int radio_edit_preset(void)
{
    char buf[MAX_FMPRESET_LEN + 1];

    if (num_presets > 0)
    {
        struct fmstation_buf *presets = presets_get();
        if (presets != NULL)
        {
            struct fmstation * const fms = &presets->fmstation[selected_preset];

            strcpy(buf, fms->name);

            if (!kbd_input(buf, MAX_FMPRESET_LEN + 1, NULL))
            {
                strcpy(fms->name, buf);
                presets_changed = true;
            }
            presets_put(presets);
        }
    }

    return 1;
}

static int radio_delete_preset(void)
{
    if (num_presets > 0)
    {
        struct fmstation_buf *presets = presets_get();
        if (presets != NULL)
        {
            struct fmstation * const fms = &presets->fmstation[selected_preset];

            if (selected_preset >= --num_presets)
                selected_preset = num_presets - 1;

            memmove(fms, fms + 1, (uintptr_t)(fms + num_presets) -
                                  (uintptr_t)fms);

            if (curr_preset >= num_presets)
                --curr_preset;
            presets_put(presets);
        }
    }

     /* Don't ask to save when all presets are deleted. */
    presets_changed = num_presets > 0;

    if (!presets_changed)
    {
        /* The preset list will be cleared, switch to Scan Mode. */
        radio_set_mode(RADIO_SCAN_MODE);
        curr_preset = -1;
        presets_loaded = false;
    }

    return 1;
}

int preset_list_load(void)
{
    char selected[MAX_PATH];
    snprintf(selected, sizeof(selected), "%s.%s", global_settings.fmr_file, "fmr");

    struct browse_context browse = {
        .dirfilter = SHOW_FMR,
        .title = str(LANG_FM_PRESET_LOAD),
        .icon = Icon_NOICON,
        .root = FMPRESET_PATH,
        .selected = selected,
    };

    return !rockbox_browse(&browse);
}

int preset_list_save(void)
{
    if(num_presets > 0)
    {
        bool bad_file_name = true;

        if(!dir_exists(FMPRESET_PATH)) /* Check if there is preset folder */
            mkdir(FMPRESET_PATH);

        struct fmstation_buf *p = presets_get();
        char *filepreset = p->filepreset;

        create_numbered_filename(filepreset, FMPRESET_PATH, "preset",
                                 ".fmr", 2 IF_CNFN_NUM_(, NULL));

        while(bad_file_name)
        {
            if(!kbd_input(filepreset, MAX_PATH, NULL))
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
                break;
            }
        }
        presets_put(p);
    }
    else
        splash(HZ, ID2P(LANG_FM_NO_PRESETS));

    return 0;
}

int preset_list_clear(void)
{
    if (preset_handle <= 0)
        return 0;
    struct fmstation_buf *presets = presets_get();
    if (presets != NULL)
    {
        /* Clear all the preset entries */
        memset(presets->fmstation, 0, sizeof(presets->fmstation));

        num_presets = 0;
        presets_loaded = false;
        /* The preset list will be cleared switch to Scan Mode. */
        radio_set_mode(RADIO_SCAN_MODE);
        curr_preset = -1;
        presets_changed = false; /* Don't ask to save when clearing the list. */
        global_settings.fmr_file[0] = '-';
        presets_put(presets);
    }
    return 0;
}

MENUITEM_FUNCTION(radio_edit_preset_item, MENU_FUNC_CHECK_RETVAL,
                  ID2P(LANG_FM_EDIT_PRESET),
                  radio_edit_preset, NULL, Icon_NOICON);
MENUITEM_FUNCTION(radio_delete_preset_item, MENU_FUNC_CHECK_RETVAL,
                  ID2P(LANG_FM_DELETE_PRESET),
                  radio_delete_preset, NULL, Icon_NOICON);
static int radio_preset_callback(int action,
                                 const struct menu_item_ex *this_item,
                                 struct gui_synclist *this_list)
{
    if (action == ACTION_STD_OK)
        action = ACTION_EXIT_AFTER_THIS_MENUITEM;
    return action;
    (void)this_item;
    (void)this_list;
}
MAKE_MENU(handle_radio_preset_menu, ID2P(LANG_PRESET),
            radio_preset_callback, Icon_NOICON, &radio_edit_preset_item,
            &radio_delete_preset_item);
/* present a list of preset stations */
static const char* presets_get_name(int selected_item, void *data,
                                    char *buffer, size_t buffer_len)
{
    struct fmstation * stations = (struct fmstation *) data;

    struct fmstation *p = &stations[selected_item];
    if(p->name[0])
    {
        snprintf(buffer, buffer_len, "%s", p->name);
    }
    else
    {
        int freq = p->frequency / FREQ_DISP_DIVISOR;
        int frac = freq % 100;
        freq /= 100;
        snprintf(buffer, buffer_len,
                 str(LANG_FM_DEFAULT_PRESET_NAME), freq, frac);
    }
    return buffer;
}

static int presets_speak_name(int selected_item, void * data)
{
    (void)data;
    preset_talk(selected_item, true, false);
    return 0;
}

const char* radio_get_preset_name(int preset, char *buffer, size_t buffer_len)
{
    buffer[0] = '\0'; /* skin_tokens.c get_radio_token() doesn't check for success */
    struct fmstation_buf *presets = presets_get();
    if (presets != NULL)
    {
        presets_get_name(preset, presets->fmstation, buffer, buffer_len);
        presets_put(presets);
        return buffer;
    }
    return NULL;
}

int handle_radio_presets(void)
{
    struct gui_synclist lists;
    int result = 0;
    int action = ACTION_NONE;

    if(presets_loaded == false)
        return result;

    struct fmstation_buf *presets = presets_get();
    if (presets == NULL)
        return result;

    gui_synclist_init(&lists, presets_get_name, presets->fmstation, false, 1, NULL);
    gui_synclist_set_title(&lists, str(LANG_PRESET), NOICON);
    if(global_settings.talk_file)
        gui_synclist_set_voice_callback(&lists, presets_speak_name);
    gui_synclist_set_nb_items(&lists, num_presets);
    gui_synclist_select_item(&lists, curr_preset<0 ? 0 : curr_preset);
    gui_synclist_speak_item(&lists);

    while (result == 0)
    {
        gui_synclist_draw(&lists);
        list_do_action(CONTEXT_STD, TIMEOUT_BLOCK, &lists, &action);
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
                radio_set_current_frequency(presets->fmstation[curr_preset].frequency);
                next_station(0);
                result = 1;
                break;
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
    presets_put(presets);
    return result - 1;
}


int presets_scan(void *viewports)
{
    bool do_scan = true;
    int curr_freq = radio_get_current_frequency();
    long talked_tick = 0;
    struct viewport *vp = (struct viewport *)viewports;

    FOR_NB_SCREENS(i)
        screens[i].set_viewport(vp?&vp[i]:NULL);
    if(num_presets > 0) /* Do that to avoid 2 questions. */
        do_scan = yesno_pop(ID2P(LANG_FM_CLEAR_PRESETS));

    if(do_scan)
    {
        struct fmstation_buf *presets = presets_get();
        if (presets == NULL)
            return 1;
        const struct fm_region_data * const fmr =
            &fm_region_data[global_settings.fm_region];

        curr_freq = fmr->freq_min;
        num_presets = 0;
        memset(presets->fmstation, 0, sizeof(presets->fmstation));

        tuner_set(RADIO_MUTE, 1);

        while(curr_freq <= fmr->freq_max)
        {
            int freq, frac;
            if(num_presets >= MAX_PRESETS || action_userabort(TIMEOUT_NOBLOCK))
                break;

            freq = curr_freq / FREQ_DISP_DIVISOR;
            frac = freq % 100;
            freq /= 100;

            if (global_settings.talk_menu &&
                TIME_AFTER(current_tick, talked_tick + (HZ * 5)))
            {
                talked_tick = current_tick;
                talk_id(LANG_FM_SCANNING, false);
                talk_value_decimal(curr_freq, UNIT_INT, 6, true);
            }
            /* (voiced above) */
            splashf(0, str(LANG_FM_SCANNING), freq, frac);

            if(tuner_set(RADIO_SCAN_FREQUENCY, curr_freq))
            {
                /* add preset */
                presets->fmstation[num_presets].name[0] = '\0';
                presets->fmstation[num_presets].frequency = curr_freq;
                num_presets++;
            }

            curr_freq += fmr->freq_step;

            yield(); /* (voice_thread) */
        }
        radio_set_current_frequency(curr_freq);

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
            radio_set_current_frequency(presets->fmstation[0].frequency);
            radio_set_mode(RADIO_PRESET_MODE);
            presets_loaded = true;
            next_station(0);
        }
        else
        {
            /* Wrap it to beginning or we'll be past end of band */
            presets_loaded = false;
            next_station(1);
        }
        presets_put(presets);
    }

    return 1;
}


void presets_save(void)
{
    struct fmstation_buf *p = presets_get();
    if (p != NULL)
    {
        if(p->filepreset[0] == '\0')
            preset_list_save();
        else
            radio_save_presets();
        presets_put(p);
    }
}

#if 0 /* disabled in draw_progressbar() */
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
