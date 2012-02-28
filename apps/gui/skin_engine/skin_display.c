/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002-2007 Björn Stenberg
 * Copyright (C) 2007-2008 Nicolas Pennequin
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
#include "string-extra.h"
#include "core_alloc.h"
#include "misc.h"
#include "font.h"
#include "system.h"
#include "rbunicode.h"
#include "sound.h"
#include "powermgmt.h"
#ifdef DEBUG
#include "debug.h"
#endif
#include "action.h"
#include "abrepeat.h"
#include "lang.h"
#include "language.h"
#include "statusbar.h"
#include "settings.h"
#include "scrollbar.h"
#include "screen_access.h"
#include "playlist.h"
#include "audio.h"
#include "tagcache.h"
#include "list.h"

#ifdef HAVE_LCD_BITMAP
#include "peakmeter.h"
/* Image stuff */
#include "bmp.h"
#ifdef HAVE_ALBUMART
#include "albumart.h"
#endif
#endif

#include "cuesheet.h"
#if CONFIG_CODEC == SWCODEC
#include "playback.h"
#endif
#include "backdrop.h"
#include "viewport.h"
#if CONFIG_TUNER
#include "radio.h"
#include "tuner.h"
#endif
#include "root_menu.h"


#include "wps_internals.h"
#include "skin_engine.h"
#include "statusbar-skinned.h"
#include "skin_display.h"

void skin_render(struct gui_wps *gwps, unsigned refresh_mode);

/* update a skinned screen, update_type is WPS_REFRESH_* values.
 * Usually it should only be WPS_REFRESH_NON_STATIC
 * A full update will be done if required (skin_do_full_update() == true)
 */
void skin_update(enum skinnable_screens skin, enum screen_type screen,
                 unsigned int update_type)
{
    struct gui_wps *gwps = skin_get_gwps(skin, screen);
    /* This maybe shouldnt be here, 
     * This is also safe for skined screen which dont use the id3 */
    struct mp3entry *id3 = skin_get_global_state()->id3;
    bool cuesheet_update = (id3 != NULL ? cuesheet_subtrack_changed(id3) : false);
    if (cuesheet_update)
        skin_request_full_update(skin);
 
    skin_render(gwps, skin_do_full_update(skin, screen) ? 
                        SKIN_REFRESH_ALL : update_type);
}

#ifdef HAVE_LCD_BITMAP

void draw_progressbar(struct gui_wps *gwps, int line, struct progressbar *pb)
{
    struct screen *display = gwps->display;
    struct viewport *vp = SKINOFFSETTOPTR(get_skin_buffer(gwps->data), pb->vp);
    struct wps_state *state = skin_get_global_state();
    struct mp3entry *id3 = state->id3;
    int x = pb->x, y = pb->y, width = pb->width, height = pb->height;
    unsigned long length, end;
    int flags = HORIZONTAL;
    
    if (height < 0)
        height = font_get(vp->font)->height;

    if (y < 0)
    {
        int line_height = font_get(vp->font)->height;
        /* center the pb in the line, but only if the line is higher than the pb */
        int center = (line_height-height)/2;
        /* if Y was not set calculate by font height,Y is -line_number-1 */
        y = line*line_height + (0 > center ? 0 : center);
    }

    if (pb->type == SKIN_TOKEN_VOLUMEBAR)
    {
        int minvol = sound_min(SOUND_VOLUME);
        int maxvol = sound_max(SOUND_VOLUME);
        length = maxvol-minvol;
        end = global_settings.volume-minvol;
    }
    else if (pb->type == SKIN_TOKEN_BATTERY_PERCENTBAR)
    {
        length = 100;
        end = battery_level();
    }
    else if (pb->type == SKIN_TOKEN_PEAKMETER_LEFTBAR ||
             pb->type == SKIN_TOKEN_PEAKMETER_RIGHTBAR)
    {
        int left, right, val;
        peak_meter_current_vals(&left, &right);
        val = pb->type == SKIN_TOKEN_PEAKMETER_LEFTBAR ? left : right;
        length = MAX_PEAK;
        end = peak_meter_scale_value(val, length);
    }
    else if (pb->type == SKIN_TOKEN_LIST_SCROLLBAR)
    {
        int val, min, max;
        skinlist_get_scrollbar(&val, &min, &max);
        end = val - min;
        length = max - min;
    }
#if CONFIG_TUNER
    else if (in_radio_screen() || (get_radio_status() != FMRADIO_OFF))
    {
#ifdef HAVE_RADIO_RSSI
        if (pb->type == SKIN_TOKEN_TUNER_RSSI_BAR)
        {
            int val = tuner_get(RADIO_RSSI);
            int min = tuner_get(RADIO_RSSI_MIN);
            int max = tuner_get(RADIO_RSSI_MAX);
            end = val - min;
            length = max - min;
        }
        else
#endif
        {
            int min = fm_region_data[global_settings.fm_region].freq_min;
            end = radio_current_frequency() - min;
            length = fm_region_data[global_settings.fm_region].freq_max - min;
        }
    }
#endif
    else if (id3 && id3->length)
    {
        length = id3->length;
        end = id3->elapsed + state->ff_rewind_count;
    }
    else
    {
        length = 1;
        end = 0;
    }
    
    if (!pb->horizontal)
    {
        /* we want to fill upwards which is technically inverted. */
        flags = INVERTFILL;
    }
    
    if (pb->invert_fill_direction)
    {
        flags ^= INVERTFILL;
    }

    if (pb->nofill)
    {
        flags |= INNER_NOFILL;
    }

    if (SKINOFFSETTOPTR(get_skin_buffer(gwps->data), pb->slider))
    {
        struct gui_img *img = SKINOFFSETTOPTR(get_skin_buffer(gwps->data), pb->slider);
        /* clear the slider */
        screen_clear_area(display, x, y, width, height);

        /* shrink the bar so the slider is inside bounds */
        if (flags&HORIZONTAL)
        {
            width -= img->bm.width;
            x += img->bm.width / 2;
        }
        else
        {
            height -= img->bm.height;
            y += img->bm.height / 2;
        }
    }
    
    if (SKINOFFSETTOPTR(get_skin_buffer(gwps->data), pb->backdrop))
    {
        struct gui_img *img = SKINOFFSETTOPTR(get_skin_buffer(gwps->data), pb->backdrop);
        img->bm.data = core_get_data(img->buflib_handle);
        display->bmp_part(&img->bm, 0, 0, x, y, width, height);
        flags |= DONT_CLEAR_EXCESS;
    }
    
    if (!pb->nobar)
    {
        struct gui_img *img = SKINOFFSETTOPTR(get_skin_buffer(gwps->data), pb->image);
        if (img)
        {
            char *img_data = core_get_data(img->buflib_handle);
            img->bm.data = img_data;
            gui_bitmap_scrollbar_draw(display, &img->bm,
                                    x, y, width, height,
                                    length, 0, end, flags);
        }
        else
            gui_scrollbar_draw(display, x, y, width, height,
                               length, 0, end, flags);
    }

    if (SKINOFFSETTOPTR(get_skin_buffer(gwps->data), pb->slider))
    {
        int xoff = 0, yoff = 0;
        int w = width, h = height;
        struct gui_img *img = SKINOFFSETTOPTR(get_skin_buffer(gwps->data), pb->slider);
        img->bm.data = core_get_data(img->buflib_handle);

        if (flags&HORIZONTAL)
        {
            w = img->bm.width;
            xoff = width * end / length;
            if (flags&INVERTFILL)
                xoff = width - xoff;
            xoff -= w / 2;
        }
        else
        {
            h = img->bm.height;
            yoff = height * end / length;
            if (flags&INVERTFILL)
                yoff = height - yoff;
            yoff -= h / 2;
        }
        display->bmp_part(&img->bm, 0, 0, x + xoff, y + yoff, w, h);
    }

    if (pb->type == SKIN_TOKEN_PROGRESSBAR)
    {
        if (id3 && id3->length)
        {
#ifdef AB_REPEAT_ENABLE
            if (ab_repeat_mode_enabled())
                ab_draw_markers(display, id3->length, x, y, width, height);
#endif

            if (id3->cuesheet)
                cue_draw_markers(display, id3->cuesheet, id3->length,
                                 x, y+1, width, height-2);
        }
#if 0 /* disable for now CONFIG_TUNER */
        else if (in_radio_screen() || (get_radio_status() != FMRADIO_OFF))
        {
            presets_draw_markers(display, x, y, width, height);
        }
#endif
    }
}

/* clears the area where the image was shown */
void clear_image_pos(struct gui_wps *gwps, struct gui_img *img)
{
    if(!gwps)
        return;
    gwps->display->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    gwps->display->fillrect(img->x, img->y, img->bm.width, img->subimage_height);
    gwps->display->set_drawmode(DRMODE_SOLID);
}

void wps_draw_image(struct gui_wps *gwps, struct gui_img *img, int subimage)
{
    struct screen *display = gwps->display;
    img->bm.data = core_get_data(img->buflib_handle);
    display->set_drawmode(DRMODE_SOLID);

    display->bmp_part(&img->bm, 0, img->subimage_height * subimage,
                      img->x, img->y, img->bm.width, img->subimage_height);
}


void wps_display_images(struct gui_wps *gwps, struct viewport* vp)
{
    if(!gwps || !gwps->data || !gwps->display)
        return;
    (void)vp;
    struct wps_data *data = gwps->data;
    struct screen *display = gwps->display;
    struct skin_token_list *list = SKINOFFSETTOPTR(get_skin_buffer(data), data->images);

    while (list)
    {
        struct wps_token *token = SKINOFFSETTOPTR(get_skin_buffer(data), list->token);
        struct gui_img *img = (struct gui_img*)SKINOFFSETTOPTR(get_skin_buffer(data), token->value.data);
        if (img->using_preloaded_icons && img->display >= 0)
        {
            screen_put_icon(display, img->x, img->y, img->display);
        }
        else if (img->loaded)
        {
            if (img->display >= 0)
            {
                wps_draw_image(gwps, img, img->display);
            }
        }
        list = SKINOFFSETTOPTR(get_skin_buffer(data), list->next);
    }
#ifdef HAVE_ALBUMART
    /* now draw the AA */
    struct skin_albumart *aa = SKINOFFSETTOPTR(get_skin_buffer(data), data->albumart);
    if (aa && SKINOFFSETTOPTR(get_skin_buffer(data), aa->vp) == vp
        && aa->draw_handle >= 0)
    {
        draw_album_art(gwps, aa->draw_handle, false);
        aa->draw_handle = -1;
    }
#endif

    display->set_drawmode(DRMODE_SOLID);
}

#endif /* HAVE_LCD_BITMAP */

/* Evaluate the conditional that is at *token_index and return whether a skip
   has ocurred. *token_index is updated with the new position.
*/
int evaluate_conditional(struct gui_wps *gwps, int offset, 
                         struct conditional *conditional, int num_options)
{
    if (!gwps)
        return false;

    char result[128];
    const char *value;

    int intval = num_options < 2 ? 2 : num_options;
    /* get_token_value needs to know the number of options in the enum */
    value = get_token_value(gwps, SKINOFFSETTOPTR(get_skin_buffer(gwps->data), conditional->token),
                    offset, result, sizeof(result), &intval);

    /* intval is now the number of the enum option we want to read,
       starting from 1. If intval is -1, we check if value is empty. */
    if (intval == -1)
    {
        if (num_options == 1) /* so %?AA<true> */
            intval = (value && *value) ? 1 : 0; /* returned as 0 for true, -1 for false */
        else
            intval = (value && *value) ? 1 : num_options;
    }
    else if (intval > num_options || intval < 1)
        intval = num_options;

    return intval -1;
}


/* Display a line appropriately according to its alignment format.
   format_align contains the text, separated between left, center and right.
   line is the index of the line on the screen.
   scroll indicates whether the line is a scrolling one or not.
*/
void write_line(struct screen *display, struct align_pos *format_align,
                int line, bool scroll, unsigned style)
{
#ifndef HAVE_LCD_BITMAP
    (void)style;
#endif
    int left_width = 0;
    int center_width = 0, center_xpos;
    int right_width = 0,  right_xpos;
    int space_width;
    int string_height;
    int scroll_width;
    int viewport_width = display->getwidth();

    /* calculate different string sizes and positions */
    display->getstringsize((unsigned char *)" ", &space_width, &string_height);
    if (format_align->left != 0) {
        display->getstringsize((unsigned char *)format_align->left,
                                &left_width, &string_height);
    }

    if (format_align->right != 0) {
        display->getstringsize((unsigned char *)format_align->right,
                                &right_width, &string_height);
    }

    if (format_align->center != 0) {
        display->getstringsize((unsigned char *)format_align->center,
                                &center_width, &string_height);
    }

    right_xpos = (viewport_width - right_width);
    center_xpos = (viewport_width - center_width) / 2;

    scroll_width = viewport_width;

    /* Checks for overlapping strings.
        If needed the overlapping strings will be merged, separated by a
        space */

    /* CASE 1: left and centered string overlap */
    /* there is a left string, need to merge left and center */
    if ((left_width != 0 && center_width != 0) &&
        (left_width + space_width > center_xpos)) {
        /* replace the former separator '\0' of left and
            center string with a space */
        *(--format_align->center) = ' ';
        /* calculate the new width and position of the merged string */
        left_width = left_width + space_width + center_width;
        /* there is no centered string anymore */
        center_width = 0;
    }
    /* there is no left string, move center to left */
    if ((left_width == 0 && center_width != 0) &&
        (left_width > center_xpos)) {
        /* move the center string to the left string */
        format_align->left = format_align->center;
        /* calculate the new width and position of the string */
        left_width = center_width;
        /* there is no centered string anymore */
        center_width = 0;
    }

    /* CASE 2: centered and right string overlap */
    /* there is a right string, need to merge center and right */
    if ((center_width != 0 && right_width != 0) &&
        (center_xpos + center_width + space_width > right_xpos)) {
        /* replace the former separator '\0' of center and
            right string with a space */
        *(--format_align->right) = ' ';
        /* move the center string to the right after merge */
        format_align->right = format_align->center;
        /* calculate the new width and position of the merged string */
        right_width = center_width + space_width + right_width;
        right_xpos = (viewport_width - right_width);
        /* there is no centered string anymore */
        center_width = 0;
    }
    /* there is no right string, move center to right */
    if ((center_width != 0 && right_width == 0) &&
        (center_xpos + center_width > right_xpos)) {
        /* move the center string to the right string */
        format_align->right = format_align->center;
        /* calculate the new width and position of the string */
        right_width = center_width;
        right_xpos = (viewport_width - right_width);
        /* there is no centered string anymore */
        center_width = 0;
    }

    /* CASE 3: left and right overlap
        There is no center string anymore, either there never
        was one or it has been merged in case 1 or 2 */
    /* there is a left string, need to merge left and right */
    if ((left_width != 0 && center_width == 0 && right_width != 0) &&
        (left_width + space_width > right_xpos)) {
        /* replace the former separator '\0' of left and
            right string with a space */
        *(--format_align->right) = ' ';
        /* calculate the new width and position of the string */
        left_width = left_width + space_width + right_width;
        /* there is no right string anymore */
        right_width = 0;
    }
    /* there is no left string, move right to left */
    if ((left_width == 0 && center_width == 0 && right_width != 0) &&
        (left_width > right_xpos)) {
        /* move the right string to the left string */
        format_align->left = format_align->right;
        /* calculate the new width and position of the string */
        left_width = right_width;
        /* there is no right string anymore */
        right_width = 0;
    }

    if (scroll && ((left_width > scroll_width) ||
                   (center_width > scroll_width) ||
                   (right_width > scroll_width)))
    {
#ifdef HAVE_LCD_BITMAP
        display->puts_scroll_style(0, line,
                             (unsigned char *)format_align->left, style);
#else
        display->puts_scroll(0, line,
                             (unsigned char *)format_align->left);
#endif
    }
    else
    {
#ifdef HAVE_LCD_BITMAP
        /* clear the line first */
        display->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        display->fillrect(0, line*string_height, viewport_width, string_height);
        display->set_drawmode(DRMODE_SOLID);
#endif

        /* Nasty hack: we output an empty scrolling string,
        which will reset the scroller for that line */
        display->puts_scroll(0, line, (unsigned char *)"");
#ifdef HAVE_LCD_BITMAP
        style |= STYLE_XY_PIXELS;
        line *= string_height;
        /* print aligned strings */
        if (left_width != 0)
        {
            display->puts_style_xyoffset(0, line,
                            (unsigned char *)format_align->left, style, 0, 0);

        }
        if (center_width != 0)
        {
            display->puts_style_xyoffset((viewport_width-center_width)/2, line,
                            (unsigned char *)format_align->center, style, 0, 0);
        }
        if (right_width != 0)
        {
            display->puts_style_xyoffset(viewport_width-right_width, line,
                            (unsigned char *)format_align->right, style, 0, 0);
        }
#else
        if (left_width != 0)
        {
            display->putsxy(0, line,
                    (unsigned char *)format_align->left);
        }
        if (center_width != 0)
        {
            display->putsxy(center_xpos, line,
                    (unsigned char *)format_align->center);
        }
        if (right_width != 0)
        {
            display->putsxy(right_xpos, line,
                    (unsigned char *)format_align->right);
        }
#endif
    }
}

#ifdef HAVE_LCD_BITMAP
void draw_peakmeters(struct gui_wps *gwps, int line_number,
                     struct viewport *viewport)
{
    struct wps_data *data = gwps->data;
    if (!data->peak_meter_enabled)
    {
        peak_meter_enable(false);
    }
    else
    {
        int h = font_get(viewport->font)->height;
        int peak_meter_y = line_number * h;

        /* The user might decide to have the peak meter in the last
            line so that it is only displayed if no status bar is
            visible. If so we neither want do draw nor enable the
            peak meter. */
        if (peak_meter_y + h <= viewport->y+viewport->height) {
            peak_meter_enable(true);
            peak_meter_screen(gwps->display, 0, peak_meter_y,
                              MIN(h, viewport->y+viewport->height - peak_meter_y));
        }
    }
}

bool skin_has_sbs(enum screen_type screen, struct wps_data *data)
{
    (void)screen;
    (void)data;
    bool draw = false;
#ifdef HAVE_LCD_BITMAP
    if (data->wps_sb_tag)
        draw = data->show_sb_on_wps;
    else if (statusbar_position(screen) != STATUSBAR_OFF)
        draw = true;
#endif
    return draw;
}
#endif

/* do the button loop as often as required for the peak meters to update
 * with a good refresh rate. 
 */
int skin_wait_for_action(enum skinnable_screens skin, int context, int timeout)
{
    (void)skin; /* silence charcell warning */
    int button = ACTION_NONE;
#ifdef HAVE_LCD_BITMAP
    /* when the peak meter is enabled we want to have a
        few extra updates to make it look smooth. On the
        other hand we don't want to waste energy if it
        isn't displayed */
    bool pm=false;
    FOR_NB_SCREENS(i)
    {
       if(skin_get_gwps(skin, i)->data->peak_meter_enabled)
           pm = true;
    }

    if (pm) {
        long next_refresh = current_tick;
        long next_big_refresh = current_tick + timeout;
        button = BUTTON_NONE;
        while (TIME_BEFORE(current_tick, next_big_refresh)) {
            button = get_action(context,TIMEOUT_NOBLOCK);
            if (button != ACTION_NONE) {
                break;
            }
            peak_meter_peek();
            sleep(0);   /* Sleep until end of current tick. */

            if (TIME_AFTER(current_tick, next_refresh)) {
                FOR_NB_SCREENS(i)
                {
                    if(skin_get_gwps(skin, i)->data->peak_meter_enabled)
                        skin_update(skin, i, SKIN_REFRESH_PEAK_METER);
                    next_refresh += HZ / PEAK_METER_FPS;
                }
            }
        }

    }

    /* The peak meter is disabled
       -> no additional screen updates needed */
    else
#endif
    {
        button = get_action(context, timeout);
    }
    return button;
}
