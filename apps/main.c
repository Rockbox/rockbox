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
#include "system.h"

#include "gcc_extensions.h"
#include "storage.h"
#include "disk.h"
#include "file_internal.h"
#include "lcd.h"
#include "rtc.h"
#include "debug.h"
#include "led.h"
#include "../kernel-internal.h"
#include "button.h"
#include "tree.h"
#include "filetypes.h"
#include "panic.h"
#include "menu.h"
#include "usb.h"
#include "powermgmt.h"
#if !defined(DX50) && !defined(DX90)
#include "adc.h"
#endif
#include "i2c.h"
#ifndef DEBUG
#include "serial.h"
#endif
#include "audio.h"
#include "mp3_playback.h"
#include "settings.h"
#include "backlight.h"
#include "status.h"
#include "debug_menu.h"
#include "font.h"
#include "language.h"
#include "wps.h"
#include "playlist.h"
#include "core_alloc.h"
#include "rolo.h"
#include "screens.h"
#include "usb_screen.h"
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
#include "viewport.h"
#include "skin_engine/skin_engine.h"
#include "statusbar-skinned.h"
#include "bootchart.h"
#include "logdiskf.h"
#if (CONFIG_PLATFORM & PLATFORM_ANDROID)
#include "notification.h"
#endif
#include "shortcuts.h"

#ifdef IPOD_ACCESSORY_PROTOCOL
#include "iap.h"
#endif

#if (CONFIG_CODEC == SWCODEC)
#include "audio_thread.h"
#include "playback.h"
#include "tdspeed.h"
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

#ifdef HAVE_HARDWARE_CLICK
#include "piezo.h"
#endif

#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
#define MAIN_NORETURN_ATTR NORETURN_ATTR
#else
/* gcc adds an implicit 'return 0;' at the end of main(), causing a warning
 * with noreturn attribute */
#define MAIN_NORETURN_ATTR
#endif

#if (CONFIG_PLATFORM & (PLATFORM_SDL|PLATFORM_MAEMO|PLATFORM_PANDORA))
#ifdef SIMULATOR
#include "sim_tasks.h"
#endif
#include "system-sdl.h"
#define HAVE_ARGV_MAIN
/* Don't use SDL_main on windows -> no more stdio redirection */
#if defined(WIN32)
#undef main
#endif
#endif

/*#define AUTOROCK*/ /* define this to check for "autostart.rock" on boot */

static void init(void);
/* main(), and various functions called by main() and init() may be
 * be INIT_ATTR. These functions must not be called after the final call
 * to root_menu() at the end of main()
 * see definition of INIT_ATTR in config.h */
#ifdef HAVE_ARGV_MAIN
int main(int argc, char *argv[]) INIT_ATTR MAIN_NORETURN_ATTR ;
int main(int argc, char *argv[])
{
    sys_handle_argv(argc, argv);
#else
int main(void) INIT_ATTR MAIN_NORETURN_ATTR;
int main(void)
{
#endif
    CHART(">init");
    init();
    CHART("<init");
    FOR_NB_SCREENS(i)
    {
        screens[i].clear_display();
        screens[i].update();
    }
#ifdef HAVE_LCD_BITMAP
    list_init();
#endif
    tree_gui_init();
    /* Keep the order of this 3
     * Must be done before any code uses the multi-screen API */
#ifdef HAVE_USBSTACK
    /* All threads should be created and public queues registered by now */
    usb_start_monitoring();
#endif

#ifdef AUTOROCK
    {
        char filename[MAX_PATH];
        const char *file =
#ifdef APPLICATION
                                ROCKBOX_DIR
#else
                                PLUGIN_APPS_DIR
#endif
                                    "/autostart.rock";
        if(file_exists(file)) /* no complaint if it doesn't exist */
        {
            plugin_load(file, NULL); /* start if it does */
        }
    }
#endif /* #ifdef AUTOROCK */

    global_status.last_volume_change = 0;
    /* no calls INIT_ATTR functions after this point anymore!
     * see definition of INIT_ATTR in config.h */
    CHART(">root_menu");
    root_menu();
}

#ifdef HAVE_DIRCACHE
static int INIT_ATTR init_dircache(bool preinit)
{
    if (preinit)
        dircache_init(MAX(global_status.dircache_size, 0));

    if (!global_settings.dircache)
        return -1;

    int result = -1;

#ifdef HAVE_EEPROM_SETTINGS
    if (firmware_settings.initialized &&
        firmware_settings.disk_clean &&
        preinit)
    {
        result = dircache_load();
        if (result < 0)
            firmware_settings.disk_clean = false;
    }
    else
#endif /* HAVE_EEPROM_SETTINGS */
    if (!preinit)
    {
        result = dircache_enable();
        if (result != 0)
        {
            if (result > 0)
            {
                /* Print "Scanning disk..." to the display. */
                splash(0, str(LANG_SCANNING_DISK));
                dircache_wait();
                backlight_on();
                show_logo();
            }

            struct dircache_info info;
            dircache_get_info(&info);
            global_status.dircache_size = info.size;
            status_save();
        }
        /* else don't wait or already enabled by load */
    }

    return result;
}
#endif /* HAVE_DIRCACHE */

#ifdef HAVE_TAGCACHE
static void init_tagcache(void) INIT_ATTR;
static void init_tagcache(void)
{
    bool clear = false;
#if 0 /* CONFIG_CODEC == SWCODEC */
    long talked_tick = 0;
#endif
    tagcache_init();

    while (!tagcache_is_initialized())
    {
        int ret = tagcache_get_commit_step();

        if (ret > 0)
        {
#if 0 /* FIXME: Audio isn't even initialized yet! */ /* CONFIG_CODEC == SWCODEC */
            /* hwcodec can't use voice here, as the database commit
             * uses the audio buffer. */
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
            if (lang_is_rtl())
            {
                splashf(0, "[%d/%d] %s", ret, tagcache_get_max_commit_step(),
                    str(LANG_TAGCACHE_INIT));
            }
            else
            {
                splashf(0, "%s [%d/%d]", str(LANG_TAGCACHE_INIT), ret,
                    tagcache_get_max_commit_step());
            }
#else
            lcd_double_height(false);
            lcd_putsf(0, 1, " DB [%d/%d]", ret,
                tagcache_get_max_commit_step());
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

#if (CONFIG_PLATFORM & PLATFORM_HOSTED)

static void init(void)
{
    system_init();
    core_allocator_init();
    kernel_init();
#ifdef APPLICATION
    paths_init();
#endif
    enable_irq();
    lcd_init();
#ifdef HAVE_REMOTE_LCD
    lcd_remote_init();
#endif
#ifdef HAVE_LCD_BITMAP
    FOR_NB_SCREENS(i)
        global_status.font_id[i] = FONT_SYSFIXED;
    font_init();
#endif
    show_logo();
#ifndef USB_NONE
    usb_init();
    usb_start_monitoring();
#endif
    button_init();
    powermgmt_init();
    backlight_init();
    unicode_init();
#ifdef SIMULATOR
    sim_tasks_init();
#endif
#if (CONFIG_PLATFORM & PLATFORM_ANDROID)
    notification_init();
#endif
    lang_init(core_language_builtin, language_strings,
              LANG_LAST_INDEX_IN_ARRAY);
#ifdef DEBUG
    debug_init();
#endif
#if CONFIG_TUNER
    radio_init();
#endif
    /* Keep the order of this 3 (viewportmanager handles statusbars)
     * Must be done before any code uses the multi-screen API */
    gui_syncstatusbar_init(&statusbars);
    gui_sync_skin_init();
    sb_skin_init();
    viewportmanager_init();

    storage_init();
#if CONFIG_CODEC == SWCODEC
    pcm_init();
    dsp_init();
#endif
    settings_reset();
    settings_load(SETTINGS_ALL);
    settings_apply(true);
#ifdef HAVE_DIRCACHE
    init_dircache(true);
    init_dircache(false);
#endif
#ifdef HAVE_TAGCACHE
    init_tagcache();
#endif
    tree_mem_init();
    filetype_init();
    playlist_init();
    shortcuts_init();

#if CONFIG_CODEC != SWCODEC
    mp3_init( global_settings.volume,
              global_settings.bass,
              global_settings.treble,
              global_settings.balance,
              global_settings.loudness,
              global_settings.avc,
              global_settings.channel_config,
              global_settings.stereo_width,
              global_settings.swap_channels,
              global_settings.mdb_strength,
              global_settings.mdb_harmonics,
              global_settings.mdb_center,
              global_settings.mdb_shape,
              global_settings.mdb_enable,
              global_settings.superbass);
#endif /* CONFIG_CODEC != SWCODEC */

    if (global_settings.audioscrobbler)
        scrobbler_init();

    audio_init();

    settings_apply_skins();
}

#else

#include "errno.h"

static void init(void) INIT_ATTR;
static void init(void)
{
    int rc;
    bool mounted = false;
#if CONFIG_CHARGING && (CONFIG_CPU == SH7034)
    /* if nobody initialized ATA before, I consider this a cold start */
    bool coldstart = (PACR2 & 0x4000) != 0; /* starting from Flash */
#endif

    system_init();
    core_allocator_init();
    kernel_init();

    /* early early early! */
    filesystem_init();

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    set_cpu_frequency(CPUFREQ_NORMAL);
#ifdef CPU_COLDFIRE
    coldfire_set_pllcr_audio_bits(DEFAULT_PLLCR_AUDIO_BITS);
#endif
    cpu_boost(true);
#endif

    i2c_init();

    power_init();

    enable_irq();
#ifdef CPU_ARM
    enable_fiq();
#endif
    /* current_tick should be ticking by now */
    CHART("ticking");

    unicode_init();
    lcd_init();
#ifdef HAVE_REMOTE_LCD
    lcd_remote_init();
#endif
#ifdef HAVE_LCD_BITMAP
    FOR_NB_SCREENS(i)
        global_status.font_id[i] = FONT_SYSFIXED;
    font_init();
#endif

    settings_reset();

    CHART(">show_logo");
    show_logo();
    CHART("<show_logo");
    lang_init(core_language_builtin, language_strings,
              LANG_LAST_INDEX_IN_ARRAY);

#ifdef DEBUG
    debug_init();
#else
#ifdef HAVE_SERIAL
    serial_setup();
#endif
#endif

#if CONFIG_RTC
    rtc_init();
#endif
#ifdef HAVE_RTC_RAM
    CHART(">settings_load(RTC)");
    settings_load(SETTINGS_RTC); /* early load parts of global_settings */
    CHART("<settings_load(RTC)");
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

    /* Don't initialize power management here if it could incorrectly
     * measure battery voltage, and it's not needed for charging. */
#if !defined(NEED_ATA_POWER_BATT_MEASURE) || \
    (CONFIG_CHARGING > CHARGING_MONITOR)
    powermgmt_init();
#endif

#if CONFIG_TUNER
    radio_init();
#endif

#ifdef HAVE_HARDWARE_CLICK
    piezo_init();
#endif

    /* Keep the order of this 3 (viewportmanager handles statusbars)
     * Must be done before any code uses the multi-screen API */
    CHART(">gui_syncstatusbar_init");
    gui_syncstatusbar_init(&statusbars);
    CHART("<gui_syncstatusbar_init");
    CHART(">sb_skin_init");
    sb_skin_init();
    CHART("<sb_skin_init");
    CHART(">gui_sync_wps_init");
    gui_sync_skin_init();
    CHART("<gui_sync_wps_init");
    CHART(">viewportmanager_init");
    viewportmanager_init();
    CHART("<viewportmanager_init");

#if CONFIG_CHARGING && (CONFIG_CPU == SH7034)
    /* charger_inserted() can't be used here because power_thread()
       hasn't checked power_input_status() yet */
    if (coldstart && (power_input_status() & POWER_INPUT_MAIN_CHARGER)
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

    CHART(">storage_init");
    rc = storage_init();
    CHART("<storage_init");
    if(rc)
    {
#ifdef HAVE_LCD_BITMAP
        lcd_clear_display();
        lcd_putsf(0, 1, "ATA error: %d", rc);
        lcd_puts(0, 3, "Press ON to debug");
        lcd_update();
        while(!(button_get(true) & BUTTON_REL)); /* DO NOT CHANGE TO ACTION SYSTEM */
        dbg_ports();
#endif
        panicf("ata: %d", rc);
    }

#if defined(NEED_ATA_POWER_BATT_MEASURE) && \
    (CONFIG_CHARGING <= CHARGING_MONITOR)
    /* After storage_init(), ATA power must be on, so battery voltage
     * can be measured. Initialize power management if it was delayed. */
    powermgmt_init();
#endif
#ifdef HAVE_EEPROM_SETTINGS
    CHART(">eeprom_settings_init");
    eeprom_settings_init();
    CHART("<eeprom_settings_init");
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
                gui_usb_screen_run(true);
                mounted = true; /* mounting done @ end of USB mode */
            }
#ifdef HAVE_USB_POWER
        /* if there is no host or user requested no USB, skip this */
        if (usb_powered_only())
            break;
#endif
    }
#endif

    if (!mounted)
    {
        CHART(">disk_mount_all");
        rc = disk_mount_all();
        CHART("<disk_mount_all");
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
            gui_usb_screen_run(true);
            system_reboot();
        }
    }

#if CONFIG_CODEC == SWCODEC
    pcm_init();
    dsp_init();
#endif

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
    {
        CHART(">settings_load(ALL)");
        settings_load(SETTINGS_ALL);
        CHART("<settings_load(ALL)");
    }

#ifdef HAVE_DIRCACHE
    CHART(">init_dircache(true)");
    rc = init_dircache(true);
    CHART("<init_dircache(true)");
#ifdef HAVE_TAGCACHE
    if (rc < 0)
        remove(TAGCACHE_STATEFILE);
#endif /* HAVE_TAGCACHE */
#endif /* HAVE_DIRCACHE */

    CHART(">settings_apply(true)");
    settings_apply(true);
    CHART("<settings_apply(true)");
#ifdef HAVE_DIRCACHE
    CHART(">init_dircache(false)");
    init_dircache(false);
    CHART("<init_dircache(false)");
#endif
#ifdef HAVE_TAGCACHE
    CHART(">init_tagcache");
    init_tagcache();
    CHART("<init_tagcache");
#endif

#ifdef HAVE_EEPROM_SETTINGS
    if (firmware_settings.initialized)
    {
        /* In case we crash. */
        firmware_settings.disk_clean = false;
        CHART(">eeprom_settings_store");
        eeprom_settings_store();
        CHART("<eeprom_settings_store");
    }
#endif
    playlist_init();
    tree_mem_init();
    filetype_init();

    if (global_settings.audioscrobbler)
        scrobbler_init();

    shortcuts_init();

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
              global_settings.swap_channels,
              global_settings.mdb_strength,
              global_settings.mdb_harmonics,
              global_settings.mdb_center,
              global_settings.mdb_shape,
              global_settings.mdb_enable,
              global_settings.superbass);
#endif /* CONFIG_CODEC != SWCODEC */

    CHART(">audio_init");
    audio_init();
    CHART("<audio_init");

    /* runtime database has to be initialized after audio_init() */
    cpu_boost(false);

#if CONFIG_CHARGING
    car_adapter_mode_init();
#endif
#ifdef IPOD_ACCESSORY_PROTOCOL
    iap_setup(global_settings.serial_bitrate);
#endif
#ifdef HAVE_ACCESSORY_SUPPLY
    accessory_supply_set(global_settings.accessory_supply);
#endif
#ifdef HAVE_LINEOUT_POWEROFF
    lineout_set(global_settings.lineout_active);
#endif
#ifdef HAVE_HOTSWAP_STORAGE_AS_MAIN
    CHART("<check_bootfile(false)");
    check_bootfile(false); /* remember write time and filesize */
    CHART(">check_bootfile(false)");
#endif
    CHART("<settings_apply_skins");
    settings_apply_skins();
    CHART(">settings_apply_skins");
}

#ifdef CPU_PP
void cop_main(void) MAIN_NORETURN_ATTR;
void cop_main(void)
{
/* This is the entry point for the coprocessor
   Anyone not running an upgraded bootloader will never reach this point,
   so it should not be assumed that the coprocessor be usable even on
   platforms which support it.

   A kernel thread is initially setup on the coprocessor and immediately
   destroyed for purposes of continuity. The cop sits idle until at least
   one thread exists on it. */

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

#endif /* SIMULATOR */
