/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Kevin Ferrare
 * Copyright (C) 2007 by Jonathan Gordon
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
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "option_select.h"
#include "sprintf.h"
#include "kernel.h"
#include "lang.h"
#include "talk.h"
#include "settings_list.h"
#include "sound.h"
#include "list.h"
#include "action.h"
#include "statusbar.h"
#include "misc.h"
#include "splash.h"
#include "menu.h"
#include "quickscreen.h"

#if defined (HAVE_SCROLLWHEEL)      || \
    (CONFIG_KEYPAD == PLAYER_PAD)
/* Define this if your target makes sense to have 
   smaller values at the top of the list increasing down the list */
#define ASCENDING_INT_SETTINGS
#endif

static int selection_to_val(const struct settings_list *setting, int selection);
int option_value_as_int(const struct settings_list *setting)
{
    int type = (setting->flags & F_T_MASK);
    int temp = 0;
    if (type == F_T_BOOL)
        temp = *(bool*)setting->setting?1:0;
    else if (type == F_T_UINT || type == F_T_INT)
        temp = *(int*)setting->setting;
    return temp;
}
static const char *unit_strings[] = 
{   
    [UNIT_INT] = "",    [UNIT_MS]  = "ms",
    [UNIT_SEC] = "s",   [UNIT_MIN] = "min", 
    [UNIT_HOUR]= "hr",  [UNIT_KHZ] = "kHz",
    [UNIT_DB]  = "dB",  [UNIT_PERCENT] = "%",
    [UNIT_MAH] = "mAh", [UNIT_PIXEL] = "px",
    [UNIT_PER_SEC] = "per sec",
    [UNIT_HERTZ] = "Hz",
    [UNIT_MB]  = "MB",  [UNIT_KBIT]  = "kb/s",
    [UNIT_PM_TICK] = "units/10ms",
};
/* these two vars are needed so arbitrary values can be added to the
   TABLE_SETTING settings if the F_ALLOW_ARBITRARY_VALS flag is set */
static int table_setting_oldval = 0, table_setting_array_position = 0;
char *option_get_valuestring(const struct settings_list *setting, 
                             char *buffer, int buf_len,
                             intptr_t temp_var)
{
    if ((setting->flags & F_BOOL_SETTING) == F_BOOL_SETTING)
    {
        bool val = (bool)temp_var;
        strncpy(buffer, str(val? setting->bool_setting->lang_yes :
                                 setting->bool_setting->lang_no), buf_len);
    }
#if 0 /* probably dont need this one */
    else if ((setting->flags & F_FILENAME) == F_FILENAME)
    {
        struct filename_setting *info = setting->filename_setting;
        snprintf(buffer, buf_len, "%s%s%s", info->prefix,
                 (char*)temp_var, info->suffix);
    }
#endif
    else if (((setting->flags & F_INT_SETTING) == F_INT_SETTING) ||
             ((setting->flags & F_TABLE_SETTING) == F_TABLE_SETTING))
    {
        const struct int_setting *int_info = setting->int_setting;
        const struct table_setting *tbl_info = setting->table_setting;
        const char *unit;
        void (*formatter)(char*, size_t, int, const char*);
        if ((setting->flags & F_INT_SETTING) == F_INT_SETTING)
        {
            formatter = int_info->formatter;
            unit = unit_strings[int_info->unit];
        }
        else
        {
            formatter = tbl_info->formatter;
            unit = unit_strings[tbl_info->unit];
        }
        if (formatter)
            formatter(buffer, buf_len, (int)temp_var, unit);
        else
            snprintf(buffer, buf_len, "%d %s", (int)temp_var, unit?unit:"");
    }
    else if ((setting->flags & F_T_SOUND) == F_T_SOUND)
    {
        char sign = ' ', *unit;
        unit = (char*)sound_unit(setting->sound_setting->setting);
        if (sound_numdecimals(setting->sound_setting->setting))
        {
            int integer, dec;
            int val = sound_val2phys(setting->sound_setting->setting,
                                 (int)temp_var);
            if(val < 0)
            {
                sign = '-';
                val = abs(val);
            }
            integer = val / 10; dec = val % 10;
            snprintf(buffer, buf_len, "%c%d.%d %s", sign, integer, dec, unit);
        }
        else
            snprintf(buffer, buf_len, "%d %s", (int)temp_var, unit);
    }
    else if ((setting->flags & F_CHOICE_SETTING) == F_CHOICE_SETTING)
    {
        if (setting->flags & F_CHOICETALKS)
        {
            int setting_id;
            const struct choice_setting *info = setting->choice_setting;
            if (info->talks[(int)temp_var] < LANG_LAST_INDEX_IN_ARRAY)
            {
                strncpy(buffer, str(info->talks[(int)temp_var]), buf_len);
            }
            else
            {
                find_setting(setting->setting, &setting_id);
                cfg_int_to_string(setting_id, (int)temp_var, buffer, buf_len);
            }
        }
        else
        {
            int value= (int)temp_var;
            char *val = P2STR(setting->choice_setting->desc[value]);
            strncpy(buffer, val, buf_len);
        }
    }
    return buffer;
}
void option_talk_value(const struct settings_list *setting, int value, bool enqueue)
{
    if ((setting->flags & F_BOOL_SETTING) == F_BOOL_SETTING)
    {
        bool val = value==1?true:false;
        talk_id(val? setting->bool_setting->lang_yes :
                setting->bool_setting->lang_no, enqueue);
    }
#if 0 /* probably dont need this one */
    else if ((setting->flags & F_FILENAME) == F_FILENAME)
    {
}
#endif
    else if (((setting->flags & F_INT_SETTING) == F_INT_SETTING) ||
               ((setting->flags & F_TABLE_SETTING) == F_TABLE_SETTING))
    {
        const struct int_setting *int_info = setting->int_setting;
        const struct table_setting *tbl_info = setting->table_setting;
        int unit;
        int32_t (*get_talk_id)(int, int);
        if ((setting->flags & F_INT_SETTING) == F_INT_SETTING)
        {
            unit = int_info->unit;
            get_talk_id = int_info->get_talk_id;
        }
        else
        {
            unit = tbl_info->unit;
            get_talk_id = tbl_info->get_talk_id;
        }
        if (get_talk_id)
            talk_id(get_talk_id(value, unit), enqueue);
        else
            talk_value(value, unit, enqueue);
    }
    else if ((setting->flags & F_T_SOUND) == F_T_SOUND)
    {
        int talkunit = UNIT_INT;
        const char *unit = sound_unit(setting->sound_setting->setting);
        if (!strcmp(unit, "dB"))
            talkunit = UNIT_DB;
        else if (!strcmp(unit, "%"))
            talkunit = UNIT_PERCENT;
        else if (!strcmp(unit, "Hz"))
            talkunit = UNIT_HERTZ;
        talk_value(value, talkunit, false);
    }
    else if ((setting->flags & F_CHOICE_SETTING) == F_CHOICE_SETTING)
    {
        if (setting->flags & F_CHOICETALKS)
        {
            talk_id(setting->choice_setting->talks[value], enqueue);
        }
        else
        {
            talk_id(P2ID(setting->choice_setting->desc[value]), enqueue);
        }
    }
}
    
static int option_talk(int selected_item, void * data)
{
    struct settings_list *setting = (struct settings_list *)data;
    int temp_var = selection_to_val(setting, selected_item);
    option_talk_value(setting, temp_var, false);
    return 0;
}

#if defined(HAVE_QUICKSCREEN) || defined(HAVE_RECORDING)
   /* only the quickscreen and recording trigger needs this */
void option_select_next_val(const struct settings_list *setting,
                            bool previous, bool apply)
{
    int val = 0;
    int *value = setting->setting;
    if ((setting->flags & F_BOOL_SETTING) == F_BOOL_SETTING)
    {
        *(bool*)value = !*(bool*)value;
        if (apply && setting->bool_setting->option_callback)
            setting->bool_setting->option_callback(*(bool*)value);
        return;
    }
    else if ((setting->flags & F_INT_SETTING) == F_INT_SETTING)
    {
        struct int_setting *info = (struct int_setting *)setting->int_setting;
        int step = info->step;
        if (step < 0)
            step = -step;
        if (!previous)
        {
            val = *value + step;
            if (val > info->max)
                val = info->min;
        }
        else
        {
            val = *value - step;
            if (val < info->min)
                val = info->max;
        }
        if (apply && info->option_callback)
            info->option_callback(val);
    }
    else if ((setting->flags & F_T_SOUND) == F_T_SOUND)
    {
        int setting_id = setting->sound_setting->setting;
        int steps = sound_steps(setting_id);
        int min = sound_min(setting_id);
        int max = sound_max(setting_id);
        if (!previous)
        {
            val = *value + steps;
            if (val >= max)
                val = min;
        }
        else
        {
            val = *value - steps;
            if (val < min)
                val = max;
        }
    }
    else if ((setting->flags & F_CHOICE_SETTING) == F_CHOICE_SETTING)
    {
        struct choice_setting *info = (struct choice_setting *)setting->choice_setting;
        val = *value + 1;
        if (!previous)
        {
            val = *value + 1;
            if (val >= info->count)
                val = 0;
        }
        else
        {
            val = *value - 1;
            if (val < 0)
                val = info->count-1;
        }
        if (apply && info->option_callback)
            info->option_callback(val);
    }
    else if ((setting->flags & F_TABLE_SETTING) == F_TABLE_SETTING)
    {
        const struct table_setting *tbl_info = setting->table_setting;
        int i, add;
        add = previous?tbl_info->count-1:1;
        for (i=0; i<tbl_info->count;i++)
        {
            if ((*value == tbl_info->values[i]) ||
                  (settings->flags&F_ALLOW_ARBITRARY_VALS &&
                    *value < tbl_info->values[i]))
            {
                val = tbl_info->values[(i+add)%tbl_info->count];
                break;
            }
        }
    }
    *value = val;
}
#endif

static int selection_to_val(const struct settings_list *setting, int selection)
{
    int min = 0, max = 0, step = 1;
    if (((setting->flags & F_BOOL_SETTING) == F_BOOL_SETTING) ||
          ((setting->flags & F_CHOICE_SETTING) == F_CHOICE_SETTING))
        return selection;
    else if ((setting->flags & F_TABLE_SETTING) == F_TABLE_SETTING)
    {
        const struct table_setting *info = setting->table_setting;
        if (setting->flags&F_ALLOW_ARBITRARY_VALS && 
            table_setting_array_position != -1    &&
            (selection >= table_setting_array_position))
        {
            if (selection == table_setting_array_position)
                return table_setting_oldval;
            return info->values[selection-1];
        }
        else
            return info->values[selection];
    }
    else if ((setting->flags & F_T_SOUND) == F_T_SOUND)
    {
        int setting_id = setting->sound_setting->setting;
#ifndef ASCENDING_INT_SETTINGS
        step = sound_steps(setting_id);
        max = sound_max(setting_id);
        min = sound_min(setting_id);
#else
        step = -sound_steps(setting_id);
        min = sound_max(setting_id);
        max = sound_min(setting_id);
#endif
    }
    else if ((setting->flags & F_INT_SETTING) == F_INT_SETTING)
    {
        const struct int_setting *info = setting->int_setting;
#ifndef ASCENDING_INT_SETTINGS
        min = info->min;
        max = info->max;
        step = info->step;
#else
        max = info->min;
        min = info->max;
        step = -info->step;
#endif
    }
    return max- (selection * step);
}
static char * value_setting_get_name_cb(int selected_item, 
                                        void * data,
                                        char *buffer,
                                        size_t buffer_len)
{
    selected_item = selection_to_val(data, selected_item);
    return option_get_valuestring(data, buffer, buffer_len, selected_item);
}

/* wrapper to convert from int param to bool param in option_screen */
static void (*boolfunction)(bool);
static void bool_funcwrapper(int value)
{
    if (value)
        boolfunction(true);
    else
        boolfunction(false);
}

static void val_to_selection(const struct settings_list *setting, int oldvalue,
                             int *nb_items, int *selected,
                             void (**function)(int))
{
    int var_type = setting->flags&F_T_MASK;
    /* set the number of items and current selection */
    if (var_type == F_T_INT || var_type == F_T_UINT)
    {
        if (setting->flags&F_CHOICE_SETTING)
        {
            *nb_items = setting->choice_setting->count;
            *selected = oldvalue;
            *function = setting->choice_setting->option_callback;
        }
        else if (setting->flags&F_TABLE_SETTING)
        {
            const struct table_setting *info = setting->table_setting;
            int i;
            *nb_items = info->count;
            *selected = -1;
            table_setting_array_position = -1;
            for (i=0;*selected==-1 && i<*nb_items;i++)
            {
                if (setting->flags&F_ALLOW_ARBITRARY_VALS &&
                    (oldvalue < info->values[i]))
                {
                    table_setting_oldval = oldvalue;
                    table_setting_array_position = i;
                    *selected = i;
                    (*nb_items)++;
                }
                else if (oldvalue == info->values[i])
                    *selected = i;
            }
            *function = info->option_callback;
        }
        else if (setting->flags&F_T_SOUND)
        {
            int setting_id = setting->sound_setting->setting;
            int steps = sound_steps(setting_id);
            int min = sound_min(setting_id);
            int max = sound_max(setting_id);
            *nb_items = (max-min)/steps + 1;
#ifndef ASCENDING_INT_SETTINGS
            *selected = (max - oldvalue) / steps;
#else
            *selected = (oldvalue - min) / steps;
#endif
            *function = sound_get_fn(setting_id);
        }
        else
        {
            const struct int_setting *info = setting->int_setting;
            int min, max, step;
            max = info->max;
            min = info->min;
            step = info->step;
            *nb_items = (max-min)/step + 1;
#ifndef ASCENDING_INT_SETTINGS
            *selected = (max - oldvalue) / step;
#else
            *selected = (oldvalue - min) / step;
#endif
            *function = info->option_callback;
        }
    }
    else if (var_type == F_T_BOOL)
    {
        *selected = oldvalue;
        *nb_items = 2;
        boolfunction = setting->bool_setting->option_callback;
        if (boolfunction)
            *function = bool_funcwrapper;
    }
}

bool option_screen(const struct settings_list *setting,
                   struct viewport parent[NB_SCREENS],
                   bool use_temp_var, unsigned char* option_title)
{
    int action;
    bool done = false;
    struct gui_synclist lists;
    int oldvalue, nb_items = 0, selected = 0, temp_var;
    int *variable;
    bool allow_wrap = setting->flags & F_NO_WRAP ? false : true;
    int var_type = setting->flags&F_T_MASK;
    void (*function)(int) = NULL;
    char *title;
    if (var_type == F_T_INT || var_type == F_T_UINT)
    {
        variable = use_temp_var ? &temp_var: (int*)setting->setting;
        temp_var = oldvalue = *(int*)setting->setting;
    }
    else if (var_type == F_T_BOOL)
    {
        /* bools always use the temp variable...
        if use_temp_var is false it will be copied to setting->setting every change */
        variable = &temp_var;
        temp_var = oldvalue = *(bool*)setting->setting?1:0;
    }
    else return false; /* only int/bools can go here */
    gui_synclist_init(&lists, value_setting_get_name_cb,
                      (void*)setting, false, 1, parent);
    if (setting->lang_id == -1)
        title = (char*)setting->cfg_vals;
    else
        title = P2STR(option_title);
    
    gui_synclist_set_title(&lists, title, Icon_Questionmark);
    gui_synclist_set_icon_callback(&lists, NULL);
    if(global_settings.talk_menu)
        gui_synclist_set_voice_callback(&lists, option_talk);
    
    val_to_selection(setting, oldvalue, &nb_items, &selected, &function);
    gui_synclist_set_nb_items(&lists, nb_items);
    gui_synclist_select_item(&lists, selected);
    
    gui_synclist_limit_scroll(&lists, true);
    gui_synclist_draw(&lists);
    /* talk the item */
    gui_synclist_speak_item(&lists);
    while (!done)
    {
        if (list_do_action(CONTEXT_LIST, TIMEOUT_BLOCK,
                           &lists, &action,
            allow_wrap? LIST_WRAP_UNLESS_HELD: LIST_WRAP_OFF))
        {
            /* setting changed */
            selected = gui_synclist_get_sel_pos(&lists);
            *variable = selection_to_val(setting, selected);
            if (var_type == F_T_BOOL && !use_temp_var)
                *(bool*)setting->setting = (*variable==1);
        }
        else if (action == ACTION_NONE)
            continue;
        else if (action == ACTION_STD_CANCEL)
        {
            /* setting canceled, restore old value if changed */
            if (*variable != oldvalue)
            {
                *variable = oldvalue;
                if (var_type == F_T_BOOL && !use_temp_var)
                    *(bool*)setting->setting = (oldvalue==1);
                splash(HZ/2, ID2P(LANG_CANCEL));
            }
            done = true;
        }
        else if (action == ACTION_STD_CONTEXT)
        {
            /* reset setting to default */
            reset_setting(setting, variable);
            if (var_type == F_T_BOOL && !use_temp_var)
                *(bool*)setting->setting = (*variable==1);
            val_to_selection(setting, *variable, &nb_items,
                                &selected, &function);
            gui_synclist_select_item(&lists, selected);
            gui_synclist_draw(&lists);
            gui_synclist_speak_item(&lists);
        }
        else if (action == ACTION_STD_OK)
        {
            /* setting accepted, store now if it used a temp var */
            if (use_temp_var)
            {
                if (var_type == F_T_INT || var_type == F_T_UINT)
                    *(int*)setting->setting = *variable;
                else 
                    *(bool*)setting->setting = (*variable==1);
            }
            settings_save();
            done = true;
        }
        else if(default_event_handler(action) == SYS_USB_CONNECTED)
            return true;
        /* callback */
        if ( function )
            function(*variable);
    }
    return false;
}

