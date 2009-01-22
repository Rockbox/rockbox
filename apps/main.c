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

#include "storage.h"
#include "disk.h"
#include "fat.h"
#include "lcd.h"
#include "rtc.h"
#include "debug.h"
#include "led.h"
#include "kernel.h"
#include "button.h"
#include "tree.h"
#include "filetypes.h"
#include "panic.h"
#include "menu.h"
#include "system.h"
#include "usb.h"
#include "powermgmt.h"
#include "adc.h"
#include "i2c.h"
#ifndef DEBUG
#include "serial.h"
#endif
#include "audio.h"
#include "mp3_playback.h"
#include "thread.h"
#include "settings.h"
#include "backlight.h"
#include "status.h"
#include "debug_menu.h"
#include "version.h"
#include "sprintf.h"
#include "font.h"
#include "language.h"
#include "gwps.h"
#include "playlist.h"
#include "buffer.h"
#include "rolo.h"
#include "screens.h"
#include "power.h"
#include "talk.h"
#include "plugin.h"
#include "misc.h"
#include "dircache.h"
#ifdef HAVE_TAGCACHE
#include "tagcache.h"
#include "tagtree.h"
#endif
#include "lang.h"
#include "string.h"
#include "splash.h"
#include "eeprom_settings.h"
#include "scrobbler.h"
#include "icon.h"

#ifdef IPOD_ACCESSORY_PROTOCOL
#include "iap.h"
#endif

#if (CONFIG_CODEC == SWCODEC)
#include "playback.h"
#endif
#if (CONFIG_CODEC == SWCODEC) && defined(HAVE_RECORDING) && !defined(SIMULATOR)
#include "pcm_record.h"
#endif

#ifdef BUTTON_REC
    #define SETTINGS_RESET BUTTON_REC
#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
    #define SETTINGS_RESET BUTTON_A
#endif

#if CONFIG_TUNER
#include "radio.h"
#endif
#if (CONFIG_STORAGE & STORAGE_MMC)
#include "ata_mmc.h"
#endif

#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif

#if CONFIG_USBOTG == USBOTG_ISP1362
#include "isp1362.h"
#endif

#if CONFIG_USBOTG == USBOTG_M5636
#include "m5636.h"
#endif

#include "cuesheet.h"

#ifdef SIMULATOR
#include "system-sdl.h"
#endif

/*#define AUTOROCK*/ /* define this to check for "autostart.rock" on boot */

const char appsversion[]=APPSVERSION;

static void init(void);
        
#ifdef SIMULATOR
void app_main(void)
#else
static void app_main(void)
#endif
{
    int i;
    init();
    FOR_NB_SCREENS(i)
    {
        screens[i].clear_display();
        screens[i].update();
    }
    tree_gui_init();
#ifdef HAVE_TOUCHSCREEN
    touchscreen_set_mode(TOUCHSCREEN_BUTTON);
#endif
    viewportmanager_set_statusbar(true);
    add_event(GUI_EVENT_STATUSBAR_TOGGLE, false, 
              viewportmanager_statusbar_changed);
#ifdef HAVE_USBSTACK
    /* All threads should be created and public queues registered by now */
    usb_start_monitoring();
#endif
    root_menu();
}

static int init_dircache(bool preinit)
{
#ifdef HAVE_DIRCACHE
    int result = 0;
    bool clear = false;
    
    if (preinit)
        dircache_init();
    
    if (!global_settings.dircache)
        return 0;
    
# ifdef HAVE_EEPROM_SETTINGS
    if (firmware_settings.initialized && firmware_settings.disk_clean 
        && preinit)
    {
        result = dircache_load();

        if (result < 0)
        {
            firmware_settings.disk_clean = false;
            if (global_status.dircache_size <= 0)
            {
                /* This will be in default language, settings are not
                   applied yet. Not really any easy way to fix that. */
                splash(0, str(LANG_SCANNING_DISK));
                clear = true;
            }
            
            dircache_build(global_status.dircache_size);
        }
    }
    else
# endif
    {
        if (preinit)
            return -1;
        
        if (!dircache_is_enabled()
            && !dircache_is_initializing())
        {
            if (global_status.dircache_size <= 0)
            {
                splash(0, str(LANG_SCANNING_DISK));
                clear = true;
            }
            result = dircache_build(global_status.dircache_size);
        }
        
        if (result < 0)
        {
            /* Initialization of dircache failed. Manual action is
             * necessary to enable dircache again.
             */
            splashf(0, "Dircache failed, disabled. Result: %d", result);
            global_settings.dircache = false;
        }
            
    }
    
    if (clear)
    {
        backlight_on();
        show_logo();
        global_status.dircache_size = dircache_get_cache_size();
        status_save();
    }
    
    return result;
#else
    (void)preinit;
    return 0;
#endif
}

#ifdef HAVE_TAGCACHE
static void init_tagcache(void)
{
    bool clear = false;

    tagcache_init();
    
    while (!tagcache_is_initialized())
    {
#ifdef HAVE_LCD_CHARCELLS
        char buf[32];
#endif
        int ret = tagcache_get_commit_step();

        if (ret > 0)
        {
#if CONFIG_CODEC == SWCODEC
            /* hwcodec can't use voice here, as the database commit
             * uses the audio buffer. */
            static long talked_tick = 0;
            if(global_settings.talk_menu
               && (talked_tick == 0
                   || TIME_AFTER(current_tick, talked_tick+7*HZ)))
            {
                talked_tick = current_tick;
                talk_id(LANG_TAGCACHE_INIT, false);
                talk_number(ret, true);
                talk_id(VOICE_OF, true);
                talk_number(tagcache_get_max_commit_step(), true);
            }
#endif
#ifdef HAVE_LCD_BITMAP
            splashf(0, "%s [%d/%d]",
                str(LANG_TAGCACHE_INIT), ret, 
                tagcache_get_max_commit_step());
#else
            lcd_double_height(false);
            snprintf(buf, sizeof(buf), " DB [%d/%d]", ret, 
                tagcache_get_max_commit_step());
            lcd_puts(0, 1, buf);
            lcd_update();
#endif
            clear = true;
        }
        sleep(HZ/4);
    }
    tagtree_init();

    if (clear)
    {
        backlight_on();
        show_logo();
    }
}
#endif

#ifdef SIMULATOR

static void init(void)
{
    kernel_init();
    buffer_init();
    enable_irq();
    lcd_init();
#ifdef HAVE_REMOTE_LCD
    lcd_remote_init();
#endif
    font_init();
    show_logo();
    button_init();
    backlight_init();
    lang_init();
#ifdef DEBUG
    debug_init();
#endif
    /* Must be done before any code uses the multi-screen APi */
    gui_syncstatusbar_init(&statusbars);
    storage_init();
    settings_reset();
    settings_load(SETTINGS_ALL);
    gui_sync_wps_init();
    settings_apply(true);
    init_dircache(true);
    init_dircache(false);
#ifdef HAVE_TAGCACHE
    init_tagcache();
#endif
    sleep(HZ/2);
    tree_mem_init();
    filetype_init();
    playlist_init();

#if CONFIG_CODEC != SWCODEC
    mp3_init( global_settings.volume,
              global_settings.bass,
              global_settings.treble,
              global_settings.balance,
              global_settings.loudness,
              global_settings.avc,
              global_settings.channel_config,
              global_settings.stereo_width,
              global_settings.mdb_strength,
              global_settings.mdb_harmonics,
              global_settings.mdb_center,
              global_settings.mdb_shape,
              global_settings.mdb_enable,
              global_settings.superbass);

    /* audio_init must to know the size of voice buffer so init voice first */
    talk_init();
#endif /* CONFIG_CODEC != SWCODEC */

    scrobbler_init();
    cuesheet_init();

    audio_init();
    button_clear_queue(); /* Empty the keyboard buffer */
}

#else

static void init(void)
{
    int rc;
    bool mounted = false;
#if CONFIG_CHARGING && (CONFIG_CPU == SH7034)
    /* if nobody initialized ATA before, I consider this a cold start */
    bool coldstart = (PACR2 & 0x4000) != 0; /* starting from Flash */
#endif

    system_init();
    kernel_init();

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    set_cpu_frequency(CPUFREQ_NORMAL);
#ifdef CPU_COLDFIRE
    coldfire_set_pllcr_audio_bits(DEFAULT_PLLCR_AUDIO_BITS);
#endif
    cpu_boost(true);
#endif
    
    buffer_init();

    settings_reset();

    i2c_init();
    
    power_init();

    enable_irq();
#ifdef CPU_ARM
    enable_fiq();
#endif
    lcd_init();
#ifdef HAVE_REMOTE_LCD
    lcd_remote_init();
#endif
    font_init();

    show_logo();
    lang_init();

#ifdef DEBUG
    debug_init();
#else
#if !defined(HAVE_FMADC) && !(CONFIG_STORAGE & STORAGE_MMC)
    serial_setup();
#endif
#endif

#if CONFIG_RTC
    rtc_init();
#endif
#ifdef HAVE_RTC_RAM
    settings_load(SETTINGS_RTC); /* early load parts of global_settings */
#endif

    adc_init();

    usb_init();
#if CONFIG_USBOTG == USBOTG_ISP1362
    isp1362_init();
#elif CONFIG_USBOTG == USBOTG_M5636
    m5636_init();
#endif

    backlight_init();

    button_init();

    powermgmt_init();

#if CONFIG_TUNER
    radio_init();
#endif

    /* Must be done before any code uses the multi-screen APi */
    gui_syncstatusbar_init(&statusbars);

#if CONFIG_CHARGING && (CONFIG_CPU == SH7034)
    if (coldstart && charger_inserted()
        && !global_settings.car_adapter_mode
#ifdef ATA_POWER_PLAYERSTYLE
        && !ide_powered() /* relies on probing result from bootloader */
#endif
        )
    {
        rc = charging_screen(); /* display a "charging" screen */
        if (rc == 1)            /* charger removed */
            power_off();
        /* "On" pressed or USB connected: proceed */
        show_logo();  /* again, to provide better visual feedback */
    }
#endif

    rc = storage_init();
    if(rc)
    {
#ifdef HAVE_LCD_BITMAP
        char str[32];
        lcd_clear_display();
        snprintf(str, 31, "ATA error: %d", rc);
        lcd_puts(0, 1, str);
        lcd_puts(0, 3, "Press ON to debug");
        lcd_update();
        while(!(button_get(true) & BUTTON_REL)); /*DO NOT CHANGE TO ACTION SYSTEM */
        dbg_ports();
#endif
        panicf("ata: %d", rc);
    }

#ifdef HAVE_EEPROM_SETTINGS
    eeprom_settings_init();
#endif

#ifndef HAVE_USBSTACK
    usb_start_monitoring();
    while (usb_detect() == USB_INSERTED)
    {
#ifdef HAVE_EEPROM_SETTINGS
        firmware_settings.disk_clean = false;
#endif
        /* enter USB mode early, before trying to mount */
        if (button_get_w_tmo(HZ/10) == SYS_USB_CONNECTED)
#if (CONFIG_STORAGE & STORAGE_MMC)
            if (!mmc_touched() ||
                (mmc_remove_request() == SYS_HOTSWAP_EXTRACTED))
#endif
            {
                usb_screen();
                mounted = true; /* mounting done @ end of USB mode */
            }
#ifdef HAVE_USB_POWER
        if (usb_powered())      /* avoid deadlock */
            break;
#endif
    }
#endif

    if (!mounted)
    {
        rc = disk_mount_all();
        if (rc<=0)
        {
            lcd_clear_display();
            lcd_puts(0, 0, "No partition");
            lcd_puts(0, 1, "found.");
#ifdef HAVE_LCD_BITMAP
            lcd_puts(0, 2, "Insert USB cable");
            lcd_puts(0, 3, "and fix it.");
#endif
            lcd_update();

            while(button_get(true) != SYS_USB_CONNECTED) {};
            usb_screen();
            system_reboot();
        }
    }

#if defined(SETTINGS_RESET) || (CONFIG_KEYPAD == IPOD_4G_PAD) || \
    (CONFIG_KEYPAD == IRIVER_H10_PAD)
#ifdef SETTINGS_RESET
    /* Reset settings if holding the reset button. (Rec on Archos,
       A on Gigabeat) */
    if ((button_status() & SETTINGS_RESET) == SETTINGS_RESET)
#else
    /* Reset settings if the hold button is turned on */
    if (button_hold())
#endif
    {
        splash(HZ*2, str(LANG_RESET_DONE_CLEAR));
        settings_reset();
    }
    else
#endif
        settings_load(SETTINGS_ALL);

    if (init_dircache(true) < 0)
    {
#ifdef HAVE_TAGCACHE
        remove(TAGCACHE_STATEFILE);
#endif
    }

    gui_sync_wps_init();
    settings_apply(true);
    init_dircache(false);
#ifdef HAVE_TAGCACHE
    init_tagcache();
#endif

#ifdef HAVE_EEPROM_SETTINGS
    if (firmware_settings.initialized)
    {
        /* In case we crash. */
        firmware_settings.disk_clean = false;
        eeprom_settings_store();
    }
#endif
    status_init();
    playlist_init();
    tree_mem_init();
    filetype_init();
    scrobbler_init();
    cuesheet_init();

#if CONFIG_CODEC != SWCODEC
    /* No buffer allocation (see buffer.c) may take place after the call to
       audio_init() since the mpeg thread takes the rest of the buffer space */
    mp3_init( global_settings.volume,
              global_settings.bass,
              global_settings.treble,
              global_settings.balance,
              global_settings.loudness,
              global_settings.avc,
              global_settings.channel_config,
              global_settings.stereo_width,
              global_settings.mdb_strength,
              global_settings.mdb_harmonics,
              global_settings.mdb_center,
              global_settings.mdb_shape,
              global_settings.mdb_enable,
              global_settings.superbass);

     /* audio_init must to know the size of voice buffer so init voice first */
    talk_init();
#endif /* CONFIG_CODEC != SWCODEC */

    audio_init();

#if (CONFIG_CODEC == SWCODEC) && defined(HAVE_RECORDING) && !defined(SIMULATOR)
    pcm_rec_init();
#endif

    /* runtime database has to be initialized after audio_init() */
    cpu_boost(false);

#ifdef AUTOROCK
    {
        static const char filename[] = PLUGIN_APPS_DIR "/autostart.rock"; 

        if(file_exists(filename)) /* no complaint if it doesn't exist */
        {
            plugin_load((char*)filename, NULL); /* start if it does */
        }
    }
#endif /* #ifdef AUTOROCK */

#if CONFIG_CHARGING
    car_adapter_mode_init();
#endif
#ifdef IPOD_ACCESSORY_PROTOCOL
    iap_setup(global_settings.serial_bitrate);
#endif
#ifdef HAVE_ACCESSORY_SUPPLY
    accessory_supply_set(global_settings.accessory_supply);
#endif
}

#ifdef CPU_PP
void cop_main(void)
{
/* This is the entry point for the coprocessor
   Anyone not running an upgraded bootloader will never reach this point,
   so it should not be assumed that the coprocessor be usable even on
   platforms which support it.

   A kernel thread is initially setup on the coprocessor and immediately
   destroyed for purposes of continuity. The cop sits idle until at least
   one thread exists on it. */

/* 3G doesn't have Rolo or dual core support yet */
#if NUM_CORES > 1
    system_init();
    kernel_init();
    /* This should never be reached */
#endif
    while(1) {
        sleep_core(COP);
    }
}
#endif /* CPU_PP */

int main(void)
{
    app_main();

    while(1) {
#if (CONFIG_LED == LED_REAL)
        led(true); sleep(HZ/10);
        led(false); sleep(HZ/10);
#endif
    }
    return 0;
}
#endif

