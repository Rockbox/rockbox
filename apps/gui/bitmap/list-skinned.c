/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Jonathan Gordon
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
#include "system.h"
#include "lcd.h"
#include "font.h"
#include "button.h"
#include "string.h"
#include "settings.h"
#include "kernel.h"
#include "file.h"

#include "action.h"
#include "screen_access.h"
#include "list.h"
#include "scrollbar.h"
#include "lang.h"
#include "sound.h"
#include "misc.h"
#include "viewport.h"
#include "statusbar-skinned.h"
#include "skin_engine/skin_engine.h"
#include "skin_engine/skin_display.h"
#include "appevents.h"

static struct listitem_viewport_cfg *listcfg[NB_SCREENS] = {NULL};
static struct gui_synclist *current_list;

static int current_row;
static int current_column;

void skinlist_set_cfg(enum screen_type screen,
                      struct listitem_viewport_cfg *cfg)
{
    if (listcfg[screen] != cfg)
    {
        if (listcfg[screen])
            screens[screen].scroll_stop(&listcfg[screen]->selected_item_vp.vp);
        listcfg[screen] = cfg;
        current_list = NULL;
        current_column = -1;
        current_row = -1;
    }
}

static bool skinlist_is_configured(enum screen_type screen,
                                    struct gui_synclist *list)
{
    return (listcfg[screen] != NULL) &&
            (!list || (list && list->selected_size == 1));
}
static int current_drawing_line;
static int offset_to_item(int offset, bool wrap)
{
    int item = current_drawing_line + offset;
    if (!current_list || current_list->nb_items == 0)
        return -1;
    if (item < 0)
    {
        if (!wrap)
            return -1;
        else
            item = (item + current_list->nb_items) % current_list->nb_items;
    }
    else if (item >= current_list->nb_items && !wrap)
        return -1;
    else
        item = item % current_list->nb_items;
    return item;
}

int skinlist_get_item_number()
{
    return current_drawing_line;
}

int skinlist_get_item_row()
{
    return current_row;
}

int skinlist_get_item_column()
{
    return current_column;
}


const char* skinlist_get_item_text(int offset, bool wrap, char* buf, size_t buf_size)
{
    int item = offset_to_item(offset, wrap);
    if (item < 0 || !current_list)
        return NULL;
    const char* ret = current_list->callback_get_item_name(
                    item, current_list->data, buf, buf_size);
    return P2STR((unsigned char*)ret);
}

enum themable_icons skinlist_get_item_icon(int offset, bool wrap)
{
    int item = offset_to_item(offset, wrap);
    if (item < 0 || !current_list || current_list->callback_get_item_icon == NULL)
        return Icon_NOICON;
    return current_list->callback_get_item_icon(item, current_list->data);
}

static bool is_selected = false;
bool skinlist_is_selected_item(void)
{
    return is_selected;
}

int skinlist_get_line_count(enum screen_type screen, struct gui_synclist *list)
{
    struct viewport *parent = (list->parent[screen]);
    if (!skinlist_is_configured(screen, list))
        return -1;
    if (listcfg[screen]->tile == true)
    {
        int rows = (parent->height / listcfg[screen]->height);
        int cols = (parent->width / listcfg[screen]->width);
        return rows*cols;
    }
    else
        return  (parent->height / listcfg[screen]->height);
}

static int current_item;
static int current_nbitems;
static bool needs_scrollbar[NB_SCREENS];
bool skinlist_needs_scrollbar(enum screen_type screen)
{
    return needs_scrollbar[screen];
}

void skinlist_get_scrollbar(int* nb_item, int* first_shown, int* last_shown)
{
    if (!skinlist_is_configured(0, NULL))
    {
        *nb_item = 0;
        *first_shown = 0;
        *last_shown = 0;
    }
    else
    {
        *nb_item = current_item;
        *first_shown = 0;
        *last_shown = current_nbitems;
    }
}

bool skinlist_get_item(struct screen *display, struct gui_synclist *list, int x, int y, int *item)
{
    const int screen = display->screen_type;
    if (!skinlist_is_configured(screen, list))
        return false;

    int row = y / listcfg[screen]->height;
    int column = x / listcfg[screen]->width;
    struct viewport *parent = (list->parent[screen]);
    int cols = (parent->width / listcfg[screen]->width);
    *item = row * cols+ column;
    return true;
}

bool skinlist_draw(struct screen *display, struct gui_synclist *list)
{
    int cur_line, display_lines;
    const int screen = display->screen_type;
    struct viewport *parent = (list->parent[screen]);
    char* label = NULL;
    const int list_start_item = list->start_item[screen];
    struct gui_wps wps;
    if (!skinlist_is_configured(screen, list))
        return false;

    current_list = list;
    wps.display = display;
    wps.data = listcfg[screen]->data;
    display_lines = skinlist_get_line_count(screen, list);
    label = (char *)SKINOFFSETTOPTR(get_skin_buffer(wps.data), listcfg[screen]->label);
    display->set_viewport(parent);
    display->clear_viewport();
    current_item = list->selected_item;
    current_nbitems = list->nb_items;
    needs_scrollbar[screen] = list->nb_items > display_lines;

    for (cur_line = 0; cur_line < display_lines; cur_line++)
    {
        struct skin_element* viewport;
        struct skin_viewport* skin_viewport = NULL;
        if (list_start_item+cur_line+1 > list->nb_items)
            break;
        current_drawing_line = list_start_item+cur_line;
        is_selected = list->show_selection_marker &&
                list_start_item+cur_line == list->selected_item;

        for (viewport = SKINOFFSETTOPTR(get_skin_buffer(wps.data), listcfg[screen]->data->tree);
             viewport;
             viewport = SKINOFFSETTOPTR(get_skin_buffer(wps.data), viewport->next))
        {
            int original_x, original_y;
            skin_viewport = SKINOFFSETTOPTR(get_skin_buffer(wps.data), viewport->data);
            char *viewport_label = SKINOFFSETTOPTR(get_skin_buffer(wps.data), skin_viewport->label);
            if (viewport->children == 0 || !viewport_label ||
                (skin_viewport->label && strcmp(label, viewport_label))
                )
                continue;
            if (is_selected)
            {
                memcpy(&listcfg[screen]->selected_item_vp, skin_viewport, sizeof(struct skin_viewport));
                skin_viewport = &listcfg[screen]->selected_item_vp;
            }
            original_x = skin_viewport->vp.x;
            original_y = skin_viewport->vp.y;
            if (listcfg[screen]->tile)
            {
                int cols = (parent->width / listcfg[screen]->width);
                current_column = (cur_line)%cols;
                current_row = (cur_line)/cols;
                
                skin_viewport->vp.x = parent->x + listcfg[screen]->width*current_column + original_x;
                skin_viewport->vp.y = parent->y + listcfg[screen]->height*current_row + original_y;
            }
            else
            {
                current_column = 1;
                current_row = cur_line;
                skin_viewport->vp.x = parent->x + original_x;
                skin_viewport->vp.y = parent->y + original_y +
                                   (listcfg[screen]->height*cur_line);
            }
            display->set_viewport(&skin_viewport->vp);
#ifdef HAVE_LCD_BITMAP
            /* Set images to not to be displayed */
            struct skin_token_list *imglist = SKINOFFSETTOPTR(get_skin_buffer(wps.data), wps.data->images);
            while (imglist)
            {
                struct wps_token *token = SKINOFFSETTOPTR(get_skin_buffer(wps.data), imglist->token);
                struct gui_img *img = SKINOFFSETTOPTR(get_skin_buffer(wps.data), token->value.data);
                img->display = -1;
                imglist = SKINOFFSETTOPTR(get_skin_buffer(wps.data), imglist->next);
            }
#endif
            struct skin_element** children = SKINOFFSETTOPTR(get_skin_buffer(wps.data), viewport->children);
            skin_render_viewport(SKINOFFSETTOPTR(get_skin_buffer(wps.data), (intptr_t)children[0]),
                                 &wps, skin_viewport, SKIN_REFRESH_ALL);
#ifdef HAVE_LCD_BITMAP
            wps_display_images(&wps, &skin_viewport->vp);
#endif
            /* force disableing scroll because it breaks later */
            if (!is_selected)
            {
                display->scroll_stop(&skin_viewport->vp);
                skin_viewport->vp.x = original_x;
                skin_viewport->vp.y = original_y;
            }
        }
    }
    current_column = -1;
    current_row = -1;
    display->set_viewport(parent);
    display->update_viewport();
    current_drawing_line = list->selected_item;
    return true;
}

