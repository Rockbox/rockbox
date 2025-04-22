/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 *
 * Copyright (C) 2006 Alexander Spyridakis, Hristo Kovachev
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

#include "version.h"
#include "plugin.h"
#include "lang_enum.h"

/* matches struct in powermgmt.h */
struct battery_tables_t {
    unsigned short * const history;
    unsigned short * const disksafe;
    unsigned short * const shutoff;
    unsigned short * const discharge;
#if CONFIG_CHARGING
    unsigned short * const charge;
#endif
    const unsigned short elems;
    bool isdefault;
};

#define BATTERY_LEVELS_DEFAULT ROCKBOX_DIR"/battery_levels.default"
#define BATTERY_LEVELS_USER    ROCKBOX_DIR"/battery_levels.cfg"

#define BATTERY_LOG HOME_DIR "/battery_bench.txt"
#define BUF_SIZE 16000

#define EV_EXIT 1337

/* seems to work with 1300, but who knows... */
#define THREAD_STACK_SIZE 4*DEFAULT_STACK_SIZE

#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)

#define BATTERY_ON BUTTON_ON
#define BATTERY_RC_ON BUTTON_RC_ON

#define BATTERY_OFF BUTTON_OFF
#define BATTERY_RC_OFF BUTTON_RC_STOP

#define BATTERY_ON_TXT  "PLAY - start"
#define BATTERY_OFF_TXT "STOP"

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)

#define BATTERY_ON  BUTTON_PLAY
#define BATTERY_OFF BUTTON_MENU
#define BATTERY_ON_TXT  "PLAY - start"
#define BATTERY_OFF_TXT "MENU"

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD || \
      CONFIG_KEYPAD == AGPTEK_ROCKER_PAD

#define BATTERY_ON  BUTTON_SELECT
#define BATTERY_OFF BUTTON_POWER
#define BATTERY_ON_TXT  "SELECT - start"
#define BATTERY_OFF_TXT "POWER"

#elif (CONFIG_KEYPAD == SANSA_E200_PAD) || \
      (CONFIG_KEYPAD == SANSA_C200_PAD) || \
      (CONFIG_KEYPAD == SANSA_CLIP_PAD) || \
      (CONFIG_KEYPAD == SANSA_M200_PAD)
#define BATTERY_ON BUTTON_SELECT
#define BATTERY_OFF BUTTON_POWER
#define BATTERY_ON_TXT  "SELECT - start"
#define BATTERY_OFF_TXT "POWER"

#elif (CONFIG_KEYPAD == SANSA_FUZE_PAD)
#define BATTERY_ON BUTTON_SELECT
#define BATTERY_OFF BUTTON_HOME
#define BATTERY_ON_TXT  "SELECT - start"
#define BATTERY_OFF_TXT "HOME"

#elif (CONFIG_KEYPAD == IRIVER_H10_PAD ||	\
       CONFIG_KEYPAD == CREATIVE_ZENXFI3_PAD || \
       CONFIG_KEYPAD == SONY_NWZ_PAD || \
       CONFIG_KEYPAD == XDUOO_X3_PAD || \
       CONFIG_KEYPAD == IHIFI_770_PAD || \
       CONFIG_KEYPAD == IHIFI_800_PAD || \
       CONFIG_KEYPAD == XDUOO_X3II_PAD || \
       CONFIG_KEYPAD == XDUOO_X20_PAD || \
       CONFIG_KEYPAD == FIIO_M3K_LINUX_PAD || \
       CONFIG_KEYPAD == EROSQ_PAD)

#define BATTERY_ON  BUTTON_PLAY
#define BATTERY_OFF BUTTON_POWER
#define BATTERY_ON_TXT  "PLAY  - start"
#define BATTERY_OFF_TXT "POWER"

#elif CONFIG_KEYPAD == GIGABEAT_PAD

#define BATTERY_ON  BUTTON_SELECT
#define BATTERY_OFF BUTTON_POWER
#define BATTERY_ON_TXT  "SELECT - start"
#define BATTERY_OFF_TXT "POWER"

#elif CONFIG_KEYPAD == GIGABEAT_S_PAD \
   || CONFIG_KEYPAD == SAMSUNG_YPR0_PAD \
   || CONFIG_KEYPAD == CREATIVE_ZEN_PAD

#define BATTERY_ON  BUTTON_SELECT
#define BATTERY_OFF BUTTON_BACK
#define BATTERY_ON_TXT  "SELECT - start"
#define BATTERY_OFF_TXT "BACK"

#elif CONFIG_KEYPAD == MROBE500_PAD

#define BATTERY_OFF BUTTON_POWER
#define BATTERY_OFF_TXT "POWER"

#elif CONFIG_KEYPAD == MROBE100_PAD

#define BATTERY_ON  BUTTON_SELECT
#define BATTERY_OFF BUTTON_POWER
#define BATTERY_ON_TXT  "SELECT - start"
#define BATTERY_OFF_TXT "POWER"

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD

#define BATTERY_ON  BUTTON_PLAY
#define BATTERY_OFF BUTTON_REC
#define BATTERY_RC_ON  BUTTON_RC_PLAY
#define BATTERY_RC_OFF BUTTON_RC_REC
#define BATTERY_ON_TXT  "PLAY - start"
#define BATTERY_OFF_TXT "REC"

#elif CONFIG_KEYPAD == COWON_D2_PAD

#define BATTERY_OFF BUTTON_POWER
#define BATTERY_OFF_TXT "POWER"

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD
#define BATTERY_ON BUTTON_PLAY
#define BATTERY_ON_TXT  "PLAY - start"
#define BATTERY_OFF BUTTON_BACK
#define BATTERY_OFF_TXT "BACK"

#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD

#define BATTERY_ON  BUTTON_MENU
#define BATTERY_OFF BUTTON_POWER
#define BATTERY_ON_TXT  "MENU - start"
#define BATTERY_OFF_TXT "POWER"

#elif CONFIG_KEYPAD == PHILIPS_HDD6330_PAD

#define BATTERY_ON  BUTTON_MENU
#define BATTERY_OFF BUTTON_POWER
#define BATTERY_ON_TXT  "MENU - start"
#define BATTERY_OFF_TXT "POWER"

#elif CONFIG_KEYPAD == PHILIPS_SA9200_PAD

#define BATTERY_ON  BUTTON_MENU
#define BATTERY_OFF BUTTON_POWER
#define BATTERY_ON_TXT  "MENU - start"
#define BATTERY_OFF_TXT "POWER"

#elif CONFIG_KEYPAD == ONDAVX747_PAD

#define BATTERY_OFF BUTTON_POWER
#define BATTERY_OFF_TXT "POWER"
#elif CONFIG_KEYPAD == ONDAVX777_PAD

#define BATTERY_OFF BUTTON_POWER
#define BATTERY_OFF_TXT "POWER"

#elif (CONFIG_KEYPAD == SAMSUNG_YH820_PAD) || \
      (CONFIG_KEYPAD == SAMSUNG_YH92X_PAD)

#define BATTERY_ON      BUTTON_LEFT
#define BATTERY_OFF     BUTTON_RIGHT
#define BATTERY_ON_TXT  "LEFT"
#define BATTERY_OFF_TXT "RIGHT"

#elif CONFIG_KEYPAD == PBELL_VIBE500_PAD

#define BATTERY_ON  BUTTON_PLAY
#define BATTERY_OFF BUTTON_REC
#define BATTERY_ON_TXT  "PLAY - start"
#define BATTERY_OFF_TXT "REC"

#elif CONFIG_KEYPAD == MPIO_HD200_PAD
#define BATTERY_ON  BUTTON_PLAY
#define BATTERY_OFF BUTTON_REC
#define BATTERY_ON_TXT  "PLAY - start"
#define BATTERY_OFF_TXT "REC"

#elif CONFIG_KEYPAD == MPIO_HD300_PAD
#define BATTERY_ON  BUTTON_PLAY
#define BATTERY_OFF BUTTON_REC
#define BATTERY_ON_TXT  "PLAY - start"
#define BATTERY_OFF_TXT "REC"

#elif CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD
#define BATTERY_ON  BUTTON_PLAYPAUSE
#define BATTERY_OFF BUTTON_POWER
#define BATTERY_ON_TXT  "PLAYPAUSE - start"
#define BATTERY_OFF_TXT "POWER"

#elif CONFIG_KEYPAD == SANSA_CONNECT_PAD
#define BATTERY_ON BUTTON_SELECT
#define BATTERY_OFF BUTTON_POWER
#define BATTERY_ON_TXT  "SELECT - start"
#define BATTERY_OFF_TXT "POWER"

#elif (CONFIG_KEYPAD == HM60X_PAD) || (CONFIG_KEYPAD == HM801_PAD)
#define BATTERY_ON BUTTON_SELECT
#define BATTERY_OFF BUTTON_POWER
#define BATTERY_ON_TXT  "SELECT - start"

#define BATTERY_OFF_TXT "POWER"

#elif CONFIG_KEYPAD == DX50_PAD
#define BATTERY_ON      BUTTON_PLAY
#define BATTERY_OFF     BUTTON_POWER_LONG
#define BATTERY_OFF_TXT "Power Long"
#define BATTERY_ON_TXT  "Play - start"

#elif CONFIG_KEYPAD == CREATIVE_ZENXFI2_PAD
#define BATTERY_ON      BUTTON_MENU
#define BATTERY_OFF     BUTTON_POWER
#define BATTERY_OFF_TXT "Power"
#define BATTERY_ON_TXT  "Menu - start"

#elif CONFIG_KEYPAD == FIIO_M3K_PAD
#define BATTERY_ON      BUTTON_PLAY
#define BATTERY_OFF     BUTTON_POWER
#define BATTERY_ON_TXT  "Play"
#define BATTERY_OFF_TXT "Power"

#elif CONFIG_KEYPAD == SHANLING_Q1_PAD
/* use touchscreen */

#elif CONFIG_KEYPAD == MA_PAD
#define BATTERY_ON     BUTTON_PLAY
#define BATTERY_OFF    BUTTON_BACK
#define BATTERY_ON_TXT  "Play"
#define BATTERY_OFF_TXT "Back"

#else
#error "No keymap defined!"
#endif

#if defined(HAVE_TOUCHSCREEN)

#ifndef BATTERY_ON
#define BATTERY_ON       BUTTON_CENTER
#endif
#ifndef BATTERY_OFF
#define BATTERY_OFF      BUTTON_TOPLEFT
#endif
#ifndef BATTERY_ON_TXT
#define BATTERY_ON_TXT  "CENTRE - start"
#endif
#ifndef BATTERY_OFF_TXT
#define BATTERY_OFF_TXT "TOPLEFT"
#endif

#endif

/****************************** Plugin Entry Point ****************************/
long start_tick;

/* Struct for battery information */
static struct batt_info
{
    /* the size of the struct elements is optimised to make the struct size
     * a power of 2 */
    unsigned secs;
    int eta;
    int voltage;
#if CONFIG_BATTERY_MEASURE & CURRENT_MEASURE
    int current;
#endif
    short level;
    unsigned short flags;
} bat[BUF_SIZE/sizeof(struct batt_info)];

#define BUF_ELEMENTS    (sizeof(bat)/sizeof(struct batt_info))


static struct
{
    unsigned int id; /* worker thread id */
    long stack[THREAD_STACK_SIZE / sizeof(long)];
} gThread;

static struct event_queue thread_q SHAREDBSS_ATTR;
static bool in_usb_mode;
static unsigned int buf_idx;

static int exit_tsr(bool reenter)
{
    int exit_status;
    long button;

    rb->lcd_clear_display();
    rb->lcd_puts_scroll(0, 0, "Batt.Bench is currently running.");
    rb->lcd_puts_scroll(0, 1, "Press " BATTERY_OFF_TXT " to cancel the test");
    rb->lcd_puts_scroll(0, 2, "Anything else will resume");
    if(rb->global_settings->talk_menu)
        rb->talk_id(VOICE_BATTERY_BENCH_IS_ALREADY_RUNNING, true);
    rb->lcd_update();

    while (1)
    {
        button = rb->button_get(true);
        if (IS_SYSEVENT(button))
            continue;
        if (button == BATTERY_OFF)
        {
            rb->queue_post(&thread_q, EV_EXIT, 0);
            rb->thread_wait(gThread.id);
            /* remove the thread's queue from the broadcast list */
            rb->queue_delete(&thread_q);
            exit_status = (reenter ? PLUGIN_TSR_TERMINATE : PLUGIN_TSR_SUSPEND);

        }
        else exit_status = PLUGIN_TSR_CONTINUE;

        break;
    }
    FOR_NB_SCREENS(idx)
        rb->screens[idx]->scroll_stop();

    return exit_status;
}

#define BIT_CHARGER     0x1
#define BIT_CHARGING    0x2
#define BIT_USB_POWER   0x4

#define HMS(x) (x)/3600,((x)%3600)/60,((x)%3600)%60

#if CONFIG_CHARGING || defined(HAVE_USB_POWER)
static unsigned int charge_state(void)
{
    unsigned int ret = 0;
#if CONFIG_CHARGING
    if (rb->charger_inserted())
        ret = BIT_CHARGER;
#if CONFIG_CHARGING >= CHARGING_MONITOR
    if (rb->charging_state())
        ret |= BIT_CHARGING;
#endif
#endif
    /* USB insertion means nothing if USB cannot power the device */
#ifdef HAVE_USB_POWER
    if (rb->usb_inserted())
        ret |= BIT_USB_POWER;
#endif
    return ret;
}
#endif


static void flush_buffer(void)
{
    int fd;
    unsigned int i;

    /* don't access the disk when in usb mode, or when no data is available */
    if (in_usb_mode || (buf_idx == 0))
        return;

    fd = rb->open(BATTERY_LOG, O_RDWR | O_CREAT | O_APPEND, 0666);
    if (fd < 0)
        return;

    for (i = 0; i < buf_idx; i++)
    {
        rb->fdprintf(fd,
                "%02d:%02d:%02d,  %05d,     %03d%%,     "
                "%02d:%02d,         %04d,   "
#if CONFIG_BATTERY_MEASURE & CURRENT_MEASURE
                "      %04d,   "
#endif
#if CONFIG_CHARGING
                "  %c"
#if CONFIG_CHARGING >= CHARGING_MONITOR
                ",  %c"
#endif
#endif
#ifdef HAVE_USB_POWER
                ",  %c"
#endif
                "\n",

                HMS(bat[i].secs), bat[i].secs, bat[i].level,
                bat[i].eta / 60, bat[i].eta % 60,
                bat[i].voltage
#if CONFIG_BATTERY_MEASURE & CURRENT_MEASURE
                , bat[i].current
#endif
#if CONFIG_CHARGING
                , (bat[i].flags & BIT_CHARGER) ? 'A' : '-'
#if CONFIG_CHARGING >= CHARGING_MONITOR
                , (bat[i].flags & BIT_CHARGING) ? 'C' : '-'
#endif
#endif
#ifdef HAVE_USB_POWER
                , (bat[i].flags & BIT_USB_POWER) ? 'U' : '-'
#endif
        );
    }
    rb->close(fd);

    buf_idx = 0;
}


static void thread(void)
{
    bool exit = false;
    char *exit_reason = "unknown";
    long sleep_time = 60 * HZ;
    struct queue_event ev;
    int fd;

    in_usb_mode = false;
    buf_idx = 0;

    while (!exit)
    {
        /* add data to buffer */
        if (buf_idx < BUF_ELEMENTS)
        {
            bat[buf_idx].secs = (*rb->current_tick - start_tick) / HZ;
            bat[buf_idx].level = rb->battery_level();
            bat[buf_idx].eta = rb->battery_time();
            bat[buf_idx].voltage = rb->battery_voltage();
#if CONFIG_BATTERY_MEASURE & CURRENT_MEASURE
            bat[buf_idx].current = rb->battery_current();
#endif
#if CONFIG_CHARGING || defined(HAVE_USB_POWER)
            bat[buf_idx].flags = charge_state();
#endif
            buf_idx++;
#ifdef USING_STORAGE_CALLBACK
            rb->register_storage_idle_func(flush_buffer);
#endif
        }

        /* What to do when the measurement buffer is full:
           1) save our measurements to disk but waste some power doing so?
           2) throw away measurements to save some power?
           The choice made here is to save the measurements. It is quite unusual
           for this to occur because it requires > 16 hours of no disk activity.
         */
        if (buf_idx == BUF_ELEMENTS) {
            flush_buffer();
        }

        /* sleep some time until next measurement */
        rb->queue_wait_w_tmo(&thread_q, &ev, sleep_time);
        switch (ev.id)
        {
            case SYS_USB_CONNECTED:
                in_usb_mode = true;
                rb->usb_acknowledge(SYS_USB_CONNECTED_ACK);
                break;
            case SYS_USB_DISCONNECTED:
                in_usb_mode = false;
                break;
            case SYS_POWEROFF:
            case SYS_REBOOT:
                exit_reason = "power off";
                exit = true;
                break;
            case EV_EXIT:
                rb->splash(HZ, "Exiting battery_bench...");
                exit_reason = "plugin exit";
                exit = true;
                break;
        }
    }

#ifdef USING_STORAGE_CALLBACK
    /* unregister flush callback and flush to disk */
    rb->unregister_storage_idle_func(flush_buffer, true);
#else
    flush_buffer();
#endif

    /* log end of bench and exit reason */
    fd = rb->open(BATTERY_LOG, O_RDWR | O_CREAT | O_APPEND, 0666);
    if (fd >= 0)
    {
        rb->fdprintf(fd, "--Battery bench ended, reason: %s--\n", exit_reason);
        rb->close(fd);
    }
}


typedef void (*plcdfunc)(int x, int y, const unsigned char *str);

static void put_centered_str(const char* str, plcdfunc putsxy, int lcd_width, int line)
{
    int strwdt, strhgt;
    rb->lcd_getstringsize(str, &strwdt, &strhgt);
    putsxy((lcd_width - strwdt)/2, line*(strhgt), str);
}

void do_export_battery_tables(void)
{
    size_t elems = rb->device_battery_tables->elems;
    if (!rb->device_battery_tables->isdefault)
        return; /* no need to print out non-defaults */
    unsigned int i;
    int fd;
    /* write out the default battery levels file */
    if (!rb->file_exists(BATTERY_LEVELS_DEFAULT))
    {
        fd = rb->open(BATTERY_LEVELS_DEFAULT, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd >= 0)
        {
            rb->fdprintf(fd, "# Rename to %s\n# " MODEL_NAME " Battery Levels (%s)\n\n",
                     BATTERY_LEVELS_USER, rb->rbversion);

            rb->fdprintf(fd, "# Battery voltage(millivolt) of {");
            for(i= 0;i < elems;i++)
            {
                rb->fdprintf(fd, "%u%%, ", i * 10);
            }
            rb->lseek(fd, -2, SEEK_CUR); /*remove last comma */
            rb->fdprintf(fd, "} when charging %sabled\n", "dis");
            rb->fdprintf(fd, "discharge: {");
            for(i= 0;i < elems;i++)
            {
                rb->fdprintf(fd, "%u, ", rb->device_battery_tables->discharge[i]);
            }
            rb->lseek(fd, -2, SEEK_CUR); /*remove last comma */
            rb->fdprintf(fd, "}\n\n");
#if CONFIG_CHARGING
            rb->fdprintf(fd, "# Battery voltage(millivolt) of {");
            for(i= 0;i < elems;i++)
            {
                rb->fdprintf(fd, "%u%%, ", i * 10);
            }
            rb->lseek(fd, -2, SEEK_CUR); /*remove last comma */
            rb->fdprintf(fd, "} when charging %sabled\n", "en");
            rb->fdprintf(fd, "charge: {");
            for(i= 0;i < elems;i++)
            {
                rb->fdprintf(fd, "%u, ", rb->device_battery_tables->charge[i]);
            }
            rb->lseek(fd, -2, SEEK_CUR); /*remove last comma */
            rb->fdprintf(fd, "}\n\n");
#endif

            rb->fdprintf(fd, "# WARNING 'shutoff' and 'disksafe' levels protect " \
                             "from battery over-discharge and dataloss\n\n");

            rb->fdprintf(fd, "# Battery voltage(millivolt) lower than this %s\n",
                             "player will shutdown");

            rb->fdprintf(fd, "#shutoff: %d\n\n", *rb->device_battery_tables->shutoff);

            rb->fdprintf(fd, "# Battery voltage(millivolt) lower than this %s\n",
                             "won't access the disk to write");
            rb->fdprintf(fd, "#disksafe: %d\n\n", *rb->device_battery_tables->disksafe);

            rb->close(fd);
        }
    }
}

enum plugin_status plugin_start(const void* parameter)
{
    int button, fd;
    bool resume = false;
    bool on = false;
    start_tick = *rb->current_tick;
    int i;
    const char *msgs[] = { "Battery Benchmark","Check file", BATTERY_LOG,
                           "for more info", BATTERY_ON_TXT, BATTERY_OFF_TXT " - quit" };

    if (parameter == rb->plugin_tsr)
    {
        resume = true;
        on = true;
    }

    rb->lcd_clear_display();
    rb->lcd_setfont(FONT_SYSFIXED);

    for (i = 0; i<(int)(sizeof(msgs)/sizeof(char *)); i++)
        put_centered_str(msgs[i],rb->lcd_putsxy,LCD_WIDTH,i+1);
    if(rb->global_settings->talk_menu)
        rb->talk_id(VOICE_BAT_BENCH_KEYS, true);
    rb->lcd_update();

#ifdef HAVE_REMOTE_LCD
    rb->lcd_remote_clear_display();
    put_centered_str(msgs[0],rb->lcd_remote_putsxy,LCD_REMOTE_WIDTH,0);
    put_centered_str(msgs[sizeof(msgs)/sizeof(char*)-2],
                    rb->lcd_remote_putsxy,LCD_REMOTE_WIDTH,1);
    put_centered_str(msgs[sizeof(msgs)/sizeof(char*)-1],
                    rb->lcd_remote_putsxy,LCD_REMOTE_WIDTH,2);
    rb->lcd_remote_update();
#endif
    if (!resume)
    {
        do_export_battery_tables();
        do
        {
            button = rb->button_get(true);
            switch (button)
            {
                case BATTERY_ON:
    #ifdef BATTERY_RC_ON
                case BATTERY_RC_ON:
    #endif
                    on = true;
                    break;
                case BATTERY_OFF:
    #ifdef BATTERY_RC_OFF
                case BATTERY_RC_OFF:
    #endif
                    return PLUGIN_OK;

                default:
                    if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                        return PLUGIN_USB_CONNECTED;
            }
        }while(!on);
    }
    fd = rb->open(BATTERY_LOG, O_RDONLY);
    if (fd < 0)
    {
        fd = rb->open(BATTERY_LOG, O_RDWR | O_CREAT, 0666);
        if (fd >= 0)
        {

            rb->fdprintf(fd,
                "# This plugin will log your battery performance in a\n"
                "# file (%s) every minute.\n"
                "# To properly test your battery:\n"
                "# 1) Select and playback an album. "
                "(Be sure to be more than the player's buffer)\n"
                "# 2) Set to repeat.\n"
                "# 3) Let the player run completely out of battery.\n"
                "# 4) Recharge and copy (or whatever you want) the txt file to "
                "your computer.\n"
                "# Now you can make graphs with the data of the battery log.\n"
                "# Do not enter another plugin during the test or else the \n"
                "# logging activity will end.\n\n"
                "# P.S: You can decide how you will make your tests.\n"
                "# Just don't open another plugin to be sure that your log "
                "will continue.\n\n",BATTERY_LOG);
            rb->fdprintf(fd,
                "# Battery bench run for %s version %s\n\n"
                ,MODEL_NAME,rb->rbversion);

            rb->fdprintf(fd, "# Battery type: %d mAh      Buffer Entries: %d\n",
#if BATTERY_CAPACITY_INC > 0
                         rb->global_settings->battery_capacity,
#else
                         BATTERY_CAPACITY_DEFAULT,
#endif
                         (int)BUF_ELEMENTS);

            rb->fdprintf(fd, "# Rockbox has been running for %02d:%02d:%02d\n",
                HMS((unsigned)start_tick/HZ));

            rb->fdprintf(fd,
                "# Time:,  Seconds:,  Level:,  Time Left:,  Voltage[mV]:"
#if CONFIG_BATTERY_MEASURE & CURRENT_MEASURE
                ", Current[mA]:"
#endif
#if CONFIG_CHARGING
                ", C:"
#endif
#if CONFIG_CHARGING >= CHARGING_MONITOR
                ", S:"
#endif
#ifdef HAVE_USB_POWER
                ", U:"
#endif
                "\n");
            rb->close(fd);
        }
        else
        {
            rb->splash(HZ / 2, "Cannot create file!");
            return PLUGIN_ERROR;
        }
    }
    else
    {
        rb->close(fd);
        fd = rb->open(BATTERY_LOG, O_RDWR | O_APPEND);
        rb->fdprintf(fd, "\n# --File already present. Resuming Benchmark--\n");
        rb->fdprintf(fd,
            "# Battery bench run for %s version %s\n\n"
            ,MODEL_NAME,rb->rbversion);
        rb->fdprintf(fd, "# Rockbox has been running for %02d:%02d:%02d\n",
            HMS((unsigned)start_tick/HZ));
        rb->close(fd);
    }

    rb->memset(&gThread, 0, sizeof(gThread));
    rb->queue_init(&thread_q, true); /* put the thread's queue in the bcast list */
    gThread.id = rb->create_thread(thread, gThread.stack, sizeof(gThread.stack),
                                   0, "Battery Benchmark"
                                   IF_PRIO(, PRIORITY_BACKGROUND)
                                   IF_COP(, CPU));

    if (gThread.id == 0 || gThread.id == UINT_MAX)
    {
        rb->splash(HZ, "Cannot create thread!");
        return PLUGIN_ERROR;
    }

    rb->plugin_tsr(exit_tsr);

    return PLUGIN_OK;
}
