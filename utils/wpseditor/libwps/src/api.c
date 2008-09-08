/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Rostilav Checkan
 *   $Id$
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

#include <stdio.h>
#include <stdlib.h>
#include "sound.h"
#include "api.h"
#include "proxy.h"
#include "dummies.h"
#include "scroll_engine.h"
#include "wpsstate.h"
#include <string.h>

struct proxy_api *xapi;

void get_current_vp(struct viewport_api *avp);
/*************************************************************

*************************************************************/
#ifdef HAVE_LCD_BITMAP
void screen_clear_area(struct screen * display, int xstart, int ystart,
                       int width, int height) {
    display->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    display->fillrect(xstart, ystart, width, height);
    display->set_drawmode(DRMODE_SOLID);
}
#endif

bool load_wps_backdrop(char* filename) {
    return xapi->load_wps_backdrop(filename);
}

bool load_remote_wps_backdrop(char* filename) {
    return xapi->load_remote_wps_backdrop(filename);
}

int read_bmp_file(const char* filename,struct bitmap *bm, int maxsize,int format) {
    if (!xapi->read_bmp_file) {
        DEBUGF1("can't read bmp file! NULL api!\n");
        return -1;
    }
    bm->format = 3;//FORMAT_ANY?
    bm->data = (unsigned char*)malloc(255);
    memset(bm->data,0,255);
    strcpy((char*)bm->data,filename);
    //bm->data[strlen(filename)] = '\0';
    xapi->read_bmp_file(filename,&bm->width, &bm->height);
    return 1;
}

bool load_wps_backdrop2(char* filename) {
    DEBUGF1("load_wps_backdrop(char* filename='%s')",filename);
    return true;
}

bool load_remote_wps_backdrop2(char* filename) {
    DEBUGF1("load_remote_wps_backdrop2(char* filename='%s')",filename);
    return true;
}

void stop_scroll() {
    DEBUGF3("stop_scroll\n");
    return;
}

void puts_scroll(int x, int y, const unsigned char *string) {
    DEBUGF2("puts_scroll(int x=%d, int y=%d, const unsigned char *string='%s'\n",x,y,string);
}

void putsxy(int x, int y, const unsigned char *str) {
    DEBUGF2("putsxy(int =%d, int y=%d, const unsigned char *str='%s')\n",x,y,str);
}

void lcd_update() {
    DEBUGF3("update\n");
}

void clear_viewport(int x, int y, int w, int h, int color) {
    DEBUGF3("clear_viewport(int x=%d, int y=%d, int w=%d, int h=%d, int color=%d)\n", x, y, w, h, color);
};

int getstringsize(const unsigned char *str, int *w, int *h) {
    //DEBUGF1("getstringsize(const unsigned char *str=\"%s\", int *w=%d, int *h=%d \n",str,*w,*h);
    *w=strlen((char*)str)*sysfont.maxwidth;
    *h=sysfont.height;
    return 1;
}

void set_wpsstate(struct wpsstate state) {
    sysfont.height = state.fontheight;
    sysfont.maxwidth = state.fontwidth;
    global_settings.volume = state.volume;
    battery_percent = state.battery_level;
    _audio_status = state.audio_status;
}

void set_trackstate(struct trackstate state) {
    if (!(gui_wps[0].state) ||
        !(gui_wps[0].state->id3))
        return;
    gui_wps[0].state->id3->title = state.title;
    gui_wps[0].state->id3->artist = state.artist;
    gui_wps[0].state->id3->album = state.album;
    gui_wps[0].state->id3->elapsed = state.elapsed;
    gui_wps[0].state->id3->length = state.length;
}

void set_next_trackstate(struct trackstate state) {
    gui_wps[0].state->nid3->title = state.title;
    gui_wps[0].state->nid3->artist = state.artist;
    gui_wps[0].state->nid3->album = state.album;
    gui_wps[0].state->nid3->elapsed = state.elapsed;
    gui_wps[0].state->nid3->length = state.length;
}

enum api_playmode playmodes[PLAYMODES_NUM] = {
            API_STATUS_PLAY,
            API_STATUS_STOP,
            API_STATUS_PAUSE,
            API_STATUS_FASTFORWARD,
            API_STATUS_FASTBACKWARD
        };

const char *playmodeNames[] = {
                                  "Play", "Stop", "Pause", "FastForward", "FastBackward"
                              };


void set_audio_status(int status) {
    DEBUGF1("%s",playmodeNames[status]);
    switch (status) {
        case API_STATUS_PLAY:
            _audio_status = AUDIO_STATUS_PLAY;
            status_set_ffmode(STATUS_PLAY);
            break;
        case API_STATUS_STOP:
            _audio_status = 0;
            status_set_ffmode(STATUS_STOP);
            break;
        case API_STATUS_PAUSE:
            _audio_status = AUDIO_STATUS_PAUSE;
            status_set_ffmode(STATUS_PLAY);
            break;
        case API_STATUS_FASTFORWARD:
            status_set_ffmode(STATUS_FASTFORWARD);
            break;
        case API_STATUS_FASTBACKWARD:
            status_set_ffmode(STATUS_FASTBACKWARD);
            break;
        default:
            DEBUGF1("ERR: Unknown status");
    }
}

void test_api(struct proxy_api *api) {
    if (!api->stop_scroll)
        api->stop_scroll=stop_scroll;
    if (!api->set_viewport)
        api->set_viewport=lcd_set_viewport;
    if (!api->clear_viewport)
        api->clear_viewport=clear_viewport;
    if (!api->getstringsize)
        api->getstringsize=getstringsize;
    if (!api->getwidth)
        api->getwidth=lcd_getwidth;
    if (!api->getheight)
        api->getheight=lcd_getheight;
    if (!api->set_drawmode)
        api->set_drawmode=lcd_set_drawmode;
    if (!api->puts_scroll)
        api->puts_scroll=puts_scroll;
    if (!api->update)
        api->update=lcd_update;
    if (!api->clear_display)
        api->clear_display=lcd_clear_display;
    if (!api->getfont)
        api->getfont=lcd_getfont;
    if (!api->putsxy)
        api->putsxy=putsxy;

#if LCD_DEPTH > 1
    if (!api->get_foreground)
        api->get_foreground=lcd_get_foreground;
    if (!api->get_background)
        api->get_background=lcd_get_background;
#endif
    if (!api->load_remote_wps_backdrop)
        api->load_remote_wps_backdrop = load_remote_wps_backdrop2;
    if (!api->load_wps_backdrop)
        api->load_wps_backdrop = load_wps_backdrop2;
    //dbgf = printf;
}

/**************************************************************

**************************************************************/

int set_api(struct proxy_api* api) {
    if (api->debugf)
        dbgf = api->debugf;
    screens[0].screen_type=SCREEN_MAIN;
    screens[0].lcdwidth=LCD_WIDTH;
    screens[0].lcdheight=LCD_HEIGHT;
    screens[0].depth=LCD_DEPTH;
#ifdef HAVE_LCD_COLOR
    screens[0].is_color=true;
#else
    screens[0].is_color=false;
#endif
    if (api->stop_scroll)
        screens[0].stop_scroll=api->stop_scroll;
    screens[0].scroll_stop = lcd_scroll_stop;
    if (api->set_viewport)
        screens[0].set_viewport=api->set_viewport;
    if (api->clear_viewport)
        screens[0].clear_viewport=lcd_clear_viewport;
    if (api->getstringsize)
        screens[0].getstringsize=api->getstringsize;
    if (api->getwidth)
        screens[0].getwidth=api->getwidth;
    if (api->getheight)
        screens[0].getheight=api->getheight;
    if (api->set_drawmode)
        screens[0].set_drawmode=api->set_drawmode;
    if (api->fillrect)
        screens[0].fillrect=api->fillrect;
    if (api->puts_scroll)
        screens[0].puts_scroll=api->puts_scroll;
    if (api->transparent_bitmap_part)
        screens[0].transparent_bitmap_part=api->transparent_bitmap_part;
    if (api->update)
        screens[0].update=api->update;
    if (api->clear_display)
        screens[0].clear_display=api->clear_display;
    if (api->getfont)
        screens[0].getfont=api->getfont;
    if (api->hline)
        screens[0].hline=api->hline;
    if (api->vline)
        screens[0].vline=api->vline;
    if (api->drawpixel)
        screens[0].drawpixel=api->drawpixel;
    if (api->putsxy)
        screens[0].putsxy=api->putsxy;
#if LCD_DEPTH > 1
    if (api->get_foreground)
        screens[0].get_foreground=api->get_foreground;
    if (api->get_background)
        screens[0].get_background=api->get_background;
#endif

    screens[0].bitmap_part = api->bitmap_part;
    /**************************
    *          OUT            *
    **************************/
    api->get_model_name = get_model_name;
    api->get_current_vp = get_current_vp;
    api->set_wpsstate = set_wpsstate;
    api->set_trackstate = set_trackstate;
    api->set_next_trackstate= set_next_trackstate;
    api->set_audio_status= set_audio_status;
    xapi = api;
    return 0;
}







