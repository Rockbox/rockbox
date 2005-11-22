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
#include "abrepeat.h"
#include "gwps-common.h"
#include "splash.h"
#include "statusbar.h"
#include "screen_access.h"
#include "quickscreen.h"

#if defined(HAVE_LCD_BITMAP)
#include "widgets.h"
#endif
#ifdef HAVE_MMC
#include "ata_mmc.h"
#endif
#if CONFIG_CODEC == SWCODEC
#include "dsp.h"
#endif

#ifdef HAVE_LCD_BITMAP
#define SCROLLBAR_WIDTH  6

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

void usb_display_info(struct screen * display)
{
    display->clear_display();

#ifdef HAVE_LCD_BITMAP
    /* Center bitmap on screen */
    display->mono_bitmap(usb_logo,
                display->width/2-BMPWIDTH_usb_logo/2,
                display->height/2-BMPHEIGHT_usb_logo/2,
                BMPWIDTH_usb_logo,
                BMPHEIGHT_usb_logo);
    display->update();
#else
    display->double_height(false);
    display->puts(0, 0, "[USB Mode]");
#ifdef SIMULATOR
    display->update();
#endif /* SIMULATOR */
#endif /* HAVE_LCD_BITMAP */
}

void usb_screen(void)
{
#ifdef USB_NONE
    /* nothing here! */
#else
    int i;
    FOR_NB_SCREENS(i) {
        screens[i].backlight_on();
        usb_display_info(&screens[i]);
    }
#ifdef HAVE_LCD_CHARCELLS
    status_set_param(false);
    status_set_audio(false);
    status_set_usb(true);
#endif /* HAVE_LCD_BITMAP */
    gui_syncstatusbar_draw(&statusbars, true);
#ifdef SIMULATOR
    while (button_get(true) & BUTTON_REL);
#else
    usb_acknowledge(SYS_USB_CONNECTED_ACK);
    while(usb_wait_for_disconnect_w_tmo(&button_queue, HZ)) {
        if(usb_inserted()) {
#ifdef HAVE_MMC /* USB-MMC bridge can report activity */
            led(mmc_usb_active(HZ));
#endif /* HAVE_MMC */
            gui_syncstatusbar_draw(&statusbars, false);
        }
    }
#endif /* SIMULATOR */
#ifdef HAVE_LCD_CHARCELLS
    status_set_usb(false);
#endif /* HAVE_LCD_CHARCELLS */
    FOR_NB_SCREENS(i)
        screens[i].backlight_on();
#endif /* USB_NONE */
}

#ifdef HAVE_MMC
int mmc_remove_request(void)
{
    struct event ev;
    int i;
    FOR_NB_SCREENS(i)
        screens[i].clear_display();
    gui_syncsplash(1, true, str(LANG_REMOVE_MMC));
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

#if defined(HAVE_CHARGING) && !defined(HAVE_POWEROFF_WHILE_CHARGING)

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
        lcd_mono_bitmap(charging_logo, pox_x, pox_y + 8, sizeof(charging_logo), 8);
        lcd_set_drawmode(DRMODE_FG);
        /* upper line */
        charging_logo[0] = charging_logo[1] = 0x00;
        memset(charging_logo+2, 0x80, 34);
        lcd_mono_bitmap(charging_logo, pox_x, pox_y, sizeof(charging_logo), 8);
        /* lower line */
        memset(charging_logo+2, 0x01, 34);
        lcd_mono_bitmap(charging_logo, pox_x, pox_y + 16, sizeof(charging_logo), 8);
        lcd_set_drawmode(DRMODE_SOLID);
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
        lcd_mono_bitmap(charging_logo, pox_x, pox_y + 8, sizeof(charging_logo), 8);
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

    ide_power_enable(false); /* power down the disk, else would be spinning */

    lcd_clear_display();
    backlight_set_timeout(global_settings.backlight_timeout);
#ifdef HAVE_REMOTE_LCD
    remote_backlight_set_timeout(global_settings.remote_backlight_timeout);
#endif
    backlight_set_on_when_charging(global_settings.backlight_on_when_charging);
    gui_syncstatusbar_draw(&statusbars, true);

#ifdef HAVE_LCD_CHARCELLS
    logo_lock_patterns(true);
#endif
    charging_display_info(false);

    do
    {
        gui_syncstatusbar_draw(&statusbars, false);
        charging_display_info(true);
        button = button_get_w_tmo(HZ/3);
        if (button == BUTTON_ON)
            rc = 2;
        else if (usb_detect())
            rc = 3;
        else if (!charger_inserted())
            rc = 1;
    } while (!rc);

#ifdef HAVE_LCD_CHARCELLS
    logo_lock_patterns(false);
#endif
    return rc;
}
#endif /* HAVE_CHARGING && !HAVE_POWEROFF_WHILE_CHARGING */


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
            lcd_mono_bitmap(bitmap_icons_7x8[Icon_UpArrow],
                            LCD_WIDTH/2 - 3, h*2, 7, 8);

            snprintf(buf, sizeof buf, "%d.%d%%", pitch / 10, pitch % 10 );
            lcd_getstringsize(buf,&w,&h);
            lcd_putsxy((LCD_WIDTH-w)/2, h, buf);

            ptr = str(LANG_PITCH_DOWN);
            lcd_getstringsize(ptr,&w,&h);
            lcd_putsxy((LCD_WIDTH-w)/2, LCD_HEIGHT - h, ptr);
            lcd_mono_bitmap(bitmap_icons_7x8[Icon_DownArrow],
                            LCD_WIDTH/2 - 3, LCD_HEIGHT - h*3, 7, 8);

            ptr = str(LANG_PAUSE);
            lcd_getstringsize(ptr,&w,&h);
            lcd_putsxy((LCD_WIDTH-(w/2))/2, LCD_HEIGHT/2 - h/2, ptr);
            lcd_mono_bitmap(bitmap_icons_7x8[Icon_Pause],
                            (LCD_WIDTH-(w/2))/2-10, LCD_HEIGHT/2 - h/2, 7, 8);

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
#endif

#if (CONFIG_KEYPAD == RECORDER_PAD) || (CONFIG_KEYPAD == IRIVER_H100_PAD) ||\
    (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define bool_to_int(b)\
    b?1:0
#define int_to_bool(i)\
    i==0?false:true

void quick_screen_quick_apply(struct gui_quickscreen *qs)
{
    global_settings.playlist_shuffle=int_to_bool(option_select_get_selected(qs->left_option));
    global_settings.dirfilter=option_select_get_selected(qs->bottom_option);
    global_settings.repeat_mode=option_select_get_selected(qs->right_option);
}

bool quick_screen_quick(void)
{
    bool res, oldrepeat;
    struct option_select left_option;
    struct option_select bottom_option;
    struct option_select right_option;
    struct opt_items left_items[] = {
        [0]={ STR(LANG_OFF) },
        [1]={ STR(LANG_ON) }
    };
    static const struct opt_items bottom_items[] = {
        [SHOW_ALL]={ STR(LANG_FILTER_ALL) },
        [SHOW_SUPPORTED]={ STR(LANG_FILTER_SUPPORTED) },
        [SHOW_MUSIC]={ STR(LANG_FILTER_MUSIC) },
        [SHOW_PLAYLIST]={ STR(LANG_FILTER_PLAYLIST) },
        [SHOW_ID3DB]={ STR(LANG_FILTER_ID3DB) }
    };
    static const struct opt_items right_items[] = {
        [REPEAT_OFF]={ STR(LANG_OFF) },
        [REPEAT_ALL]={ STR(LANG_REPEAT_ALL) },
        [REPEAT_ONE]={ STR(LANG_REPEAT_ONE) },
        [REPEAT_SHUFFLE]={ STR(LANG_SHUFFLE) },
#ifdef AB_REPEAT_ENABLE
        [REPEAT_AB]={ STR(LANG_REPEAT_AB) }
#endif
    };
    struct gui_quickscreen qs;

    option_select_init_items(&left_option,
                             str(LANG_SHUFFLE),
                             bool_to_int(global_settings.playlist_shuffle),
                             left_items,
                             2);
    option_select_init_items(&bottom_option,
                             str(LANG_FILTER),
                             global_settings.dirfilter,
                             bottom_items,
                             sizeof(bottom_items)/sizeof(struct opt_items));
    option_select_init_items(&right_option,
                             str(LANG_REPEAT),
                             global_settings.repeat_mode,
                             right_items,
                             sizeof(right_items)/sizeof(struct opt_items));

    gui_quickscreen_init(&qs, &left_option, &bottom_option, &right_option,
                         str(LANG_F2_MODE), &quick_screen_quick_apply);
    oldrepeat=global_settings.repeat_mode;
    res=gui_syncquickscreen_run(&qs);
    if(!res)
    {
        if ( oldrepeat != global_settings.repeat_mode &&
            (audio_status() & AUDIO_STATUS_PLAY) )
            audio_flush_and_reload_tracks();
        if(global_settings.playlist_shuffle
           && audio_status() & AUDIO_STATUS_PLAY)
        {
#if CONFIG_CODEC == SWCODEC
            dsp_set_replaygain(true);
#endif
            if (global_settings.playlist_shuffle)
                playlist_randomise(NULL, current_tick, true);
            else
                playlist_sort(NULL, true);
        }
        settings_save();
    }
    return(res);
}

#ifdef BUTTON_F3
void quick_screen_f3_apply(struct gui_quickscreen *qs)
{
    global_settings.scrollbar=int_to_bool(option_select_get_selected(qs->left_option));

    global_settings.flip_display=int_to_bool(option_select_get_selected(qs->bottom_option));
    button_set_flip(global_settings.flip_display);
    lcd_set_flip(global_settings.flip_display);

    global_settings.statusbar=int_to_bool(option_select_get_selected(qs->right_option));
    gui_syncstatusbar_draw(&statusbars, true);
}

bool quick_screen_f3(void)
{
    bool res;
    struct option_select left_option;
    struct option_select bottom_option;
    struct option_select right_option;
    static const struct opt_items onoff_items[] = {
        [0]={ STR(LANG_OFF) },
        [1]={ STR(LANG_ON) }
    };
    static const struct opt_items yesno_items[] = {
        [0]={ STR(LANG_SET_BOOL_NO) },
        [1]={ STR(LANG_SET_BOOL_YES) }
    };

    struct gui_quickscreen qs;

    option_select_init_items(&left_option,
                             str(LANG_F3_SCROLL),
                             bool_to_int(global_settings.scrollbar),
                             onoff_items,
                             2);
    option_select_init_items(&bottom_option,
                             str(LANG_FLIP_DISPLAY),
                             bool_to_int(global_settings.flip_display),
                             yesno_items,
                             2);
    option_select_init_items(&right_option,
                             str(LANG_F3_STATUS),
                             bool_to_int(global_settings.statusbar),
                             onoff_items,
                             2);
    gui_quickscreen_init(&qs, &left_option, &bottom_option, &right_option,
                         str(LANG_F3_BAR), &quick_screen_f3_apply);
    res=gui_syncquickscreen_run(&qs);
    if(!res)
        settings_save();
    return(res);
}
#endif /* BUTTON_F3 */
#endif /* CONFIG_KEYPAD in (RECORDER_PAD |IRIVER_H100_PAD | IRIVER_H300_PAD) */

#if defined(HAVE_CHARGING) || defined(SIMULATOR)
void charging_splash(void)
{
    gui_syncsplash(2*HZ, true, str(LANG_BATTERY_CHARGE));
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
    int lastmode = lcd_get_drawmode();

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

        lcd_set_drawmode(DRMODE_COMPLEMENT);
        lcd_fillrect(cursor[cursorpos][INDEX_X],
                     cursor[cursorpos][INDEX_Y] + lcd_getymargin(),
                     cursor[cursorpos][INDEX_WIDTH],
                     line_height);
        lcd_set_drawmode(DRMODE_SOLID);

        lcd_puts(0, 4, str(LANG_TIME_SET));
        lcd_puts(0, 5, str(LANG_TIME_REVERT));
#ifdef HAVE_LCD_BITMAP
        gui_syncstatusbar_draw(&statusbars, true);
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

    lcd_set_drawmode(lastmode);
    return false;
}
#endif /* defined(HAVE_LCD_BITMAP) && defined (HAVE_RTC) */

#if (CONFIG_KEYPAD == RECORDER_PAD) && !defined(HAVE_SW_POWEROFF)
bool shutdown_screen(void)
{
    int button;
    bool done = false;

    lcd_stop_scroll();

    gui_syncsplash(0, true, str(LANG_CONFIRM_SHUTDOWN));

    while(!done)
    {
        button = button_get_w_tmo(HZ*2);
        switch(button)
        {
            case BUTTON_OFF:
                sys_poweroff();
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

int draw_id3_item(int line, int top, int header, const char* body)
{
    if (line >= top)
    {
#if defined(HAVE_LCD_BITMAP)
        const int rows = LCD_HEIGHT / font_get(FONT_UI)->height;
#else
        const int rows = 2;
#endif
        int y = line - top;

        if (y < rows)
        {
            lcd_puts(0, y, str(header));
        }

        if (++y < rows)
        {
            lcd_puts_scroll(0, y,
                body ? (const unsigned char*) body : str(LANG_ID3_NO_INFO));
        }
    }

    return line + 2;
}

#if CONFIG_CODEC == SWCODEC
#define ID3_ITEMS   13
#else
#define ID3_ITEMS   11
#endif

bool browse_id3(void)
{
    char buf[64];
    const struct mp3entry* id3 = audio_current_track();
#if defined(HAVE_LCD_BITMAP)
    const int y_margin = lcd_getymargin();
    const int line_height = font_get(FONT_UI)->height;
    const int rows = (LCD_HEIGHT - y_margin) / line_height;
    const bool show_scrollbar = global_settings.scrollbar
        && (ID3_ITEMS * 2 > rows);
#else
    const int rows = 2;
#endif
    const int top_max = (ID3_ITEMS * 2) - (rows & ~1);
    int top = 0;
    int button;
    bool exit = false;

    if (!id3 || (!(audio_status() & AUDIO_STATUS_PLAY)))
    {
        return false;
    }

#if defined(HAVE_LCD_BITMAP)
    lcd_setmargins(show_scrollbar ? SCROLLBAR_WIDTH : 0, y_margin);
#endif

    while (!exit)
    {
        int line = 0;
        int old_top = top;
        char* body;

        lcd_clear_display();
        gui_syncstatusbar_draw(&statusbars, true);
        line = draw_id3_item(line, top, LANG_ID3_TITLE,  id3->title);
        line = draw_id3_item(line, top, LANG_ID3_ARTIST, id3->artist);
        line = draw_id3_item(line, top, LANG_ID3_ALBUM,  id3->album);

        if (id3->track_string)
        {
            body = id3->track_string;
        }
        else if (id3->tracknum)
        {
            snprintf(buf, sizeof(buf), "%d", id3->tracknum);
            body = buf;
        }
        else
        {
            body = NULL;
        }

        line = draw_id3_item(line, top, LANG_ID3_TRACKNUM, body);

        body = id3->genre_string ? id3->genre_string : id3_get_genre(id3);
        line = draw_id3_item(line, top, LANG_ID3_GENRE, body);

        if (id3->year_string)
        {
            body = id3->year_string;
        }
        else if (id3->year)
        {
            snprintf(buf, sizeof(buf), "%d", id3->year);
            body = buf;
        }
        else
        {
            body = NULL;
        }

        line = draw_id3_item(line, top, LANG_ID3_YEAR, body);

        gui_wps_format_time(buf, sizeof(buf), id3->length);
        line = draw_id3_item(line, top, LANG_ID3_LENGHT, buf);

        snprintf(buf, sizeof(buf), "%d/%d", playlist_get_display_index(),
            playlist_amount());
        line = draw_id3_item(line, top, LANG_ID3_PLAYLIST, buf);

        snprintf(buf, sizeof(buf), "%d kbps%s", id3->bitrate, 
            id3->vbr ? str(LANG_ID3_VBR) : (const unsigned char*) "");
        line = draw_id3_item(line, top, LANG_ID3_BITRATE, buf);

        snprintf(buf, sizeof(buf), "%ld Hz", id3->frequency);
        line = draw_id3_item(line, top, LANG_ID3_FRECUENCY, buf);

#if CONFIG_CODEC == SWCODEC
        line = draw_id3_item(line, top, LANG_ID3_TRACK_GAIN,
            id3->track_gain_string);

        line = draw_id3_item(line, top, LANG_ID3_ALBUM_GAIN,
            id3->album_gain_string);
#endif

        line = draw_id3_item(line, top, LANG_ID3_PATH, id3->path);

#if defined(HAVE_LCD_BITMAP)
        if (show_scrollbar)
        {
            scrollbar(0, y_margin, SCROLLBAR_WIDTH - 1, rows * line_height,
                ID3_ITEMS * 2 + (rows & 1), top, top + rows, VERTICAL);
        }
#endif

        while (!exit && (top == old_top))
        {
            gui_syncstatusbar_draw(&statusbars, false);
            lcd_update();
            button = button_get_w_tmo(HZ / 2);

            switch(button)
            {
            /* It makes more sense to have the keys mapped "backwards" when
             * scrolling a list.
             */
#if defined(HAVE_LCD_BITMAP)
            case SETTINGS_INC:
            case SETTINGS_INC | BUTTON_REPEAT:
#else
            case SETTINGS_DEC:
#endif
                if (top > 0)
                {
                    top -= 2;
                }
                else if (!(button & BUTTON_REPEAT))
                {
                    top = top_max;
                }

                break;

#if defined(HAVE_LCD_BITMAP)
            case SETTINGS_DEC:
            case SETTINGS_DEC | BUTTON_REPEAT:
#else
            case SETTINGS_INC:
#endif
                if (top < top_max)
                {
                    top += 2;
                }
                else if (!(button & BUTTON_REPEAT))
                {
                    top = 0;
                }

                break;

#ifdef SETTINGS_OK2
            case SETTINGS_OK2:
#endif
            case SETTINGS_CANCEL:
                lcd_stop_scroll();
                /* Eat release event */
                button_get(true);
                exit = true;
                break;

            default:
                if (default_event_handler(button) == SYS_USB_CONNECTED)
                {
                    return true;
                }

                break;
            }
        }
    }

    return false;
}

bool set_rating(void)
{
    struct mp3entry* id3 = audio_current_track();
    int button;
    bool exit = false;
    char rating_text[20];

    if (!(audio_status() & AUDIO_STATUS_PLAY)||id3==NULL)
        return false;
    while (!exit)
    {
        lcd_clear_display();
        lcd_puts(0, 0, str(LANG_RATING));
        snprintf(rating_text,sizeof(rating_text),"%d",id3->rating);
        lcd_puts(0,1,rating_text);
        lcd_update();
        button = button_get(true);

        switch(button)
        {
            case SETTINGS_DEC:
                if (id3->rating > 0)
                    id3->rating--;
                else
                    id3->rating = 10;
                break;

            case SETTINGS_INC:
                if (id3->rating < 10)
                    id3->rating++;
                else
                    id3->rating = 0;
                break;
            case SETTINGS_CANCEL:
#ifdef SETTINGS_OK2
            case SETTINGS_OK2:
#endif
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
