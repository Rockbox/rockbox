/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by wavey@wavey.org
 * RTC config saving code (C) 2002 by hessu@hes.iki.fi
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include "config.h"
#include "kernel.h"
#include "settings.h"
#include "disk.h"
#include "panic.h"
#include "debug.h"
#include "button.h"
#include "lcd.h"
#include "mpeg.h"
#include "string.h"
#include "ata.h"
#include "power.h"
#include "backlight.h"
#include "powermgmt.h"

struct user_settings global_settings;

/********************************************

Config block as saved on the battery-packed RTC user RAM memory block
of 44 bytes, starting at offset 0x14 of the RTC memory space.

offset  abs
0x00    0x14    "Roc"   header signature: 0x52 0x6f 0x63
0x03    0x17    <version byte: 0x0>
0x04    0x18    <volume byte>
0x05    0x19    <balance byte>
0x06    0x1a    <bass byte>
0x07    0x1b    <treble byte>
0x08    0x1c    <loudness byte>
0x09    0x1d    <bass boost byte>
0x0a    0x1e    <contrast byte>
0x0b    0x1f    <backlight byte>
0x0c    0x20    <poweroff timer byte>
0x0d    0x21    <resume settings byte>
0x0e    0x22    <shuffle mode & directory filter byte>
0x0f    0x23    <scroll speed & WPS display byte>
0x10    0x24    <playlist options byte>
0x11    0x25    <AVC byte>

        <all unused space filled with 0xff>

  the geeky but useless statistics part:
0x24    <total uptime in seconds: 32 bits uint, actually unused for now>

0x2a    <checksum 2 bytes: xor of 0x0-0x29>

Config memory is reset to 0xff and initialized with 'factory defaults' if
a valid header & checksum is not found. Config version number is only
increased when information is _relocated_ or space is _reused_ so that old
versions can read and modify configuration changed by new versions. New
versions should check for the value of '0xff' in each config memory
location used, and reset the setting in question with a factory default if
needed. Memory locations not used by a given version should not be
modified unless the header & checksum test fails.

*************************************/

#include "rtc.h"
static unsigned char rtc_config_block[44];

/*
 * Calculates the checksum for the config block and places it in the given 2-byte buffer
 */

static void calculate_config_checksum(unsigned char *cksum)
{
    unsigned char *p;
    cksum[0] = cksum[1] = 0;
    
    for (p = rtc_config_block; 
         p < rtc_config_block + sizeof(rtc_config_block) - 2;
         p++)
    {
        cksum[0] = cksum[0] ^ *p;
        p++;
        cksum[1] = cksum[1] ^ *p;
    }
}

/*
 * initialize the config block buffer
 */
static void init_config_buffer( void )
{
    DEBUGF( "init_config_buffer()\n" );
    
    /* reset to 0xff - all unused */
    memset(rtc_config_block, 0xff, sizeof(rtc_config_block));
    /* insert header */
    rtc_config_block[0] = 'R';
    rtc_config_block[1] = 'o';
    rtc_config_block[2] = 'c';
    rtc_config_block[3] = 0x0;	/* config block version number */
}

#ifdef HAVE_RTC
/*
 * save the config block buffer on the RTC RAM
 */
static int save_config_buffer( void )
{
    unsigned char addr = 0x14;
    int r;
    unsigned char *p;
    
    DEBUGF( "save_config_buffer()\n" );
    
    /* update the checksum in the end of the block before saving */
    calculate_config_checksum(rtc_config_block + sizeof(rtc_config_block) - 2);
    
    /* FIXME: okay, it _would_ be cleaner and faster to implement rtc_write so
       that it would write a number of bytes at a time since the RTC chip
       supports that, but this will have to do for now 8-) */
    for (p = rtc_config_block; 
         p < rtc_config_block + sizeof(rtc_config_block);
         p++)
    {
        r = rtc_write(addr, *p);
        if (r) {
            DEBUGF( "save_config_buffer: rtc_write failed at addr 0x%02x: %d\n", addr, r );
            return r;
        }
        addr++;
    }
    
    return 0;
}

/*
 * load the config block buffer from the RTC RAM
 */
static int load_config_buffer( void )
{
    unsigned char addr = 0x14;
    unsigned char cksum[2];
    unsigned char *p;
    
    DEBUGF( "load_config_buffer()\n" );
    
    /* FIXME: the same comment applies here as for rtc_write */
    for (p = rtc_config_block; 
         p < rtc_config_block + sizeof(rtc_config_block);
         p++)
    {
        *p = rtc_read(addr);
        addr++;
    }
    
    /* calculate the checksum, check it and the header */
    calculate_config_checksum(cksum);
    
    if (rtc_config_block[0x0] == 'R'
         && rtc_config_block[0x1] == 'o'
         && rtc_config_block[0x2] == 'c'
         && rtc_config_block[0x3] == 0x0
         && cksum[0] == rtc_config_block[0x2a]
         && cksum[1] == rtc_config_block[0x2b]) {
            DEBUGF( "load_config_buffer: header & checksum test ok\n" );
            return 0; /* header and checksum is valid */
        }
    
    /* if checksum is not valid, initialize the config buffer to all-unused */
    DEBUGF( "load_config_buffer: header & checksum test failed\n" );
    init_config_buffer();
    return 1;
}

#else /* HAVE_RTC */
/*
 * save the config block buffer on the 61 Sector
 */
static int save_config_buffer( void )
{
    DEBUGF( "save_config_buffer()\n" );

    /* update the checksum in the end of the block before saving */
    calculate_config_checksum(rtc_config_block + sizeof(rtc_config_block) - 2);
#ifdef SAVE_TO_DISK
    if(battery_level_safe())
        return !ata_write_sectors( 61, 1, rtc_config_block);
    else
        return -1;
#else
    return 0;
#endif
}

/*
 * load the config block buffer from disk
 */
static int load_config_buffer( void )
{
#ifdef SAVE_TO_DISK
    unsigned char cksum[2];
    
    DEBUGF( "load_config_buffer()\n" );
    
    ata_read_sectors( 61, 1,  rtc_config_block);

    /* calculate the checksum, check it and the header */
    calculate_config_checksum(cksum);
    
    if (rtc_config_block[0x0] == 'R'
         && rtc_config_block[0x1] == 'o'
         && rtc_config_block[0x2] == 'c'
         && rtc_config_block[0x3] == 0x0
         && cksum[0] == rtc_config_block[0x2a]
         && cksum[1] == rtc_config_block[0x2b]) {
            DEBUGF( "load_config_buffer: header & checksum test ok\n" );
            return 0; /* header and checksum is valid */
        }
#endif
    /* if checksum is not valid, initialize the config buffer to all-unused */
    DEBUGF( "load_config_buffer: header & checksum test failed\n" );
    init_config_buffer();
    return 1;
}


#endif /* HAVE_RTC */


/*
 * persist all runtime user settings to disk or RTC RAM
 */
int settings_save( void )
{
    DEBUGF( "settings_save()\n" );
    
    /* update the config block buffer with current
       settings and save the block in the RTC */
    rtc_config_block[0x4] = (unsigned char)global_settings.volume;
    rtc_config_block[0x5] = (unsigned char)global_settings.balance;
    rtc_config_block[0x6] = (unsigned char)global_settings.bass;
    rtc_config_block[0x7] = (unsigned char)global_settings.treble;
    rtc_config_block[0x8] = (unsigned char)global_settings.loudness;
    rtc_config_block[0x9] = (unsigned char)global_settings.bass_boost;
    
    rtc_config_block[0xa] = (unsigned char)global_settings.contrast;
    rtc_config_block[0xb] = (unsigned char)global_settings.backlight;
    rtc_config_block[0xc] = (unsigned char)global_settings.poweroff;
    rtc_config_block[0xd] = (unsigned char)global_settings.resume;
    
    rtc_config_block[0xe] = (unsigned char)
        ((global_settings.playlist_shuffle & 1) |
         ((global_settings.mp3filter & 1) << 1) |
         ((global_settings.sort_case & 1) << 2) |
         ((global_settings.discharge & 1) << 3) |
         ((global_settings.statusbar & 1) << 4));

    rtc_config_block[0xf] = (unsigned char)
        ((global_settings.scroll_speed << 3) |
         (global_settings.wps_display & 7));
    
    rtc_config_block[0x11] = (unsigned char)global_settings.avc;
    
    memcpy(&rtc_config_block[0x24], &global_settings.total_uptime, 4);
    
    if(save_config_buffer())
    {
        lcd_clear_display();
#ifdef HAVE_LCD_CHARCELLS
        lcd_puts(0, 0, "Save failed");
        lcd_puts(0, 1, "Batt. low?");
#else
        lcd_puts(4, 2, "Save failed");
        lcd_puts(2, 4, "Is battery low?");
        lcd_update();
#endif
        sleep(HZ*2);
        return -1;
    }
    return 0;
}

/*
 * load settings from disk or RTC RAM
 */
void settings_load(void)
{
    unsigned char c;
    
    DEBUGF( "reload_all_settings()\n" );

    /* populate settings with default values */
    settings_reset();
    
    /* load the buffer from the RTC (resets it to all-unused if the block
       is invalid) and decode the settings which are set in the block */
    if (!load_config_buffer()) {
        if (rtc_config_block[0x4] != 0xFF)
            global_settings.volume = rtc_config_block[0x4];
        if (rtc_config_block[0x5] != 0xFF)
            global_settings.balance = rtc_config_block[0x5];
        if (rtc_config_block[0x6] != 0xFF)
            global_settings.bass = rtc_config_block[0x6];
        if (rtc_config_block[0x7] != 0xFF)
            global_settings.treble = rtc_config_block[0x7];
        if (rtc_config_block[0x8] != 0xFF)
            global_settings.loudness = rtc_config_block[0x8];
        if (rtc_config_block[0x9] != 0xFF)
            global_settings.bass_boost = rtc_config_block[0x9];
    
        if (rtc_config_block[0xa] != 0xFF)
            global_settings.contrast = rtc_config_block[0xa];
        if (rtc_config_block[0xb] != 0xFF)
            global_settings.backlight = rtc_config_block[0xb];
        if (rtc_config_block[0xc] != 0xFF)
            global_settings.poweroff = rtc_config_block[0xc];
        if (rtc_config_block[0xd] != 0xFF)
            global_settings.resume = rtc_config_block[0xd];
        if (rtc_config_block[0xe] != 0xFF) {
            global_settings.playlist_shuffle = rtc_config_block[0xe] & 1;
            global_settings.mp3filter = (rtc_config_block[0xe] >> 1) & 1;
            global_settings.sort_case = (rtc_config_block[0xe] >> 2) & 1;
            global_settings.discharge = (rtc_config_block[0xe] >> 3) & 1;
            global_settings.statusbar = (rtc_config_block[0xe] >> 4) & 1;
        }
        
        c = rtc_config_block[0xf] >> 3;
        if (c != 31)
            global_settings.scroll_speed = c;

        c = rtc_config_block[0xf] & 7;
        if (c != 7)
            global_settings.wps_display = c;
        
        if (rtc_config_block[0x11] != 0xFF)
            global_settings.avc = rtc_config_block[0x11];

        if (rtc_config_block[0x24] != 0xFF)
            memcpy(&global_settings.total_uptime, &rtc_config_block[0x24], 4);
    }
    lcd_scroll_speed(global_settings.scroll_speed);
    backlight_time(global_settings.backlight);
#ifdef HAVE_CHARGE_CTRL
    charge_restart_level = global_settings.discharge ? CHARGE_RESTART_LO : CHARGE_RESTART_HI;
#endif
}

/*
 * reset all settings to their default value 
 */
void settings_reset(void) {
        
    DEBUGF( "settings_reset()\n" );

    global_settings.volume      = mpeg_sound_default(SOUND_VOLUME);
    global_settings.balance     = DEFAULT_BALANCE_SETTING;
    global_settings.bass        = mpeg_sound_default(SOUND_BASS);
    global_settings.treble      = mpeg_sound_default(SOUND_TREBLE);
    global_settings.loudness    = DEFAULT_LOUDNESS_SETTING;
    global_settings.bass_boost  = DEFAULT_BASS_BOOST_SETTING;
    global_settings.avc         = DEFAULT_AVC_SETTING;
    global_settings.contrast    = DEFAULT_CONTRAST_SETTING;
    global_settings.poweroff    = DEFAULT_POWEROFF_SETTING;
    global_settings.backlight   = DEFAULT_BACKLIGHT_SETTING;
    global_settings.wps_display = DEFAULT_WPS_DISPLAY;
    global_settings.mp3filter   = true;
    global_settings.sort_case   = false;
    global_settings.statusbar   = true;
    global_settings.loop_playlist = true;
    global_settings.playlist_shuffle = false;
    global_settings.discharge    = 0;
    global_settings.total_uptime = 0;
    global_settings.scroll_speed = 8;
}


/*
 * dump the list of current settings
 */
void settings_display(void)
{
#ifdef DEBUG
    DEBUGF( "\nsettings_display()\n" );

    DEBUGF( "\nvolume:\t\t%d\nbalance:\t%d\nbass:\t\t%d\ntreble:\t\t%d\nloudness:\t%d\nbass boost:\t%d\n",
            global_settings.volume,
            global_settings.balance,
            global_settings.bass,
            global_settings.treble,
            global_settings.loudness,
            global_settings.bass_boost );

    DEBUGF( "contrast:\t%d\npoweroff:\t%d\nbacklight:\t%d\n",
            global_settings.contrast,
            global_settings.poweroff,
            global_settings.backlight );
#endif
}

void set_bool(char* string, bool* variable )
{
    bool done = false;
    int button;

    lcd_clear_display();
    lcd_puts_scroll(0,0,string);

    while ( !done ) {
        lcd_puts(0, 1, *variable ? "on " : "off");
        lcd_update();

        button = button_get(true);
        switch ( button ) {
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_LEFT:
#else
            case BUTTON_STOP:
            case BUTTON_MENU:
#endif
                done = true;
                break;

            default:
                if(!(button & BUTTON_REL))
                   *variable = !*variable;
                break;
        }
    }
    lcd_stop_scroll();
}

void set_int(char* string, 
             char* unit,
             int* variable,
             void (*function)(int),
             int step,
             int min,
             int max )
{
    bool done = false;

    lcd_clear_display();
    lcd_puts_scroll(0,0,string);

    while (!done) {
        char str[32];
        snprintf(str,sizeof str,"%d %s  ", *variable, unit);
        lcd_puts(0,1,str);
        lcd_update();

        switch( button_get(true) ) {
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_UP:
            case BUTTON_UP | BUTTON_REPEAT:
#else
            case BUTTON_RIGHT:
            case BUTTON_RIGHT | BUTTON_REPEAT:
#endif
                *variable += step;
                if(*variable > max )
                    *variable = max;
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_DOWN:
            case BUTTON_DOWN | BUTTON_REPEAT:
#else
            case BUTTON_LEFT:
            case BUTTON_LEFT | BUTTON_REPEAT:
#endif
                *variable -= step;
                if(*variable < min )
                    *variable = min;
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_LEFT:
#else
            case BUTTON_STOP:
            case BUTTON_MENU:
#endif
                done = true;
                break;
        }
        if ( function )
            function(*variable);
    }
    lcd_stop_scroll();
}

void set_option(char* string, int* variable, char* options[], int numoptions )
{
    bool done = false;

    lcd_clear_display();
    lcd_puts_scroll(0,0,string);

    while ( !done ) {
        lcd_puts(0, 1, options[*variable]);
        lcd_update();

        switch ( button_get(true) ) {
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_UP:
            case BUTTON_UP | BUTTON_REPEAT:
#else
            case BUTTON_RIGHT:
            case BUTTON_RIGHT | BUTTON_REPEAT:
#endif
                if ( *variable < (numoptions-1) )
                    (*variable)++;
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_DOWN:
            case BUTTON_DOWN | BUTTON_REPEAT:
#else
            case BUTTON_LEFT:
            case BUTTON_LEFT | BUTTON_REPEAT:
#endif
                if ( *variable > 0 )
                    (*variable)--;
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_LEFT:
#else
            case BUTTON_STOP:
            case BUTTON_MENU:
#endif
                done = true;
                break;
        }
    }
    lcd_stop_scroll();
}
