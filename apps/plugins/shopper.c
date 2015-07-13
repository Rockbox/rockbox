/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Daniel Rigby
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
#include "plugin.h"
#include "lib/playback_control.h"



#define MAX_LIST_SIZE 400
#define DESC_SIZE 40
#define MAX_LINE_LEN (DESC_SIZE + 1)

enum flag_type {
    FL_CLEARED = 0,
    FL_SET,
    FL_CATEGORY
};

enum view_type {
    EDIT_SHOPPING_LIST = 0,
    VIEW_SHOPPING_LIST
};

#define VIEW_TYPE_SIZE VIEW_SHOPPING_LIST + 1

struct items_list_s {
    unsigned int id;
    enum flag_type flag;
    char desc[DESC_SIZE];
};

static struct items_list_s items_list[MAX_LIST_SIZE];
static int total_item_count = 0;
static int view_id_list[MAX_LIST_SIZE];
static int view_item_count;
static enum view_type view = EDIT_SHOPPING_LIST;
static char filename[MAX_PATH];
static bool changed = false;
static bool show_categories = true;
static char category_string[] = "Hide categories";

static const char *list_get_name_cb(int selected_item, void* data,
                                    char* buf, size_t buf_len)
{
    (void)data;
    rb->strlcpy(buf, items_list[view_id_list[selected_item]].desc, buf_len);
    return buf;
}

static enum themable_icons list_get_icon_cb(int selected_item, void *data)
{
    (void)data;
    if (items_list[view_id_list[selected_item]].flag == FL_CATEGORY)
        return Icon_Rockbox;
    else if (items_list[view_id_list[selected_item]].flag == FL_SET)
        return Icon_Cursor;
    else
        return Icon_NOICON;
}

static bool save_changes(void)
{
    int fd;
    int i;

    fd = rb->open(filename,O_WRONLY|O_CREAT|O_TRUNC);
    if (fd < 0)
    {
        rb->splash(HZ*2, "Changes NOT saved");
        return false;
    }

    rb->lcd_clear_display();
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(1);
#endif
    for (i = 0;i < total_item_count; i++)
    {
        switch (items_list[i].flag)
        {
            case FL_CATEGORY:
            {
                rb->fdprintf(fd,"#%s\n",items_list[i].desc);
                break;
            }
            case FL_SET:
            {
                rb->fdprintf(fd,"!%s\n",items_list[i].desc);
                break;
            }
            case FL_CLEARED:
            {
                rb->fdprintf(fd," %s\n",items_list[i].desc);
                break;
            }
        }
    }
    /* save current view */
    rb->fdprintf(fd,"$%d%d\n",view, show_categories);

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(0);
#endif
    rb->close(fd);

    return true;
}

static void create_view(struct gui_synclist *lists)
{
    unsigned int cnt = 0;
    int i, j;

    switch (view)
    {
        case EDIT_SHOPPING_LIST:
        {
            for (i = 0; i < total_item_count; i++)
            {
                if (show_categories || (items_list[i].flag != FL_CATEGORY))
                    view_id_list[cnt++] = i;
            }
            view_item_count = cnt;
            rb->gui_synclist_set_title(lists,"Select items",Icon_Playlist);
            break;
        }
        case VIEW_SHOPPING_LIST:
        {
            for (i = 0; i < total_item_count; i++)
            {
                if ((items_list[i].flag == FL_CATEGORY) && show_categories)
                {
                    for (j = i+1; j < total_item_count; j++)
                    {
                        if (items_list[j].flag == FL_SET)
                        {
                            view_id_list[cnt++] = i;
                            break;
                        }
                        if (items_list[j].flag == FL_CATEGORY)
                            break;
                    }
                }
                else if (items_list[i].flag == FL_SET)
                    view_id_list[cnt++] = i;
            }
            view_item_count = cnt;
            rb->gui_synclist_set_title(lists,"Shopping list",Icon_Playlist);
            break;
        }
    }
}

static bool toggle(int selected_item)
{
    if (items_list[view_id_list[selected_item]].flag == FL_CATEGORY)
        return false;
    else if (items_list[view_id_list[selected_item]].flag == FL_SET)
        items_list[view_id_list[selected_item]].flag = FL_CLEARED;
    else
        items_list[view_id_list[selected_item]].flag = FL_SET;
    return true;
}

static void update_category_string(void)
{
    if (show_categories)
        rb->strcpy(category_string,"Hide categories");
    else
        rb->strcpy(category_string,"Show categories");
}

static enum plugin_status load_file(void)
{
    int fd;
    static char temp_line[DESC_SIZE];
    static struct items_list_s new_item;
    static int count = 0;
    int linelen;
    total_item_count = 0;

    fd = rb->open(filename,O_RDONLY);
    if (fd < 0)
    {
        rb->splashf(HZ*2,"Couldn't open file: %s",filename);
        return PLUGIN_ERROR;
    }

    /* read in the file */
    while (rb->read_line(fd,temp_line,MAX_LINE_LEN))
    {
        if (rb->strncmp(temp_line, "$", 1) == 0)
        {
            /* read view preferences */
            linelen = rb->strlen(temp_line);
            if (linelen >= 2)
            {
                unsigned int val = temp_line[1] - '0';
                if (val < VIEW_TYPE_SIZE)
                {
                    view = val;
                }
            }
            if (linelen >= 3)
            {
                unsigned int val = temp_line[2] - '0';
                if (val <= 2)
                {
                    show_categories = val;
                    update_category_string();
                }
            }
        }
        else
        {
            new_item.id = count;
            if (rb->strncmp(temp_line, " ", 1) == 0)
            {
                /* read description, flag = cleared */
                new_item.flag = FL_CLEARED;
                rb->memcpy(new_item.desc, &temp_line[1], DESC_SIZE);
            }
            else if (rb->strncmp(temp_line, "!", 1) == 0)
            {
                /* read description, flag = set */
                new_item.flag = FL_SET;
                rb->memcpy(new_item.desc, &temp_line[1], DESC_SIZE);
            }
            else if (rb->strncmp(temp_line, "#", 1) == 0)
            {
                /* read description, flag = category */
                new_item.flag = FL_CATEGORY;
                rb->memcpy(new_item.desc, &temp_line[1], DESC_SIZE);
            }
            else
            {
                /* read description, flag = cleared */
                new_item.flag = FL_CLEARED;
                rb->memcpy(new_item.desc, temp_line, DESC_SIZE);
            }
            items_list[total_item_count] = new_item;
            total_item_count++;
            if (total_item_count == MAX_LIST_SIZE)
            {
                total_item_count = MAX_LIST_SIZE - 1;
                rb->splashf(HZ*2, "Truncating shopping list to %d items",
                            MAX_LIST_SIZE - 1);
                changed = true;
                rb->close(fd);
                return PLUGIN_OK;
            }
        }
    }
    rb->close(fd);
    changed = false;
    return PLUGIN_OK;
}

/* this is the plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
    struct gui_synclist lists;
    bool exit = false;
    int button;
    int cur_sel = 0;

#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(1);
#endif
    if (parameter)
    {
        rb->strcpy(filename,(char*)parameter);

        if (load_file() == PLUGIN_ERROR)
            return PLUGIN_ERROR;
    }
    else
        return PLUGIN_ERROR;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(0);
#endif
    /* now dump it in the list */
    rb->gui_synclist_init(&lists,list_get_name_cb,0, false, 1, NULL);
    rb->gui_synclist_set_icon_callback(&lists, list_get_icon_cb);
    rb->gui_synclist_limit_scroll(&lists,true);
    create_view(&lists);
    rb->gui_synclist_set_nb_items(&lists,view_item_count);
    rb->gui_synclist_select_item(&lists, 0);
    rb->gui_synclist_draw(&lists);
    rb->lcd_update();

    while (!exit)
    {
        rb->gui_synclist_draw(&lists);
        cur_sel = rb->gui_synclist_get_sel_pos(&lists);
        button = rb->get_action(CONTEXT_LIST,TIMEOUT_BLOCK);
        if (rb->gui_synclist_do_button(&lists,&button,LIST_WRAP_UNLESS_HELD))
            continue;
        switch (button)
        {
            case ACTION_STD_OK:
            {
                changed |= toggle(cur_sel);
                break;
            }
            case ACTION_STD_MENU:
            case ACTION_STD_CONTEXT:
            {
                switch(view)
                {
                    case EDIT_SHOPPING_LIST:
                    {
                        MENUITEM_STRINGLIST(menu, "Options", NULL,
                                                  "View shopping list",
                                                  "Clear all items",
                                                  "Mark all items",
                                                  category_string,
                                                  "Revert to saved",
                                                  "Show Playback Menu",
                                                  "Quit without saving",
                                                  "Quit");

                        switch (rb->do_menu(&menu, NULL, NULL, false))
                        {
                            case 0:
                            {
                                /* view shopping list */
                                view = VIEW_SHOPPING_LIST;
                                changed = true;
                                break;
                            }
                            case 1:
                            {
                                /* clear all items */
                                int i;
                                for (i = 0; i < total_item_count; i++)
                                {
                                    if (items_list[i].flag == FL_SET)
                                        items_list[i].flag = FL_CLEARED;
                                }
                                changed = true;
                                break;
                            }
                            case 2:
                            {
                                /* mark all items */
                                int i;
                                for (i = 0; i < total_item_count; i++)
                                {
                                    if (items_list[i].flag == FL_CLEARED)
                                        items_list[i].flag = FL_SET;
                                }
                                changed = true;
                                break;
                            }
                            case 3:
                            {
                                /* toggle categories */
                                show_categories ^= true;
                                update_category_string();
                                changed = true;
                                break;
                            }
                            case 4:
                            {
                                /* revert to saved */
                                if (load_file() == PLUGIN_ERROR)
                                    return PLUGIN_ERROR;
                                break;
                            }
                            case 5:
                            {
                                /* playback menu */
                                playback_control(NULL);
                                break;
                            }
                            case 6:
                            {
                                /* Quit without saving */
                                exit = 1;
                                break;
                            }
                            case 7:
                            {
                                /* Save and quit */
                                if (changed)
                                    save_changes();
                                exit = 1;
                                break;
                            }
                            default:
                            {
                                break;
                            }
                        }
                        break;
                    }

                    case VIEW_SHOPPING_LIST:
                    {
                        MENUITEM_STRINGLIST(menu, "Options", NULL,
                                                  "Edit list",
                                                  "Reset list",
                                                  category_string,
                                                  "Revert to saved",
                                                  "Show Playback Menu",
                                                  "Quit without saving",
                                                  "Quit");

                        switch (rb->do_menu(&menu, NULL, NULL, false))
                        {
                            case 0:
                            {
                                /* edit list */
                                view = EDIT_SHOPPING_LIST;
                                changed = true;
                                break;
                            }
                            case 1:
                            {
                                /* reset list */
                                int i;
                                for (i = 0; i < total_item_count; i++)
                                {
                                    if (items_list[i].flag == FL_SET)
                                        items_list[i].flag = FL_CLEARED;
                                }
                                view = EDIT_SHOPPING_LIST;
                                changed = true;
                                break;
                            }
                            case 2:
                            {
                                /* toggle categories */
                                show_categories ^= true;
                                update_category_string();
                                changed = true;
                                break;
                            }
                            case 3:
                            {
                                /* revert to saved */
                                if (load_file() == PLUGIN_ERROR)
                                    return PLUGIN_ERROR;
                                break;
                            }
                            case 4:
                            {
                                /* playback menu */
                                playback_control(NULL);
                                break;
                            }
                            case 5:
                            {
                                /* Quit without saving */
                                exit = 1;
                                break;
                            }
                            case 6:
                            {
                                /* Save and quit */
                                if (changed)
                                    save_changes();
                                exit = 1;
                                break;
                            }
                            default:
                            {
                                break;
                            }
                        }
                        break;
                    }
                }
                break;
            }
            case ACTION_STD_CANCEL:
            {
                if (changed)
                    save_changes();
                exit = 1;
                break;
            }
        }

        create_view(&lists);
        rb->gui_synclist_set_nb_items(&lists,view_item_count);
        if (view_item_count > 0 && view_item_count <= cur_sel)
            rb->gui_synclist_select_item(&lists,view_item_count-1);
    }
    return PLUGIN_OK;
}
