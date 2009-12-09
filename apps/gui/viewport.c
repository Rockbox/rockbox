/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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

#include <stdlib.h>
#include "config.h"
#include "lcd.h"
#include "lcd-remote.h"
#include "font.h"
#include "viewport.h"
#include "screen_access.h"
#include "settings.h"
#include "misc.h"
#include "panic.h"
#include "viewport.h"

/*some short cuts for fg/bg/line selector handling */
#ifdef HAVE_LCD_COLOR
#define FG_FALLBACK global_settings.fg_color
#define BG_FALLBACK global_settings.bg_color
#else
#define FG_FALLBACK LCD_DEFAULT_FG
#define BG_FALLBACK LCD_DEFAULT_BG
#endif

/* all below isn't needed for pc tools (i.e. checkwps/wps editor)
 * only viewport_parse_viewport() is */
#ifndef __PCTOOL__
#include "sprintf.h"
#include "string.h"
#include "kernel.h"
#include "system.h"
#include "statusbar.h"
#include "appevents.h"
#ifdef HAVE_LCD_BITMAP
#include "language.h"
#endif
#include "statusbar-skinned.h"
#include "debug.h"
#include "viewport.h"

#define VPSTACK_DEPTH 16
struct viewport_stack_item
{
    struct  viewport* vp;
    bool   enabled;
};

#ifdef HAVE_LCD_BITMAP
static void viewportmanager_redraw(void* data);
   
static int theme_stack_top[NB_SCREENS]; /* the last item added */
static struct viewport_stack_item theme_stack[NB_SCREENS][VPSTACK_DEPTH];
static bool is_theme_enabled(enum screen_type screen);

static void toggle_theme(void)
{
    bool enable_event = false;
    static bool was_enabled[NB_SCREENS] = {false};
    int i;
    FOR_NB_SCREENS(i)
    {
        enable_event = enable_event || is_theme_enabled(i);
    }
    if (enable_event)
    {
        add_event(GUI_EVENT_ACTIONUPDATE, false, viewportmanager_redraw);
#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
        add_event(LCD_EVENT_ACTIVATION, false, do_sbs_update_callback);
#endif
        add_event(PLAYBACK_EVENT_TRACK_CHANGE, false,
                                                do_sbs_update_callback);
        add_event(PLAYBACK_EVENT_NEXTTRACKID3_AVAILABLE, false,
                                                do_sbs_update_callback);
        
        /* remove the left overs from the previous screen.
         * could cause a tiny flicker. Redo your screen code if that happens */
        FOR_NB_SCREENS(i)
        {
            if (!was_enabled[i])
            {
                struct viewport deadspace, user;
                viewport_set_defaults(&user, i);
                deadspace = user; /* get colours and everything */
                /* above */
                deadspace.x = 0;
                deadspace.y = 0;
                deadspace.width = screens[i].lcdwidth;
                deadspace.height = user.y;
                if (deadspace.width && deadspace.height)
                {
                    screens[i].set_viewport(&deadspace);
                    screens[i].clear_viewport();
                    screens[i].update_viewport();
                }
                /* below */
                deadspace.y = user.y + user.height;
                deadspace.height = screens[i].lcdheight - deadspace.y;
                if (deadspace.width && deadspace.height)
                {
                    screens[i].set_viewport(&deadspace);
                    screens[i].clear_viewport();
                    screens[i].update_viewport();
                }
                /* left */
                deadspace.x = 0;
                deadspace.y = 0;
                deadspace.width = user.x;
                deadspace.height = screens[i].lcdheight;
                if (deadspace.width && deadspace.height)
                {
                    screens[i].set_viewport(&deadspace);
                    screens[i].clear_viewport();
                    screens[i].update_viewport();
                }
                /* below */
                deadspace.x = user.x + user.width;
                deadspace.width = screens[i].lcdwidth - deadspace.x;
                if (deadspace.width && deadspace.height)
                {
                    screens[i].set_viewport(&deadspace);
                    screens[i].clear_viewport();
                    screens[i].update_viewport();
                }
            }
        }
        send_event(GUI_EVENT_ACTIONUPDATE, (void*)1); /* force a redraw */
    }
    else
    {
        FOR_NB_SCREENS(i)
            screens[i].stop_scroll();
#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
        remove_event(LCD_EVENT_ACTIVATION, do_sbs_update_callback);
#endif
        remove_event(PLAYBACK_EVENT_TRACK_CHANGE, do_sbs_update_callback);
        remove_event(PLAYBACK_EVENT_NEXTTRACKID3_AVAILABLE, do_sbs_update_callback);
        remove_event(GUI_EVENT_ACTIONUPDATE, viewportmanager_redraw);
    }
        
    FOR_NB_SCREENS(i)
        was_enabled[i] = is_theme_enabled(i);
}

void viewportmanager_theme_enable(enum screen_type screen, bool enable,
                                 struct viewport *viewport)
{
    int top = ++theme_stack_top[screen];
    if (top >= VPSTACK_DEPTH-1)
        panicf("Stack overflow... viewportmanager");
    theme_stack[screen][top].enabled = enable;
    theme_stack[screen][top].vp = viewport;
    toggle_theme();
    /* then be nice and set the viewport up */
    if (viewport)
        viewport_set_defaults(viewport, screen);
}

void viewportmanager_theme_undo(enum screen_type screen)
{
    int top = --theme_stack_top[screen];
    if (top < 0)
        panicf("Stack underflow... viewportmanager");
    
    toggle_theme();
}


static bool is_theme_enabled(enum screen_type screen)
{
    int top = theme_stack_top[screen];
    return theme_stack[screen][top].enabled;
}

static bool custom_vp_loaded_ok[NB_SCREENS];
static struct viewport custom_vp[NB_SCREENS];

static unsigned viewport_init_ui_vp(void);
#endif /* HAVE_LCD_BITMAP */

int viewport_get_nb_lines(const struct viewport *vp)
{
#ifdef HAVE_LCD_BITMAP
    return vp->height/font_get(vp->font)->height;
#else
    (void)vp;
    return 2;
#endif
}

static void viewportmanager_redraw(void* data)
{
    int i;
    FOR_NB_SCREENS(i)
    {
#ifdef HAVE_LCD_BITMAP
       if (statusbar_position(i) == STATUSBAR_CUSTOM)
           sb_skin_update(i, NULL != data);
        else if (statusbar_position(i) != STATUSBAR_OFF)
#endif
            gui_statusbar_draw(&statusbars.statusbars[i], NULL != data);
    }
}

void viewportmanager_init()
{
#ifdef HAVE_LCD_BITMAP
    int i;
    FOR_NB_SCREENS(i)
    {
        theme_stack_top[i] = -1; /* the next call fixes this to 0 */
        /* We always want the theme enabled by default... */
        viewportmanager_theme_enable(i, true, NULL); 
    }
#else
    add_event(GUI_EVENT_ACTIONUPDATE, false, viewportmanager_redraw);
#endif
}

#ifdef HAVE_LCD_BITMAP
void viewportmanager_theme_changed(const int which)
{
    int i;
#ifdef HAVE_BUTTONBAR
    if (which & THEME_BUTTONBAR)
    {   /* don't handle further, the custom ui viewport ignores the buttonbar,
         * as does viewport_set_defaults(), since only lists use it*/
        screens[SCREEN_MAIN].has_buttonbar = global_settings.buttonbar;
    }
#endif
    if (which & THEME_UI_VIEWPORT)
    {
        viewport_init_ui_vp();
    }
    if (which & THEME_LANGUAGE)
    {   
    }
    if (which & THEME_STATUSBAR)
    {
        FOR_NB_SCREENS(i)
        {
            /* This can probably be done better...
             * disable the theme so it's forced to do a full redraw  */
            viewportmanager_theme_enable(i, false, NULL);
            viewportmanager_theme_undo(i);
        }
    }
    send_event(GUI_EVENT_THEME_CHANGED, NULL);
}

/*
 * (re)parse the UI vp from the settings
 *  - Returns
 *          0 if no UI vp is used at all
 *          else the bit for the screen (1<<screen) is set
 */
static unsigned viewport_init_ui_vp(void)
{
    int screen;
    const char *ret = NULL;
    char *setting;
    FOR_NB_SCREENS(screen)
    {
#ifdef HAVE_REMOTE_LCD
        if ((screen == SCREEN_REMOTE))
            setting = global_settings.remote_ui_vp_config;
        else
#endif
            setting = global_settings.ui_vp_config;
            
        ret = viewport_parse_viewport(&custom_vp[screen], screen,
                                     setting, ',');

        custom_vp_loaded_ok[screen] = ret?true:false;
    }
    return true; /* meh fixme */
}

#ifdef HAVE_TOUCHSCREEN
/* check if a point (x and y coordinates) are within a viewport */
bool viewport_point_within_vp(const struct viewport *vp,
                               const int x, const int y)
{
    bool is_x = (x >= vp->x && x < (vp->x + vp->width));
    bool is_y = (y >= vp->y && y < (vp->y + vp->height));
    return (is_x && is_y);
}
#endif /* HAVE_TOUCHSCREEN */

static void set_default_align_flags(struct viewport *vp)
{
    vp->flags &= ~VP_FLAG_ALIGNMENT_MASK;
#ifndef __PCTOOL__
    if (UNLIKELY(lang_is_rtl()))
        vp->flags |= VP_FLAG_ALIGN_RIGHT;
#endif
}

#endif /* HAVE_LCD_BITMAP */
#endif /* __PCTOOL__ */

#ifdef HAVE_LCD_COLOR
#define ARG_STRING(_depth) ((_depth) == 2 ? "dddddgg":"dddddcc")
#else
#define ARG_STRING(_depth) "dddddgg"
#endif


void viewport_set_fullscreen(struct viewport *vp,
                              const enum screen_type screen)
{
    vp->x = 0;
    vp->y = 0;
    vp->width = screens[screen].lcdwidth;
    vp->height = screens[screen].lcdheight;

#ifdef HAVE_LCD_BITMAP
    set_default_align_flags(vp);
    vp->font = FONT_UI; /* default to UI to discourage SYSFONT use */
    vp->drawmode = DRMODE_SOLID;
#if LCD_DEPTH > 1
#ifdef HAVE_REMOTE_LCD
    /* We only need this test if there is a remote LCD */
    if (screen == SCREEN_MAIN)
#endif
    {
        vp->fg_pattern = FG_FALLBACK;
        vp->bg_pattern = BG_FALLBACK;
#ifdef HAVE_LCD_COLOR
        vp->lss_pattern = global_settings.lss_color;
        vp->lse_pattern = global_settings.lse_color;
        vp->lst_pattern = global_settings.lst_color;
#endif
    }
#endif

#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
    if (screen == SCREEN_REMOTE)
    {
        vp->fg_pattern = LCD_REMOTE_DEFAULT_FG;
        vp->bg_pattern = LCD_REMOTE_DEFAULT_BG;
    }
#endif
#endif
}

void viewport_set_defaults(struct viewport *vp,
                            const enum screen_type screen)
{
    /* Reposition:
       1) If the "ui viewport" setting is set, and a sbs is loaded which specifies a %Vi
            return the intersection of those two viewports
       2) If only one of the "ui viewport" setting, or sbs %Vi is set
            return it
       3) No user viewports set
            return the full display
     */
#if defined(HAVE_LCD_BITMAP) && !defined(__PCTOOL__)
    
    struct viewport *sbs_area = NULL, *user_setting = NULL;
    if (!is_theme_enabled(screen))
   {
       viewport_set_fullscreen(vp, screen);
       return;
   }
    /* get the two viewports */
    if (custom_vp_loaded_ok[screen])
        user_setting = &custom_vp[screen];
    if (sb_skin_get_state(screen))    
        sbs_area = sb_skin_get_info_vp(screen);
        
    /* have both? get their intersection */
    if (sbs_area && user_setting)
    {
        struct viewport *a = sbs_area, *b = user_setting;
        /* if ui vp and info vp overlap, intersect */
        if (a->x             < b->x + b->width   &&
            a->x + a->width  > b->x              &&
            a->y             < b->y + b->height  &&
            a->y + a->height > b->y)
        {
            /* copy from ui vp first (for other field),fix coordinates after */
            *vp = *user_setting;
            set_default_align_flags(vp);
            vp->x = MAX(a->x, b->x);
            vp->y = MAX(a->y, b->y);
            vp->width = MIN(a->x + a->width, b->x + b->width) - vp->x;
            vp->height = MIN(a->y + a->height, b->y + b->height) - vp->y;
            return;
        }
        /* else (no overlap at all) fall back to info vp from sbs, that
         * has no redraw problems */
    }

    /* if only one is active use it
     * or if the above check for overlapping failed, use info vp then, because
     * that doesn't give redraw problems */
    if (sbs_area)
        *vp = *sbs_area;
    else if (user_setting)
        *vp = *user_setting;
    /* have neither so its fullscreen which was fixed at the beginning */   
    else  
#endif /* HAVE_LCD_BITMAP */
        viewport_set_fullscreen(vp, screen);  
}


#ifdef HAVE_LCD_BITMAP
const char* viewport_parse_viewport(struct viewport *vp,
                                    enum screen_type screen,
                                    const char *bufptr,
                                    const char separator)
{
    /* parse the list to the viewport struct */
    const char *ptr = bufptr;
    int depth;
    uint32_t set = 0;

    enum {
        PL_X = 0,
        PL_Y,
        PL_WIDTH,
        PL_HEIGHT,
        PL_FONT,
        PL_FG,
        PL_BG,
    };
    
    /* Work out the depth of this display */
    depth = screens[screen].depth;
#if (LCD_DEPTH == 1) || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH == 1)
    if (depth == 1)
    {
        if (!(ptr = parse_list("ddddd", &set, separator, ptr,
                    &vp->x, &vp->y, &vp->width, &vp->height, &vp->font)))
            return NULL;
    }
    else
#endif
#if (LCD_DEPTH > 1) || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)
    if (depth >= 2)
    {
        if (!(ptr = parse_list(ARG_STRING(depth), &set, separator, ptr,
                    &vp->x, &vp->y, &vp->width, &vp->height, &vp->font,
                    &vp->fg_pattern,&vp->bg_pattern)))
            return NULL;
    }
    else
#endif
    {}
#undef ARG_STRING

    /* X and Y *must* be set */
    if (!LIST_VALUE_PARSED(set, PL_X) || !LIST_VALUE_PARSED(set, PL_Y))
        return NULL;
    /* check for negative values */        
    if (vp->x < 0)
        vp->x += screens[screen].lcdwidth;
    if (vp->y < 0)
        vp->y += screens[screen].lcdheight;
        
    /* fix defaults, 
     * and negative width/height which means "extend to edge minus value */
    if (!LIST_VALUE_PARSED(set, PL_WIDTH))
        vp->width = screens[screen].lcdwidth - vp->x;
    else if (vp->width < 0)
        vp->width = (vp->width + screens[screen].lcdwidth) - vp->x;
    if (!LIST_VALUE_PARSED(set, PL_HEIGHT))
        vp->height = screens[screen].lcdheight - vp->y;
    else if (vp->height < 0)
        vp->height = (vp->height + screens[screen].lcdheight) - vp->y;

#if (LCD_DEPTH > 1) || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)
    if (!LIST_VALUE_PARSED(set, PL_FG))
        vp->fg_pattern = FG_FALLBACK;
    if (!LIST_VALUE_PARSED(set, PL_BG))
        vp->bg_pattern = BG_FALLBACK;
#endif /* LCD_DEPTH > 1 || LCD_REMOTE_DEPTH > 1 */

#ifdef HAVE_LCD_COLOR
    vp->lss_pattern = global_settings.lss_color;
    vp->lse_pattern = global_settings.lse_color;
    vp->lst_pattern = global_settings.lst_color;
#endif

    /* Validate the viewport dimensions - we know that the numbers are
       non-negative integers, ignore bars and assume the viewport takes them
       * into account */
    if ((vp->x >= screens[screen].lcdwidth) ||
        ((vp->x + vp->width) > screens[screen].lcdwidth) ||
        (vp->y >= screens[screen].lcdheight) ||
        ((vp->y + vp->height) > screens[screen].lcdheight))
    {
        return NULL;
    }

    /* Default to using the user font if the font was an invalid number or '-'*/
    if (((vp->font != FONT_SYSFIXED) && (vp->font != FONT_UI))
            || !LIST_VALUE_PARSED(set, PL_FONT)
            )
        vp->font = FONT_UI;

    /* Set the defaults for fields not user-specified */
    vp->drawmode = DRMODE_SOLID;
    set_default_align_flags(vp);

    return ptr;
}
#endif
