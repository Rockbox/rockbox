/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2016 by Amaury Pouly
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

#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "lcd.h"
#include "font.h"
#include "adc.h"
#include "adc-target.h"
#include "button.h"
#include "button-target.h"
#include "powermgmt.h"
#include "power-nwz.h"
#include "nvp-nwz.h"
#include "nwz_sysinfo.h"
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "nwzlinux_codec.h"

/* NOTE: some targets with touchscreen don't have the usual keypad, on those
 * we use a mixture of rewind/forward/volume+/- to emulate it */
#define ACT_NONE    0
#define ACT_CANCEL  1
#define ACT_OK      2
#define ACT_PREV    3
#define ACT_NEXT    4
#define ACT_DEC     5
#define ACT_INC     6
#define ACT_REPEAT  0x1000

int xlate_button(int btn)
{
    switch(btn)
    {
#ifdef BUTTON_POWER
        case BUTTON_POWER:
#endif
        case BUTTON_BACK:
            return ACT_CANCEL;
        case BUTTON_PLAY:
            return ACT_OK;
        case BUTTON_UP:
#ifdef BUTTON_FF
        case BUTTON_FF:
#endif
            return ACT_PREV;
        case BUTTON_RIGHT:
        case BUTTON_VOL_UP:
            return ACT_INC;
        case BUTTON_DOWN:
#ifdef BUTTON_FF
        case BUTTON_REW:
#endif
            return ACT_NEXT;
        case BUTTON_LEFT:
        case BUTTON_VOL_DOWN:
            return ACT_DEC;
        default:
            return ACT_NONE;
    }
}

int my_get_status(void)
{
    return xlate_button(button_status());
}

int my_get_action(int tmo)
{
    int btn = button_get_w_tmo(tmo);
    while(btn & BUTTON_REL)
        btn = button_get_w_tmo(tmo);
    bool repeat = btn & BUTTON_REPEAT;
    int act = xlate_button(btn & ~BUTTON_REPEAT);
    if(repeat)
        act |= ACT_REPEAT;
    return act;
}

bool dbg_hw_info_adc(void)
{
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        int button = my_get_action(HZ / 25);
        switch(button)
        {
            case ACT_OK:
                lcd_setfont(FONT_UI);
                return true;
            case ACT_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }

        lcd_clear_display();

        /* add battery readout in mV, this it is not the direct output of a channel */
        lcd_putsf(0, 0, "Battery(mV) %d", _battery_voltage());
        for(unsigned i = NWZ_ADC_MIN_CHAN; i <= NWZ_ADC_MAX_CHAN; i++)
            lcd_putsf(0, i + 1, "%7s %3d", adc_name(i), adc_read(i));

        lcd_update();
        yield();
    }
}

static const char *charge_status_name(int chgstat)
{
    switch(chgstat)
    {
        case NWZ_POWER_STATUS_CHARGE_STATUS_CHARGING: return "charging";
        case NWZ_POWER_STATUS_CHARGE_STATUS_SUSPEND: return "suspend";
        case NWZ_POWER_STATUS_CHARGE_STATUS_TIMEOUT: return "timeout";
        case NWZ_POWER_STATUS_CHARGE_STATUS_NORMAL: return "normal";
        default: return "unknown";
    }
}

static const char *get_batt_gauge_name(int gauge)
{
    switch(gauge)
    {
        case NWZ_POWER_BAT_NOBAT: return "no batt";
        case NWZ_POWER_BAT_VERYLOW: return "very low";
        case NWZ_POWER_BAT_LOW: return "low";
        case NWZ_POWER_BAT_GAUGE0: return "____";
        case NWZ_POWER_BAT_GAUGE1: return "O___";
        case NWZ_POWER_BAT_GAUGE2: return "OO__";
        case NWZ_POWER_BAT_GAUGE3: return "OOO_";
        case NWZ_POWER_BAT_GAUGE4: return "OOOO";
        default: return "unknown";
    }
}

static const char *acc_charge_mode_name(int mode)
{
    switch(mode)
    {
        case NWZ_POWER_ACC_CHARGE_NONE: return "none";
        case NWZ_POWER_ACC_CHARGE_VBAT: return "vbat";
        case NWZ_POWER_ACC_CHARGE_VSYS: return "vsys";
        default: return "unknown";
    }
}

bool dbg_hw_info_power(void)
{
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        int button = my_get_action(HZ / 25);
        switch(button)
        {
            case ACT_OK:
                lcd_setfont(FONT_UI);
                return true;
            case ACT_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }

        lcd_clear_display();

        int line = 0;
        int status = nwz_power_get_status();
        int chgstat = status & NWZ_POWER_STATUS_CHARGE_STATUS;
        int acc_chg_mode = nwz_power_get_acc_charge_mode();
        lcd_putsf(0, line++, "ac detected: %s",
            (status & NWZ_POWER_STATUS_AC_DET) ? "yes" : "no");
        lcd_putsf(0, line++, "vbus detected: %s   ",
            (status & NWZ_POWER_STATUS_VBUS_DET) ? "yes" : "no");
        lcd_putsf(0, line++, "vbus voltage: %d mV (AD=%d)",
            nwz_power_get_vbus_voltage(), nwz_power_get_vbus_adval());
        lcd_putsf(0, line++, "vbus limit: %d mA      ",
            nwz_power_get_vbus_limit());
        lcd_putsf(0, line++, "vsys voltage: %d mV (AD=%d)",
            nwz_power_get_vsys_voltage(), nwz_power_get_vsys_adval());
        lcd_putsf(0, line++, "charge switch: %s       ",
            nwz_power_get_charge_switch() ? "on" : "off");
        lcd_putsf(0, line++, "full voltage: %s V      ",
            (status & NWZ_POWER_STATUS_CHARGE_LOW) ? "4.1" : "4.2");
        lcd_putsf(0, line++, "current limit: %d mA",
            nwz_power_get_charge_current());
        lcd_putsf(0, line++, "charge status: %s (%x)",
            charge_status_name(chgstat), chgstat);
        lcd_putsf(0, line++, "battery full: %s       ",
            nwz_power_is_fully_charged() ? "yes" : "no");
        lcd_putsf(0, line++, "bat gauge: %s (%d)",
            get_batt_gauge_name(nwz_power_get_battery_gauge()),
            nwz_power_get_battery_gauge());
        lcd_putsf(0, line++, "avg voltage: %d mV (AD=%d)",
            nwz_power_get_battery_voltage(), nwz_power_get_battery_adval());
        lcd_putsf(0, line++, "sample count: %d       ",
            nwz_power_get_sample_count());
        lcd_putsf(0, line++, "raw voltage: %d mV (AD=%d)",
            nwz_power_get_vbat_voltage(), nwz_power_get_vbat_adval());
        lcd_putsf(0, line++, "acc charge mode: %s (%d)",
            acc_charge_mode_name(acc_chg_mode), acc_chg_mode);

        lcd_update();
        yield();
    }
}

bool dbg_hw_info_button(void)
{
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        int btn = my_get_action(0);
        switch(btn)
        {
            case ACT_OK:
                lcd_setfont(FONT_UI);
                return true;
            case ACT_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }

        lcd_clear_display();
        int line = 0;

#ifdef HAVE_BUTTON_DATA
        int data;
        btn = button_read_device(&data);
#else
        btn = button_read_device();
#endif
        lcd_putsf(0, line++, "raw buttons: %x", btn);
#ifdef HAS_BUTTON_HOLD
        lcd_putsf(0, line++, "hold: %d", button_hold());
#endif
#ifdef HAVE_HEADPHONE_DETECTION
        lcd_putsf(0, line++, "headphones: %d", headphones_inserted());
#endif
#ifdef HAVE_BUTTON_DATA
#ifdef HAVE_TOUCHSCREEN
        lcd_putsf(0, line++, "touch: x=%d y=%d", data >> 16, data & 0xffff);
#else
        lcd_putsf(0, line++, "data: %d", data);
#endif
#endif

        lcd_update();
        yield();
    }
}

int read_sysinfo(int ioctl_nr, unsigned int *val)
{
    int fd = open(NWZ_SYSINFO_DEV, O_RDONLY);
    if(fd < 0)
        return -1;
    int ret = ioctl(fd, ioctl_nr, val);
    close(fd);
    return ret;
}

bool dbg_hw_info_sysinfo(void)
{
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        int btn = my_get_action(0);
        switch(btn)
        {
            case ACT_OK:
                lcd_setfont(FONT_UI);
                return true;
            case ACT_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }

        lcd_clear_display();
        int line = 0;
        unsigned int val;

        lcd_putsf(0, line++, "mid: %x (%s)", nwz_get_model_id(), nwz_get_model_name());
#define print_info(def, name) \
        if(read_sysinfo(NWZ_SYSINFO_GET_##def##_TYPE, &val) < 0) \
            lcd_putsf(0, line++, "%s: -", name); \
        else \
            lcd_putsf(0, line++, "%s: %d", name, val);

        /* WARNING the interpretation of values is difficult because it changes
         * very often */
        /* DAC type: ... */
        print_info(DAC, "dac")
        /* Noise cancelling: 0=no, 1=yes */
        print_info(NCR, "nc")
        /* Speaker: 0=no, 1=yes */
        print_info(SPK, "spk")
        /* Microphone: 0=no, 1=yes */
        print_info(MIC, "mic")
        /* Video encoder: 0=no, ... */
        print_info(NPE, "videoenc")
        /* FM receiver: 0=no, 1=si470x */
        print_info(FMT, "fm")
        /* Touch screen: 0=none, ... */
        print_info(TSP, "touch")
        /* Bluetooth: 0=no, 1=yes */
        print_info(WAB, "bt")
        /* SD card: 0=no, ... */
        print_info(SDC, "sd")

        lcd_update();
        yield();
    }
}

#include "pcm-alsa.h"

bool dbg_hw_info_audio(void)
{
    lcd_setfont(FONT_SYSFIXED);
    int vol = 0;
    enum { VOL, ACOUSTIC, CUEREV, NR_SETTINGS } setting = VOL;

    while(1)
    {
        int btn = my_get_action(0);
        switch(btn)
        {
            case ACT_PREV:
                setting = (setting + NR_SETTINGS - 1) % NR_SETTINGS;
                break;
            case ACT_NEXT:
                setting = (setting + 1) % NR_SETTINGS;
                break;
            case ACT_OK:
                lcd_setfont(FONT_UI);
                return true;
            case ACT_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }
        if(btn == ACT_INC || btn == ACT_DEC)
        {
            bool inc = (btn == ACT_INC);
            switch(setting)
            {
                case VOL:
                    vol += inc ? 1 : -1;
                    pcm_set_mixer_volume(vol, vol);
                    break;
                case ACOUSTIC:
                    audiohw_enable_acoustic(!audiohw_acoustic_enabled());
                    break;
                case CUEREV:
                    audiohw_enable_cuerev(!audiohw_cuerev_enabled());
                    break;
                default:
                    break;
            }
        }

        lcd_clear_display();
        int line = 0;
#define SEL_COL(item) lcd_set_foreground(item == setting ? LCD_RGBPACK(255, 0, 0) : LCD_WHITE);

        SEL_COL(VOL)
        lcd_putsf(0, line++, "vol: %d dB", vol);
        SEL_COL(ACOUSTIC)
        lcd_putsf(0, line++, "acoustic: %s", audiohw_acoustic_enabled() ? "on" : "off");
        SEL_COL(CUEREV)
        lcd_putsf(0, line++, "cue/rev: %s", audiohw_cuerev_enabled() ? "on" : "off");
        lcd_set_foreground(LCD_WHITE);
#undef SEL_COL

        lcd_update();
        yield();
    }
}

static struct
{
    const char *name;
    bool (*fn)(void);
} debug_screens[] =
{
    {"sysinfo", dbg_hw_info_sysinfo},
    {"adc", dbg_hw_info_adc},
    {"power", dbg_hw_info_power},
    {"button", dbg_hw_info_button},
    {"audio", dbg_hw_info_audio},
};

bool dbg_hw_info(void)
{
    int nr_lines = lcd_getheight() / font_get(lcd_getfont())->height;
    int len = ARRAYLEN(debug_screens);
    int top_visible = 0;
    int highlight = 0;
    while(1)
    {
        int button = my_get_action(HZ / 10);
        switch(button)
        {
            case ACT_NEXT:
                highlight = (highlight + 1) % len;
                break;
            case ACT_PREV:
                highlight = (highlight + len - 1) % len;
                break;
            case ACT_OK:
                // if the screen returns true, advance to next screen
                while(debug_screens[highlight].fn())
                    highlight = (highlight + 1) % len;
                lcd_setfont(FONT_UI);
                break;
            case ACT_CANCEL:
                return false;
        }
        // adjust top visible if needed
        if(highlight < top_visible)
            top_visible = highlight;
        else if(highlight >= top_visible + nr_lines)
            top_visible = highlight - nr_lines + 1;

        lcd_clear_display();

        for(int i = top_visible; i < len && i < top_visible + nr_lines; i++)
        {
            if(i == highlight)
            {
                lcd_set_foreground(LCD_BLACK);
                lcd_set_background(LCD_RGBPACK(255, 255, 0));
            }
            else
            {
                lcd_set_foreground(LCD_WHITE);
                lcd_set_background(LCD_BLACK);
            }
            lcd_putsf(0, i - top_visible, "%s", debug_screens[i].name);
        }
        lcd_set_foreground(LCD_WHITE);
        lcd_set_background(LCD_BLACK);

        lcd_update();
        yield();
    }
    return false;
}
