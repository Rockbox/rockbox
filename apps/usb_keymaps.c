/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Tomer Shalev
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

#ifdef USB_ENABLE_HID
#include "action.h"
#include "lang.h"
#include "usbstack/usb_hid.h"
//#define LOGF_ENABLE
#include "logf.h"

#define MODIFIER(modifier, key) ((key) | (modifier << 8))

#define ALT(key) MODIFIER(HID_KEYBOARD_LEFT_ALT, key)
#define CTRL(key) MODIFIER(HID_KEYBOARD_LEFT_CONTROL, key)
#define SHIFT(key) MODIFIER(HID_KEYBOARD_LEFT_SHIFT, key)

#define NUM_KEY_MAPPINGS (sizeof(hid_key_mappings) / \
        sizeof(hid_key_mappings[0]))

typedef struct
{
    int action;
    int id;
} mapping_t;

typedef struct
{
    int lang_name;
    usage_page_t usage_page;
    mapping_t mapping[];
} hid_key_mapping_t;

static const hid_key_mapping_t hid_key_mapping_multimedia = {
    LANG_MULTIMEDIA_MODE,
    HID_USAGE_PAGE_CONSUMER,
    {
        /* Volume up */
        { ACTION_USB_HID_DEC,    HID_CONSUMER_USAGE_VOLUME_DECREMENT },
        /* Volume down */
        { ACTION_USB_HID_INC,    HID_CONSUMER_USAGE_VOLUME_INCREMENT },
        /* Volume mute */
        { ACTION_USB_HID_SELECT, HID_CONSUMER_USAGE_MUTE },
        /* Playback play / pause */
        { ACTION_USB_HID_START,  HID_CONSUMER_USAGE_PLAY_PAUSE },
        /* Playback stop */
        { ACTION_USB_HID_QUIT,   HID_CONSUMER_USAGE_STOP },
        /* Scan previous track */
        { ACTION_USB_HID_LEFT,   HID_CONSUMER_USAGE_SCAN_PREVIOUS_TRACK },
        /* Scan next track */
        { ACTION_USB_HID_RIGHT,  HID_CONSUMER_USAGE_SCAN_NEXT_TRACK },
        { 0,                     0 },
    }
};

static const hid_key_mapping_t hid_key_mapping_presentation = {
    LANG_PRESENTATION_MODE,
    HID_USAGE_PAGE_KEYBOARD_KEYPAD,
    {
        /* Slideshow start */
        { ACTION_USB_HID_START,       HID_KEYBOARD_F5 },
        /* Slideshow leave */
        { ACTION_USB_HID_QUIT,        HID_KEYBOARD_ESCAPE },
        /* Slide previous */
        { ACTION_USB_HID_LEFT,        HID_KEYBOARD_P },
        /* Slide next */
        { ACTION_USB_HID_RIGHT,       HID_KEYBOARD_N },
        /* Slide first */
        { ACTION_USB_HID_LEFT_LONG,   HID_KEYBOARD_HOME },
        /* Slide last */
        { ACTION_USB_HID_RIGHT_LONG,  HID_KEYBOARD_END },
        /* Screen black */
        { ACTION_USB_HID_MENU,        HID_KEYBOARD_DOT },
        /* Screen white*/
        { ACTION_USB_HID_MENU_LONG,   HID_KEYBOARD_COMMA },
        /* Link previous */
        { ACTION_USB_HID_DEC,         SHIFT(HID_KEYBOARD_TAB) },
        /* Link next */
        { ACTION_USB_HID_INC,         HID_KEYBOARD_TAB },
        /* Mouse click */
        { ACTION_USB_HID_SELECT,      HID_KEYBOARD_RETURN },
        /* Mouse over */
        { ACTION_USB_HID_SELECT_LONG, SHIFT(HID_KEYBOARD_RETURN) },
        { 0,                          0 },
    }
};

static const hid_key_mapping_t hid_key_mapping_browser = {
    LANG_BROWSER_MODE,
    HID_USAGE_PAGE_KEYBOARD_KEYPAD,
    {
        /* Scroll up */
        { ACTION_USB_HID_DEC,         HID_KEYBOARD_UP_ARROW },
        /* Scroll down */
        { ACTION_USB_HID_INC,         HID_KEYBOARD_DOWN_ARROW },
        /* Scroll page up */
        { ACTION_USB_HID_START,       HID_KEYBOARD_PAGE_UP },
        /* Scroll page down */
        { ACTION_USB_HID_MENU,        HID_KEYBOARD_PAGE_DOWN },
        /* Zoom in */
        { ACTION_USB_HID_START_LONG,  CTRL(HID_KEYPAD_PLUS) },
        /* Zoom out */
        { ACTION_USB_HID_MENU_LONG,   CTRL(HID_KEYPAD_MINUS) },
        /* Zoom reset */
        { ACTION_USB_HID_SELECT_LONG, CTRL(HID_KEYPAD_0_AND_INSERT) },
        /* Tab previous */
        { ACTION_USB_HID_LEFT,        CTRL(HID_KEYBOARD_PAGE_UP) },
        /* Tab next */
        { ACTION_USB_HID_RIGHT,       CTRL(HID_KEYBOARD_PAGE_DOWN) },
        /* Tab close */
        { ACTION_USB_HID_QUIT_LONG,   CTRL(HID_KEYBOARD_W) },
        /* History back */
        { ACTION_USB_HID_LEFT_LONG,   ALT(HID_KEYBOARD_LEFT_ARROW) },
        /* History forward */
        { ACTION_USB_HID_RIGHT_LONG,  ALT(HID_KEYBOARD_RIGHT_ARROW) },
        /* View full-screen */
        { ACTION_USB_HID_SELECT,      HID_KEYBOARD_F11 },
        { 0,                          0 },
    }
};

#ifdef HAVE_USB_HID_MOUSE
static const hid_key_mapping_t hid_key_mapping_mouse = {
    LANG_MOUSE_MODE,
    HID_USAGE_PAGE_GENERIC_DESKTOP_CONTROLS,
    {
        /* Cursor move up */
        { ACTION_USB_HID_MOUSE_UP,                HID_MOUSE_UP },
        { ACTION_USB_HID_MOUSE_UP_REP,            HID_MOUSE_UP_REP },
        { ACTION_USB_HID_MOUSE_LDRAG_UP,          HID_MOUSE_LDRAG_UP },
        { ACTION_USB_HID_MOUSE_LDRAG_UP_REP,      HID_MOUSE_LDRAG_UP_REP },
        { ACTION_USB_HID_MOUSE_RDRAG_UP,          HID_MOUSE_RDRAG_UP },
        { ACTION_USB_HID_MOUSE_RDRAG_UP_REP,      HID_MOUSE_RDRAG_UP_REP },
        /* Cursor move down */
        { ACTION_USB_HID_MOUSE_DOWN,              HID_MOUSE_DOWN },
        { ACTION_USB_HID_MOUSE_DOWN_REP,          HID_MOUSE_DOWN_REP },
        { ACTION_USB_HID_MOUSE_LDRAG_DOWN,        HID_MOUSE_LDRAG_DOWN },
        { ACTION_USB_HID_MOUSE_LDRAG_DOWN_REP,    HID_MOUSE_LDRAG_DOWN_REP },
        { ACTION_USB_HID_MOUSE_RDRAG_DOWN,        HID_MOUSE_RDRAG_DOWN },
        { ACTION_USB_HID_MOUSE_RDRAG_DOWN_REP,    HID_MOUSE_RDRAG_DOWN_REP },
        /* Cursor move left */
        { ACTION_USB_HID_MOUSE_LEFT,              HID_MOUSE_LEFT },
        { ACTION_USB_HID_MOUSE_LEFT_REP,          HID_MOUSE_LEFT_REP },
        { ACTION_USB_HID_MOUSE_LDRAG_LEFT,        HID_MOUSE_LDRAG_LEFT },
        { ACTION_USB_HID_MOUSE_LDRAG_LEFT_REP,    HID_MOUSE_LDRAG_LEFT_REP },
        { ACTION_USB_HID_MOUSE_RDRAG_LEFT,        HID_MOUSE_RDRAG_LEFT },
        { ACTION_USB_HID_MOUSE_RDRAG_LEFT_REP,    HID_MOUSE_RDRAG_LEFT_REP },
        /* Cursor move right */
        { ACTION_USB_HID_MOUSE_RIGHT,             HID_MOUSE_RIGHT },
        { ACTION_USB_HID_MOUSE_RIGHT_REP,         HID_MOUSE_RIGHT_REP },
        { ACTION_USB_HID_MOUSE_LDRAG_RIGHT,       HID_MOUSE_LDRAG_RIGHT },
        { ACTION_USB_HID_MOUSE_LDRAG_RIGHT_REP,   HID_MOUSE_LDRAG_RIGHT_REP },
        { ACTION_USB_HID_MOUSE_RDRAG_RIGHT,       HID_MOUSE_RDRAG_RIGHT },
        { ACTION_USB_HID_MOUSE_RDRAG_RIGHT_REP,   HID_MOUSE_RDRAG_RIGHT_REP },
        /* Mouse button left-click */
        { ACTION_USB_HID_MOUSE_BUTTON_LEFT,       HID_MOUSE_BUTTON_LEFT },
        { ACTION_USB_HID_MOUSE_BUTTON_LEFT_REL,   HID_MOUSE_BUTTON_LEFT_REL },
        /* Mouse button right-click */
        { ACTION_USB_HID_MOUSE_BUTTON_RIGHT,      HID_MOUSE_BUTTON_RIGHT },
        { ACTION_USB_HID_MOUSE_BUTTON_RIGHT_REL,  HID_MOUSE_BUTTON_RIGHT_REL },
        /* Mouse wheel scroll up */
        { ACTION_USB_HID_MOUSE_WHEEL_SCROLL_UP,   HID_MOUSE_SCROLL_UP },
        /* Mouse wheel scroll down */
        { ACTION_USB_HID_MOUSE_WHEEL_SCROLL_DOWN, HID_MOUSE_SCROLL_DOWN },
        { 0,                                     0 },
    }
};

#define USB_KEYPAD_MODE_MOUSE 3 /* Value of the mouse setting (hard-coded) */
#endif /* HAVE_USB_HID_MOUSE */

static const hid_key_mapping_t *hid_key_mappings[] =
{
    &hid_key_mapping_multimedia,
    &hid_key_mapping_presentation,
    &hid_key_mapping_browser,
#ifdef HAVE_USB_HID_MOUSE
    &hid_key_mapping_mouse,
#endif
};

extern int usb_keypad_mode;

int get_hid_usb_action(void)
{
    int action, context = CONTEXT_USB_HID;

#ifdef HAVE_USB_HID_MOUSE
    if (usb_keypad_mode == USB_KEYPAD_MODE_MOUSE)
        context = CONTEXT_USB_HID_MOUSE;
#endif

    action = get_action(context, HZ/4);
    /* Skip key mappings in a cyclic way */
    if (action == ACTION_USB_HID_MODE)
    {
        usb_keypad_mode = (usb_keypad_mode + 1) % NUM_KEY_MAPPINGS;
    }
    else if (action == ACTION_USB_HID_MODE_LONG)
    {
        usb_keypad_mode = (usb_keypad_mode - 1) % NUM_KEY_MAPPINGS;
    }
    else if (action > ACTION_USB_HID_FIRST && action < ACTION_USB_HID_LAST)
    {
        const mapping_t *mapping;
        const hid_key_mapping_t *key_mapping =
            hid_key_mappings[usb_keypad_mode];

        for (mapping = key_mapping->mapping; mapping->action; mapping++)
        {
            if (action == mapping->action)
            {
                logf("Action %d", action);
                usb_hid_send(key_mapping->usage_page, mapping->id);
                break;
            }
        }
#ifdef DEBUG
        if (!mapping->action)
            logf("Action %d not found", action);
#endif
    }

    return action;
}

int keypad_mode_name_get(void)
{
    return hid_key_mappings[usb_keypad_mode]->lang_name;
}

#endif
