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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <windows.h>
#include "uisw32.h"
#include "config.h"
#include "sh7034.h"
#include "button.h"
#include "kernel.h"

#define KEY(k)      (HIBYTE(GetKeyState (k)) & 1)

int last_key ;
static int release_mask;
static int repeat_mask;


void button_init(void) 
{ 
	last_key = 0 ;
}

int button_set_repeat(int newmask)
{
    int oldmask = repeat_mask;
    repeat_mask = newmask;
    return oldmask;
}

int button_set_release(int newmask)
{
    int oldmask = release_mask;
    release_mask = newmask;
    return oldmask;
}

static int real_button_get(void)
{
    int btn = 0;
    Sleep (25);

    if (bActive)
    {
        if (KEY (VK_NUMPAD4) ||
            KEY (VK_LEFT)) // left button
            btn |= BUTTON_LEFT;

        if (KEY (VK_NUMPAD6) ||
            KEY (VK_RIGHT))
            btn |= BUTTON_RIGHT; // right button

        if (KEY (VK_NUMPAD8) ||
            KEY (VK_UP))
            btn |= BUTTON_UP; // up button

        if (KEY (VK_NUMPAD2) ||
            KEY (VK_DOWN))
            btn |= BUTTON_DOWN; // down button

        if (KEY (VK_ADD))
            btn |= BUTTON_ON; // on button
      
#ifdef HAVE_RECORDER_KEYPAD
        if (KEY (VK_RETURN))
            btn |= BUTTON_OFF; // off button
        
        if (KEY (VK_DIVIDE) || KEY(VK_F1))
            btn |= BUTTON_F1; // F1 button
      
        if (KEY (VK_MULTIPLY) || KEY(VK_F2))
            btn |= BUTTON_F2; // F2 button

        if (KEY (VK_SUBTRACT) || KEY(VK_F3))
            btn |= BUTTON_F3; // F3 button
      
        if (KEY (VK_NUMPAD5) ||
            KEY (VK_SPACE))
            btn |= BUTTON_PLAY; // play button
#else
        if (KEY (VK_RETURN))
            btn |= BUTTON_MENU; // menu button
#endif

        if (btn != 0) {
            last_key = 0 ;
        }
    }

    return btn;	
}

int button_get(bool block)
{
    int btn;
    do {

        btn = real_button_get();

        if (btn)
            break;
        
    } while (block);

    return btn;
}

int button_get_w_tmo(int ticks)
{
    int btn;
    do {
        btn = real_button_get();

        if(!btn)
            /* prevent busy-looping */
            sleep(10); /* one tick! */
        else
            return btn;
        
    } while (--ticks);

    return btn;
}
