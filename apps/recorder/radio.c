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
#include "sprintf.h"
#include "lcd.h"
#include "mas.h"
#include "settings.h"
#include "button.h"
#include "fmradio.h"
#include "status.h"
#include "kernel.h"
#include "mpeg.h"
#include "ctype.h"
#include "file.h"
#include "errno.h"
#include "atoi.h"
#include "string.h"
#include "system.h"
#include "radio.h"
#include "menu.h"
#include "misc.h"
#include "keyboard.h"
#include "screens.h"

#ifdef HAVE_FMRADIO

#define MAX_FREQ (110000000)
#define MIN_FREQ (80000000)
#define PLL_FREQ_STEP 10000
#define FREQ_STEP 100000

static int curr_preset = -1;
static int curr_freq = 99400000;
static int pll_cnt;

#define MAX_PRESETS 32
static bool presets_loaded = false;
static struct fmstation presets[MAX_PRESETS];

void fm_load_presets(void);
bool fm_preset_select(void);
bool fm_f3_menu(void);

void fm_set_frequency(int freq)
{
    /* We add the standard Intermediate Frequency 10.7MHz before calculating
    ** the divisor
    ** The reference frequency is set to 50kHz, and the VCO output is prescaled
    ** by 2.
    */
    
    pll_cnt = (freq + 10700000) / (PLL_FREQ_STEP/2) / 2;

    /* 0x100000 == FM mode
    ** 0x000002 == Microprocessor controlled Mute
    */
    fmradio_set(1, 0x100002 | pll_cnt << 3);
}

static int find_preset(int freq)
{
    int i;
    if(presets_loaded)
    {
        for(i = 0;i < MAX_PRESETS;i++)
        {
            if(freq == presets[i].frequency)
                return i;
        }
    }

    return -1;
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

    fm_load_presets();
    
    /* Enable the Left and right A/D Converter */
    mas_codec_writereg(0x0, 0xccc7);

    mas_codec_writereg(6, 0x4000);

    fmradio_set(2, 0x140884); /* 5kHz, 7.2MHz crystal */
    fm_set_frequency(curr_freq);
    curr_preset = find_preset(curr_freq);
    
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
            {
                search_dir = 0;
                curr_preset = find_preset(curr_freq);
            }
        }

        freq = curr_freq / 100000;
        snprintf(buf, 128, "Freq: %d.%dMHz", freq / 10, freq % 10);
        lcd_puts(0, 2, buf);
        
        val = fmradio_read(3);
        stereo = (val & 0x100000)?true:false;
        lock = (val & 0x80000)?true:false;
        snprintf(buf, 128, "Mode: %s", stereo?"Stereo":"Mono");
        lcd_puts(0, 3, buf);

        if(curr_preset >= 0)
        {
            lcd_puts_scroll(0, 1, presets[curr_preset].name);
        }
        
        lcd_update();

        if(search_dir)
            button = button_get(false);
        else
            button = button_get_w_tmo(HZ/2);
        switch(button)
        {
            case BUTTON_OFF:
                done = true;
                break;
                
            case BUTTON_LEFT:
                curr_freq -= FREQ_STEP;
                if(curr_freq < MIN_FREQ)
                    curr_freq = MIN_FREQ;

                fm_set_frequency(curr_freq);
                curr_preset = find_preset(curr_freq);
                search_dir = 0;
                break;

            case BUTTON_RIGHT:
                curr_freq += FREQ_STEP;
                if(curr_freq > MAX_FREQ)
                    curr_freq = MAX_FREQ;
                
                fm_set_frequency(curr_freq);
                curr_preset = find_preset(curr_freq);
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

            case BUTTON_F2:
                fm_preset_select();
                curr_preset = find_preset(curr_freq);
                lcd_clear_display();
                lcd_setmargins(0, 8);
                status_draw(false);
                break;
                
            case BUTTON_F3:
                fm_f3_menu();
                curr_preset = find_preset(curr_freq);
                lcd_clear_display();
                lcd_setmargins(0, 8);
                status_draw(false);
                break;
                
            case BUTTON_NONE:
                status_draw(true);
                break;

            case SYS_USB_CONNECTED:
                usb_screen();
                fmradio_set_status(0);
                return true;
        }
    }

    fmradio_set_status(0);
    return false;
}

static bool parseline(char* line, char** freq, char** name)
{
    char* ptr;

    while ( isspace(*line) )
        line++;

    if ( *line == '#' )
        return false;

    ptr = strchr(line, ':');
    if ( !ptr )
        return false;

    *freq = line;
    *ptr = 0;
    ptr++;
    while (isspace(*ptr))
        ptr++;
    *name = ptr;
    return true;
}

void fm_save_presets(void)
{
    int fd;
    int i;
    
    fd = open("/.rockbox/fm_presets.txt", O_WRONLY);
    if(fd >= 0)
    {
        for(i = 0;i < MAX_PRESETS;i++)
        {
            fprintf(fd, "%d:%s\n", presets[i].frequency, presets[i].name);
        }
        close(fd);
    }
    else
    {
        splash(HZ*2, 0, true, "Preset save failed");
    }
}

void fm_load_presets(void)
{
    int fd;
    int rc;
    char buf[128];
    char *freq;
    char *name;
    int num_presets = 0;
    bool done = false;
    int i;

    if(!presets_loaded)
    {
        memset(presets, 0, sizeof(presets));
    
        fd = open("/.rockbox/fm_presets.txt", O_RDONLY);
        if(fd >= 0)
        {
            i = 0;
            while(!done)
            {
                rc = read_line(fd, buf, 128);
                if(rc > 0)
                {
                    if(parseline(buf, &freq, &name))
                    {
                        presets[i].frequency = atoi(freq);
                        strncpy(presets[i].name, name, 27);
                        presets[i].name[27] = 0;
                        i++;
                        presets_loaded = true;
                        if(num_presets == MAX_PRESETS)
                            done = true;
                    }
                }
                else
                    done = true;
            }
            close(fd);
        }
    }
}

bool fm_preset_select(void)
{
    struct menu_items menu[MAX_PRESETS];
    int m, result;
    int i;
    bool reload_dir = false;
    int num_presets;

    if(presets_loaded)
    {
        num_presets = 0;
        
        for(i = 0;i < MAX_PRESETS;i++)
        {
            if(presets[i].frequency)
            {
                menu[num_presets].desc = presets[i].name;
                /* We use the function pointer entry for the preset
                   entry index */
                menu[num_presets++].function = (void *)i;
            }
        }

        if(num_presets)
        {
            /* DIY menu handling, since we want to exit after selection */
            m = menu_init( menu, num_presets );
            result = menu_show(m);
            menu_exit(m);
            if (result == MENU_SELECTED_EXIT)
                return false;
            else if (result == MENU_ATTACHED_USB)
                reload_dir = true;
            
            if (result >= 0)
            {
                i = (int)menu[result].function;
                curr_freq = presets[i].frequency;
                fm_set_frequency(curr_freq);
            }
        }
        else
        {
            splash(HZ*2, 0, true, "No presets");
        }
    }

    return reload_dir;
}

static bool fm_add_preset(void)
{
    char buf[27];
    int i = find_preset(0);

    if(i >= 0)
    {
        memset(buf, 0, 27);
        
        if (!kbd_input(buf, 27))
        {
            buf[27] = 0;
            strcpy(presets[i].name, buf);
            presets[i].frequency = curr_freq;
            fm_save_presets();
        }
    }
    else
    {
        splash(HZ*2, 0, true, "No free preset entries");
    }
    return true;
}

bool fm_delete_preset(void)
{
    struct menu_items menu[MAX_PRESETS];
    int m, result;
    int i;
    bool reload_dir = false;
    int num_presets;

    if(presets_loaded)
    {
        num_presets = 0;
        
        for(i = 0;i < MAX_PRESETS;i++)
        {
            if(presets[i].frequency)
            {
                menu[num_presets].desc = presets[i].name;
                /* We use the function pointer entry for the preset
                   entry index */
                menu[num_presets++].function = (void *)i;
            }
        }
        
        /* DIY menu handling, since we want to exit after selection */
        m = menu_init( menu, num_presets );
        result = menu_show(m);
        menu_exit(m);
        if (result == MENU_SELECTED_EXIT)
            return false;
        else if (result == MENU_ATTACHED_USB)
            reload_dir = true;
        
        if (result >= 0)
        {
            i = (int)menu[result].function;
            presets[i].frequency = 0;
            fm_save_presets();
        }
    }

    return reload_dir;
}

static struct menu_items f3_items[] = {
    { "Add preset...", fm_add_preset },
    { "Delete preset...", fm_delete_preset }
};
    
bool fm_f3_menu(void)
{
    int m;
    bool result;

    m = menu_init( f3_items, sizeof f3_items / sizeof(struct menu_items) );
    result = menu_run(m);
    menu_exit(m);
    return result;
}

#endif
