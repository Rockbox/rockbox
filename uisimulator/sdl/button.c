/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Felix Arends
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

#include "uisdl.h"
#include "lcd-charcells.h"
#include "lcd-remote.h"
#include "config.h"
#include "button.h"
#include "kernel.h"
#include "backlight.h"
#include "misc.h"

#include "debug.h"

static intptr_t button_data; /* data value from last message dequeued */

#ifdef HAVE_TOUCHSCREEN
static int mouse_coords = 0;
static enum touchscreen_mode touchscreen_mode = TOUCHSCREEN_POINT;
void touchscreen_set_mode(enum touchscreen_mode mode)
{
    touchscreen_mode = mode;
}
enum touchscreen_mode touchscreen_get_mode(void)
{
    return touchscreen_mode;
}
#endif
/* how long until repeat kicks in */
#define REPEAT_START      6

/* the speed repeat starts at */
#define REPEAT_INTERVAL_START   4

/* speed repeat finishes at */
#define REPEAT_INTERVAL_FINISH  2

#if defined(IRIVER_H100_SERIES) || defined (IRIVER_H300_SERIES)
int _remote_type=REMOTETYPE_H100_LCD;

int remote_type(void)
{
    return _remote_type;
}
#endif

struct event_queue button_queue;

static int btn = 0;    /* Hopefully keeps track of currently pressed keys... */

#ifdef HAVE_BACKLIGHT
static bool filter_first_keypress;

void set_backlight_filter_keypress(bool value)
{
    filter_first_keypress = value;
}
#ifdef HAVE_REMOTE_LCD
static bool remote_filter_first_keypress;

void set_remote_backlight_filter_keypress(bool value)
{
    remote_filter_first_keypress = value;
}
#endif
#endif

#ifdef HAS_BUTTON_HOLD
bool hold_button_state = false;
bool button_hold(void) {
    return hold_button_state;
}
#endif

#ifdef HAS_REMOTE_BUTTON_HOLD
bool remote_hold_button_state = false;
bool remote_button_hold(void) {
    return remote_hold_button_state;
}
#endif

static int lastbtn;
void button_event(int key, bool pressed)
{
    int new_btn = 0;
    int diff = 0;
    int data = 0;
    static int count = 0;
    static int repeat_speed = REPEAT_INTERVAL_START;
    static int repeat_count = 0;
    static bool repeat = false;
    static bool post = false;
#ifdef HAVE_BACKLIGHT
    static bool skip_release = false;
#ifdef HAVE_REMOTE_LCD
    static bool skip_remote_release = false;
#endif
#endif 
    static bool usb_connected = false;
    if (usb_connected && key != SDLK_u)
        return;
    switch (key)
    {

#ifdef HAVE_TOUCHSCREEN
    case BUTTON_TOUCHSCREEN:
        data = mouse_coords;
        switch (touchscreen_mode)
        {
            case TOUCHSCREEN_POINT:
                new_btn = BUTTON_TOUCHSCREEN;
                break;
            case TOUCHSCREEN_BUTTON:
            {
                static int touchscreen_buttons[3][3] = {
                    {BUTTON_TOPLEFT, BUTTON_TOPMIDDLE, BUTTON_TOPRIGHT},
                    {BUTTON_MIDLEFT, BUTTON_CENTER, BUTTON_MIDRIGHT},
                    {BUTTON_BOTTOMLEFT, BUTTON_BOTTOMMIDDLE, BUTTON_BOTTOMRIGHT},
                };
                int px_x = ((data&0xffff0000)>>16), px_y = ((data&0x0000ffff));
                new_btn = touchscreen_buttons[px_y/(LCD_HEIGHT/3)][px_x/(LCD_WIDTH/3)];
                break;
            }
        }
        break;
    case SDLK_KP7:
        new_btn = BUTTON_TOPLEFT;
        break;
    case SDLK_KP8:
        new_btn = BUTTON_TOPMIDDLE;
        break;
    case SDLK_KP9:
        new_btn = BUTTON_TOPRIGHT;
        break;
    case SDLK_KP4:
        new_btn = BUTTON_MIDLEFT;
        break;
    case SDLK_KP5:
        new_btn = BUTTON_CENTER;
        break;
    case SDLK_KP6:
        new_btn = BUTTON_MIDRIGHT;
        break;
    case SDLK_KP1:
        new_btn = BUTTON_BOTTOMLEFT;
        break;
    case SDLK_KP2:
        new_btn = BUTTON_BOTTOMMIDDLE;
        break;
    case SDLK_KP3:
        new_btn = BUTTON_BOTTOMRIGHT;
        break;
    case SDLK_F4:
        if(pressed)
        {
            touchscreen_mode = (touchscreen_mode == TOUCHSCREEN_POINT ? TOUCHSCREEN_BUTTON : TOUCHSCREEN_POINT);
            printf("Touchscreen mode: %s\n", touchscreen_mode == TOUCHSCREEN_POINT ? "TOUCHSCREEN_POINT" : "TOUCHSCREEN_BUTTON");
        }
        break;
            
#endif
    case SDLK_u:
        if (!pressed)
        {
            usb_connected = !usb_connected;
            if (usb_connected)
                queue_post(&button_queue, SYS_USB_CONNECTED, 0);
            else
                queue_post(&button_queue, SYS_USB_DISCONNECTED, 0);
        }
        return;

#ifdef HAS_BUTTON_HOLD
    case SDLK_h:
        if(pressed)
        {
            hold_button_state = !hold_button_state;
            DEBUGF("Hold button is %s\n", hold_button_state?"ON":"OFF");
        }
        return;
#endif
        
#ifdef HAS_REMOTE_BUTTON_HOLD
    case SDLK_j:
        if(pressed)
        {
            remote_hold_button_state = !remote_hold_button_state;
            DEBUGF("Remote hold button is %s\n",
                   remote_hold_button_state?"ON":"OFF");
        }
        return;
#endif
        
#if CONFIG_KEYPAD == GIGABEAT_PAD
    case SDLK_KP4:
    case SDLK_LEFT:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_KP6:
    case SDLK_RIGHT:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_KP8:
    case SDLK_UP:
        new_btn = BUTTON_UP;
        break;
    case SDLK_KP2:
    case SDLK_DOWN:
        new_btn = BUTTON_DOWN;
        break;
    case SDLK_KP_PLUS:
    case SDLK_F8:
        new_btn = BUTTON_POWER;
        break;
    case SDLK_ESCAPE:
        new_btn = BUTTON_POWER;
        break;
    case SDLK_KP_ENTER:
    case SDLK_RETURN:
    case SDLK_a:
        new_btn = BUTTON_A;
        break;
    case SDLK_KP5:
    case SDLK_SPACE:
        new_btn = BUTTON_SELECT;
        break;
    case SDLK_KP_PERIOD:
    case SDLK_INSERT:
        new_btn = BUTTON_MENU;
        break;
    case SDLK_KP9:
        new_btn = BUTTON_VOL_UP;
        break;
    case SDLK_KP3:
        new_btn = BUTTON_VOL_DOWN;
        break;

#elif CONFIG_KEYPAD == GIGABEAT_S_PAD
    case SDLK_KP4:
    case SDLK_LEFT:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_KP6:
    case SDLK_RIGHT:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_KP8:
    case SDLK_UP:
        new_btn = BUTTON_UP;
        break;
    case SDLK_KP2:
    case SDLK_DOWN:
        new_btn = BUTTON_DOWN;
        break;
    case SDLK_F8:
    case SDLK_ESCAPE:
        new_btn = BUTTON_POWER;
        break;
    case SDLK_KP_PLUS:
    case SDLK_KP_ENTER:
    case SDLK_RETURN:
        new_btn = BUTTON_PLAY;
        break;
    case SDLK_KP7:
        new_btn = BUTTON_BACK;
        break;
    case SDLK_KP5:
    case SDLK_SPACE:
        new_btn = BUTTON_SELECT;
        break;
    case SDLK_KP9:
    case SDLK_KP_PERIOD:
    case SDLK_INSERT:
        new_btn = BUTTON_MENU;
        break;

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
    case SDLK_KP4:
    case SDLK_LEFT:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_KP6:
    case SDLK_RIGHT:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_KP8:
    case SDLK_UP:
        new_btn = BUTTON_UP;
        break;
    case SDLK_KP2:
    case SDLK_DOWN:
        new_btn = BUTTON_DOWN;
        break;
    case SDLK_KP_PLUS:
    case SDLK_F8:
        new_btn = BUTTON_PLAY;
        break;
    case SDLK_ESCAPE:
        new_btn = BUTTON_POWER;
        break;
    case SDLK_KP_ENTER:
    case SDLK_RETURN:
    case SDLK_a:
        new_btn = BUTTON_POWER;
        break;
    case SDLK_KP_DIVIDE:
    case SDLK_F1:
        new_btn = BUTTON_REC;
        break;
    case SDLK_KP5:
    case SDLK_SPACE:
        new_btn = BUTTON_SELECT;
        break;

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
    case SDLK_KP4:
    case SDLK_LEFT:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_KP6:
    case SDLK_RIGHT:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_KP8:
    case SDLK_UP:
        new_btn = BUTTON_VOL_UP;
        break;
    case SDLK_KP2:
    case SDLK_DOWN:
        new_btn = BUTTON_VOL_DOWN;
        break;
    case SDLK_KP_PERIOD:
    case SDLK_INSERT:
        new_btn = BUTTON_MODE;
        break;
    case SDLK_KP_DIVIDE:
    case SDLK_F1:
        new_btn = BUTTON_REC;
        break;
    case SDLK_KP5:
    case SDLK_SPACE:
        new_btn = BUTTON_PLAY;
        break;

#elif (CONFIG_KEYPAD == IPOD_1G2G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) \
   || (CONFIG_KEYPAD == IPOD_4G_PAD)
    case SDLK_KP4:
    case SDLK_LEFT:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_KP6:
    case SDLK_RIGHT:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_KP8:
    case SDLK_UP:
        new_btn = BUTTON_SCROLL_BACK;
        break;
    case SDLK_KP2:
    case SDLK_DOWN:
        new_btn = BUTTON_SCROLL_FWD;
        break;
    case SDLK_KP_PLUS:
    case SDLK_F8:
        new_btn = BUTTON_PLAY;
        break;
    case SDLK_KP5:
    case SDLK_SPACE:
        new_btn = BUTTON_SELECT;
        break;
    case SDLK_KP_PERIOD:
    case SDLK_INSERT:
        new_btn = BUTTON_MENU;
        break;

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
    case SDLK_KP4:
    case SDLK_LEFT:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_KP6:
    case SDLK_RIGHT:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_KP8:
    case SDLK_UP:
        new_btn = BUTTON_SCROLL_UP;
        break;
    case SDLK_KP2:
    case SDLK_DOWN:
        new_btn = BUTTON_SCROLL_DOWN;
        break;
    case SDLK_KP_PLUS:
    case SDLK_F8:
        new_btn = BUTTON_POWER;
        break;
    case SDLK_ESCAPE:
        new_btn = BUTTON_POWER;
        break;
    case SDLK_KP_DIVIDE:
    case SDLK_F1:
        new_btn = BUTTON_REW;
        break;
    case SDLK_KP_MULTIPLY:
    case SDLK_F2:
        new_btn = BUTTON_FF;
        break;
    case SDLK_KP5:
    case SDLK_SPACE:
        new_btn = BUTTON_PLAY;
        break;

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
    case SDLK_t:
        if(pressed)
            switch(_remote_type)
            {
                case REMOTETYPE_UNPLUGGED: 
                    _remote_type=REMOTETYPE_H100_LCD;
                    DEBUGF("Changed remote type to H100\n");
                    break;
                case REMOTETYPE_H100_LCD:
                    _remote_type=REMOTETYPE_H300_LCD;
                    DEBUGF("Changed remote type to H300\n");
                    break;
                case REMOTETYPE_H300_LCD:
                    _remote_type=REMOTETYPE_H300_NONLCD;
                    DEBUGF("Changed remote type to H300 NON-LCD\n");
                    break;
                case REMOTETYPE_H300_NONLCD:
                    _remote_type=REMOTETYPE_UNPLUGGED;
                    DEBUGF("Changed remote type to none\n");
                    break;
            }
        break;
    case SDLK_KP4:
    case SDLK_LEFT:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_KP6:
    case SDLK_RIGHT:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_KP8:
    case SDLK_UP:
        new_btn = BUTTON_UP;
        break;
    case SDLK_KP2:
    case SDLK_DOWN:
        new_btn = BUTTON_DOWN;
        break;
    case SDLK_KP_PLUS:
    case SDLK_F8:
        new_btn = BUTTON_ON;
        break;
    case SDLK_KP_ENTER:
    case SDLK_RETURN:
    case SDLK_a:
        new_btn = BUTTON_OFF;
        break;
    case SDLK_KP_DIVIDE:
    case SDLK_F1:
        new_btn = BUTTON_REC;
        break;
    case SDLK_KP5:
    case SDLK_SPACE:
        new_btn = BUTTON_SELECT;
        break;
    case SDLK_KP_PERIOD:
    case SDLK_INSERT:
        new_btn = BUTTON_MODE;
        break;

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
    case SDLK_KP4:
    case SDLK_LEFT:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_KP6:
    case SDLK_RIGHT:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_KP8:
    case SDLK_UP:
        new_btn = BUTTON_UP;
        break;
    case SDLK_KP2:
    case SDLK_DOWN:
        new_btn = BUTTON_DOWN;
        break;
    case SDLK_KP_PLUS:
    case SDLK_F8:
        new_btn = BUTTON_PLAY;
        break;
    case SDLK_KP_ENTER:
    case SDLK_RETURN:
    case SDLK_a:
       new_btn = BUTTON_EQ;
       break;
    case SDLK_KP5:
    case SDLK_SPACE:
        new_btn = BUTTON_SELECT;
        break;
    case SDLK_KP_PERIOD:
    case SDLK_INSERT:
        new_btn = BUTTON_MODE;
        break;

#elif CONFIG_KEYPAD == ONDIO_PAD
    case SDLK_KP4:
    case SDLK_LEFT:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_KP6:
    case SDLK_RIGHT:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_KP8:
    case SDLK_UP:
        new_btn = BUTTON_UP;
        break;
    case SDLK_KP2:
    case SDLK_DOWN:
        new_btn = BUTTON_DOWN;
        break;
    case SDLK_KP_ENTER:
    case SDLK_RETURN:
    case SDLK_a:
        new_btn = BUTTON_OFF;
        break;
    case SDLK_KP_PERIOD:
    case SDLK_INSERT:
        new_btn = BUTTON_MENU;
        break;

#elif CONFIG_KEYPAD == PLAYER_PAD
    case SDLK_KP4:
    case SDLK_LEFT:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_KP6:
    case SDLK_RIGHT:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_KP8:
    case SDLK_UP:
        new_btn = BUTTON_PLAY;
        break;
    case SDLK_KP2:
    case SDLK_DOWN:
        new_btn = BUTTON_STOP;
        break;
    case SDLK_KP_PLUS:
    case SDLK_F8:
        new_btn = BUTTON_ON;
        break;
    case SDLK_KP_PERIOD:
    case SDLK_INSERT:
        new_btn = BUTTON_MENU;
        break;

#elif CONFIG_KEYPAD == RECORDER_PAD
    case SDLK_KP4:
    case SDLK_LEFT:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_KP6:
    case SDLK_RIGHT:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_KP8:
    case SDLK_UP:
        new_btn = BUTTON_UP;
        break;
    case SDLK_KP2:
    case SDLK_DOWN:
        new_btn = BUTTON_DOWN;
        break;
    case SDLK_KP_PLUS:
    case SDLK_F8:
        new_btn = BUTTON_ON;
        break;
    case SDLK_KP_ENTER:
    case SDLK_RETURN:
    case SDLK_a:
        new_btn = BUTTON_OFF;
        break;
    case SDLK_KP_DIVIDE:
    case SDLK_F1:
        new_btn = BUTTON_F1;
        break;
    case SDLK_KP_MULTIPLY:
    case SDLK_F2:
        new_btn = BUTTON_F2;
        break;
    case SDLK_KP_MINUS:
    case SDLK_F3:
        new_btn = BUTTON_F3;
        break;
    case SDLK_KP5:
    case SDLK_SPACE:
        new_btn = BUTTON_PLAY;
        break;

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
    case SDLK_KP4:
    case SDLK_LEFT:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_KP6:
    case SDLK_RIGHT:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_KP8:
    case SDLK_UP:
        new_btn = BUTTON_UP;
        break;
    case SDLK_KP2:
    case SDLK_DOWN:
        new_btn = BUTTON_DOWN;
        break;
    case SDLK_KP_PLUS:
    case SDLK_F8:
        new_btn = BUTTON_ON;
        break;
    case SDLK_KP_ENTER:
    case SDLK_RETURN:
    case SDLK_a:
        new_btn = BUTTON_OFF;
        break;
    case SDLK_KP_DIVIDE:
    case SDLK_F1:
        new_btn = BUTTON_F1;
        break;
    case SDLK_KP_MULTIPLY:
    case SDLK_F2:
        new_btn = BUTTON_F2;
        break;
    case SDLK_KP_MINUS:
    case SDLK_F3:
        new_btn = BUTTON_F3;
        break;
    case SDLK_KP5:
    case SDLK_SPACE:
        new_btn = BUTTON_SELECT;
        break;

#elif CONFIG_KEYPAD == SANSA_E200_PAD
    case SDLK_KP4:
    case SDLK_LEFT:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_KP6:
    case SDLK_RIGHT:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_KP8:
    case SDLK_UP:
        new_btn = BUTTON_SCROLL_BACK;
        break;
    case SDLK_KP2:
    case SDLK_DOWN:
        new_btn = BUTTON_SCROLL_FWD;
        break;
    case SDLK_KP9:
        new_btn = BUTTON_UP;
        break;
    case SDLK_KP3:
        new_btn = BUTTON_DOWN;
        break;
    case SDLK_KP1:
        new_btn = BUTTON_POWER;
        break;
    case SDLK_KP7:
        new_btn = BUTTON_REC;
        break;
    case SDLK_KP5:
    case SDLK_SPACE:
        new_btn = BUTTON_SELECT;
        break;

#elif CONFIG_KEYPAD == SANSA_C200_PAD
    case SDLK_KP4:
    case SDLK_LEFT:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_KP6:
    case SDLK_RIGHT:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_KP8:
    case SDLK_UP:
        new_btn = BUTTON_UP;
        break;
    case SDLK_KP2:
    case SDLK_DOWN:
        new_btn = BUTTON_DOWN;
        break;
    case SDLK_KP3:
        new_btn = BUTTON_POWER;
        break;
    case SDLK_KP1:
        new_btn = BUTTON_REC;
        break;
    case SDLK_KP5:
    case SDLK_KP_ENTER:
    case SDLK_RETURN:
        new_btn = BUTTON_SELECT;
        break;
    case SDLK_KP7:
        new_btn = BUTTON_VOL_DOWN;
        break;
    case SDLK_KP9:
        new_btn = BUTTON_VOL_UP;
        break;
        
#elif CONFIG_KEYPAD == MROBE500_PAD
    case SDLK_F9:
        new_btn = BUTTON_RC_HEART;
        break;
    case SDLK_F10:
        new_btn = BUTTON_RC_MODE;
        break;
    case SDLK_F11:
        new_btn = BUTTON_RC_VOL_DOWN;
        break;
    case SDLK_F12:
        new_btn = BUTTON_RC_VOL_UP;
        break;
    case SDLK_LEFT:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_RIGHT:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_UP:
        new_btn = BUTTON_RC_PLAY;
        break;
    case SDLK_DOWN:
        new_btn = BUTTON_RC_DOWN;
        break;
    case SDLK_F8:
    case SDLK_ESCAPE:
        new_btn = BUTTON_POWER;
        break;
#elif CONFIG_KEYPAD == MROBE100_PAD
    case SDLK_KP1:
        new_btn = BUTTON_DISPLAY;
        break;
    case SDLK_KP7:
        new_btn = BUTTON_MENU;
        break;
    case SDLK_KP9:
        new_btn = BUTTON_PLAY;
        break;
    case SDLK_KP4:
    case SDLK_LEFT:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_KP6:
    case SDLK_RIGHT:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_KP8:
    case SDLK_UP:
        new_btn = BUTTON_UP;
        break;
    case SDLK_KP2:
    case SDLK_DOWN:
        new_btn = BUTTON_DOWN;
        break;
    case SDLK_KP5:
    case SDLK_SPACE:
        new_btn = BUTTON_SELECT;
        break;
    case SDLK_KP_MULTIPLY:
    case SDLK_F8:
    case SDLK_ESCAPE:
        new_btn = BUTTON_POWER;
        break;
    
#elif CONFIG_KEYPAD == COWOND2_PAD
    case SDLK_ESCAPE:
        new_btn = BUTTON_POWER;
        break;
    case SDLK_KP_PLUS:
        new_btn = BUTTON_PLUS;
        break;
    case SDLK_KP_MINUS:
        new_btn = BUTTON_MINUS;
        break;
    case SDLK_KP_ENTER:
        new_btn = BUTTON_MENU;
        break;

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD
    case SDLK_KP1:
        new_btn = BUTTON_BACK;
        break;
    case SDLK_KP3:
        new_btn = BUTTON_MENU;
        break;
    case SDLK_KP7:
        new_btn = BUTTON_CUSTOM;
        break;
    case SDLK_KP9:
        new_btn = BUTTON_PLAY;
        break;
    case SDLK_KP4:
    case SDLK_LEFT:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_KP6:
    case SDLK_RIGHT:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_KP8:
    case SDLK_UP:
        new_btn = BUTTON_UP;
        break;
    case SDLK_KP2:
    case SDLK_DOWN:
        new_btn = BUTTON_DOWN;
        break;
    case SDLK_KP5:
    case SDLK_SPACE:
        new_btn = BUTTON_SELECT;
        break;
    case SDLK_KP_MULTIPLY:
    case SDLK_F8:
    case SDLK_ESCAPE:
        new_btn = BUTTON_POWER;
        break;
#elif CONFIG_KEYPAD == CREATIVEZV_PAD
    case SDLK_KP1:
        new_btn = BUTTON_PREV;
        break;
    case SDLK_KP3:
        new_btn = BUTTON_NEXT;
        break;
    case SDLK_KP7:
        new_btn = BUTTON_BACK;
        break;
    case SDLK_p:
        new_btn = BUTTON_PLAY;
        break;
    case SDLK_KP9:
        new_btn = BUTTON_MENU;
        break;
    case SDLK_KP4:
    case SDLK_LEFT:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_KP6:
    case SDLK_RIGHT:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_KP8:
    case SDLK_UP:
        new_btn = BUTTON_UP;
        break;
    case SDLK_KP2:
    case SDLK_DOWN:
        new_btn = BUTTON_DOWN;
        break;
    case SDLK_KP5:
    case SDLK_SPACE:
        new_btn = BUTTON_SELECT;
        break;
    case SDLK_KP_MULTIPLY:
    case SDLK_F8:
    case SDLK_ESCAPE:
        new_btn = BUTTON_POWER;
        break;
    case SDLK_z:
        new_btn = BUTTON_VOL_DOWN;
        break;
    case SDLK_s:
        new_btn = BUTTON_VOL_UP;

#elif CONFIG_KEYPAD == MEIZU_M6SL_PAD
    case SDLK_KP1:
        new_btn = BUTTON_PREV;
        break;
    case SDLK_KP3:
        new_btn = BUTTON_NEXT;
        break;
    case SDLK_KP_ENTER:
    case SDLK_RETURN:
    case SDLK_a:
        new_btn = BUTTON_PLAY;
        break;
    case SDLK_KP_PERIOD:
    case SDLK_INSERT:
        new_btn = BUTTON_MENU;
        break;
    case SDLK_KP8:
    case SDLK_UP:
        new_btn = BUTTON_UP;
        break;
    case SDLK_KP2:
    case SDLK_DOWN:
        new_btn = BUTTON_DOWN;
        break;
    case SDLK_KP5:
    case SDLK_SPACE:
        new_btn = BUTTON_SELECT;
        break;
#else
#error No keymap defined!
#endif /* CONFIG_KEYPAD */
    case SDLK_KP0:
    case SDLK_F5:
        if(pressed)
        {
            queue_broadcast(SYS_SCREENDUMP, 0);
            return;
        }
        break;
    }
    
    if (pressed)
        btn |= new_btn;
    else
        btn &= ~new_btn;

    /* Lots of stuff copied from real button.c. Not good, I think... */

    /* Find out if a key has been released */
    diff = btn ^ lastbtn;
    if(diff && (btn & diff) == 0)
    {
#ifdef HAVE_BACKLIGHT
#ifdef HAVE_REMOTE_LCD
        if(diff & BUTTON_REMOTE)
            if(!skip_remote_release)
                queue_post(&button_queue, BUTTON_REL | diff, data);
            else
                skip_remote_release = false;
        else
#endif
            if(!skip_release)
                queue_post(&button_queue, BUTTON_REL | diff, data);
            else
                skip_release = false;
#else
        queue_post(&button_queue, BUTTON_REL | diff, data);
#endif
    }

    else
    {
        if ( btn )
        {
            /* normal keypress */
            if ( btn != lastbtn )
            {
                post = true;
                repeat = false;
                repeat_speed = REPEAT_INTERVAL_START;

            }
            else /* repeat? */
            {
                if ( repeat )
                {
                    if (!post)
                        count--;
                    if (count == 0)
                    {
                        post = true;
                        /* yes we have repeat */
                        repeat_speed--;
                        if (repeat_speed < REPEAT_INTERVAL_FINISH)
                            repeat_speed = REPEAT_INTERVAL_FINISH;
                        count = repeat_speed;

                        repeat_count++;
                    }
                }
                else
                {
                    if (count++ > REPEAT_START)
                    {
                        post = true;
                        repeat = true;
                        repeat_count = 0;
                        /* initial repeat */
                        count = REPEAT_INTERVAL_START;
                    }
                }
            }
            if ( post )
            {
                if(repeat)
                {
                    if (queue_empty(&button_queue))
                    {
                        queue_post(&button_queue, BUTTON_REPEAT | btn, data);
#ifdef HAVE_BACKLIGHT
#ifdef HAVE_REMOTE_LCD
                            if(btn & BUTTON_REMOTE)
                            {
                                if(skip_remote_release)
                                    skip_remote_release = false;
                            }
                            else
#endif                                    
                                if(skip_release)
                                    skip_release = false;
#endif
                        post = false;
                    }
                }
                else
                {
#ifdef HAVE_BACKLIGHT
#ifdef HAVE_REMOTE_LCD
                        if (btn & BUTTON_REMOTE) {
                            if (!remote_filter_first_keypress 
                                || is_remote_backlight_on(false))
                                queue_post(&button_queue, btn, data);
                            else
                                skip_remote_release = true;
                        }
                        else
#endif                                    
                            if (!filter_first_keypress 
                                || is_backlight_on(false))
                                queue_post(&button_queue, btn, data);
                            else
                                skip_release = true;
#else /* no backlight, nothing to skip */
                        queue_post(&button_queue, btn, data);
#endif
                    post = false;
                }    

#ifdef HAVE_REMOTE_LCD
                if(btn & BUTTON_REMOTE)
                    remote_backlight_on();
                else
#endif
                    backlight_on();

            }
        }
        else
        {
            repeat = false;
            count = 0;
        }
    }
    lastbtn = btn & ~(BUTTON_REL | BUTTON_REPEAT);
}

/* Again copied from real button.c... */

int button_queue_count( void )
{
    return queue_count(&button_queue);
}

long button_get(bool block)
{
    struct queue_event ev;

    if ( block || !queue_empty(&button_queue) ) {
        queue_wait(&button_queue, &ev);
        button_data = ev.data;
        return ev.id;
    }
    return BUTTON_NONE;
}

long button_get_w_tmo(int ticks)
{
    struct queue_event ev;
    queue_wait_w_tmo(&button_queue, &ev, ticks);
    if (ev.id == SYS_TIMEOUT)
        ev.id = BUTTON_NONE;
    else
        button_data = ev.data;

    return ev.id;
}

intptr_t button_get_data(void)
{
#ifdef HAVE_TOUCHSCREEN
    return button_data;
#else
    /* Needed by the accelerating wheel driver for Sansa e200 */
    return 1 << 24;
#endif
}

#ifdef HAVE_TOUCHSCREEN
extern bool debug_wps;
void mouse_tick_task(void)
{
    static int last_check = 0;
    int x,y;
    if (TIME_BEFORE(current_tick, last_check+(HZ/10)))
        return;
    last_check = current_tick;
    if (SDL_GetMouseState(&x, &y) & SDL_BUTTON(SDL_BUTTON_LEFT))
    {
        if(background)
        {
            x -= UI_LCD_POSX;
            y -= UI_LCD_POSY;
            
            if(x<0 || y<0 || x>UI_LCD_WIDTH || y>UI_LCD_HEIGHT)
                return;
        }
        
        mouse_coords = (x<<16)|y;
        button_event(BUTTON_TOUCHSCREEN, true);
        if (debug_wps)
            printf("Mouse at: (%d, %d)\n", x, y);
    }
}
#endif
void button_init(void)
{
#ifdef HAVE_TOUCHSCREEN
    tick_add_task(mouse_tick_task);
#endif
}

int button_status(void)
{
    return btn;
}

void button_clear_queue(void)
{
    queue_clear(&button_queue);
}

