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
#include "font.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "system.h"
#include "settings.h"
#include "settings_list.h"
#include "rbunicode.h"
#include "rtc.h"
#include "audio.h"
#include "status.h"
#include "power.h"
#include "powermgmt.h"
#include "sound.h"
#include "debug.h"
#ifdef HAVE_LCD_CHARCELLS
#include "hwcompat.h"
#endif
#include "abrepeat.h"
#include "mp3_playback.h"
#include "lang.h"
#include "misc.h"
#include "splash.h"
#include "scrollbar.h"
#include "led.h"
#include "lcd.h"
#ifdef HAVE_LCD_BITMAP
#include "peakmeter.h"
/* Image stuff */
#include "bmp.h"
#include "albumart.h"
#endif
#include "dsp.h"
#include "action.h"
#include "cuesheet.h"
#include "playlist.h"
#if CONFIG_CODEC == SWCODEC
#include "playback.h"
#endif
#include "backdrop.h"
#include "viewport.h"


#include "wps_internals.h"
#include "skin_engine.h"
#include "statusbar-skinned.h"

static bool skin_redraw(struct gui_wps *gwps, unsigned refresh_mode);


/* TODO: maybe move this whole function into wps.c instead ? */
bool gui_wps_display(struct gui_wps *gwps)
{
    struct screen *display = gwps->display;

    /* Update the values in the first (default) viewport - in case the user
       has modified the statusbar or colour settings */
#if LCD_DEPTH > 1
    if (display->depth > 1)
    {
        struct viewport *vp = &find_viewport(VP_DEFAULT_LABEL, gwps->data)->vp;
        vp->fg_pattern = display->get_foreground();
        vp->bg_pattern = display->get_background();
    }
#endif
    display->backdrop_show(BACKDROP_SKIN_WPS);
    return skin_redraw(gwps, WPS_REFRESH_ALL);
}

/* update a skinned screen, update_type is WPS_REFRESH_* values.
 * Usually it should only be WPS_REFRESH_NON_STATIC
 * A full update will be done if required (state.do_full_update == true)
 */
bool skin_update(struct gui_wps *gwps, unsigned int update_type)
{
    bool retval;
    /* This maybe shouldnt be here, but while the skin is only used to
     * display the music screen this is better than whereever we are being
     * called from. This is also safe for skined screen which dont use the id3 */
    struct mp3entry *id3 = gwps->state->id3;
    bool cuesheet_update = (id3 != NULL ? cuesheet_subtrack_changed(id3) : false);
    gwps->sync_data->do_full_update |= cuesheet_update;

    retval = skin_redraw(gwps, gwps->sync_data->do_full_update ?
                                        WPS_REFRESH_ALL : update_type);
    return retval;
}

#ifdef HAVE_LCD_BITMAP

void skin_statusbar_changed(struct gui_wps *skin)
{
    if (!skin)
        return;
    struct wps_data *data = skin->data;
    const struct screen *display = skin->display;
    const int   screen = display->screen_type;

    struct viewport *vp = &find_viewport(VP_DEFAULT_LABEL, data)->vp;
    viewport_set_fullscreen(vp, screen);

    if (data->wps_sb_tag)
    {   /* fix up the default viewport */
        if (data->show_sb_on_wps)
        {
            if (statusbar_position(screen) != STATUSBAR_OFF)
                return;     /* vp is fixed already */

            vp->y       = STATUSBAR_HEIGHT;
            vp->height  = display->lcdheight - STATUSBAR_HEIGHT;
        }
        else
        {
            if (statusbar_position(screen) == STATUSBAR_OFF)
                return;     /* vp is fixed already */
            vp->y       = vp->x = 0;
            vp->height  = display->lcdheight;
            vp->width   = display->lcdwidth;
        }
    }
}

static void draw_progressbar(struct gui_wps *gwps,
                             struct skin_viewport *wps_vp)
 {
    struct screen *display = gwps->display;
    struct wps_state *state = gwps->state;
    struct progressbar *pb = wps_vp->pb;
    struct mp3entry *id3 = state->id3;
    int y = pb->y;

    if (y < 0)
    {
        int line_height = font_get(wps_vp->vp.font)->height;
        /* center the pb in the line, but only if the line is higher than the pb */
        int center = (line_height-pb->height)/2;
        /* if Y was not set calculate by font height,Y is -line_number-1 */
        y = (-y -1)*line_height + (0 > center ? 0 : center);
    }

    int elapsed, length;
    if (id3)
    {
        elapsed = id3->elapsed;
        length = id3->length;
    }
    else
    {
        elapsed = 0;
        length = 0;
    }

    if (pb->have_bitmap_pb)
        gui_bitmap_scrollbar_draw(display, pb->bm,
                                pb->x, y, pb->width, pb->bm.height,
                                length ? length : 1, 0,
                                length ? elapsed + state->ff_rewind_count : 0,
                                HORIZONTAL);
    else
        gui_scrollbar_draw(display, pb->x, y, pb->width, pb->height,
                           length ? length : 1, 0,
                           length ? elapsed + state->ff_rewind_count : 0,
                           HORIZONTAL);
#ifdef AB_REPEAT_ENABLE
    if ( ab_repeat_mode_enabled() && length != 0 )
        ab_draw_markers(display, length,
                        pb->x, pb->x + pb->width, y, pb->height);
#endif

    if (id3 && id3->cuesheet)
        cue_draw_markers(display, state->id3->cuesheet, length,
                         pb->x, pb->x + pb->width, y+1, pb->height-2);
}

/* clears the area where the image was shown */
static void clear_image_pos(struct gui_wps *gwps, struct gui_img *img)
{
    if(!gwps)
        return;
    gwps->display->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    gwps->display->fillrect(img->x, img->y, img->bm.width, img->subimage_height);
    gwps->display->set_drawmode(DRMODE_SOLID);
}

static void wps_draw_image(struct gui_wps *gwps, struct gui_img *img, int subimage)
{
    struct screen *display = gwps->display;
    if(img->always_display)
        display->set_drawmode(DRMODE_FG);
    else
        display->set_drawmode(DRMODE_SOLID);

#if LCD_DEPTH > 1
    if(img->bm.format == FORMAT_MONO) {
#endif
        display->mono_bitmap_part(img->bm.data,
                                  0, img->subimage_height * subimage,
                                  img->bm.width, img->x,
                                  img->y, img->bm.width,
                                  img->subimage_height);
#if LCD_DEPTH > 1
    } else {
        display->transparent_bitmap_part((fb_data *)img->bm.data,
                                         0, img->subimage_height * subimage,
                                         STRIDE(display->screen_type,
                                            img->bm.width, img->bm.height),
                                         img->x, img->y, img->bm.width,
                                         img->subimage_height);
    }
#endif
}

static void wps_display_images(struct gui_wps *gwps, struct viewport* vp)
{
    if(!gwps || !gwps->data || !gwps->display)
        return;

    struct wps_data *data = gwps->data;
    struct screen *display = gwps->display;
    struct skin_token_list *list = data->images;

    while (list)
    {
        struct gui_img *img = (struct gui_img*)list->token->value.data;
        if (img->loaded)
        {
            if (img->display >= 0)
            {
                wps_draw_image(gwps, img, img->display);
            }
            else if (img->always_display && img->vp == vp)
            {
                wps_draw_image(gwps, img, 0);
            }
        }
        list = list->next;
    }
#ifdef HAVE_ALBUMART
    /* now draw the AA */
    if (data->albumart && data->albumart->vp == vp
	    && data->albumart->draw)
    {
        draw_album_art(gwps, playback_current_aa_hid(data->playback_aa_slot),
                        false);
		data->albumart->draw = false;
    }
#endif

    display->set_drawmode(DRMODE_SOLID);
}

#else /* HAVE_LCD_CHARCELL */

static bool draw_player_progress(struct gui_wps *gwps)
{
    struct wps_state *state = gwps->state;
    struct screen *display = gwps->display;
    unsigned char progress_pattern[7];
    int pos = 0;
    int i;

    int elapsed, length;
    if (LIKELY(state->id3))
    {
        elapsed = state->id3->elapsed;
        length = state->id3->length;
    }
    else
    {
        elapsed = 0;
        length = 0;
    }

    if (length)
        pos = 36 * (elapsed + state->ff_rewind_count) / length;

    for (i = 0; i < 7; i++, pos -= 5)
    {
        if (pos <= 0)
            progress_pattern[i] = 0x1fu;
        else if (pos >= 5)
            progress_pattern[i] = 0x00u;
        else
            progress_pattern[i] = 0x1fu >> pos;
    }

    display->define_pattern(gwps->data->wps_progress_pat[0], progress_pattern);
    return true;
}

static void draw_player_fullbar(struct gui_wps *gwps, char* buf, int buf_size)
{
    static const unsigned char numbers[10][4] = {
        {0x0e, 0x0a, 0x0a, 0x0e}, /* 0 */
        {0x04, 0x0c, 0x04, 0x04}, /* 1 */
        {0x0e, 0x02, 0x04, 0x0e}, /* 2 */
        {0x0e, 0x02, 0x06, 0x0e}, /* 3 */
        {0x08, 0x0c, 0x0e, 0x04}, /* 4 */
        {0x0e, 0x0c, 0x02, 0x0c}, /* 5 */
        {0x0e, 0x08, 0x0e, 0x0e}, /* 6 */
        {0x0e, 0x02, 0x04, 0x08}, /* 7 */
        {0x0e, 0x0e, 0x0a, 0x0e}, /* 8 */
        {0x0e, 0x0e, 0x02, 0x0e}, /* 9 */
    };

    struct wps_state *state = gwps->state;
    struct screen *display = gwps->display;
    struct wps_data *data = gwps->data;
    unsigned char progress_pattern[7];
    char timestr[10];
    int time;
    int time_idx = 0;
    int pos = 0;
    int pat_idx = 1;
    int digit, i, j;
    bool softchar;

    int elapsed, length;
    if (LIKELY(state->id3))
    {
        elapsed = state->id3->elapsed;
        length = state->id3->length;
    }
    else
    {
        elapsed = 0;
        length = 0;
    }

    if (buf_size < 34) /* worst case: 11x UTF-8 char + \0 */
        return;

    time = elapsed + state->ff_rewind_count;
    if (length)
        pos = 55 * time / length;

    memset(timestr, 0, sizeof(timestr));
    format_time(timestr, sizeof(timestr)-2, time);
    timestr[strlen(timestr)] = ':';   /* always safe */

    for (i = 0; i < 11; i++, pos -= 5)
    {
        softchar = false;
        memset(progress_pattern, 0, sizeof(progress_pattern));

        if ((digit = timestr[time_idx]))
        {
            softchar = true;
            digit -= '0';

            if (timestr[time_idx + 1] == ':')  /* ones, left aligned */
            {
                memcpy(progress_pattern, numbers[digit], 4);
                time_idx += 2;
            }
            else  /* tens, shifted right */
            {
                for (j = 0; j < 4; j++)
                    progress_pattern[j] = numbers[digit][j] >> 1;

                if (time_idx > 0)  /* not the first group, add colon in front */
                {
                    progress_pattern[1] |= 0x10u;
                    progress_pattern[3] |= 0x10u;
                }
                time_idx++;
            }

            if (pos >= 5)
                progress_pattern[5] = progress_pattern[6] = 0x1fu;
        }

        if (pos > 0 && pos < 5)
        {
            softchar = true;
            progress_pattern[5] = progress_pattern[6] = (~0x1fu >> pos) & 0x1fu;
        }

        if (softchar && pat_idx < 8)
        {
            display->define_pattern(data->wps_progress_pat[pat_idx],
                                    progress_pattern);
            buf = utf8encode(data->wps_progress_pat[pat_idx], buf);
            pat_idx++;
        }
        else if (pos <= 0)
            buf = utf8encode(' ', buf);
        else
            buf = utf8encode(0xe115, buf); /* 2/7 _ */
    }
    *buf = '\0';
}

#endif /* HAVE_LCD_CHARCELL */

/* Return the index to the end token for the conditional token at index.
   The conditional token can be either a start token or a separator
   (i.e. option) token.
*/
static int find_conditional_end(struct wps_data *data, int index)
{
    int ret = index;
    while (data->tokens[ret].type != WPS_TOKEN_CONDITIONAL_END)
        ret = data->tokens[ret].value.i;

    /* ret now is the index to the end token for the conditional. */
    return ret;
}

/* Evaluate the conditional that is at *token_index and return whether a skip
   has ocurred. *token_index is updated with the new position.
*/
static bool evaluate_conditional(struct gui_wps *gwps, int *token_index)
{
    if (!gwps)
        return false;

    struct wps_data *data = gwps->data;

    int i, cond_end;
    int cond_index = *token_index;
    char result[128];
    const char *value;
    unsigned char num_options = data->tokens[cond_index].value.i & 0xFF;
    unsigned char prev_val = (data->tokens[cond_index].value.i & 0xFF00) >> 8;

    /* treat ?xx<true> constructs as if they had 2 options. */
    if (num_options < 2)
        num_options = 2;

    int intval = num_options;
    /* get_token_value needs to know the number of options in the enum */
    value = get_token_value(gwps, &data->tokens[cond_index + 1],
                            result, sizeof(result), &intval);

    /* intval is now the number of the enum option we want to read,
       starting from 1. If intval is -1, we check if value is empty. */
    if (intval == -1)
        intval = (value && *value) ? 1 : num_options;
    else if (intval > num_options || intval < 1)
        intval = num_options;

    data->tokens[cond_index].value.i = (intval << 8) + num_options;

    /* skip to the appropriate enum case */
    int next = cond_index + 2;
    for (i = 1; i < intval; i++)
    {
        next = data->tokens[next].value.i;
    }
    *token_index = next;

    if (prev_val == intval)
    {
        /* Same conditional case as previously. Return without clearing the
           pictures */
        return false;
    }

    cond_end = find_conditional_end(data, cond_index + 2);
    for (i = cond_index + 3; i < cond_end; i++)
    {
#ifdef HAVE_LCD_BITMAP
        /* clear all pictures in the conditional and nested ones */
        if (data->tokens[i].type == WPS_TOKEN_IMAGE_PRELOAD_DISPLAY)
            clear_image_pos(gwps, find_image(data->tokens[i].value.i&0xFF, gwps->data));
#endif
#ifdef HAVE_ALBUMART
        if (data->albumart && data->tokens[i].type == WPS_TOKEN_ALBUMART_DISPLAY)
        {
            draw_album_art(gwps,
                    playback_current_aa_hid(data->playback_aa_slot), true);
            data->albumart->draw = false;
        }
#endif
    }

    return true;
}

#ifdef HAVE_LCD_BITMAP
struct gui_img* find_image(char label, struct wps_data *data)
{
    struct skin_token_list *list = data->images;
    while (list)
    {
        struct gui_img *img = (struct gui_img *)list->token->value.data;
        if (img->label == label)
            return img;
        list = list->next;
    }
    return NULL;
}
#endif

struct skin_viewport* find_viewport(char label, struct wps_data *data)
{
    struct skin_token_list *list = data->viewports;
    while (list)
    {
        struct skin_viewport *vp = (struct skin_viewport *)list->token->value.data;
        if (vp->label == label)
            return vp;
        list = list->next;
    }
    return NULL;
}


/* Read a (sub)line to the given alignment format buffer.
   linebuf is the buffer where the data is actually stored.
   align is the alignment format that'll be used to display the text.
   The return value indicates whether the line needs to be updated.
*/
static bool get_line(struct gui_wps *gwps,
                     struct skin_subline *subline,
                     struct align_pos *align,
                     char *linebuf,
                     int linebuf_size)
{
    struct wps_data *data = gwps->data;

    char temp_buf[128];
    char *buf = linebuf;  /* will always point to the writing position */
    char *linebuf_end = linebuf + linebuf_size - 1;
    bool update = false;
    int i;

    /* alignment-related variables */
    int cur_align;
    char* cur_align_start;
    cur_align_start = buf;
    cur_align = WPS_ALIGN_LEFT;
    align->left = NULL;
    align->center = NULL;
    align->right = NULL;
    /* Process all tokens of the desired subline */
    for (i = subline->first_token_idx;
         i <= subline->last_token_idx; i++)
    {
        switch(data->tokens[i].type)
        {
            case WPS_TOKEN_CONDITIONAL:
                /* place ourselves in the right conditional case */
                update |= evaluate_conditional(gwps, &i);
                break;

            case WPS_TOKEN_CONDITIONAL_OPTION:
                /* we've finished in the curent conditional case,
                    skip to the end of the conditional structure */
                i = find_conditional_end(data, i);
                break;

#ifdef HAVE_LCD_BITMAP
            case WPS_TOKEN_IMAGE_PRELOAD_DISPLAY:
            {
                char n = data->tokens[i].value.i & 0xFF;
                int subimage = data->tokens[i].value.i >> 8;
                struct gui_img *img = find_image(n, data);

                if (img && img->loaded)
                    img->display = subimage;
                break;
            }
#endif

            case WPS_TOKEN_ALIGN_LEFT:
            case WPS_TOKEN_ALIGN_CENTER:
            case WPS_TOKEN_ALIGN_RIGHT:
                /* remember where the current aligned text started */
                switch (cur_align)
                {
                    case WPS_ALIGN_LEFT:
                        align->left = cur_align_start;
                        break;

                    case WPS_ALIGN_CENTER:
                        align->center = cur_align_start;
                        break;

                    case WPS_ALIGN_RIGHT:
                        align->right = cur_align_start;
                        break;
                }
                /* start a new alignment */
                switch (data->tokens[i].type)
                {
                    case WPS_TOKEN_ALIGN_LEFT:
                        cur_align = WPS_ALIGN_LEFT;
                        break;
                    case WPS_TOKEN_ALIGN_CENTER:
                        cur_align = WPS_ALIGN_CENTER;
                        break;
                    case WPS_TOKEN_ALIGN_RIGHT:
                        cur_align = WPS_ALIGN_RIGHT;
                        break;
                    default:
                        break;
                }
                *buf++ = 0;
                cur_align_start = buf;
                break;
            case WPS_VIEWPORT_ENABLE:
            {
                char label = data->tokens[i].value.i;
                char temp = VP_DRAW_HIDEABLE;
                /* viewports are allowed to share id's so find and enable
                 * all of them */
                struct skin_token_list *list = data->viewports;
                while (list)
                {
                    struct skin_viewport *vp =
                                (struct skin_viewport *)list->token->value.data;
                    if (vp->label == label)
                    {
                        if (vp->hidden_flags&VP_DRAW_WASHIDDEN)
                            temp |= VP_DRAW_WASHIDDEN;
                        vp->hidden_flags = temp;
                    }
                    list = list->next;
                }
            }
                break;
            default:
            {
                /* get the value of the tag and copy it to the buffer */
                const char *value = get_token_value(gwps, &data->tokens[i],
                                              temp_buf, sizeof(temp_buf), NULL);
                if (value)
                {
                    update = true;
                    while (*value && (buf < linebuf_end))
                        *buf++ = *value++;
                }
                break;
            }
        }
    }

    /* close the current alignment */
    switch (cur_align)
    {
        case WPS_ALIGN_LEFT:
            align->left = cur_align_start;
            break;

        case WPS_ALIGN_CENTER:
            align->center = cur_align_start;
            break;

        case WPS_ALIGN_RIGHT:
            align->right = cur_align_start;
            break;
    }

    return update;
}
static void get_subline_timeout(struct gui_wps *gwps, struct skin_subline *subline)
{
    struct wps_data *data = gwps->data;
    int i;
    subline->time_mult = DEFAULT_SUBLINE_TIME_MULTIPLIER;

    for (i = subline->first_token_idx;
         i <= subline->last_token_idx; i++)
    {
        switch(data->tokens[i].type)
        {
            case WPS_TOKEN_CONDITIONAL:
                /* place ourselves in the right conditional case */
                evaluate_conditional(gwps, &i);
                break;

            case WPS_TOKEN_CONDITIONAL_OPTION:
                /* we've finished in the curent conditional case,
                    skip to the end of the conditional structure */
                i = find_conditional_end(data, i);
                break;

            case WPS_TOKEN_SUBLINE_TIMEOUT:
                subline->time_mult = data->tokens[i].value.i;
                break;

            default:
                break;
        }
    }
}

/* Calculates which subline should be displayed for the specified line
   Returns true iff the subline must be refreshed */
static bool update_curr_subline(struct gui_wps *gwps, struct skin_line *line)
{
    /* shortcut this whole thing if we need to reset the line completly */
    if (line->curr_subline == NULL)
    {
        line->subline_expire_time = current_tick;
        line->curr_subline = &line->sublines;
        if (!line->curr_subline->next)
        {
            line->subline_expire_time += 100*HZ;
        }
        else
        {
            get_subline_timeout(gwps, line->curr_subline);
            line->subline_expire_time += TIMEOUT_UNIT*line->curr_subline->time_mult;
        }
        return true;
    }
    /* if time to advance to next sub-line  */
    if (TIME_AFTER(current_tick, line->subline_expire_time - 1))
    {
        /* if there is only one subline, there is no need to search for a new one */
        if (&line->sublines == line->curr_subline &&
             line->curr_subline->next == NULL)
        {
            line->subline_expire_time += 100 * HZ;
            return false;
        }
        if (line->curr_subline->next)
            line->curr_subline = line->curr_subline->next;
        else
            line->curr_subline = &line->sublines;
        get_subline_timeout(gwps, line->curr_subline);
        line->subline_expire_time += TIMEOUT_UNIT*line->curr_subline->time_mult;
        return true;
    }
    return false;
}

/* Display a line appropriately according to its alignment format.
   format_align contains the text, separated between left, center and right.
   line is the index of the line on the screen.
   scroll indicates whether the line is a scrolling one or not.
*/
static void write_line(struct screen *display,
                       struct align_pos *format_align,
                       int line,
                       bool scroll)
{
    int left_width = 0, left_xpos;
    int center_width = 0, center_xpos;
    int right_width = 0,  right_xpos;
    int ypos;
    int space_width;
    int string_height;
    int scroll_width;

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

    left_xpos = 0;
    right_xpos = (display->getwidth() - right_width);
    center_xpos = (display->getwidth() + left_xpos - center_width) / 2;

    scroll_width = display->getwidth() - left_xpos;

    /* Checks for overlapping strings.
        If needed the overlapping strings will be merged, separated by a
        space */

    /* CASE 1: left and centered string overlap */
    /* there is a left string, need to merge left and center */
    if ((left_width != 0 && center_width != 0) &&
        (left_xpos + left_width + space_width > center_xpos)) {
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
        (left_xpos + left_width > center_xpos)) {
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
        right_xpos = (display->getwidth() - right_width);
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
        right_xpos = (display->getwidth() - right_width);
        /* there is no centered string anymore */
        center_width = 0;
    }

    /* CASE 3: left and right overlap
        There is no center string anymore, either there never
        was one or it has been merged in case 1 or 2 */
    /* there is a left string, need to merge left and right */
    if ((left_width != 0 && center_width == 0 && right_width != 0) &&
        (left_xpos + left_width + space_width > right_xpos)) {
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

    ypos = (line * string_height);


    if (scroll && ((left_width > scroll_width) ||
                   (center_width > scroll_width) ||
                   (right_width > scroll_width)))
    {
        display->puts_scroll(0, line,
                             (unsigned char *)format_align->left);
    }
    else
    {
#ifdef HAVE_LCD_BITMAP
        /* clear the line first */
        display->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        display->fillrect(left_xpos, ypos, display->getwidth(), string_height);
        display->set_drawmode(DRMODE_SOLID);
#endif

        /* Nasty hack: we output an empty scrolling string,
        which will reset the scroller for that line */
        display->puts_scroll(0, line, (unsigned char *)"");

        /* print aligned strings */
        if (left_width != 0)
        {
            display->putsxy(left_xpos, ypos,
                            (unsigned char *)format_align->left);
        }
        if (center_width != 0)
        {
            display->putsxy(center_xpos, ypos,
                            (unsigned char *)format_align->center);
        }
        if (right_width != 0)
        {
            display->putsxy(right_xpos, ypos,
                            (unsigned char *)format_align->right);
        }
    }
}

static bool skin_redraw(struct gui_wps *gwps, unsigned refresh_mode)
{
    struct wps_data *data = gwps->data;
    struct screen *display = gwps->display;

    if (!data || !display || !gwps->state)
        return false;

    unsigned flags;
    char linebuf[MAX_PATH];

    struct align_pos align;
    align.left = NULL;
    align.center = NULL;
    align.right = NULL;


    struct skin_token_list *viewport_list;

    bool update_line, new_subline_refresh;

#ifdef HAVE_LCD_BITMAP

    /* to find out wether the peak meter is enabled we
       assume it wasn't until we find a line that contains
       the peak meter. We can't use peak_meter_enabled itself
       because that would mean to turn off the meter thread
       temporarily. (That shouldn't matter unless yield
       or sleep is called but who knows...)
    */
    bool enable_pm = false;

#endif

    /* reset to first subline if refresh all flag is set */
    if (refresh_mode == WPS_REFRESH_ALL)
    {
        struct skin_line *line;
        struct skin_viewport *skin_viewport = find_viewport(VP_DEFAULT_LABEL, data);

        if (!(skin_viewport->hidden_flags & VP_NEVER_VISIBLE))
        {
            display->set_viewport(&skin_viewport->vp);
            display->clear_viewport();
        }

        for (viewport_list = data->viewports;
             viewport_list; viewport_list = viewport_list->next)
        {
            skin_viewport =
                  (struct skin_viewport *)viewport_list->token->value.data;
            for(line = skin_viewport->lines; line; line = line->next)
            {
                line->curr_subline = NULL;
            }
        }
    }

#ifdef HAVE_LCD_CHARCELLS
    int i;
    for (i = 0; i < 8; i++)
    {
       if (data->wps_progress_pat[i] == 0)
           data->wps_progress_pat[i] = display->get_locked_pattern();
    }
#endif

    /* disable any viewports which are conditionally displayed */
    for (viewport_list = data->viewports;
         viewport_list; viewport_list = viewport_list->next)
    {
        struct skin_viewport *skin_viewport =
                        (struct skin_viewport *)viewport_list->token->value.data;
        if (skin_viewport->hidden_flags&VP_NEVER_VISIBLE)
        {
            continue;
        }
        if (skin_viewport->hidden_flags&VP_DRAW_HIDEABLE)
        {
            if (skin_viewport->hidden_flags&VP_DRAW_HIDDEN)
                skin_viewport->hidden_flags |= VP_DRAW_WASHIDDEN;
            else
                skin_viewport->hidden_flags |= VP_DRAW_HIDDEN;
        }
    }
    int viewport_count = 0;
    for (viewport_list = data->viewports;
         viewport_list; viewport_list = viewport_list->next, viewport_count++)
    {
        struct skin_viewport *skin_viewport =
                        (struct skin_viewport *)viewport_list->token->value.data;
        unsigned vp_refresh_mode = refresh_mode;

        display->set_viewport(&skin_viewport->vp);

        int hidden_vp = 0;

#ifdef HAVE_LCD_BITMAP
        /* Set images to not to be displayed */
        struct skin_token_list *imglist = data->images;
        while (imglist)
        {
            struct gui_img *img = (struct gui_img *)imglist->token->value.data;
            img->display = -1;
            imglist = imglist->next;
        }
#endif
        /* dont redraw the viewport if its disabled */
        if (skin_viewport->hidden_flags&VP_NEVER_VISIBLE)
        {   /* don't draw anything into this one */
            vp_refresh_mode = 0; hidden_vp = true;
        }
        else if ((skin_viewport->hidden_flags&VP_DRAW_HIDDEN))
        {
            if (!(skin_viewport->hidden_flags&VP_DRAW_WASHIDDEN))
                display->scroll_stop(&skin_viewport->vp);
            skin_viewport->hidden_flags |= VP_DRAW_WASHIDDEN;
            continue;
        }
        else if (((skin_viewport->hidden_flags&
                   (VP_DRAW_WASHIDDEN|VP_DRAW_HIDEABLE))
                    == (VP_DRAW_WASHIDDEN|VP_DRAW_HIDEABLE)))
        {
            vp_refresh_mode = WPS_REFRESH_ALL;
            skin_viewport->hidden_flags = VP_DRAW_HIDEABLE;
        }

        if (vp_refresh_mode == WPS_REFRESH_ALL)
        {
            display->clear_viewport();
        }

        /* loop over the lines for this viewport */
        struct skin_line *line;
        int line_count = 0;

        for (line = skin_viewport->lines; line; line = line->next, line_count++)
        {
            struct skin_subline *subline;
            memset(linebuf, 0, sizeof(linebuf));
            update_line = false;

            /* get current subline for the line */
            new_subline_refresh = update_curr_subline(gwps, line);
            subline = line->curr_subline;
            flags = line->curr_subline->line_type;

            if (vp_refresh_mode == WPS_REFRESH_ALL || (flags & vp_refresh_mode)
                || new_subline_refresh || hidden_vp)
            {
                /* get_line tells us if we need to update the line */
                update_line = get_line(gwps, subline,
                                       &align, linebuf, sizeof(linebuf));
            }
#ifdef HAVE_LCD_BITMAP
            /* peakmeter */
            if (flags & vp_refresh_mode & WPS_REFRESH_PEAK_METER)
            {
                /* the peakmeter should be alone on its line */
                update_line = false;

                int h = font_get(skin_viewport->vp.font)->height;
                int peak_meter_y = line_count* h;

                /* The user might decide to have the peak meter in the last
                    line so that it is only displayed if no status bar is
                    visible. If so we neither want do draw nor enable the
                    peak meter. */
                if (peak_meter_y + h <= skin_viewport->vp.y+skin_viewport->vp.height) {
                    /* found a line with a peak meter -> remember that we must
                        enable it later */
                    enable_pm = true;
                    peak_meter_enabled = true;
                    peak_meter_screen(gwps->display, 0, peak_meter_y,
                                      MIN(h, skin_viewport->vp.y+skin_viewport->vp.height - peak_meter_y));
                }
                else
                {
                    peak_meter_enabled = false;
                }
            }

#else /* HAVE_LCD_CHARCELL */

            /* progressbar */
            if (flags & vp_refresh_mode & WPS_REFRESH_PLAYER_PROGRESS)
            {
                if (data->full_line_progressbar)
                    draw_player_fullbar(gwps, linebuf, sizeof(linebuf));
                else
                    draw_player_progress(gwps);
            }
#endif

            if (update_line && !hidden_vp &&
                /* conditionals clear the line which means if the %Vd is put into the default
                   viewport there will be a blank line.
                   To get around this we dont allow any actual drawing to happen in the
                   deault vp if other vp's are defined */
                ((skin_viewport->label != VP_DEFAULT_LABEL && viewport_list->next) ||
                 !viewport_list->next))
            {
                if (flags & WPS_REFRESH_SCROLL)
                {
                    /* if the line is a scrolling one we don't want to update
                       too often, so that it has the time to scroll */
                    if ((vp_refresh_mode & WPS_REFRESH_SCROLL) || new_subline_refresh)
                        write_line(display, &align, line_count, true);
                }
                else
                    write_line(display, &align, line_count, false);
            }
        }
#ifdef HAVE_LCD_BITMAP
        /* progressbar */
        if (vp_refresh_mode & WPS_REFRESH_PLAYER_PROGRESS)
        {
            if (skin_viewport->pb)
            {
                draw_progressbar(gwps, skin_viewport);
            }
        }
        /* Now display any images in this viewport */
        if (!hidden_vp)
            wps_display_images(gwps, &skin_viewport->vp);
#endif
    }

#ifdef HAVE_LCD_BITMAP
    data->peak_meter_enabled = enable_pm;
#endif

    if (refresh_mode & WPS_REFRESH_STATUSBAR)
    {
        viewportmanager_set_statusbar(gwps->sync_data->statusbars);
    }
    /* Restore the default viewport */
    display->set_viewport(NULL);

    display->update();

    return true;
}
