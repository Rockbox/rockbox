/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Heikki Hannikainen
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
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string-extra.h>
#include "lcd.h"
#include "lang.h"
#include "menu.h"
#include "debug_menu.h"
#include "kernel.h"
#include "action.h"
#include "debug.h"
#include "thread.h"
#include "powermgmt.h"
#include "system.h"
#include "font.h"
#include "audio.h"
#include "settings.h"
#include "list.h"
#include "dir.h"
#include "panic.h"
#include "screens.h"
#include "misc.h"
#include "splash.h"
#include "shortcuts.h"
#include "dircache.h"
#include "viewport.h"
#ifdef HAVE_TAGCACHE
#include "tagcache.h"
#endif
#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif
#include "crc32.h"
#include "logf.h"
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
#include "disk.h"
#include "adc.h"
#include "usb.h"
#include "rtc.h"
#include "storage.h"
#include "fs_defines.h"
#include "eeprom_24cxx.h"
#if (CONFIG_STORAGE & STORAGE_MMC) || (CONFIG_STORAGE & STORAGE_SD)
#include "sdmmc.h"
#endif
#if (CONFIG_STORAGE & STORAGE_ATA)
#include "ata.h"
#endif
#endif /* CONFIG_PLATFORM & PLATFORM_NATIVE */
#include "power.h"

#if ((CONFIG_PLATFORM & PLATFORM_NATIVE) || defined(SAMSUNG_YPR0) || defined(SAMSUNG_YPR1) \
        || defined(SONY_NWZ_LINUX)) \
    && CONFIG_TUNER != 0
#include "tuner.h"
#include "radio.h"
#endif

#include "scrollbar.h"
#include "peakmeter.h"
#include "skin_engine/skin_engine.h"
#include "logfdisp.h"
#include "core_alloc.h"
#include "pcmbuf.h"
#include "buffering.h"
#include "playback.h"
#if defined(HAVE_SPDIF_OUT) || defined(HAVE_SPDIF_IN)
#include "spdif.h"
#endif
#ifdef IRIVER_H300_SERIES
#include "pcf50606.h"   /* for pcf50606_read */
#endif
#ifdef IAUDIO_X5
#include "ds2411.h"
#endif
#include "hwcompat.h"
#include "button.h"
#if CONFIG_RTC == RTC_PCF50605
#include "pcf50605.h"
#endif
#include "appevents.h"

#if defined(HAVE_AS3514) && CONFIG_CHARGING
#include "ascodec.h"
#endif

#ifdef IPOD_NANO2G
#include "pmu-target.h"
#endif

#ifdef SANSA_CONNECT
#include "avr-sansaconnect.h"
#endif

#ifdef HAVE_USBSTACK
#include "usb_core.h"
#endif

#if defined(IPOD_ACCESSORY_PROTOCOL)
#include "iap.h"
#endif

#include "talk.h"

#if defined(HAVE_DEVICEDATA)// && !defined(SIMULATOR)
#include "devicedata.h"
#endif

#if defined(HAVE_BOOTDATA) && !defined(SIMULATOR)
#include "bootdata.h"
#include "multiboot.h"
#include "rbpaths.h"
#include "pathfuncs.h"
#include "rb-loader.h"
#endif

#if defined(IPOD_6G)
#include "nor-target.h"
#endif

#define SCREEN_MAX_CHARS (LCD_WIDTH / SYSFONT_WIDTH)

static const char* threads_getname(int selected_item, void *data,
                                   char *buffer, size_t buffer_len)
{
    int *x_offset = (int*) data;

#if NUM_CORES > 1
    if (selected_item < (int)NUM_CORES)
    {
        struct core_debug_info coreinfo;
        core_get_debug_info(selected_item, &coreinfo);
        snprintf(buffer, buffer_len, "Idle (%d): %2d%%", selected_item,
                 coreinfo.idle_stack_usage);
        return buffer;
    }

    selected_item -= NUM_CORES;
#endif

    const char *fmtstr = "%2d: ---";

    struct thread_debug_info threadinfo;
    if (thread_get_debug_info(selected_item, &threadinfo) > 0)
    {
        fmtstr = "%2d:" IF_COP(" (%d)") " %s%n" IF_PRIO(" %d %d")
                 IFN_SDL(" %2d%%") " %s";
    }
    int status_len;
    size_t len = snprintf(buffer, buffer_len, fmtstr,
             selected_item,
             IF_COP(threadinfo.core,)
             threadinfo.statusstr,
             &status_len,
             IF_PRIO(threadinfo.base_priority, threadinfo.current_priority,)
             IFN_SDL(threadinfo.stack_usage,)
             threadinfo.name);

    int start = 0;
#if LCD_WIDTH <= 128
    if (len >= SCREEN_MAX_CHARS)
    {
        int ch_offset = (*x_offset)%(len-1);
        int rem = SCREEN_MAX_CHARS - (len - ch_offset);
        if (rem > 0)
            ch_offset -= rem;

        if (ch_offset > 0)
        {
            /* don't scroll the # and status */
            status_len++;
            if ((unsigned int)ch_offset + status_len < buffer_len)
                memmove(&buffer[ch_offset], &buffer[0], status_len);
            start = ch_offset;
        }
    }
#else
    (void) x_offset;
    (void) len;
#endif
    return &buffer[start];
}

static int dbg_threads_action_callback(int action, struct gui_synclist *lists)
{

    if (action == ACTION_NONE)
    {
        return ACTION_REDRAW;
    }
#if LCD_WIDTH <= 128
    int *x_offset = ((int*) lists->data);
    if (action == ACTION_STD_OK)
    {
        *x_offset += 1;
        action = ACTION_REDRAW;
    }
    else if (IS_SYSEVENT(action))
    {
        return ACTION_REDRAW;
    }
    else if (action != ACTION_UNKNOWN)
    {
        *x_offset = 0;
    }
#else
    (void) lists;
#endif
    return action;
}
/* Test code!!! */
static bool dbg_os(void)
{
    struct simplelist_info info;
    int xoffset = 0;

    simplelist_info_init(&info, IF_COP("Core and ") "Stack usage:",
                         MAXTHREADS IF_COP( + NUM_CORES ), &xoffset);
    info.scroll_all = false;
    info.action_callback = dbg_threads_action_callback;
    info.get_name = threads_getname;
    return simplelist_show_list(&info);
}

#ifdef __linux__
#include "cpuinfo-linux.h"

#define MAX_STATES 16
static struct time_state states[MAX_STATES];

static const char* get_cpuinfo(int selected_item, void *data,
                                   char *buffer, size_t buffer_len)
{
    (void)data;(void)buffer_len;
    const char* text;
    long time, diff;
    struct cpuusage us;
    static struct cpuusage last_us;
    int state_count = *(int*)data;

    if (cpuusage_linux(&us) != 0)
        return NULL;

    switch(selected_item)
    {
        case 0:
            diff = abs(last_us.usage - us.usage);
            sprintf(buffer, "Usage: %ld.%02ld%% (%c %ld.%02ld)",
                                    us.usage/100, us.usage%100,
                                    (us.usage >= last_us.usage) ? '+':'-',
                                    diff/100, diff%100);
            last_us.usage = us.usage;
            return buffer;
        case 1:
            text = "User";
            time = us.utime;
            diff = us.utime - last_us.utime;
            last_us.utime = us.utime;
            break;
        case 2:
            text = "Sys";
            time = us.stime;
            diff = us.stime - last_us.stime;
            last_us.stime = us.stime;
            break;
        case 3:
            text = "Real";
            time = us.rtime;
            diff = us.rtime - last_us.rtime;
            last_us.rtime = us.rtime;
            break;
        case 4:
            return "*** Per CPU freq stats ***";
        default:
        {
            int cpu = (selected_item - 5) / (state_count + 1);
            int cpu_line = (selected_item - 5) % (state_count + 1);

            /* scaling info */
            int min_freq = min_scaling_frequency(cpu);
            int cur_freq = current_scaling_frequency(cpu);
            /* fallback if scaling frequency is not available */
            if(cur_freq <= 0)
                cur_freq = frequency_linux(cpu);
            int max_freq = max_scaling_frequency(cpu);
            char governor[20];
            bool have_governor = current_scaling_governor(cpu, governor, sizeof(governor));
            if(cpu_line == 0)
            {
                sprintf(buffer,
                        " CPU%d: %s: %d/%d/%d MHz",
                        cpu,
                        have_governor ? governor : "Min/Cur/Max freq",
                        min_freq > 0 ? min_freq/1000 : -1,
                        cur_freq > 0 ? cur_freq/1000 : -1,
                        max_freq > 0 ? max_freq/1000 : -1);
            }
            else
            {
                cpustatetimes_linux(cpu, states, ARRAYLEN(states));
                snprintf(buffer, buffer_len, "   %ld %ld",
                            states[cpu_line-1].frequency,
                            states[cpu_line-1].time);
            }
            return buffer;
        }
    }
    sprintf(buffer, "%s: %ld.%02lds (+ %ld.%02ld)", text,
                    time / us.hz, time % us.hz,
                    diff / us.hz, diff % us.hz);
    return buffer;
}

static int cpuinfo_cb(int action, struct gui_synclist *lists)
{
    (void)lists;
    if (action == ACTION_NONE)
        action = ACTION_REDRAW;
    return action;
}

static bool dbg_cpuinfo(void)
{
    struct simplelist_info info;
    int cpu_count = MAX(cpucount_linux(), 1);
    int state_count = cpustatetimes_linux(0, states, ARRAYLEN(states));
    printf("%s(): %d %d\n", __func__, cpu_count, state_count);
    simplelist_info_init(&info, "CPU info:", 5 + cpu_count*(state_count+1), &state_count);
    info.get_name = get_cpuinfo;
    info.action_callback = cpuinfo_cb;
    info.timeout = HZ;
    info.scroll_all = true;
    return simplelist_show_list(&info);
}

#endif

static unsigned int ticks, freq_sum;
#ifndef CPU_MULTI_FREQUENCY
static unsigned int boost_ticks;
#endif

static void dbg_audio_task(void)
{
#ifdef CPUFREQ_NORMAL
#ifndef CPU_MULTI_FREQUENCY
    if(FREQ > CPUFREQ_NORMAL)
        boost_ticks++;
#endif
    freq_sum += FREQ/1000000; /* in MHz */
#endif
    ticks++;
}

static bool dbg_buffering_thread(void)
{
    int button;
    int line;
    bool done = false;
    size_t bufused;
    size_t bufsize = pcmbuf_get_bufsize();
    int pcmbufdescs = pcmbuf_descs();
    struct buffering_debug d;
    size_t filebuflen = audio_get_filebuflen();
    /* This is a size_t, but call it a long so it puts a - when it's bad. */
#if LCD_WIDTH > 96
    #define STR_DATAREM "data_rem"
    const char * const fmt_used = "%s: %6ld/%ld";
#else /* clipzip, ?*/
    #define STR_DATAREM "remain"
    const char * const fmt_used = "%s:%ld/%ld";
#endif

#ifndef CPU_MULTI_FREQUENCY
    boost_ticks = 0;
#endif
    ticks = freq_sum = 0;

    tick_add_task(dbg_audio_task);

    FOR_NB_SCREENS(i)
        screens[i].setfont(FONT_SYSFIXED);

    while(!done)
    {
        button = get_action(CONTEXT_STD,HZ/5);
        switch(button)
        {
            case ACTION_STD_NEXT:
                audio_next();
                break;
            case ACTION_STD_PREV:
                audio_prev();
                break;
            case ACTION_STD_CANCEL:
                done = true;
                break;
        }

        buffering_get_debugdata(&d);
        bufused = bufsize - pcmbuf_free();

        FOR_NB_SCREENS(i)
        {
            line = 0;
            screens[i].clear_display();


            screens[i].putsf(0, line++, fmt_used, "pcm", (long) bufused, (long) bufsize);

            gui_scrollbar_draw(&screens[i],0, line*8, screens[i].lcdwidth, 6,
                               bufsize, 0, bufused, HORIZONTAL);
            line++;

            screens[i].putsf(0, line++, fmt_used, "alloc", audio_filebufused(),
                            (long) filebuflen);

#if LCD_HEIGHT > 80 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_HEIGHT > 80)
            if (screens[i].lcdheight > 80)
            {
                gui_scrollbar_draw(&screens[i],0, line*8, screens[i].lcdwidth, 6,
                                   filebuflen, 0, audio_filebufused(), HORIZONTAL);
                line++;

                screens[i].putsf(0, line++, fmt_used, "real", (long)d.buffered_data,
                                (long)filebuflen);

                gui_scrollbar_draw(&screens[i],0, line*8, screens[i].lcdwidth, 6,
                                   filebuflen, 0, (long)d.buffered_data, HORIZONTAL);
                line++;
            }
#endif

            screens[i].putsf(0, line++, fmt_used, "usefl", (long)(d.useful_data),
                                                       (long)filebuflen);

#if LCD_HEIGHT > 80 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_HEIGHT > 80)
            if (screens[i].lcdheight > 80)
            {
                gui_scrollbar_draw(&screens[i],0, line*8, screens[i].lcdwidth, 6,
                                   filebuflen, 0, d.useful_data, HORIZONTAL);
                line++;
            }
#endif

            screens[i].putsf(0, line++, "%s: %ld", STR_DATAREM, (long)d.data_rem);

            screens[i].putsf(0, line++, "track count: %2u", audio_track_count());

            screens[i].putsf(0, line++, "handle count: %d", (int)d.num_handles);

#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
            screens[i].putsf(0, line++, "cpu freq: %3dMHz",
                             (int)((FREQ + 500000) / 1000000));
#endif

            if (ticks > 0)
            {
                int avgclock   = freq_sum * 10 / ticks;      /* in 100 kHz */
#ifdef CPU_MULTI_FREQUENCY
                int boostquota = (avgclock * 100 - CPUFREQ_NORMAL/1000) /
                    ((CPUFREQ_MAX - CPUFREQ_NORMAL) / 1000000); /* in 0.1 % */
#else
                int boostquota = boost_ticks * 1000 / ticks; /* in 0.1 % */
#endif
                screens[i].putsf(0, line++, "boost:%3d.%d%% (%d.%dMHz)",
                                 boostquota/10, boostquota%10, avgclock/10, avgclock%10);
            }

            screens[i].putsf(0, line++, "pcmbufdesc: %2d/%2d",
                             pcmbuf_used_descs(), pcmbufdescs);
            screens[i].putsf(0, line++, "watermark: %6d",
                             (int)(d.watermark));

            screens[i].update();
        }
    }

    tick_remove_task(dbg_audio_task);

    FOR_NB_SCREENS(i)
        screens[i].setfont(FONT_UI);

    return false;
#undef STR_DATAREM
}

#ifdef BUFLIB_DEBUG_PRINT
static const char* bf_getname(int selected_item, void *data,
                                   char *buffer, size_t buffer_len)
{
    (void)data;
    core_print_block_at(selected_item, buffer, buffer_len);
    return buffer;
}

static int bf_action_cb(int action, struct gui_synclist* list)
{
    if (action == ACTION_STD_OK)
    {
        if (gui_synclist_get_sel_pos(list) == 0 && core_test_free())
        {
            splash(HZ, "Freed test handle. New alloc should trigger compact");
        }
        else
        {
            splash(HZ/1, "Attempting a 64k allocation");
            int handle = core_alloc(64<<10);
            splash(HZ/2, (handle > 0) ? "Success":"Fail");
            /* for some reason simplelist doesn't allow adding items here if
             * info.get_name is given, so use normal list api */
            gui_synclist_set_nb_items(list, core_get_num_blocks());
            core_free(handle);
        }
        action = ACTION_REDRAW;
    }
    return action;
}

static bool dbg_buflib_allocs(void)
{
    struct simplelist_info info;
    simplelist_info_init(&info, "mem allocs", core_get_num_blocks(), NULL);
    info.get_name = bf_getname;
    info.action_callback = bf_action_cb;
    info.timeout = HZ;
    return simplelist_show_list(&info);
}
#endif /* BUFLIB_DEBUG_PRINT */

#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
static const char* dbg_partitions_getname(int selected_item, void *data,
                                          char *buffer, size_t buffer_len)
{
    (void)data;
    int partition = selected_item/2;

    struct partinfo p;
    if (!disk_partinfo(partition, &p))
        return buffer;

    // XXX fix this up to use logical sector size
    // XXX and if mounted, show free info...
    if (selected_item%2)
    {
        snprintf(buffer, buffer_len, "   T:%x %llu MB", p.type,
                 (uint64_t)(p.size / ( 2048 / ( SECTOR_SIZE / 512 ))));
    }
    else
    {
        snprintf(buffer, buffer_len, "P%d: S:%llx", partition, (uint64_t)p.start);
    }
    return buffer;
}

static bool dbg_partitions(void)
{
    struct simplelist_info info;
    simplelist_info_init(&info, "Partition Info", NUM_DRIVES * MAX_PARTITIONS_PER_DRIVE, NULL);
    info.selection_size = 2;
    info.scroll_all = true;
    info.get_name = dbg_partitions_getname;
    return simplelist_show_list(&info);
}
#endif /* PLATFORM_NATIVE */

#if defined(CPU_COLDFIRE) && defined(HAVE_SPDIF_OUT)
static bool dbg_spdif(void)
{
    int line;
    unsigned int control;
    int x;
    char *s;
    int category;
    int generation;
    unsigned int interruptstat;
    bool valnogood, symbolerr, parityerr;
    bool done = false;
    bool spdif_src_on;
    int spdif_source = spdif_get_output_source(&spdif_src_on);
    spdif_set_output_source(AUDIO_SRC_SPDIF IF_SPDIF_POWER_(, true));

    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);

#ifdef HAVE_SPDIF_POWER
    spdif_power_enable(true); /* We need SPDIF power for both sending & receiving */
#endif

    while (!done)
    {
        line = 0;

        control = EBU1RCVCCHANNEL1;
        interruptstat = INTERRUPTSTAT;
        INTERRUPTCLEAR = 0x03c00000;

        valnogood = (interruptstat & 0x01000000)?true:false;
        symbolerr = (interruptstat & 0x00800000)?true:false;
        parityerr = (interruptstat & 0x00400000)?true:false;

        lcd_putsf(0, line++, "Val: %s Sym: %s Par: %s",
                 valnogood?"--":"OK",
                 symbolerr?"--":"OK",
                 parityerr?"--":"OK");

        lcd_putsf(0, line++, "Status word: %08x", (int)control);

        line++;

        x = control >> 31;
        lcd_putsf(0, line++, "PRO: %d (%s)",
                 x, x?"Professional":"Consumer");

        x = (control >> 30) & 1;
        lcd_putsf(0, line++, "Audio: %d (%s)",
                 x, x?"Non-PCM":"PCM");

        x = (control >> 29) & 1;
        lcd_putsf(0, line++, "Copy: %d (%s)",
                 x, x?"Permitted":"Inhibited");

        x = (control >> 27) & 7;
        switch(x)
        {
        case 0:
            s = "None";
            break;
        case 1:
            s = "50/15us";
            break;
        default:
            s = "Reserved";
            break;
        }
        lcd_putsf(0, line++, "Preemphasis: %d (%s)", x, s);

        x = (control >> 24) & 3;
        lcd_putsf(0, line++, "Mode: %d", x);

        category = (control >> 17) & 127;
        switch(category)
        {
        case 0x00:
            s = "General";
            break;
        case 0x40:
            s = "Audio CD";
            break;
        default:
            s = "Unknown";
        }
        lcd_putsf(0, line++, "Category: 0x%02x (%s)", category, s);

        x = (control >> 16) & 1;
        generation = x;
        if(((category & 0x70) == 0x10) ||
           ((category & 0x70) == 0x40) ||
           ((category & 0x78) == 0x38))
        {
            generation = !generation;
        }
        lcd_putsf(0, line++, "Generation: %d (%s)",
                 x, generation?"Original":"No ind.");

        x = (control >> 12) & 15;
        lcd_putsf(0, line++, "Source: %d", x);


        x = (control >> 8) & 15;
        switch(x)
        {
        case 0:
            s = "Unspecified";
            break;
        case 8:
            s = "A (Left)";
            break;
        case 4:
            s = "B (Right)";
            break;
        default:
            s = "";
            break;
        }
        lcd_putsf(0, line++, "Channel: %d (%s)", x, s);

        x = (control >> 4) & 15;
        switch(x)
        {
        case 0:
            s = "44.1kHz";
            break;
        case 0x4:
            s = "48kHz";
            break;
        case 0xc:
            s = "32kHz";
            break;
        }
        lcd_putsf(0, line++, "Frequency: %d (%s)", x, s);

        x = (control >> 2) & 3;
        lcd_putsf(0, line++, "Clock accuracy: %d", x);
        line++;

#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
        lcd_putsf(0, line++, "Measured freq: %ldHz",
                 spdif_measure_frequency());
#endif

        lcd_update();

        if (action_userabort(HZ/10))
            break;
    }

    spdif_set_output_source(spdif_source IF_SPDIF_POWER_(, spdif_src_on));

#ifdef HAVE_SPDIF_POWER
    spdif_power_enable(global_settings.spdif_enable);
#endif

    lcd_setfont(FONT_UI);
    return false;
}
#endif /* CPU_COLDFIRE */

#if (CONFIG_RTC == RTC_PCF50605) && (CONFIG_PLATFORM & PLATFORM_NATIVE)
static bool dbg_pcf(void)
{
    int line;

    lcd_setfont(FONT_SYSFIXED);
    lcd_clear_display();

    while(1)
    {
        line = 0;

        lcd_putsf(0, line++, "DCDC1:  %02x", pcf50605_read(0x1b));
        lcd_putsf(0, line++, "DCDC2:  %02x", pcf50605_read(0x1c));
        lcd_putsf(0, line++, "DCDC3:  %02x", pcf50605_read(0x1d));
        lcd_putsf(0, line++, "DCDC4:  %02x", pcf50605_read(0x1e));
        lcd_putsf(0, line++, "DCDEC1: %02x", pcf50605_read(0x1f));
        lcd_putsf(0, line++, "DCDEC2: %02x", pcf50605_read(0x20));
        lcd_putsf(0, line++, "DCUDC1: %02x", pcf50605_read(0x21));
        lcd_putsf(0, line++, "DCUDC2: %02x", pcf50605_read(0x22));
        lcd_putsf(0, line++, "IOREGC: %02x", pcf50605_read(0x23));
        lcd_putsf(0, line++, "D1REGC: %02x", pcf50605_read(0x24));
        lcd_putsf(0, line++, "D2REGC: %02x", pcf50605_read(0x25));
        lcd_putsf(0, line++, "D3REGC: %02x", pcf50605_read(0x26));
        lcd_putsf(0, line++, "LPREG1: %02x", pcf50605_read(0x27));
        lcd_update();
        if (action_userabort(HZ/10))
        {
            lcd_setfont(FONT_UI);
            return false;
        }
    }

    lcd_setfont(FONT_UI);
    return false;
}
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
static bool dbg_cpufreq(void)
{
    int line;
    int button;
    int x = 0;
    bool done = false;

    lcd_setfont(FONT_SYSFIXED);
    lcd_clear_display();

    while(!done)
    {
        line = 0;

        int temp = FREQ / 1000;
        lcd_putsf(x, line++, "Frequency: %ld.%ld MHz", temp / 1000, temp % 1000);
        lcd_putsf(x, line++, "boost_counter: %d", get_cpu_boost_counter());

#ifdef HAVE_ADJUSTABLE_CPU_VOLTAGE
        extern int get_cpu_voltage_setting(void);
        temp = get_cpu_voltage_setting();
        lcd_putsf(x, line++, "CPU voltage: %d.%03dV", temp / 1000, temp % 1000);
#endif

        lcd_update();
        button = get_action(CONTEXT_STD,HZ/10);

        switch(button)
        {
            case ACTION_STD_PREV:
                cpu_boost(true);
                break;

            case ACTION_STD_NEXT:
                cpu_boost(false);
                break;
            case ACTION_STD_MENU:
                x--;
                break;
            case ACTION_STD_OK:
                x = 0;
                while (get_cpu_boost_counter() > 0)
                    cpu_boost(false);
                set_cpu_frequency(CPUFREQ_DEFAULT);
                break;

            case ACTION_STD_CANCEL:
                done = true;;
        }
        lcd_clear_display();
    }
    lcd_setfont(FONT_UI);
    return false;
}
#endif /* HAVE_ADJUSTABLE_CPU_FREQ */

#if defined(HAVE_TSC2100) && (CONFIG_PLATFORM & PLATFORM_NATIVE)
#include "tsc2100.h"
static const char* tsc2100_debug_getname(int selected_item, void * data,
                                         char *buffer, size_t buffer_len)
{
    int *page = (int*)data;
    bool reserved = false;
    switch (*page)
    {
        case 0:
            if ((selected_item > 0x0a)  ||
                (selected_item == 0x04) ||
                (selected_item == 0x08))
                reserved = true;
            break;
        case 1:
            if ((selected_item > 0x05) ||
                (selected_item == 0x02))
                reserved = true;
            break;
        case 2:
            if (selected_item > 0x1e)
                reserved = true;
            break;
    }
    if (reserved)
        snprintf(buffer, buffer_len, "%02x: RSVD", selected_item);
    else
        snprintf(buffer, buffer_len, "%02x: %04x", selected_item,
                    tsc2100_readreg(*page, selected_item)&0xffff);
    return buffer;
}
static int tsc2100debug_action_callback(int action, struct gui_synclist *lists)
{
    int *page = (int*)lists->data;
    if (action == ACTION_STD_OK)
    {
        *page = (*page+1)%3;
        snprintf((char*)lists->title, 32, "tsc2100 registers - Page %d", *page);
        return ACTION_REDRAW;
    }
    return action;
}
static bool tsc2100_debug(void)
{
    int page = 0;
    char title[32];
    snprintf(title, 32, "tsc2100 registers - Page %d", page);
    struct simplelist_info info;
    simplelist_info_init(&info, title, 32, &page);
    info.timeout = HZ/100;
    info.get_name = tsc2100_debug_getname;
    info.action_callback= tsc2100debug_action_callback;
    return simplelist_show_list(&info);
}
#endif
#if (CONFIG_BATTERY_MEASURE != 0) && !defined(SIMULATOR)
/*
 * view_battery() shows a automatically scaled graph of the battery voltage
 * over time. Usable for estimating battery life / charging rate.
 * The power_history array is updated in power_thread of powermgmt.c.
 */

#define BAT_LAST_VAL  MIN(LCD_WIDTH, POWER_HISTORY_LEN)
#define BAT_TSPACE    20
#define BAT_YSPACE    (LCD_HEIGHT - BAT_TSPACE)


static bool view_battery(void)
{
    int view = 0;
    int i, x, y, z, y1, y2, grid, graph;
    unsigned short maxv, minv;

    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        lcd_clear_display();
        switch (view) {
            case 0: /* voltage history graph */
                /* Find maximum and minimum voltage for scaling */
                minv = power_history[0];
                maxv = minv + 1;
                for (i = 1; i < BAT_LAST_VAL && power_history[i]; i++) {
                    if (power_history[i] > maxv)
                        maxv = power_history[i];
                    if (power_history[i] < minv)
                        minv = power_history[i];
                }
                /* print header */
#if (CONFIG_BATTERY_MEASURE & VOLTAGE_MEASURE)
                /* adjust grid scale */
                if ((maxv - minv) > 50)
                    grid = 50;
                else
                    grid = 5;

                lcd_putsf(0, 0, "%s %d.%03dV", "Battery", power_history[0] / 1000,
                         power_history[0] % 1000);
                lcd_putsf(0, 1, "%d.%03d-%d.%03dV (%2dmV)",
                          minv / 1000, minv % 1000, maxv / 1000, maxv % 1000,
                          grid);
#elif (CONFIG_BATTERY_MEASURE & PERCENTAGE_MEASURE)
                /* adjust grid scale */
                if ((maxv - minv) > 10)
                    grid = 10;
                else
                    grid = 1;
                lcd_putsf(0, 0, "%s %d%%", "Battery", power_history[0]);
                lcd_putsf(0, 1, "%d%%-%d%% (%d %%)", minv, maxv, grid);
#endif

                i = 1;
                while ((y = (minv - (minv % grid)+i*grid)) < maxv)
                {
                    graph = ((y-minv)*BAT_YSPACE)/(maxv-minv);
                    graph = LCD_HEIGHT-1 - graph;

                    /* draw dotted horizontal grid line */
                    for (x=0; x<LCD_WIDTH;x=x+2)
                        lcd_drawpixel(x,graph);

                    i++;
                }

                x = 0;
                /* draw plot of power history
                 * skip empty entries
                 */
                for (i = BAT_LAST_VAL - 1; i > 0; i--)
                {
                    if (power_history[i] && power_history[i-1])
                    {
                        y1 = (power_history[i] - minv) * BAT_YSPACE /
                            (maxv - minv);
                        y1 = MIN(MAX(LCD_HEIGHT-1 - y1, BAT_TSPACE),
                                 LCD_HEIGHT-1);
                        y2 = (power_history[i-1] - minv) * BAT_YSPACE /
                            (maxv - minv);
                        y2 = MIN(MAX(LCD_HEIGHT-1 - y2, BAT_TSPACE),
                                 LCD_HEIGHT-1);

                        lcd_set_drawmode(DRMODE_SOLID);

                        /* make line thicker */
                        lcd_drawline(((x*LCD_WIDTH)/(BAT_LAST_VAL)),
                                     y1,
                                     (((x+1)*LCD_WIDTH)/(BAT_LAST_VAL)),
                                     y2);
                        lcd_drawline(((x*LCD_WIDTH)/(BAT_LAST_VAL))+1,
                                     y1+1,
                                     (((x+1)*LCD_WIDTH)/(BAT_LAST_VAL))+1,
                                     y2+1);
                        x++;
                    }
                }
                break;

            case 1: /* status: */
#if CONFIG_CHARGING >= CHARGING_MONITOR
                lcd_putsf(0, 0, "Pwr status: %s",
                         charging_state() ? "charging" : "discharging");
#else
                lcd_putsf(0, 0, "Pwr status: %s", "unknown");
#endif
                battery_read_info(&y, &z);
                if (y > 0)
                    lcd_putsf(0, 1, "%s: %d.%03d V (%d %%)", "Battery", y / 1000, y % 1000, z);
                else if (z > 0)
                    lcd_putsf(0, 1, "%s: %d %%", "Battery", z);
#ifdef ADC_EXT_POWER
                y = (adc_read(ADC_EXT_POWER) * EXT_SCALE_FACTOR) / 1000;
                lcd_putsf(0, 2, "%s: %d.%03d V", "External", y / 1000, y % 1000);
#endif
#if CONFIG_CHARGING
#if defined IPOD_NANO || defined IPOD_VIDEO
                int usb_pwr  = (GPIOL_INPUT_VAL & 0x10)?true:false;
                int ext_pwr  = (GPIOL_INPUT_VAL & 0x08)?false:true;
                int dock     = (GPIOA_INPUT_VAL & 0x10)?true:false;
                int charging = (GPIOB_INPUT_VAL & 0x01)?false:true;
                int headphone= (GPIOA_INPUT_VAL & 0x80)?true:false;

                lcd_putsf(0, 3, "USB pwr:   %s",
                            usb_pwr ? "present" : "absent");
                lcd_putsf(0, 4, "EXT pwr:   %s",
                            ext_pwr ? "present" : "absent");
                lcd_putsf(0, 5, "%s:   %s", "Battery",
                    charging ? "charging" : (usb_pwr||ext_pwr) ? "charged" : "discharging");
                lcd_putsf(0, 6, "Dock mode: %s",
                         dock    ? "enabled" : "disabled");
                lcd_putsf(0, 7, "Headphone: %s",
                         headphone ? "connected" : "disconnected");
#ifdef IPOD_VIDEO
                if(probed_ramsize == 64)
                    x = (adc_read(ADC_4066_ISTAT) * 2400) / (1024 * 2);
                else
#endif
                    x = (adc_read(ADC_4066_ISTAT) * 2400) / (1024 * 3);
                lcd_putsf(0, 8, "Ibat: %d mA", x);
                lcd_putsf(0, 9, "Vbat * Ibat: %d mW", x * y / 1000);
#elif defined TOSHIBA_GIGABEAT_S
                int line = 3;
                unsigned int st;

                static const unsigned char * const chrgstate_strings[] =
                {
                    "Disabled",
                    "Error",
                    "Discharging",
                    "Precharge",
                    "Constant Voltage",
                    "Constant Current",
                    "<unknown>",
                };

                lcd_putsf(0, line++, "Charger: %s",
                         charger_inserted() ? "present" : "absent");

                st = power_input_status() &
                     (POWER_INPUT_CHARGER | POWER_INPUT_BATTERY);

                lcd_putsf(0, line++, "%.*s%.*s",
                         !!(st & POWER_INPUT_MAIN_CHARGER)*5, " Main",
                         !!(st & POWER_INPUT_USB_CHARGER)*4, " USB");

                y = ARRAYLEN(chrgstate_strings) - 1;

                switch (charge_state)
                {
                case CHARGE_STATE_DISABLED: y--;
                case CHARGE_STATE_ERROR:    y--;
                case DISCHARGING:           y--;
                case TRICKLE:               y--;
                case TOPOFF:                y--;
                case CHARGING:              y--;
                default:;
                }

                lcd_putsf(0, line++, "State: %s", chrgstate_strings[y]);

                lcd_putsf(0, line++, "%s Switch: %s", "Battery",
                         (st & POWER_INPUT_BATTERY) ? "On" : "Off");

                y = chrgraw_adc_voltage();
                lcd_putsf(0, line++, "CHRGRAW: %d.%03d V",
                         y / 1000, y % 1000);

                y = application_supply_adc_voltage();
                lcd_putsf(0, line++, "BP     : %d.%03d V",
                         y / 1000, y % 1000);

                y = battery_adc_charge_current();
                lcd_putsf(0, line++, "CHRGISN:% d mA", y);

                y = cccv_regulator_dissipation();
                lcd_putsf(0, line++, "P CCCV : %d mW", y);

                y = battery_charge_current();
                lcd_putsf(0, line++, "I Charge:% d mA", y);

                y = battery_adc_temp();

                if (y != INT_MIN) {
                    lcd_putsf(0, line++, "T %s: %d\u00b0C (%d\u00b0F)",
                              "Battery", y, (9*y + 160) / 5);
                } else {
                    /* Conversion disabled */
                    lcd_putsf(0, line++, "T %s: ?", "Battery");
                }
#elif defined(HAVE_AS3514) && CONFIG_CHARGING
                static const char * const chrgstate_strings[] =
                {
                    [CHARGE_STATE_DISABLED - CHARGE_STATE_DISABLED]= "Disabled",
                    [CHARGE_STATE_ERROR - CHARGE_STATE_DISABLED] = "Error",
                    [DISCHARGING - CHARGE_STATE_DISABLED]       = "Discharging",
                    [CHARGING - CHARGE_STATE_DISABLED]          = "Charging",
                };
                const char *str = NULL;

                lcd_putsf(0, 3, "Charger: %s",
                         charger_inserted() ? "present" : "absent");

                y = charge_state - CHARGE_STATE_DISABLED;
                if ((unsigned)y < ARRAYLEN(chrgstate_strings))
                    str = chrgstate_strings[y];

                lcd_putsf(0, 4, "State: %s",
                         str ? str : "<unknown>");

                lcd_putsf(0, 5, "CHARGER: %02X", ascodec_read_charger());
#elif defined(IPOD_NANO2G)
                y = pmu_read_battery_voltage();
                lcd_putsf(17, 1, "RAW: %d.%03d V", y / 1000, y % 1000);
                y = pmu_read_battery_current();
                lcd_putsf(0, 2, "%s current: %d mA", "Battery", y);
                lcd_putsf(0, 3, "PWRCON: %08x %08x", PWRCON, PWRCONEXT);
                lcd_putsf(0, 4, "CLKCON: %08x %03x %03x", CLKCON, CLKCON2, CLKCON3);
                lcd_putsf(0, 5, "PLL: %06x %06x %06x", PLL0PMS, PLL1PMS, PLL2PMS);
                x = pmu_read(0x1b) & 0xf;
                y = pmu_read(0x1a) * 25 + 625;
                lcd_putsf(0, 6, "AUTO: %x / %d mV", x, y);
                x = pmu_read(0x1f) & 0xf;
                y = pmu_read(0x1e) * 25 + 625;
                lcd_putsf(0, 7, "DOWN1: %x / %d mV", x, y);
                x = pmu_read(0x23) & 0xf;
                y = pmu_read(0x22) * 25 + 625;
                lcd_putsf(0, 8, "DOWN2: %x / %d mV", x, y);
                x = pmu_read(0x27) & 0xf;
                y = pmu_read(0x26) * 100 + 900;
                lcd_putsf(0, 9, "MEMLDO: %x / %d mV", x, y);
                for (i = 0; i < 6; i++)
                {
                    x = pmu_read(0x2e + (i << 1)) & 0xf;
                    y = pmu_read(0x2d + (i << 1)) * 100 + 900;
                    lcd_putsf(0, 10 + i, "LDO%d: %x / %d mV", i + 1, x, y);
                }
#elif defined(SANSA_CONNECT)
                lcd_putsf(0, 3, "Charger: %s",
                         charger_inserted() ? "present" : "absent");
                x = (avr_hid_hdq_read_short(HDQ_REG_TEMP) / 4) - 273;
                lcd_putsf(0, 4, "%s temperature: %d C", "Battery", x);
                x = (avr_hid_hdq_read_short(HDQ_REG_AI) * 357) / 200;
                lcd_putsf(0, 5, "%s current: %d.%01d mA", "Battery", x / 10, x % 10);
                x = (avr_hid_hdq_read_short(HDQ_REG_AP) * 292) / 20;
                lcd_putsf(0, 6, "Discharge power: %d.%01d mW", x / 10, x % 10);
                x = (avr_hid_hdq_read_short(HDQ_REG_SAE) * 292) / 2;
                lcd_putsf(0, 7, "Available energy: %d.%01d mWh", x / 10, x % 10);
#else
                lcd_putsf(0, 3, "Charger: %s",
                         charger_inserted() ? "present" : "absent");
#endif /* target type */
#endif /* CONFIG_CHARGING */
                break;
            case 2: /* voltage deltas: */
#if (CONFIG_BATTERY_MEASURE & VOLTAGE_MEASURE)
                lcd_puts(0, 0, "Voltage deltas:");
                for (i = 0; i < POWER_HISTORY_LEN-1; i++) {
                    y = power_history[i] - power_history[i+1];
                    lcd_putsf(0, i+1, "-%d min: %c%d.%03d V", i,
                             (y < 0) ? '-' : ' ', ((y < 0) ? y * -1 : y) / 1000,
                             ((y < 0) ? y * -1 : y ) % 1000);
                }
#elif (CONFIG_BATTERY_MEASURE & PERCENTAGE_MEASURE)
                lcd_puts(0, 0, "Percentage deltas:");
                for (i = 0; i < POWER_HISTORY_LEN-1; i++) {
                    y = power_history[i] - power_history[i+1];
                    lcd_putsf(0, i+1, "-%d min: %c%d%%", i,
                             (y < 0) ? '-' : ' ', ((y < 0) ? y * -1 : y));
                }
#endif
                break;

            case 3: /* remaining time estimation: */

#if (CONFIG_BATTERY_MEASURE & VOLTAGE_MEASURE)
                lcd_putsf(0, 5, "Last PwrHist: %d.%03dV",
                    power_history[0] / 1000,
                    power_history[0] % 1000);
#endif

                lcd_putsf(0, 6, "%s level: %d%%", "Battery", battery_level());

                int time_left = battery_time();
                if (time_left >= 0)
                    lcd_putsf(0, 7, "Est. remain: %d m", time_left);
                else
                    lcd_puts(0, 7, "Estimation n/a");

#if (CONFIG_BATTERY_MEASURE & CURRENT_MEASURE)
                lcd_putsf(0, 8, "%s current: %d mA", "Battery", battery_current());
#endif
                break;
        }

        lcd_update();

        switch(get_action(CONTEXT_STD,HZ/2))
        {
            case ACTION_STD_PREV:
                if (view)
                    view--;
                break;

            case ACTION_STD_NEXT:
                if (view < 3)
                    view++;
                break;

            case ACTION_STD_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }
    }
    lcd_setfont(FONT_UI);
    return false;
}

#endif /* (CONFIG_BATTERY_MEASURE != 0)  */

#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
#if (CONFIG_STORAGE & STORAGE_MMC) || (CONFIG_STORAGE & STORAGE_SD)

#if (CONFIG_STORAGE & STORAGE_MMC)
#define CARDTYPE "MMC"
#elif (CONFIG_STORAGE & STORAGE_SD)
#define CARDTYPE "microSD"
#endif

static int disk_callback(int btn, struct gui_synclist *lists)
{
    tCardInfo *card;
    int *cardnum = (int*)lists->data;
    unsigned char card_name[6];
    unsigned char pbuf[32];
    /* Casting away const is safe; the buffer is defined as non-const. */
    char *title = (char *)lists->title;
    static const unsigned char i_vmin[] = { 0, 1, 5, 10, 25, 35, 60, 100 };
    static const unsigned char i_vmax[] = { 1, 5, 10, 25, 35, 45, 80, 200 };
    static const unsigned char * const kbit_units[] = { "kBit/s", "MBit/s", "GBit/s" };
    static const unsigned char * const nsec_units[] = { "ns", "µs", "ms" };
#if (CONFIG_STORAGE & STORAGE_MMC)
    static const char * const mmc_spec_vers[] = { "1.0-1.2", "1.4", "2.0-2.2",
        "3.1-3.31", "4.0" };
#endif

    if ((btn == ACTION_STD_OK) || (btn == SYS_FS_CHANGED) || (btn == ACTION_REDRAW))
    {
#ifdef HAVE_HOTSWAP
        if (btn == ACTION_STD_OK)
        {
            *cardnum ^= 0x1; /* change cards */
        }
#endif

        simplelist_set_line_count(0);

        card = card_get_info(*cardnum);

        if (card->initialized > 0)
        {
            unsigned i;
            for (i=0; i<sizeof(card_name); i++)
            {
                card_name[i] = card_extract_bits(card->cid, (103-8*i), 8);
            }
            strmemccpy(card_name, card_name, sizeof(card_name));
            simplelist_addline(
                    "%s Rev %d.%d", card_name,
                    (int) card_extract_bits(card->cid, 63, 4),
                    (int) card_extract_bits(card->cid, 59, 4));
            simplelist_addline(
                    "Prod: %d/%d",
#if (CONFIG_STORAGE & STORAGE_SD)
                    (int) card_extract_bits(card->cid, 11, 4),
                    (int) card_extract_bits(card->cid, 19, 8) + 2000
#elif (CONFIG_STORAGE & STORAGE_MMC)
                    (int) card_extract_bits(card->cid, 15, 4),
                    (int) card_extract_bits(card->cid, 11, 4) + 1997
#endif
                    );
            simplelist_addline(
#if (CONFIG_STORAGE & STORAGE_SD)
                    "Ser#: 0x%08lx",
                    card_extract_bits(card->cid, 55, 32)
#elif (CONFIG_STORAGE & STORAGE_MMC)
                    "Ser#: 0x%04lx",
                    card_extract_bits(card->cid, 47, 16)
#endif
                    );

            simplelist_addline("M=%02x, "
#if (CONFIG_STORAGE & STORAGE_SD)
                    "O=%c%c",
                    (int) card_extract_bits(card->cid, 127, 8),
                    (int) card_extract_bits(card->cid, 119, 8),
                    (int) card_extract_bits(card->cid, 111, 8)
#elif (CONFIG_STORAGE & STORAGE_MMC)
                    "O=%04x",
                    (int) card_extract_bits(card->cid, 127, 8),
                    (int) card_extract_bits(card->cid, 119, 16)
#endif
                    );

#if (CONFIG_STORAGE & STORAGE_MMC)
            int temp = card_extract_bits(card->csd, 125, 4);
            simplelist_addline(
                     "MMC v%s", temp < 5 ?
                            mmc_spec_vers[temp] : "?.?");
#endif
            simplelist_addline(
                    "Blocks: 0x%08lx", card->numblocks);
            output_dyn_value(pbuf, sizeof pbuf, card->speed / 1000,
                                            kbit_units, 3, false);
            simplelist_addline(
                    "Speed: %s", pbuf);
            output_dyn_value(pbuf, sizeof pbuf, card->taac,
                            nsec_units, 3, false);
            simplelist_addline(
                    "Taac: %s", pbuf);
            simplelist_addline(
                    "Nsac: %d clk", card->nsac);
            simplelist_addline(
                    "R2W: *%d", card->r2w_factor);
#if (CONFIG_STORAGE & STORAGE_SD)
            int csd_structure = card_extract_bits(card->csd, 127, 2);
            const char *ver;
            switch(csd_structure) {
            case 0:
                    ver = "1 (SD)";
                    break;
            case 1:
                    ver = "2 (SDHC/SDXC)";
                    break;
            case 2:
                    ver = "3 (SDUC)";
                    break;
            default:
                    ver = "Unknown";
                    break;
            }
            simplelist_addline("SDVer: %s", ver);
            if (csd_structure == 0) /* CSD version 1.0 */
#endif
            {
            simplelist_addline(
                    "IRmax: %d..%d mA",
                    i_vmin[card_extract_bits(card->csd, 61, 3)],
                    i_vmax[card_extract_bits(card->csd, 58, 3)]);
            simplelist_addline(
                    "IWmax: %d..%d mA",
                    i_vmin[card_extract_bits(card->csd, 55, 3)],
                    i_vmax[card_extract_bits(card->csd, 52, 3)]);
            }
        }
        else if (card->initialized == 0)
        {
            simplelist_addline("Not Found!");
        }
#if (CONFIG_STORAGE & STORAGE_SD)
        else /* card->initialized < 0 */
        {
            simplelist_addline("Init Error! (%d)", card->initialized);
        }
#endif
        snprintf(title, 16, "[" CARDTYPE " %d]", *cardnum);
        gui_synclist_set_title(lists, title, Icon_NOICON);
        gui_synclist_set_nb_items(lists, simplelist_get_line_count());
        gui_synclist_select_item(lists, 0);
        btn = ACTION_REDRAW;
    }
    return btn;
}
#elif  (CONFIG_STORAGE & STORAGE_ATA)
static int disk_callback(int btn, struct gui_synclist *lists)
{
    static const char atanums[] = { " 0 1 2 3 4 5 6" };

    (void)lists;
    int i;
    char buf[128];
    unsigned short* identify_info = ata_get_identify();
    bool timing_info_present = false;
    (void)btn;

    simplelist_set_line_count(0);

    for (i=0; i < 20; i++)
        ((unsigned short*)buf)[i]=htobe16(identify_info[i+27]);
    buf[40]=0;
    /* kill trailing space */
    for (i=39; i && buf[i]==' '; i--)
        buf[i] = 0;
    simplelist_addline("Model: %s", buf);
    for (i=0; i < 10; i++)
        ((unsigned short*)buf)[i]=htobe16(identify_info[i+10]);
    buf[20]=0;
    /* kill trailing space */
    for (i=19; i && buf[i]==' '; i--)
        buf[i] = 0;
    simplelist_addline("Serial number: %s", buf);
    for (i=0; i < 4; i++)
        ((unsigned short*)buf)[i]=htobe16(identify_info[i+23]);
    buf[8]=0;
    simplelist_addline(
             "Firmware: %s", buf);

    uint64_t total_sectors = (identify_info[61] << 16) | identify_info[60];
#ifdef HAVE_LBA48
    if (identify_info[83] & 0x0400 && total_sectors == 0x0FFFFFFF)
        total_sectors = ((uint64_t)identify_info[103] << 48) |
                ((uint64_t)identify_info[102] << 32) |
                ((uint64_t)identify_info[101] << 16) |
                identify_info[100];
#endif

    uint32_t sector_size;

    /* Logical sector size > 512B ? */
    if ((identify_info[106] & 0xd000) == 0x5000) /* B14, B12 */
        sector_size = (identify_info[117] | (identify_info[118] << 16)) * 2;
    else
        sector_size = 512;

    total_sectors *= sector_size;   /* Convert to bytes */
    total_sectors /= (1024 * 1024); /* Convert to MB */

    simplelist_addline("Size: %llu MB", (uint64_t)total_sectors);
    simplelist_addline("Logical sector size: %lu B", sector_size);
#ifdef MAX_LOG_SECTOR_SIZE
    simplelist_addline("Sector multiplier: %u", disk_get_sector_multiplier());
#endif

    if((identify_info[106] & 0xe000) == 0x6000) /* B14, B13 */
        sector_size *= BIT_N(identify_info[106] & 0x000f);
    simplelist_addline(
            "Physical sector size: %lu B", sector_size);

#ifndef HAVE_MULTIVOLUME
    // XXX this needs to be fixed for multi-volume setups
    sector_t free;
    volume_size( IF_MV(0,) NULL, &free );
    simplelist_addline(
            "Free: %llu MB", (uint64_t)(free / 1024));
#endif

    simplelist_addline("SSD detected: %s", ata_disk_isssd() ? "yes" : "no");
    simplelist_addline(
             "Spinup time: %d ms", storage_spinup_time() * (1000/HZ));
    i = identify_info[82] & (1<<3);
    simplelist_addline(
             "Power mgmt: %s", i ? "enabled" : "unsupported");
    i = identify_info[83] & (1<<3);
    simplelist_addline(
             "Adv Power mgmt: %s", i ? "enabled" : "unsupported");
    i = identify_info[83] & (1<<9);
    simplelist_addline(
             "Noise mgmt: %s", i ? "enabled" : "unsupported");
    simplelist_addline(
             "Flush cache: %s", identify_info[83] & (1<<13) ? "extended" : identify_info[83] & (1<<12) ? "standard" : identify_info[80] >= (1<<5) ? "ATA-5" : "unsupported");
    i = identify_info[82] & (1<<6);
    simplelist_addline(
             "Read-ahead: %s", i ? "enabled" : "unsupported");
    timing_info_present = identify_info[53] & (1<<1);
    if(timing_info_present) {
        simplelist_addline(
                 "PIO modes: 0 1 2%.*s%.*s",
                 (identify_info[64] & (1<<0)) << 1, &atanums[3*2],
                 (identify_info[64] & (1<<1))     , &atanums[4*2]);
    }
    else {
        simplelist_addline(
                 "No PIO mode info");
    }
    timing_info_present = identify_info[53] & (1<<1);
    if(timing_info_present) {
        simplelist_addline(
                 "Cycle times %dns/%dns",
                 identify_info[67],
                 identify_info[68] );
    } else {
        simplelist_addline(
                 "No timing info");
    }

#ifdef HAVE_ATA_DMA
    if (identify_info[63] & (1<<0)) {
        simplelist_addline(
                 "MDMA modes:%.*s%.*s%.*s",
                  (identify_info[63] & (1<<0)) << 1, &atanums[0*2],
                  (identify_info[63] & (1<<1))     , &atanums[1*2],
                  (identify_info[63] & (1<<2)) >> 1, &atanums[2*2]);
        simplelist_addline(
                 "MDMA Cycle times %dns/%dns",
                 identify_info[65],
                 identify_info[66] );
    }
    else {
        simplelist_addline(
                "No MDMA mode info");
    }
    if (identify_info[53] & (1<<2)) {
        simplelist_addline(
                 "UDMA modes:%.*s%.*s%.*s%.*s%.*s%.*s%.*s",
                 (identify_info[88] & (1<<0)) << 1, &atanums[0*2],
                 (identify_info[88] & (1<<1))     , &atanums[1*2],
                 (identify_info[88] & (1<<2)) >> 1, &atanums[2*2],
                 (identify_info[88] & (1<<3)) >> 2, &atanums[3*2],
                 (identify_info[88] & (1<<4)) >> 3, &atanums[4*2],
                 (identify_info[88] & (1<<5)) >> 4, &atanums[5*2],
                 (identify_info[88] & (1<<6)) >> 5, &atanums[6*2]);
    }
    else {
        simplelist_addline(
                "No UDMA mode info");
    }
#endif /* HAVE_ATA_DMA */
    timing_info_present = identify_info[53] & (1<<1);
    if(timing_info_present) {
        i = identify_info[49] & (1<<11);
        simplelist_addline(
            "IORDY support: %s", i ? "yes" : "no");
        i = identify_info[49] & (1<<10);
        simplelist_addline(
                "IORDY disable: %s", i ? "yes" : "no");
    } else {
        simplelist_addline(
                "No timing info");
    }
    simplelist_addline(
             "Cluster size: %d bytes", volume_get_cluster_size(IF_MV(0)));
#ifdef HAVE_ATA_DMA
    i = ata_get_dma_mode();
    if (i == 0) {
        simplelist_addline(
                 "DMA not enabled");
    } else if (i == 0xff) {
        simplelist_addline(
                 "CE-ATA mode");
    } else {
        simplelist_addline(
                 "DMA mode: %s %c",
                (i & 0x40) ? "UDMA" : "MDMA",
                '0' + (i & 7));
    }
#endif /* HAVE_ATA_DMA */
    i = identify_info[83] & (1 << 2);
    simplelist_addline(
            "CFA compatible: %s", i ? "yes" : "no");
    i = identify_info[0] & (1 << 6);
    simplelist_addline(
            "Fixed device: %s", i ? "yes" : "no");
    i = identify_info[0] & (1 << 7);
    simplelist_addline(
            "Removeable media: %s", i ? "yes" : "no");

    return btn;
}

#ifdef HAVE_ATA_SMART
static struct ata_smart_values smart_data STORAGE_ALIGN_ATTR;

static const char * ata_smart_get_attr_name(unsigned char id)
{
    if (id == 1)    return "Raw Read Error Rate";
    if (id == 2)    return "Throughput Performance";
    if (id == 3)    return "Spin-Up Time";
    if (id == 4)    return "Start/Stop Count";
    if (id == 5)    return "Reallocated Sector Count";
    if (id == 7)    return "Seek Error Rate";
    if (id == 8)    return "Seek Time Performance";
    if (id == 9)    return "Power-On Hours Count";
    if (id == 10)   return "Spin-Up Retry Count";
    if (id == 12)   return "Power Cycle Count";
    if (id == 191)  return "G-Sense Error Rate";
    if (id == 192)  return "Power-Off Retract Count";
    if (id == 193)  return "Load/Unload Cycle Count";
    if (id == 194)  return "HDA Temperature";
    if (id == 195)  return "Hardware ECC Recovered";
    if (id == 196)  return "Reallocated Event Count";
    if (id == 197)  return "Current Pending Sector Count";
    if (id == 198)  return "Uncorrectable Sector Count";
    if (id == 199)  return "UDMA CRC Error Count";
    if (id == 200)  return "Write Error Rate";
    if (id == 201)  return "TA Counter Detected";
    if (id == 220)  return "Disk Shift";
    if (id == 222)  return "Loaded Hours";
    if (id == 223)  return "Load/Unload Retry Count";
    if (id == 224)  return "Load Friction";
    if (id == 225)  return "Load Cycle Count";
    if (id == 226)  return "Load-In Time";
    if (id == 240)  return "Transfer Error Rate";   /* Fujitsu */
    return "Unknown Attribute";
};

static int ata_smart_get_attr_rawfmt(unsigned char id)
{
    if (id == 3)      /* Spin-up time */
        return RAWFMT_RAW16_OPT_AVG16;

    if (id == 5 ||    /* Reallocated sector count */
        id == 196)    /* Reallocated event count */
        return RAWFMT_RAW16_OPT_RAW16;

    if (id == 190 ||  /* Airflow Temperature */
        id == 194)    /* HDA Temperature */
        return RAWFMT_TEMPMINMAX;

    return RAWFMT_RAW48;
};

static int ata_smart_attr_to_string(
                struct ata_smart_attribute *attr, char *str, int size)
{
    uint16_t w[3]; /* 3 words to store 6 bytes of raw data */
    char buf[size]; /* temp string to store attribute data */
    int len, slen;
    int id = attr->id;

    if (id == 0)
        return 0; /* null attribute */

    /* align and convert raw data */
    memcpy(w, attr->raw, 6);
    w[0] = letoh16(w[0]);
    w[1] = letoh16(w[1]);
    w[2] = letoh16(w[2]);

    len = snprintf(buf, size, ": %u,%u ", attr->current, attr->worst);

    switch (ata_smart_get_attr_rawfmt(id))
    {
        case RAWFMT_RAW16_OPT_RAW16:
            len += snprintf(buf+len, size-len, "%u", w[0]);
            if ((w[1] || w[2]) && (len < size))
                len += snprintf(buf+len, size-len, " %u %u", w[1],w[2]);
            break;

        case RAWFMT_RAW16_OPT_AVG16:
            len += snprintf(buf+len, size-len, "%u", w[0]);
            if (w[1] && (len < size))
                len += snprintf(buf+len, size-len, " Avg: %u", w[1]);
            break;

        case RAWFMT_TEMPMINMAX:
            len += snprintf(buf+len, size-len, "%u -/+: %u/%u", w[0],w[1],w[2]);
            break;

        case RAWFMT_RAW48:
        default: {
            uint32_t tmp;
            memcpy(&tmp, w, sizeof(tmp));
            /* shows first 4 bytes of raw data as uint32 LE,
               and the ramaining 2 bytes as uint16 LE */
            len += snprintf(buf+len, size-len, "%lu", letoh32(tmp));
            if (w[2] && (len < size))
                len += snprintf(buf+len, size-len, " %u", w[2]);
            break;
        }
    }
    /* ignore trailing \0 when truncated */
    if (len >= size) len = size-1;

    /* fill return string; when max. size is exceded: first truncate
       attribute name, then attribute data and finally attribute id */
    slen = snprintf(str, size, "%d ", id);
    if (slen < size) {
        /* maximum space disponible for attribute name,
           including initial space separator */
        int name_sz = size - (slen + len);
        if (name_sz > 1) {
            len = snprintf(str+slen, name_sz, " %s",
                           ata_smart_get_attr_name(id));
            if (len >= name_sz) len = name_sz-1;
            slen += len;
        }

        strmemccpy(str+slen, buf, size-slen);
    }

    return 1; /* ok */
}

static bool ata_smart_dump(void)
{
    int fd;

    fd = creat("/smart_data.bin", 0666);
    if(fd >= 0)
    {
        write(fd, &smart_data, sizeof(struct ata_smart_values));
        close(fd);
    }

    fd = creat("/smart_data.txt", 0666);
    if(fd >= 0)
    {
        int i;
        char buf[128];
        for (i = 0; i < NUMBER_ATA_SMART_ATTRIBUTES; i++)
        {
            if (ata_smart_attr_to_string(
                    &smart_data.vendor_attributes[i], buf, sizeof(buf)))
            {
                write(fd, buf, strlen(buf));
                write(fd, "\n", 1);
            }
        }
        close(fd);
    }

    return false;
}

static int ata_smart_callback(int btn, struct gui_synclist *lists)
{
    (void)lists;
    static bool read_done = false;

    if (btn == ACTION_STD_CANCEL)
    {
        read_done = false;
        return btn;
    }

    /* read S.M.A.R.T. data only on first redraw */
    if (!read_done)
    {
        int rc;
        memset(&smart_data, 0, sizeof(struct ata_smart_values));
        rc = ata_read_smart(&smart_data);
        simplelist_set_line_count(0);
        if (rc == 0)
        {
            int i;
            char buf[SIMPLELIST_MAX_LINELENGTH];
            simplelist_addline("Id  Name:  Current,Worst  Raw");
            for (i = 0; i < NUMBER_ATA_SMART_ATTRIBUTES; i++)
            {
                if (ata_smart_attr_to_string(
                        &smart_data.vendor_attributes[i], buf, sizeof(buf)))
                {
                    simplelist_addline(buf);
                }
            }
        }
        else
        {
            simplelist_addline("ATA SMART error: %#x", rc);
        }
        read_done = true;
    }

    if (btn == ACTION_STD_CONTEXT)
    {
        splash(0, "Dumping data...");
        ata_smart_dump();
        splash(HZ, "SMART data dumped");
    }

    return btn;
}

static bool dbg_ata_smart(void)
{
    struct simplelist_info info;
    simplelist_info_init(&info, "S.M.A.R.T. Data [CONTEXT to dump]", 1, NULL);
    info.action_callback = ata_smart_callback;
    info.scroll_all = true;
    return simplelist_show_list(&info);
}
#endif /* HAVE_ATA_SMART */
#else /* No SD, MMC or ATA */
static int disk_callback(int btn, struct gui_synclist *lists)
{
    (void)lists;
    struct storage_info info;
    storage_get_info(0,&info);
    simplelist_addline("Vendor: %s", info.vendor);
    simplelist_addline("Model: %s", info.product);
    simplelist_addline("Firmware: %s", info.revision);
    simplelist_addline(
            "Size: %llu MB", (uint64_t)(info.num_sectors*(info.sector_size/512)/2048));
    sector_t free;
    volume_size( IF_MV(0,) NULL, &free );
    simplelist_addline(
             "Free: %ld MB", free / 1024);
    simplelist_addline(
             "Cluster size: %d bytes", volume_get_cluster_size(IF_MV(0)));
    return btn;
}
#endif

#if  (CONFIG_STORAGE & STORAGE_ATA)
static bool dbg_identify_info(void)
{
    int fd = creat("/identify_info.bin", 0666);
    if(fd >= 0)
    {
        const unsigned short *identify_info = ata_get_identify();
#ifdef ROCKBOX_LITTLE_ENDIAN
        /* this is a pointer to a driver buffer so we can't modify it */
        for (int i = 0; i < ATA_IDENTIFY_WORDS; ++i)
        {
            unsigned short word = swap16(identify_info[i]);
            write(fd, &word, 2);
        }
#else
        write(fd, identify_info, ATA_IDENTIFY_WORDS*2);
#endif
        close(fd);
    }
    return false;
}
#endif

static bool dbg_disk_info(void)
{
    struct simplelist_info info;
    simplelist_info_init(&info, "Disk Info", 1, NULL);
#if (CONFIG_STORAGE & STORAGE_MMC) || (CONFIG_STORAGE & STORAGE_SD)
    char title[16];
    int card = 0;
    info.callback_data = (void*)&card;
    info.title = title;
#endif
    info.action_callback = disk_callback;
    info.scroll_all = true;
    return simplelist_show_list(&info);
}
#endif /* PLATFORM_NATIVE */

#ifdef HAVE_DIRCACHE
static int dircache_callback(int btn, struct gui_synclist *lists)
{
    (void)lists;
    struct dircache_info info;
    dircache_get_info(&info);

    if (global_settings.dircache)
    {
        switch (btn)
        {
        case ACTION_STD_CONTEXT:
            splash(HZ/2, "Rebuilding cache");
            dircache_suspend();
            *(int *)lists->data = dircache_resume();
            /* Fallthrough */
        case ACTION_UNKNOWN:
            btn = ACTION_NONE;
            break;
    #ifdef DIRCACHE_DUMPSTER
        case ACTION_STD_OK:
            splash(0, "Dumping cache");
            dircache_dump();
            btn = ACTION_NONE;
            break;
    #endif /* DIRCACHE_DUMPSTER */
        case ACTION_STD_CANCEL:
            if (*(int *)lists->data > 0 && info.status == DIRCACHE_SCANNING)
            {
                splash(HZ, ID2P(LANG_SCANNING_DISK));
                btn = ACTION_NONE;
            }
            break;
        }
    }

    simplelist_set_line_count(0);

    simplelist_addline("Cache status: %s", info.statusdesc);
    simplelist_addline("Last size: %zu B", info.last_size);
    simplelist_addline("Size: %zu B", info.size);
    unsigned int utilized = info.size ? 1000ull*info.sizeused / info.size : 0;
    simplelist_addline("Used: %zu B (%u.%u%%)", info.sizeused,
                       utilized / 10, utilized % 10);
    simplelist_addline("Limit: %zu B", info.size_limit);
    simplelist_addline("Reserve: %zu/%zu B", info.reserve_used, info.reserve);
    long ticks = ALIGN_UP(info.build_ticks, HZ / 10);
    simplelist_addline("Scanning took: %ld.%ld s",
                       ticks / HZ, (ticks*10 / HZ) % 10);
    simplelist_addline("Entry count: %u", info.entry_count);

    if (btn == ACTION_NONE)
        btn = ACTION_REDRAW;

    return btn;
}

static bool dbg_dircache_info(void)
{
    struct simplelist_info info;
    int syncbuild = 0;
    simplelist_info_init(&info, "Dircache Info", 8, &syncbuild);
    info.action_callback = dircache_callback;
    info.scroll_all = true;
    return simplelist_show_list(&info);
}

#endif /* HAVE_DIRCACHE */

#ifdef HAVE_TAGCACHE
static int database_callback(int btn, struct gui_synclist *lists)
{
    (void)lists;
    struct tagcache_stat *stat = tagcache_get_stat();
    static bool synced = false;
    static int update_entries = 0;

    simplelist_set_line_count(0);

    simplelist_addline("Initialized: %s",
             stat->initialized ? "Yes" : "No");
    simplelist_addline("DB Ready: %s",
             stat->ready ? "Yes" : "No");
    simplelist_addline("DB Path: %s", stat->db_path);
    simplelist_addline("RAM Cache: %s",
             stat->ramcache ? "Yes" : "No");
    simplelist_addline("RAM: %d/%d B",
             stat->ramcache_used, stat->ramcache_allocated);
    simplelist_addline("Total entries: %d",
                       stat->total_entries);
    simplelist_addline("Progress: %d%% (%d entries)",
             stat->progress, stat->processed_entries);
    simplelist_addline("Curfile: %s",
                       stat->curentry ? stat->curentry : "---");
    simplelist_addline("Commit step: %d",
             stat->commit_step);
    simplelist_addline("Commit delayed: %s",
             stat->commit_delayed ? "Yes" : "No");

    simplelist_addline("Queue length: %d",
             stat->queue_length);

    if (synced)
    {
        synced = false;
        tagcache_screensync_event();
    }

    if (!btn && stat->curentry)
    {
        synced = true;
        if (update_entries <= stat->processed_entries)
        {
            update_entries = stat->processed_entries + 100;
            return ACTION_REDRAW;
        }
        return ACTION_NONE;
    }

    if (btn == ACTION_STD_CANCEL)
    {
        update_entries = 0;
        tagcache_screensync_enable(false);
    }
    return btn;
}
static bool dbg_tagcache_info(void)
{
    struct simplelist_info info;
    simplelist_info_init(&info, "Database Info", 8, NULL);
    info.action_callback = database_callback;
    info.scroll_all = true;

    /* Don't do nonblock here, must give enough processing time
       for tagcache thread. */
    /* info.timeout = TIMEOUT_NOBLOCK; */
    info.timeout = 1;
    tagcache_screensync_enable(true);
    return simplelist_show_list(&info);
}
#endif

#if defined CPU_COLDFIRE
static bool dbg_save_roms(void)
{
    int fd;
    int oldmode = system_memory_guard(MEMGUARD_NONE);

#if defined(IRIVER_H100_SERIES)
    fd = creat("/internal_rom_000000-1FFFFF.bin", 0666);
#elif defined(IRIVER_H300_SERIES)
    fd = creat("/internal_rom_000000-3FFFFF.bin", 0666);
#elif defined(IAUDIO_X5) || defined(IAUDIO_M5) || defined(IAUDIO_M3)
    fd = creat("/internal_rom_000000-3FFFFF.bin", 0666);
#elif defined(MPIO_HD200) || defined(MPIO_HD300)
    fd = creat("/internal_rom_000000-1FFFFF.bin", 0666);
#endif
    if(fd >= 0)
    {
        write(fd, (void *)0, FLASH_SIZE);
        close(fd);
    }
    system_memory_guard(oldmode);

#ifdef HAVE_EEPROM
    fd = creat("/internal_eeprom.bin", 0666);
    if (fd >= 0)
    {
        int old_irq_level;
        char buf[EEPROM_SIZE];
        int err;

        old_irq_level = disable_irq_save();

        err = eeprom_24cxx_read(0, buf, sizeof buf);

        restore_irq(old_irq_level);

        if (err)
            splashf(HZ*3, "Eeprom read failure (%d)", err);
        else
        {
            write(fd, buf, sizeof buf);
        }

        close(fd);
    }
#endif

    return false;
}
#elif defined(CPU_PP) && !(CONFIG_STORAGE & STORAGE_SD)
static bool dbg_save_roms(void)
{
    int fd = creat("/internal_rom_000000-0FFFFF.bin", 0666);
    if(fd >= 0)
    {
        write(fd, (void *)0x20000000, FLASH_SIZE);
        close(fd);
    }

    return false;
}
#elif CONFIG_CPU == AS3525v2 || CONFIG_CPU == AS3525
static bool dbg_save_roms(void)
{
    int fd = creat("/rom.bin", 0666);
    if(fd >= 0)
    {
        write(fd, (void *)0x80000000, 0x20000);
        close(fd);
    }

    return false;
}
#elif CONFIG_CPU == IMX31L
bool __dbg_dvfs_dptc(void);
static bool dbg_save_roms(void)
{
    int fd = creat("/flash_rom_A0000000-A01FFFFF.bin", 0666);
    if (fd >= 0)
    {
        write(fd, (void*)0xa0000000, FLASH_SIZE);
        close(fd);
    }

    return false;
}
#elif defined(CPU_TCC780X)
static bool dbg_save_roms(void)
{
    int fd = creat("/eeprom_E0000000-E0001FFF.bin", 0666);
    if (fd >= 0)
    {
        write(fd, (void*)0xe0000000, 0x2000);
        close(fd);
    }

    return false;
}
#elif CONFIG_CPU == RK27XX
static bool dbg_save_roms(void)
{
    char buf[0x200];

    int fd = creat("/rom.bin", 0666);
    if(fd < 0)
        return false;

    for(int addr = 0; addr < 0x2000; addr += sizeof(buf))
    {
        int old_irq = disable_irq_save();

        /* map rom at 0 */
        SCU_REMAP = 0;
        commit_discard_idcache();

        /* copy rom */
        memcpy((void *)buf, (void *)addr, sizeof(buf));

        /* map iram back at 0 */
        SCU_REMAP = 0xdeadbeef;
        commit_discard_idcache();

        restore_irq(old_irq);

        write(fd, (void *)buf, sizeof(buf));
    }
    close(fd);

    return false;
}
#endif /* CPU */

#ifndef SIMULATOR
#if CONFIG_TUNER

#ifdef CONFIG_TUNER_MULTI
static int tuner_type = 0;
#define IF_TUNER_TYPE(type) if(tuner_type==type)
#else
#define IF_TUNER_TYPE(type)
#endif

static int radio_callback(int btn, struct gui_synclist *lists)
{
    (void)lists;
    if (btn == ACTION_STD_CANCEL)
        return btn;
    simplelist_set_line_count(1);

#if (CONFIG_TUNER & LV24020LP)
    simplelist_addline(
                        "CTRL_STAT: %02X", lv24020lp_get(LV24020LP_CTRL_STAT) );
    simplelist_addline(
                        "RADIO_STAT: %02X", lv24020lp_get(LV24020LP_REG_STAT) );
    simplelist_addline(
                        "MSS_FM: %d kHz", lv24020lp_get(LV24020LP_MSS_FM) );
    simplelist_addline(
                        "MSS_IF: %d Hz", lv24020lp_get(LV24020LP_MSS_IF) );
    simplelist_addline(
                        "MSS_SD: %d Hz", lv24020lp_get(LV24020LP_MSS_SD) );
    simplelist_addline(
                        "if_set: %d Hz", lv24020lp_get(LV24020LP_IF_SET) );
    simplelist_addline(
                        "sd_set: %d Hz", lv24020lp_get(LV24020LP_SD_SET) );
#endif /* LV24020LP */
#if (CONFIG_TUNER & TEA5767)
    struct tea5767_dbg_info nfo;
    tea5767_dbg_info(&nfo);
    simplelist_addline("Philips regs:");
    simplelist_addline(
             "   %s: %02X %02X %02X %02X %02X", "Read",
             (unsigned)nfo.read_regs[0], (unsigned)nfo.read_regs[1],
             (unsigned)nfo.read_regs[2], (unsigned)nfo.read_regs[3],
             (unsigned)nfo.read_regs[4]);
    simplelist_addline(
             "   %s: %02X %02X %02X %02X %02X", "Write",
             (unsigned)nfo.write_regs[0], (unsigned)nfo.write_regs[1],
             (unsigned)nfo.write_regs[2], (unsigned)nfo.write_regs[3],
             (unsigned)nfo.write_regs[4]);
#endif /* TEA5767 */
#if (CONFIG_TUNER & SI4700)
    IF_TUNER_TYPE(SI4700)
    {
        struct si4700_dbg_info nfo;
        si4700_dbg_info(&nfo);
        simplelist_addline("SI4700 regs:");
        for (int i = 0; i < 16; i += 4) {
            simplelist_addline("%02X: %04X %04X %04X %04X",
                i, nfo.regs[i], nfo.regs[i+1], nfo.regs[i+2], nfo.regs[i+3]);
        }
    }
#endif /* SI4700 */
#if (CONFIG_TUNER & RDA5802)
    IF_TUNER_TYPE(RDA5802)
    {
        struct rda5802_dbg_info nfo;
        rda5802_dbg_info(&nfo);
        simplelist_addline("RDA5802 regs:");
        for (int i = 0; i < 16; i += 4) {
            simplelist_addline("%02X: %04X %04X %04X %04X",
                i, nfo.regs[i], nfo.regs[i+1], nfo.regs[i+2], nfo.regs[i+3]);
        }
    }
#endif /* RDA55802 */
#if (CONFIG_TUNER & STFM1000)
    IF_TUNER_TYPE(STFM1000)
    {
        struct stfm1000_dbg_info nfo;
        stfm1000_dbg_info(&nfo);
        simplelist_addline("STFM1000 regs:");
        simplelist_addline("chipid: 0x%lx", nfo.chipid);
    }
#endif /* STFM1000 */
#if (CONFIG_TUNER & TEA5760)
    IF_TUNER_TYPE(TEA5760)
    {
        struct tea5760_dbg_info nfo;
        tea5760_dbg_info(&nfo);
        simplelist_addline("TEA5760 regs:");
        for (int i = 0; i < 16; i += 4) {
            simplelist_addline("%02X: %02X %02X %02X %02X",
                i, nfo.read_regs[i], nfo.read_regs[i+1], nfo.read_regs[i+2], nfo.read_regs[i+3]);
        }
    }
#endif /* TEA5760 */

#ifdef HAVE_RDS_CAP
    {
        char buf[65*4];
        uint16_t pi;
        time_t seconds;

        tuner_get_rds_info(RADIO_RDS_NAME, buf, sizeof (buf));
        tuner_get_rds_info(RADIO_RDS_PROGRAM_INFO, &pi, sizeof (pi));
        simplelist_addline("PI:%04X PS:'%-8s'", pi, buf);
        tuner_get_rds_info(RADIO_RDS_TEXT, buf, sizeof (buf));
        simplelist_addline("RT:%s", buf);
        tuner_get_rds_info(RADIO_RDS_CURRENT_TIME, &seconds, sizeof (seconds));

        struct tm* time = gmtime(&seconds);
        simplelist_addline(
            "CT:%4d-%02d-%02d %02d:%02d:%02d",
            time->tm_year + 1900, time->tm_mon + 1, time->tm_mday,
            time->tm_hour, time->tm_min, time->tm_sec);
    }
#endif /* HAVE_RDS_CAP */
    return ACTION_REDRAW;
}
static bool dbg_fm_radio(void)
{
    struct simplelist_info info;
#ifdef CONFIG_TUNER_MULTI
    tuner_type = tuner_detect_type();
#endif
    info.scroll_all = true;
    simplelist_info_init(&info, "FM Radio", 1, NULL);
    simplelist_set_line_count(0);
    simplelist_addline("HW detected: %s",
                       radio_hardware_present() ? "yes" : "no");

    info.action_callback = radio_hardware_present()?radio_callback : NULL;
    return simplelist_show_list(&info);
}
#endif /* CONFIG_TUNER */
#endif /* !SIMULATOR */

#if !defined(APPLICATION)
extern bool do_screendump_instead_of_usb;

static bool dbg_screendump(void)
{
    do_screendump_instead_of_usb = !do_screendump_instead_of_usb;
    splashf(HZ, "Screendump %sabled", do_screendump_instead_of_usb?"en":"dis");
    return false;
}
#endif /* !APPLICATION */

extern bool write_metadata_log;

static bool dbg_metadatalog(void)
{
    write_metadata_log = !write_metadata_log;
    splashf(HZ, "Metadata log %sabled", write_metadata_log ? "en" : "dis");
    return false;
}

#if defined(CPU_COLDFIRE)
static bool dbg_set_memory_guard(void)
{
    static const struct opt_items names[MAXMEMGUARD] = {
        { "None",             -1 },
        { "Flash ROM writes", -1 },
        { "Zero area (all)",  -1 }
    };
    int mode = system_memory_guard(MEMGUARD_KEEP);

    set_option( "Catch mem accesses", &mode, RB_INT, names, MAXMEMGUARD, NULL);
    system_memory_guard(mode);

    return false;
}
#endif /* defined(CPU_COLDFIRE) */

#if defined(HAVE_EEPROM) && !defined(HAVE_EEPROM_SETTINGS)
static bool dbg_write_eeprom(void)
{
    int fd = open("/internal_eeprom.bin", O_RDONLY);

    if (fd >= 0)
    {
        char buf[EEPROM_SIZE];
        int rc = read(fd, buf, EEPROM_SIZE);

        if(rc == EEPROM_SIZE)
        {
            int old_irq_level = disable_irq_save();

            int err = eeprom_24cxx_write(0, buf, sizeof buf);
            if (err)
                splashf(HZ*3, "Eeprom write failure (%d)", err);
            else
                splash(HZ*3, "Eeprom written successfully");

            restore_irq(old_irq_level);
        }
        else
        {
            splashf(HZ*3, "File read error (%d)",rc);
        }
        close(fd);
    }
    else
    {
        splash(HZ*3, "Failed to open 'internal_eeprom.bin'");
    }

    return false;
}
#endif /* defined(HAVE_EEPROM) && !defined(HAVE_EEPROM_SETTINGS) */
#ifdef CPU_BOOST_LOGGING
static bool cpu_boost_log(void)
{
    int count = cpu_boost_log_getcount();
    char *str = cpu_boost_log_getlog_first();
    bool done;
    lcd_setfont(FONT_SYSFIXED);
    for (int i = 0; i < count ;)
    {
        lcd_clear_display();
        for(int j=0; j<LCD_HEIGHT/SYSFONT_HEIGHT; j++,i++)
        {
            if (!str)
                str = cpu_boost_log_getlog_next();
            if (str)
            {
                if(strlen(str) > LCD_WIDTH/SYSFONT_WIDTH)
                    lcd_puts_scroll(0, j, str);
                else
                    lcd_puts(0, j,str);
            }
            str = NULL;
        }
        lcd_update();
        done = false;
        while (!done)
        {
            switch(get_action(CONTEXT_STD,TIMEOUT_BLOCK))
            {
                case ACTION_STD_OK:
                case ACTION_STD_PREV:
                case ACTION_STD_NEXT:
                    done = true;
                break;
                case ACTION_STD_CANCEL:
                    i = count;
                    done = true;
                break;
            }
        }
    }
    lcd_scroll_stop();
    get_action(CONTEXT_STD,TIMEOUT_BLOCK);
    lcd_setfont(FONT_UI);
    return false;
}

static bool cpu_boost_log_dump(void)
{
    int fd;
    int count = cpu_boost_log_getcount();
    char *str = cpu_boost_log_getlog_first();

    splashf(HZ, "Boost Log File Dumped");

    /* nothing to print ? */
    if(count == 0)
        return false;

#if CONFIG_RTC
    char fname[MAX_PATH];
    struct tm *nowtm = get_time();
    fd = open_pathfmt(fname, sizeof(fname), O_CREAT|O_WRONLY|O_TRUNC,
                      "%s/boostlog_%04d%02d%02d%02d%02d%02d.txt", ROCKBOX_DIR,
                      nowtm->tm_year + 1900, nowtm->tm_mon + 1, nowtm->tm_mday,
                      nowtm->tm_hour, nowtm->tm_min, nowtm->tm_sec);
#else
    fd = open(ROCKBOX_DIR "/boostlog.txt", O_CREAT|O_WRONLY|O_TRUNC, 0666);
#endif
    if(-1 != fd) {
        for (int i = 0; i < count; i++)
        {
            if (!str)
                str = cpu_boost_log_getlog_next();
            if (str)
            {
               fdprintf(fd, "%s\n", str);
               str = NULL;
            }
        }

        close(fd);
        return true;
    }

    return false;
}
#endif

#if (defined(HAVE_WHEEL_ACCELERATION) && (CONFIG_KEYPAD==IPOD_4G_PAD) \
     && !defined(IPOD_MINI) && !defined(SIMULATOR))
extern bool wheel_is_touched;
extern int old_wheel_value;
extern int new_wheel_value;
extern int wheel_delta;
extern unsigned int accumulated_wheel_delta;
extern unsigned int wheel_velocity;

static bool dbg_scrollwheel(void)
{
    lcd_setfont(FONT_SYSFIXED);

    while (1)
    {
        if (action_userabort(HZ/10))
            break;

        lcd_clear_display();

        /* show internal variables of scrollwheel driver */
        lcd_putsf(0, 0, "wheel touched: %s", (wheel_is_touched) ? "true" : "false");
        lcd_putsf(0, 1, "new position: %2d", new_wheel_value);
        lcd_putsf(0, 2, "old position: %2d", old_wheel_value);
        lcd_putsf(0, 3, "wheel delta: %2d", wheel_delta);
        lcd_putsf(0, 4, "accumulated delta: %2d", accumulated_wheel_delta);
        lcd_putsf(0, 5, "velo [deg/s]: %4d", (int)wheel_velocity);

        /* show effective accelerated scrollspeed */
        lcd_putsf(0, 6, "accel. speed: %4d",
                button_apply_acceleration((1<<31)|(1<<24)|wheel_velocity) );

        lcd_update();
    }
    lcd_setfont(FONT_UI);
    return false;
}
#endif

static const char* dbg_talk_get_name(int selected_item, void *data,
                                     char *buffer, size_t buffer_len)
{
    struct talk_debug_data *talk_data = data;
    switch(selected_item)
    {
        case 0:
            if (talk_data->status != TALK_STATUS_ERR_NOFILE)
                snprintf(buffer, buffer_len, "Current voice file: %s",
                            talk_data->voicefile);
            else
                buffer = "No voice information available";
            break;
        case 1:
            if (talk_data->status != TALK_STATUS_OK)
                snprintf(buffer, buffer_len, "Talk Status: ERR (%i)",
                            talk_data->status);
            else
                buffer = "Talk Status: OK";
            break;
        case 2:
            snprintf(buffer, buffer_len, "Number of (empty) clips in voice file: (%d) %d",
                    talk_data->num_empty_clips, talk_data->num_clips);
            break;
        case 3:
            snprintf(buffer, buffer_len, "Min/Avg/Max size of clips: %d / %d / %d",
                    talk_data->min_clipsize, talk_data->avg_clipsize, talk_data->max_clipsize);
            break;
        case 4:
            snprintf(buffer, buffer_len, "Memory allocated: %ld.%02ld KB",
                    talk_data->memory_allocated / 1024, talk_data->memory_allocated % 1024);
            break;
        case 5:
            snprintf(buffer, buffer_len, "Memory used: %ld.%02ld KB",
                    talk_data->memory_used / 1024, talk_data->memory_used % 1024);
            break;
        case 6:
            snprintf(buffer, buffer_len, "Number of clips in cache: %d",
                    talk_data->cached_clips);
            break;
        case 7:
            snprintf(buffer, buffer_len, "Cache hits / misses: %d / %d",
                    talk_data->cache_hits, talk_data->cache_misses);
            break;
        default:
            buffer = "TODO";
            break;
    }

    return buffer;
}

static bool dbg_talk(void)
{
    struct simplelist_info list;
    struct talk_debug_data data;
    if (talk_get_debug_data(&data))
        simplelist_info_init(&list, "Voice Information:", 8, &data);
    else
        simplelist_info_init(&list, "Voice Information:", 2, &data);
    list.scroll_all = true;
    list.timeout = HZ;
    list.get_name = dbg_talk_get_name;

    return simplelist_show_list(&list);
}

#ifdef HAVE_USBSTACK
#if defined(ROCKBOX_HAS_LOGF) && defined(USB_ENABLE_SERIAL)
static bool toggle_usb_serial(void)
{
    bool enabled = !usb_core_driver_enabled(USB_DRIVER_SERIAL);

    usb_core_enable_driver(USB_DRIVER_SERIAL, enabled);
    splashf(HZ, "USB Serial %sabled", enabled ? "en" : "dis");

    return false;
}
#endif
#endif

#if CONFIG_USBOTG == USBOTG_ISP1583
extern int dbg_usb_num_items(void);
extern const char* dbg_usb_item(int selected_item, void *data,
                                char *buffer, size_t buffer_len);

static int isp1583_action_callback(int action, struct gui_synclist *lists)
{
    (void)lists;
    if (action == ACTION_NONE)
        action = ACTION_REDRAW;
    return action;
}

static bool dbg_isp1583(void)
{
    struct simplelist_info isp1583;
    isp1583.scroll_all = true;
    simplelist_info_init(&isp1583, "ISP1583", dbg_usb_num_items(), NULL);
    isp1583.timeout = HZ/100;
    isp1583.get_name = dbg_usb_item;
    isp1583.action_callback = isp1583_action_callback;
    return simplelist_show_list(&isp1583);
}
#endif

#if defined(CREATIVE_ZVx) && !defined(SIMULATOR)
extern int pic_dbg_num_items(void);
extern const char* pic_dbg_item(int selected_item, void *data,
                                char *buffer, size_t buffer_len);

static int pic_action_callback(int action, struct gui_synclist *lists)
{
    (void)lists;
    if (action == ACTION_NONE)
        action = ACTION_REDRAW;
    return action;
}

static bool dbg_pic(void)
{
    struct simplelist_info pic;
    pic.scroll_all = true;
    simplelist_info_init(&pic, "PIC", pic_dbg_num_items(), NULL);
    pic.timeout = HZ/100;
    pic.get_name = pic_dbg_item;
    pic.action_callback = pic_action_callback;
    return simplelist_show_list(&pic);
}
#endif

#if defined(HAVE_BOOTDATA) && !defined(SIMULATOR)
static bool dbg_boot_data(void)
{
    struct simplelist_info info;
    info.scroll_all = true;
    simplelist_info_init(&info, "Boot data", 1, NULL);
    simplelist_set_line_count(0);

    if (!boot_data_valid)
    {
        simplelist_addline("Boot data invalid");
        simplelist_addline("Magic[0]: %08lx", boot_data.magic[0]);
        simplelist_addline("Magic[1]: %08lx", boot_data.magic[1]);
        simplelist_addline("Length: %lu", boot_data.length);
    }
    else
    {
        simplelist_addline("Boot data valid");
        simplelist_addline("Version: %d", (int)boot_data.version);

        if (boot_data.version == 0)
        {
            simplelist_addline("Boot volume: %d", (int)boot_data._boot_volume);
        }
        else if (boot_data.version == 1)
        {
            simplelist_addline("Boot drive: %d", (int)boot_data.boot_drive);
            simplelist_addline("Boot partition: %d", (int)boot_data.boot_partition);
        }
        simplelist_addline("Boot path: %s%s/%s", root_realpath(), BOOTDIR, BOOTFILE);
    }

    simplelist_addline("Bootdata RAW:");
    for (size_t i = 0; i < boot_data.length; i += 4)
    {
        simplelist_addline("%02x: %02x %02x %02x %02x", i,
                           boot_data.payload[i + 0], boot_data.payload[i + 1],
                           boot_data.payload[i + 2], boot_data.payload[i + 3]);
    }

    return simplelist_show_list(&info);
}
#endif /* defined(HAVE_BOOTDATA) && !defined(SIMULATOR) */

#if defined(HAVE_DEVICEDATA)// && !defined(SIMULATOR)
static bool dbg_device_data(void)
{
    struct simplelist_info info;
    info.scroll_all = true;
    simplelist_info_init(&info, "Device data", 1, NULL);
    simplelist_set_line_count(0);

    simplelist_addline("Device data");

#if defined(EROS_QN)
    simplelist_addline("Lcd Version: %d", (int)device_data.lcd_version);
#endif

    simplelist_addline("Device data RAW:");
    for (size_t i = 0; i < device_data.length; i += 4)
    {
        simplelist_addline("%02x: %02x %02x %02x %02x", i,
                           device_data.payload[i + 0], device_data.payload[i + 1],
                           device_data.payload[i + 2], device_data.payload[i + 3]);
    }

    return simplelist_show_list(&info);
}
#endif /* defined(HAVE_DEVICEDATA)*/


#if defined(IPOD_6G) && !defined(SIMULATOR)
#define SYSCFG_MAX_ENTRIES 9 // 9 on iPod Classic/6G

static bool dbg_syscfg(void) {
    struct simplelist_info info;
    struct SysCfgHeader syscfg_hdr;
    size_t syscfg_hdr_size = sizeof(struct SysCfgHeader);
    size_t syscfg_entry_size = sizeof(struct SysCfgEntry);
    struct SysCfgEntry syscfg_entries[SYSCFG_MAX_ENTRIES];

    simplelist_info_init(&info, "SysCfg NOR contents", 1, NULL);
    simplelist_set_line_count(0);

    bootflash_init(SPI_PORT);
    bootflash_read(SPI_PORT, 0, syscfg_hdr_size, &syscfg_hdr);

    if (syscfg_hdr.magic != SYSCFG_MAGIC) {
        simplelist_addline("SCfg magic not found");
        bootflash_close(SPI_PORT);
        return simplelist_show_list(&info);
    }

    simplelist_addline("Total size: %lu bytes, %lu entries", syscfg_hdr.size, syscfg_hdr.num_entries);

    size_t calculated_syscfg_size = syscfg_hdr_size + syscfg_entry_size * syscfg_hdr.num_entries;

    if (syscfg_hdr.size != calculated_syscfg_size) {
        simplelist_addline("Wrong size: expected %zu, got %lu", calculated_syscfg_size, syscfg_hdr.size);
        bootflash_close(SPI_PORT);
        return simplelist_show_list(&info);
    }

    if (syscfg_hdr.num_entries > SYSCFG_MAX_ENTRIES) {
        simplelist_addline("Too many entries, showing only first %u", SYSCFG_MAX_ENTRIES);
    }

    size_t syscfg_num_entries = MIN(syscfg_hdr.num_entries, SYSCFG_MAX_ENTRIES);
    size_t syscfg_entries_size = syscfg_entry_size * syscfg_num_entries;

    bootflash_read(SPI_PORT, syscfg_hdr_size, syscfg_entries_size, &syscfg_entries);
    bootflash_close(SPI_PORT);

    for (size_t i = 0; i < syscfg_num_entries; i++) {
        struct SysCfgEntry* entry = &syscfg_entries[i];
        char* tag = (char *)&entry->tag;
        uint32_t* data32 = (uint32_t *)entry->data;

        switch (entry->tag) {
            case SYSCFG_TAG_SRNM:
                simplelist_addline("Serial number (SrNm): %s", entry->data);
                break;
            case SYSCFG_TAG_FWID:
                simplelist_addline("Firmware ID (FwId): %07lX", data32[1] & 0x0FFFFFFF);
                break;
            case SYSCFG_TAG_HWID:
                simplelist_addline("Hardware ID (HwId): %08lX", data32[0]);
                break;
            case SYSCFG_TAG_HWVR:
                simplelist_addline("Hardware version (HwVr): %06lX", data32[1]);
                break;
            case SYSCFG_TAG_CODC:
                simplelist_addline("Codec (Codc): %s", entry->data);
                break;
            case SYSCFG_TAG_SWVR:
                simplelist_addline("Software version (SwVr): %s", entry->data);
                break;
            case SYSCFG_TAG_MLBN:
                simplelist_addline("Logic board serial number (MLBN): %s", entry->data);
                break;
            case SYSCFG_TAG_MODN:
                simplelist_addline("Model number (Mod#): %s", entry->data);
                break;
            case SYSCFG_TAG_REGN:
                simplelist_addline("Sales region (Regn): %08lX %08lX", data32[0], data32[1]);
                break;
            default:
                simplelist_addline("%c%c%c%c: %08lX %08lX %08lX %08lX",
                    tag[3], tag[2], tag[1], tag[0],
                    data32[0], data32[1], data32[2], data32[3]
                );
                break;
        }
    }

    return simplelist_show_list(&info);
}
#endif

/****** The menu *********/
static const struct {
    unsigned char *desc; /* string or ID */
    bool (*function) (void); /* return true if USB was connected */
} menuitems[] = {
#if defined(CPU_COLDFIRE) || \
    (defined(CPU_PP) && !(CONFIG_STORAGE & STORAGE_SD)) || \
    CONFIG_CPU == IMX31L || defined(CPU_TCC780X) || CONFIG_CPU == AS3525v2 || \
    CONFIG_CPU == AS3525 || CONFIG_CPU == RK27XX
        { "Dump ROM contents", dbg_save_roms },
#endif
#if defined(CPU_COLDFIRE) || defined(CPU_PP) \
    || CONFIG_CPU == S3C2440 || CONFIG_CPU == IMX31L || CONFIG_CPU == AS3525 \
    || CONFIG_CPU == DM320 || defined(CPU_S5L870X) || CONFIG_CPU == AS3525v2 \
    || CONFIG_CPU == RK27XX || CONFIG_CPU == JZ4760B
        { "View I/O ports", dbg_ports },
#endif
#if (CONFIG_RTC == RTC_PCF50605) && (CONFIG_PLATFORM & PLATFORM_NATIVE)
        { "View PCF registers", dbg_pcf },
#endif
#if defined(HAVE_TSC2100) && (CONFIG_PLATFORM & PLATFORM_NATIVE)
        { "TSC2100 debug", tsc2100_debug },
#endif
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        { "CPU frequency", dbg_cpufreq },
#endif
#if CONFIG_CPU == IMX31L
        { "DVFS/DPTC", __dbg_dvfs_dptc },
#endif
#if defined(IRIVER_H100_SERIES) && !defined(SIMULATOR)
        { "S/PDIF analyzer", dbg_spdif },
#endif
#if defined(CPU_COLDFIRE)
        { "Catch mem accesses", dbg_set_memory_guard },
#endif
        { "View OS stacks", dbg_os },
#ifdef __linux__
        { "View CPU stats", dbg_cpuinfo },
#endif
#if (CONFIG_BATTERY_MEASURE != 0) && !defined(SIMULATOR)
        { "View battery", view_battery },
#endif
#ifndef APPLICATION
        { "Screendump", dbg_screendump },
#endif
        { "Skin Engine RAM usage", dbg_skin_engine },
#if ((CONFIG_PLATFORM & PLATFORM_NATIVE) || defined(SONY_NWZ_LINUX) || defined(HIBY_LINUX) || defined(FIIO_M3K_LINUX)) && !defined(SIMULATOR)
        { "View HW info", dbg_hw_info },
#endif
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
        { "View partitions", dbg_partitions },
#endif
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
        { "View disk info", dbg_disk_info },
#if (CONFIG_STORAGE & STORAGE_ATA)
        { "Dump ATA identify info", dbg_identify_info},
#ifdef HAVE_ATA_SMART
        { "View/Dump S.M.A.R.T. data", dbg_ata_smart},
#endif
#endif
#endif
        { "Metadata log", dbg_metadatalog },
#ifdef HAVE_DIRCACHE
        { "View dircache info", dbg_dircache_info },
#endif
#ifdef HAVE_TAGCACHE
        { "View database info", dbg_tagcache_info },
#endif
        { "View buffering thread", dbg_buffering_thread },
#ifdef PM_DEBUG
        { "pm histogram", peak_meter_histogram},
#endif /* PM_DEBUG */
#ifdef BUFLIB_DEBUG_PRINT
        { "View buflib allocs", dbg_buflib_allocs },
#endif
#ifndef SIMULATOR
#if CONFIG_TUNER
        { "FM Radio", dbg_fm_radio },
#endif
#endif
#if defined(HAVE_EEPROM) && !defined(HAVE_EEPROM_SETTINGS)
        { "Write back EEPROM", dbg_write_eeprom },
#endif
#if CONFIG_USBOTG == USBOTG_ISP1583
        { "View ISP1583 info", dbg_isp1583 },
#endif
#if defined(CREATIVE_ZVx) && !defined(SIMULATOR)
        { "View PIC info", dbg_pic },
#endif
#ifdef ROCKBOX_HAS_LOGF
        {"Show Log File", logfdisplay },
        {"Dump Log File", logfdump },
#endif
#if defined(HAVE_USBSTACK)
#if defined(ROCKBOX_HAS_LOGF) && defined(USB_ENABLE_SERIAL)
        {"USB Serial driver (logf)", toggle_usb_serial },
#endif
#endif /* HAVE_USBSTACK */
#ifdef CPU_BOOST_LOGGING
        {"Show cpu_boost log",cpu_boost_log},
        {"Dump cpu_boost log",cpu_boost_log_dump},
#endif
#if (defined(HAVE_WHEEL_ACCELERATION) && (CONFIG_KEYPAD==IPOD_4G_PAD) \
     && !defined(IPOD_MINI) && !defined(SIMULATOR))
        {"Debug scrollwheel", dbg_scrollwheel },
#endif
        {"Talk engine stats", dbg_talk },
#if defined(HAVE_BOOTDATA) && !defined(SIMULATOR)
        {"Boot data", dbg_boot_data },
#endif

#if defined(HAVE_DEVICEDATA)// && !defined(SIMULATOR)
        {"Device data", dbg_device_data },
#endif

#if defined(IPOD_6G) && !defined(SIMULATOR)
        {"View SysCfg", dbg_syscfg },
#endif
};

static int menu_action_callback(int btn, struct gui_synclist *lists)
{
    int selection = gui_synclist_get_sel_pos(lists);
    if (btn == ACTION_STD_OK)
    {
        FOR_NB_SCREENS(i)
           viewportmanager_theme_enable(i, false, NULL);
        menuitems[selection].function();
        btn = ACTION_REDRAW;
        FOR_NB_SCREENS(i)
            viewportmanager_theme_undo(i, false);
    }
    else if (btn == ACTION_STD_CONTEXT)
    {
        MENUITEM_STRINGLIST(menu_items, "Debug Menu", NULL, ID2P(LANG_ADD_TO_FAVES));
        if (do_menu(&menu_items, NULL, NULL, false) == 0)
            shortcuts_add(SHORTCUT_DEBUGITEM, menuitems[selection].desc);
        return ACTION_STD_CANCEL;
    }
    return btn;
}

static const char* menu_get_name(int item, void * data,
                                    char *buffer, size_t buffer_len)
{
    (void)data; (void)buffer; (void)buffer_len;
    return menuitems[item].desc;
}

static int menu_get_talk(int item, void *data)
{
    (void)data;
    if (global_settings.talk_menu && menuitems[item].desc)
    {
        talk_number(item + 1, true);
        talk_id(VOICE_PAUSE, true);
#if 0 /* no debug items currently have lang ids */
        long id = P2ID((const unsigned char *)(menuitems[item].desc));
        if(id>=0)
            talk_id(id, true);
        else
#endif
        talk_spell(menuitems[item].desc, true);
     }
    return 0;
}

int debug_menu(void)
{
    struct simplelist_info info;

    simplelist_info_init(&info, "Debug Menu", ARRAYLEN(menuitems), NULL);
    info.action_callback = menu_action_callback;
    info.get_name        = menu_get_name;
    info.get_talk        = menu_get_talk;
    return (simplelist_show_list(&info)) ? 1 : 0;
}

bool run_debug_screen(char* screen)
{
    for (unsigned i=0; i<ARRAYLEN(menuitems); i++)
        if (!strcmp(screen, menuitems[i].desc))
        {
            FOR_NB_SCREENS(j)
               viewportmanager_theme_enable(j, false, NULL);
            menuitems[i].function();
            FOR_NB_SCREENS(j)
                viewportmanager_theme_undo(j, false);
            return true;
        }

    return false;
}
