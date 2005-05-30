/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "backlight.h"
#include "button.h"
#include "lcd.h"
#include "lang.h"
#include "icons.h"
#include "font.h"
#include "audio.h"
#include "mp3_playback.h"
#include "usb.h"
#include "settings.h"
#include "status.h"
#include "playlist.h"
#include "sprintf.h"
#include "kernel.h"
#include "power.h"
#include "system.h"
#include "powermgmt.h"
#include "adc.h"
#include "action.h"
#include "talk.h"
#include "misc.h"
#include "id3.h" 
#include "screens.h"
#include "debug.h"
#include "led.h"
#include "sound.h"
#ifdef HAVE_MMC
#include "ata_mmc.h"
#endif

#ifdef HAVE_LCD_BITMAP
#define BMPHEIGHT_usb_logo 32
#define BMPWIDTH_usb_logo 100
static const unsigned char usb_logo[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x40, 0x20, 0x10, 0x08,
    0x04, 0x04, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x81, 0x81, 0x81, 0x81,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0xf1, 0x4f, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0xc0, 
    0x00, 0x00, 0xe0, 0x1c, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x06, 0x81, 0xc0, 0xe0, 0xe0, 0xe0, 0xe0,
    0xc0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x80, 0xc0, 0xe0, 0x70, 0x38, 0x1c, 0x1c,
    0x0c, 0x0e, 0x0e, 0x06, 0x06, 0x06, 0x06, 0x06, 0x0f, 0x1f, 0x1f, 0x1f, 0x1f,
    0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xc0, 0xc0, 0x80, 0x80, 0x00, 0x00,
    0x00, 0x00, 0xe0, 0x1f, 0x00, 0xf8, 0x06, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x82, 0x7e, 0x00, 0xc0, 0x3e, 0x01, 
    0x70, 0x4f, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x80, 0x00, 0x07, 0x0f, 0x1f, 0x1f, 0x1f, 0x1f,
    0x0f, 0x07, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x07, 0x0f,
    0x1f, 0x3f, 0x7b, 0xf3, 0xe3, 0xc3, 0x83, 0x83, 0x83, 0x83, 0xe3, 0xe3, 0xe3,
    0xe3, 0xe3, 0xe3, 0x03, 0x03, 0x03, 0x3f, 0x1f, 0x1f, 0x0f, 0x0f, 0x07, 0x02,
    0xc0, 0x3e, 0x01, 0xe0, 0x9f, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0xf0, 0x0f, 0x80, 0x78, 0x07, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x0c, 0x10, 0x20, 0x40, 0x40, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x81, 0x81, 0x81, 0x81, 0x81, 0x87, 0x87, 0x87,
    0x87, 0x87, 0x87, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xf0,
    0x0f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x07, 0x00, 0x00, 0x00, 0x00, 
};
#endif

void usb_display_info(void)
{
    lcd_clear_display();

#ifdef HAVE_LCD_BITMAP
    lcd_bitmap(usb_logo, 6, 16, BMPWIDTH_usb_logo, BMPHEIGHT_usb_logo, false);
    status_draw(true);
    lcd_update();
#else
    lcd_double_height(false);
    lcd_puts(0, 0, "[USB Mode]");
    status_set_param(false);
    status_set_audio(false);
    status_set_usb(true);
    status_draw(false);
#endif
}

void usb_screen(void)
{
#ifdef USB_NONE
    /* nothing here! */
#else
#ifndef SIMULATOR
    backlight_on();
    usb_acknowledge(SYS_USB_CONNECTED_ACK);
    usb_display_info();
    while(usb_wait_for_disconnect_w_tmo(&button_queue, HZ)) {
        if(usb_inserted()) {

#ifdef HAVE_MMC /* USB-MMC bridge can report activity */

            led(mmc_usb_active(HZ));

#endif

            status_draw(false);
        }
    }
#ifdef HAVE_LCD_CHARCELLS
    status_set_usb(false);
#endif

    backlight_on();
#endif
#endif /* USB_NONE */
}

#ifdef HAVE_MMC
int mmc_remove_request(void)
{
    struct event ev;

    lcd_clear_display();
    splash(1, true, str(LANG_REMOVE_MMC));
    if (global_settings.talk_menu)
        talk_id(LANG_REMOVE_MMC, false);
    
    while (1)
    {
        queue_wait_w_tmo(&button_queue, &ev, HZ/2);
        switch (ev.id)
        {
            case SYS_MMC_EXTRACTED:
                return SYS_MMC_EXTRACTED;
            
            case SYS_USB_DISCONNECTED:
                return SYS_USB_DISCONNECTED;
        }
    }
}
#endif


/* some simulator dummies */
#ifdef SIMULATOR
#define BATTERY_SCALE_FACTOR 7000
unsigned short adc_read(int channel)
{
   (void)channel;
   return 100;
}
#endif

#ifdef HAVE_CHARGING

#ifdef HAVE_LCD_BITMAP
void charging_display_info(bool animate)
{
    unsigned char charging_logo[36];
    const int pox_x = (LCD_WIDTH - sizeof(charging_logo)) / 2;
    const int pox_y = 32;
    static unsigned phase = 3;
    unsigned i;
    char buf[32];
    (void)buf;

#ifdef NEED_ATA_POWER_BATT_MEASURE
    if (ide_powered()) /* FM and V2 can only measure when ATA power is on */
#endif
    {
        int battery_voltage;
        int batt_int, batt_frac;

        battery_voltage = (adc_read(ADC_UNREG_POWER) * BATTERY_SCALE_FACTOR) / 10000;
        batt_int = battery_voltage / 100;
        batt_frac = battery_voltage % 100;

        snprintf(buf, 32, "  Batt: %d.%02dV %d%%  ", batt_int, batt_frac,
                 battery_level());
        lcd_puts(0, 7, buf);
    }

#ifdef HAVE_CHARGE_CTRL

    snprintf(buf, 32, "Charge mode:");
    lcd_puts(0, 2, buf);

    if (charge_state == 1)
        snprintf(buf, 32, str(LANG_BATTERY_CHARGE));
    else if (charge_state == 2)
        snprintf(buf, 32, str(LANG_BATTERY_TOPOFF_CHARGE));
    else if (charge_state == 3)
        snprintf(buf, 32, str(LANG_BATTERY_TRICKLE_CHARGE));
    else
        snprintf(buf, 32, "not charging");

    lcd_puts(0, 3, buf);
    if (!charger_enabled)
        animate = false;
#endif /* HAVE_CHARGE_CTRL */

    
    /* middle part */
    memset(charging_logo+3, 0x00, 32);
    charging_logo[0] = 0x3C;
    charging_logo[1] = 0x24;
    charging_logo[2] = charging_logo[35] = 0xFF;
    
    if (!animate)
    {   /* draw the outline */
        /* middle part */
        lcd_bitmap(charging_logo, pox_x, pox_y + 8, sizeof(charging_logo), 8, true);
        /* upper line */
        charging_logo[0] = charging_logo[1] = 0x00; 
        memset(charging_logo+2, 0x80, 34);
        lcd_bitmap(charging_logo, pox_x, pox_y, sizeof(charging_logo), 8, false);
        /* lower line */
        memset(charging_logo+2, 0x01, 34);
        lcd_bitmap(charging_logo, pox_x, pox_y + 16, sizeof(charging_logo), 8, false);
    }
    else
    {   /* animate the middle part */
        for (i = 3; i<MIN(sizeof(charging_logo)-1, phase); i++)
        {
            if ((i-phase) % 8 == 0)
            {   /* draw a "bubble" here */
                unsigned bitpos;
                bitpos = (phase + i/8) % 15; /* "bounce" effect */
                if (bitpos > 7) 
                    bitpos = 14 - bitpos;
                charging_logo[i] = 0x01 << bitpos;
            }
        }
        lcd_bitmap(charging_logo, pox_x, pox_y + 8, sizeof(charging_logo), 8, true);
        phase++;
    }
    lcd_update();
}
#else /* not HAVE_LCD_BITMAP */

static unsigned char logo_chars[5];
static const unsigned char logo_pattern[] = {
    0x07, 0x04, 0x1c, 0x14, 0x1c, 0x04, 0x07, /* char 1 */
    0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, /* char 2 */
    0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, /* char 3 */
    0x1f, 0x01, 0x01, 0x01, 0x01, 0x01, 0x1f, /* char 4 */
};

static void logo_lock_patterns(bool on)
{
    int i; 

    if (on)
    {
        for (i = 0; i < 4; i++)
            logo_chars[i] = lcd_get_locked_pattern();
        logo_chars[4] = '\0';
    }
    else
    {
        for (i = 0; i < 4; i++)
            lcd_unlock_pattern(logo_chars[i]);
    }
}

void charging_display_info(bool animate)
{
    int battery_voltage;
    unsigned i, ypos;
    static unsigned phase = 3;
    char buf[28];

    battery_voltage = (adc_read(ADC_UNREG_POWER) * BATTERY_SCALE_FACTOR)
                      / 10000;
    snprintf(buf, sizeof(buf), "%s %d.%02dV", logo_chars,
             battery_voltage / 100, battery_voltage % 100);
    lcd_puts(0, 1, buf);
    
    memcpy(buf, logo_pattern, 28); /* copy logo patterns */

    if (!animate) /* build the screen */
    {
        lcd_double_height(false);
        lcd_puts(0, 0, "[Charging]");
    }
    else          /* animate the logo */
    {
        for (i = 3; i < MIN(19, phase); i++)
        {
            if ((i - phase) % 5 == 0)
            {    /* draw a "bubble" here */
                ypos = (phase + i/5) % 9; /* "bounce" effect */
                if (ypos > 4)
                    ypos = 8 - ypos;
                buf[5 - ypos + 7 * (i/5)] |= 0x10 >> (i%5);
            }
        }
        phase++;
    }
    
    for (i = 0; i < 4; i++)
        lcd_define_pattern(logo_chars[i], buf + 7 * i);
}
#endif /* (not) HAVE_LCD_BITMAP */

/* blocks while charging, returns on event:
   1 if charger cable was removed
   2 if Off/Stop key was pressed
   3 if On key was pressed
   4 if USB was connected */
int charging_screen(void)
{
    unsigned int button;
    int rc = 0;
#ifdef BUTTON_OFF
    const unsigned int offbutton = BUTTON_OFF;
#else
    const unsigned int offbutton = BUTTON_STOP;
#endif

    ide_power_enable(false); /* power down the disk, else would be spinning */

    lcd_clear_display();
    backlight_set_timeout(global_settings.backlight_timeout);
#ifdef HAVE_REMOTE_LCD
    remote_backlight_set_timeout(global_settings.remote_backlight_timeout);
#endif
    backlight_set_on_when_charging(global_settings.backlight_on_when_charging);
    status_draw(true);

#ifdef HAVE_LCD_CHARCELLS
    logo_lock_patterns(true);
#endif
    charging_display_info(false);

    do
    {
        status_draw(false);
        charging_display_info(true);
        button = button_get_w_tmo(HZ/3);
#ifdef BUTTON_ON
        if (button == (BUTTON_ON | BUTTON_REL))
#else
        if (button == (BUTTON_RIGHT | BUTTON_REL))
#endif
            rc = 3;
        else if (button == offbutton)
            rc = 2;
        else
        {
            if (usb_detect())
                rc = 4;
            else if (!charger_inserted())
                rc = 1;
        }
    } while (!rc);

#ifdef HAVE_LCD_CHARCELLS
    logo_lock_patterns(false);
#endif
    return rc;
}
#endif /* HAVE_CHARGING */


#if CONFIG_KEYPAD == RECORDER_PAD
/* returns:
   0 if no key was pressed
   1 if a key was pressed (or if ON was held down long enough to repeat)
   2 if USB was connected */
int pitch_screen(void)
{
    int button;
    int pitch = sound_get_pitch();
    bool exit = false;
    bool used = false;

    while (!exit) {

        if ( used ) {
            char* ptr;
            char buf[32];
            int w, h;

            lcd_clear_display();
            lcd_setfont(FONT_SYSFIXED);
    
            ptr = str(LANG_PITCH_UP);
            lcd_getstringsize(ptr,&w,&h);
            lcd_putsxy((LCD_WIDTH-w)/2, 0, ptr);
            lcd_bitmap(bitmap_icons_7x8[Icon_UpArrow],
                       LCD_WIDTH/2 - 3, h*2, 7, 8, true);

            snprintf(buf, sizeof buf, "%d.%d%%", pitch / 10, pitch % 10 );
            lcd_getstringsize(buf,&w,&h);
            lcd_putsxy((LCD_WIDTH-w)/2, h, buf);

            ptr = str(LANG_PITCH_DOWN);
            lcd_getstringsize(ptr,&w,&h);
            lcd_putsxy((LCD_WIDTH-w)/2, LCD_HEIGHT - h, ptr);
            lcd_bitmap(bitmap_icons_7x8[Icon_DownArrow],
                       LCD_WIDTH/2 - 3, LCD_HEIGHT - h*3, 7, 8, true);

            ptr = str(LANG_PAUSE);
            lcd_getstringsize(ptr,&w,&h);
            lcd_putsxy((LCD_WIDTH-(w/2))/2, LCD_HEIGHT/2 - h/2, ptr);
            lcd_bitmap(bitmap_icons_7x8[Icon_Pause],
                       (LCD_WIDTH-(w/2))/2-10, LCD_HEIGHT/2 - h/2, 7, 8, true);

            lcd_update();
        }

        /* use lastbutton, so the main loop can decide whether to
           exit to browser or not */
        button = button_get(true);
        switch (button) {
            case BUTTON_UP:
            case BUTTON_ON | BUTTON_UP:
            case BUTTON_ON | BUTTON_UP | BUTTON_REPEAT:
                used = true;
                if ( pitch < 2000 )
                    pitch++;
                sound_set_pitch(pitch);
                break;

            case BUTTON_DOWN:
            case BUTTON_ON | BUTTON_DOWN:
            case BUTTON_ON | BUTTON_DOWN | BUTTON_REPEAT:
                used = true;
                if ( pitch > 500 )
                    pitch--;
                sound_set_pitch(pitch);
                break;

            case BUTTON_ON | BUTTON_PLAY:
                audio_pause();
                used = true;
                break;

            case BUTTON_PLAY | BUTTON_REL:
                audio_resume();
                used = true;
                break;

            case BUTTON_ON | BUTTON_PLAY | BUTTON_REL:
                audio_resume();
                exit = true;
                break;

            case BUTTON_ON | BUTTON_RIGHT:
            case BUTTON_LEFT | BUTTON_REL:
                if ( pitch < 2000 ) {
                    pitch += 20;
                    sound_set_pitch(pitch);
                }
                break;

            case BUTTON_ON | BUTTON_LEFT:
            case BUTTON_RIGHT | BUTTON_REL:
                if ( pitch > 500 ) {
                    pitch -= 20;
                    sound_set_pitch(pitch);
                }
                break;

#ifdef SIMULATOR
            case BUTTON_ON:
#else
            case BUTTON_ON | BUTTON_REL:
            case BUTTON_ON | BUTTON_UP | BUTTON_REL:
            case BUTTON_ON | BUTTON_DOWN | BUTTON_REL:
#endif
                exit = true;
                break;

            case BUTTON_ON | BUTTON_REPEAT:
                used = true;
                break;

            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED)
                    return 2;
                break;
        }
    }

    lcd_setfont(FONT_UI);

    if ( used )
        return 1;
    else
        return 0;
}

bool quick_screen(int context, int button)
{
    bool exit = false;
    bool used = false;
    int w, h, key;
    char buf[32];
    int oldrepeat = global_settings.repeat_mode;
    
    /* just to stop compiler warning */
    context = context;
    lcd_setfont(FONT_SYSFIXED);

    lcd_getstringsize("A",&w,&h);

    while (!exit) {
        char* ptr=NULL;

        lcd_clear_display();

        switch(button)
        {
            case BUTTON_F2:
                /* Shuffle mode */
                lcd_putsxy(0, LCD_HEIGHT/2 - h*2, str(LANG_SHUFFLE));
                lcd_putsxy(0, LCD_HEIGHT/2 - h, str(LANG_F2_MODE));
                lcd_putsxy(0, LCD_HEIGHT/2, 
                           global_settings.playlist_shuffle ? 
                           str(LANG_ON) : str(LANG_OFF));

                /* Directory Filter */
                switch ( global_settings.dirfilter ) {
                    case SHOW_ALL:
                        ptr = str(LANG_FILTER_ALL);
                        break;
        
                    case SHOW_SUPPORTED:
                        ptr = str(LANG_FILTER_SUPPORTED);
                        break;
        
                    case SHOW_MUSIC:
                        ptr = str(LANG_FILTER_MUSIC);
                        break;
                        
                    case SHOW_PLAYLIST:
                        ptr = str(LANG_FILTER_PLAYLIST);
                        break;

                    case SHOW_ID3DB:
                        ptr = str(LANG_FILTER_ID3DB);
                        break;
                }
        
                snprintf(buf, sizeof buf, "%s:", str(LANG_FILTER));
                lcd_getstringsize(buf,&w,&h);
                lcd_putsxy((LCD_WIDTH-w)/2, LCD_HEIGHT - h*2, buf);
                lcd_getstringsize(ptr,&w,&h);
                lcd_putsxy((LCD_WIDTH-w)/2, LCD_HEIGHT - h, ptr);

                /* Repeat Mode */
                switch ( global_settings.repeat_mode ) {
                    case REPEAT_OFF:
                        ptr = str(LANG_OFF);
                        break;
        
                    case REPEAT_ALL:
                        ptr = str(LANG_REPEAT_ALL);
                        break;
        
                    case REPEAT_ONE:
                        ptr = str(LANG_REPEAT_ONE);
                        break;
                }
        
                lcd_getstringsize(str(LANG_REPEAT),&w,&h);
                lcd_putsxy(LCD_WIDTH - w, LCD_HEIGHT/2 - h*2, str(LANG_REPEAT));
                lcd_putsxy(LCD_WIDTH - w, LCD_HEIGHT/2 - h, str(LANG_F2_MODE));
                lcd_putsxy(LCD_WIDTH - w, LCD_HEIGHT/2, ptr);
                break;
            case BUTTON_F3:
                /* Scrollbar */
                lcd_putsxy(0, LCD_HEIGHT/2 - h*2, str(LANG_F3_SCROLL));
                lcd_putsxy(0, LCD_HEIGHT/2 - h, str(LANG_F3_BAR));
                lcd_putsxy(0, LCD_HEIGHT/2, 
                           global_settings.scrollbar ? str(LANG_ON) : str(LANG_OFF));
    
                /* Status bar */
                ptr = str(LANG_F3_STATUS);
                lcd_getstringsize(ptr,&w,&h);
                lcd_putsxy(LCD_WIDTH - w, LCD_HEIGHT/2 - h*2, ptr);
                lcd_putsxy(LCD_WIDTH - w, LCD_HEIGHT/2 - h, str(LANG_F3_BAR));
                lcd_putsxy(LCD_WIDTH - w, LCD_HEIGHT/2, 
                           global_settings.statusbar ? str(LANG_ON) : str(LANG_OFF));
    
                /* Flip */
                ptr = str(LANG_FLIP_DISPLAY);
                lcd_getstringsize(ptr,&w,&h);
                lcd_putsxy((LCD_WIDTH-w)/2, LCD_HEIGHT - h*2, str(LANG_FLIP_DISPLAY));
                ptr = global_settings.flip_display ?
                           str(LANG_SET_BOOL_YES) : str(LANG_SET_BOOL_NO);
                lcd_getstringsize(ptr,&w,&h);
                lcd_putsxy((LCD_WIDTH-w)/2, LCD_HEIGHT - h, ptr);
                break;
        }

        lcd_bitmap(bitmap_icons_7x8[Icon_FastBackward], 
                   LCD_WIDTH/2 - 16, LCD_HEIGHT/2 - 4, 7, 8, true);
        lcd_bitmap(bitmap_icons_7x8[Icon_DownArrow],
                   LCD_WIDTH/2 - 3, LCD_HEIGHT - h*3, 7, 8, true);
        lcd_bitmap(bitmap_icons_7x8[Icon_FastForward],
                   LCD_WIDTH/2 + 8, LCD_HEIGHT/2 - 4, 7, 8, true);

        lcd_update();
        key = button_get(true);
        
        /*  
         *  This is a temporary kludge so that the F2 & F3 menus operate in exactly
         *  the same manner up until the full F2/F3 configurable menus are complete
         */  
          
        if( key == BUTTON_LEFT || key == BUTTON_RIGHT || key == BUTTON_DOWN || key == ( BUTTON_LEFT | BUTTON_REPEAT ) || key == ( BUTTON_RIGHT  | BUTTON_REPEAT ) || key == ( BUTTON_DOWN  | BUTTON_REPEAT ) )
            key = button | key;
            
        switch (key) {
            case BUTTON_F2 | BUTTON_LEFT:
            case BUTTON_F2 | BUTTON_LEFT | BUTTON_REPEAT:
                global_settings.playlist_shuffle =
                    !global_settings.playlist_shuffle;

                if(audio_status() & AUDIO_STATUS_PLAY)
                {
                    if (global_settings.playlist_shuffle)
                        playlist_randomise(NULL, current_tick, true);
                    else
                        playlist_sort(NULL, true);
                }
                used = true;
                break;

            case BUTTON_F2 | BUTTON_DOWN:
            case BUTTON_F2 | BUTTON_DOWN | BUTTON_REPEAT:
                global_settings.dirfilter++;
                if ( global_settings.dirfilter >= NUM_FILTER_MODES )
                    global_settings.dirfilter = 0;
                used = true;
                break;

            case BUTTON_F2 | BUTTON_RIGHT:
            case BUTTON_F2 | BUTTON_RIGHT | BUTTON_REPEAT:
                global_settings.repeat_mode++;
                if ( global_settings.repeat_mode >= NUM_REPEAT_MODES )
                    global_settings.repeat_mode = 0;
                used = true;
                break;

            case BUTTON_F3 | BUTTON_LEFT:
            case BUTTON_F3 | BUTTON_LEFT | BUTTON_REPEAT:
                global_settings.scrollbar = !global_settings.scrollbar;
                used = true;
                break;

            case BUTTON_F3 | BUTTON_RIGHT:
            case BUTTON_F3 | BUTTON_RIGHT | BUTTON_REPEAT:
                global_settings.statusbar = !global_settings.statusbar;
                used = true;
                break;

            case BUTTON_F3 | BUTTON_DOWN:
            case BUTTON_F3 | BUTTON_DOWN | BUTTON_REPEAT:
            case BUTTON_F3 | BUTTON_UP:
            case BUTTON_F3 | BUTTON_UP | BUTTON_REPEAT:
                global_settings.flip_display = !global_settings.flip_display;
                button_set_flip(global_settings.flip_display);
                lcd_set_flip(global_settings.flip_display);
                used = true;
                break;

            case BUTTON_F3 | BUTTON_REL:
            case BUTTON_F2 | BUTTON_REL:
            
                if( used )
                    exit = true;

                used = true;
                    
                break;

            case BUTTON_OFF | BUTTON_REL:
                lcd_setfont(FONT_UI);
                return false;

            default:
                if(default_event_handler(key) == SYS_USB_CONNECTED)
                    return true;
                break;
        }
    }

    settings_save();
    
    switch( button )
    {
        case BUTTON_F2:

            if ( oldrepeat != global_settings.repeat_mode )
                audio_flush_and_reload_tracks();

            break;
        case BUTTON_F3:

            if (global_settings.statusbar)
                lcd_setmargins(0, STATUSBAR_HEIGHT);
            else
                lcd_setmargins(0, 0);
                
            break;
    }
    
    lcd_setfont(FONT_UI);

    return false;
}
#endif

#ifdef HAVE_LCD_BITMAP
#define SPACE 3 /* pixels between words */
#define MAXLETTERS 128 /* 16*8 */
#define MAXLINES 10
#else
#define SPACE 1 /* one letter space */
#undef LCD_WIDTH
#define LCD_WIDTH 11
#undef LCD_HEIGHT
#define LCD_HEIGHT 2
#define MAXLETTERS 22 /* 11 * 2 */
#define MAXLINES 2
#endif

void splash(int ticks,       /* how long the splash is displayed */
            bool center,     /* FALSE means left-justified, TRUE means
                                horizontal and vertical center */
            const char *fmt, /* what to say *printf style */
            ...)
{
    char *next;
    char *store=NULL;
    int x=0;
    int y=0;
    int w, h;
    unsigned char splash_buf[MAXLETTERS];
    va_list ap;
    unsigned char widths[MAXLINES];
    int line=0;
    bool first=true;
#ifdef HAVE_LCD_BITMAP
    int maxw=0;
#endif

#ifdef HAVE_LCD_CHARCELLS
    lcd_double_height (false);    
#endif
    va_start( ap, fmt );
    vsnprintf( splash_buf, sizeof(splash_buf), fmt, ap );

    if(center) {
        
        /* first a pass to measure sizes */
        next = strtok_r(splash_buf, " ", &store);
        while (next) {
#ifdef HAVE_LCD_BITMAP
            lcd_getstringsize(next, &w, &h);
#else
            w = strlen(next);
            h = 1; /* store height in characters */
#endif
            if(!first) {
                if(x+w> LCD_WIDTH) {
                    /* Too wide, wrap */
                    y+=h;
                    line++;
                    if((y > (LCD_HEIGHT-h)) || (line > MAXLINES))
                        /* STOP */
                        break;
                    x=0;
                    first=true;
                }
            }
            else
                first = false;

            /* think of it as if the text was written here at position x,y
               being w pixels/chars wide and h high */

            x += w+SPACE;
            widths[line]=x-SPACE; /* don't count the trailing space */
#ifdef HAVE_LCD_BITMAP
            /* store the widest line */
            if(widths[line]>maxw)
                maxw = widths[line];
#endif
            next = strtok_r(NULL, " ", &store);
        }
#ifdef HAVE_LCD_BITMAP
        /* Start displaying the message at position y. The reason for the
           added h here is that it isn't added until the end of lines in the
           loop above and we always break the loop in the middle of a line. */
        y = (LCD_HEIGHT - (y+h) )/2;
#else
        y = 0; /* vertical center on 2 lines would be silly */
#endif
        first=true;

        /* Now recreate the string again since the strtok_r() above has ruined
           the one we already have! Here's room for improvements! */
        vsnprintf( splash_buf, sizeof(splash_buf), fmt, ap );
    }
    va_end( ap );

    if(center)
    {
        x = (LCD_WIDTH-widths[0])/2;
        if(x < 0)
            x = 0;
    }

#ifdef HAVE_LCD_BITMAP
    /* If we center the display and it wouldn't cover the full screen,
       then just clear the box we need and put a nice little frame and
       put the text in there! */
    if(center && (y > 2)) {
        if(maxw < (LCD_WIDTH -4)) {
            int xx = (LCD_WIDTH-maxw)/2 - 2;
            lcd_clearrect(xx, y-2, maxw+4, LCD_HEIGHT-y*2+4);
            lcd_drawrect(xx, y-2, maxw+4, LCD_HEIGHT-y*2+4);
        }
        else {
            lcd_clearrect(0, y-2, LCD_WIDTH, LCD_HEIGHT-y*2+4);
            lcd_drawline(0, y-2, LCD_WIDTH-1, y-2);
            lcd_drawline(0, LCD_HEIGHT-y+2, LCD_WIDTH-1, LCD_HEIGHT-y+2);
        }
    }
    else
#endif
        lcd_clear_display();
    line=0;
    next = strtok_r(splash_buf, " ", &store);
    while (next) {
#ifdef HAVE_LCD_BITMAP
        lcd_getstringsize(next, &w, &h);
#else
        w = strlen(next);
        h = 1;
#endif
        if(!first) {
            if(x+w> LCD_WIDTH) {
                /* too wide */
                y+=h;
                line++; /* goto next line */
                first=true;
                if(y > (LCD_HEIGHT-h))
                    /* STOP */
                    break;
                if(center) {
                    x = (LCD_WIDTH-widths[line])/2;
                    if(x < 0)
                       x = 0;
                }
                else
                    x=0;
            }
        }
        else
            first=false;
#ifdef HAVE_LCD_BITMAP
        lcd_putsxy(x, y, next);
#else
        lcd_puts(x, y, next);
#endif
        x += w+SPACE; /*  pixels space! */
        next = strtok_r(NULL, " ", &store);
    }
    lcd_update();

    if(ticks)
        /* unbreakable! */
        sleep(ticks);
}

#if defined(HAVE_CHARGING) || defined(SIMULATOR)
void charging_splash(void)
{
    splash(2*HZ, true, str(LANG_BATTERY_CHARGE));
    button_clear_queue();
}
#endif


#if defined(HAVE_LCD_BITMAP) && defined (HAVE_RTC)

/* little helper function for voice output */
static void say_time(int cursorpos, const struct tm *tm)
{
    static const int unit[] = { UNIT_HOUR, UNIT_MIN, UNIT_SEC, 0, 0, 0 };
    int value = 0;

    if (!global_settings.talk_menu)
        return;

    switch(cursorpos)
    {
    case 0:
        value = tm->tm_hour;
        break;
    case 1:
        value = tm->tm_min;
        break;
    case 2:
        value = tm->tm_sec;
        break;
    case 3:
        value = tm->tm_year + 1900;
        break;
    case 5:
        value = tm->tm_mday;
        break;
    }
    
    if (cursorpos == 4) /* month */
        talk_id(LANG_MONTH_JANUARY + tm->tm_mon, false);
    else
        talk_value(value, unit[cursorpos], false);
}


#define INDEX_X 0
#define INDEX_Y 1
#define INDEX_WIDTH 2
bool set_time_screen(const char* string, struct tm *tm)
{
    bool done = false;
    int button;
    int min = 0, steps = 0;
    int cursorpos = 0;
    int lastcursorpos = !cursorpos;
    unsigned char buffer[19];
    int realyear;
    int julianday;
    int i;
    unsigned char reffub[5];
    unsigned int width, height;
    unsigned int separator_width, weekday_width;
    unsigned int line_height, prev_line_height;

    static const int dayname[] = {
        LANG_WEEKDAY_SUNDAY,
        LANG_WEEKDAY_MONDAY,
        LANG_WEEKDAY_TUESDAY,
        LANG_WEEKDAY_WEDNESDAY,
        LANG_WEEKDAY_THURSDAY,
        LANG_WEEKDAY_FRIDAY,
        LANG_WEEKDAY_SATURDAY
    };
    static const int monthname[] = {
        LANG_MONTH_JANUARY,
        LANG_MONTH_FEBRUARY,
        LANG_MONTH_MARCH,
        LANG_MONTH_APRIL,
        LANG_MONTH_MAY,
        LANG_MONTH_JUNE,
        LANG_MONTH_JULY,
        LANG_MONTH_AUGUST,
        LANG_MONTH_SEPTEMBER,
        LANG_MONTH_OCTOBER,
        LANG_MONTH_NOVEMBER,
        LANG_MONTH_DECEMBER
    };
    char cursor[][3] = {{ 0,  8, 12}, {18,  8, 12}, {36,  8, 12},
                        {24, 16, 24}, {54, 16, 18}, {78, 16, 12}};
    char daysinmonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    int monthname_len = 0, dayname_len = 0;

    int *valptr = NULL;

#ifdef HAVE_LCD_BITMAP
    if(global_settings.statusbar)
        lcd_setmargins(0, STATUSBAR_HEIGHT);
    else
        lcd_setmargins(0, 0);
#endif
    lcd_clear_display();
    lcd_puts_scroll(0, 0, string);

    while ( !done ) {
        /* calculate the number of days in febuary */
        realyear = tm->tm_year + 1900;
        if((realyear % 4 == 0 && !(realyear % 100 == 0)) || realyear % 400 == 0)
            daysinmonth[1] = 29;
        else
            daysinmonth[1] = 28;

        /* fix day if month or year changed */
        if (tm->tm_mday > daysinmonth[tm->tm_mon])
            tm->tm_mday = daysinmonth[tm->tm_mon];

        /* calculate day of week */
        julianday = 0;
        for(i = 0; i < tm->tm_mon; i++) {
           julianday += daysinmonth[i];
        }
        julianday += tm->tm_mday;
        tm->tm_wday = (realyear + julianday + (realyear - 1) / 4 -
                       (realyear - 1) / 100 + (realyear - 1) / 400 + 7 - 1) % 7;

        snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d ",
                 tm->tm_hour, tm->tm_min, tm->tm_sec);
        lcd_puts(0, 1, buffer);

        /* recalculate the positions and offsets */
        lcd_getstringsize(string, &width, &prev_line_height);
        lcd_getstringsize(buffer, &width, &line_height);
        lcd_getstringsize(":", &separator_width, &height);

        /* hour */
        strncpy(reffub, buffer, 2);
        reffub[2] = '\0';
        lcd_getstringsize(reffub, &width, &height);
        cursor[0][INDEX_X] = 0;
        cursor[0][INDEX_Y] = prev_line_height;
        cursor[0][INDEX_WIDTH] = width;

        /* minute */
        strncpy(reffub, buffer + 3, 2);
        reffub[2] = '\0';
        lcd_getstringsize(reffub, &width, &height);
        cursor[1][INDEX_X] = cursor[0][INDEX_WIDTH] + separator_width;
        cursor[1][INDEX_Y] = prev_line_height;
        cursor[1][INDEX_WIDTH] = width;

        /* second */
        strncpy(reffub, buffer + 6, 2);
        reffub[2] = '\0';
        lcd_getstringsize(reffub, &width, &height);
        cursor[2][INDEX_X] = cursor[0][INDEX_WIDTH] + separator_width +
                             cursor[1][INDEX_WIDTH] + separator_width;
        cursor[2][INDEX_Y] = prev_line_height;
        cursor[2][INDEX_WIDTH] = width;

        lcd_getstringsize(buffer, &width, &prev_line_height);

        snprintf(buffer, sizeof(buffer), "%s %04d %s %02d ",
                 str(dayname[tm->tm_wday]), tm->tm_year+1900,
                 str(monthname[tm->tm_mon]), tm->tm_mday);
        lcd_puts(0, 2, buffer);

        /* recalculate the positions and offsets */
        lcd_getstringsize(buffer, &width, &line_height);

        /* store these 2 to prevent _repeated_ strlen calls */
        monthname_len = strlen(str(monthname[tm->tm_mon]));
        dayname_len = strlen(str(dayname[tm->tm_wday]));

        /* weekday */
        strncpy(reffub, buffer, dayname_len);
        reffub[dayname_len] = '\0';
        lcd_getstringsize(reffub, &weekday_width, &height);
        lcd_getstringsize(" ", &separator_width, &height);

        /* year */
        strncpy(reffub, buffer + dayname_len + 1, 4);
        reffub[4] = '\0';
        lcd_getstringsize(reffub, &width, &height);
        cursor[3][INDEX_X] = weekday_width + separator_width;
        cursor[3][INDEX_Y] = cursor[0][INDEX_Y] + prev_line_height;
        cursor[3][INDEX_WIDTH] = width;

        /* month */
        strncpy(reffub, buffer + dayname_len + 6, monthname_len);
        reffub[monthname_len] = '\0';
        lcd_getstringsize(reffub, &width, &height);
        cursor[4][INDEX_X] = weekday_width + separator_width +
                             cursor[3][INDEX_WIDTH] + separator_width;
        cursor[4][INDEX_Y] = cursor[0][INDEX_Y] + prev_line_height;
        cursor[4][INDEX_WIDTH] = width;

        /* day */
        strncpy(reffub, buffer + dayname_len + monthname_len + 7, 2);
        reffub[2] = '\0';
        lcd_getstringsize(reffub, &width, &height);
        cursor[5][INDEX_X] = weekday_width + separator_width +
                             cursor[3][INDEX_WIDTH] + separator_width +
                             cursor[4][INDEX_WIDTH] + separator_width;
        cursor[5][INDEX_Y] = cursor[0][INDEX_Y] + prev_line_height;
        cursor[5][INDEX_WIDTH] = width;

        lcd_invertrect(cursor[cursorpos][INDEX_X],
                       cursor[cursorpos][INDEX_Y] + lcd_getymargin(),
                       cursor[cursorpos][INDEX_WIDTH],
                       line_height);

        lcd_puts(0, 4, str(LANG_TIME_SET));
        lcd_puts(0, 5, str(LANG_TIME_REVERT));
#ifdef HAVE_LCD_BITMAP
        status_draw(true);
#endif
        lcd_update();

        /* calculate the minimum and maximum for the number under cursor */
        if(cursorpos!=lastcursorpos) {
            lastcursorpos=cursorpos;
            switch(cursorpos) {
                case 0: /* hour */
                    min = 0;
                    steps = 24;
                    valptr = &tm->tm_hour;
                    break;
                case 1: /* minute */
                    min = 0;
                    steps = 60;
                    valptr = &tm->tm_min;
                    break;
                case 2: /* second */
                    min = 0;
                    steps = 60;
                    valptr = &tm->tm_sec;
                    break;
                case 3: /* year */
                    min = 1;
                    steps = 200;
                    valptr = &tm->tm_year;
                    break;
                case 4: /* month */
                    min = 0;
                    steps = 12;
                    valptr = &tm->tm_mon;
                    break;
                case 5: /* day */
                    min = 1;
                    steps = daysinmonth[tm->tm_mon];
                    valptr = &tm->tm_mday;
                    break;
            }
            say_time(cursorpos, tm);
        }

        button = button_get_w_tmo(HZ/2);
        switch ( button ) {
            case BUTTON_LEFT:
                cursorpos = (cursorpos + 6 - 1) % 6;
                break;
            case BUTTON_RIGHT:
                cursorpos = (cursorpos + 6 + 1) % 6;
                break;
            case BUTTON_UP:
            case BUTTON_UP | BUTTON_REPEAT:
                *valptr = (*valptr + steps - min + 1) %
                    steps + min;
                if(*valptr == 0)
                    *valptr = min;
                say_time(cursorpos, tm);
                break;
            case BUTTON_DOWN:
            case BUTTON_DOWN | BUTTON_REPEAT:
                *valptr = (*valptr + steps - min - 1) % 
                    steps + min;
                if(*valptr == 0)
                    *valptr = min;
                say_time(cursorpos, tm);
                break;

#ifdef BUTTON_ON
            case BUTTON_ON:
#elif defined BUTTON_MENU
            case BUTTON_MENU:
#endif
                done = true;
                break;

            case BUTTON_OFF:
                done = true;
                tm->tm_year = -1;
                break;

            default:
                if (default_event_handler(button) == SYS_USB_CONNECTED)
                    return true;
                break;
        }
    }

    return false;
}
#endif /* defined(HAVE_LCD_BITMAP) && defined (HAVE_RTC) */

#if CONFIG_KEYPAD == RECORDER_PAD || CONFIG_KEYPAD == IRIVER_H100_PAD
bool shutdown_screen(void)
{
    int button;
    bool done = false;

    lcd_stop_scroll();

    splash(0, true, str(LANG_CONFIRM_SHUTDOWN));

    while(!done)
    {
        button = button_get_w_tmo(HZ*2);
        switch(button)
        {
            case BUTTON_OFF:
                clean_shutdown();
                break;

            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED)
                    return true;
                
                /* Return if any other button was pushed, or if there
                   was a timeout. We ignore RELEASE events, since we may
                   have been called by a button down event, and the user might
                   not have released the button yet.
                   We also ignore REPEAT events, since we don't want to
                   remove the splash when the user holds OFF to shut down. */
                if(!(button & (BUTTON_REL | BUTTON_REPEAT)))
                   done = true;
                break;
        }
    }
    return false;
}
#endif

bool browse_id3(void)
{
    struct mp3entry* id3 = audio_current_track();
    int button;
    int menu_pos = 0;
    int menu_max = 8;
    bool exit = false;
    char scroll_text[MAX_PATH];

    if (!(audio_status() & AUDIO_STATUS_PLAY))
        return false;

    while (!exit)
    {
        lcd_clear_display();

        switch (menu_pos)
        {
            case 0:
                lcd_puts(0, 0, str(LANG_ID3_TITLE));
                lcd_puts_scroll(0, 1, id3->title ? id3->title : 
                                (char*)str(LANG_ID3_NO_TITLE));
                break;

            case 1:
                lcd_puts(0, 0, str(LANG_ID3_ARTIST));
                lcd_puts_scroll(0, 1, 
                                id3->artist ? id3->artist : 
                                (char*)str(LANG_ID3_NO_ARTIST));
                break;

            case 2:
                lcd_puts(0, 0, str(LANG_ID3_ALBUM));
                lcd_puts_scroll(0, 1, id3->album ? id3->album : 
                                (char*)str(LANG_ID3_NO_ALBUM));
                break;

            case 3:
                lcd_puts(0, 0, str(LANG_ID3_TRACKNUM));
                
                if (id3->tracknum) {
                    snprintf(scroll_text,sizeof(scroll_text), "%d",
                             id3->tracknum);
                    lcd_puts_scroll(0, 1, scroll_text);
                }
                else
                    lcd_puts_scroll(0, 1, str(LANG_ID3_NO_TRACKNUM));
                break;

            case 4:
                lcd_puts(0, 0, str(LANG_ID3_GENRE));
                lcd_puts_scroll(0, 1,
                                id3_get_genre(id3) ?
                                id3_get_genre(id3) :
                                (char*)str(LANG_ID3_NO_INFO));
                break;

            case 5:
                lcd_puts(0, 0, str(LANG_ID3_YEAR));
                if (id3->year) {
                    snprintf(scroll_text,sizeof(scroll_text), "%d",
                             id3->year);
                    lcd_puts_scroll(0, 1, scroll_text);
                }
                else
                    lcd_puts_scroll(0, 1, str(LANG_ID3_NO_INFO));
                break;

            case 6:
                lcd_puts(0, 0, str(LANG_ID3_LENGHT));
                snprintf(scroll_text,sizeof(scroll_text), "%d:%02d",
                         (int) (id3->length / 60000),
                         (int) (id3->length % 60000 / 1000) );
                lcd_puts(0, 1, scroll_text);
                break;

            case 7:
                lcd_puts(0, 0, str(LANG_ID3_PLAYLIST));
                snprintf(scroll_text,sizeof(scroll_text), "%d/%d",
                         playlist_get_display_index(), playlist_amount());
                lcd_puts_scroll(0, 1, scroll_text);
                break;


            case 8:
                lcd_puts(0, 0, str(LANG_ID3_BITRATE));
                snprintf(scroll_text,sizeof(scroll_text), "%d kbps", 
                         id3->bitrate);
                lcd_puts(0, 1, scroll_text);
                break;

            case 9:
                lcd_puts(0, 0, str(LANG_ID3_FRECUENCY));
                snprintf(scroll_text,sizeof(scroll_text), "%d Hz",
                         id3->frequency);
                lcd_puts(0, 1, scroll_text);
                break;

            case 10:
                lcd_puts(0, 0, str(LANG_ID3_PATH));
                lcd_puts_scroll(0, 1, id3->path);
                break;
        }
        lcd_update();

        button = button_get(true);

        switch(button)
        {
            case SETTINGS_DEC:
                if (menu_pos > 0)
                    menu_pos--;
                else
                    menu_pos = menu_max;
                break;

            case SETTINGS_INC:
                if (menu_pos < menu_max)
                    menu_pos++;
                else
                    menu_pos = 0;
                break;
            
            case SETTINGS_CANCEL:
                lcd_stop_scroll();
                /* eat release event */
                button_get(true);
                exit = true;
                break;

            default:
                if(default_event_handler(button) ==  SYS_USB_CONNECTED)
                    return true;
                break;
        }
    }
    return false;
}

