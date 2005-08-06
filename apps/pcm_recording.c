/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
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
#include <stdlib.h>

#include "system.h"
#include "lcd.h"
#include "led.h"
#include "audio.h"
#include "button.h"
#include "kernel.h"
#include "settings.h"
#include "lang.h"
#include "font.h"
#include "icons.h"
#include "screens.h"
#include "status.h"
#include "menu.h"
#include "sound_menu.h"
#include "timefuncs.h"
#include "debug.h"
#include "misc.h"
#include "tree.h"
#include "string.h"
#include "dir.h"
#include "errno.h"
#include "atoi.h"
#include "sound.h"
#include "ata.h"
#include "logf.h"
#if defined(HAVE_UDA1380)
#include "uda1380.h"
#elif defined(HAVE_TLV320)
#include "tlv320.h"
#endif
#include "pcm_record.h"

#if defined(HAVE_UDA1380) || defined(HAVE_TLV320)

bool pcm_rec_screen(void)
{
    char buf[80];
    char filename[MAX_PATH];
    char *rec_sources[2] = {"Line-in","Mic"};
    int line=0;
    int play_vol;
    int rec_monitor, rec_gain, rec_vol, rec_source, rec_count, rec_waveform;   
    int rec_time;
    int done, button;
    int w, h;

    lcd_setfont(FONT_SYSFIXED);
    lcd_getstringsize("M", &w, &h);
    lcd_setmargins(global_settings.invert_cursor ? 0 : w, 8);

    play_vol = 120;

    //cpu_boost(true);

#if defined(HAVE_UDA1380)
    uda1380_enable_output(true);
#elif defined(HAVE_TLV320)
    tlv320_enable_output(true);
#endif

#if defined(HAVE_UDA1380)
    uda1380_set_master_vol(play_vol, play_vol);
#elif defined(HAVE_TLV320)
    tlv320_set_headphone_vol(play_vol, play_vol);
#endif

    rec_monitor = 0;     // No record feedback
    rec_source  = 1;     // Mic
    rec_gain    = 0;     // 0-15
    rec_vol     = 0;     // 0-255
    rec_count   = 0;
    rec_waveform = 0;

    pcm_open_recording();
    pcm_set_recording_options(rec_source, rec_waveform);
    pcm_set_recording_gain(rec_gain, rec_vol);

    //rec_create_directory();

    done = 0;
    while(!done)
    {
        line = 0;

        snprintf(buf, sizeof(buf), "PlayVolume: %3d", play_vol);
        lcd_puts(0,line++, buf);    
        snprintf(buf, sizeof(buf), "Gain   : %2d  Volume  : %2d", rec_gain, rec_vol);
        lcd_puts(0,line++, buf);    
        snprintf(buf, sizeof(buf), "Monitor: %2d  Waveform: %2d", rec_monitor, rec_waveform);
        lcd_puts(0,line++, buf);    

        line++;

        rec_time = pcm_recorded_time();

        snprintf(buf, sizeof(buf), "Status: %s", pcm_status() & AUDIO_STATUS_RECORD ? "RUNNING" : "STOPPED");
        lcd_puts(0,line++, buf);
        snprintf(buf, sizeof(buf), "Source: %s", rec_sources[rec_source]);
        lcd_puts(0,line++, buf);
        snprintf(buf, sizeof(buf), "File  : %s", filename);
        lcd_puts(0,line++, buf);
        snprintf(buf, sizeof(buf), "Time  : %02d:%02d.%02d", rec_time/HZ/60, (rec_time/HZ)%60, rec_time%HZ);
        lcd_puts(0,line++, buf);

        line++;

        snprintf(buf, sizeof(buf), "MODE    : Select source");
        lcd_puts(0,line++, buf);
        snprintf(buf, sizeof(buf), "UP/DOWN : Record volume");
        lcd_puts(0,line++, buf);
        snprintf(buf, sizeof(buf), "LFT/RGHT: Record gain");
        lcd_puts(0,line++, buf);
        snprintf(buf, sizeof(buf), "RMT MENU: Toggle monitor");
        lcd_puts(0,line++, buf);
        snprintf(buf, sizeof(buf), "RMT PLAY: Toggle waveform");
        lcd_puts(0,line++, buf);


        lcd_update();


        button = button_get_w_tmo(HZ/8);
        switch (button)
        {
            case BUTTON_OFF:
                done = true;
                break;

            case BUTTON_REC:
                if (pcm_status() & AUDIO_STATUS_RECORD)
                {
                    pcm_stop_recording();

                } else
                {
                                snprintf(filename, MAX_PATH, "/record-%0d.wav", rec_count++);
                                pcm_record(filename);
                            }
                break;

            case BUTTON_ON:
                break;

            case BUTTON_MODE:
                rec_source = 1 - rec_source;
                pcm_set_recording_options(rec_source, rec_waveform);
                break;

            case BUTTON_RIGHT:
            case BUTTON_RIGHT | BUTTON_REPEAT:
                if (rec_gain < 15)
                    rec_gain++;

                pcm_set_recording_gain(rec_gain, rec_vol);
                break;

            case BUTTON_LEFT:
            case BUTTON_LEFT | BUTTON_REPEAT:
                if (rec_gain > 0)
                    rec_gain--;

                pcm_set_recording_gain(rec_gain, rec_vol);
                break;

            case BUTTON_RC_MENU:
                rec_monitor = 1 - rec_monitor;
#if defined(HAVE_UDA1380)
                uda1380_set_monitor(rec_monitor);
#endif
                break;

            case BUTTON_RC_ON:
                rec_waveform = 1 - rec_waveform;
                pcm_set_recording_options(rec_source, rec_waveform);
                break;

            case BUTTON_UP:
            case BUTTON_UP | BUTTON_REPEAT:
                if (rec_vol<255) 
                    rec_vol++;
                pcm_set_recording_gain(rec_gain, rec_vol);
                break;

            case BUTTON_DOWN:
            case BUTTON_DOWN | BUTTON_REPEAT:
                if (rec_vol)
                    rec_vol--;
                pcm_set_recording_gain(rec_gain, rec_vol);
                break;

            case SYS_USB_CONNECTED:
                if (pcm_status() & AUDIO_STATUS_RECORD)
                {
                    // ignore usb while recording
                } else
                {
                    pcm_stop_recording();
#if defined(HAVE_UDA1380)
                    uda1380_enable_output(false);
#elif defined(HAVE_TLV320)
                    tlv320_enable_output(false);
#endif
                    default_event_handler(SYS_USB_CONNECTED);
                    return false;
                }
                break;

        }

    }

    pcm_stop_recording(); 
    pcm_close_recording();

#if defined(HAVE_UDA1380)
    uda1380_enable_output(false);
#elif defined(HAVE_TLV320)
    tlv320_enable_output(false);
#endif

    return true;   
}

#endif

    
