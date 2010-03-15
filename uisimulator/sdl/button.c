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
#include "sim_tasks.h"
#include "button-sdl.h"
#include "backlight.h"

#include "debug.h"

#ifdef HAVE_TOUCHSCREEN
#include "touchscreen.h"
static int mouse_coords = 0;
#endif
/* how long until repeat kicks in */
#define REPEAT_START      6

/* the speed repeat starts at */
#define REPEAT_INTERVAL_START   4

/* speed repeat finishes at */
#define REPEAT_INTERVAL_FINISH  2

#ifdef HAVE_TOUCHSCREEN
#define USB_KEY SDLK_c /* SDLK_u is taken by BUTTON_MIDLEFT */
#else
#define USB_KEY SDLK_u
#endif

#if defined(IRIVER_H100_SERIES) || defined (IRIVER_H300_SERIES)
int _remote_type=REMOTETYPE_H100_LCD;

int remote_type(void)
{
    return _remote_type;
}
#endif

struct event_queue button_queue;

static int btn = 0;    /* Hopefully keeps track of currently pressed keys... */

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

void button_event(int key, bool pressed)
{
    int new_btn = 0;
    static bool usb_connected = false;
    if (usb_connected && key != USB_KEY)
        return;
    switch (key)
    {

#ifdef HAVE_TOUCHSCREEN
    case BUTTON_TOUCHSCREEN:
        switch (touchscreen_get_mode())
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
                int px_x = ((mouse_coords&0xffff0000)>>16);
                int px_y = ((mouse_coords&0x0000ffff));
                new_btn = touchscreen_buttons[px_y/(LCD_HEIGHT/3)][px_x/(LCD_WIDTH/3)];
                break;
            }
        }
        break;
    case SDLK_KP7:
    case SDLK_7:
        new_btn = BUTTON_TOPLEFT;
        break;
    case SDLK_KP8:
    case SDLK_8:
    case SDLK_UP:
        new_btn = BUTTON_TOPMIDDLE;
        break;
    case SDLK_KP9:
    case SDLK_9:
        new_btn = BUTTON_TOPRIGHT;
        break;
    case SDLK_KP4:
    case SDLK_u:
    case SDLK_LEFT:
        new_btn = BUTTON_MIDLEFT;
        break;
    case SDLK_KP5:
    case SDLK_i:
        new_btn = BUTTON_CENTER;
        break;
    case SDLK_KP6:
    case SDLK_o:
    case SDLK_RIGHT:
        new_btn = BUTTON_MIDRIGHT;
        break;
    case SDLK_KP1:
    case SDLK_j:
        new_btn = BUTTON_BOTTOMLEFT;
        break;
    case SDLK_KP2:
    case SDLK_k:
    case SDLK_DOWN:
        new_btn = BUTTON_BOTTOMMIDDLE;
        break;
    case SDLK_KP3:
    case SDLK_l:
        new_btn = BUTTON_BOTTOMRIGHT;
        break;
    case SDLK_F4:
        if(pressed)
        {
            touchscreen_set_mode(touchscreen_get_mode() == TOUCHSCREEN_POINT ? TOUCHSCREEN_BUTTON : TOUCHSCREEN_POINT);
            printf("Touchscreen mode: %s\n", touchscreen_get_mode() == TOUCHSCREEN_POINT ? "TOUCHSCREEN_POINT" : "TOUCHSCREEN_BUTTON");
        }
        break;
            
#endif
    case USB_KEY:
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
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_LEFT:
        new_btn = BUTTON_RC_REW;
        break;
    case SDLK_KP6:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_RIGHT:
        new_btn = BUTTON_RC_FF;
        break;
    case SDLK_KP8:
        new_btn = BUTTON_VOL_UP;
        break;
    case SDLK_UP:
        new_btn = BUTTON_RC_VOL_UP;
        break;
    case SDLK_KP2:
        new_btn = BUTTON_VOL_DOWN;
        break;
    case SDLK_DOWN:
        new_btn = BUTTON_RC_VOL_DOWN;
        break;
    case SDLK_KP_PERIOD:
        new_btn = BUTTON_MODE;
        break;
    case SDLK_INSERT:
        new_btn = BUTTON_RC_MODE;
        break;
    case SDLK_KP_DIVIDE:
        new_btn = BUTTON_REC;
        break;
    case SDLK_F1:
        new_btn = BUTTON_RC_REC;
        break;
    case SDLK_KP5:
        new_btn = BUTTON_PLAY;
        break;
    case SDLK_SPACE:
        new_btn = BUTTON_RC_PLAY;
        break;
    case SDLK_KP_ENTER:
    case SDLK_RETURN:
        new_btn = BUTTON_RC_MENU;
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
    case SDLK_PAGEUP:
        new_btn = BUTTON_UP;
        break;
    case SDLK_KP3:
    case SDLK_PAGEDOWN:
        new_btn = BUTTON_DOWN;
        break;
    case SDLK_KP1:
    case SDLK_HOME:
        new_btn = BUTTON_POWER;
        break;
    case SDLK_KP7:
    case SDLK_END:
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
    case SDLK_MINUS:
    case SDLK_LESS:
    case SDLK_LEFTBRACKET:
    case SDLK_KP_DIVIDE:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_PLUS:
    case SDLK_GREATER:
    case SDLK_RIGHTBRACKET:
    case SDLK_KP_MULTIPLY:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_PAGEUP:
        new_btn = BUTTON_RC_PLAY;
        break;
    case SDLK_PAGEDOWN:
        new_btn = BUTTON_RC_DOWN;
        break;
    case SDLK_F8:
    case SDLK_ESCAPE:
        new_btn = BUTTON_POWER;
        break;
#elif CONFIG_KEYPAD == MROBE100_PAD
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
        new_btn = BUTTON_RC_FF;
        break;
    case SDLK_RIGHT:
        new_btn = BUTTON_RC_REW;
        break;
    case SDLK_UP:
        new_btn = BUTTON_RC_PLAY;
        break;
    case SDLK_DOWN:
        new_btn = BUTTON_RC_DOWN;
        break;
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
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_KP6:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_KP8:
        new_btn = BUTTON_UP;
        break;
    case SDLK_KP2:
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
    
#elif CONFIG_KEYPAD == COWON_D2_PAD
    case SDLK_KP_MULTIPLY:
    case SDLK_F8:
    case SDLK_ESCAPE:
    case SDLK_BACKSPACE:
    case SDLK_DELETE:
        new_btn = BUTTON_POWER;
        break;
    case SDLK_KP_PLUS:
    case SDLK_EQUALS:
        new_btn = BUTTON_PLUS;
        break;
    case SDLK_KP_MINUS:
    case SDLK_MINUS:
        new_btn = BUTTON_MINUS;
        break;
    case SDLK_KP_ENTER:
    case SDLK_RETURN:
    case SDLK_SPACE:
    case SDLK_INSERT:
        new_btn = BUTTON_MENU;
        break;
#elif CONFIG_KEYPAD == IAUDIO67_PAD
    case SDLK_UP:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_DOWN:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_LEFT:
        new_btn = BUTTON_STOP;
        break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
    case SDLK_RIGHT:
        new_btn = BUTTON_PLAY;
        break;
    case SDLK_PLUS:
        new_btn = BUTTON_VOLUP;
        break;
    case SDLK_MINUS:
        new_btn = BUTTON_VOLDOWN;
        break;
    case SDLK_SPACE:
        new_btn = BUTTON_MENU;
        break;
    case SDLK_BACKSPACE:
        new_btn = BUTTON_POWER;
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
#elif CONFIG_KEYPAD == SANSA_FUZE_PAD
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
    case SDLK_PAGEUP:
    case SDLK_KP9:
        new_btn = BUTTON_UP;
        break;
    case SDLK_PAGEDOWN:
    case SDLK_KP3:
        new_btn = BUTTON_DOWN;
        break;
    case SDLK_KP_MINUS:
    case SDLK_KP1:
        new_btn = BUTTON_POWER;
        break;
    case SDLK_KP_MULTIPLY:
        new_btn = BUTTON_HOME;
        break;
    case SDLK_KP5:
    case SDLK_SPACE:
    case SDLK_KP_ENTER:
    case SDLK_RETURN:
        new_btn = BUTTON_SELECT;
        break;
#elif CONFIG_KEYPAD == SANSA_CLIP_PAD
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
    
    case SDLK_INSERT:
    case SDLK_KP_MULTIPLY:
        new_btn = BUTTON_HOME;
        break;
    case SDLK_SPACE:
    case SDLK_KP5:
        new_btn = BUTTON_SELECT;
        break;
    case SDLK_PAGEDOWN:
    case SDLK_KP3:
        new_btn = BUTTON_VOL_DOWN;
        break;
    case SDLK_PAGEUP:
    case SDLK_KP9:
        new_btn = BUTTON_VOL_UP;
        break;
    case SDLK_ESCAPE:
    case SDLK_KP1:
        new_btn = BUTTON_POWER;
        break;
#elif CONFIG_KEYPAD == SANSA_M200_PAD
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
    case SDLK_PLUS:
        new_btn = BUTTON_POWER;
        break;
    case SDLK_KP5:
        new_btn = BUTTON_SELECT;
        break;
    case SDLK_KP7:
        new_btn = BUTTON_VOL_DOWN;
        break;
    case SDLK_KP9:
        new_btn = BUTTON_VOL_UP;
        break;
#elif CONFIG_KEYPAD == PHILIPS_SA9200_PAD
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
    case SDLK_KP1:
        new_btn = BUTTON_MENU;
        break;
    case SDLK_KP5:
    case SDLK_SPACE:
        new_btn = BUTTON_PLAY;
        break;
    case SDLK_KP7:
        new_btn = BUTTON_PREV;
        break;
    case SDLK_KP9:
        new_btn = BUTTON_NEXT;
        break;
    case SDLK_KP_ENTER:
    case SDLK_RETURN:
        new_btn = BUTTON_POWER;
        break;
    case SDLK_PAGEUP:
        new_btn = BUTTON_VOL_UP;
        break;
    case SDLK_PAGEDOWN:
        new_btn = BUTTON_VOL_DOWN;
        break;
#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD
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
    case SDLK_KP7:
    case SDLK_ESCAPE:
        new_btn = BUTTON_POWER;
        break;
    case SDLK_KP1:
        new_btn = BUTTON_PLAYLIST;
        break;
    case SDLK_KP9:
        new_btn = BUTTON_VOL_UP;
        break;
    case SDLK_KP3:
        new_btn = BUTTON_VOL_DOWN;
        break;
    case SDLK_KP_MINUS:
        new_btn = BUTTON_MENU;
        break;
    case SDLK_KP_PLUS:
        new_btn = BUTTON_VIEW;
        break;
#elif CONFIG_KEYPAD == ONDAVX747_PAD
    case SDLK_ESCAPE:
        new_btn = BUTTON_POWER;
        break;
    case SDLK_KP_PLUS:
    case SDLK_PLUS:
    case SDLK_GREATER:
    case SDLK_RIGHTBRACKET:
    case SDLK_KP_MULTIPLY:
        new_btn = BUTTON_VOL_UP;
        break;
    case SDLK_KP_MINUS:
    case SDLK_MINUS:
    case SDLK_LESS:
    case SDLK_LEFTBRACKET:
    case SDLK_KP_DIVIDE:
        new_btn = BUTTON_VOL_DOWN;
        break;
    case SDLK_KP_ENTER:
    case SDLK_RETURN:
        new_btn = BUTTON_MENU;
        break;
#elif CONFIG_KEYPAD == ONDAVX777_PAD
    case SDLK_ESCAPE:
        new_btn = BUTTON_POWER;
        break;
#elif CONFIG_KEYPAD == SAMSUNG_YH_PAD
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
    case SDLK_KP_ENTER:
        new_btn = BUTTON_PLAY;
        break;
    case SDLK_KP9:
    case SDLK_PAGEUP:
        new_btn = BUTTON_FFWD;
        break;
#ifdef SAMSUNG_YH820
    case SDLK_KP7:
#else
    case SDLK_KP3:
#endif
    case SDLK_PAGEDOWN:
        new_btn = BUTTON_REW;
        break;
    case SDLK_KP_PLUS:
        new_btn = BUTTON_REC;
        break;
#elif CONFIG_KEYPAD == MINI2440_PAD
    case SDLK_LEFT:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_RIGHT:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_UP:
        new_btn = BUTTON_UP;
        break;
    case SDLK_DOWN:
        new_btn = BUTTON_DOWN;
        break;
    case SDLK_F8:
    case SDLK_ESCAPE:
        new_btn = BUTTON_POWER;
        break;
    case SDLK_KP_ENTER:
    case SDLK_RETURN:
    case SDLK_a:
        new_btn = BUTTON_A;
        break;
    case SDLK_SPACE:
        new_btn = BUTTON_SELECT;
        break;
    case SDLK_KP_PERIOD:
    case SDLK_INSERT:
        new_btn = BUTTON_MENU;
        break;
    case SDLK_KP_PLUS:
        new_btn = BUTTON_VOL_UP;
        break;
    case SDLK_KP_MINUS:
        new_btn = BUTTON_VOL_DOWN;
        break;
#elif CONFIG_KEYPAD == PBELL_VIBE500_PAD
    case SDLK_KP4:
    case SDLK_LEFT:
        new_btn = BUTTON_PREV;
        break;
    case SDLK_KP6:
    case SDLK_RIGHT:
        new_btn = BUTTON_NEXT;
        break;
    case SDLK_KP8:
    case SDLK_UP:
        new_btn = BUTTON_UP;
        break;
    case SDLK_KP2:
    case SDLK_DOWN:
        new_btn = BUTTON_DOWN;
        break;
    case SDLK_KP7:
        new_btn = BUTTON_MENU;
        break;
    case SDLK_KP9:
        new_btn = BUTTON_PLAY;
        break;
    case SDLK_KP5:
        new_btn = BUTTON_OK;
        break;
    case SDLK_KP_DIVIDE:
        new_btn = BUTTON_CANCEL;
        break;
    case SDLK_KP_PLUS:
        new_btn = BUTTON_POWER;
        break;
    case SDLK_KP_MULTIPLY:
        new_btn = BUTTON_REC;
        break;
#else
#error No keymap defined!
#endif /* CONFIG_KEYPAD */
    case SDLK_KP0:
    case SDLK_F5:
        if(pressed)
        {
            sim_trigger_screendump();
            return;
        }
        break;
    }
    
    /* Call to make up for scrollwheel target implementation.  This is
     * not handled in the main button.c driver, but on the target
     * implementation (look at button-e200.c for example if you are trying to 
     * figure out why using button_get_data needed a hack before).
     */
#if defined(BUTTON_SCROLL_FWD) && defined(BUTTON_SCROLL_BACK)
    if((new_btn == BUTTON_SCROLL_FWD || new_btn == BUTTON_SCROLL_BACK) && 
        pressed)
    {
        /* Clear these buttons from the data - adding them to the queue is
         *  handled in the scrollwheel drivers for the targets.  They do not
         *  store the scroll forward/back buttons in their button data for
         *  the button_read call.
         */
#ifdef HAVE_BACKLIGHT
        backlight_on();
#endif
#ifdef HAVE_BUTTON_LIGHT
        buttonlight_on();
#endif
        queue_post(&button_queue, new_btn, 1<<24);
        new_btn &= ~(BUTTON_SCROLL_FWD | BUTTON_SCROLL_BACK);
    }
#endif

    if (pressed)
        btn |= new_btn;
    else
        btn &= ~new_btn;
}
#if defined(HAVE_BUTTON_DATA) && defined(HAVE_TOUCHSCREEN)
int button_read_device(int* data)
{
    *data = mouse_coords;
#else
int button_read_device(void)
{
#endif

#ifdef HAS_BUTTON_HOLD
    int hold_button = button_hold();

#ifdef HAVE_BACKLIGHT
    /* light handling */
    static int hold_button_old = false;
    if (hold_button != hold_button_old)
    {
        hold_button_old = hold_button;
        backlight_hold_changed(hold_button);
    }
#endif

    if (hold_button)
        return BUTTON_NONE;
#endif

    return btn;
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
            
            if(x<0 || y<0 || x>SIM_LCD_WIDTH || y>SIM_LCD_HEIGHT)
                return;
        }
        
        mouse_coords = (x<<16)|y;
        button_event(BUTTON_TOUCHSCREEN, true);
        if (debug_wps)
            printf("Mouse at: (%d, %d)\n", x, y);
    }
}
#endif

void button_init_sdl(void)
{
#ifdef HAVE_TOUCHSCREEN
    tick_add_task(mouse_tick_task);
#endif
}

/* Button maps: simulated key, x, y, radius, name */
/* Run sim with --mapping to get coordinates      */
/* or --debugbuttons to check                     */
/* The First matching button is returned          */

#ifdef SANSA_FUZE
struct button_map bm[] = {
    { SDLK_KP8,          70, 265, 35, "Scroll Back" },
    { SDLK_KP9,         141, 255, 31, "Play" },
    { SDLK_KP_MULTIPLY, 228, 267, 18, "Home" },
    { SDLK_LEFT,         69, 329, 31, "Left" },
    { SDLK_SPACE,       141, 330, 20, "Select" },
    { SDLK_RIGHT,       214, 331, 23, "Right" },
    { SDLK_KP3,         142, 406, 30, "Menu" },
    { SDLK_DOWN,        221, 384, 24, "Scroll Fwd" },
    { SDLK_KP_MINUS,    270, 299, 25, "Power" },
    { SDLK_h,           269, 358, 26, "Hold" },
    { 0, 0, 0, 0, "None" }
};
#elif defined (SANSA_CLIP)
struct button_map bm[] = {
    { SDLK_KP_MULTIPLY, 165, 158,  17, "Home" },
    { SDLK_KP5,         102, 230,  29, "Select" },
    { SDLK_KP8,         100, 179,  25, "Play" },
    { SDLK_KP4,          53, 231,  21, "Left" },
    { SDLK_KP6,         147, 232,  19, "Right" },
    { SDLK_KP2,         105, 275,  22, "Menu" },
    { 0, 0, 0, 0, "None" }
};
#elif defined (SANSA_C200) || defined(SANSA_C200V2)
struct button_map bm[] = {
    
    { SDLK_KP7,      84,   7, 21, "Vol Down" },
    { SDLK_KP9,     158,   7, 20, "Vol Up" },
    { SDLK_KP1,     173, 130, 27, "Record" },
    { SDLK_KP5,     277,  75, 21, "Select" },
    { SDLK_KP4,     233,  75, 24, "Left" },
    { SDLK_KP6,     313,  74, 18, "Right" },
    { SDLK_KP8,     276,  34, 15, "Play" },
    { SDLK_KP2,     277, 119, 17, "Down" },
    { SDLK_KP3,     314, 113, 19, "Menu" },
    { 0, 0, 0, 0, "None" }
};
#elif defined (SANSA_E200V2) || defined(SANSA_E200)
struct button_map bm[] = {
    { SDLK_KP7,         5,  92, 18, "Record" },
    { SDLK_KP9,       128, 295, 43, "Play" },
    { SDLK_KP4,        42, 380, 33, "Left" },
    { SDLK_KP5,       129, 378, 36, "Select" },
    { SDLK_KP6,       218, 383, 30, "Right" },
    { SDLK_KP3,       129, 461, 29, "Down" },
    { SDLK_KP1,        55, 464, 20, "Menu" },
    { SDLK_KP8,        92, 338, 17, "Scroll Back" },
    { SDLK_KP2,       167, 342, 17, "Scroll Fwd"  },
    { 0, 0, 0, 0, "None" }
};
#elif defined (SANSA_M200V4)
struct button_map bm[] = {
    { SDLK_KP_PLUS, 54,  14, 16, "Power" },
    { SDLK_KP7,     96,  13, 12, "Vol Down" },
    { SDLK_KP9,    139,  14, 14, "Vol Up" },
    { SDLK_KP5,    260,  82, 20, "Select" },
    { SDLK_KP8,    258,  35, 30, "Play" },
    { SDLK_KP4,    214,  84, 25, "Left" },
    { SDLK_KP6,    300,  83, 24, "Right" },
    { SDLK_KP2,    262, 125, 28, "Repeat" },
    { SDLK_h,      113, 151, 21, "Hold" },
    { 0, 0, 0, 0, "None" }
};
#elif defined (IPOD_VIDEO)
struct button_map bm[] = {
    { SDLK_KP_PERIOD,  174, 350, 35, "Menu" },
    { SDLK_KP8,        110, 380, 33, "Scroll Back" },
    { SDLK_KP2,        234, 377, 34, "Scroll Fwd" },
    { SDLK_KP4,         78, 438, 47, "Left" },
    { SDLK_KP5,        172, 435, 43, "Select" },
    { SDLK_KP6,        262, 438, 52, "Right" },
    { SDLK_KP_PLUS,    172, 519, 43, "Play" },
    { 0, 0, 0 , 0, "None" }
};
#elif defined (IPOD_MINI) || defined(IPOD_MINI2G)
struct button_map bm[] = {
    { SDLK_KP5,       92, 267, 29, "Select" },
    { SDLK_KP4,       31, 263, 37, "Left" },
    { SDLK_KP6,      150, 268, 33, "Right" },
    { SDLK_KP_PERIOD, 93, 209, 30, "Menu" },
    { SDLK_KP_PLUS,   93, 324, 25, "Play" },
    { SDLK_KP8,       53, 220, 29, "Scroll Back" },
    { SDLK_KP2,      134, 219, 31, "Scroll Fwd" },
    { 0, 0, 0 , 0, "None" }
};
#elif defined (IPOD_3G)
struct button_map bm[] = {
    { SDLK_KP5,       108, 296, 26, "Select" },
    { SDLK_KP8,        70, 255, 26, "Scroll Back" },
    { SDLK_KP2,       149, 256, 28, "Scroll Fwd" },
    { SDLK_KP4,        27, 186, 22, "Left" },
    { SDLK_KP_PERIOD,  82, 185, 22, "Menu" },
    { SDLK_KP_PERIOD, 133, 185, 21, "Play" },
    { SDLK_KP6,       189, 188, 21, "Right" },
    { 0, 0, 0 , 0, "None" }
};
#elif defined (IPOD_4G)
struct button_map bm[] = {
    { SDLK_KP5,        96, 269, 27, "Select" },
    { SDLK_KP4,        39, 267, 30, "Left" },
    { SDLK_KP6,       153, 270, 27, "Right" },
    { SDLK_KP_PERIOD,  96, 219, 30, "Menu" },
    { SDLK_KP_PLUS,    95, 326, 27, "Play" },
    { SDLK_KP8,        57, 233, 29, "Scroll Back" },
    { SDLK_KP2,       132, 226, 29, "Scroll Fwd" },
    { 0, 0, 0 , 0, "None" }
};
#elif defined (IPOD_COLOR) 
struct button_map bm[] = {
    { SDLK_KP5,        128, 362, 35, "Select" },
    { SDLK_KP4,         55, 358, 38, "Left" },
    { SDLK_KP6,        203, 359, 39, "Right" },
    { SDLK_KP_PERIOD,  128, 282, 34, "Menu" },
    { SDLK_KP_PLUS,    129, 439, 41, "Play" },
    { SDLK_KP8,         76, 309, 34, "Scroll Back" },
    { SDLK_KP2,        182, 311, 45, "Scroll Fwd" },
    { 0, 0, 0 , 0, "None" }
};
#elif defined (IPOD_1G2G) 
struct button_map bm[] = {
    { SDLK_KP5,       112, 265, 31, "Select" },
    { SDLK_KP8,        74, 224, 28, "Scroll Back" },
    { SDLK_KP2,       146, 228, 28, "Scroll Fwd" },
    /* Dummy button to make crescent shape */
    { SDLK_y,         112, 265, 76, "None" },
    { SDLK_KP8,        74, 224, 28, "Scroll Back" },
    { SDLK_KP2,       146, 228, 28, "Scroll Fwd" },
    { SDLK_KP6,       159, 268, 64, "Right" },
    { SDLK_KP4,        62, 266, 62, "Left" },
    { SDLK_KP_PERIOD, 111, 216, 64, "Menu" },
    { SDLK_KP_PLUS,   111, 326, 55, "Down" },
    { 0, 0, 0 , 0, "None" }
};
#elif defined (IPOD_NANO) 
struct button_map bm[] = {
    { SDLK_KP5,       98, 316, 37, "Select" },
    { SDLK_KP4,       37, 312, 28, "Left" },
    { SDLK_KP6,      160, 313, 25, "Right" },
    { SDLK_KP_PERIOD,102, 256, 23, "Menu" },
    { SDLK_KP_PLUS,   99, 378, 28, "Play" },
    { SDLK_KP8,       58, 272, 24, "Scroll Back" },
    { SDLK_KP2,      141, 274, 22, "Scroll Fwd" },
    { 0, 0, 0 , 0, "None" }
};
#elif defined (IPOD_NANO2G) 
struct button_map bm[] = {
    { SDLK_KP5,       118, 346, 37, "Select" },
    { SDLK_KP4,        51, 345, 28, "Left" },
    { SDLK_KP6,       180, 346, 26, "Right" },
    { SDLK_KP_PERIOD, 114, 286, 23, "Menu" },
    { SDLK_KP_PLUS,   115, 412, 27, "Down" },
    { SDLK_KP8,        67, 303, 28, "Scroll Back" },
    { SDLK_KP2,       163, 303, 27, "Scroll Fwd" },
    { 0, 0, 0 , 0, "None" }
};
#elif defined (COWON_D2)
struct button_map bm[] = {
    { SDLK_DELETE,     51, 14, 17, "Power" },
    { SDLK_h,         138, 14, 16, "Hold" },
    { SDLK_MINUS,     320, 14, 10, "Minus" },
    { SDLK_INSERT,    347, 13, 13, "Menu" },
    { SDLK_KP_PLUS,   374, 14, 12, "Plus" },
    { 0, 0, 0, 0, "None" }
};
#elif defined (IAUDIO_M3)
struct button_map bm[] = {
    { SDLK_KP5,         256,  72, 29, "Play" },
    { SDLK_KP6,         255, 137, 28, "Right" },
    { SDLK_KP4,         257, 201, 26, "Left" },
    { SDLK_KP8,         338,  31, 27, "Up" },
    { SDLK_KP2,         339,  92, 23, "Down" },
    { SDLK_KP_PERIOD,   336,  50, 23, "Mode" },
    { SDLK_KP_DIVIDE,   336, 147, 23, "Rec" },
    { SDLK_h,           336, 212, 30, "Hold" },
    /* remote */
    { SDLK_SPACE,       115, 308, 20, "RC Play" },
    { SDLK_RIGHT,        85, 308, 20, "RC Rew" },
    { SDLK_LEFT,        143, 308, 20, "RC FF" },
    { SDLK_UP,          143, 498, 20, "RC Up" },
    { SDLK_DOWN,         85, 498, 20, "RC Down" },
    { SDLK_INSERT,      212, 308, 30, "RC Mode" },
    { SDLK_F1,          275, 308, 25, "RC Rec" },
    { SDLK_KP_ENTER,    115, 498, 20, "RC Menu" },
    { 0, 0, 0, 0, "None" }
};
#elif defined (IAUDIO_M5)
struct button_map bm[] = {
    { SDLK_KP_ENTER,  333,  41, 17, "Enter" },
    { SDLK_h,         334,  74, 21, "Hold" },
    { SDLK_KP_DIVIDE, 333, 142, 24, "Record" },
    { SDLK_KP_PLUS,   332, 213, 20, "Play" },
    { SDLK_KP5,       250, 291, 19, "Select" },
    { SDLK_KP8,       249, 236, 32, "Up" },
    { SDLK_KP4,       194, 292, 29, "Left" },
    { SDLK_KP6,       297, 290, 27, "Right" },
    { SDLK_KP2,       252, 335, 28, "Down" },
    { 0, 0, 0, 0, "None" }
};
#elif defined (IAUDIO_7)
struct button_map bm[] = {  
    { 0, 0, 0, 0, "None" }
};
#elif defined (IAUDIO_X5)
struct button_map bm[] = {
    { SDLK_KP_ENTER,  275,  38, 17, "Power" },
    { SDLK_h,         274,  83, 16, "Hold" },
    { SDLK_KP_DIVIDE, 276, 128, 22, "Record" },
    { SDLK_KP_PLUS,   274, 186, 22, "Play" },
    { SDLK_KP5,       200, 247, 16, "Select" },
    { SDLK_KP8,       200, 206, 16, "Up" },
    { SDLK_KP4,       163, 248, 19, "Left" },
    { SDLK_KP6,       225, 247, 24, "Right" },
    { SDLK_KP2,       199, 279, 20, "Down" },
    { 0, 0, 0, 0, "None" }
};
#elif defined (ARCHOS_PLAYER)
struct button_map bm[] = {
    { SDLK_KP_PLUS,   79, 252, 23, "On" },
    { SDLK_KP_PERIOD, 81, 310, 20, "Menu" },
    { SDLK_KP8,      154, 237, 28, "Play" },
    { SDLK_KP4,      121, 282, 23, "Left" },
    { SDLK_KP6,      187, 282, 22, "Right" },
    { SDLK_KP2,      157, 312, 20, "Down" },
    { 0, 0, 0, 0, "None" }
};
#elif defined (ARCHOS_RECORDER)
struct button_map bm[] = {
    { SDLK_F1,        94, 205, 22, "F1" },
    { SDLK_F2,       136, 204, 21, "F2" },
    { SDLK_F3,       174, 204, 24, "F3" },
    { SDLK_KP_PLUS,   75, 258, 19, "On" },
    { SDLK_KP_ENTER,  76, 307, 15, "Off" },
    { SDLK_KP5,      151, 290, 20, "Select" },
    { SDLK_KP8,      152, 251, 23, "Up" },
    { SDLK_KP4,      113, 288, 26, "Left" },
    { SDLK_KP6,      189, 291, 23, "Right" },
    { SDLK_KP2,      150, 327, 27, "Down" },
    { 0, 0, 0, 0, "None" }
};
#elif defined (ARCHOS_FMRECORDER) || defined (ARCHOS_RECORDERV2)
struct button_map bm[] = {
    { SDLK_F1,         88, 210, 28, "F1" },
    { SDLK_F2,        144, 212, 28, "F2" },
    { SDLK_F3,        197, 212, 28, "F3" },
    { SDLK_KP5,       144, 287, 21, "Select" },
    { SDLK_KP_PLUS,    86, 320, 13, "Menu" },
    { SDLK_KP_ENTER,  114, 347, 13, "Stop" },
    { SDLK_y,         144, 288, 31, "None" },
    { SDLK_KP8,       144, 259, 25, "Up" },
    { SDLK_KP2,       144, 316, 31, "Down" },
    { SDLK_KP6,       171, 287, 32, "Right" },
    { SDLK_KP4,       117, 287, 31, "Left" },
    { 0, 0, 0, 0, "None" }
};
#elif defined (ARCHOS_ONDIOSP) || defined (ARCHOS_ONDIOFM)
struct button_map bm[] = {
    { SDLK_KP_ENTER,  75, 23, 30, "Enter" },
    { SDLK_KP8,       75, 174, 33, "KP8" },
    { SDLK_KP4,       26, 186, 48, "KP4" },
    { SDLK_KP6,      118, 196, 32, "KP6" },
    { SDLK_KP2,       75, 234, 16, "KP2" },
    { SDLK_KP_PERIOD, 54, 250, 24, "Period" },
    { 0, 0, 0, 0, "None" }
};
#elif defined (IRIVER_H10)
struct button_map bm[] = {
    { SDLK_KP_PLUS,       38,  70, 37, "Power" },
    { SDLK_KP4,          123, 194, 26, "Cancel" },
    { SDLK_KP6,          257, 195, 34, "Select" },
    { SDLK_KP8,          190, 221, 28, "Up" },
    { SDLK_KP2,          192, 320, 27, "Down" },
    { SDLK_KP_DIVIDE,    349,  49, 20, "Rew" },
    { SDLK_KP5,          349,  96, 20, "Play" },
    { SDLK_KP_MULTIPLY,  350, 141, 23, "FF" },
    { 0, 0, 0, 0, "None" }
};
#elif defined (IRIVER_H10_5GB)
struct button_map bm[] = {
    { SDLK_KP_PLUS,      34,  76, 23, "Power" },
    { SDLK_KP4,         106, 222, 28, "Cancel" },
    { SDLK_KP6,         243, 220, 31, "Select" },
    { SDLK_KP8,         176, 254, 34, "Up" },
    { SDLK_KP2,         175, 371, 35, "Down" },
    { SDLK_KP_DIVIDE,   319,  63, 26, "Rew" },
    { SDLK_KP5,         320, 124, 26, "Play" },
    { SDLK_KP_MULTIPLY, 320, 181, 32, "FF" },
    { 0, 0, 0, 0, "None" }
};
#elif defined (IRIVER_H120) || defined (IRIVER_H100)
struct button_map bm[] = {
    { SDLK_KP_DIVIDE,   46, 162, 13, "Record" },
    { SDLK_KP_PLUS,    327,  36, 16, "Play" },
    { SDLK_KP_ENTER,   330,  99, 18, "Stop" },
    { SDLK_KP_PERIOD,  330, 163, 18, "AB" },
    { SDLK_KP5,        186, 227, 27, "5" },
    { SDLK_KP8,        187, 185, 19, "8" },
    { SDLK_KP4,        142, 229, 23, "4" },
    { SDLK_KP6,        231, 229, 22, "6" },
    { SDLK_KP2,        189, 272, 28, "2" },
/* Remote Buttons */
    { SDLK_KP_ENTER,   250, 404, 20, "Stop" },
    { SDLK_SPACE,      285, 439, 29, "Space" },
    { SDLK_h,          336, 291, 24, "Hold" },
    { 0, 0, 0, 0, "None" }
};
#elif defined (IRIVER_H300)
struct button_map bm[] = {
    { SDLK_KP_PLUS,     56, 335, 20, "Play" },
    { SDLK_KP8,        140, 304, 29, "Up" },
    { SDLK_KP_DIVIDE,  233, 331, 23, "Record" },
    { SDLK_KP_ENTER,    54, 381, 24, "Stop" },
    { SDLK_KP4,        100, 353, 17, "Left" },
    { SDLK_KP5,        140, 351, 19, "Navi" },
    { SDLK_KP6,        185, 356, 19, "Right" },
    { SDLK_KP_PERIOD,  230, 380, 20, "AB" },
    { SDLK_KP2,        142, 402, 24, "Down" },
    { SDLK_KP_ENTER,   211, 479, 21, "Stop" },
    { SDLK_KP_PLUS,    248, 513, 29, "Play" },
    { 0, 0, 0, 0, "None" }
};
#elif defined (MROBE_500)
struct button_map bm[] = {
    { SDLK_KP9,     171, 609, 9, "Play" },
    { SDLK_KP4,     158, 623, 9, "Left" },
    { SDLK_KP6,     184, 622, 9, "Right" },
    { SDLK_KP7,     171, 638, 11, "Menu" },
    { 0, 0, 0, 0, "None" }
};
#elif defined (MROBE_100)
struct button_map bm[] = {
    { SDLK_KP7,      80, 233, 30, "Menu" },
    { SDLK_KP8,     138, 250, 19, "Up" },
    { SDLK_KP9,     201, 230, 27, "Play" },
    { SDLK_KP4,      63, 305, 25, "Left" },
    { SDLK_KP5,     125, 309, 28, "Select" },
    { SDLK_KP6,     200, 307, 35, "Right" },
    { SDLK_KP1,      52, 380, 32, "Display" },
    { SDLK_KP2,     125, 363, 30, "Down" },
    { SDLK_KP9,     168, 425, 10, "Play" },
    { SDLK_KP4,     156, 440, 11, "Left" },
    { SDLK_KP6,     180, 440, 13, "Right" },
    { SDLK_KP7,     169, 452, 10, "Menu" },
    { SDLK_KP_MULTIPLY, 222, 15, 16, "Power" },
    { 0, 0, 0, 0, "None" }
};
#elif defined (GIGABEAT_F)
struct button_map bm[] = {
    { SDLK_KP_PLUS,    361, 187, 22, "Power" },
    { SDLK_KP_PERIOD,  361, 270, 17, "Menu" },
    { SDLK_KP9,        365, 345, 26, "Vol Up" },
    { SDLK_KP3,        363, 433, 25, "Vol Down" },
    { SDLK_KP_ENTER,   365, 520, 19, "A" },
    { SDLK_KP8,        167, 458, 35, "Up" },
    { SDLK_KP4,         86, 537, 29, "Left" },
    { SDLK_KP5,        166, 536, 30, "Select" },
    { SDLK_KP6,        248, 536, 30, "Right" },
    { SDLK_KP2,        169, 617, 28, "Down" },
    { 0, 0, 0, 0, "None" }
};
#elif defined (GIGABEAT_S)
struct button_map bm[] = {
    { SDLK_KP_PLUS, 416, 383, 23, "Play" },
    { SDLK_KP7,     135, 442, 46, "Back" },
    { SDLK_KP9,     288, 447, 35, "Menu" },
    { SDLK_KP8,     214, 480, 32, "Up" },
    { SDLK_KP4,     128, 558, 33, "Left" },
    { SDLK_KP5,     214, 556, 34, "Select" },
    { SDLK_KP6,     293, 558, 35, "Right" },
    { SDLK_KP2,     214, 637, 38, "Down" },
    { 0, 0, 0, 0, "None" }
};
#elif defined (SAMSUNG_YH820)
struct button_map bm[] = {
    { SDLK_KP_PLUS, 330,  53, 23, "Record" },
    { SDLK_KP7,     132, 208, 21, "Left" },
    { SDLK_KP5,     182, 210, 18, "Play" },
    { SDLK_KP9,     234, 211, 22, "Right" },
    { SDLK_KP8,     182, 260, 15, "Up" },
    { SDLK_KP4,     122, 277, 29, "Menu" },
    { SDLK_KP6,     238, 276, 25, "Select" },
    { SDLK_KP2,     183, 321, 24, "Down" },
    { 0, 0, 0, 0, "None" }
};
#elif defined (SAMSUNG_YH920) || defined (SAMSUNG_YH925)
struct button_map bm[] = {
    { SDLK_KP9,     370,  32, 15, "FF" },
    { SDLK_KP5,     369,  84, 25, "Play" },
    { SDLK_KP5,     367, 125, 27, "Play" },
    { SDLK_KP3,     369, 188, 17, "Rew" },
    { SDLK_KP_PLUS, 370, 330, 30, "Record" },
    { SDLK_KP4,     146, 252, 32, "Menu" },
    { SDLK_KP8,     204, 226, 27, "Up" },
    { SDLK_KP6,     257, 250, 34, "Select" },
    { SDLK_KP2,     205, 294, 35, "Down" },
    { 0, 0, 0, 0, "None" }
};
#elif defined (ONDA_VX747) || defined (ONDA_VX747P)
struct button_map bm[] = {
    { SDLK_MINUS,   113, 583, 28, "Minus" },
    { SDLK_PLUS,    227, 580, 28, "Plus" },
    { SDLK_RETURN,  171, 583, 34, "Menu" },
    { 0, 0, 0, 0, "None" }
};
#elif defined (PHILIPS_SA9200)
struct button_map bm[] = {
    { SDLK_KP_ENTER,  25, 155, 33, "Power" },
    { SDLK_PAGEUP,   210,  98, 31, "Vol Up" },
    { SDLK_PAGEDOWN, 210, 168, 34, "Vol Down" },
    { SDLK_KP7,       40, 249, 26, "Prev" },
    { SDLK_KP8,      110, 247, 22, "Up" },
    { SDLK_KP9,      183, 249, 31, "Next" },
    { SDLK_KP4,       45, 305, 25, "Left" },
    { SDLK_KP5,      111, 304, 24, "Play" },
    { SDLK_KP6,      183, 304, 21, "Right" },
    { SDLK_KP1,       43, 377, 21, "Menu" },
    { SDLK_KP2,      112, 371, 24, "Down" },
    { 0, 0, 0, 0, "None" }
};
#elif defined (CREATIVE_ZVM) || defined (CREATIVE_ZVM60GB) || \
    defined (CREATIVE_ZV)
struct button_map bm[] = {
    { SDLK_KP7,      52, 414, 35, "Custom" },
    { SDLK_KP8,     185, 406, 55, "Up" },
    { SDLK_KP9,     315, 421, 46, "Play" },
    { SDLK_KP4,     122, 500, 41, "Left" },
    { SDLK_KP6,     247, 493, 49, "Right" },
    { SDLK_KP1,      58, 577, 49, "Back" },
    { SDLK_KP2,     186, 585, 46, "Down" },
    { SDLK_KP3,     311, 569, 47, "Menu" },
    { 0, 0, 0, 0, "None" }
};
#else
struct button_map bm[] = {
    { 0, 0, 0, 0, ""}
};
#endif

int xy2button( int x, int y) {
    int i;
    extern bool debug_buttons;
    
    for ( i = 0; bm[i].button; i++ )
        /* check distance from center of button < radius */
        if ( ( (x-bm[i].x)*(x-bm[i].x) ) + ( ( y-bm[i].y)*(y-bm[i].y) ) < bm[i].radius*bm[i].radius ) {
            if (debug_buttons) 
                printf("Button: %s\n", bm[i].description );
            return bm[i].button;
        }
    return 0;
}
