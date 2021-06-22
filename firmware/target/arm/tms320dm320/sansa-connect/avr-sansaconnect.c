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
#include "system.h"
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

#define AVR_DELAY_US            100

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
#define CMD_UNKNOWN_D3          0xD3
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

/* protects spi avr commands from concurrent access */
static struct mutex avr_mtx;

/* buttons thread */
#define BTN_INTERRUPT 1
static int btn = 0;
static bool hold_switch;
#ifndef BOOTLOADER
static long avr_stack[DEFAULT_STACK_SIZE/sizeof(long)];
static const char avr_thread_name[] = "avr";
static struct semaphore avr_thread_trigger;
#endif

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

#ifndef BOOTLOADER
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
#endif

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

#ifndef BOOTLOADER
    /* check if stored hold_switch state changed (prevents lost changes) */
    if ((state[1] & 0x20) /* hold change notification */ ||
        (hold_switch != ((state[1] & 0x02) >> 1)))
    {
#endif
        hold_switch = (state[1] & 0x02) >> 1;
#ifdef BUTTON_DEBUG
        dbgprintf("HOLD changed (%d)", hold_switch);
#endif
#ifndef BOOTLOADER
        backlight_hold_changed(hold_switch);
    }
#endif
#ifndef BOOTLOADER
    if ((hold_switch == false) && (state[1] & 0x80)) /* scrollwheel change */
    {
        handle_wheel(state[0]);
    }
#endif

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
        case CMD_UNKNOWN_D3:          return 1;
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

    IO_SERIAL1_TX_ENABLE = 0;
    bitclr16(&IO_CLK_MOD2, CLK_MOD2_SIF1);

    mutex_unlock(&avr_mtx);

    return success;
}


static void avr_hid_sync(void)
{
    uint8_t data;
    while (!avr_run_command(CMD_VER, &data, sizeof(data)))
    {
        /* TODO: Program HID if failing for long time.
         * To do so, unfortunately, AVR firmware would have to be written
         * from scratch as OF blob cannot be used due to licensing.
         */
    }

}

static void avr_execute_command(uint8_t opcode, uint8_t *data, size_t data_length)
{
    while (!avr_run_command(opcode, data, data_length))
    {
        /* Resync and try again */
        avr_hid_sync();
    }
}

void avr_hid_init(void)
{
    /*
       setup alternate GIO functions:
       GIO29 - SIF1 Enable (Directly connected to AVR's SS)
       GIO30 - SIF1 Clock
       GIO31 - SIF1 Data In
       GIO32 - SIF1 Data Out
    */
    IO_GIO_FSEL2 = (IO_GIO_FSEL2 & 0x00FF) | 0xAA00;
    /* GIO29, GIO30 - outputs, GIO31 - input */
    IO_GIO_DIR1 = (IO_GIO_DIR1 & ~((1 << 13) | (1 << 14))) | (1 << 15);
    /* GIO32 - output */
    bitclr16(&IO_GIO_DIR2, (1 << 0));

    /* RATE = 219 (0xDB) -> 200 kHz */
    IO_SERIAL1_MODE = 0x6DB;

    mutex_init(&avr_mtx);
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

static void avr_hid_get_state(void)
{
    uint8_t state[8];
    avr_execute_command(CMD_STATE, state, sizeof(state));

    avr_battery_status = state[6];
    avr_battery_level = state[7];
    parse_button_state(state);
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
    avr_hid_sys_ctrl(SYS_CTRL_POWEROFF);
}

#ifndef BOOTLOADER
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
}

void avr_thread(void)
{
    bool headphones_active_state = headphones_inserted();
    bool headphones_state;

    set_audio_output(headphones_active_state);

    while (1)
    {
        semaphore_wait(&avr_thread_trigger, TIMEOUT_BLOCK);

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
    }
}

void GIO0(void) __attribute__ ((section(".icode")));
void GIO0(void)
{
    /* Clear interrupt */
    IO_INTC_IRQ1 = (1 << 5);

    semaphore_release(&avr_thread_trigger);
}

void GIO2(void) __attribute__ ((section(".icode")));
void GIO2(void)
{
    /* Clear interrupt */
    IO_INTC_IRQ1 = (1 << 7);

    semaphore_release(&avr_thread_trigger);
}
#endif

void button_init_device(void)
{
    btn = 0;
    hold_switch = false;
#ifndef BOOTLOADER
    semaphore_init(&avr_thread_trigger, 1, 1);
    create_thread(avr_thread, avr_stack, sizeof(avr_stack), 0,
                  avr_thread_name IF_PRIO(, PRIORITY_USER_INTERFACE)
                  IF_COP(, CPU));
#endif
    IO_GIO_DIR0 |= 0x01; /* Set GIO0 as input */

    /* Get in sync with AVR */
    avr_hid_sync();

    /* Enable wheel */
    avr_hid_enable_wheel();
    /* Read button status and tell avr we want interrupt on next change */
    avr_hid_get_state();

#ifndef BOOTLOADER
    IO_GIO_IRQPORT |= 0x05; /* Enable GIO0/GIO2 external interrupt */
    IO_GIO_INV0 &= ~0x05; /* Clear INV for GIO0/GIO2 */
    /* falling edge detection on GIO0, any edge on GIO2 */
    IO_GIO_IRQEDGE = (IO_GIO_IRQEDGE & ~0x01) | 0x04;

    /* Enable GIO0 and GIO2 interrupts */
    IO_INTC_EINT1 |= INTR_EINT1_EXT0 | INTR_EINT1_EXT2;
#endif
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

void lcd_enable(bool on)
{
    (void)on;
}

