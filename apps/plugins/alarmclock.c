/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Clément Pit-Claudel
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
#include "lib/pluginlib_actions.h"



const struct button_mapping *plugin_contexts[] = { pla_main_ctx };

static int current = 0;
static bool tomorrow = false;
static int alarm[2] = {0, 0}, maxval[2] = {24, 60}, prev_tick = 3600 * 24;
static bool quit = false, usb = false, waiting = false, done = false;

static inline int get_button(void) {
    return pluginlib_getaction(HZ/2, plugin_contexts,
                               ARRAYLEN(plugin_contexts));
}

int rem_seconds(void) {
    int seconds = (((alarm[0] - rb->get_time()->tm_hour) * 3600)
                  +((alarm[1] - rb->get_time()->tm_min)  * 60)
                  -(rb->get_time()->tm_sec));

    /* The tomorrow flag means that the alarm should ring on the next day */
    if (seconds > prev_tick) tomorrow = false;
    prev_tick = seconds;

    return seconds + (tomorrow ? 24 * 3600 : 0);
}

void draw_centered_string(struct screen * display, char * string) {
    int w, h;
    display->getstringsize(string, &w, &h);

    if (w > display->lcdwidth || h > display->lcdheight) {
        rb->splash(0, string);
    } else {
        display->putsxy((display->lcdwidth - w) / 2,
                        (display->lcdheight - h) / 2,
                        string);
        display->update();
    }
}

void draw(struct screen * display) {
    char info[128];
    display->clear_display();

    int secs = rem_seconds();

    if (waiting)
        rb->snprintf(info, sizeof(info), "Next alarm in %02dh,"
                                         " %02dmn, and %02ds.",
                           secs / 3600, (secs / 60) % 60, secs % 60);
    else {
        if (current == 0)
            rb->snprintf(info, sizeof(info), "Set alarm at [%02d]:%02d.",
                                                                 alarm[0],
                                                                 alarm[1]);
        else
            rb->snprintf(info, sizeof(info), "Set alarm at %02d:[%02d].",
                                                                 alarm[0],
                                                                 alarm[1]);
    }
    draw_centered_string(display, info);
}

bool can_play(void) {
    int audio_status = rb->audio_status();
    if ((!audio_status && rb->global_status->resume_index != -1)
        && (rb->playlist_resume() != -1)) {
        return true;
    }
    else if (audio_status & AUDIO_STATUS_PAUSE)
        return true;

    return false;
}

void play(void) {
    int audio_status = rb->audio_status();
    if (!audio_status && rb->global_status->resume_index != -1) {
        if (rb->playlist_resume() != -1) {
            rb->playlist_start(rb->global_status->resume_index,
                rb->global_status->resume_offset);
        }
    }
    else if (audio_status & AUDIO_STATUS_PAUSE)
        rb->audio_resume();
}

enum plugin_status plugin_start(const void* parameter)
{
    int button;
    int i;
    (void)parameter;

    if (!can_play()) {
        rb->splash(4*HZ, "No track to resume! This plugin will resume a track "
                         "at a specific time. Therefore, you need to first play"
                         " one, and then pause it and start the plugin again.");
        quit = true;
    }

    while(!quit) {
        button = get_button();

        if (button == PLA_EXIT || button == PLA_CANCEL)
            quit = true;

        FOR_NB_SCREENS(i) {
            draw(rb->screens[i]);
        }
        if (waiting) {
            if (rem_seconds() <= 0) {
                quit = done = true;
                play();
            }
        }
        else {
            switch (button) {
                case PLA_UP:
                case PLA_UP_REPEAT:
#ifdef HAVE_SCROLLWHEEL
                case PLA_SCROLL_FWD:
                case PLA_SCROLL_FWD_REPEAT:
#endif
                    alarm[current] = (alarm[current] + 1) % maxval[current];
                    break;
                case PLA_DOWN:
                case PLA_DOWN_REPEAT:
#ifdef HAVE_SCROLLWHEEL
                case PLA_SCROLL_BACK:
                case PLA_SCROLL_BACK_REPEAT:
#endif
                    alarm[current] = (alarm[current] + maxval[current] - 1)
                        % maxval[current];
                    break;

                case PLA_LEFT:
                case PLA_LEFT_REPEAT:
                case PLA_RIGHT:
                case PLA_RIGHT_REPEAT:
                    current = (current + 1) % 2;
                    break;

                case PLA_SELECT:
                case PLA_SELECT_REPEAT: {
                    if (rem_seconds() < 0)
                        tomorrow = true;

                    waiting = true;
                    break;
                }

                default:
                    if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                        quit = usb = true;
                    break;
            }
        }
    }

    return (usb) ? PLUGIN_USB_CONNECTED
                 : (done ? PLUGIN_GOTO_WPS : PLUGIN_OK);
}

