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
#include <string.h>
#include "lcd.h"
#include "menu.h"
#include "debug_menu.h"
#include "kernel.h"
#include "sprintf.h"
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
#include "ata.h"
#include "fat.h"
#include "mas.h"
#include "eeprom_24cxx.h"
#if defined(HAVE_MMC) || defined(HAVE_HOTSWAP)
#include "ata_mmc.h"
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
#include "pcm_playback.h"
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

static char* dbg_menu_getname(int item, void * data, char *buffer);
static bool dbg_list(char *title, int count, int selection_size,
                     int (*action_callback)(int btn, struct gui_synclist *lists), 
                     char* (*dbg_getname)(int item, void * data, char *buffer))
{
    struct gui_synclist lists;
    int action;
    
    gui_synclist_init(&lists, dbg_getname, NULL, false, selection_size);
    gui_synclist_set_title(&lists, title, NOICON);
    gui_synclist_set_icon_callback(&lists, NULL);
    gui_synclist_set_nb_items(&lists, count*selection_size);
    if (dbg_getname != dbg_menu_getname)
        gui_synclist_hide_selection_marker(&lists, true);
    action_signalscreenchange();
    gui_synclist_draw(&lists);
    while(1)
    {
        gui_syncstatusbar_draw(&statusbars, true);
        action = get_action(CONTEXT_STD, HZ/5); 
        if (gui_synclist_do_button(&lists, action, LIST_WRAP_UNLESS_HELD))
            gui_synclist_draw(&lists);
        if (action_callback)
            action = action_callback(action, &lists);
        if (action == ACTION_STD_CANCEL)
            break;
        else if(default_event_handler(action) == SYS_USB_CONNECTED)
            return true;
    }
    action_signalscreenchange();
    return false;
}
/*---------------------------------------------------*/
/*    SPECIAL DEBUG STUFF                            */
/*---------------------------------------------------*/
extern struct thread_entry threads[MAXTHREADS];


#ifndef SIMULATOR
static char thread_status_char(int status)
{
    switch (status)
    {
        case STATE_RUNNING      : return 'R';
        case STATE_BLOCKED      : return 'B';
        case STATE_SLEEPING     : return 'S';
        case STATE_BLOCKED_W_TMO: return 'T';
    }

    return '?';
}
#if NUM_CORES > 1
#define IF_COP2(...) __VA_ARGS__
#else
#define IF_COP2(...)
#endif
static char* threads_getname(int selected_item, void * data, char *buffer)
{
    (void)data;
    struct thread_entry *thread = NULL;
    int status, usage;
    thread = &threads[selected_item];
    
    if (thread->name == NULL)
    {
        snprintf(buffer, MAX_PATH, "%2d: ---", selected_item);
        return buffer;
    }
    
    usage = thread_stack_usage(thread);
    status = thread_get_status(thread);
#ifdef HAVE_PRIORITY_SCHEDULING
    snprintf(buffer, MAX_PATH, "%2d: " IF_COP2("(%d) ") "%c%c %d %2d%% %s", 
             selected_item,
             IF_COP2(thread->core,)
             (status == STATE_RUNNING) ? '*' : ' ',
             thread_status_char(status),
             thread->priority,
             usage, thread->name);
#else
    snprintf(buffer, MAX_PATH, "%2d: " IF_COP2("(%d) ") "%c%c %2d%% %s", 
             selected_item,
             IF_COP2(thread->core,)
             (status == STATE_RUNNING) ? '*' : ' ',
             thread_status_char(status),
             usage, thread->name);
#endif
    return buffer;
}
static int dbg_threads_action_callback(int action, struct gui_synclist *lists)
{
#ifdef ROCKBOX_HAS_LOGF
    if (action == ACTION_STD_OK)
    {
        struct thread_entry *thread = &threads[gui_synclist_get_sel_pos(lists)];
        if (thread->name != NULL)
            remove_thread(thread);
    }
#endif
    gui_synclist_draw(lists);
    return action;
}
/* Test code!!! */
static bool dbg_os(void)
{
    return dbg_list(IF_COP2("Core and ") "Stack usage:", MAXTHREADS, 1,
                    dbg_threads_action_callback, threads_getname);
}
#endif /* !SIMULATOR */

#ifdef HAVE_LCD_BITMAP
#if CONFIG_CODEC != SWCODEC
#ifndef SIMULATOR
static bool dbg_audio_thread(void)
{
    char buf[32];
    struct audio_debug d;

    lcd_setmargins(0, 0);
    lcd_setfont(FONT_SYSFIXED);

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
    return false;
}
#endif /* !SIMULATOR */
#else /* CONFIG_CODEC == SWCODEC */
extern size_t filebuflen;
/* This is a size_t, but call it a long so it puts a - when it's bad. */

static unsigned int ticks, boost_ticks;

static void dbg_audio_task(void)
{
#ifndef SIMULATOR
    if(FREQ > CPUFREQ_NORMAL)
        boost_ticks++;
#endif

    ticks++;
}

static bool dbg_audio_thread(void)
{
    char buf[32];
    int button;
    int line;
    bool done = false;
    size_t bufused;
    size_t bufsize = pcmbuf_get_bufsize();
    int pcmbufdescs = pcmbuf_descs();

    ticks = boost_ticks = 0;

    tick_add_task(dbg_audio_task);

    lcd_setmargins(0, 0);
    lcd_setfont(FONT_SYSFIXED);
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
        action_signalscreenchange();
        line = 0;

        lcd_clear_display();

        bufused = bufsize - pcmbuf_free();

        snprintf(buf, sizeof(buf), "pcm: %7ld/%7ld", (long) bufused, (long) bufsize);
        lcd_puts(0, line++, buf);

        /* Playable space left */
        gui_scrollbar_draw(&screens[SCREEN_MAIN],0, line*8, LCD_WIDTH, 6, bufsize, 0, bufused, HORIZONTAL);
        line++;

        snprintf(buf, sizeof(buf), "codec: %8ld/%8ld", audio_filebufused(), (long) filebuflen);
        lcd_puts(0, line++, buf);

        /* Playable space left */
        gui_scrollbar_draw(&screens[SCREEN_MAIN],0, line*8, LCD_WIDTH, 6, filebuflen, 0,
                  audio_filebufused(), HORIZONTAL);
        line++;

        snprintf(buf, sizeof(buf), "track count: %2d", audio_track_count());
        lcd_puts(0, line++, buf);

#ifndef SIMULATOR
        snprintf(buf, sizeof(buf), "cpu freq: %3dMHz",
                 (int)((FREQ + 500000) / 1000000));
        lcd_puts(0, line++, buf);
#endif

        if (ticks > 0)
        {
            snprintf(buf, sizeof(buf), "boost ratio: %3d%%",
                     boost_ticks * 100 / ticks);
            lcd_puts(0, line++, buf);
        }

        snprintf(buf, sizeof(buf), "pcmbufdesc: %2d/%2d",
                pcmbuf_used_descs(), pcmbufdescs);
        lcd_puts(0, line++, buf);

        lcd_update();
    }

    tick_remove_task(dbg_audio_task);

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
    old_level = set_irq_level(HIGHEST_IRQ_LEVEL);

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

    set_irq_level(old_level); /* enable interrupts again */

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
#ifdef CPU_PP502x
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
        [tmo]"r"(10226)
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

    lcd_setmargins(0, 0);
    lcd_setfont(FONT_SYSFIXED);
    lcd_clear_display();

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

    while(1)
    {
        if (action_userabort(TIMEOUT_BLOCK))
            return false;
    }
#elif CONFIG_CPU == MCF5249 || CONFIG_CPU == MCF5250
    char buf[32];
    unsigned manu, id; /* flash IDs */
    int got_id; /* flag if we managed to get the flash IDs */
    int oldmode;  /* saved memory guard mode */
    int line = 0;

    oldmode = system_memory_guard(MEMGUARD_NONE);  /* disable memory guard */

    /* get flash ROM type */
    got_id = dbg_flash_id(&manu, &id, 0x5555, 0x2AAA); /* try SST, Atmel, NexFlash */
    if (!got_id)
        got_id = dbg_flash_id(&manu, &id, 0x555, 0x2AA); /* try AMD, Macronix */

    system_memory_guard(oldmode);  /* re-enable memory guard */

    lcd_setmargins(0, 0);
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

    while(1)
    {
        if (action_userabort(TIMEOUT_BLOCK))
            return false;
    }
#elif defined(CPU_PP502x)
    char buf[32];
    char pp_version[] = { (PP_VER2 >> 24) & 0xff, (PP_VER2 >> 16) & 0xff,
                          (PP_VER2 >> 8) & 0xff, (PP_VER2) & 0xff,
                          (PP_VER1 >> 24) & 0xff, (PP_VER1 >> 16) & 0xff,
                          (PP_VER1 >> 8) & 0xff, (PP_VER1) & 0xff, '\0' };
                          
    lcd_setmargins(0, 0);
    lcd_setfont(FONT_SYSFIXED);
    lcd_clear_display();

    lcd_puts(0, 0, "[Hardware info]");

    snprintf(buf, sizeof(buf), "HW rev: 0x%08x", ipod_hw_rev);
    lcd_puts(0, 1, buf);

    snprintf(buf, sizeof(buf), "PP version: %s", pp_version);
    lcd_puts(0, 2, buf);

    snprintf(buf, sizeof(buf), "Est. clock (kHz): %d", perfcheck());
    lcd_puts(0, 3, buf);
    
    lcd_update();

    while(1)
    {
        if (action_userabort(TIMEOUT_BLOCK))
            return false;
    }
#endif /* CONFIG_CPU */
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
                action_signalscreenchange();
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
static char* dbg_partitions_getname(int selected_item, void * data, char *buffer)
{
    (void)data;
    int partition = selected_item/2;
    struct partinfo* p = disk_partinfo(partition);
    if (selected_item%2)
    {
        snprintf(buffer, MAX_PATH, "   T:%x %ld MB", p->type, p->size / 2048);
    }
    else
    {
        snprintf(buffer, MAX_PATH, "P%d: S:%lx", partition, p->start);
    }
    return buffer;
}

bool dbg_partitions(void)
{
    return dbg_list("Partition Info", 4, 2,
                    NULL, dbg_partitions_getname);
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

    lcd_setmargins(0, 0);
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

#elif (CONFIG_KEYPAD == IPOD_3G_PAD) || \
    (CONFIG_KEYPAD == IPOD_4G_PAD)
#   define DEBUG_CANCEL  BUTTON_MENU

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
#   define DEBUG_CANCEL  BUTTON_PLAY

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#   define DEBUG_CANCEL  BUTTON_REC

#elif CONFIG_KEYPAD == GIGABEAT_PAD
#   define DEBUG_CANCEL  BUTTON_A

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#   define DEBUG_CANCEL  BUTTON_REW

#elif CONFIG_KEYPAD == SANSA_E200_PAD
#   define DEBUG_CANCEL  BUTTON_LEFT
#endif /* key definitios */

/* Test code!!! */
bool dbg_ports(void)
{
#if CONFIG_CPU == SH7034
    unsigned short porta;
    unsigned short portb;
    unsigned char portc;
    char buf[32];
    int adc_battery_voltage, adc_battery_level;

    lcd_setfont(FONT_SYSFIXED);
    lcd_setmargins(0, 0);
    lcd_clear_display();

    while(1)
    {
        porta = PADR;
        portb = PBDR;
        portc = PCDR;

        snprintf(buf, 32, "PADR: %04x", porta);
        lcd_puts(0, 0, buf);
        snprintf(buf, 32, "PBDR: %04x", portb);
        lcd_puts(0, 1, buf);

        snprintf(buf, 32, "AN0: %03x AN4: %03x", adc_read(0), adc_read(4));
        lcd_puts(0, 2, buf);
        snprintf(buf, 32, "AN1: %03x AN5: %03x", adc_read(1), adc_read(5));
        lcd_puts(0, 3, buf);
        snprintf(buf, 32, "AN2: %03x AN6: %03x", adc_read(2), adc_read(6));
        lcd_puts(0, 4, buf);
        snprintf(buf, 32, "AN3: %03x AN7: %03x", adc_read(3), adc_read(7));
        lcd_puts(0, 5, buf);

        battery_read_info(NULL, &adc_battery_voltage,
                          &adc_battery_level);
        snprintf(buf, 32, "Batt: %d.%02dV %d%%  ", adc_battery_voltage / 100,
                 adc_battery_voltage % 100, adc_battery_level);
        lcd_puts(0, 6, buf);

        lcd_update();
        if (button_get_w_tmo(HZ/10) == (DEBUG_CANCEL|BUTTON_REL))
            return false;
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
    int adc_battery, adc_battery_voltage, adc_battery_level;
    char buf[128];
    int line;

    lcd_setmargins(0, 0);
    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);

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

        snprintf(buf, sizeof(buf), "GPIO_READ:     %08x", gpio_read);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO_OUT:      %08x", gpio_out);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO_FUNCTION: %08x", gpio_function);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO_ENABLE:   %08x", gpio_enable);
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "GPIO1_READ:     %08x", gpio1_read);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO1_OUT:      %08x", gpio1_out);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO1_FUNCTION: %08x", gpio1_function);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO1_ENABLE:   %08x", gpio1_enable);
        lcd_puts(0, line++, buf);

        adc_buttons = adc_read(ADC_BUTTONS);
        adc_remote  = adc_read(ADC_REMOTE);
        battery_read_info(&adc_battery, &adc_battery_voltage,
                          &adc_battery_level);
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
        snprintf(buf, sizeof(buf), "ADC_BATTERY: %02x", adc_battery);
        lcd_puts(0, line++, buf);
#if defined(IRIVER_H100_SERIES) || defined(IRIVER_H300_SERIES)
        snprintf(buf, sizeof(buf), "ADC_REMOTEDETECT: %02x",
                 adc_read(ADC_REMOTEDETECT));
        lcd_puts(0, line++, buf);
#endif

        snprintf(buf, 32, "Batt: %d.%02dV %d%%  ", adc_battery_voltage / 100,
                 adc_battery_voltage % 100, adc_battery_level);
        lcd_puts(0, line++, buf);

#if defined(IRIVER_H100_SERIES) || defined(IRIVER_H300_SERIES)
        snprintf(buf, sizeof(buf), "remotetype: %d", remote_type());
        lcd_puts(0, line++, buf);
#endif

        lcd_update();
        if (button_get_w_tmo(HZ/10) == (DEBUG_CANCEL|BUTTON_REL))
            return false;
    }

#elif defined(CPU_PP502x)

    unsigned int gpio_a, gpio_b, gpio_c, gpio_d;
    unsigned int gpio_e, gpio_f, gpio_g, gpio_h;
    unsigned int gpio_i, gpio_j, gpio_k, gpio_l;

    char buf[128];
    int line;

    lcd_setmargins(0, 0);
    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        gpio_a = GPIOA_INPUT_VAL;
        gpio_b = GPIOB_INPUT_VAL;
        gpio_c = GPIOC_INPUT_VAL;

        gpio_g = GPIOG_INPUT_VAL;
        gpio_h = GPIOH_INPUT_VAL;
        gpio_i = GPIOI_INPUT_VAL;

        line = 0;
        snprintf(buf, sizeof(buf), "GPIO_A: %02x GPIO_G: %02x", gpio_a, gpio_g);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO_B: %02x GPIO_H: %02x", gpio_b, gpio_h);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO_C: %02x GPIO_I: %02x", gpio_c, gpio_i);
        lcd_puts(0, line++, buf);

        gpio_d = GPIOD_INPUT_VAL;
        gpio_e = GPIOE_INPUT_VAL;
        gpio_f = GPIOF_INPUT_VAL;

        gpio_j = GPIOJ_INPUT_VAL;
        gpio_k = GPIOK_INPUT_VAL;
        gpio_l = GPIOL_INPUT_VAL;

        snprintf(buf, sizeof(buf), "GPIO_D: %02x GPIO_J: %02x", gpio_d, gpio_j);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO_E: %02x GPIO_K: %02x", gpio_e, gpio_k);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO_F: %02x GPIO_L: %02x", gpio_f, gpio_l);
        lcd_puts(0, line++, buf);
        line++;
        
        snprintf(buf, sizeof(buf), "CLOCK_SRC:   %08lx", inl(0x60006020));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "CLOCK_0x2C:  %08lx", inl(0x6000602c));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "CLOCK_0xA0:  %08lx", inl(0x600060a0));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "PLL_CONTROL: %08lx", inl(0x60006034));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "PLL_STATUS:  %08lx", inl(0x6000603c));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "DEV_0x34:    %08lx", inl(0x70000034));
        lcd_puts(0, line++, buf);

#if defined(IRIVER_H10) || defined(IRIVER_H10_5GB)
        line++;
        snprintf(buf, sizeof(buf), "ADC_BATTERY:   %02x", adc_read(ADC_BATTERY));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "ADC_UNKNOWN_1: %02x", adc_read(ADC_UNKNOWN_1));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "ADC_REMOTE:    %02x", adc_read(ADC_REMOTE));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "ADC_SCROLLPAD: %02x", adc_read(ADC_SCROLLPAD));
        lcd_puts(0, line++, buf);
#elif defined(SANSA_E200)
        line++;
        snprintf(buf, sizeof(buf), "ADC_BVDD:   %02x", adc_read(ADC_BVDD));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "ADC_RTCSUP: %02x", adc_read(ADC_RTCSUP));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "ADC_UVDD:    %02x", adc_read(ADC_UVDD));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "ADC_CHG_IN: %02x", adc_read(ADC_CHG_IN));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "ADC_CVDD: %02x", adc_read(ADC_CVDD));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "ADC_BATTEMP: %02x", adc_read(ADC_BATTEMP));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "ADC_MICSUP1: %02x", adc_read(ADC_MICSUP1));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "ADC_MICSUP2: %02x", adc_read(ADC_MICSUP2));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "ADC_VBE1: %02x", adc_read(ADC_VBE1));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "ADC_VBE2: %02x", adc_read(ADC_VBE2));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "ADC_I_MICSUP1: %02x", adc_read(ADC_I_MICSUP1));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "ADC_I_MICSUP2: %02x", adc_read(ADC_I_MICSUP2));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "ADC_VBAT: %02x", adc_read(ADC_VBAT));
        lcd_puts(0, line++, buf);
#endif
        lcd_update();
        if (button_get_w_tmo(HZ/10) == (DEBUG_CANCEL|BUTTON_REL))
            return false;
    }

#elif CONFIG_CPU == PP5002
    unsigned int gpio_a, gpio_b, gpio_c, gpio_d;

    char buf[128];
    int line;

    lcd_setmargins(0, 0);
    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        gpio_a = GPIOA_INPUT_VAL;
        gpio_b = GPIOB_INPUT_VAL;
        gpio_c = GPIOC_INPUT_VAL;
        gpio_d = GPIOD_INPUT_VAL;

        line = 0;
        snprintf(buf, sizeof(buf), "GPIO_A: %02x GPIO_B: %02x", gpio_a, gpio_b);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO_C: %02x GPIO_D: %02x", gpio_c, gpio_d);
        lcd_puts(0, line++, buf);

        lcd_update();
        if (button_get_w_tmo(HZ/10) == (DEBUG_CANCEL|BUTTON_REL))
            return false;
    }
#elif CONFIG_CPU == S3C2440
    char buf[50];
    int line;

    lcd_setmargins(0, 0);
    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        line = 0;
        snprintf(buf, sizeof(buf), "[Ports and Registers]");                        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "GPACON: %08x GPBCON: %08x", GPACON, GPBCON);    lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPADAT: %08x GPBDAT: %08x", GPADAT, GPBDAT);    lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPAUP:  %08x GPBUP:  %08x", 0, GPBUP);          lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPCCON: %08x GPDCON: %08x", GPCCON, GPDCON);    lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPCDAT: %08x GPDDAT: %08x", GPCDAT, GPDDAT);    lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPCUP:  %08x GPDUP:  %08x", GPCUP, GPDUP);      lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "GPCCON: %08x GPDCON: %08x", GPCCON, GPDCON);    lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPCDAT: %08x GPDDAT: %08x", GPCDAT, GPDDAT);    lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPCUP:  %08x GPDUP:  %08x", GPCUP, GPDUP);      lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "GPECON: %08x GPFCON: %08x", GPECON, GPFCON);    lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPEDAT: %08x GPFDAT: %08x", GPEDAT, GPFDAT);    lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPEUP:  %08x GPFUP:  %08x", GPEUP, GPFUP);      lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "GPGCON: %08x GPHCON: %08x", GPGCON, GPHCON);    lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPGDAT: %08x GPHDAT: %08x", GPGDAT, GPHDAT);    lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPGUP:  %08x GPHUP:  %08x", GPGUP, GPHUP);      lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "GPJCON: %08x", GPJCON);                         lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPJDAT: %08x", GPJDAT);                         lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPJUP:  %08x", GPJUP);                          lcd_puts(0, line++, buf);
        
        line++;

        snprintf(buf, sizeof(buf), "SRCPND:  %08x INTMOD:  %08x", SRCPND, INTMOD);  lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "INTMSK:  %08x INTPND:  %08x", INTMSK, INTPND);  lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "CLKCON:  %08x CLKSLOW: %08x", CLKCON, CLKSLOW); lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "MPLLCON: %08x UPLLCON: %08x", MPLLCON, UPLLCON); lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "CLKDIVN: %08x", CLKDIVN); lcd_puts(0, line++, buf);

        lcd_update();
        if (button_get_w_tmo(HZ/10) == (DEBUG_CANCEL|BUTTON_REL))
            return false;
    }
#endif /* CPU */
    return false;
}
#else /* !HAVE_LCD_BITMAP */
bool dbg_ports(void)
{
    unsigned short porta;
    unsigned short portb;
    unsigned char portc;
    char buf[32];
    int button;
    int adc_battery_voltage;
    int currval = 0;

    lcd_clear_display();

    while(1)
    {
        porta = PADR;
        portb = PBDR;
        portc = PCDR;

        switch(currval)
        {
        case 0:
            snprintf(buf, 32, "PADR: %04x", porta);
            break;
        case 1:
            snprintf(buf, 32, "PBDR: %04x", portb);
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
            break;
        }
        lcd_puts(0, 0, buf);

        battery_read_info(NULL, &adc_battery_voltage, NULL);
        snprintf(buf, 32, "Batt: %d.%02dV", adc_battery_voltage / 100,
                 adc_battery_voltage % 100);
        lcd_puts(0, 1, buf);
        lcd_update();

        button = get_action(CONTEXT_SETTINGS,HZ/5);

        switch(button)
        {
            case ACTION_STD_CANCEL:
            action_signalscreenchange();
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
    return false;
}
#endif /* !HAVE_LCD_BITMAP */
#endif /* !SIMULATOR */

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
static bool dbg_cpufreq(void)
{
    char buf[128];
    int line;
    int button;

#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(0, 0);
    lcd_setfont(FONT_SYSFIXED);
#endif
    lcd_clear_display();

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
                action_signalscreenchange();
                return false;
        }
    }

    return false;
}
#endif /* HAVE_ADJUSTABLE_CPU_FREQ */

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

    lcd_setmargins(0, 0);
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        lcd_clear_display();
        switch (view) {
            case 0: /* voltage history graph */
                /* Find maximum and minimum voltage for scaling */
                maxv = 0;
                minv = 65535;
                for (i = 0; i < BAT_LAST_VAL; i++) {
                    if (power_history[i] > maxv)
                        maxv = power_history[i];
                    if (power_history[i] && (power_history[i] < minv))
                    {
                        minv = power_history[i];
                    }
                }

                if ((minv < 1) || (minv >= 65535))
                    minv = 1;
                if (maxv < 2)
                    maxv = 2;
                if (minv == maxv)
                    maxv < 65535 ? maxv++ : minv--;

                snprintf(buf, 30, "Battery %d.%02d", power_history[0] / 100,
                         power_history[0] % 100);
                lcd_puts(0, 0, buf);
                snprintf(buf, 30, "scale %d.%02d-%d.%02d V",
                         minv / 100, minv % 100, maxv / 100, maxv % 100);
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

                battery_read_info(NULL, &y, NULL);
                snprintf(buf, 30, "Battery: %d.%02d V", y / 100, y % 100);
                lcd_puts(0, 1, buf);
#ifdef ADC_EXT_POWER
                y = (adc_read(ADC_EXT_POWER) * EXT_SCALE_FACTOR) / 10000;
                snprintf(buf, 30, "External: %d.%02d V", y / 100, y % 100);
                lcd_puts(0, 2, buf);
#endif
#if CONFIG_CHARGING
#if CONFIG_CHARGING == CHARGING_CONTROL
                snprintf(buf, 30, "Chgr: %s %s",
                         charger_inserted() ? "present" : "absent",
                         charger_enabled ? "on" : "off");
                lcd_puts(0, 3, buf);
                snprintf(buf, 30, "short delta: %d", short_delta);
                lcd_puts(0, 5, buf);
                snprintf(buf, 30, "long delta: %d", long_delta);
                lcd_puts(0, 6, buf);
                lcd_puts(0, 7, power_message);
                snprintf(buf, 30, "USB Inserted: %s",
                         usb_inserted() ? "yes" : "no");
                lcd_puts(0, 8, buf);
#if defined IRIVER_H300_SERIES
                snprintf(buf, 30, "USB Charging Enabled: %s",
                         usb_charging_enabled() ? "yes" : "no");
                lcd_puts(0, 9, buf);
#endif
#else /* CONFIG_CHARGING != CHARGING_CONTROL */
#if defined IPOD_NANO || defined IPOD_VIDEO
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
#else
                snprintf(buf, 30, "Charger: %s",
                         charger_inserted() ? "present" : "absent");
                lcd_puts(0, 3, buf);
#endif
#endif /* CONFIG_CHARGING != CHARGING_CONTROL */
#endif /* CONFIG_CHARGING */
                break;

            case 2: /* voltage deltas: */
                lcd_puts(0, 0, "Voltage deltas:");

                for (i = 0; i <= 6; i++) {
                    y = power_history[i] - power_history[i+i];
                    snprintf(buf, 30, "-%d min: %s%d.%02d V", i,
                             (y < 0) ? "-" : "", ((y < 0) ? y * -1 : y) / 100,
                             ((y < 0) ? y * -1 : y ) % 100);
                    lcd_puts(0, i+1, buf);
                }
                break;

            case 3: /* remaining time estimation: */

#if CONFIG_CHARGING == CHARGING_CONTROL
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
#endif /* CONFIG_CHARGING == CHARGING_CONTROL */

                snprintf(buf, 30, "Last PwrHist: %d.%02d V",
                    power_history[0] / 100,
                    power_history[0] % 100);
                lcd_puts(0, 5, buf);

                snprintf(buf, 30, "battery level: %d%%", battery_level());
                lcd_puts(0, 6, buf);

                snprintf(buf, 30, "Est. remain: %d m", battery_time());
                lcd_puts(0, 7, buf);
                break;
        }

        lcd_update();

        switch(get_action(CONTEXT_SETTINGS,HZ/2))
        {
            case ACTION_SETTINGS_DEC:
                if (view)
                    view--;
                break;

            case ACTION_SETTINGS_INC:
                if (view < 3)
                    view++;
                break;

            case ACTION_STD_CANCEL:
                action_signalscreenchange();
                return false;
        }
    }
    return false;
}

#endif /* HAVE_LCD_BITMAP */
#endif

#ifndef SIMULATOR
#if defined(HAVE_MMC) || defined(HAVE_HOTSWAP)
static bool dbg_card_info(void)
{
    bool done = false;
    int currval = 0;
    int line;
    tCardInfo *card;
    unsigned char pbuf[32], pbuf2[32];
    unsigned char card_name[7];

    static const unsigned char i_vmin[] = { 0, 1, 5, 10, 25, 35, 60, 100 };
    static const unsigned char i_vmax[] = { 1, 5, 10, 25, 35, 45, 80, 200 };
    static const unsigned char *kbit_units[] = { "kBit/s", "MBit/s", "GBit/s" };
    static const unsigned char *nsec_units[] = { "ns", "µs", "ms" };
    static const char *spec_vers[] = { "1.0-1.2", "1.4", "2.0-2.2",
                                       "3.1-3.31", "4.0" };

    card_name[6] = '\0';

    lcd_setmargins(0, 0);
    lcd_setfont(FONT_SYSFIXED);

    while (!done)
    {
        card = card_get_info(currval / 2);

        line = 0;
        lcd_clear_display();
        snprintf(pbuf, sizeof(pbuf), "[MMC%d p%d]", currval / 2,
                 (currval % 2) + 1);
        lcd_puts(0, line++, pbuf);

        if (card->initialized)
        {
            if (!(currval % 2))   /* General info */
            {
                int temp;

                strncpy(card_name, ((unsigned char*)card->cid) + 3, 6);
                snprintf(pbuf, sizeof(pbuf), "%s Rev %d.%d", card_name,
                         (int) card_extract_bits(card->cid, 72, 4),
                         (int) card_extract_bits(card->cid, 76, 4));
                lcd_puts(0, line++, pbuf);
                snprintf(pbuf, sizeof(pbuf), "Prod: %d/%d",
                         (int) card_extract_bits(card->cid, 112, 4),
                         (int) card_extract_bits(card->cid, 116, 4) + 1997);
                lcd_puts(0, line++, pbuf);
                snprintf(pbuf, sizeof(pbuf), "Ser#: 0x%08lx",
                         card_extract_bits(card->cid, 80, 32));
                lcd_puts(0, line++, pbuf);
                snprintf(pbuf, sizeof(pbuf), "M=%02x, O=%04x",
                         (int) card_extract_bits(card->cid, 0, 8),
                         (int) card_extract_bits(card->cid, 8, 16));
                lcd_puts(0, line++, pbuf);
                temp = card_extract_bits(card->csd, 2, 4);
                snprintf(pbuf, sizeof(pbuf), "MMC v%s", temp < 5 ?
                         spec_vers[temp] : "?.?");
                lcd_puts(0, line++, pbuf);
                snprintf(pbuf, sizeof(pbuf), "Blocks: 0x%06lx", card->numblocks);
                lcd_puts(0, line++, pbuf);
                snprintf(pbuf, sizeof(pbuf), "Blksz.: %d P:%c%c", card->blocksize,
                         card_extract_bits(card->csd, 48, 1) ? 'R' : '-',
                         card_extract_bits(card->csd, 106, 1) ? 'W' : '-');
                lcd_puts(0, line++, pbuf);
            }
            else                  /* Technical details */
            {
                output_dyn_value(pbuf2, sizeof pbuf2, card->speed / 1000,
                                 kbit_units, false);
                snprintf(pbuf, sizeof pbuf, "Speed: %s", pbuf2);
                lcd_puts(0, line++, pbuf);

                output_dyn_value(pbuf2, sizeof pbuf2, card->tsac,
                                 nsec_units, false);
                snprintf(pbuf, sizeof pbuf, "Tsac: %s", pbuf2);
                lcd_puts(0, line++, pbuf);

                snprintf(pbuf, sizeof(pbuf), "Nsac: %d clk", card->nsac);
                lcd_puts(0, line++, pbuf);
                snprintf(pbuf, sizeof(pbuf), "R2W: *%d", card->r2w_factor);
                lcd_puts(0, line++, pbuf);
                snprintf(pbuf, sizeof(pbuf), "IRmax: %d..%d mA",
                         i_vmin[card_extract_bits(card->csd, 66, 3)],
                         i_vmax[card_extract_bits(card->csd, 69, 3)]);
                lcd_puts(0, line++, pbuf);
                snprintf(pbuf, sizeof(pbuf), "IWmax: %d..%d mA",
                         i_vmin[card_extract_bits(card->csd, 72, 3)],
                         i_vmax[card_extract_bits(card->csd, 75, 3)]);
                lcd_puts(0, line++, pbuf);
            }
        }
        else
            lcd_puts(0, line++, "Not found!");

        lcd_update();

        switch (get_action(CONTEXT_SETTINGS,HZ/2))
        {
            case ACTION_STD_CANCEL:
                done = true;
                break;

            case ACTION_SETTINGS_DEC:
                currval--;
                if (currval < 0)
                    currval = 3;
                break;

            case ACTION_SETTINGS_INC:
                currval++;
                if (currval > 3)
                    currval = 0;
                break;
        }
    }
    action_signalscreenchange();
    return false;
}
#else /* !defined(HAVE_MMC) && !defined(HAVE_HOTSWAP) */
static bool dbg_disk_info(void)
{
    char buf[128];
    bool done = false;
    int i;
    int page = 0;
    const int max_page = 11;
    unsigned short* identify_info = ata_get_identify();
    bool timing_info_present = false;
    char pio3[2], pio4[2];

    lcd_setmargins(0, 0);

    while(!done)
    {
        int y=0;
        int key;
        lcd_clear_display();
#ifdef HAVE_LCD_BITMAP
        lcd_puts(0, y++, "Disk info:");
        y++;
#endif

        switch (page) {
            case 0:
                for (i=0; i < 20; i++)
                    ((unsigned short*)buf)[i]=htobe16(identify_info[i+27]);
                buf[40]=0;
                /* kill trailing space */
                for (i=39; i && buf[i]==' '; i--)
                    buf[i] = 0;
                lcd_puts(0, y++, "Model");
                lcd_puts_scroll(0, y++, buf);
                break;

            case 1:
                for (i=0; i < 4; i++)
                    ((unsigned short*)buf)[i]=htobe16(identify_info[i+23]);
                buf[8]=0;
                lcd_puts(0, y++, "Firmware");
                lcd_puts(0, y++, buf);
                break;

            case 2:
                snprintf(buf, sizeof buf, "%ld MB",
                         ((unsigned long)identify_info[61] << 16 |
                          (unsigned long)identify_info[60]) / 2048 );
                lcd_puts(0, y++, "Size");
                lcd_puts(0, y++, buf);
                break;

            case 3: {
                unsigned long free;
                fat_size( IF_MV2(0,) NULL, &free );
                snprintf(buf, sizeof buf, "%ld MB",  free / 1024 );
                lcd_puts(0, y++, "Free");
                lcd_puts(0, y++, buf);
                break;
            }

            case 4:
                snprintf(buf, sizeof buf, "%d ms", ata_spinup_time * (1000/HZ));
                lcd_puts(0, y++, "Spinup time");
                lcd_puts(0, y++, buf);
                break;

            case 5:
                i = identify_info[83] & (1<<3);
                lcd_puts(0, y++, "Power mgmt:");
                lcd_puts(0, y++, i ? "enabled" : "unsupported");
                break;

            case 6:
                i = identify_info[83] & (1<<9);
                lcd_puts(0, y++, "Noise mgmt:");
                lcd_puts(0, y++, i ? "enabled" : "unsupported");
                break;

            case 7:
                i = identify_info[82] & (1<<6);
                lcd_puts(0, y++, "Read-ahead:");
                lcd_puts(0, y++, i ? "enabled" : "unsupported");
                break;

            case 8:
                timing_info_present = identify_info[53] & (1<<1);
                if(timing_info_present) {
                    pio3[1] = 0;
                    pio4[1] = 0;
                    lcd_puts(0, y++, "PIO modes:");
                    pio3[0] = (identify_info[64] & (1<<0)) ? '3' : 0;
                    pio4[0] = (identify_info[64] & (1<<1)) ? '4' : 0;
                    snprintf(buf, 128, "0 1 2 %s %s", pio3, pio4);
                    lcd_puts(0, y++, buf);
                } else {
                    lcd_puts(0, y++, "No PIO mode info");
                }
                break;

            case 9:
                timing_info_present = identify_info[53] & (1<<1);
                if(timing_info_present) {
                    lcd_puts(0, y++, "Cycle times");
                    snprintf(buf, 128, "%dns/%dns",
                             identify_info[67],
                             identify_info[68]);
                    lcd_puts(0, y++, buf);
                } else {
                    lcd_puts(0, y++, "No timing info");
                }
                break;

            case 10:
                timing_info_present = identify_info[53] & (1<<1);
                if(timing_info_present) {
                    i = identify_info[49] & (1<<11);
                    snprintf(buf, 128, "IORDY support: %s", i ? "yes" : "no");
                    lcd_puts(0, y++, buf);
                    i = identify_info[49] & (1<<10);
                    snprintf(buf, 128, "IORDY disable: %s", i ? "yes" : "no");
                    lcd_puts(0, y++, buf);
                } else {
                    lcd_puts(0, y++, "No timing info");
                }
                break;

            case 11:
                lcd_puts(0, y++, "Cluster size");
                snprintf(buf, 128, "%d bytes", fat_get_cluster_size(IF_MV(0)));
                lcd_puts(0, y++, buf);
                break;
        }
        lcd_update();

        /* Wait for a key to be pushed */
        key = get_action(CONTEXT_SETTINGS,HZ/5);
        switch(key) {
            case ACTION_STD_CANCEL:
                done = true;
                break;

            case ACTION_SETTINGS_DEC:
                if (--page < 0)
                    page = max_page;
                break;

            case ACTION_SETTINGS_INC:
                if (++page > max_page)
                    page = 0;
                break;
        }
        lcd_stop_scroll();
    }
    action_signalscreenchange();
    return false;
}
#endif /* !defined(HAVE_MMC) && !defined(HAVE_HOTSWAP) */
#endif /* !SIMULATOR */

#ifdef HAVE_DIRCACHE
static bool dbg_dircache_info(void)
{
    bool done = false;
    int line;
    char buf[32];

    lcd_setmargins(0, 0);
    lcd_setfont(FONT_SYSFIXED);

    while (!done)
    {
        line = 0;

        lcd_clear_display();
        snprintf(buf, sizeof(buf), "Cache initialized: %s",
                 dircache_is_enabled() ? "Yes" : "No");
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "Cache size: %d B",
                 dircache_get_cache_size());
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "Last size: %d B",
                 global_status.dircache_size);
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "Limit: %d B", DIRCACHE_LIMIT);
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "Reserve: %d/%d B",
                 dircache_get_reserve_used(), DIRCACHE_RESERVE);
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "Scanning took: %d s",
                 dircache_get_build_ticks() / HZ);
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "Entry count: %d",
                 dircache_get_entry_count());
        lcd_puts(0, line++, buf);

        lcd_update();

        if (action_userabort(HZ/2))
            return false;
    }

    return false;
}

#endif /* HAVE_DIRCACHE */

#ifdef HAVE_LCD_BITMAP
#ifdef HAVE_TAGCACHE
static bool dbg_tagcache_info(void)
{
    bool done = false;
    int line;
    char buf[32];
    struct tagcache_stat *stat;

    lcd_setmargins(0, 0);
    lcd_setfont(FONT_SYSFIXED);

    while (!done)
    {
        line = 0;

        lcd_clear_display();
        stat = tagcache_get_stat();
        snprintf(buf, sizeof(buf), "Initialized: %s", stat->initialized ? "Yes" : "No");
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "DB Ready: %s", stat->ready ? "Yes" : "No");
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "RAM Cache: %s", stat->ramcache ? "Yes" : "No");
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "RAM: %d/%d B",
                 stat->ramcache_used, stat->ramcache_allocated);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "Progress: %d%% (%d entries)",
                 stat->progress, stat->processed_entries);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "Commit step: %d", stat->commit_step);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "Commit delayed: %s",
                 stat->commit_delayed ? "Yes" : "No");
        lcd_puts(0, line++, buf);

        lcd_update();

        if (action_userabort(HZ/2))
            return false;
    }

    return false;
}
#endif
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
#elif defined(IAUDIO_X5) || defined(IAUDIO_M5)
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

        old_irq_level = set_irq_level(HIGHEST_IRQ_LEVEL);

        err = eeprom_24cxx_read(0, buf, sizeof buf);
        if (err)
            gui_syncsplash(HZ*3, "Eeprom read failure (%d)",err);
        else
        {
            write(fd, buf, sizeof buf);
        }

        set_irq_level(old_irq_level);

        close(fd);
    }
#endif

    return false;
}
#elif defined(CPU_PP) && !defined(SANSA_E200)
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
static bool dbg_fm_radio(void)
{
    char buf[32];
    bool fm_detected;

    lcd_setmargins(0, 0);

    fm_detected = radio_hardware_present();

    while(1)
    {
        int row = 0;

        lcd_clear_display();

        snprintf(buf, sizeof buf, "HW detected: %s", fm_detected?"yes":"no");
        lcd_puts(0, row++, buf);
#if (CONFIG_TUNER & LV24020LP)
        if (fm_detected)
        {
            snprintf(buf, sizeof buf, "CTRL_STAT: %02X",
                     sanyo_get(RADIO_ALL) );
            lcd_puts(0, row++, buf);

            snprintf(buf, sizeof buf, "RADIO_STAT: %02X",
                     sanyo_get(RADIO_REG_STAT));
            lcd_puts(0, row++, buf);

            snprintf(buf, sizeof buf, "MSS_FM: %d kHz",
                     (sanyo_get(RADIO_MSS_FM) ) );
            lcd_puts(0, row++, buf);

            snprintf(buf, sizeof buf, "MSS_IF: %d Hz",
                     (sanyo_get(RADIO_MSS_IF) ) );
            lcd_puts(0, row++, buf);

            snprintf(buf, sizeof buf, "MSS_SD: %d Hz",
                     (sanyo_get(RADIO_MSS_SD) ) );
            lcd_puts(0, row++, buf);

            snprintf(buf, sizeof buf, "if_set: %d Hz",
                     (sanyo_get(RADIO_IF_SET) ) );
            lcd_puts(0, row++, buf);

            snprintf(buf, sizeof buf, "sd_set: %d Hz",
                     (sanyo_get(RADIO_SD_SET) ) );
            lcd_puts(0, row++, buf);
        }
#endif
#if (CONFIG_TUNER & S1A0903X01)
        snprintf(buf, sizeof buf, "Samsung regs: %08X",
                 samsung_get(RADIO_ALL));
        lcd_puts(0, row++, buf);
#endif
#if (CONFIG_TUNER & TEA5767)
        {
            struct philips_dbg_info info;
            philips_dbg_info(&info);

            snprintf(buf, sizeof buf, "Philips regs:");
            lcd_puts(0, row++, buf);

            snprintf(buf, sizeof buf, "   Read: %02X %02X %02X %02X %02X",
                (unsigned)info.read_regs[0], (unsigned)info.read_regs[1],
                (unsigned)info.read_regs[2], (unsigned)info.read_regs[3],
                (unsigned)info.read_regs[4]);
            lcd_puts(0, row++, buf);

            snprintf(buf, sizeof buf, "   Write: %02X %02X %02X %02X %02X",
                (unsigned)info.write_regs[0], (unsigned)info.write_regs[1],
                (unsigned)info.write_regs[2], (unsigned)info.write_regs[3],
                (unsigned)info.write_regs[4]);
            lcd_puts(0, row++, buf);
        }
#endif
        lcd_update();

        if (action_userabort(HZ))
            return false;
    }
    return false;
}
#endif /* CONFIG_TUNER */
#endif /* !SIMULATOR */

#ifdef HAVE_LCD_BITMAP
extern bool do_screendump_instead_of_usb;

static bool dbg_screendump(void)
{
    do_screendump_instead_of_usb = !do_screendump_instead_of_usb;
    gui_syncsplash(HZ, "Screendump %s",
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

#if defined(TOSHIBA_GIGABEAT_F) && !defined(SIMULATOR)

extern volatile bool lcd_poweroff;

static bool dbg_lcd_power_off(void)
{
    lcd_setmargins(0, 0);

    while(1)
    {
        int button;

        lcd_clear_display();
        lcd_puts(0, 0, "LCD Power Off");
        if(lcd_poweroff)
            lcd_puts(1, 1, "Yes");
        else
            lcd_puts(1, 1, "No");

        lcd_update();

        button = get_action(CONTEXT_STD,HZ/5);
        switch(button)
        {
            case ACTION_STD_PREV:
            case ACTION_STD_NEXT:
                lcd_poweroff = !lcd_poweroff;
                break;
            case ACTION_STD_OK:
            case ACTION_STD_CANCEL:
                action_signalscreenchange();
                return false;
            default:
                sleep(HZ/10);
                break;
        }
    }
    return false;
}
#endif

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
            old_irq_level = set_irq_level(HIGHEST_IRQ_LEVEL);

            err = eeprom_24cxx_write(0, buf, sizeof buf);
            if (err)
                gui_syncsplash(HZ*3, "Eeprom write failure (%d)",err);
            else
                gui_syncsplash(HZ*3, "Eeprom written successfully");

            set_irq_level(old_irq_level);
        }
        else
        {
            gui_syncsplash(HZ*3, "File read error (%d)",rc);
        }
        close(fd);
    }
    else
    {
        gui_syncsplash(HZ*3, "Failed to open 'internal_eeprom.bin'");
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
    lcd_setmargins(0, 0);
    lcd_setfont(FONT_SYSFIXED);
    str = cpu_boost_log_getlog_first();
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
        action_signalscreenchange();
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
    action_signalscreenchange();
    return false;
}
#endif



/****** The menu *********/
struct the_menu_item {
    unsigned char *desc; /* string or ID */
    bool (*function) (void); /* return true if USB was connected */
};
static const struct the_menu_item menuitems[] = {
#if defined(TOSHIBA_GIGABEAT_F) && !defined(SIMULATOR)
        { "LCD Power Off", dbg_lcd_power_off },
#endif
#if CONFIG_CPU == SH7034 || defined(CPU_COLDFIRE) || \
    (defined(CPU_PP) && !defined(SANSA_E200))
        { "Dump ROM contents", dbg_save_roms },
#endif
#if CONFIG_CPU == SH7034 || defined(CPU_COLDFIRE) || defined(CPU_PP) || CONFIG_CPU == S3C2440
        { "View I/O ports", dbg_ports },
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
#ifndef SIMULATOR
        { "View OS stacks", dbg_os },
#endif
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
#if defined(HAVE_MMC)
        { "View MMC info", dbg_card_info },
#elif defined(HAVE_HOTSWAP)
        { "View microSD info", dbg_card_info },
#else
        { "View disk info", dbg_disk_info },
#endif
#endif
#ifdef HAVE_DIRCACHE
        { "View dircache info", dbg_dircache_info },
#endif
#ifdef HAVE_LCD_BITMAP
#ifdef HAVE_TAGCACHE
        { "View database info", dbg_tagcache_info },
#endif
#if CONFIG_CODEC == SWCODEC || !defined(SIMULATOR)
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
#ifdef ROCKBOX_HAS_LOGF
        {"logf", logfdisplay },
        {"logfdump", logfdump },
#endif
#ifdef CPU_BOOST_LOGGING
        {"cpu_boost log",cpu_boost_log},
#endif
    };
static int menu_action_callback(int btn, struct gui_synclist *lists)
{
    if (btn == ACTION_STD_OK)
    {
        menuitems[gui_synclist_get_sel_pos(lists)].function();
        gui_synclist_draw(lists);
    }
    return btn;
}
static char* dbg_menu_getname(int item, void * data, char *buffer)
{
    (void)data; (void)buffer;
    return menuitems[item].desc;
}
bool debug_menu(void)
{
    dbg_list("Debug Menu",ARRAYLEN(menuitems) , 1,
                     menu_action_callback, 
                     dbg_menu_getname);
    return false;
}
