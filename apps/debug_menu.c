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
#include <string.h>
#include "lcd.h"
#include "menu.h"
#include "debug_menu.h"
#include "kernel.h"
#include "sprintf.h"
#include "structec.h"
#include "action.h"
#include "debug.h"
#include "thread.h"
#include "powermgmt.h"
#include "system.h"
#include "font.h"
#include "audio.h"
#include "mp3_playback.h"
#include "settings.h"
#include "list.h"
#include "statusbar.h"
#include "dir.h"
#include "panic.h"
#include "screens.h"
#include "misc.h"
#include "splash.h"
#include "dircache.h"
#include "viewport.h"
#ifdef HAVE_TAGCACHE
#include "tagcache.h"
#endif
#include "lcd-remote.h"
#include "crc32.h"
#include "logf.h"
#ifndef SIMULATOR
#include "disk.h"
#include "adc.h"
#include "power.h"
#include "usb.h"
#include "rtc.h"
#include "storage.h"
#include "fat.h"
#include "mas.h"
#include "eeprom_24cxx.h"
#if (CONFIG_STORAGE & STORAGE_MMC) || (CONFIG_STORAGE & STORAGE_SD)
#include "hotswap.h"
#endif
#if (CONFIG_STORAGE & STORAGE_ATA)
#include "ata.h"
#endif
#if CONFIG_TUNER
#include "tuner.h"
#include "radio.h"
#endif
#endif

#ifdef HAVE_LCD_BITMAP
#include "scrollbar.h"
#include "peakmeter.h"
#endif
#include "logfdisp.h"
#if CONFIG_CODEC == SWCODEC
#include "pcmbuf.h"
#include "buffering.h"
#include "playback.h"
#if defined(HAVE_SPDIF_OUT) || defined(HAVE_SPDIF_IN)
#include "spdif.h"
#endif
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

#if CONFIG_CPU == DM320 || CONFIG_CPU == S3C2440 || CONFIG_CPU == TCC7801 \
    || CONFIG_CPU == IMX31L || CONFIG_CPU == AS3525
#include "debug-target.h"
#endif

#if defined(SANSA_E200) || defined(PHILIPS_SA9200)
#include "ascodec.h"
#include "as3514.h"
#endif

#if defined(HAVE_USBSTACK)
#include "usb_core.h"
#endif
#ifdef USB_STORAGE
#include "usbstack/usb_storage.h"
#endif

/*---------------------------------------------------*/
/*    SPECIAL DEBUG STUFF                            */
/*---------------------------------------------------*/
extern struct thread_entry threads[MAXTHREADS];

static char thread_status_char(unsigned status)
{
    static const char thread_status_chars[THREAD_NUM_STATES+1] =
    {
        [0 ... THREAD_NUM_STATES] = '?',
        [STATE_RUNNING]           = 'R',
        [STATE_BLOCKED]           = 'B',
        [STATE_SLEEPING]          = 'S',
        [STATE_BLOCKED_W_TMO]     = 'T',
        [STATE_FROZEN]            = 'F',
        [STATE_KILLED]            = 'K',
    };

    if (status > THREAD_NUM_STATES)
        status = THREAD_NUM_STATES;

    return thread_status_chars[status];
}

static char* threads_getname(int selected_item, void *data,
                             char *buffer, size_t buffer_len)
{
    (void)data;
    struct thread_entry *thread;
    char name[32];

#if NUM_CORES > 1
    if (selected_item < (int)NUM_CORES)
    {
        snprintf(buffer, buffer_len, "Idle (%d): %2d%%", selected_item,
                 idle_stack_usage(selected_item));
        return buffer;
    }

    selected_item -= NUM_CORES;
#endif

    thread = &threads[selected_item];

    if (thread->state == STATE_KILLED)
    {
        snprintf(buffer, buffer_len, "%2d: ---", selected_item);
        return buffer;
    }

    thread_get_name(name, 32, thread);

    snprintf(buffer, buffer_len,
             "%2d: " IF_COP("(%d) ") "%c%c " IF_PRIO("%d %d ") "%2d%% %s",
             selected_item,
             IF_COP(thread->core,)
#ifdef HAVE_SCHEDULER_BOOSTCTRL
             (thread->cpu_boost) ? '+' :
#endif
                 ((thread->state == STATE_RUNNING) ? '*' : ' '),
             thread_status_char(thread->state),
             IF_PRIO(thread->base_priority, thread->priority, )
             thread_stack_usage(thread), name);

    return buffer;
}
static int dbg_threads_action_callback(int action, struct gui_synclist *lists)
{
    (void)lists;
#ifdef ROCKBOX_HAS_LOGF
    if (action == ACTION_STD_OK)
    {
        int selpos = gui_synclist_get_sel_pos(lists);
#if NUM_CORES > 1
        if (selpos >= NUM_CORES)
            remove_thread(&threads[selpos - NUM_CORES]);
#else
        remove_thread(&threads[selpos]);
#endif
        return ACTION_REDRAW;
    }
#endif /* ROCKBOX_HAS_LOGF */
    if (action == ACTION_NONE)
        action = ACTION_REDRAW;
    return action;
}
/* Test code!!! */
static bool dbg_os(void)
{
    struct simplelist_info info;
    simplelist_info_init(&info, IF_COP("Core and ") "Stack usage:",
#if NUM_CORES == 1
                            MAXTHREADS,
#else
                            MAXTHREADS+NUM_CORES,
#endif
                            NULL);
#ifndef ROCKBOX_HAS_LOGF
    info.hide_selection = true;
    info.scroll_all = true;
#endif
    info.action_callback = dbg_threads_action_callback;
    info.get_name = threads_getname;
    return simplelist_show_list(&info);
}

#ifdef HAVE_LCD_BITMAP
#if CONFIG_CODEC != SWCODEC
#ifndef SIMULATOR
static bool dbg_audio_thread(void)
{
    char buf[32];
    struct audio_debug d;

    lcd_setfont(FONT_SYSFIXED);
    viewportmanager_set_statusbar(false);

    while(1)
    {
        if (action_userabort(HZ/5))
            return false;

        audio_get_debugdata(&d);

        lcd_clear_display();

        snprintf(buf, sizeof(buf), "read: %x", d.audiobuf_read);
        lcd_puts(0, 0, buf);
        snprintf(buf, sizeof(buf), "write: %x", d.audiobuf_write);
        lcd_puts(0, 1, buf);
        snprintf(buf, sizeof(buf), "swap: %x", d.audiobuf_swapwrite);
        lcd_puts(0, 2, buf);
        snprintf(buf, sizeof(buf), "playing: %d", d.playing);
        lcd_puts(0, 3, buf);
        snprintf(buf, sizeof(buf), "playable: %x", d.playable_space);
        lcd_puts(0, 4, buf);
        snprintf(buf, sizeof(buf), "unswapped: %x", d.unswapped_space);
        lcd_puts(0, 5, buf);

        /* Playable space left */
        gui_scrollbar_draw(&screens[SCREEN_MAIN],0, 6*8, 112, 4, d.audiobuflen, 0,
                  d.playable_space, HORIZONTAL);

        /* Show the watermark limit */
        gui_scrollbar_draw(&screens[SCREEN_MAIN],0, 6*8+4, 112, 4, d.audiobuflen, 0,
                  d.low_watermark_level, HORIZONTAL);

        snprintf(buf, sizeof(buf), "wm: %x - %x",
                 d.low_watermark_level, d.lowest_watermark_level);
        lcd_puts(0, 7, buf);

        lcd_update();
    }
    viewportmanager_set_statusbar(true);
    return false;
}
#endif /* !SIMULATOR */
#else /* CONFIG_CODEC == SWCODEC */
extern size_t filebuflen;
/* This is a size_t, but call it a long so it puts a - when it's bad. */

static unsigned int ticks, boost_ticks, freq_sum;

static void dbg_audio_task(void)
{
#ifndef SIMULATOR
    if(FREQ > CPUFREQ_NORMAL)
        boost_ticks++;
    freq_sum += FREQ/1000000; /* in MHz */
#endif
    ticks++;
}

static bool dbg_buffering_thread(void)
{
    char buf[32];
    int button;
    int line;
    bool done = false;
    size_t bufused;
    size_t bufsize = pcmbuf_get_bufsize();
    int pcmbufdescs = pcmbuf_descs();
    struct buffering_debug d;

    ticks = boost_ticks = freq_sum = 0;

    tick_add_task(dbg_audio_task);

    lcd_setfont(FONT_SYSFIXED);
    viewportmanager_set_statusbar(false);
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

        line = 0;
        lcd_clear_display();

        bufused = bufsize - pcmbuf_free();

        snprintf(buf, sizeof(buf), "pcm: %7ld/%7ld", (long) bufused, (long) bufsize);
        lcd_puts(0, line++, buf);

        gui_scrollbar_draw(&screens[SCREEN_MAIN],0, line*8, LCD_WIDTH, 6,
                           bufsize, 0, bufused, HORIZONTAL);
        line++;

        snprintf(buf, sizeof(buf), "alloc: %8ld/%8ld", audio_filebufused(),
                 (long) filebuflen);
        lcd_puts(0, line++, buf);

#if LCD_HEIGHT > 80
        gui_scrollbar_draw(&screens[SCREEN_MAIN],0, line*8, LCD_WIDTH, 6,
                           filebuflen, 0, audio_filebufused(), HORIZONTAL);
        line++;

        snprintf(buf, sizeof(buf), "real:  %8ld/%8ld", (long)d.buffered_data,
                 (long)filebuflen);
        lcd_puts(0, line++, buf);

        gui_scrollbar_draw(&screens[SCREEN_MAIN],0, line*8, LCD_WIDTH, 6,
                           filebuflen, 0, (long)d.buffered_data, HORIZONTAL);
        line++;
#endif

        snprintf(buf, sizeof(buf), "usefl: %8ld/%8ld", (long)(d.useful_data),
                                                       (long)filebuflen);
        lcd_puts(0, line++, buf);

#if LCD_HEIGHT > 80
        gui_scrollbar_draw(&screens[SCREEN_MAIN],0, line*8, LCD_WIDTH, 6,
                           filebuflen, 0, d.useful_data, HORIZONTAL);
        line++;
#endif

        snprintf(buf, sizeof(buf), "data_rem: %ld", (long)d.data_rem);
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "track count: %2d", audio_track_count());
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "handle count: %d", (int)d.num_handles);
        lcd_puts(0, line++, buf);

#ifndef SIMULATOR
        snprintf(buf, sizeof(buf), "cpu freq: %3dMHz",
                 (int)((FREQ + 500000) / 1000000));
        lcd_puts(0, line++, buf);
#endif

        if (ticks > 0)
        {
            int boostquota = boost_ticks * 1000 / ticks; /* in 0.1 % */
            int avgclock   = freq_sum * 10 / ticks;      /* in 100 kHz */
            snprintf(buf, sizeof(buf), "boost ratio: %3d.%d%% (%2d.%dMHz)",
                     boostquota/10, boostquota%10, avgclock/10, avgclock%10);
            lcd_puts(0, line++, buf);
        }

        snprintf(buf, sizeof(buf), "pcmbufdesc: %2d/%2d",
                pcmbuf_used_descs(), pcmbufdescs);
        lcd_puts(0, line++, buf);

        lcd_update();
    }

    tick_remove_task(dbg_audio_task);
    viewportmanager_set_statusbar(true);

    return false;
}
#endif /* CONFIG_CODEC */
#endif /* HAVE_LCD_BITMAP */


#if (CONFIG_CPU == SH7034 || defined(CPU_COLDFIRE))
/* Tool function to read the flash manufacturer and type, if available.
   Only chips which could be reprogrammed in system will return values.
   (The mode switch addresses vary between flash manufacturers, hence addr1/2) */
   /* In IRAM to avoid problems when running directly from Flash */
static bool dbg_flash_id(unsigned* p_manufacturer, unsigned* p_device,
                         unsigned addr1, unsigned addr2)
                         ICODE_ATTR __attribute__((noinline));
static bool dbg_flash_id(unsigned* p_manufacturer, unsigned* p_device,
                         unsigned addr1, unsigned addr2)

{
    unsigned not_manu, not_id; /* read values before switching to ID mode */
    unsigned manu, id; /* read values when in ID mode */

#if CONFIG_CPU == SH7034
    volatile unsigned char* flash = (unsigned char*)0x2000000; /* flash mapping */
#elif defined(CPU_COLDFIRE)
    volatile unsigned short* flash = (unsigned short*)0; /* flash mapping */
#endif
    int old_level; /* saved interrupt level */

    not_manu = flash[0]; /* read the normal content */
    not_id   = flash[1]; /* should be 'A' (0x41) and 'R' (0x52) from the "ARCH" marker */

    /* disable interrupts, prevent any stray flash access */
    old_level = disable_irq_save();

    flash[addr1] = 0xAA; /* enter command mode */
    flash[addr2] = 0x55;
    flash[addr1] = 0x90; /* ID command */
    /* Atmel wants 20ms pause here */
    /* sleep(HZ/50); no sleeping possible while interrupts are disabled */

    manu = flash[0]; /* read the IDs */
    id   = flash[1];

    flash[0] = 0xF0; /* reset flash (back to normal read mode) */
    /* Atmel wants 20ms pause here */
    /* sleep(HZ/50); no sleeping possible while interrupts are disabled */

    restore_irq(old_level); /* enable interrupts again */

    /* I assume success if the obtained values are different from
        the normal flash content. This is not perfectly bulletproof, they
        could theoretically be the same by chance, causing us to fail. */
    if (not_manu != manu || not_id != id) /* a value has changed */
    {
        *p_manufacturer = manu; /* return the results */
        *p_device = id;
        return true; /* success */
    }
    return false; /* fail */
}
#endif /* (CONFIG_CPU == SH7034 || CPU_COLDFIRE) */

#ifndef SIMULATOR
#ifdef CPU_PP
static int perfcheck(void)
{
    int result;

    asm (
        "mrs     r2, CPSR            \n"
        "orr     r0, r2, #0xc0       \n" /* disable IRQ and FIQ */
        "msr     CPSR_c, r0          \n"
        "mov     %[res], #0          \n"
        "ldr     r0, [%[timr]]       \n"
        "add     r0, r0, %[tmo]      \n"
    "1:                              \n"
        "add     %[res], %[res], #1  \n"
        "ldr     r1, [%[timr]]       \n"
        "cmp     r1, r0              \n"
        "bmi     1b                  \n"
        "msr     CPSR_c, r2          \n" /* reset IRQ and FIQ state */
        :
        [res]"=&r"(result)
        :
        [timr]"r"(&USEC_TIMER),
        [tmo]"r"(
#if CONFIG_CPU == PP5002
        16000
#else /* PP5020/5022/5024 */
        10226
#endif
        )
        :
        "r0", "r1", "r2"
    );
    return result;
}
#endif

#ifdef HAVE_LCD_BITMAP
static bool dbg_hw_info(void)
{
#if CONFIG_CPU == SH7034
    char buf[32];
    int bitmask = HW_MASK;
    int rom_version = ROM_VERSION;
    unsigned manu, id; /* flash IDs */
    bool got_id; /* flag if we managed to get the flash IDs */
    unsigned rom_crc = 0xffffffff; /* CRC32 of the boot ROM */
    bool has_bootrom; /* flag for boot ROM present */
    int oldmode;  /* saved memory guard mode */

    oldmode = system_memory_guard(MEMGUARD_NONE);  /* disable memory guard */

    /* get flash ROM type */
    got_id = dbg_flash_id(&manu, &id, 0x5555, 0x2AAA); /* try SST, Atmel, NexFlash */
    if (!got_id)
        got_id = dbg_flash_id(&manu, &id, 0x555, 0x2AA); /* try AMD, Macronix */

    /* check if the boot ROM area is a flash mirror */
    has_bootrom = (memcmp((char*)0, (char*)0x02000000, 64*1024) != 0);
    if (has_bootrom)  /* if ROM and Flash different */
    {
        /* calculate CRC16 checksum of boot ROM */
        rom_crc = crc_32((unsigned char*)0x0000, 64*1024, 0xffffffff);
    }

    system_memory_guard(oldmode);  /* re-enable memory guard */

    lcd_setfont(FONT_SYSFIXED);
    lcd_clear_display();
    viewportmanager_set_statusbar(false);

    lcd_puts(0, 0, "[Hardware info]");

    snprintf(buf, 32, "ROM: %d.%02d", rom_version/100, rom_version%100);
    lcd_puts(0, 1, buf);

    snprintf(buf, 32, "Mask: 0x%04x", bitmask);
    lcd_puts(0, 2, buf);

    if (got_id)
        snprintf(buf, 32, "Flash: M=%02x D=%02x", manu, id);
    else
        snprintf(buf, 32, "Flash: M=?? D=??"); /* unknown, sorry */
    lcd_puts(0, 3, buf);

    if (has_bootrom)
    {
        if (rom_crc == 0x56DBA4EE) /* known Version 1 */
            snprintf(buf, 32, "Boot ROM: V1");
        else
            snprintf(buf, 32, "ROMcrc: 0x%08x", rom_crc);
    }
    else
    {
        snprintf(buf, 32, "Boot ROM: none");
    }
    lcd_puts(0, 4, buf);

    lcd_update();

    while (!(action_userabort(TIMEOUT_BLOCK)));

#elif CONFIG_CPU == MCF5249 || CONFIG_CPU == MCF5250
    char buf[32];
    unsigned manu, id; /* flash IDs */
    int got_id; /* flag if we managed to get the flash IDs */
    int oldmode;  /* saved memory guard mode */
    int line = 0;
    viewportmanager_set_statusbar(false);

    oldmode = system_memory_guard(MEMGUARD_NONE);  /* disable memory guard */

    /* get flash ROM type */
    got_id = dbg_flash_id(&manu, &id, 0x5555, 0x2AAA); /* try SST, Atmel, NexFlash */
    if (!got_id)
        got_id = dbg_flash_id(&manu, &id, 0x555, 0x2AA); /* try AMD, Macronix */

    system_memory_guard(oldmode);  /* re-enable memory guard */

    lcd_setfont(FONT_SYSFIXED);
    lcd_clear_display();

    lcd_puts(0, line++, "[Hardware info]");

    if (got_id)
        snprintf(buf, 32, "Flash: M=%04x D=%04x", manu, id);
    else
        snprintf(buf, 32, "Flash: M=???? D=????"); /* unknown, sorry */
    lcd_puts(0, line++, buf);

#ifdef IAUDIO_X5
    {
        struct ds2411_id id;

        lcd_puts(0, ++line, "Serial Number:");

        got_id = ds2411_read_id(&id);

        if (got_id == DS2411_OK)
        {
            snprintf(buf, 32, "  FC=%02x", (unsigned)id.family_code);
            lcd_puts(0, ++line, buf);
            snprintf(buf, 32, "  ID=%02X %02X %02X %02X %02X %02X",
                (unsigned)id.uid[0], (unsigned)id.uid[1], (unsigned)id.uid[2],
                (unsigned)id.uid[3], (unsigned)id.uid[4], (unsigned)id.uid[5]);
            lcd_puts(0, ++line, buf);
            snprintf(buf, 32, "  CRC=%02X", (unsigned)id.crc);
        }
        else
        {
            snprintf(buf, 32, "READ ERR=%d", got_id);
        }

        lcd_puts(0, ++line, buf);
    }
#endif

    lcd_update();

    while (!(action_userabort(TIMEOUT_BLOCK)));

#elif defined(CPU_PP502x)
    int line = 0;
    char buf[32];
    char pp_version[] = { (PP_VER2 >> 24) & 0xff, (PP_VER2 >> 16) & 0xff,
                          (PP_VER2 >> 8) & 0xff, (PP_VER2) & 0xff,
                          (PP_VER1 >> 24) & 0xff, (PP_VER1 >> 16) & 0xff,
                          (PP_VER1 >> 8) & 0xff, (PP_VER1) & 0xff, '\0' };

    lcd_setfont(FONT_SYSFIXED);
    lcd_clear_display();
    viewportmanager_set_statusbar(false);

    lcd_puts(0, line++, "[Hardware info]");

#ifdef IPOD_ARCH
    snprintf(buf, sizeof(buf), "HW rev: 0x%08lx", IPOD_HW_REVISION);
    lcd_puts(0, line++, buf);
#endif

#ifdef IPOD_COLOR
    extern int lcd_type; /* Defined in lcd-colornano.c */

    snprintf(buf, sizeof(buf), "LCD type: %d", lcd_type);
    lcd_puts(0, line++, buf);
#endif

    snprintf(buf, sizeof(buf), "PP version: %s", pp_version);
    lcd_puts(0, line++, buf);

    snprintf(buf, sizeof(buf), "Est. clock (kHz): %d", perfcheck());
    lcd_puts(0, line++, buf);

    lcd_update();

    while (!(action_userabort(TIMEOUT_BLOCK)));

#elif CONFIG_CPU == PP5002
    int line = 0;
    char buf[32];
    char pp_version[] = { (PP_VER4 >> 8) & 0xff, PP_VER4 & 0xff,
                          (PP_VER3 >> 8) & 0xff, PP_VER3 & 0xff,
                          (PP_VER2 >> 8) & 0xff, PP_VER2 & 0xff,
                          (PP_VER1 >> 8) & 0xff, PP_VER1 & 0xff, '\0' };


    lcd_setfont(FONT_SYSFIXED);
    lcd_clear_display();

    lcd_puts(0, line++, "[Hardware info]");

#ifdef IPOD_ARCH
    snprintf(buf, sizeof(buf), "HW rev: 0x%08lx", IPOD_HW_REVISION);
    lcd_puts(0, line++, buf);
#endif

    snprintf(buf, sizeof(buf), "PP version: %s", pp_version);
    lcd_puts(0, line++, buf);

    snprintf(buf, sizeof(buf), "Est. clock (kHz): %d", perfcheck());
    lcd_puts(0, line++, buf);

    lcd_update();

    while (!(action_userabort(TIMEOUT_BLOCK)));
    
#else
    /* Define this function in your target tree */
    return __dbg_hw_info();
#endif /* CONFIG_CPU */
    viewportmanager_set_statusbar(true);
    return false;
}
#else /* !HAVE_LCD_BITMAP */
static bool dbg_hw_info(void)
{
    char buf[32];
    int button;
    int currval = 0;
    int rom_version = ROM_VERSION;
    unsigned manu, id; /* flash IDs */
    bool got_id; /* flag if we managed to get the flash IDs */
    unsigned rom_crc = 0xffffffff; /* CRC32 of the boot ROM */
    bool has_bootrom; /* flag for boot ROM present */
    int oldmode;  /* saved memory guard mode */

    oldmode = system_memory_guard(MEMGUARD_NONE);  /* disable memory guard */

    /* get flash ROM type */
    got_id = dbg_flash_id(&manu, &id, 0x5555, 0x2AAA); /* try SST, Atmel, NexFlash */
    if (!got_id)
        got_id = dbg_flash_id(&manu, &id, 0x555, 0x2AA); /* try AMD, Macronix */

    /* check if the boot ROM area is a flash mirror */
    has_bootrom = (memcmp((char*)0, (char*)0x02000000, 64*1024) != 0);
    if (has_bootrom)  /* if ROM and Flash different */
    {
        /* calculate CRC16 checksum of boot ROM */
        rom_crc = crc_32((unsigned char*)0x0000, 64*1024, 0xffffffff);
    }

    system_memory_guard(oldmode);  /* re-enable memory guard */

    lcd_clear_display();

    lcd_puts(0, 0, "[HW Info]");
    while(1)
    {
        switch(currval)
        {
            case 0:
                snprintf(buf, 32, "ROM: %d.%02d",
                         rom_version/100, rom_version%100);
                break;
            case 1:
                if (got_id)
                    snprintf(buf, 32, "Flash:%02x,%02x", manu, id);
                else
                    snprintf(buf, 32, "Flash:??,??"); /* unknown, sorry */
                break;
            case 2:
                if (has_bootrom)
                {
                    if (rom_crc == 0x56DBA4EE) /* known Version 1 */
                        snprintf(buf, 32, "BootROM: V1");
                    else if (rom_crc == 0x358099E8)
                        snprintf(buf, 32, "BootROM: V2");
                        /* alternative boot ROM found in one single player so far */
                    else
                        snprintf(buf, 32, "R: %08x", rom_crc);
                }
                else
                    snprintf(buf, 32, "BootROM: no");
        }

        lcd_puts(0, 1, buf);
        lcd_update();

        button = get_action(CONTEXT_SETTINGS,TIMEOUT_BLOCK);

        switch(button)
        {
            case ACTION_STD_CANCEL:
                return false;

            case ACTION_SETTINGS_DEC:
                currval--;
                if(currval < 0)
                    currval = 2;
                break;

            case ACTION_SETTINGS_INC:
                currval++;
                if(currval > 2)
                    currval = 0;
                break;
        }
    }
    return false;
}
#endif /* !HAVE_LCD_BITMAP */
#endif /* !SIMULATOR */

#ifndef SIMULATOR
static char* dbg_partitions_getname(int selected_item, void *data,
                                    char *buffer, size_t buffer_len)
{
    (void)data;
    int partition = selected_item/2;
    struct partinfo* p = disk_partinfo(partition);
    if (selected_item%2)
    {
        snprintf(buffer, buffer_len, "   T:%x %ld MB", p->type, p->size / 2048);
    }
    else
    {
        snprintf(buffer, buffer_len, "P%d: S:%lx", partition, p->start);
    }
    return buffer;
}

bool dbg_partitions(void)
{
    struct simplelist_info info;
    simplelist_info_init(&info, "Partition Info", 4, NULL);
    info.selection_size = 2;
    info.hide_selection = true;
    info.scroll_all = true;
    info.get_name = dbg_partitions_getname;
    return simplelist_show_list(&info);
}
#endif

#if defined(CPU_COLDFIRE) && defined(HAVE_SPDIF_OUT)
static bool dbg_spdif(void)
{
    char buf[128];
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
    viewportmanager_set_statusbar(false);

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

        snprintf(buf, sizeof(buf), "Val: %s Sym: %s Par: %s",
                 valnogood?"--":"OK",
                 symbolerr?"--":"OK",
                 parityerr?"--":"OK");
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "Status word: %08x", (int)control);
        lcd_puts(0, line++, buf);

        line++;

        x = control >> 31;
        snprintf(buf, sizeof(buf), "PRO: %d (%s)",
                 x, x?"Professional":"Consumer");
        lcd_puts(0, line++, buf);

        x = (control >> 30) & 1;
        snprintf(buf, sizeof(buf), "Audio: %d (%s)",
                 x, x?"Non-PCM":"PCM");
        lcd_puts(0, line++, buf);

        x = (control >> 29) & 1;
        snprintf(buf, sizeof(buf), "Copy: %d (%s)",
                 x, x?"Permitted":"Inhibited");
        lcd_puts(0, line++, buf);

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
        snprintf(buf, sizeof(buf), "Preemphasis: %d (%s)", x, s);
        lcd_puts(0, line++, buf);

        x = (control >> 24) & 3;
        snprintf(buf, sizeof(buf), "Mode: %d", x);
        lcd_puts(0, line++, buf);

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
        snprintf(buf, sizeof(buf), "Category: 0x%02x (%s)", category, s);
        lcd_puts(0, line++, buf);

        x = (control >> 16) & 1;
        generation = x;
        if(((category & 0x70) == 0x10) ||
           ((category & 0x70) == 0x40) ||
           ((category & 0x78) == 0x38))
        {
            generation = !generation;
        }
        snprintf(buf, sizeof(buf), "Generation: %d (%s)",
                 x, generation?"Original":"No ind.");
        lcd_puts(0, line++, buf);

        x = (control >> 12) & 15;
        snprintf(buf, sizeof(buf), "Source: %d", x);
        lcd_puts(0, line++, buf);

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
        snprintf(buf, sizeof(buf), "Channel: %d (%s)", x, s);
        lcd_puts(0, line++, buf);

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
        snprintf(buf, sizeof(buf), "Frequency: %d (%s)", x, s);
        lcd_puts(0, line++, buf);

        x = (control >> 2) & 3;
        snprintf(buf, sizeof(buf), "Clock accuracy: %d", x);
        lcd_puts(0, line++, buf);
        line++;

#ifndef SIMULATOR
        snprintf(buf, sizeof(buf), "Measured freq: %ldHz",
                 spdif_measure_frequency());
        lcd_puts(0, line++, buf);
#endif

        lcd_update();

        if (action_userabort(HZ/10))
            break;
    }

    spdif_set_output_source(spdif_source IF_SPDIF_POWER_(, spdif_src_on));

#ifdef HAVE_SPDIF_POWER
    spdif_power_enable(global_settings.spdif_enable);
#endif

    viewportmanager_set_statusbar(true);
    return false;
}
#endif /* CPU_COLDFIRE */

#ifndef SIMULATOR
#ifdef HAVE_LCD_BITMAP
 /* button definitions */
#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
   (CONFIG_KEYPAD == IRIVER_H300_PAD)
#   define DEBUG_CANCEL  BUTTON_OFF

#elif CONFIG_KEYPAD == RECORDER_PAD
#   define DEBUG_CANCEL  BUTTON_OFF

#elif CONFIG_KEYPAD == ONDIO_PAD
#   define DEBUG_CANCEL  BUTTON_MENU

#elif (CONFIG_KEYPAD == IPOD_1G2G_PAD) || \
    (CONFIG_KEYPAD == IPOD_3G_PAD) || \
    (CONFIG_KEYPAD == IPOD_4G_PAD)
#   define DEBUG_CANCEL  BUTTON_MENU

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
#   define DEBUG_CANCEL  BUTTON_PLAY

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#   define DEBUG_CANCEL  BUTTON_REC

#elif (CONFIG_KEYPAD == IAUDIO_M3_PAD)
#   define DEBUG_CANCEL  BUTTON_RC_REC

#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
#   define DEBUG_CANCEL  BUTTON_REW

#elif (CONFIG_KEYPAD == MROBE100_PAD)
#   define DEBUG_CANCEL  BUTTON_MENU

#elif (CONFIG_KEYPAD == SANSA_E200_PAD) || \
      (CONFIG_KEYPAD == SANSA_C200_PAD)
#   define DEBUG_CANCEL  BUTTON_LEFT

/* This is temporary until the SA9200 touchpad works */
#elif (CONFIG_KEYPAD == PHILIPS_SA9200_PAD) || \
      (CONFIG_KEYPAD == PHILIPS_HDD1630_PAD)
#   define DEBUG_CANCEL  BUTTON_POWER

#endif /* key definitions */

/* Test code!!! */
bool dbg_ports(void)
{
#if CONFIG_CPU == SH7034
    char buf[32];
    int adc_battery_voltage, adc_battery_level;

    lcd_setfont(FONT_SYSFIXED);
    lcd_clear_display();
    viewportmanager_set_statusbar(false);

    while(1)
    {
        snprintf(buf, 32, "PADR: %04x", (unsigned short)PADR);
        lcd_puts(0, 0, buf);
        snprintf(buf, 32, "PBDR: %04x", (unsigned short)PBDR);
        lcd_puts(0, 1, buf);

        snprintf(buf, 32, "AN0: %03x AN4: %03x", adc_read(0), adc_read(4));
        lcd_puts(0, 2, buf);
        snprintf(buf, 32, "AN1: %03x AN5: %03x", adc_read(1), adc_read(5));
        lcd_puts(0, 3, buf);
        snprintf(buf, 32, "AN2: %03x AN6: %03x", adc_read(2), adc_read(6));
        lcd_puts(0, 4, buf);
        snprintf(buf, 32, "AN3: %03x AN7: %03x", adc_read(3), adc_read(7));
        lcd_puts(0, 5, buf);

        battery_read_info(&adc_battery_voltage, &adc_battery_level);
        snprintf(buf, 32, "Batt: %d.%03dV %d%%  ", adc_battery_voltage / 1000,
                 adc_battery_voltage % 1000, adc_battery_level);
        lcd_puts(0, 6, buf);

        lcd_update();
        if (button_get_w_tmo(HZ/10) == (DEBUG_CANCEL|BUTTON_REL))
        {
            viewportmanager_set_statusbar(true);
            return false;
        }
    }
#elif defined(CPU_COLDFIRE)
    unsigned int gpio_out;
    unsigned int gpio1_out;
    unsigned int gpio_read;
    unsigned int gpio1_read;
    unsigned int gpio_function;
    unsigned int gpio1_function;
    unsigned int gpio_enable;
    unsigned int gpio1_enable;
    int adc_buttons, adc_remote;
    int adc_battery_voltage, adc_battery_level;
    char buf[128];
    int line;

    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);
    viewportmanager_set_statusbar(false);

    while(1)
    {
        line = 0;
        gpio_read = GPIO_READ;
        gpio1_read = GPIO1_READ;
        gpio_out = GPIO_OUT;
        gpio1_out = GPIO1_OUT;
        gpio_function = GPIO_FUNCTION;
        gpio1_function = GPIO1_FUNCTION;
        gpio_enable = GPIO_ENABLE;
        gpio1_enable = GPIO1_ENABLE;

        snprintf(buf, sizeof(buf), "GPIO_READ: %08x", gpio_read);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO_OUT:  %08x", gpio_out);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO_FUNC: %08x", gpio_function);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO_ENA:  %08x", gpio_enable);
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "GPIO1_READ: %08x", gpio1_read);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO1_OUT:  %08x", gpio1_out);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO1_FUNC: %08x", gpio1_function);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO1_ENA:  %08x", gpio1_enable);
        lcd_puts(0, line++, buf);

        adc_buttons = adc_read(ADC_BUTTONS);
        adc_remote  = adc_read(ADC_REMOTE);
        battery_read_info(&adc_battery_voltage, &adc_battery_level);
#if defined(IAUDIO_X5) ||  defined(IAUDIO_M5) || defined(IRIVER_H300_SERIES)
        snprintf(buf, sizeof(buf), "ADC_BUTTONS (%c): %02x",
            button_scan_enabled() ? '+' : '-', adc_buttons);
#else
        snprintf(buf, sizeof(buf), "ADC_BUTTONS: %02x", adc_buttons);
#endif
        lcd_puts(0, line++, buf);
#if defined(IAUDIO_X5) || defined(IAUDIO_M5)
        snprintf(buf, sizeof(buf), "ADC_REMOTE  (%c): %02x",
            remote_detect() ? '+' : '-', adc_remote);
#else
        snprintf(buf, sizeof(buf), "ADC_REMOTE:  %02x", adc_remote);
#endif
        lcd_puts(0, line++, buf);
#if defined(IRIVER_H100_SERIES) || defined(IRIVER_H300_SERIES)
        snprintf(buf, sizeof(buf), "ADC_REMOTEDETECT: %02x",
                 adc_read(ADC_REMOTEDETECT));
        lcd_puts(0, line++, buf);
#endif

        snprintf(buf, 32, "Batt: %d.%03dV %d%%  ", adc_battery_voltage / 1000,
                 adc_battery_voltage % 1000, adc_battery_level);
        lcd_puts(0, line++, buf);

#if defined(IRIVER_H100_SERIES) || defined(IRIVER_H300_SERIES)
        snprintf(buf, sizeof(buf), "remotetype: %d", remote_type());
        lcd_puts(0, line++, buf);
#endif

        lcd_update();
        if (button_get_w_tmo(HZ/10) == (DEBUG_CANCEL|BUTTON_REL))
        {
            viewportmanager_set_statusbar(true);
            return false;
        }
    }

#elif defined(CPU_PP502x)

    char buf[128];
    int line;

    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);
    viewportmanager_set_statusbar(false);

    while(1)
    {
        line = 0;
        lcd_puts(0, line++, "GPIO STATES:");
        snprintf(buf, sizeof(buf), "A: %02x  E: %02x  I: %02x",
                                   (unsigned int)GPIOA_INPUT_VAL,
                                   (unsigned int)GPIOE_INPUT_VAL,
                                   (unsigned int)GPIOI_INPUT_VAL);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "B: %02x  F: %02x  J: %02x",
                                   (unsigned int)GPIOB_INPUT_VAL,
                                   (unsigned int)GPIOF_INPUT_VAL,
                                   (unsigned int)GPIOJ_INPUT_VAL);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "C: %02x  G: %02x  K: %02x",
                                   (unsigned int)GPIOC_INPUT_VAL,
                                   (unsigned int)GPIOG_INPUT_VAL,
                                   (unsigned int)GPIOK_INPUT_VAL);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "D: %02x  H: %02x  L: %02x",
                                   (unsigned int)GPIOD_INPUT_VAL,
                                   (unsigned int)GPIOH_INPUT_VAL,
                                   (unsigned int)GPIOL_INPUT_VAL);
        lcd_puts(0, line++, buf);
        line++;
        snprintf(buf, sizeof(buf), "GPO32_VAL: %08lx", GPO32_VAL);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPO32_EN:  %08lx", GPO32_ENABLE);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "DEV_EN:    %08lx", DEV_EN);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "DEV_EN2:   %08lx", DEV_EN2);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "DEV_EN3:   %08lx", inl(0x60006044));
        lcd_puts(0, line++, buf);                    /* to be verified */
        snprintf(buf, sizeof(buf), "DEV_INIT1: %08lx", DEV_INIT1);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "DEV_INIT2: %08lx", DEV_INIT2);
        lcd_puts(0, line++, buf);
#ifdef ADC_ACCESSORY
        snprintf(buf, sizeof(buf), "ACCESSORY: %d", adc_read(ADC_ACCESSORY));
        lcd_puts(0, line++, buf);
#endif

#if defined(IPOD_ACCESSORY_PROTOCOL)
extern unsigned char serbuf[];
        snprintf(buf, sizeof(buf), "IAP PACKET: %02x %02x %02x %02x %02x %02x %02x %02x", 
		 serbuf[0], serbuf[1], serbuf[2], serbuf[3], serbuf[4], serbuf[5],
		 serbuf[6], serbuf[7]);
	lcd_puts(0, line++, buf);
#endif

#if defined(IRIVER_H10) || defined(IRIVER_H10_5GB)
        line++;
        snprintf(buf, sizeof(buf), "BATT: %03x UNK1: %03x",
                                adc_read(ADC_BATTERY), adc_read(ADC_UNKNOWN_1));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "REM:  %03x PAD: %03x",
                                 adc_read(ADC_REMOTE), adc_read(ADC_SCROLLPAD));
        lcd_puts(0, line++, buf);
#elif defined(SANSA_E200) || defined(PHILIPS_SA9200)
        snprintf(buf, sizeof(buf), "ADC_BVDD:     %4d", adc_read(ADC_BVDD));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "ADC_RTCSUP:   %4d", adc_read(ADC_RTCSUP));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "ADC_UVDD:     %4d", adc_read(ADC_UVDD));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "ADC_CHG_IN:   %4d", adc_read(ADC_CHG_IN));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "ADC_CVDD:     %4d", adc_read(ADC_CVDD));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "ADC_BATTEMP:  %4d", adc_read(ADC_BATTEMP));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "ADC_MICSUP1:  %4d", adc_read(ADC_MICSUP1));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "ADC_MICSUP2:  %4d", adc_read(ADC_MICSUP2));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "ADC_VBE1:     %4d", adc_read(ADC_VBE1));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "ADC_VBE2:     %4d", adc_read(ADC_VBE2));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "ADC_I_MICSUP1:%4d", adc_read(ADC_I_MICSUP1));
        lcd_puts(0, line++, buf);
#if !defined(PHILIPS_SA9200)
        snprintf(buf, sizeof(buf), "ADC_I_MICSUP2:%4d", adc_read(ADC_I_MICSUP2));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "ADC_VBAT:     %4d", adc_read(ADC_VBAT));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "CHARGER: %02X/%02X", 
                                   ascodec_read(AS3514_CHARGER), 
                                   ascodec_read(AS3514_IRQ_ENRD0));
        lcd_puts(0, line++, buf);
#endif
#endif
        lcd_update();
        if (button_get_w_tmo(HZ/10) == (DEBUG_CANCEL|BUTTON_REL))
        {
            viewportmanager_set_statusbar(true);
            return false;
        }
    }

#elif CONFIG_CPU == PP5002
    char buf[128];
    int line;

    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);
    viewportmanager_set_statusbar(false);

    while(1)
    {
        line = 0;
        snprintf(buf, sizeof(buf), "GPIO_A: %02x GPIO_B: %02x",
                 (unsigned int)GPIOA_INPUT_VAL, (unsigned int)GPIOB_INPUT_VAL);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO_C: %02x GPIO_D: %02x",
                 (unsigned int)GPIOC_INPUT_VAL, (unsigned int)GPIOD_INPUT_VAL);
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "DEV_EN:       %08lx", DEV_EN);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "CLOCK_ENABLE: %08lx", CLOCK_ENABLE);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "CLOCK_SOURCE: %08lx", CLOCK_SOURCE);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "PLL_CONTROL:  %08lx", PLL_CONTROL);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "PLL_DIV:      %08lx", PLL_DIV);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "PLL_MULT:     %08lx", PLL_MULT);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "TIMING1_CTL:  %08lx", TIMING1_CTL);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "TIMING2_CTL:  %08lx", TIMING2_CTL);
        lcd_puts(0, line++, buf);

        lcd_update();
        if (button_get_w_tmo(HZ/10) == (DEBUG_CANCEL|BUTTON_REL))
        {
            viewportmanager_set_statusbar(true);
            return false;
        }
    }
    viewportmanager_set_statusbar(true);
#else
    return __dbg_ports();
#endif /* CPU */
    return false;
}
#else /* !HAVE_LCD_BITMAP */
bool dbg_ports(void)
{
    char buf[32];
    int button;
    int adc_battery_voltage;
    int currval = 0;

    lcd_clear_display();
    viewportmanager_set_statusbar(false);

    while(1)
    {
        switch(currval)
        {
        case 0:
            snprintf(buf, 32, "PADR: %04x", (unsigned short)PADR);
            break;
        case 1:
            snprintf(buf, 32, "PBDR: %04x", (unsigned short)PBDR);
            break;
        case 2:
            snprintf(buf, 32, "AN0: %03x", adc_read(0));
            break;
        case 3:
            snprintf(buf, 32, "AN1: %03x", adc_read(1));
            break;
        case 4:
            snprintf(buf, 32, "AN2: %03x", adc_read(2));
            break;
        case 5:
            snprintf(buf, 32, "AN3: %03x", adc_read(3));
            break;
        case 6:
            snprintf(buf, 32, "AN4: %03x", adc_read(4));
            break;
        case 7:
            snprintf(buf, 32, "AN5: %03x", adc_read(5));
            break;
        case 8:
            snprintf(buf, 32, "AN6: %03x", adc_read(6));
            break;
        case 9:
            snprintf(buf, 32, "AN7: %03x", adc_read(7));
            break;
        }
        lcd_puts(0, 0, buf);

        battery_read_info(&adc_battery_voltage, NULL);
        snprintf(buf, 32, "Batt: %d.%03dV", adc_battery_voltage / 1000,
                 adc_battery_voltage % 1000);
        lcd_puts(0, 1, buf);
        lcd_update();

        button = get_action(CONTEXT_SETTINGS,HZ/5);

        switch(button)
        {
            case ACTION_STD_CANCEL:
            return false;

        case ACTION_SETTINGS_DEC:
            currval--;
            if(currval < 0)
                currval = 9;
            break;

        case ACTION_SETTINGS_INC:
            currval++;
            if(currval > 9)
                currval = 0;
            break;
        }
    }
    viewportmanager_set_statusbar(true);
    return false;
}
#endif /* !HAVE_LCD_BITMAP */
#endif /* !SIMULATOR */

#if (CONFIG_RTC == RTC_PCF50605) && !defined(SIMULATOR)
static bool dbg_pcf(void)
{
    char buf[128];
    int line;

#ifdef HAVE_LCD_BITMAP
    lcd_setfont(FONT_SYSFIXED);
#endif
    lcd_clear_display();
    viewportmanager_set_statusbar(false);

    while(1)
    {
        line = 0;

        snprintf(buf, sizeof(buf), "DCDC1:  %02x", pcf50605_read(0x1b));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "DCDC2:  %02x", pcf50605_read(0x1c));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "DCDC3:  %02x", pcf50605_read(0x1d));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "DCDC4:  %02x", pcf50605_read(0x1e));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "DCDEC1: %02x", pcf50605_read(0x1f));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "DCDEC2: %02x", pcf50605_read(0x20));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "DCUDC1: %02x", pcf50605_read(0x21));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "DCUDC2: %02x", pcf50605_read(0x22));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "IOREGC: %02x", pcf50605_read(0x23));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "D1REGC: %02x", pcf50605_read(0x24));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "D2REGC: %02x", pcf50605_read(0x25));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "D3REGC: %02x", pcf50605_read(0x26));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "LPREG1: %02x", pcf50605_read(0x27));
        lcd_puts(0, line++, buf);

        lcd_update();
        if (button_get_w_tmo(HZ/10) == (DEBUG_CANCEL|BUTTON_REL))
        {
            viewportmanager_set_statusbar(true);
            return false;
        }
    }

    viewportmanager_set_statusbar(true);
    return false;
}
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
static bool dbg_cpufreq(void)
{
    char buf[128];
    int line;
    int button;

#ifdef HAVE_LCD_BITMAP
    lcd_setfont(FONT_SYSFIXED);
#endif
    lcd_clear_display();
    viewportmanager_set_statusbar(false);

    while(1)
    {
        line = 0;

        snprintf(buf, sizeof(buf), "Frequency: %ld", FREQ);
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "boost_counter: %d", get_cpu_boost_counter());
        lcd_puts(0, line++, buf);

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

            case ACTION_STD_OK:
                while (get_cpu_boost_counter() > 0)
                    cpu_boost(false);
                set_cpu_frequency(CPUFREQ_DEFAULT);
                break;

            case ACTION_STD_CANCEL:
                viewportmanager_set_statusbar(true);
                return false;
        }
    }
    viewportmanager_set_statusbar(true);
    return false;
}
#endif /* HAVE_ADJUSTABLE_CPU_FREQ */

#if defined(HAVE_TSC2100) && !defined(SIMULATOR)
#include "tsc2100.h"
static char *itob(int n, int len)
{
    static char binary[64];
    int i,j;
    for (i=1, j=0;i<=len;i++)
    {
        binary[j++] = n&(1<<(len-i))?'1':'0';
        if (i%4 == 0)
            binary[j++] = ' ';
    }
    binary[j] = '\0';
    return binary;
}
static char* tsc2100_debug_getname(int selected_item, void * data,
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
        snprintf(buffer, buffer_len, "%02x: RESERVED", selected_item);
    else
        snprintf(buffer, buffer_len, "%02x: %s", selected_item,
                    itob(tsc2100_readreg(*page, selected_item)&0xffff,16));
    return buffer;
}
static int tsc2100debug_action_callback(int action, struct gui_synclist *lists)
{
    int *page = (int*)lists->data;
    if (action == ACTION_STD_OK)
    {
        *page = (*page+1)%3;
        snprintf(lists->title, 32,
                 "tsc2100 registers - Page %d", *page);
        return ACTION_REDRAW;
    }
    return action;
}
static bool tsc2100_debug(void)
{
    int page = 0;
    char title[32] = "tsc2100 registers - Page 0";
    struct simplelist_info info;
    simplelist_info_init(&info, title, 32, &page);
    info.timeout = HZ/100;
    info.get_name = tsc2100_debug_getname;
    info.action_callback= tsc2100debug_action_callback;
    return simplelist_show_list(&info);
}
#endif
#ifndef SIMULATOR
#ifdef HAVE_LCD_BITMAP
/*
 * view_battery() shows a automatically scaled graph of the battery voltage
 * over time. Usable for estimating battery life / charging rate.
 * The power_history array is updated in power_thread of powermgmt.c.
 */

#define BAT_LAST_VAL  MIN(LCD_WIDTH, POWER_HISTORY_LEN)
#define BAT_YSPACE    (LCD_HEIGHT - 20)

static bool view_battery(void)
{
    int view = 0;
    int i, x, y;
    unsigned short maxv, minv;
    char buf[32];

    lcd_setfont(FONT_SYSFIXED);
    viewportmanager_set_statusbar(false);

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

                snprintf(buf, 30, "Battery %d.%03d", power_history[0] / 1000,
                         power_history[0] % 1000);
                lcd_puts(0, 0, buf);
                snprintf(buf, 30, "scale %d.%03d-%d.%03dV",
                         minv / 1000, minv % 1000, maxv / 1000, maxv % 1000);
                lcd_puts(0, 1, buf);

                x = 0;
                for (i = BAT_LAST_VAL - 1; i >= 0; i--) {
                    y = (power_history[i] - minv) * BAT_YSPACE / (maxv - minv);
                    lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                    lcd_vline(x, LCD_HEIGHT-1, 20);
                    lcd_set_drawmode(DRMODE_SOLID);
                    lcd_vline(x, LCD_HEIGHT-1,
                              MIN(MAX(LCD_HEIGHT-1 - y, 20), LCD_HEIGHT-1));
                    x++;
                }

                break;

            case 1: /* status: */
                lcd_puts(0, 0, "Power status:");

                battery_read_info(&y, NULL);
                snprintf(buf, 30, "Battery: %d.%03d V", y / 1000, y % 1000);
                lcd_puts(0, 1, buf);
#ifdef ADC_EXT_POWER
                y = (adc_read(ADC_EXT_POWER) * EXT_SCALE_FACTOR) / 1000;
                snprintf(buf, 30, "External: %d.%03d V", y / 1000, y % 1000);
                lcd_puts(0, 2, buf);
#endif
#if CONFIG_CHARGING
#if defined ARCHOS_RECORDER
                snprintf(buf, 30, "Chgr: %s %s",
                         charger_inserted() ? "present" : "absent",
                         charger_enabled() ? "on" : "off");
                lcd_puts(0, 3, buf);
                snprintf(buf, 30, "short delta: %d", short_delta);
                lcd_puts(0, 5, buf);
                snprintf(buf, 30, "long delta: %d", long_delta);
                lcd_puts(0, 6, buf);
                lcd_puts(0, 7, power_message);
                snprintf(buf, 30, "USB Inserted: %s",
                         usb_inserted() ? "yes" : "no");
                lcd_puts(0, 8, buf);
#elif defined IRIVER_H300_SERIES
                snprintf(buf, 30, "USB Charging Enabled: %s",
                         usb_charging_enabled() ? "yes" : "no");
                lcd_puts(0, 9, buf);
#elif defined IPOD_NANO || defined IPOD_VIDEO
                int usb_pwr  = (GPIOL_INPUT_VAL & 0x10)?true:false;
                int ext_pwr  = (GPIOL_INPUT_VAL & 0x08)?false:true;
                int dock     = (GPIOA_INPUT_VAL & 0x10)?true:false;
                int charging = (GPIOB_INPUT_VAL & 0x01)?false:true;
                int headphone= (GPIOA_INPUT_VAL & 0x80)?true:false;

                snprintf(buf, 30, "USB pwr:   %s",
                         usb_pwr ? "present" : "absent");
                lcd_puts(0, 3, buf);
                snprintf(buf, 30, "EXT pwr:   %s",
                         ext_pwr ? "present" : "absent");
                lcd_puts(0, 4, buf);
                snprintf(buf, 30, "Battery:   %s",
                    charging ? "charging" : (usb_pwr||ext_pwr) ? "charged" : "discharging");
                lcd_puts(0, 5, buf);
                snprintf(buf, 30, "Dock mode: %s",
                         dock    ? "enabled" : "disabled");
                lcd_puts(0, 6, buf);
                snprintf(buf, 30, "Headphone: %s",
                         headphone ? "connected" : "disconnected");
                lcd_puts(0, 7, buf);
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

                snprintf(buf, 30, "Charger: %s",
                         charger_inserted() ? "present" : "absent");
                lcd_puts(0, line++, buf);

                st = power_input_status() &
                     (POWER_INPUT_CHARGER | POWER_INPUT_BATTERY);
                snprintf(buf, 30, "%s%s",
                         (st & POWER_INPUT_MAIN_CHARGER) ? " Main" : "",
                         (st & POWER_INPUT_USB_CHARGER) ? " USB" : "");
                lcd_puts(0, line++, buf);

                snprintf(buf, 30, "IUSB Max: %d", usb_allowed_current());
                lcd_puts(0, line++, buf);

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

                snprintf(buf, 30, "State: %s", chrgstate_strings[y]);
                lcd_puts(0, line++, buf);

                snprintf(buf, 30, "Battery Switch: %s",
                         (st & POWER_INPUT_BATTERY) ? "On" : "Off");
                lcd_puts(0, line++, buf);

                y = chrgraw_adc_voltage();
                snprintf(buf, 30, "CHRGRAW: %d.%03d V",
                         y / 1000, y % 1000);
                lcd_puts(0, line++, buf);

                y = application_supply_adc_voltage();
                snprintf(buf, 30, "BP     : %d.%03d V",
                         y / 1000, y % 1000);
                lcd_puts(0, line++, buf);

                y = battery_adc_charge_current();
                if (y < 0) x = '-', y = -y;
                else       x = ' ';
                snprintf(buf, 30, "CHRGISN:%c%d mA", x, y);
                lcd_puts(0, line++, buf);

                y = cccv_regulator_dissipation();
                snprintf(buf, 30, "P CCCV : %d mW", y);
                lcd_puts(0, line++, buf);

                y = battery_charge_current();
                if (y < 0) x = '-', y = -y;
                else       x = ' ';
                snprintf(buf, 30, "I Charge:%c%d mA", x, y);
                lcd_puts(0, line++, buf);

                y = battery_adc_temp();

                if (y != INT_MIN) {
                    snprintf(buf, 30, "T Battery: %dC (%dF)", y,
                             (9*y + 160) / 5);
                } else {
                    /* Conversion disabled */
                    snprintf(buf, 30, "T Battery: ?");
                }
                    
                lcd_puts(0, line++, buf);
#else
                snprintf(buf, 30, "Charger: %s",
                         charger_inserted() ? "present" : "absent");
                lcd_puts(0, 3, buf);
#endif /* target type */
#endif /* CONFIG_CHARGING */
                break;

            case 2: /* voltage deltas: */
                lcd_puts(0, 0, "Voltage deltas:");

                for (i = 0; i <= 6; i++) {
                    y = power_history[i] - power_history[i+1];
                    snprintf(buf, 30, "-%d min: %s%d.%03d V", i,
                             (y < 0) ? "-" : "", ((y < 0) ? y * -1 : y) / 1000,
                             ((y < 0) ? y * -1 : y ) % 1000);
                    lcd_puts(0, i+1, buf);
                }
                break;

            case 3: /* remaining time estimation: */

#ifdef ARCHOS_RECORDER
                snprintf(buf, 30, "charge_state: %d", charge_state);
                lcd_puts(0, 0, buf);

                snprintf(buf, 30, "Cycle time: %d m", powermgmt_last_cycle_startstop_min);
                lcd_puts(0, 1, buf);

                snprintf(buf, 30, "Lvl@cyc st: %d%%", powermgmt_last_cycle_level);
                lcd_puts(0, 2, buf);

                snprintf(buf, 30, "P=%2d I=%2d", pid_p, pid_i);
                lcd_puts(0, 3, buf);

                snprintf(buf, 30, "Trickle sec: %d/60", trickle_sec);
                lcd_puts(0, 4, buf);
#endif /* ARCHOS_RECORDER */

                snprintf(buf, 30, "Last PwrHist: %d.%03dV",
                    power_history[0] / 1000,
                    power_history[0] % 1000);
                lcd_puts(0, 5, buf);

                snprintf(buf, 30, "battery level: %d%%", battery_level());
                lcd_puts(0, 6, buf);

                snprintf(buf, 30, "Est. remain: %d m", battery_time());
                lcd_puts(0, 7, buf);
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
                viewportmanager_set_statusbar(true);
                return false;
        }
    }
    viewportmanager_set_statusbar(true);
    return false;
}

#endif /* HAVE_LCD_BITMAP */
#endif

#ifndef SIMULATOR
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
    unsigned char card_name[7];
    unsigned char pbuf[32];
    char *title = lists->title;
    static const unsigned char i_vmin[] = { 0, 1, 5, 10, 25, 35, 60, 100 };
    static const unsigned char i_vmax[] = { 1, 5, 10, 25, 35, 45, 80, 200 };
    static const unsigned char *kbit_units[] = { "kBit/s", "MBit/s", "GBit/s" };
    static const unsigned char *nsec_units[] = { "ns", "µs", "ms" };
    static const char *spec_vers[] = { "1.0-1.2", "1.4", "2.0-2.2",
        "3.1-3.31", "4.0" };
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
            card_name[6] = '\0';
            strncpy(card_name, ((unsigned char*)card->cid) + 3, 6);
            simplelist_addline(SIMPLELIST_ADD_LINE,
                    "%s Rev %d.%d", card_name,
                    (int) card_extract_bits(card->cid, 72, 4),
                    (int) card_extract_bits(card->cid, 76, 4));
            simplelist_addline(SIMPLELIST_ADD_LINE,
                    "Prod: %d/%d",
                    (int) card_extract_bits(card->cid, 112, 4),
                    (int) card_extract_bits(card->cid, 116, 4) + 1997);
            simplelist_addline(SIMPLELIST_ADD_LINE,
                    "Ser#: 0x%08lx",
                    card_extract_bits(card->cid, 80, 32));
            simplelist_addline(SIMPLELIST_ADD_LINE,
                    "M=%02x, O=%04x",
                    (int) card_extract_bits(card->cid, 0, 8),
                    (int) card_extract_bits(card->cid, 8, 16));
            int temp = card_extract_bits(card->csd, 2, 4);
            simplelist_addline(SIMPLELIST_ADD_LINE,
                     CARDTYPE " v%s", temp < 5 ?
                            spec_vers[temp] : "?.?");
            simplelist_addline(SIMPLELIST_ADD_LINE,
                    "Blocks: 0x%08lx", card->numblocks);
            output_dyn_value(pbuf, sizeof pbuf, card->speed / 1000,
                                            kbit_units, false);
            simplelist_addline(SIMPLELIST_ADD_LINE,
                    "Speed: %s", pbuf);
            output_dyn_value(pbuf, sizeof pbuf, card->tsac,
                            nsec_units, false);
            simplelist_addline(SIMPLELIST_ADD_LINE,
                    "Tsac: %s", pbuf);
            simplelist_addline(SIMPLELIST_ADD_LINE,
                    "Nsac: %d clk", card->nsac);
            simplelist_addline(SIMPLELIST_ADD_LINE,
                    "R2W: *%d", card->r2w_factor);
            simplelist_addline(SIMPLELIST_ADD_LINE,
                    "IRmax: %d..%d mA",
                    i_vmin[card_extract_bits(card->csd, 66, 3)],
                    i_vmax[card_extract_bits(card->csd, 69, 3)]);
            simplelist_addline(SIMPLELIST_ADD_LINE,
                    "IWmax: %d..%d mA",
                    i_vmin[card_extract_bits(card->csd, 72, 3)],
                    i_vmax[card_extract_bits(card->csd, 75, 3)]);
        }
        else if (card->initialized == 0)
        {
            simplelist_addline(SIMPLELIST_ADD_LINE, "Not Found!");
        }
#if (CONFIG_STORAGE & STORAGE_SD)
        else /* card->initialized < 0 */
        {
            simplelist_addline(SIMPLELIST_ADD_LINE, "Init Error! (%d)", card->initialized);
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
    simplelist_addline(SIMPLELIST_ADD_LINE, "Model: %s", buf);
    for (i=0; i < 4; i++)
        ((unsigned short*)buf)[i]=htobe16(identify_info[i+23]);
    buf[8]=0;
    simplelist_addline(SIMPLELIST_ADD_LINE,
             "Firmware: %s", buf);
    snprintf(buf, sizeof buf, "%ld MB",
             ((unsigned long)identify_info[61] << 16 |
              (unsigned long)identify_info[60]) / 2048 );
    simplelist_addline(SIMPLELIST_ADD_LINE,
             "Size: %s", buf);
    unsigned long free;
    fat_size( IF_MV2(0,) NULL, &free );
    simplelist_addline(SIMPLELIST_ADD_LINE,
             "Free: %ld MB", free / 1024);
    simplelist_addline(SIMPLELIST_ADD_LINE,
             "Spinup time: %d ms", storage_spinup_time() * (1000/HZ));
    i = identify_info[83] & (1<<3);
    simplelist_addline(SIMPLELIST_ADD_LINE,
             "Power mgmt: %s", i ? "enabled" : "unsupported");
    i = identify_info[83] & (1<<9);
    simplelist_addline(SIMPLELIST_ADD_LINE,
             "Noise mgmt: %s", i ? "enabled" : "unsupported");
    i = identify_info[82] & (1<<6);
    simplelist_addline(SIMPLELIST_ADD_LINE,
             "Read-ahead: %s", i ? "enabled" : "unsupported");
    timing_info_present = identify_info[53] & (1<<1);
    if(timing_info_present) {
        char pio3[2], pio4[2];pio3[1] = 0;
        pio4[1] = 0;
        pio3[0] = (identify_info[64] & (1<<0)) ? '3' : 0;
        pio4[0] = (identify_info[64] & (1<<1)) ? '4' : 0;
        simplelist_addline(SIMPLELIST_ADD_LINE,
                 "PIO modes: 0 1 2 %s %s", pio3, pio4);
    }
    else {
        simplelist_addline(SIMPLELIST_ADD_LINE,
                 "No PIO mode info");
    }
    timing_info_present = identify_info[53] & (1<<1);
    if(timing_info_present) {
        simplelist_addline(SIMPLELIST_ADD_LINE,
                 "Cycle times %dns/%dns",
                 identify_info[67],
                 identify_info[68] );
    } else {
        simplelist_addline(SIMPLELIST_ADD_LINE,
                 "No timing info");
    }
#if defined (TOSHIBA_GIGABEAT_F) || defined (TOSHIBA_GIGABEAT_S)
    if (identify_info[63] & (1<<0)) {
        char mdma0[2], mdma1[2], mdma2[2];
        mdma0[1] = mdma1[1] = mdma2[1] = 0;
        mdma0[0] = (identify_info[63] & (1<<0)) ? '0' : 0;
        mdma1[0] = (identify_info[63] & (1<<1)) ? '1' : 0;
        mdma2[0] = (identify_info[63] & (1<<2)) ? '2' : 0;
        simplelist_addline(SIMPLELIST_ADD_LINE,
                 "MDMA modes: %s %s %s", mdma0, mdma1, mdma2);
        simplelist_addline(SIMPLELIST_ADD_LINE,
                 "MDMA Cycle times %dns/%dns",
                 identify_info[65],
                 identify_info[66] );
    }
    else {
        simplelist_addline(SIMPLELIST_ADD_LINE,
                "No MDMA mode info");
    }
    if (identify_info[88] & (1<<0)) {
        char udma0[2], udma1[2], udma2[2], udma3[2], udma4[2], udma5[2];
        udma0[1] = udma1[1] = udma2[1] = udma3[1] = udma4[1] = udma5[1] = 0;
        udma0[0] = (identify_info[88] & (1<<0)) ? '0' : 0;
        udma1[0] = (identify_info[88] & (1<<1)) ? '1' : 0;
        udma2[0] = (identify_info[88] & (1<<2)) ? '2' : 0;
        udma3[0] = (identify_info[88] & (1<<3)) ? '3' : 0;
        udma4[0] = (identify_info[88] & (1<<4)) ? '4' : 0;
        udma5[0] = (identify_info[88] & (1<<5)) ? '5' : 0;
        simplelist_addline(SIMPLELIST_ADD_LINE,
                 "UDMA modes: %s %s %s %s %s %s", udma0, udma1, udma2,
                                                  udma3, udma4, udma5);
    }
    else {
        simplelist_addline(SIMPLELIST_ADD_LINE,
                "No UDMA mode info");
    }
#endif /* defined (TOSHIBA_GIGABEAT_F) || defined (TOSHIBA_GIGABEAT_S) */
    timing_info_present = identify_info[53] & (1<<1);
    if(timing_info_present) {
        i = identify_info[49] & (1<<11);
        simplelist_addline(SIMPLELIST_ADD_LINE,
            "IORDY support: %s", i ? "yes" : "no");
        i = identify_info[49] & (1<<10);
        simplelist_addline(SIMPLELIST_ADD_LINE,
                "IORDY disable: %s", i ? "yes" : "no");
    } else {
        simplelist_addline(SIMPLELIST_ADD_LINE,
                "No timing info");
    }
    simplelist_addline(SIMPLELIST_ADD_LINE,
             "Cluster size: %d bytes", fat_get_cluster_size(IF_MV(0)));
    return btn;
}
#else /* No SD, MMC or ATA */
static int disk_callback(int btn, struct gui_synclist *lists)
{
    (void)btn;
    (void)lists;
    struct storage_info info;
    storage_get_info(0,&info);
    simplelist_addline(SIMPLELIST_ADD_LINE, "Vendor: %s", info.vendor);
    simplelist_addline(SIMPLELIST_ADD_LINE, "Model: %s", info.product);
    simplelist_addline(SIMPLELIST_ADD_LINE, "Firmware: %s", info.revision);
    simplelist_addline(SIMPLELIST_ADD_LINE,
             "Size: %ld MB", info.num_sectors*(info.sector_size/512)/2024);
    unsigned long free;
    fat_size( IF_MV2(0,) NULL, &free );
    simplelist_addline(SIMPLELIST_ADD_LINE,
             "Free: %ld MB", free / 1024);
    simplelist_addline(SIMPLELIST_ADD_LINE,
             "Cluster size: %d bytes", fat_get_cluster_size(IF_MV(0)));
    return btn;
}
#endif

#if  (CONFIG_STORAGE & STORAGE_ATA) 
static bool dbg_identify_info(void)
{
    int fd = creat("/identify_info.bin");
    if(fd >= 0)
    {
#ifdef ROCKBOX_LITTLE_ENDIAN
        ecwrite(fd, ata_get_identify(), SECTOR_SIZE/2, "s", true);
#else
        write(fd, ata_get_identify(), SECTOR_SIZE);
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
    info.hide_selection = true;
    info.scroll_all = true;
    return simplelist_show_list(&info);
}
#endif /* !SIMULATOR */

#ifdef HAVE_DIRCACHE
static int dircache_callback(int btn, struct gui_synclist *lists)
{
    (void)btn; (void)lists;
    simplelist_set_line_count(0);
    simplelist_addline(SIMPLELIST_ADD_LINE, "Cache initialized: %s",
             dircache_is_enabled() ? "Yes" : "No");
    simplelist_addline(SIMPLELIST_ADD_LINE, "Cache size: %d B",
             dircache_get_cache_size());
    simplelist_addline(SIMPLELIST_ADD_LINE, "Last size: %d B",
             global_status.dircache_size);
    simplelist_addline(SIMPLELIST_ADD_LINE, "Limit: %d B",
             DIRCACHE_LIMIT);
    simplelist_addline(SIMPLELIST_ADD_LINE, "Reserve: %d/%d B",
             dircache_get_reserve_used(), DIRCACHE_RESERVE);
    simplelist_addline(SIMPLELIST_ADD_LINE, "Scanning took: %d s",
             dircache_get_build_ticks() / HZ);
    simplelist_addline(SIMPLELIST_ADD_LINE, "Entry count: %d",
             dircache_get_entry_count());
    return btn;
}

static bool dbg_dircache_info(void)
{
    struct simplelist_info info;
    simplelist_info_init(&info, "Dircache Info", 7, NULL);
    info.action_callback = dircache_callback;
    info.hide_selection = true;
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

    simplelist_set_line_count(0);

    simplelist_addline(SIMPLELIST_ADD_LINE, "Initialized: %s",
             stat->initialized ? "Yes" : "No");
    simplelist_addline(SIMPLELIST_ADD_LINE, "DB Ready: %s",
             stat->ready ? "Yes" : "No");
    simplelist_addline(SIMPLELIST_ADD_LINE, "RAM Cache: %s",
             stat->ramcache ? "Yes" : "No");
    simplelist_addline(SIMPLELIST_ADD_LINE, "RAM: %d/%d B",
             stat->ramcache_used, stat->ramcache_allocated);
    simplelist_addline(SIMPLELIST_ADD_LINE, "Progress: %d%% (%d entries)",
             stat->progress, stat->processed_entries);
    simplelist_addline(SIMPLELIST_ADD_LINE, "Curfile: %s",
                       stat->curentry ? stat->curentry : "---");
    simplelist_addline(SIMPLELIST_ADD_LINE, "Commit step: %d",
             stat->commit_step);
    simplelist_addline(SIMPLELIST_ADD_LINE, "Commit delayed: %s",
             stat->commit_delayed ? "Yes" : "No");

    simplelist_addline(SIMPLELIST_ADD_LINE, "Queue length: %d", 
             stat->queue_length);
    
    if (synced)
    {
        synced = false;
        tagcache_screensync_event();
    }

    if (!btn && stat->curentry)
    {
        synced = true;
        return ACTION_REDRAW;
    }

    if (btn == ACTION_STD_CANCEL)
        tagcache_screensync_enable(false);

    return btn;
}
static bool dbg_tagcache_info(void)
{
    struct simplelist_info info;
    simplelist_info_init(&info, "Database Info", 8, NULL);
    info.action_callback = database_callback;
    info.hide_selection = true;
    info.scroll_all = true;
    
    /* Don't do nonblock here, must give enough processing time
       for tagcache thread. */
    /* info.timeout = TIMEOUT_NOBLOCK; */
    info.timeout = 1;
    tagcache_screensync_enable(true);
    return simplelist_show_list(&info);
}
#endif

#if CONFIG_CPU == SH7034
static bool dbg_save_roms(void)
{
    int fd;
    int oldmode = system_memory_guard(MEMGUARD_NONE);

    fd = creat("/internal_rom_0000-FFFF.bin");
    if(fd >= 0)
    {
        write(fd, (void *)0, 0x10000);
        close(fd);
    }

    fd = creat("/internal_rom_2000000-203FFFF.bin");
    if(fd >= 0)
    {
        write(fd, (void *)0x2000000, 0x40000);
        close(fd);
    }

    system_memory_guard(oldmode);
    return false;
}
#elif defined CPU_COLDFIRE
static bool dbg_save_roms(void)
{
    int fd;
    int oldmode = system_memory_guard(MEMGUARD_NONE);

#if defined(IRIVER_H100_SERIES)
    fd = creat("/internal_rom_000000-1FFFFF.bin");
#elif defined(IRIVER_H300_SERIES)
    fd = creat("/internal_rom_000000-3FFFFF.bin");
#elif defined(IAUDIO_X5) || defined(IAUDIO_M5) || defined(IAUDIO_M3)
    fd = creat("/internal_rom_000000-3FFFFF.bin");
#endif
    if(fd >= 0)
    {
        write(fd, (void *)0, FLASH_SIZE);
        close(fd);
    }
    system_memory_guard(oldmode);

#ifdef HAVE_EEPROM
    fd = creat("/internal_eeprom.bin");
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
    int fd;

    fd = creat("/internal_rom_000000-0FFFFF.bin");
    if(fd >= 0)
    {
        write(fd, (void *)0x20000000, FLASH_SIZE);
        close(fd);
    }

    return false;
}
#endif /* CPU */

#ifndef SIMULATOR
#if CONFIG_TUNER
static int radio_callback(int btn, struct gui_synclist *lists)
{
    (void)lists;
    if (btn == ACTION_STD_CANCEL)
        return btn;
    simplelist_set_line_count(1);

#if (CONFIG_TUNER & LV24020LP)
    simplelist_addline(SIMPLELIST_ADD_LINE,
                        "CTRL_STAT: %02X", lv24020lp_get(LV24020LP_CTRL_STAT) );
    simplelist_addline(SIMPLELIST_ADD_LINE,
                        "RADIO_STAT: %02X", lv24020lp_get(LV24020LP_REG_STAT) );
    simplelist_addline(SIMPLELIST_ADD_LINE,
                        "MSS_FM: %d kHz", lv24020lp_get(LV24020LP_MSS_FM) );
    simplelist_addline(SIMPLELIST_ADD_LINE,
                        "MSS_IF: %d Hz", lv24020lp_get(LV24020LP_MSS_IF) );
    simplelist_addline(SIMPLELIST_ADD_LINE,
                        "MSS_SD: %d Hz", lv24020lp_get(LV24020LP_MSS_SD) );
    simplelist_addline(SIMPLELIST_ADD_LINE,
                        "if_set: %d Hz", lv24020lp_get(LV24020LP_IF_SET) );
    simplelist_addline(SIMPLELIST_ADD_LINE,
                        "sd_set: %d Hz", lv24020lp_get(LV24020LP_SD_SET) );
#endif /* LV24020LP */
#if (CONFIG_TUNER & S1A0903X01)
    simplelist_addline(SIMPLELIST_ADD_LINE,
                        "Samsung regs: %08X", s1a0903x01_get(RADIO_ALL));
    /* This one doesn't return dynamic data atm */
#endif /* S1A0903X01 */
#if (CONFIG_TUNER & TEA5767)
    struct tea5767_dbg_info nfo;
    tea5767_dbg_info(&nfo);
    simplelist_addline(SIMPLELIST_ADD_LINE, "Philips regs:");
    simplelist_addline(SIMPLELIST_ADD_LINE,
             "   Read: %02X %02X %02X %02X %02X",
             (unsigned)nfo.read_regs[0], (unsigned)nfo.read_regs[1],
             (unsigned)nfo.read_regs[2], (unsigned)nfo.read_regs[3],
             (unsigned)nfo.read_regs[4]);
    simplelist_addline(SIMPLELIST_ADD_LINE,
             "   Write: %02X %02X %02X %02X %02X",
             (unsigned)nfo.write_regs[0], (unsigned)nfo.write_regs[1],
             (unsigned)nfo.write_regs[2], (unsigned)nfo.write_regs[3],
             (unsigned)nfo.write_regs[4]);
#endif /* TEA5767 */
#if (CONFIG_TUNER & SI4700)
    struct si4700_dbg_info nfo;
    si4700_dbg_info(&nfo);
    simplelist_addline(SIMPLELIST_ADD_LINE, "SI4700 regs:");
    /* Registers */
    simplelist_addline(SIMPLELIST_ADD_LINE,
             "%04X %04X %04X %04X",
             (unsigned)nfo.regs[0], (unsigned)nfo.regs[1],
             (unsigned)nfo.regs[2], (unsigned)nfo.regs[3]);
    simplelist_addline(SIMPLELIST_ADD_LINE,
             "%04X %04X %04X %04X",
             (unsigned)nfo.regs[4], (unsigned)nfo.regs[5],
             (unsigned)nfo.regs[6], (unsigned)nfo.regs[7]);
    simplelist_addline(SIMPLELIST_ADD_LINE,
             "%04X %04X %04X %04X",
             (unsigned)nfo.regs[8], (unsigned)nfo.regs[9],
             (unsigned)nfo.regs[10], (unsigned)nfo.regs[11]);
    simplelist_addline(SIMPLELIST_ADD_LINE,
             "%04X %04X %04X %04X",
             (unsigned)nfo.regs[12], (unsigned)nfo.regs[13],
             (unsigned)nfo.regs[14], (unsigned)nfo.regs[15]);
#endif /* SI4700 */
    return ACTION_REDRAW;
}
static bool dbg_fm_radio(void)
{
    struct simplelist_info info;
    info.scroll_all = true;
    simplelist_info_init(&info, "FM Radio", 1, NULL);
    simplelist_set_line_count(0);
    simplelist_addline(SIMPLELIST_ADD_LINE, "HW detected: %s",
                       radio_hardware_present() ? "yes" : "no");

    info.action_callback = radio_hardware_present()?radio_callback : NULL;
    info.hide_selection = true;
    return simplelist_show_list(&info);
}
#endif /* CONFIG_TUNER */
#endif /* !SIMULATOR */

#ifdef HAVE_LCD_BITMAP
extern bool do_screendump_instead_of_usb;

static bool dbg_screendump(void)
{
    do_screendump_instead_of_usb = !do_screendump_instead_of_usb;
    splashf(HZ, "Screendump %s",
                 do_screendump_instead_of_usb?"enabled":"disabled");
    return false;
}
#endif /* HAVE_LCD_BITMAP */

#if CONFIG_CPU == SH7034 || defined(CPU_COLDFIRE)
static bool dbg_set_memory_guard(void)
{
    static const struct opt_items names[MAXMEMGUARD] = {
        { "None",             -1 },
        { "Flash ROM writes", -1 },
        { "Zero area (all)",  -1 }
    };
    int mode = system_memory_guard(MEMGUARD_KEEP);

    set_option( "Catch mem accesses", &mode, INT, names, MAXMEMGUARD, NULL);
    system_memory_guard(mode);

    return false;
}
#endif /* CONFIG_CPU == SH7034 || defined(CPU_COLDFIRE) */

#if defined(HAVE_EEPROM) && !defined(HAVE_EEPROM_SETTINGS)
static bool dbg_write_eeprom(void)
{
    int fd;
    int rc;
    int old_irq_level;
    char buf[EEPROM_SIZE];
    int err;

    fd = open("/internal_eeprom.bin", O_RDONLY);

    if (fd >= 0)
    {
        rc = read(fd, buf, EEPROM_SIZE);

        if(rc == EEPROM_SIZE)
        {
            old_irq_level = disable_irq_save();

            err = eeprom_24cxx_write(0, buf, sizeof buf);
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
    int i = 0,j=0;
    int count = cpu_boost_log_getcount();
    int lines = LCD_HEIGHT/SYSFONT_HEIGHT;
    char *str;
    bool done;
    lcd_setfont(FONT_SYSFIXED);
    str = cpu_boost_log_getlog_first();
    viewportmanager_set_statusbar(false);
    while (i < count)
    {
        lcd_clear_display();
        for(j=0; j<lines; j++,i++)
        {
            if (!str)
                str = cpu_boost_log_getlog_next();
            if (str)
            {
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
    get_action(CONTEXT_STD,TIMEOUT_BLOCK);
    lcd_setfont(FONT_UI);
    viewportmanager_set_statusbar(true);
    return false;
}
#endif

#if (defined(HAVE_SCROLLWHEEL) && (CONFIG_KEYPAD==IPOD_4G_PAD) && !defined(SIMULATOR))
extern bool wheel_is_touched;
extern int old_wheel_value;
extern int new_wheel_value;
extern int wheel_delta;
extern unsigned int accumulated_wheel_delta;
extern unsigned int wheel_velocity;

static bool dbg_scrollwheel(void)
{
    char buf[64];
    unsigned int speed;

    lcd_setfont(FONT_SYSFIXED);
    viewportmanager_set_statusbar(false);

    while (1)
    {
        if (action_userabort(HZ/10))
            break;

        lcd_clear_display();

        /* show internal variables of scrollwheel driver */
        snprintf(buf, sizeof(buf), "wheel touched: %s", (wheel_is_touched) ? "true" : "false");
        lcd_puts(0, 0, buf);
        snprintf(buf, sizeof(buf), "new position: %2d", new_wheel_value);
        lcd_puts(0, 1, buf);
        snprintf(buf, sizeof(buf), "old position: %2d", old_wheel_value);
        lcd_puts(0, 2, buf);
        snprintf(buf, sizeof(buf), "wheel delta: %2d", wheel_delta);
        lcd_puts(0, 3, buf);
        snprintf(buf, sizeof(buf), "accumulated delta: %2d", accumulated_wheel_delta);
        lcd_puts(0, 4, buf);
        snprintf(buf, sizeof(buf), "velo [deg/s]: %4d", (int)wheel_velocity);
        lcd_puts(0, 5, buf);

        /* show effective accelerated scrollspeed */
        speed = button_apply_acceleration( (1<<31)|(1<<24)|wheel_velocity);
        snprintf(buf, sizeof(buf), "accel. speed: %4d", speed);
        lcd_puts(0, 6, buf);

        lcd_update();
    }
    viewportmanager_set_statusbar(true);
    return false;
}
#endif

#if defined(HAVE_USBSTACK) && defined(ROCKBOX_HAS_LOGF) && defined(USB_SERIAL)
static bool logf_usb_serial(void)
{
    bool serial_enabled = !usb_core_driver_enabled(USB_DRIVER_SERIAL);
    usb_core_enable_driver(USB_DRIVER_SERIAL,serial_enabled);
    splashf(HZ, "USB logf %s",
                 serial_enabled?"enabled":"disabled");
    return false;
}
#endif

#if defined(HAVE_USBSTACK) && defined(USB_STORAGE)
static bool usb_reconnect(void)
{
    splash(HZ, "Reconnect mass storage");
    usb_storage_reconnect();
    return false;
}
#endif

#if CONFIG_USBOTG == USBOTG_ISP1583
extern int dbg_usb_num_items(void);
extern char* dbg_usb_item(int selected_item, void *data, char *buffer, size_t buffer_len);

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
    isp1583.hide_selection = true;
    isp1583.get_name = dbg_usb_item;
    isp1583.action_callback = isp1583_action_callback;
    return simplelist_show_list(&isp1583);
}
#endif

#if defined(CREATIVE_ZVx) && !defined(SIMULATOR)
extern int pic_dbg_num_items(void);
extern char* pic_dbg_item(int selected_item, void *data, char *buffer, size_t buffer_len);

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
    pic.hide_selection = true;
    pic.get_name = pic_dbg_item;
    pic.action_callback = pic_action_callback;
    return simplelist_show_list(&pic);
}
#endif


/****** The menu *********/
struct the_menu_item {
    unsigned char *desc; /* string or ID */
    bool (*function) (void); /* return true if USB was connected */
};
static const struct the_menu_item menuitems[] = {
#if CONFIG_CPU == SH7034 || defined(CPU_COLDFIRE) || \
    (defined(CPU_PP) && !(CONFIG_STORAGE & STORAGE_SD))
        { "Dump ROM contents", dbg_save_roms },
#endif
#if CONFIG_CPU == SH7034 || defined(CPU_COLDFIRE) || defined(CPU_PP) \
    || CONFIG_CPU == S3C2440 || CONFIG_CPU == IMX31L
        { "View I/O ports", dbg_ports },
#endif
 #if (CONFIG_RTC == RTC_PCF50605) && !defined(SIMULATOR)
        { "View PCF registers", dbg_pcf },
#endif
#if defined(HAVE_TSC2100) && !defined(SIMULATOR)
        { "TSC2100 debug", tsc2100_debug },
#endif
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        { "CPU frequency", dbg_cpufreq },
#endif
#if defined(IRIVER_H100_SERIES) && !defined(SIMULATOR)
        { "S/PDIF analyzer", dbg_spdif },
#endif
#if CONFIG_CPU == SH7034 || defined(CPU_COLDFIRE)
        { "Catch mem accesses", dbg_set_memory_guard },
#endif
        { "View OS stacks", dbg_os },
#ifdef HAVE_LCD_BITMAP
#ifndef SIMULATOR
        { "View battery", view_battery },
#endif
        { "Screendump", dbg_screendump },
#endif
#ifndef SIMULATOR
        { "View HW info", dbg_hw_info },
#endif
#ifndef SIMULATOR
        { "View partitions", dbg_partitions },
#endif
#ifndef SIMULATOR
        { "View disk info", dbg_disk_info },
#if (CONFIG_STORAGE & STORAGE_ATA)
        { "Dump ATA identify info", dbg_identify_info},
#endif
#endif
#ifdef HAVE_DIRCACHE
        { "View dircache info", dbg_dircache_info },
#endif
#ifdef HAVE_TAGCACHE
        { "View database info", dbg_tagcache_info },
#endif
#ifdef HAVE_LCD_BITMAP
#if CONFIG_CODEC == SWCODEC
        { "View buffering thread", dbg_buffering_thread },
#elif !defined(SIMULATOR)
        { "View audio thread", dbg_audio_thread },
#endif
#ifdef PM_DEBUG
        { "pm histogram", peak_meter_histogram},
#endif /* PM_DEBUG */
#endif /* HAVE_LCD_BITMAP */
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
        {"logf", logfdisplay },
        {"logfdump", logfdump },
#endif
#if defined(HAVE_USBSTACK) && defined(ROCKBOX_HAS_LOGF) && defined(USB_SERIAL)
        {"logf over usb",logf_usb_serial },
#endif
#if defined(HAVE_USBSTACK) && defined(USB_STORAGE)
        {"reconnect usb storage",usb_reconnect},
#endif
#ifdef CPU_BOOST_LOGGING
        {"cpu_boost log",cpu_boost_log},
#endif
#if (defined(HAVE_SCROLLWHEEL) && (CONFIG_KEYPAD==IPOD_4G_PAD) && !defined(SIMULATOR))
        {"Debug scrollwheel", dbg_scrollwheel},
#endif
    };
static int menu_action_callback(int btn, struct gui_synclist *lists)
{
    if (btn == ACTION_STD_OK)
    {
        menuitems[gui_synclist_get_sel_pos(lists)].function();
        btn = ACTION_REDRAW;
    }
    return btn;
}
static char* dbg_menu_getname(int item, void * data,
                              char *buffer, size_t buffer_len)
{
    (void)data; (void)buffer; (void)buffer_len;
    return menuitems[item].desc;
}
bool debug_menu(void)
{
    struct simplelist_info info;

    simplelist_info_init(&info, "Debug Menu", ARRAYLEN(menuitems), NULL);
    info.action_callback = menu_action_callback;
    info.get_name = dbg_menu_getname;

    return simplelist_show_list(&info);
}
