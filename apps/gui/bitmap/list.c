/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Jonathan Gordon
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* This file contains the code to draw the list widget on BITMAP LCDs. */

#include "config.h"
#include "lcd.h"
#include "font.h"
#include "button.h"
#include "sprintf.h"
#include "string.h"
#include "settings.h"
#include "kernel.h"
#include "system.h"

#include "action.h"
#include "screen_access.h"
#include "list.h"
#include "scrollbar.h"
#include "statusbar.h"
#include "textarea.h"
#include "lang.h"
#include "sound.h"
#include "misc.h"
#include "talk.h"
#include "viewport.h"

#define SCROLLBAR_WIDTH 6
#define ICON_PADDING 1

/* globals */
struct viewport title_text[NB_SCREENS], title_icons[NB_SCREENS],
                list_text[NB_SCREENS], list_icons[NB_SCREENS];

/* should probably be moved somewhere else */
int list_title_height(struct gui_synclist *list, struct viewport *vp)
{
    (void)list;
    return font_get(vp->font)->height;
}
int gui_list_get_item_offset(struct gui_synclist * gui_list, int item_width,
                             int text_pos, struct screen * display,
                             struct viewport *vp);
bool list_display_title(struct gui_synclist *list, struct viewport *vp);

/* Draw the list...
    internal screen layout:
        -----------------
        |TI|  title     |   TI is title icon
        -----------------
        | | |            |
        |S|I|            |   S - scrollbar
        | | | items      |   I - icons
        | | |            |
        ------------------
*/
static bool draw_title(struct screen *display, struct viewport *parent,
                       struct gui_synclist *list)
{
    struct viewport *vp_icons = &title_icons[display->screen_type];
    struct viewport *vp_text = &title_text[display->screen_type];
    if (!list_display_title(list, parent))
        return false;
    *vp_text = *parent;
    vp_text->height = list_title_height(list, parent);
    if (list->title_icon != Icon_NOICON && global_settings.show_icons)
    {
        *vp_icons = *vp_text;
        vp_icons->width = get_icon_width(display->screen_type)
                          + ICON_PADDING*2;
        vp_icons->x += ICON_PADDING;
        
        vp_text->width -= vp_icons->width + vp_icons->x;
        vp_text->x += vp_icons->width + vp_icons->x;
        
        display->set_viewport(vp_icons);
        screen_put_icon(display, 0, 0, list->title_icon);
    }
    display->set_viewport(vp_text);
    vp_text->drawmode = STYLE_DEFAULT;
#ifdef HAVE_LCD_COLOR
    if (list->title_color >= 0)
    {
        vp_text->drawmode |= (STYLE_COLORED|list->title_color);}
#endif
    display->puts_scroll_style_offset(0, 0, list->title,
                                      vp_text->drawmode, 0);
    return true;
}
    
void list_draw(struct screen *display, struct viewport *parent,
               struct gui_synclist *list)
{
    int start, end, line_height, i;
    int icon_width = get_icon_width(display->screen_type) + ICON_PADDING;
    bool show_cursor = !global_settings.cursor_style &&
                        list->show_selection_marker;
#ifdef HAVE_LCD_COLOR
    unsigned char cur_line = 0;
#endif
    int item_offset;
    bool show_title;
    line_height = font_get(parent->font)->height;
    display->set_viewport(parent);
    display->clear_viewport();
    display->stop_scroll();
    list_text[display->screen_type] = *parent;
    if ((show_title = draw_title(display, parent, list)))
    {
        list_text[display->screen_type].y += list_title_height(list, parent);
        list_text[display->screen_type].height -= list_title_height(list, parent);
    }
        
    start = list->start_item[display->screen_type];
    end = start + viewport_get_nb_lines(&list_text[display->screen_type]);
    
    /* draw the scrollbar if its needed */
    if (global_settings.scrollbar &&
        viewport_get_nb_lines(&list_text[display->screen_type]) < list->nb_items)
    {
        struct viewport vp;
        vp = list_text[display->screen_type];
        vp.width = SCROLLBAR_WIDTH;
        list_text[display->screen_type].width -= SCROLLBAR_WIDTH;
        list_text[display->screen_type].x += SCROLLBAR_WIDTH;
        vp.height = line_height *
                    viewport_get_nb_lines(&list_text[display->screen_type]);
        vp.x = parent->x;
        display->set_viewport(&vp);
        gui_scrollbar_draw(display, 0, 0, SCROLLBAR_WIDTH-1,
                           vp.height, list->nb_items,
                           list->start_item[display->screen_type],
                           list->start_item[display->screen_type] + end-start,
                           VERTICAL);
    }
    else if (show_title)
    {
        /* shift everything right a bit... */
        list_text[display->screen_type].width -= SCROLLBAR_WIDTH;
        list_text[display->screen_type].x += SCROLLBAR_WIDTH;
    }
    
    /* setup icon placement */
    list_icons[display->screen_type] = list_text[display->screen_type];
    int icon_count = global_settings.show_icons && 
            (list->callback_get_item_icon != NULL) ? 1 : 0;
    if (show_cursor)
        icon_count++;
    if (icon_count)
    {
        list_icons[display->screen_type].width = icon_width * icon_count;
        list_text[display->screen_type].width -= 
                list_icons[display->screen_type].width + ICON_PADDING;
        list_text[display->screen_type].x += 
                list_icons[display->screen_type].width + ICON_PADDING;
    }
    
    for (i=start; i<end && i<list->nb_items; i++)
    {
        /* do the text */
        unsigned char *s;
        char entry_buffer[MAX_PATH];
        unsigned char *entry_name;
        int text_pos = 0;
        s = list->callback_get_item_name(i, list->data, entry_buffer);
        entry_name = P2STR(s);
        display->set_viewport(&list_text[display->screen_type]);
        list_text[display->screen_type].drawmode = STYLE_DEFAULT;
        /* position the string at the correct offset place */
        int item_width,h;
        display->getstringsize(entry_name, &item_width, &h);
        item_offset = gui_list_get_item_offset(list, item_width,
                                               text_pos, display, 
                                               &list_text[display->screen_type]);

#ifdef HAVE_LCD_COLOR
        /* if the list has a color callback */
        if (list->callback_get_item_color)
        {
            int color = list->callback_get_item_color(i, list->data);
            /* if color selected */
            if (color >= 0)
            {
                list_text[display->screen_type].drawmode |= STYLE_COLORED|color;
            }
        }
#endif

        if(list->show_selection_marker && global_settings.cursor_style &&
           i >= list->selected_item &&
           i <  list->selected_item + list->selected_size)
        {/* The selected item must be displayed scrolling */
            if (global_settings.cursor_style == 1 || display->depth < 16)
            {
                /* Display inverted-line-style */
                list_text[display->screen_type].drawmode |= STYLE_INVERT;
            }
#ifdef HAVE_LCD_COLOR
            else if (global_settings.cursor_style == 2)
            {
                /* Display colour line selector */
                list_text[display->screen_type].drawmode |= STYLE_COLORBAR;
            }
            else if (global_settings.cursor_style == 3)
            {
                /* Display gradient line selector */
                list_text[display->screen_type].drawmode = STYLE_GRADIENT;

                /* Make the lcd driver know how many lines the gradient should
                   cover and current line number */
                /* number of selected lines */
                list_text[display->screen_type].drawmode |= NUMLN_PACK(list->selected_size);
                /* current line number, zero based */
                list_text[display->screen_type].drawmode |= CURLN_PACK(cur_line);
                cur_line++;
            }
#endif
            /* if the text is smaller than the viewport size */
            if (item_offset > item_width - (list_text[display->screen_type].width - text_pos))
            {
                /* don't scroll */
                display->puts_style_offset(0, i-start, entry_name,
                                           list_text[display->screen_type].drawmode, item_offset);
            }
            else
            {
                display->puts_scroll_style_offset(0, i-start, entry_name,
                                                  list_text[display->screen_type].drawmode, item_offset);
            }
        }
        else
            display->puts_style_offset(0, i-start, entry_name,
                                                list_text[display->screen_type].drawmode, item_offset);
        /* do the icon */
        if (list->callback_get_item_icon && global_settings.show_icons)
        {
            display->set_viewport(&list_icons[display->screen_type]);
            screen_put_icon_with_offset(display, show_cursor?1:0,
                                        (i-start),show_cursor?ICON_PADDING:0,0,
                                        list->callback_get_item_icon(i, list->data));
            if (show_cursor && i >= list->selected_item &&
                i <  list->selected_item + list->selected_size)
            {
                screen_put_icon(display, 0, i-start, Icon_Cursor);
            }
        }
        else if (show_cursor && i >= list->selected_item &&
                i <  list->selected_item + list->selected_size)
        {
            display->set_viewport(&list_icons[display->screen_type]);
            screen_put_icon(display, 0, (i-start), Icon_Cursor);
        }
    }
    
    display->set_viewport(parent);
    display->update_viewport();
    display->set_viewport(NULL);
}


