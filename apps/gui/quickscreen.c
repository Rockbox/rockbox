/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2008 by Jonathan Gordon
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
#include "icons.h"
#include "font.h"
#include "misc.h"
#include "action.h"
#include "settings_list.h"
#include "lang.h"
#include "viewport.h"
#include "quickscreen.h"
#include "talk.h"
#include "option_select.h"
#include "appevents.h"
#include "statusbar-skinned.h"

 /* 2 lines each for the top, middle, and bottom section = 6 in total.
    With less space, top and bottom sections lose a line = 4 in total. */
#define MIN_LINES (3*2)

#define MAX_NEEDED_LINES 10

 /* Minimum number of pixels between the 2 center items, between
    text and icons, or between text and parent boundaries. */
#define MARGIN 10
#define CENTER_ICONAREA_SIZE (MARGIN + 8*2)

#define FOR_QS_ITEMS(i) for (int i = 0; i < QUICKSCREEN_ITEM_COUNT; i++)

struct quickscreen
{
    const struct settings_list *items[QUICKSCREEN_ITEM_COUNT];
    struct viewport parent[NB_SCREENS];
    struct viewport vps[NB_SCREENS][QUICKSCREEN_ITEM_COUNT];
    struct viewport vp_icons[NB_SCREENS];
    int button_enter;
    enum quickscreen_return result;
    enum quickscreen_item volume_item;
};

/* Skin draws custom Quickscreen UI */
static bool qs_skinned[NB_SCREENS];

/* Toggle built-in interface, based on
   whether skin draws custom QS UI. */
void quickscreen_set_skinned(enum screen_type screen, bool skinned)
{
    qs_skinned[screen] = skinned;
}

static void quickscreen_fix_viewports(struct quickscreen *qs, enum screen_type screen)
{
    int line_height, width, pad = 0;
    int left_width = 0, right_width = 0;
    unsigned char *str;
    struct viewport *parent = &qs->parent[screen];
    struct viewport *vps = qs->vps[screen];
    struct viewport *vp_icons = &qs->vp_icons[screen];

    /* How many un-cropped lines can be displayed in the UI vp */
    int nb_lines = viewport_get_nb_lines(parent);
    if (nb_lines == 0)
        nb_lines = 1; /* cropped single line */

    line_height = parent->height/nb_lines;

    /* center the icons VP first */
    *vp_icons = *parent;
    vp_icons->width = CENTER_ICONAREA_SIZE; /* absolute smallest allowed */
    vp_icons->x = parent->x;
    vp_icons->x += (parent->width - CENTER_ICONAREA_SIZE)/2;

    vps[QUICKSCREEN_BOTTOM] = *parent;
    vps[QUICKSCREEN_TOP] = *parent;
    vps[QUICKSCREEN_TOP].y = parent->y;

    /* Top and bottom use 2 lines each, unless there's insufficient space */
    vps[QUICKSCREEN_TOP].height = vps[QUICKSCREEN_BOTTOM].height
            = (nb_lines < MIN_LINES ? 1 : 2)*line_height;
    vps[QUICKSCREEN_BOTTOM].y
            = parent->y + parent->height - vps[QUICKSCREEN_BOTTOM].height;

    /* enough space vertically, so put a nice margin */
    if (nb_lines >= MAX_NEEDED_LINES)
    {
        vps[QUICKSCREEN_TOP].y += MARGIN;
        vps[QUICKSCREEN_BOTTOM].y -= MARGIN;
    }

    vp_icons->y = vps[QUICKSCREEN_TOP].y + vps[QUICKSCREEN_TOP].height;
    vp_icons->height = vps[QUICKSCREEN_BOTTOM].y - vp_icons->y;

    /* adjust the left/right items widths to fit the screen nicely */
    if (qs->items[QUICKSCREEN_LEFT])
    {
        str = P2STR(ID2P(qs->items[QUICKSCREEN_LEFT]->lang_id));
        left_width = font_getstringsize(str, NULL, NULL, parent->font);
    }
    if (qs->items[QUICKSCREEN_RIGHT])
    {
        str = P2STR(ID2P(qs->items[QUICKSCREEN_RIGHT]->lang_id));
        right_width = font_getstringsize(str, NULL, NULL, parent->font);
    }

    width = MAX(left_width, right_width);

    /* crop text vp, if necessary */
    if (width*2 + vp_icons->width > parent->width)
    {
        width = parent->width;
        if (width > vp_icons->width) /* check if icons fit */
            width -=  vp_icons->width;
        else if (width > MARGIN) /* margin without icons */
            width -= MARGIN;

        if (width >= 2)
            width /= 2;
    }
    /* space for lager gap between between icons */
    else
    {
        int excess = parent->width - vp_icons->width - width*2;
        if (excess > MARGIN*4)
        {
            pad = MARGIN;
            excess -= MARGIN*2;
        }
        vp_icons->x -= excess/2;
        vp_icons->width += excess;
    }

    vps[QUICKSCREEN_LEFT] = *parent;
    vps[QUICKSCREEN_LEFT].x = parent->x + pad;
    vps[QUICKSCREEN_LEFT].width = width;

    vps[QUICKSCREEN_RIGHT] = *parent;
    vps[QUICKSCREEN_RIGHT].x = parent->x + parent->width - width - pad;
    vps[QUICKSCREEN_RIGHT].width = width;

    if (nb_lines >= 2) /* otherwise, use parent height and y position  */
    {
        vps[QUICKSCREEN_LEFT].height = vps[QUICKSCREEN_RIGHT].height
            = 2*line_height;
        vps[QUICKSCREEN_LEFT].y = vps[QUICKSCREEN_RIGHT].y
            = parent->y + (parent->height/2) - line_height;
    }

    /* shrink the icons vp by a few pixels if there is room so the arrows
       aren't drawn right next to the text */
    if (vp_icons->width > CENTER_ICONAREA_SIZE*2)
    {
        vp_icons->width -= CENTER_ICONAREA_SIZE*2/3;
        vp_icons->x += CENTER_ICONAREA_SIZE*2/6;
    }
    if (vp_icons->height > CENTER_ICONAREA_SIZE*2)
    {
        vp_icons->height -= CENTER_ICONAREA_SIZE*2/3;
        vp_icons->y += CENTER_ICONAREA_SIZE*2/6;
    }

    /* text alignment */
    vps[QUICKSCREEN_LEFT].flags &= ~VP_FLAG_ALIGNMENT_MASK;   /* left-aligned  */
    vps[QUICKSCREEN_TOP].flags    |= VP_FLAG_ALIGN_CENTER;    /* centered      */
    vps[QUICKSCREEN_BOTTOM].flags |= VP_FLAG_ALIGN_CENTER;    /* centered      */
    vps[QUICKSCREEN_RIGHT].flags  &= ~VP_FLAG_ALIGNMENT_MASK; /* right-aligned */
    vps[QUICKSCREEN_RIGHT].flags  |= VP_FLAG_ALIGN_RIGHT;
}

/* Draw settings item into current viewport */
static void quickscreen_draw_setting(const struct settings_list *item,
                                     struct screen *display, bool single_line)
{
    char buf[MAX_PATH];
    const char *title = P2STR(ID2P(item->lang_id));
    const char *value = option_get_valuestring(item, buf, sizeof buf,
                                               option_value_as_int(item));
    if (single_line)
    {
        char text[MAX_PATH];
        snprintf(text, sizeof text, "%s: %s", title, value);
        display->puts_scroll(0, 0, text);
    }
    else
    {
        display->puts_scroll(0, 0, title);
        display->puts_scroll(0, 1, value);
    }
}

/* Redraw viewports affected by the adjusted setting */
static void quickscreen_update(struct quickscreen *qs, enum quickscreen_item selected)
{
    FOR_NB_SCREENS(screen)
    {
        if (qs_skinned[screen])
            continue;

        struct screen *display = &screens[screen];
        struct viewport *vps = qs->vps[screen];

        FOR_QS_ITEMS(i)
            if (qs->items[i] == qs->items[selected])
            {
                struct viewport *last_vp = display->set_viewport(&vps[i]);
                display->clear_viewport();
                quickscreen_draw_setting(qs->items[i], display,
                                         viewport_get_nb_lines(&vps[i]) < 2);
                display->set_viewport(last_vp);
            }

        skin_mark_dirty(screen);
    }
}

/* Redraw whole Quickscreen */
static void quickscreen_draw(struct quickscreen *qs, enum screen_type screen)
{
    struct screen *display = &screens[screen];
    struct viewport *parent = &qs->parent[screen];
    struct viewport *vps = qs->vps[screen];
    struct viewport *vp_icons = &qs->vp_icons[screen];
    struct viewport *last_vp = display->set_viewport(parent);
    display->clear_viewport();

    FOR_QS_ITEMS(i)
        if (qs->items[i])
        {
            display->set_viewport(&vps[i]);
            quickscreen_draw_setting(qs->items[i], display,
                                     viewport_get_nb_lines(&vps[i]) < 2);
        }

    /* icons */
    if (parent->width > CENTER_ICONAREA_SIZE && vp_icons->height >= 8)
    {
        display->set_viewport(vp_icons);
        if (qs->items[QUICKSCREEN_TOP])
            display->mono_bitmap(bitmap_icons_7x8[Icon_UpArrow],
                                 (vp_icons->width/2) - 4, 0, 7, 8);

        if (qs->items[QUICKSCREEN_RIGHT])
            display->mono_bitmap(bitmap_icons_7x8[Icon_FastForward],
                                 vp_icons->width - 8, (vp_icons->height/2) - 4, 7, 8);

        if (qs->items[QUICKSCREEN_LEFT])
            display->mono_bitmap(bitmap_icons_7x8[Icon_FastBackward],
                                 0, (vp_icons->height/2) - 4, 7, 8);

        if (qs->items[QUICKSCREEN_BOTTOM])
            display->mono_bitmap(bitmap_icons_7x8[Icon_DownArrow],
                                 (vp_icons->width/2) - 4, vp_icons->height - 8, 7, 8);
    }

    skin_mark_dirty(screen);
    display->set_viewport(last_vp);
}

static void quickscreen_draw_cb(unsigned short id, void *data, void *userdata)
{
    (void)id;
    (void)data;

    FOR_NB_SCREENS(i)
        if (!qs_skinned[i])
            quickscreen_draw((struct quickscreen *) userdata, i);
}

static void talk_qs_option(const struct settings_list *opt, bool enqueue)
{
    if (!global_settings.talk_menu || !opt)
        return;

    if (enqueue)
        talk_id(opt->lang_id, enqueue);
    option_talk_value(opt, option_value_as_int(opt), enqueue);
}

/*
 * Does the actions associated to the given button if any
 *  - qs : the quickscreen
 *  - button : the key we are going to analyse
 * returns : true if the button corresponded to an action, false otherwise
 */
static bool quickscreen_do_button(struct quickscreen * qs, int button,
                                  enum quickscreen_item *item)
{
    bool previous = false;
    switch(button)
    {
        case ACTION_QS_TOP:
            *item = QUICKSCREEN_TOP;
            break;

        case ACTION_QS_LEFT:
            *item = QUICKSCREEN_LEFT;
            previous = true;
            break;

        case ACTION_QS_DOWN:
            *item = QUICKSCREEN_BOTTOM;
            previous = true;
            break;

        case ACTION_QS_RIGHT:
            *item = QUICKSCREEN_RIGHT;
            break;

        default:
            return false;
    }

    if (qs->items[*item] == NULL)
        return false;

    option_select_next_val(qs->items[*item], previous, true);
    talk_qs_option(qs->items[*item], false);
    return true;
}

#ifdef HAVE_TOUCHSCREEN
static int quickscreen_touchscreen_button(void)
{
    struct gesture_event gevent;
    if (!action_gesture_get_event(&gevent))
        return ACTION_NONE;

    switch (gevent.id) {
    case GESTURE_TAP:
    case GESTURE_HOLD:
        break;
    default:
        return ACTION_NONE;
    }

    enum { left=1, right=2, top=4, bottom=8 };

    int bits = 0;

    if(gevent.x < LCD_WIDTH/3)
        bits |= left;
    else if(gevent.x > 2*LCD_WIDTH/3)
        bits |= right;

    if(gevent.y < LCD_HEIGHT/3)
        bits |= top;
    else if(gevent.y > 2*LCD_HEIGHT/3)
        bits |= bottom;

    switch(bits) {
    case top:
        return ACTION_QS_TOP;
    case bottom:
        return ACTION_QS_DOWN;
    case left:
        return ACTION_QS_LEFT;
    case right:
        return ACTION_QS_RIGHT;
    default:
        return ACTION_STD_CANCEL;
    }
}
#endif

/* Undo activity, viewport, and event listener setup. */
static void cleanup(void *parameter)
{
    struct quickscreen *qs = (struct quickscreen *) parameter;
    remove_event_ex(GUI_EVENT_NEED_UI_UPDATE, quickscreen_draw_cb, qs);

    FOR_NB_SCREENS(i)
    {
        if (!qs_skinned[i])
            FOR_QS_ITEMS(j)
                screens[i].scroll_stop_viewport(&qs->vps[i][j]);
        viewportmanager_theme_undo(i, true);
    }
    pop_current_activity();
}

/* Set up activity, viewport, and event listener. Draw initial Quickscreen. */
static inline void setup(struct quickscreen *qs)
{
    push_current_activity(ACTIVITY_QUICKSCREEN);

    FOR_NB_SCREENS(i)
    {
        screens[i].set_viewport(NULL);
        screens[i].scroll_stop();
        viewportmanager_theme_enable(i, true, &qs->parent[i]);
        if (!qs_skinned[i])
        {
            quickscreen_fix_viewports(qs, i);
            quickscreen_draw(qs, i);
        }
    }
    add_event_ex(GUI_EVENT_NEED_UI_UPDATE, false, quickscreen_draw_cb, qs);
}

static void quickscreen_main(struct quickscreen * qs)
{
     /* To quit we need either :
     *  - a second press on the button that made us enter
     *  - an action taken while pressing the enter button,
     *    then release the enter button */
    bool can_quit = false;
    int button;
    enum quickscreen_item item;

    setup(qs);

    /* Announce current selection on entering this screen. This is all
       queued up, but can be interrupted as soon as a setting is
       changed. */
    cond_talk_ids(VOICE_QUICKSCREEN);
    talk_qs_option(qs->items[QUICKSCREEN_TOP], true);
    if (qs->items[QUICKSCREEN_TOP] != qs->items[QUICKSCREEN_BOTTOM])
        talk_qs_option(qs->items[QUICKSCREEN_BOTTOM], true);
    talk_qs_option(qs->items[QUICKSCREEN_LEFT], true);
    if (qs->items[QUICKSCREEN_LEFT] != qs->items[QUICKSCREEN_RIGHT])
        talk_qs_option(qs->items[QUICKSCREEN_RIGHT], true);

#ifdef HAVE_TOUCHSCREEN
    action_gesture_reset();
#endif
    while (true)
    {
        button = get_action(CONTEXT_QUICKSCREEN, HZ/5);
#ifdef HAVE_TOUCHSCREEN
        if (button == ACTION_TOUCHSCREEN)
            button = quickscreen_touchscreen_button();
#endif
        if (default_event_handler_ex(button, cleanup, qs)
            == SYS_USB_CONNECTED)
        {
            qs->result |= QUICKSCREEN_IN_USB;
            return;
        }
        if (quickscreen_do_button(qs, button, &item))
        {
            qs->result |= QUICKSCREEN_CHANGED;
            can_quit = true;
            quickscreen_update(qs, item);
        }
        else if (button == qs->button_enter)
            can_quit = true;
        else if (button == ACTION_QS_VOLUP || button == ACTION_QS_VOLDOWN)
        {
            adjust_volume(button == ACTION_QS_VOLUP ? 1 : -1);
            if (qs->volume_item < QUICKSCREEN_ITEM_COUNT)
                quickscreen_update(qs, qs->volume_item);
        }
        else if (button == ACTION_STD_CONTEXT)
        {
            qs->result |= QUICKSCREEN_GOTO_SHORTCUTS_MENU;
            break;
        }
        if ((button == qs->button_enter) && can_quit)
            break;

        if (button == ACTION_STD_CANCEL)
            break;

        /* Ignore SBS update_delay, so that setting
           changes are immediately visible. */
        sb_skin_force_next_update();
    }
    /* Notify that we're exiting this screen */
    cond_talk_ids_fq(VOICE_OK);

    cleanup(qs);
}

/* Entry point for external callers */
int quickscreen_show(int button_enter)
{
    struct quickscreen qs;
    qs.button_enter = button_enter;
    qs.result = QUICKSCREEN_OK;
    qs.volume_item = QUICKSCREEN_ITEM_COUNT;

    FOR_QS_ITEMS(i)
    {
        qs.items[i] = global_settings.qs_items[i];

        if (!is_setting_quickscreenable(qs.items[i]))
            qs.items[i] = NULL;

        if (qs.items[i] && qs.items[i]->lang_id == LANG_VOLUME)
            qs.volume_item = i;
    }

    quickscreen_main(&qs);

    if (qs.result & QUICKSCREEN_CHANGED)
        settings_save();

    return qs.result & ~QUICKSCREEN_CHANGED;
}

/* stuff to make the quickscreen configurable */
bool is_setting_quickscreenable(const struct settings_list *setting)
{
    if (!setting)
        return true;

    /* to keep things simple, only settings which have a lang_id set are ok */
    if (setting->lang_id < 0 || (setting->flags & F_BANFROMQS))
        return false;

    switch (setting->flags & F_T_MASK)
    {
        case F_T_BOOL:
            return true;
        case F_T_INT:
        case F_T_UINT:
            return (setting->RESERVED != NULL);
        default:
            return false;
    }
}
