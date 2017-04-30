/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2015 Lorenzo Miori
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
#include "button.h"
#include "kernel.h"
#include "system.h"
#include "button-target.h"
#include "stdio.h"
#include "unistd.h"

#include <fcntl.h>
#include <signal.h>
#include <linux/input.h>
#include <stropts.h>
#include <poll.h>

#include <pthread.h>

#define KEYBOARD_BUFFER_SIZE    64

int keyboard_dev = 0;
int key = BUTTON_NONE;
struct input_event ev[KEYBOARD_BUFFER_SIZE];

int button_read_device(void)
{
   
    int ret = 0;
    int i = 0;
    
    /* This I/O call is *not* blocking. Refer to button_init(). */
    ret = read(keyboard_dev, ev, sizeof(struct input_event) * KEYBOARD_BUFFER_SIZE);

    if (ret >= (int) sizeof(struct input_event))
    {

        for (i = 0; i < ret / (int)sizeof(struct input_event); i++)
        {
    
            /* key-down event */
            if (ev[i].value == 1 && ev[i].type == 1)
            {
                /* Check for all the keys */
                if (ev[i].code == KEY_F1) {
                    key = BUTTON_USER;
                }
                if (ev[i].code == KEY_ENTER) {
                    key = BUTTON_SELECT;
                }
                if (ev[i].code == KEY_UP) {
                    key = BUTTON_UP;
                }
                if (ev[i].code == KEY_DOWN) {
                    key = BUTTON_DOWN;
                }
                if (ev[i].code == KEY_LEFT) {
                    key = BUTTON_LEFT;
                }
                if (ev[i].code == KEY_RIGHT) {
                    key = BUTTON_RIGHT;
                }
                if (ev[i].code == KEY_F3) {
                    key = BUTTON_MENU;
                }
                if (ev[i].code == KEY_F2) {
                    key = BUTTON_BACK;
                }
                if (ev[i].code == KEY_F4) {
                    key = BUTTON_POWER;
                }
            }

            /* key-up event */
            if (ev[i].value == 0 && ev[i].type == 1)
            {
                key = BUTTON_NONE;
            }

        }
    }
    
    return key;
}

bool headphones_inserted(void)
{
    return 0;
}

void button_init_device(void)
{
    int flags = 0;

    /* open the event device */
    keyboard_dev = open("/dev/input/event1", O_RDONLY);

    /* set the handle to be non-blocking */
    flags = fcntl(keyboard_dev, F_GETFL, 0);
    fcntl(keyboard_dev, F_SETFL, flags | O_NONBLOCK);
}

#ifdef BUTTON_DRIVER_CLOSE
void button_close_device(void)
{

}
#endif /* BUTTON_DRIVER_CLOSE */
