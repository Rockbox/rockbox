/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: skin_parser.c 26752 2010-06-10 21:22:16Z bieber $
 *
 * Copyright (C) 2010 Jonathan Gordon
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
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "strlcat.h"

#include "config.h"
#include "kernel.h"
#ifdef HAVE_ALBUMART
#include "albumart.h"
#endif
#include "skin_display.h"
#include "skin_engine.h"
#include "skin_parser.h"
#include "tag_table.h"
#include "skin_scan.h"
#if CONFIG_TUNER
#include "radio.h"
#endif
#include "viewport.h"
#include "cuesheet.h"
#include "language.h"
#include "playback.h"
#include "playlist.h"
#include "root_menu.h"
#include "misc.h"


#define MAX_LINE 1024

struct skin_draw_info {
    struct gui_wps *gwps;
    struct skin_viewport *skin_vp;
    int line_number;
    unsigned long refresh_type;
    
    char* cur_align_start;
    struct align_pos align;
    bool no_line_break;
    bool line_scrolls;
    bool force_redraw;
    
    char *buf;
    size_t buf_size;
    
    int offset; /* used by the playlist viewer */
};

typedef bool (*skin_render_func)(struct skin_element* alternator, struct skin_draw_info *info);
bool skin_render_alternator(struct skin_element* alternator, struct skin_draw_info *info);

#ifdef HAVE_LCD_BITMAP
static void skin_render_playlistviewer(struct playlistviewer* viewer,
                                       struct gui_wps *gwps,
                                       struct skin_viewport* skin_viewport,
                                       unsigned long refresh_type);
#endif

static bool do_non_text_tags(struct gui_wps *gwps, struct skin_draw_info *info,
                             struct skin_element *element, struct viewport* vp)
{
#ifndef HAVE_LCD_BITMAP
    (void)vp; /* silence warnings */
    (void)info;
#endif    
    struct wps_token *token = (struct wps_token *)element->data;

#ifdef HAVE_LCD_BITMAP
    struct wps_data *data = gwps->data;
    bool do_refresh = (element->tag->flags & info->refresh_type) > 0;
#endif
    switch (token->type)
    {   
#if (LCD_DEPTH > 1) || (defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1))
        case SKIN_TOKEN_VIEWPORT_FGCOLOUR:
        {
            struct viewport_colour *col = token->value.data;
            col->vp->fg_pattern = col->colour;
        }
        break;
        case SKIN_TOKEN_VIEWPORT_BGCOLOUR:
        {
            struct viewport_colour *col = token->value.data;
            col->vp->bg_pattern = col->colour;
        }
        break;
#endif
        case SKIN_TOKEN_VIEWPORT_ENABLE:
        {
            char *label = token->value.data;
            char temp = VP_DRAW_HIDEABLE;
            struct skin_element *viewport = gwps->data->tree;
            while (viewport)
            {
                struct skin_viewport *skinvp = (struct skin_viewport*)viewport->data;
                if (skinvp->label && !skinvp->is_infovp &&
                    !strcmp(skinvp->label, label))
                {
                    if (skinvp->hidden_flags&VP_DRAW_HIDDEN)
                    {
                        temp |= VP_DRAW_WASHIDDEN;
                    }    
                    skinvp->hidden_flags = temp;
                }
                viewport = viewport->next;
            }
        }
        break;
#ifdef HAVE_LCD_BITMAP
        case SKIN_TOKEN_UIVIEWPORT_ENABLE:
            sb_set_info_vp(gwps->display->screen_type, 
                           token->value.data);
            break;
        case SKIN_TOKEN_PEAKMETER:
            data->peak_meter_enabled = true;
            if (do_refresh)
                draw_peakmeters(gwps, info->line_number, vp);
            break;
#endif
#ifdef HAVE_LCD_BITMAP
        case SKIN_TOKEN_PEAKMETER_LEFTBAR:
        case SKIN_TOKEN_PEAKMETER_RIGHTBAR:
            data->peak_meter_enabled = true;
            /* fall through to the progressbar code */
#endif
        case SKIN_TOKEN_VOLUMEBAR:
        case SKIN_TOKEN_BATTERY_PERCENTBAR:
#ifdef HAVE_LCD_BITMAP
        case SKIN_TOKEN_PROGRESSBAR:
        case SKIN_TOKEN_TUNER_RSSI_BAR:
        {
            struct progressbar *bar = (struct progressbar*)token->value.data;
            if (do_refresh)
                draw_progressbar(gwps, info->line_number, bar);
        }
#endif
        break;
#ifdef HAVE_LCD_BITMAP
        case SKIN_TOKEN_IMAGE_DISPLAY_LISTICON:
        case SKIN_TOKEN_IMAGE_PRELOAD_DISPLAY:
        {
            struct image_display *id = token->value.data;
            const char* label = id->label;
            struct gui_img *img = skin_find_item(label,SKIN_FIND_IMAGE, data);
            if (img && img->loaded)
            {
                if (id->token == NULL)
                {
                    img->display = id->subimage;
                }
                else
                {
                    char buf[16];
                    const char *out;
                    int a = img->num_subimages;
                    out = get_token_value(gwps, id->token, info->offset, 
                                          buf, sizeof(buf), &a);

                    /* NOTE: get_token_value() returns values starting at 1! */
                    if (a == -1)
                        a = (out && *out) ? 1 : 2;
                    if (token->type == SKIN_TOKEN_IMAGE_DISPLAY_LISTICON)
                        a -= 2; /* 2 is added in statusbar-skinned.c! */
                    else
                        a--;
                    a += id->offset;

                    /* Clear the image, as in conditionals */
                    clear_image_pos(gwps, img);

                    /* If the token returned a value which is higher than
                     * the amount of subimages, don't draw it. */
                    if (a >= 0 && a < img->num_subimages)
                    {
                        img->display = a;
                    }
                }
            }
            break;
        }
#ifdef HAVE_ALBUMART
        case SKIN_TOKEN_ALBUMART_DISPLAY:
            /* now draw the AA */
            if (do_refresh && data->albumart)
            {
                int handle = playback_current_aa_hid(data->playback_aa_slot);
#if CONFIG_TUNER
                if (in_radio_screen() || (get_radio_status() != FMRADIO_OFF))
                {
                    struct dim dim = {data->albumart->width, data->albumart->height};
                    handle = radio_get_art_hid(&dim);
                }
#endif
                data->albumart->draw_handle = handle;
            }
            break;
#endif
        case SKIN_TOKEN_DRAW_INBUILTBAR:
            gui_statusbar_draw(&(statusbars.statusbars[gwps->display->screen_type]),
                               info->refresh_type == SKIN_REFRESH_ALL,
                               token->value.data);
            break;
        case SKIN_TOKEN_VIEWPORT_CUSTOMLIST:
            if (do_refresh)
                skin_render_playlistviewer(token->value.data, gwps,
                                           info->skin_vp, info->refresh_type);
            break;
        
#endif /* HAVE_LCD_BITMAP */
#ifdef HAVE_SKIN_VARIABLES
        case SKIN_TOKEN_VAR_SET:
            if (do_refresh)
            {
                struct skin_var_changer *data = token->value.data;
                if (data->direct)
                    data->var->value = data->newval;
                else
                {
                    data->var->value += data->newval;
                    if (data->max)
                    {
                        if (data->var->value > data->max)
                            data->var->value = 1;
                        else if (data->var->value < 1)
                            data->var->value = data->max;
                    }
                }
                if (data->var->value < 1)
                    data->var->value = 1;
                data->var->last_changed = current_tick;
            }
            break;
#endif
        default:
            return false;
    }
    return true;
}
                


static void do_tags_in_hidden_conditional(struct skin_element* branch,
                                          struct skin_draw_info *info)
{
#ifdef HAVE_LCD_BITMAP
    struct gui_wps *gwps = info->gwps;
    struct wps_data *data = gwps->data;
#endif    
    /* Tags here are ones which need to be "turned off" or cleared 
     * if they are in a conditional branch which isnt being used */
    if (branch->type == LINE_ALTERNATOR)
    {
        int i;
        for (i=0; i<branch->children_count; i++)
        {
            do_tags_in_hidden_conditional(branch->children[i], info);
        }
    }
    else if (branch->type == LINE && branch->children_count)
    {
        struct skin_element *child = branch->children[0];
        struct wps_token *token;
        while (child)
        {
            if (child->type == CONDITIONAL)
            {
                int i;
                for (i=0; i<child->children_count; i++)
                {
                    do_tags_in_hidden_conditional(child->children[i], info);
                }
                child = child->next;
                continue;
            }
            else if (child->type != TAG || !child->data)
            {
                child = child->next;
                continue;
            }
            token = (struct wps_token *)child->data;
#ifdef HAVE_LCD_BITMAP
            /* clear all pictures in the conditional and nested ones */
            if (token->type == SKIN_TOKEN_IMAGE_PRELOAD_DISPLAY)
            {
                struct image_display *id = token->value.data;
                struct gui_img *img = skin_find_item(id->label, 
                                                     SKIN_FIND_IMAGE, data);
                clear_image_pos(gwps, img);
            }
            else if (token->type == SKIN_TOKEN_PEAKMETER)
            {
                data->peak_meter_enabled = false;
            }
            else if (token->type == SKIN_TOKEN_VIEWPORT_ENABLE)
            {
                char *label = token->value.data;
                struct skin_element *viewport;
                for (viewport = data->tree;
                     viewport;
                     viewport = viewport->next)
                {
                    struct skin_viewport *skin_viewport = (struct skin_viewport*)viewport->data;
                    if (skin_viewport->label && strcmp(skin_viewport->label, label))
                        continue;
                    if (skin_viewport->hidden_flags&VP_NEVER_VISIBLE)
                    {
                        continue;
                    }
                    if (skin_viewport->hidden_flags&VP_DRAW_HIDEABLE)
                    {
                        if (skin_viewport->hidden_flags&VP_DRAW_HIDDEN)
                            skin_viewport->hidden_flags |= VP_DRAW_WASHIDDEN;
                        else
                        {
                            gwps->display->set_viewport(&skin_viewport->vp);
                            gwps->display->clear_viewport();
                            gwps->display->scroll_stop(&skin_viewport->vp);
                            gwps->display->set_viewport(&info->skin_vp->vp);
                            skin_viewport->hidden_flags |= VP_DRAW_HIDDEN;
                        }
                    }
                }
            }
#endif
#ifdef HAVE_ALBUMART
            else if (data->albumart && token->type == SKIN_TOKEN_ALBUMART_DISPLAY)
            {
                draw_album_art(gwps,
                        playback_current_aa_hid(data->playback_aa_slot), true);
            }
#endif
            child = child->next;
        }
    }
}
    
static void fix_line_alignment(struct skin_draw_info *info, struct skin_element *element)
{
    struct align_pos *align = &info->align;
    char *cur_pos = info->cur_align_start + strlen(info->cur_align_start);
    switch (element->tag->type)
    {
        case SKIN_TOKEN_ALIGN_LEFT:
            *cur_pos = '\0'; cur_pos++; *cur_pos = '\0';
            align->left = cur_pos;
            info->cur_align_start = cur_pos;
            break;
        case SKIN_TOKEN_ALIGN_LEFT_RTL:
            *cur_pos = '\0'; cur_pos++; *cur_pos = '\0';
            if (lang_is_rtl())
                align->right = cur_pos;
            else
                align->left = cur_pos;
            info->cur_align_start = cur_pos;
            break;
        case SKIN_TOKEN_ALIGN_CENTER:
            *cur_pos = '\0'; cur_pos++; *cur_pos = '\0';
            align->center = cur_pos;
            info->cur_align_start = cur_pos;
            break;
        case SKIN_TOKEN_ALIGN_RIGHT:
            *cur_pos = '\0'; cur_pos++; *cur_pos = '\0';
            align->right = cur_pos;
            info->cur_align_start = cur_pos;
            break;
        case SKIN_TOKEN_ALIGN_RIGHT_RTL:
            *cur_pos = '\0'; cur_pos++; *cur_pos = '\0';
            if (lang_is_rtl())
                align->left = cur_pos;
            else
                align->right = cur_pos;
            info->cur_align_start = cur_pos;
            break;
        default:
            break;
    }
}
    
/* Draw a LINE element onto the display */
static bool skin_render_line(struct skin_element* line, struct skin_draw_info *info)
{
    bool needs_update = false;
    int last_value, value;
    
    if (line->children_count == 0)
        return false; /* empty line, do nothing */
        
    struct skin_element *child = line->children[0];
    struct conditional *conditional;
    skin_render_func func = skin_render_line;
    int old_refresh_mode = info->refresh_type;
    while (child)
    {
        switch (child->type)
        {
            case CONDITIONAL:
                conditional = (struct conditional*)child->data;
                last_value = conditional->last_value;
                value = evaluate_conditional(info->gwps, info->offset, 
                                             conditional, child->children_count);
                conditional->last_value = value;
                if (child->children_count == 1)
                {
                    /* special handling so 
                     * %?aa<true> and %?<true|false> need special handlng here */
                    
                    if (value == -1) /* tag is false */
                    {
                        /* we are in a false branch of a %?aa<true> conditional */
                        if (last_value == 0)
                            do_tags_in_hidden_conditional(child->children[0], info);
                        break;
                    }
                }
                else
                {
                    if (last_value >= 0 && value != last_value && last_value < child->children_count)
                        do_tags_in_hidden_conditional(child->children[last_value], info);
                }
                if (child->children[value]->type == LINE_ALTERNATOR)
                {
                    func = skin_render_alternator;
                }
                else if (child->children[value]->type == LINE)
                    func = skin_render_line;
                
                if (value != last_value)
                {
                    info->refresh_type = SKIN_REFRESH_ALL;
                    info->force_redraw = true;
                }
                    
                if (func(child->children[value], info))
                    needs_update = true;
                else
                    needs_update = needs_update || (last_value != value);
                    
                info->refresh_type = old_refresh_mode;
                break;
            case TAG:
                if (child->tag->flags & NOBREAK)
                    info->no_line_break = true;
                if (child->tag->type == SKIN_TOKEN_SUBLINE_SCROLL)
                    info->line_scrolls = true;
                
                fix_line_alignment(info, child);
                
                if (!child->data)
                {
                    break;
                }
                if (!do_non_text_tags(info->gwps, info, child, &info->skin_vp->vp))
                {
                    static char tempbuf[128];
                    const char *value = get_token_value(info->gwps, child->data,
                                                        info->offset, tempbuf,
                                                        sizeof(tempbuf), NULL);
                    if (value)
                    {
#if CONFIG_RTC
                        if (child->tag->flags&SKIN_RTC_REFRESH)
                            needs_update = needs_update || info->refresh_type&SKIN_REFRESH_DYNAMIC;
#endif
                        needs_update = needs_update || 
                                ((child->tag->flags&info->refresh_type)!=0);
                        strlcat(info->cur_align_start, value, 
                                info->buf_size - (info->cur_align_start-info->buf));
                    }
                }
                break;
            case TEXT:
                strlcat(info->cur_align_start, child->data, 
                        info->buf_size - (info->cur_align_start-info->buf));
                needs_update = needs_update || 
                                (info->refresh_type&SKIN_REFRESH_STATIC) != 0;
                break;
            case COMMENT:
            default:
                break;
        }

        child = child->next;
    }
    return needs_update;
}

static int get_subline_timeout(struct gui_wps *gwps, struct skin_element* line)
{
    struct skin_element *element=line;
    struct wps_token *token;
    int retval = DEFAULT_SUBLINE_TIME_MULTIPLIER*TIMEOUT_UNIT;
    if (element->type == LINE)
    {
        if (element->children_count == 0)
            return retval; /* empty line, so force redraw */
        element = element->children[0];
    }
    while (element)
    {
        if (element->type == TAG &&
            element->tag->type == SKIN_TOKEN_SUBLINE_TIMEOUT )
        {
            token = element->data;
            return token->value.i;
        }
        else if (element->type == CONDITIONAL)
        {
            struct conditional *conditional = element->data;
            int val = evaluate_conditional(gwps, 0, conditional,
                                           element->children_count);
            if (val >= 0)
            {
                retval = get_subline_timeout(gwps, element->children[val]);
                if (retval >= 0)
                    return retval;
            }
        }
        element = element->next;
    }
    return retval;
}

bool skin_render_alternator(struct skin_element* element, struct skin_draw_info *info)
{
    bool changed_lines = false;
    struct line_alternator *alternator = (struct line_alternator*)element->data;
    unsigned old_refresh = info->refresh_type;
    if (info->refresh_type == SKIN_REFRESH_ALL)
    {
        alternator->current_line = element->children_count-1;
        changed_lines = true;
    }
    else if (TIME_AFTER(current_tick, alternator->next_change_tick))
    {
        changed_lines = true;
    }

    if (changed_lines)
    {
        struct skin_element *current_line;
        int start = alternator->current_line;
        int try_line = start;
        bool suitable = false;
        int rettimeout = DEFAULT_SUBLINE_TIME_MULTIPLIER*TIMEOUT_UNIT;
        
        /* find a subline which has at least one token in it,
         * and that line doesnt have a timeout set to 0 through conditionals */
        do {
            try_line++;
            if (try_line >= element->children_count)
                try_line = 0;
            if (element->children[try_line]->children_count != 0)
            {
                current_line = element->children[try_line];
                rettimeout = get_subline_timeout(info->gwps, 
                                                 current_line->children[0]);
                if (rettimeout > 0)
                {
                    suitable = true;
                }
            }
        }
        while (try_line != start && !suitable);
        
        if (suitable)
        {
            alternator->current_line = try_line;
            alternator->next_change_tick = current_tick + rettimeout;
        }

        info->refresh_type = SKIN_REFRESH_ALL;
        info->force_redraw = true;
    }
    bool ret = skin_render_line(element->children[alternator->current_line], info);
    info->refresh_type = old_refresh;
    return changed_lines || ret;
}

static void skin_render_viewport(struct skin_element* viewport, struct gui_wps *gwps,
                                 struct skin_viewport* skin_viewport, unsigned long refresh_type)
{
    struct screen *display = gwps->display;
    char linebuf[MAX_LINE];
    skin_render_func func = skin_render_line;
    struct skin_element* line = viewport;
    struct skin_draw_info info = {
        .gwps = gwps,
        .buf = linebuf,
        .buf_size = sizeof(linebuf),
        .line_number = 0,
        .no_line_break = false,
        .line_scrolls = false,
        .refresh_type = refresh_type,
        .skin_vp = skin_viewport,
        .offset = 0
    };
    
    struct align_pos * align = &info.align;
    bool needs_update;
#ifdef HAVE_LCD_BITMAP
    /* Set images to not to be displayed */
    struct skin_token_list *imglist = gwps->data->images;
    while (imglist)
    {
        struct gui_img *img = (struct gui_img *)imglist->token->value.data;
        img->display = -1;
        imglist = imglist->next;
    }
#endif
    
    while (line)
    {
        linebuf[0] = '\0';
        info.no_line_break = false;
        info.line_scrolls = false;
        info.force_redraw = false;
    
        info.cur_align_start = info.buf;
        align->left = info.buf;
        align->center = NULL;
        align->right = NULL;
        
        
        if (line->type == LINE_ALTERNATOR)
            func = skin_render_alternator;
        else if (line->type == LINE)
            func = skin_render_line;
        
        needs_update = func(line, &info);
#if (LCD_DEPTH > 1) || (defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1))
        if (skin_viewport->vp.fg_pattern != skin_viewport->start_fgcolour ||
            skin_viewport->vp.bg_pattern != skin_viewport->start_bgcolour)
        {
            /* 2bit lcd drivers need lcd_set_viewport() to be called to change
             * the colour, 16bit doesnt. But doing this makes static text
             * get the new colour also */
            needs_update = true;
            display->set_viewport(&skin_viewport->vp);
        }
#endif
        /* only update if the line needs to be, and there is something to write */
        if (refresh_type && needs_update)
        {
            if (info.line_scrolls)
            {
                /* if the line is a scrolling one we don't want to update
                   too often, so that it has the time to scroll */
                if ((refresh_type & SKIN_REFRESH_SCROLL) || info.force_redraw)
                    write_line(display, align, info.line_number, true);
            }
            else
                write_line(display, align, info.line_number, false);
        }
        if (!info.no_line_break)
            info.line_number++;
        line = line->next;
    }
#ifdef HAVE_LCD_BITMAP
    wps_display_images(gwps, &skin_viewport->vp);
#endif
}

void skin_render(struct gui_wps *gwps, unsigned refresh_mode)
{
    struct wps_data *data = gwps->data;
    struct screen *display = gwps->display;
    
    struct skin_element* viewport;
    struct skin_viewport* skin_viewport;
    
    int old_refresh_mode = refresh_mode;
    
#ifdef HAVE_LCD_CHARCELLS
    int i;
    for (i = 0; i < 8; i++)
    {
        if (data->wps_progress_pat[i] == 0)
            data->wps_progress_pat[i] = display->get_locked_pattern();
    }
#endif
    viewport = data->tree;
    skin_viewport = (struct skin_viewport *)viewport->data;
    if (skin_viewport->label && viewport->next &&
        !strcmp(skin_viewport->label,VP_DEFAULT_LABEL))
        refresh_mode = 0;
    
    for (viewport = data->tree;
         viewport;
         viewport = viewport->next)
    {
        /* SETUP */
        skin_viewport = (struct skin_viewport*)viewport->data;
        unsigned vp_refresh_mode = refresh_mode;
#if (LCD_DEPTH > 1) || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)
        skin_viewport->vp.fg_pattern = skin_viewport->start_fgcolour;
        skin_viewport->vp.bg_pattern = skin_viewport->start_bgcolour;
#endif
        
        /* dont redraw the viewport if its disabled */
        if (skin_viewport->hidden_flags&VP_NEVER_VISIBLE)
        {   /* don't draw anything into this one */
            vp_refresh_mode = 0;
        }
        else if ((skin_viewport->hidden_flags&VP_DRAW_HIDDEN))
        {
            skin_viewport->hidden_flags |= VP_DRAW_WASHIDDEN;
            continue;
        }
        else if (((skin_viewport->hidden_flags&
                   (VP_DRAW_WASHIDDEN|VP_DRAW_HIDEABLE))
                    == (VP_DRAW_WASHIDDEN|VP_DRAW_HIDEABLE)))
        {
            vp_refresh_mode = SKIN_REFRESH_ALL;
            skin_viewport->hidden_flags = VP_DRAW_HIDEABLE;
        }
        
        display->set_viewport(&skin_viewport->vp);
        if ((vp_refresh_mode&SKIN_REFRESH_ALL) == SKIN_REFRESH_ALL)
        {
            display->clear_viewport();
        }
        /* render */
        if (viewport->children_count)
            skin_render_viewport(viewport->children[0], gwps,
                                 skin_viewport, vp_refresh_mode);
        refresh_mode = old_refresh_mode;
    }
    
    /* Restore the default viewport */
    display->set_viewport(NULL);
    display->update();
}

#ifdef HAVE_LCD_BITMAP
static __attribute__((noinline)) void skin_render_playlistviewer(struct playlistviewer* viewer,
                                       struct gui_wps *gwps,
                                       struct skin_viewport* skin_viewport,
                                       unsigned long refresh_type)
{
    struct screen *display = gwps->display;
    char linebuf[MAX_LINE];
    skin_render_func func = skin_render_line;
    struct skin_element* line;
    struct skin_draw_info info = {
        .gwps = gwps,
        .buf = linebuf,
        .buf_size = sizeof(linebuf),
        .line_number = 0,
        .no_line_break = false,
        .line_scrolls = false,
        .refresh_type = refresh_type,
        .skin_vp = skin_viewport,
        .offset = viewer->start_offset
    };
    
    struct align_pos * align = &info.align;
    bool needs_update;
    int cur_pos, start_item, max;
    int nb_lines = viewport_get_nb_lines(viewer->vp);
#if CONFIG_TUNER
    if (get_current_activity() == ACTIVITY_FM)
    {
        cur_pos = radio_current_preset();
        start_item = cur_pos + viewer->start_offset;
        max = start_item+radio_preset_count();
    }
    else
#endif
    {
        struct cuesheet *cue = skin_get_global_state()->id3 ? 
                               skin_get_global_state()->id3->cuesheet : NULL;
        cur_pos = playlist_get_display_index();
        max = playlist_amount()+1;
        if (cue)
            max += cue->track_count;
        start_item = MAX(0, cur_pos + viewer->start_offset); 
    }
    if (max-start_item > nb_lines)
        max = start_item + nb_lines;
    
    line = viewer->line;
    while (start_item < max)
    {
        linebuf[0] = '\0';
        info.no_line_break = false;
        info.line_scrolls = false;
        info.force_redraw = false;
    
        info.cur_align_start = info.buf;
        align->left = info.buf;
        align->center = NULL;
        align->right = NULL;
        
        
        if (line->type == LINE_ALTERNATOR)
            func = skin_render_alternator;
        else if (line->type == LINE)
            func = skin_render_line;
        
        needs_update = func(line, &info);
        
        /* only update if the line needs to be, and there is something to write */
        if (refresh_type && needs_update)
        {
            if (info.line_scrolls)
            {
                /* if the line is a scrolling one we don't want to update
                   too often, so that it has the time to scroll */
                if ((refresh_type & SKIN_REFRESH_SCROLL) || info.force_redraw)
                    write_line(display, align, info.line_number, true);
            }
            else
                write_line(display, align, info.line_number, false);
        }
        info.line_number++;
        info.offset++;
        start_item++;
    }
}
#endif
