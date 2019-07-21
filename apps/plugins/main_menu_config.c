/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 Jonathan Gordon
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

/* mandatory include for all plugins */
#include "plugin.h"

static struct menu_table *menu_table;
static int menu_item_count;

#define MAX_ITEM_NAME 64
#define MAX_ITEMS 16
struct items
{
    unsigned char *name;
    char string[MAX_ITEM_NAME];
    bool enabled;
};

static struct items menu_items[MAX_ITEMS];


static const char * menu_get_name(int selected_item, void * data,
                                   char * buffer, size_t buffer_len)
{
    (void)data;
    (void)buffer;
    (void)buffer_len;
    unsigned char *p = menu_items[selected_item].name;
    int id = P2ID(p);
    return (id != -1) ? rb->str(id) : p;
}

static enum themable_icons menu_get_icon(int selected_item, void * data)
{
    (void)data;

    return menu_items[selected_item].enabled ? Icon_Config : Icon_NOICON;
}

static unsigned char *item_name(int n)
{
    const struct menu_item_ex *item = menu_table[n].item;
    return (item->flags & MENU_HAS_DESC) ?
      item->callback_and_desc->desc :
      (rb->strcmp("wps", menu_table[n].string) ?
       (unsigned char *)menu_table[n].string :
       ID2P(LANG_RESUME_PLAYBACK));
}

void load_from_cfg(void)
{
    char config_str[128];
    char *token, *save;
    int done = 0;
    int i = 0;
    bool found = false;

    config_str[0] = '\0';
    rb->root_menu_write_to_cfg(NULL, config_str, sizeof(config_str));

    token = rb->strtok_r(config_str, ", ", &save);

    while (token)
    {
        for (i = 0, found = false; i < menu_item_count; i++)
        {
            found = rb->strcmp(token, menu_table[i].string) == 0;
            if (found) break;
        }
        if (found)
        {
            menu_items[done].name = item_name(i);
            rb->strcpy(menu_items[done].string, token);
            menu_items[done].enabled = true;
            done++;
        }
        token = rb->strtok_r(NULL, ", ", &save);
    }

    if (done < menu_item_count)
    {
        for (i = 0; i < menu_item_count; i++)
        {
            found = false;
            for (int j = 0; !found && j < done; j++)
            {
                found = rb->strcmp(menu_table[i].string, menu_items[j].string) == 0;
            }

            if (!found)
            {
                menu_items[done].name = item_name(i);
                rb->strcpy(menu_items[done].string, menu_table[i].string);
                menu_items[done].enabled = false;
                done++;
            }
        }
    }
}

static void save_to_cfg(void)
{
    char out[128];
    int i, j = 0;

    out[0] = '\0';
    for (i = 0; i < menu_item_count; i++)
    {
        if (menu_items[i].enabled)
        {
            j += rb->snprintf(&out[j],sizeof(out) - j, "%s, ", menu_items[i].string);
        }
    }

    rb->root_menu_load_from_cfg(&rb->global_settings->root_menu_customized, out);
}

static void swap_items(int a, int b)
{
    unsigned char *name;
    char temp[MAX_ITEM_NAME];
    bool enabled;

    name = menu_items[a].name;
    rb->strcpy(temp, menu_items[a].string);
    enabled = menu_items[a].enabled;
    menu_items[a].name = menu_items[b].name;
    rb->strcpy(menu_items[a].string,
            menu_items[b].string);
    menu_items[a].enabled = menu_items[b].enabled;
    menu_items[b].name = name;
    rb->strcpy(menu_items[b].string, temp);
    menu_items[b].enabled = enabled;
}

static int menu_speak_item(int selected_item, void *data)
{
    (void) data;
    int id = P2ID(menu_items[selected_item].name);

    if (id != -1)
    {
        rb->talk_id(id, false);
        rb->talk_id(menu_items[selected_item].enabled ? LANG_ON : LANG_OFF, true);
    }

    return 0;
}

/* this is the plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;
    struct gui_synclist list;
    bool done = false;
    int action, cur_sel;
    
    menu_table = rb->root_menu_get_options(&menu_item_count);
    load_from_cfg();

    rb->gui_synclist_init(&list, menu_get_name, NULL, false, 1, NULL);
    if (rb->global_settings->talk_menu)
        rb->gui_synclist_set_voice_callback(&list, menu_speak_item);
    rb->gui_synclist_set_icon_callback(&list, menu_get_icon);
    rb->gui_synclist_set_nb_items(&list, menu_item_count);
    rb->gui_synclist_set_title(&list, rb->str(LANG_MAIN_MENU_ORDER), Icon_Rockbox);
    rb->gui_synclist_draw(&list);
    rb->gui_synclist_speak_item(&list);

    while (!done)
    {
        cur_sel = rb->gui_synclist_get_sel_pos(&list);
        action = rb->get_action(CONTEXT_LIST,TIMEOUT_BLOCK);
        if (rb->gui_synclist_do_button(&list,&action,LIST_WRAP_UNLESS_HELD))
            continue;

        switch (action)
        {
            case ACTION_STD_OK:
            {
                MENUITEM_STRINGLIST(menu, "Main Menu Editor", NULL,
                                    ID2P(LANG_TOGGLE_ITEM),
                                    ID2P(LANG_MOVE_ITEM_UP),
                                    ID2P(LANG_MOVE_ITEM_DOWN),
                                    "----------",
                                    ID2P(LANG_LOAD_DEFAULT_CONFIGURATION),
                                    ID2P(LANG_SAVE_EXIT));
                switch (rb->do_menu(&menu, NULL, NULL, false))
                {
                    case 0:
                        menu_items[cur_sel].enabled = !menu_items[cur_sel].enabled;
                        break;
                    case 1:
                        if (cur_sel == 0)
                            break;
                        swap_items(cur_sel, cur_sel - 1);
                        break;
                    case 2:
                        if (cur_sel + 1 == menu_item_count)
                            break;
                        swap_items(cur_sel, cur_sel + 1);
                        break;
                    case 4:
                        rb->root_menu_set_default(&rb->global_settings->root_menu_customized, NULL);
                        load_from_cfg();
                        break;
                    case 5:
                        done = true;
                        save_to_cfg();
                        rb->global_settings->root_menu_customized = true;
                        rb->settings_save();
                        break;
                }
                if (!done)
                {
                    rb->gui_synclist_draw(&list);
                    rb->gui_synclist_speak_item(&list);
                }
                break;
            }
            case ACTION_STD_CANCEL:
                done = true;
                break;
        }
    }


    return PLUGIN_OK;
}
