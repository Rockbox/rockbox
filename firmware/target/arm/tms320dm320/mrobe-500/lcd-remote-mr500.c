/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2009 Karl Kurbjun
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
#include "system.h"
#include "file.h"
#include "lcd-remote.h"
#include "adc.h"
#include "scroll_engine.h"
#include "uart-target.h"

static enum remote_control_states
{
    REMOTE_CONTROL_IDLE,
    REMOTE_CONTROL_NOP,
    REMOTE_CONTROL_POWER,
    REMOTE_CONTROL_MASK,
    REMOTE_CONTROL_DRAW1,
    REMOTE_CONTROL_DRAW_PAUSE1,
    REMOTE_CONTROL_DRAW2,
    REMOTE_CONTROL_DRAW_PAUSE2,
    REMOTE_CONTROL_SLEEP
} remote_state_control = REMOTE_CONTROL_NOP;

bool remote_initialized=true;

unsigned char remote_contrast=DEFAULT_REMOTE_CONTRAST_SETTING;
unsigned char remote_power=0x00;

/*** hardware configuration ***/

int lcd_remote_default_contrast(void)
{
    return DEFAULT_REMOTE_CONTRAST_SETTING;
}

void lcd_remote_sleep(void)
{
    remote_state_control=REMOTE_CONTROL_SLEEP;
}

void lcd_remote_powersave(bool on)
{
    if(on)
    {
        remote_power|=0xC0;
        remote_state_control=REMOTE_CONTROL_POWER;
    }
    else
    {
        remote_power&=~(0xC0);
        remote_state_control=REMOTE_CONTROL_POWER;
    }
}

void lcd_remote_set_contrast(int val)
{
    remote_contrast=(char)val;
    remote_state_control=REMOTE_CONTROL_POWER;
}

void lcd_remote_set_invert_display(bool yesno)
{
    (void)yesno;
}

/* turn the display upside down (call lcd_remote_update() afterwards) */
void lcd_remote_set_flip(bool yesno)
{
    (void)yesno;
}

bool remote_detect(void)
{
    return true;
}

void lcd_remote_on(void)
{
    remote_power|=0x80;
    remote_state_control=REMOTE_CONTROL_POWER;
}

void lcd_remote_off(void)
{
    remote_power&=~(0x80);
    remote_state_control=REMOTE_CONTROL_POWER;
}

unsigned char lcd_remote_test[16]=
    {0x80,0xFF,0x80,0x00,0xFF,0x89,0x89,0x00,0xC1,0x89,0x8F,0x80,0xFF,0x80,0,0};

/* Monitor remote hotswap */
static void remote_tick(void)
{
    unsigned char i;
    unsigned char remote_payload[10], remote_payload_size;
    unsigned char remote_check_xor, remote_check_sum;
    
    switch (remote_state_control)
    {
        case REMOTE_CONTROL_IDLE:
            
            remote_payload_size=0;
            remote_state_control=REMOTE_CONTROL_IDLE;
            break;
        case REMOTE_CONTROL_NOP:
            remote_payload[0]=0x11;
            remote_payload[1]=0x30;
            
            remote_payload_size=2;
            remote_state_control=REMOTE_CONTROL_NOP;
            break;
        case REMOTE_CONTROL_POWER:
            remote_payload[0]=0x31;
            remote_payload[1]=remote_power;
            remote_payload[2]=remote_contrast;
            
            remote_payload_size=3;
            remote_state_control=REMOTE_CONTROL_NOP;
            break;
        case REMOTE_CONTROL_MASK:
            remote_payload[0]=0x41;
            remote_payload[1]=0x94;
            
            remote_payload_size=2;
            remote_state_control=REMOTE_CONTROL_NOP;
            break;
        case REMOTE_CONTROL_DRAW1:
            remote_payload[0]=0x51;
            remote_payload[1]=0x80;
            remote_payload[2]=14;
            remote_payload[3]=0;
            remote_payload[4]=0;
            remote_payload[5]=14;
            remote_payload[6]=8;
            
            remote_payload_size=7;
            remote_state_control=REMOTE_CONTROL_DRAW_PAUSE1;
            break;
        case REMOTE_CONTROL_DRAW_PAUSE1:
            remote_payload[0]=0x11;
            remote_payload[1]=0x30;
            
            remote_payload_size=2;
            remote_state_control=REMOTE_CONTROL_DRAW2;
            break;
        case REMOTE_CONTROL_DRAW2:
            remote_payload[0]=0x51;
            remote_payload[1]=0x80;
            remote_payload[2]=14;
            remote_payload[3]=0;
            remote_payload[4]=8;
            remote_payload[5]=14;
            remote_payload[6]=16; 
            
            remote_payload_size=7;
            remote_state_control=REMOTE_CONTROL_DRAW_PAUSE2;
            break;
        case REMOTE_CONTROL_DRAW_PAUSE2:
            remote_payload[0]=0x11;
            remote_payload[1]=0x30;
            
            remote_payload_size=2;
            remote_state_control=REMOTE_CONTROL_NOP;
            break;
        case REMOTE_CONTROL_SLEEP:
            remote_payload[0]=0x71;
            remote_payload[1]=0x30;

            remote_payload_size=2;
            remote_state_control=REMOTE_CONTROL_IDLE;
            break;
        default:
            remote_payload_size=0;
            break;
    }
    
    if(remote_payload_size==0)
    {
        return;
    }
    
    remote_check_xor=remote_payload[0];
    remote_check_sum=remote_payload[0];
    for(i=1; i<remote_payload_size; i++)
    {
        remote_check_xor^=remote_payload[i];
        remote_check_sum+=remote_payload[i];
    }
    
    if(remote_payload[0]==0x51)
    {
        unsigned char offset;
        unsigned char x;
        
        if(remote_payload[4]==8)
        {
            offset=79;
        }
        else
        {
            offset=0;
        }
    
        for (x = 0; x < 14; x++)
        {
            remote_check_xor^=lcd_remote_test[x];
            remote_check_sum+=lcd_remote_test[x];
        }

        uart1_puts(remote_payload, remote_payload_size);
        lcd_remote_test[14]=remote_check_xor;
        lcd_remote_test[15]=remote_check_sum;
        uart1_puts(lcd_remote_test, 16);
    }
    else
    {
        remote_payload[remote_payload_size]=remote_check_xor;
        remote_payload[remote_payload_size+1]=remote_check_sum;
    
        uart1_puts(remote_payload, remote_payload_size+2);
    }
}

void lcd_remote_init_device(void)
{
    lcd_remote_clear_display();
    if (remote_detect())
        lcd_remote_on();

    /* put the remote control in the tick task */
    tick_add_task(remote_tick);
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_remote_update(void)
{
    if(remote_state_control!=REMOTE_CONTROL_DRAW1
        && remote_state_control!=REMOTE_CONTROL_DRAW_PAUSE1
        && remote_state_control!=REMOTE_CONTROL_DRAW2
        && remote_state_control!=REMOTE_CONTROL_DRAW_PAUSE2)
    {
        remote_state_control=REMOTE_CONTROL_DRAW1;
    }
}

/* Update a fraction of the display. */
void lcd_remote_update_rect(int x, int y, int width, int height)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    lcd_remote_update();
}

void _remote_backlight_on(void)
{
    remote_power|=0x40;
    remote_state_control=REMOTE_CONTROL_POWER;
}

void _remote_backlight_off(void)
{
    remote_power&=~(0x40);
    remote_state_control=REMOTE_CONTROL_POWER;
}
