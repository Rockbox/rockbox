/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id: $
*
* Copyright (C) 2011-2021 by Tomasz Moń
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

#include <stdio.h>
#include "config.h"
#include "file.h"
#include "system.h"
#include "time.h"
#include "power.h"
#include "kernel.h"
#include "panic.h"
/*#define LOGF_ENABLE*/
#include "logf.h"
#include "avr-sansaconnect.h"
#include "uart-target.h"
#include "usb.h"
#include "button.h"
#include "backlight.h"
#include "powermgmt.h"
#include "aic3x.h"

//#define BUTTON_DEBUG

#ifdef BUTTON_DEBUG
#include "lcd-target.h"
#include "lcd.h"
#include "font.h"
#include "common.h"
#endif

#ifdef BUTTON_DEBUG
#define dbgprintf DEBUGF
#else
#define dbgprintf(...)
#endif

#define AVR_DELAY_US            200
#define AVR_MAX_RETRIES         10

#define CMD_SYNC                0xAA
#define CMD_CLOSE               0xCC
#define CMD_FILL                0xFF

/* Actual command opcodes handled by AVR */
#define CMD_STATE               0xBB
#define CMD_VER                 0xBC
#define CMD_MONOTIME            0xBD
#define CMD_PGMWAKE             0xBF
#define CMD_HDQ_READ            0xC0
#define CMD_HDQ_WRITE           0xC1
#define CMD_HDQ_STATUS          0xC2
#define CMD_GET_LAST_RESET_TYPE 0xC4
#define CMD_UNKNOWN_C5          0xC5
#define CMD_GET_BATTERY_TEMP    0xC8
#define CMD_LCM_POWER           0xC9
#define CMD_AMP_ENABLE          0xCA
#define CMD_WHEEL_EN            0xD0
#define CMD_SET_INTCHRG         0xD1
#define CMD_GET_INTCHRG         0xD2
#define CMD_WIFI_PD             0xD3
#define CMD_UNKNOWN_D4          0xD4
#define CMD_UNKNOWN_D5          0xD5
#define CMD_UNKNOWN_D6          0xD6
#define CMD_CODEC_RESET         0xD7
#define CMD_ADC_START           0xD8
#define CMD_ADC_RESULT          0xD9
#define CMD_SYS_CTRL            0xDA
#define CMD_SET_USBCHRG         0xE2
#define CMD_GET_USBCHRG         0xE3
#define CMD_MONORSTCNT          0xE4

/* CMD_LCM_POWER parameters */
#define LCM_POWER_OFF           0x00
#define LCM_POWER_ON            0x01
#define LCM_POWER_SLEEP         0x02
#define LCM_POWER_WAKE          0x03
#define LCM_REPOWER_ON          0x04

/* CMD_SYS_CTRL parameters */
#define SYS_CTRL_POWEROFF       0x00
#define SYS_CTRL_RESET          0x01
#define SYS_CTRL_SLEEP          0x02
#define SYS_CTRL_DISABLE_WD     0x03
#define SYS_CTRL_KICK_WD        0x04
#define SYS_CTRL_EN_HDQ_THERM   0x05
#define SYS_CTRL_EN_TS_THERM    0x06
#define SYS_CTRL_FRESET         0x80

/* HDQ status codes */
#define HDQ_STATUS_OK           0x00
#define HDQ_STATUS_NOT_READY    0x01
#define HDQ_STATUS_TIMEOUT      0x02

/* protects spi avr commands from concurrent access */
static struct mutex avr_mtx;
/* serializes hdq read/write and status retrieval */
static struct mutex hdq_mtx;

/* AVR thread events */
#define INPUT_INTERRUPT          1
#define MONOTIME_OFFSET_UPDATE   2
#define POWER_OFF_REQUEST        3
static int btn = 0;
static bool hold_switch;
static bool input_interrupt_pending;
/* AVR implements 32-bit counter incremented every second.
 * The counter value cannot be modified to arbitrary value,
 * so the epoch offset needs to be stored in a file.
 */
#define MONOTIME_OFFSET_FILE ROCKBOX_DIR "/monotime_offset.dat"
static uint32_t monotime_offset;
/* Buffer last read monotime value. Reading monotime takes
 * atleast 1400 us so the tick counter is used together with
 * last read monotime value to return current time.
 */
static bool monotime_available;
static uint32_t monotime_value;
static unsigned long monotime_value_tick;

static long avr_stack[DEFAULT_STACK_SIZE/sizeof(long)];
static const char avr_thread_name[] = "avr";
static struct event_queue avr_queue;

/* OF bootloader will refuse to start software if low power is set
 * Bits 3, 4, 5, 6 and 7 are unknown.
 */
#define BATTERY_STATUS_LOW_POWER         (1 << 2)
#define BATTERY_STATUS_CHARGER_CONNECTED (1 << 1)
#define BATTERY_STATUS_CHARGING          (1 << 0)
static uint8_t avr_battery_status;

#define BATTERY_LEVEL_NOT_DETECTED       (1 << 7)
#define BATTERY_LEVEL_PERCENTAGE_MASK     0x7F
static uint8_t avr_battery_level = 100;

static inline uint16_t be2short(uint8_t *buf)
{
   return (uint16_t)((buf[0] << 8) | buf[1]);
}

#define BUTTON_DIRECT_MASK (BUTTON_LEFT | BUTTON_UP | BUTTON_RIGHT | BUTTON_DOWN | BUTTON_SELECT | BUTTON_VOL_UP | BUTTON_VOL_DOWN | BUTTON_NEXT | BUTTON_PREV)

static void handle_wheel(uint8_t wheel)
{
    static int key = 0;
    static uint8_t velocity = 0;
    static uint32_t wheel_delta = 1ul << 24;
    static uint8_t wheel_prev = 0;
    static unsigned long nextbacklight_hw_on = 0;
    static int prev_key = -1;
    static int prev_key_post = 0;

    if (TIME_AFTER(current_tick, nextbacklight_hw_on))
    {
        backlight_on();
        reset_poweroff_timer();
        nextbacklight_hw_on = current_tick + HZ/4;
    }

    if (wheel_prev < wheel)
    {
        key = BUTTON_SCROLL_FWD;
        velocity = wheel - wheel_prev;
    }
    else if (wheel_prev > wheel)
    {
        key = BUTTON_SCROLL_BACK;
        velocity = wheel_prev - wheel;
    }

    if (prev_key != key && velocity < 2 /* filter "rewinds" */)
    {
        /* direction reversal */
        prev_key = key;
        wheel_delta = 1ul << 24;
        return;
    }

    /* TODO: take velocity into account */
    if (queue_empty(&button_queue))
    {
        if (prev_key_post == key)
        {
            key |= BUTTON_REPEAT;
        }

        /* Post directly, don't update btn as avr doesn't give
           interrupt on scroll stop */
        queue_post(&button_queue, key, wheel_delta);

        wheel_delta = 1ul << 24;

        prev_key_post = key;
    }
    else
    {
        /* skipped post - increment delta and limit to 7 bits */
        wheel_delta += 1ul << 24;

        if (wheel_delta > (0x7ful << 24))
            wheel_delta = 0x7ful << 24;
    }

    wheel_prev = wheel;

    prev_key = key;
}

/* buf must be 8-byte state array (reply from avr_hid_get_state() */
static void parse_button_state(uint8_t *state)
{
    uint16_t main_btns_state = be2short(&state[2]);
#ifdef BUTTON_DEBUG
    uint16_t main_btns_changed = be2short(&state[4]);
#endif

    /* make sure other bits doesn't conflict with our "free bits" buttons */
    main_btns_state &= BUTTON_DIRECT_MASK;

    if (state[1] & 0x01) /* is power button pressed? */
    {
        main_btns_state |= BUTTON_POWER;
    }

    btn = main_btns_state;

    /* check if stored hold_switch state changed (prevents lost changes) */
    if ((state[1] & 0x20) /* hold change notification */ ||
        (hold_switch != ((state[1] & 0x02) >> 1)))
    {
        hold_switch = (state[1] & 0x02) >> 1;
#ifdef BUTTON_DEBUG
        dbgprintf("HOLD changed (%d)", hold_switch);
#endif
#ifndef BOOTLOADER
        backlight_hold_changed(hold_switch);
#endif
    }

    if ((hold_switch == false) && (state[1] & 0x80)) /* scrollwheel change */
    {
        handle_wheel(state[0]);
    }

#ifdef BUTTON_DEBUG
    if (state[1] & 0x10) /* power button change */
    {
        /* power button state has changed */
        main_btns_changed |= BUTTON_POWER;
    }

    if (btn & BUTTON_LEFT)        dbgprintf("LEFT");
    if (btn & BUTTON_UP)          dbgprintf("UP");
    if (btn & BUTTON_RIGHT)       dbgprintf("RIGHT");
    if (btn & BUTTON_DOWN)        dbgprintf("DOWN");
    if (btn & BUTTON_SELECT)      dbgprintf("SELECT");
    if (btn & BUTTON_VOL_UP)      dbgprintf("VOL UP");
    if (btn & BUTTON_VOL_DOWN)    dbgprintf("VOL DOWN");
    if (btn & BUTTON_NEXT)        dbgprintf("NEXT");
    if (btn & BUTTON_PREV)        dbgprintf("PREV");
    if (btn & BUTTON_POWER)       dbgprintf("POWER");
    if (btn & BUTTON_HOLD)        dbgprintf("HOLD");
    if (btn & BUTTON_SCROLL_FWD)  dbgprintf("SCROLL FWD");
    if (btn & BUTTON_SCROLL_BACK) dbgprintf("SCROLL BACK");
#endif
}

static bool avr_command_reads_data(uint8_t opcode)
{
    switch (opcode)
    {
        case CMD_STATE:
        case CMD_VER:
        case CMD_GET_LAST_RESET_TYPE:
        case CMD_GET_INTCHRG:
        case CMD_MONOTIME:
        case CMD_UNKNOWN_C5:
        case CMD_MONORSTCNT:
        case CMD_HDQ_STATUS:
        case CMD_GET_BATTERY_TEMP:
        case CMD_GET_USBCHRG:
        case CMD_ADC_RESULT:
            return true;
        default:
            return false;
    }
}

static size_t avr_command_data_size(uint8_t opcode)
{
    switch (opcode)
    {
        case CMD_STATE:               return 8;
        case CMD_VER:                 return 1;
        case CMD_MONOTIME:            return 4;
        case CMD_PGMWAKE:             return 4;
        case CMD_HDQ_READ:            return 1;
        case CMD_HDQ_WRITE:           return 2;
        case CMD_HDQ_STATUS:          return 2;
        case CMD_GET_LAST_RESET_TYPE: return 1;
        case CMD_UNKNOWN_C5:          return 1;
        case CMD_GET_BATTERY_TEMP:    return 2;
        case CMD_LCM_POWER:           return 1;
        case CMD_AMP_ENABLE:          return 1;
        case CMD_WHEEL_EN:            return 1;
        case CMD_SET_INTCHRG:         return 1;
        case CMD_GET_INTCHRG:         return 1;
        case CMD_WIFI_PD:             return 1;
        case CMD_UNKNOWN_D4:          return 1;
        case CMD_UNKNOWN_D5:          return 2;
        case CMD_UNKNOWN_D6:          return 2;
        case CMD_CODEC_RESET:         return 0;
        case CMD_ADC_START:           return 1;
        case CMD_ADC_RESULT:          return 2;
        case CMD_SYS_CTRL:            return 1;
        case CMD_SET_USBCHRG:         return 1;
        case CMD_GET_USBCHRG:         return 1;
        case CMD_MONORSTCNT:          return 2;
        default:
            panicf("Invalid AVR opcode %02X", opcode);
            return 0;
    }
}

static uint8_t spi_read_byte(void)
{
    uint16_t rxdata;

    do
    {
        rxdata = IO_SERIAL1_RX_DATA;
    }
    while (rxdata & (1<<8));

    return rxdata & 0xFF;
}

static void avr_hid_select(void)
{
    /* Drive GIO29 (AVR SS) low */
    IO_GIO_BITCLR1 = (1 << 13);
}

static void avr_hid_release(void)
{
    /* Drive GIO29 (AVR SS) high */
    IO_GIO_BITSET1 = (1 << 13);
}

static bool avr_run_command(uint8_t opcode, uint8_t *data, size_t data_length)
{
    bool success = true;
    const bool is_read = avr_command_reads_data(opcode);
    size_t i;
    uint8_t rx;

    /* Verify command data size and also make sure command is valid */
    if (avr_command_data_size(opcode) != data_length)
    {
        panicf("AVR %02x invalid data length", opcode);
    }

    mutex_lock(&avr_mtx);

    bitset16(&IO_CLK_MOD2, CLK_MOD2_SIF1);
    IO_SERIAL1_TX_ENABLE = 0x0001;

    avr_hid_select();
    udelay(10);

    IO_SERIAL1_TX_DATA = CMD_SYNC;
    spi_read_byte();
    /* Allow AVR to process CMD_SYNC */
    udelay(AVR_DELAY_US);

    IO_SERIAL1_TX_DATA = opcode;
    rx = spi_read_byte();
    if (rx != CMD_SYNC)
    {
        /* AVR failed to register CMD_SYNC */
        success = false;
    }
    /* Allow AVR to process opcode */
    udelay(AVR_DELAY_US);

    if (is_read)
    {
        for (i = 0; i < data_length; i++)
        {
            IO_SERIAL1_TX_DATA = CMD_FILL;
            data[i] = spi_read_byte();
            udelay(AVR_DELAY_US);
        }
    }
    else
    {
        for (i = 0; i < data_length; i++)
        {
            IO_SERIAL1_TX_DATA = data[i];
            spi_read_byte();
            udelay(AVR_DELAY_US);
        }
    }

    IO_SERIAL1_TX_DATA = CMD_CLOSE;
    rx = spi_read_byte();
    udelay(AVR_DELAY_US);
    if (is_read)
    {
        success = success && (rx == CMD_CLOSE);
    }

    avr_hid_release();

    IO_SERIAL1_TX_ENABLE = 0;
    bitclr16(&IO_CLK_MOD2, CLK_MOD2_SIF1);

    mutex_unlock(&avr_mtx);

    return success;
}

static bool avr_hid_get_state(void)
{
    uint8_t state[8];
    if (avr_run_command(CMD_STATE, state, sizeof(state)))
    {
        avr_battery_status = state[6];
        avr_battery_level = state[7];
        parse_button_state(state);
        return true;
    }
    return false;
}

static bool avr_hid_sync(void)
{
    int retry;
    for (retry = 0; retry < AVR_MAX_RETRIES; retry++)
    {
        if (avr_hid_get_state())
        {
            return true;
        }
        mdelay(100);
    }
    /* TODO: Program HID as it appears to be not programmed.
     * To do so, unfortunately, AVR firmware would have to be written
     * from scratch as OF blob cannot be used due to licensing.
     */
    return false;
}

static bool avr_execute_command(uint8_t opcode, uint8_t *data, size_t data_length)
{
    int retry;
    for (retry = 0; retry < AVR_MAX_RETRIES; retry++)
    {
        if (avr_run_command(opcode, data, data_length))
        {
            return true;
        }

        /* Resync and try again */
        avr_hid_sync();
    }
    return false;
}

void avr_hid_init(void)
{
    /*
       setup alternate GIO functions:
       GIO30 - SIF1 Clock
       GIO31 - SIF1 Data In
       GIO32 - SIF1 Data Out
       Manually drive GIO29 output (directly connected to AVR's SS).
    */
    IO_GIO_FSEL2 = (IO_GIO_FSEL2 & 0x00FF) | 0xA800;
    /* GIO29, GIO30 - outputs, GIO31 - input */
    IO_GIO_DIR1 = (IO_GIO_DIR1 & ~((1 << 13) | (1 << 14))) | (1 << 15);
    /* GIO32 - output */
    bitclr16(&IO_GIO_DIR2, (1 << 0));
    avr_hid_release();

    /* Master, MSB first, RATE = 219 (Bit rate = ARM clock / 2*(RATE + 1)))
     * Boosted 148.5 MHz / 440 = 337.5 kHz
     * Default 74.25 MHz / 440 = 168.75 kHz
     */
    IO_SERIAL1_MODE = 0x6DB;

    mutex_init(&avr_mtx);
    mutex_init(&hdq_mtx);
}

int _battery_level(void)
{
    /* OF still plays music when level is at 0 */
    if (avr_battery_level & BATTERY_LEVEL_NOT_DETECTED)
    {
        return 0;
    }
    return avr_battery_level & BATTERY_LEVEL_PERCENTAGE_MASK;
}

int _battery_voltage(void)
{
    return avr_hid_hdq_read_short(HDQ_REG_VOLT);
}

int _battery_time(void)
{
    /* HDQ_REG_TTE reads as 65535 when charging */
    return avr_hid_hdq_read_short(HDQ_REG_TTE);
}

unsigned int power_input_status(void)
{
    if (avr_battery_status & BATTERY_STATUS_CHARGER_CONNECTED)
    {
        return POWER_INPUT_USB_CHARGER;
    }
    return POWER_INPUT_NONE;
}

bool charging_state(void)
{
    return (avr_battery_status & BATTERY_STATUS_CHARGING) != 0;
}

static int avr_hid_hdq_read_byte_internal(uint8_t address)
{
    uint8_t result[2];

    if (!avr_execute_command(CMD_HDQ_READ, &address, sizeof(address)))
    {
        return -1;
    }

    do
    {
        mdelay(10);
        if (!avr_execute_command(CMD_HDQ_STATUS, result, sizeof(result)))
        {
            return -1;
        }
    }
    while (result[0] == HDQ_STATUS_NOT_READY);

    if (result[0] != HDQ_STATUS_OK)
    {
        logf("HDQ read %d status %d", address, result[0]);
        return -1;
    }

    return result[1];
}

int avr_hid_hdq_read_byte(uint8_t address)
{
    int retry;
    int value = -1;
    for (retry = 0; (retry < 3) && (value < 0); retry++)
    {
        mutex_lock(&hdq_mtx);
        value = avr_hid_hdq_read_byte_internal(address);
        mutex_unlock(&hdq_mtx);
    }
    return value;
}

int avr_hid_hdq_read_short(uint8_t address)
{
    int old_hi = -1, old_lo = -1, hi = -2, lo = -2;
    /* Keep reading until we read the same value twice.
     * There's no atomic 16-bit value retrieval, so keep reading
     * until we read the same value twice. HDQ registers update
     * no more than once per 2.56 seconds so usually there will
     * be 4 reads and sometimes 6 reads.
     */
    while ((old_hi != hi) || (old_lo != lo))
    {
        old_hi = hi;
        old_lo = lo;
        hi = avr_hid_hdq_read_byte(address + 1);
        lo = avr_hid_hdq_read_byte(address);
    }
    if ((hi < 0) || (lo < 0))
    {
        return -1;
    }
    return (hi << 8) | lo;
}

static void avr_hid_enable_wheel(void)
{
    uint8_t enable = 0x01;
    avr_execute_command(CMD_WHEEL_EN, &enable, sizeof(enable));
}

/* command that is sent by "hidtool -J 1" issued on every OF boot */
void avr_hid_enable_charger(void)
{
    uint8_t enable = 0x01;
    avr_execute_command(CMD_SET_INTCHRG, &enable, sizeof(enable));
}

void avr_hid_wifi_pd(int high)
{
    uint8_t state = high ? 0x01 : 0x00;
    avr_execute_command(CMD_WIFI_PD, &state, sizeof(state));
}

static void avr_hid_lcm_power(uint8_t parameter)
{
    avr_execute_command(CMD_LCM_POWER, &parameter, sizeof(parameter));
}

void avr_hid_lcm_sleep(void)
{
    avr_hid_lcm_power(LCM_POWER_SLEEP);
}

void avr_hid_lcm_wake(void)
{
    avr_hid_lcm_power(LCM_POWER_WAKE);
}

void avr_hid_lcm_power_on(void)
{
    avr_hid_lcm_power(LCM_POWER_ON);
}

void avr_hid_lcm_power_off(void)
{
    avr_hid_lcm_power(LCM_POWER_OFF);
}

void avr_hid_reset_codec(void)
{
    avr_execute_command(CMD_CODEC_RESET, NULL, 0);
}

void avr_hid_set_amp_enable(uint8_t enable)
{
    avr_execute_command(CMD_AMP_ENABLE, &enable, sizeof(enable));
}

static void avr_hid_sys_ctrl(uint8_t parameter)
{
    avr_execute_command(CMD_SYS_CTRL, &parameter, sizeof(parameter));
}

void avr_hid_power_off(void)
{
    /* Do not execute command directly here because we can get called inside
     * tick task context that must not acquire mutex.
     */
    queue_post(&avr_queue, POWER_OFF_REQUEST, 0);
}

static bool avr_state_changed(void)
{
    return (IO_GIO_BITSET0 & 0x1) ? false : true;
}

static bool headphones_inserted(void)
{
    return (IO_GIO_BITSET0 & 0x04) ? false : true;
}

static void set_audio_output(bool headphones)
{
#ifdef BOOTLOADER
    (void)headphones;
#else
    if (headphones)
    {
        /* Stereo output on headphones */
        avr_hid_set_amp_enable(0);
        aic3x_switch_output(true);
    }
    else
    {
        /* Mono output on built-in speaker */
        aic3x_switch_output(false);
        avr_hid_set_amp_enable(1);
    }
#endif
}

static void read_monotime_offset(void)
{
    int fd = open(MONOTIME_OFFSET_FILE, O_RDONLY);
    if (fd >= 0)
    {
        uint32_t offset;
        if (sizeof(offset) == read(fd, &offset, sizeof(offset)))
        {
            monotime_offset = offset;
        }
        close(fd);
    }
}

static bool write_monotime_offset(void)
{
    bool success = false;
    int fd = open(MONOTIME_OFFSET_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd >= 0)
    {
        uint32_t offset = monotime_offset;
        if (sizeof(monotime_offset) == write(fd, &offset, sizeof(offset)))
        {
            success = true;
        }
        close(fd);
    }
    return success;
}

static void read_monotime(void)
{
    uint8_t tmp[4];
    uint32_t t1, t2;

    if (!avr_execute_command(CMD_MONOTIME, tmp, sizeof(tmp)))
    {
        return;
    }
    t1 = (tmp[0]) | (tmp[1] << 8) | (tmp[2] << 16) | (tmp[3] << 24);

    if (!avr_execute_command(CMD_MONOTIME, tmp, sizeof(tmp)))
    {
        return;
    }
    t2 = (tmp[0]) | (tmp[1] << 8) | (tmp[2] << 16) | (tmp[3] << 24);

    if ((t1 == t2) || (t1 + 1 == t2))
    {
        int flags = disable_irq_save();
        monotime_value = t2;
        monotime_value_tick = current_tick;
        restore_irq(flags);
        monotime_available = true;
    }
}

static time_t get_timestamp(void)
{
    time_t timestamp;
    if (!monotime_available)
    {
        read_monotime();
    }
    if (monotime_available)
    {
        int flags = disable_irq_save();
        timestamp = monotime_value;
        timestamp += monotime_offset;
        timestamp += ((current_tick - monotime_value_tick) / HZ);
        restore_irq(flags);
        return timestamp;
    }
    return 0;
}

void rtc_init(void)
{
    /* This is called before disk is mounted */
}

int rtc_read_datetime(struct tm *tm)
{
    time_t time = get_timestamp();
    gmtime_r(&time, tm);
    return 1;
}

int rtc_write_datetime(const struct tm *tm)
{
    time_t offset = mktime((struct tm *)tm);
    int flags = disable_irq_save();
    offset -= monotime_value;
    offset -= ((current_tick - monotime_value_tick) / HZ);
    monotime_offset = offset;
    restore_irq(flags);
    queue_post(&avr_queue, MONOTIME_OFFSET_UPDATE, 0);
    return 1;
}

void avr_thread(void)
{
    struct queue_event ev;
    bool headphones_active_state = headphones_inserted();
    bool headphones_state;
    bool disk_access_available = true;
    bool monotime_offset_update_pending = false;

    set_audio_output(headphones_active_state);
    read_monotime_offset();
    read_monotime();

    while (1)
    {
        if (avr_state_changed())
        {
            /* We have to read AVR state, simply check if there's any event
             * pending but do not block. It is possible that AVR interrupt
             * line is held active even though we read the state (change
             * occured during read).
             */
            queue_wait_w_tmo(&avr_queue, &ev, 0);
        }
        else
        {
            queue_wait(&avr_queue, &ev);
        }

        if (ev.id == SYS_USB_CONNECTED)
        {
            /* Allow USB to gain exclusive storage access */
            usb_acknowledge(SYS_USB_CONNECTED_ACK);
            disk_access_available = false;
        }
        else if (ev.id == SYS_USB_DISCONNECTED)
        {
            disk_access_available = true;
        }
        else if (ev.id == MONOTIME_OFFSET_UPDATE)
        {
            monotime_offset_update_pending = true;
        }
        else if (ev.id == POWER_OFF_REQUEST)
        {
            avr_hid_reset_codec();
            avr_hid_sys_ctrl(SYS_CTRL_POWEROFF);
        }

        input_interrupt_pending = false;
        if (avr_state_changed())
        {
            /* Read buttons state */
            avr_hid_get_state();
        }

        headphones_state = headphones_inserted();
        if (headphones_state != headphones_active_state)
        {
            set_audio_output(headphones_state);
            headphones_active_state = headphones_state;
        }

        if (disk_access_available)
        {
            if (monotime_offset_update_pending && write_monotime_offset())
            {
                monotime_offset_update_pending = false;
            }
        }

        /* Update buffered monotime value every hour */
        if (TIME_AFTER(current_tick, monotime_value_tick + 3600 * HZ))
        {
            read_monotime();
        }
    }
}

void GIO0(void) __attribute__ ((section(".icode")));
void GIO0(void)
{
    /* Clear interrupt */
    IO_INTC_IRQ1 = (1 << 5);

    if (!input_interrupt_pending)
    {
        input_interrupt_pending = true;
        queue_post(&avr_queue, INPUT_INTERRUPT, 0);
    }
}

void GIO2(void) __attribute__ ((section(".icode")));
void GIO2(void)
{
    /* Clear interrupt */
    IO_INTC_IRQ1 = (1 << 7);

    /* Prevent event queue overflow by allowing just one pending event */
    if (!input_interrupt_pending)
    {
        input_interrupt_pending = true;
        queue_post(&avr_queue, INPUT_INTERRUPT, 0);
    }
}

void button_init_device(void)
{
    btn = 0;
    hold_switch = false;

    queue_init(&avr_queue, true);
    input_interrupt_pending = true;
    queue_post(&avr_queue, INPUT_INTERRUPT, 0);
    create_thread(avr_thread, avr_stack, sizeof(avr_stack), 0,
                  avr_thread_name IF_PRIO(, PRIORITY_USER_INTERFACE)
                  IF_COP(, CPU));

    IO_GIO_DIR0 |= 0x01; /* Set GIO0 as input */

    /* Get in sync with AVR */
    avr_hid_sync();

    /* Enable wheel */
    avr_hid_enable_wheel();
    /* Read button status and tell avr we want interrupt on next change */
    avr_hid_get_state();

    IO_GIO_IRQPORT |= 0x05; /* Enable GIO0/GIO2 external interrupt */
    IO_GIO_INV0 &= ~0x05; /* Clear INV for GIO0/GIO2 */
    /* falling edge detection on GIO0, any edge on GIO2 */
    IO_GIO_IRQEDGE = (IO_GIO_IRQEDGE & ~0x01) | 0x04;

    /* Enable GIO0 and GIO2 interrupts */
    IO_INTC_EINT1 |= INTR_EINT1_EXT0 | INTR_EINT1_EXT2;
}

int button_read_device(void)
{
    if(hold_switch)
        return 0;
    else
        return btn;
}

bool button_hold(void)
{
    return hold_switch;
}
