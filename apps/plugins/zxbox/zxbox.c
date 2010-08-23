/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2006 Anton Romanov
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

#include "zxconfig.h"

PLUGIN_HEADER
PLUGIN_IRAM_DECLARE

#include "spkey_p.h"

spkeyboard kb_mkey;
bool exit_requested=false;
bool clear_kbd=0;
extern bool zxbox_menu(void);

/* DUMMIES ... to clean */
unsigned int scrmul=0;
int privatemap;
int use_shm = 0;
int small_screen,pause_on_iconify;
int vga_pause_bg;

#include "keymaps.h"
#include "zxvid_com.h"
#include "spmain.h"
#include "spperif.h"

struct zxbox_settings settings;

/* doesn't fit into .ibss */
unsigned char image_array [ HEIGHT * WIDTH ];

static int previous_state;

#ifdef USE_GREY
GREY_INFO_STRUCT_IRAM
static unsigned char *gbuf;
static size_t         gbuf_size = 0;
#endif

long video_frames IBSS_ATTR = 0 ;
long start_time IBSS_ATTR = 0;

enum plugin_status plugin_start(const void* parameter)
{
    PLUGIN_IRAM_INIT(rb)

#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif
    rb->splash(HZ, "Welcome to ZXBox");


    sp_init();

#ifdef USE_GREY
    /* get the remainder of the plugin buffer */
    gbuf = (unsigned char *) rb->plugin_get_buffer(&gbuf_size);
#ifdef USE_BUFFERED_GREY
    grey_init(gbuf, gbuf_size, GREY_BUFFERED|GREY_ON_COP, LCD_WIDTH,
              LCD_HEIGHT, NULL);
#else
    grey_init(gbuf, gbuf_size, GREY_ON_COP, LCD_WIDTH, LCD_HEIGHT, NULL);
#endif /* USE_BUFFERED_GREY */
    /* switch on greyscale overlay */
    grey_show(true);
#endif /* USE_GREY */


#if defined(HAVE_ADJUSTABLE_CPU_FREQ)
    rb->cpu_boost(true);
#endif

    start_time = *rb->current_tick;

#ifdef RB_PROFILE
   rb->profile_thread();
#endif

    start_spectemu(parameter);

#ifdef RB_PROFILE
   rb->profstop();
#endif

#if defined(HAVE_ADJUSTABLE_CPU_FREQ)
    rb->cpu_boost(false);
#endif

#ifdef USE_GREY
grey_show(false);
grey_release();
#endif 

#if CONFIG_CODEC == SWCODEC && !defined SIMULATOR
    rb->pcm_play_stop();
#endif

return PLUGIN_OK;
}

void init_spect_key(void)
{
  clear_keystates();
  init_basekeys();
}

void spkb_process_events( int evenframe )
{

    if(evenframe){
        int ki;
#if (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) || \
    (CONFIG_KEYPAD == IPOD_1G2G_PAD)
        if (rb->button_hold())
        {
#if defined(HAVE_ADJUSTABLE_CPU_FREQ)
            rb->cpu_boost(false);
#endif
            exit_requested=1;
#ifdef USE_GREY
            grey_show(false);
#endif
            return;
        }
#endif
        int buttons = rb->button_status();
        if ( buttons == previous_state )
            return;
        previous_state = buttons;
#if (CONFIG_KEYPAD != IPOD_4G_PAD) && (CONFIG_KEYPAD != IPOD_3G_PAD) && \
    (CONFIG_KEYPAD != IPOD_1G2G_PAD)
        if (buttons & ZX_MENU)
        {
#if defined(HAVE_ADJUSTABLE_CPU_FREQ)
            rb->cpu_boost(false);
#endif
            exit_requested=1;
#ifdef USE_GREY
            grey_show(false);
#endif
            return;
        }
#endif
        spkb_state_changed = 1;
        if (settings.kempston){
            if ( buttons & ZX_RIGHT ){
                 ki = KS_TO_KEY(SK_KP_Right);
                spkb_kbstate[ki].state = 1;
            }
            else if (buttons & ZX_LEFT){
                 ki = KS_TO_KEY(SK_KP_Left);
                spkb_kbstate[ki].state = 1;

            }
            else{
                 ki = KS_TO_KEY(SK_KP_Right);
                spkb_kbstate[ki].state = 0;
                 ki = KS_TO_KEY(SK_KP_Left);
                spkb_kbstate[ki].state = 0;

            }
            if ( buttons & ZX_UP ){
                 ki = KS_TO_KEY(SK_KP_Up);
                spkb_kbstate[ki].state = 1;
            }
            else if (buttons & ZX_DOWN){
                 ki = KS_TO_KEY(SK_KP_Down);
                spkb_kbstate[ki].state = 1;
            }
            else{
                 ki = KS_TO_KEY(SK_KP_Up);
                spkb_kbstate[ki].state = 0;
                 ki = KS_TO_KEY(SK_KP_Down);
                spkb_kbstate[ki].state = 0;
            }

            if ( buttons & ZX_SELECT ){
                 ki = KS_TO_KEY(SK_KP_Insert);
                spkb_kbstate[ki].state = 1;
            }
            else{
                 ki = KS_TO_KEY(SK_KP_Insert);
                spkb_kbstate[ki].state = 0;
            }

        }
        else {

            if ( buttons & ZX_RIGHT ){
                ki = KS_TO_KEY(intkeys[3]);
                spkb_kbstate[ki].state = 1;
            }
            else{
                ki = KS_TO_KEY(intkeys[3]);
                spkb_kbstate[ki].state = 0;
            }
    
            if ( buttons & ZX_LEFT ){
                ki = KS_TO_KEY(intkeys[2]);
                spkb_kbstate[ki].state = 1;
            }
            else{
                ki = KS_TO_KEY(intkeys[2]);
                spkb_kbstate[ki].state = 0;
            }
    
            if ( buttons & ZX_UP ){
                ki = KS_TO_KEY(intkeys[0]);
                spkb_kbstate[ki].state = 1;
            }
            else{
                ki = KS_TO_KEY(intkeys[0]);
                spkb_kbstate[ki].state = 0;
            }
            
            if ( buttons & ZX_DOWN ){
                ki = KS_TO_KEY(intkeys[1]);
                spkb_kbstate[ki].state = 1;
            }
            else{
                ki = KS_TO_KEY(intkeys[1]);
                spkb_kbstate[ki].state = 0;
            }
            
            if ( buttons & ZX_SELECT ){
                ki = KS_TO_KEY(intkeys[4]);
                spkb_kbstate[ki].state = 1;
            }
            else{
                ki = KS_TO_KEY(intkeys[4]);
                spkb_kbstate[ki].state = 0;
            }
        }
        process_keys();
    }
}

void press_key(int c){
    spkb_state_changed = 1;
    int ki;
    if ( c == 'E' )
        ki = KS_TO_KEY(SK_KP_Enter);
    else if (c == 'S' )
        ki = KS_TO_KEY(SK_KP_Space);
    else
        ki = KS_TO_KEY(c);
    spkb_kbstate[ki].state = 1;
    process_keys();
}

