/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2003 Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include <stdio.h>
#include <stdbool.h>
#include "lcd.h"
#include "mas.h"
#include "settings.h"
#include "button.h"
#include "fmradio.h"
#include "status.h"
#include "kernel.h"
#include "mpeg.h"

#ifdef HAVE_FMRADIO

#define MAX_FREQ (110000000)
#define MIN_FREQ (80000000)
#define FREQ_STEP 100000

static int curr_freq = 99400000;
static int pll_cnt;

void fm_set_frequency(int freq)
{
    /* We add the standard Intermediate Frequency 10.7MHz before calculating
    ** the divisor
    ** The reference frequency is set to 50kHz, and the VCO output is prescaled
    ** by 2.
    */
    
    pll_cnt = (freq + 10700000) / 50000 / 2;

    /* 0x100000 == FM mode
    ** 0x000002 == Microprocessor controlled Mute
    */
    fmradio_set(1, 0x100002 | pll_cnt << 3);
}

bool radio_screen(void)
{
    char buf[128];
    bool done = false;
    int button;
    int val;
    int freq;
    int i_freq;
    bool lock;
    bool stereo;
    int search_dir = 0;
    
    lcd_clear_display();
    lcd_setmargins(0, 8);
    status_draw(false);
    fmradio_set_status(FMRADIO_PLAYING);
    
    /* Enable the Left and right A/D Converter */
    mas_codec_writereg(0x0, 0xccc7);

    mas_codec_writereg(6, 0x4000);

    fmradio_set(2, 0x108884); /* 50kHz, 7.2MHz crystal */
    fm_set_frequency(curr_freq);
    
    while(!done)
    {
        if(search_dir)
        {
            curr_freq += search_dir * FREQ_STEP;
            if(curr_freq < MIN_FREQ)
                curr_freq = MAX_FREQ;
            if(curr_freq > MAX_FREQ)
                curr_freq = MIN_FREQ;

            /* Tune in and delay */
            fm_set_frequency(curr_freq);
            sleep(10);
            
            /* Start IF measurement */
            fmradio_set(1, 0x100006 | pll_cnt << 3);
            sleep(10);

            /* Now check how close to the IF frequency we are */
            val = fmradio_read(3);
            i_freq = (val & 0x7ffff) / 80;
            lcd_puts(0, 5, "Debug data:");
            snprintf(buf, 128, "IF: %d.%dMHz",
                     i_freq / 100, (i_freq % 100) / 10);
            lcd_puts(0, 6, buf);

            /* Stop searching if the IF frequency is close to 10.7MHz */
            if(i_freq > 1065 && i_freq < 1075)
                search_dir = 0;
        }

        freq = curr_freq / 100000;
        snprintf(buf, 128, "Freq: %d.%dMHz", freq / 10, freq % 10);
        lcd_puts(0, 1, buf);
        
        val = fmradio_read(3);
        stereo = (val & 0x100000)?true:false;
        lock = (val & 0x80000)?true:false;
        snprintf(buf, 128, "Mode: %s", stereo?"Stereo":"Mono");
        lcd_puts(0, 2, buf);
        
        lcd_update();

        if(search_dir)
            button = button_get(false);
        else
            button = button_get_w_tmo(HZ/2);
        switch(button)
        {
            case BUTTON_OFF | BUTTON_REL:
                done = true;
                break;
                
            case BUTTON_LEFT:
                curr_freq -= 100000;
                if(curr_freq < MIN_FREQ)
                    curr_freq = MIN_FREQ;

                fm_set_frequency(curr_freq);
                search_dir = 0;
                break;

            case BUTTON_RIGHT:
                curr_freq += 100000;
                if(curr_freq > MAX_FREQ)
                    curr_freq = MAX_FREQ;
                
                fm_set_frequency(curr_freq);
                search_dir = 0;
                break;

            case BUTTON_LEFT | BUTTON_REPEAT:
                search_dir = -1;
                break;
                
            case BUTTON_RIGHT | BUTTON_REPEAT:
                search_dir = 1;
                break;

            case BUTTON_UP:
            case BUTTON_UP | BUTTON_REPEAT:
                global_settings.volume++;
                if(global_settings.volume > mpeg_sound_max(SOUND_VOLUME))
                    global_settings.volume = mpeg_sound_max(SOUND_VOLUME);
                mpeg_sound_set(SOUND_VOLUME, global_settings.volume);
                status_draw(false);
                settings_save();
                break;

            case BUTTON_DOWN:
            case BUTTON_DOWN | BUTTON_REPEAT:
                global_settings.volume--;
                if(global_settings.volume < mpeg_sound_min(SOUND_VOLUME))
                    global_settings.volume = mpeg_sound_min(SOUND_VOLUME);
                mpeg_sound_set(SOUND_VOLUME, global_settings.volume);
                status_draw(false);
                settings_save();
                break;

            case BUTTON_NONE:
                status_draw(true);
                break;
        }
    }

    fmradio_set_status(0);
    return false;
}
#endif
