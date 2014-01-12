/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Mark Arigo
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
#include "kernel.h"
#include "thread.h"
#include "system.h"
#include "lcd-remote.h"
#include "lcd-remote-target.h"
#include "button.h"
#include "button-target.h"

/* Temporary defines until we sort out why the gui stuff doesn't like this size
   (I believe the status bar isn't doing sanity checks and is writing outside
   the frame buffer size). */
#define RC_WIDTH                79
#define RC_HEIGHT               16

#define RC_CONTRAST_MASK        0x000000ff
#define RC_SCREEN_ON            0x00000100
#define RC_BACKLIGHT_ON         0x00000200

#define RC_DEV_INIT             0x00001000
#define RC_DETECTED             0x00002000
#define RC_AWAKE                0x00004000
#define RC_POWER_OFF            0x00008000

#define RC_UPDATE_LCD           0x00010000
#define RC_UPDATE_CONTROLLER    0x00020000
#define RC_UPDATE_ICONS         0x00040000
#define RC_UPDATE_MASK          (RC_UPDATE_LCD|RC_UPDATE_CONTROLLER|RC_UPDATE_ICONS)

#define RC_TX_ERROR             0x00100000
#define RC_RX_ERROR             0x00200000
#define RC_TIMEOUT_ERROR        0x00400000
#define RC_ERROR_MASK           (RC_TX_ERROR|RC_RX_ERROR|RC_TIMEOUT_ERROR)

#define RC_FORCE_DETECT         0x80000000

#define RX_READY                0x01
#define TX_READY                0x20

#define POLL_TIMEOUT            50000

bool remote_initialized = false;
unsigned int rc_status = 0;
unsigned char rc_buf[5];

/* ================================================== */
/* Remote thread functions                            */
/*   These functions are private to the remote thread */
/* ================================================== */
static struct semaphore rc_thread_wakeup;
static unsigned int remote_thread_id;
static int remote_stack[512/sizeof(int)];
static const char * const remote_thread_name = "remote";

static bool remote_wait_ready(int ready_mask)
{
    unsigned long current;
    unsigned long start = USEC_TIMER;
    unsigned long timeout = start + POLL_TIMEOUT;

    rc_status &= ~RC_TIMEOUT_ERROR;

    if (start <= timeout)
    {
        do
        {
            if (SER1_LSR & ready_mask)
                return true;

            //~ sleep(1);

            current = USEC_TIMER;
        } while (current < timeout);
    }
    else
    {
        do
        {
            if (SER1_LSR & ready_mask)
                return true;

            //~ sleep(1);

            current = USEC_TIMER - POLL_TIMEOUT;
        } while (current < start);
    }

    rc_status |= RC_TIMEOUT_ERROR;
    return false;
}

static bool remote_rx(void)
{
    int i;
    unsigned char chksum[2];

    rc_status &= ~RC_RX_ERROR;

    for (i = 0; i < 5; i++)
    {
        if (!remote_wait_ready(RX_READY))
        {
            rc_status |= RC_RX_ERROR;
            return false;
        }

        rc_buf[i] = SER1_RBR;
    }

    /* check opcode */
    if ((rc_buf[0] & 0xf0) != 0xf0)
    {
        rc_status |= RC_RX_ERROR;
        return false;
    }

    /* verify the checksums */
    chksum[0] = chksum[1] = 0;
    for (i = 0; i < 3; i++)
    {
        chksum[0] ^= rc_buf[i];
        chksum[1] += rc_buf[i];
    }

    if ((chksum[0] != rc_buf[3]) && (chksum[1] != rc_buf[4]))
    {
        rc_status |= RC_RX_ERROR;
        return false;
    }

    /* reception error */
    if ((rc_buf[0] & 0x1) || (rc_buf[0] & 0x2) || (rc_buf[0] & 0x4))
    {
        rc_status |= RC_RX_ERROR;
        return false;
    }

    return true;
}

static bool remote_tx(unsigned char *data, int len)
{
    int i;
    unsigned char chksum[2];

    rc_status &= ~RC_TX_ERROR;
    
    chksum[0] = chksum[1] = 0;

    for (i = 0; i < len; i++)
    {
        if (!remote_wait_ready(TX_READY))
        {
            rc_status |= RC_TX_ERROR;
            return false;
        }

        SER1_THR = data[i];
        chksum[0] ^= data[i];
        chksum[1] += data[i];
    }

    for (i = 0; i < 2; i++)
    {
        if (!remote_wait_ready(TX_READY))
        {
            rc_status |= RC_TX_ERROR;
            return false;
        }

        SER1_THR = chksum[i];
    }

    return remote_rx();
}

static void remote_dev_enable(bool enable)
{
    if (enable)
    {
        outl(inl(0x70000018) | 0xaa000, 0x70000018);
        DEV_INIT2 &= ~0x800;

        GPIO_CLEAR_BITWISE(GPIOL_OUTPUT_VAL, 0x80);
        GPIO_SET_BITWISE(GPIOL_OUTPUT_EN, 0x80);

        DEV_EN |= DEV_SER1;

        SER1_RBR;
        SER1_LCR = 0x80;
        SER1_DLL = 0x50;
        SER1_DLM = 0x00;
        SER1_LCR = 0x03;
        SER1_FCR = 0x07;

        rc_status |= RC_DEV_INIT;
    }
    else
    {
        outl(inl(0x70000018) & ~0xaa000, 0x70000018);
        DEV_INIT2 &= ~0x800;

        GPIO_SET_BITWISE(GPIOL_OUTPUT_VAL, 0x80);
        GPIO_SET_BITWISE(GPIOL_OUTPUT_EN, 0x80);

        DEV_RS |= DEV_SER1;
        nop;
        DEV_RS &= ~DEV_SER1;

        DEV_EN &= ~DEV_SER1;

        rc_status &= ~RC_DEV_INIT;
    }
}

static void remote_update_lcd(void)
{
    int x, y, draw_now;
    unsigned char data[RC_WIDTH + 7];

    /* If the draw_now bit is set, the draw occurs directly on the LCD.
       Otherwise, the data is stored in an off-screen buffer and displayed
       the next time a draw operation is executed with this flag set. */
    draw_now = 0;

    for (y = 0; y < 2; y++)
    {
        data[0] = 0x51;
        data[1] = draw_now << 7;
        data[2] = RC_WIDTH;         /* width */
        data[3] = 0;                /* x1    */
        data[4] = y << 3;           /* y1    */
        data[5] = RC_WIDTH;         /* x2    */
        data[6] = (y + 1) << 3;     /* y2    */

        for (x = 0; x < RC_WIDTH; x++)
            data[x + 7] = *FBREMOTEADDR(x,y);

        remote_tx(data, RC_WIDTH + 7);

        draw_now = 1;
    }
}

static void remote_update_controller(void)
{
    unsigned char data[3];

    data[0] = 0x31;
    data[1] = 0x00;
    if (rc_status & RC_SCREEN_ON)
        data[1] |= 0x80;
    if (rc_status & RC_BACKLIGHT_ON)
        data[1] |= 0x40;
    data[2] = (unsigned char)(rc_status & RC_CONTRAST_MASK);
    remote_tx(data, 3);
}

#if 0
static void remote_update_icons(unsigned char symbols)
{
    unsigned char data[2];

    if (!(rc_status & RC_AWAKE) && !(rc_status & RC_SCREEN_ON))
        return;

    data[0] = 0x41;
    data[1] = symbols;
    remote_tx(data, 2);
}
#endif

static bool remote_nop(void)
{
    unsigned char val[2];

    val[0] = 0x11;
    val[1] = 0x30;
    return remote_tx(val, 2);
}

static void remote_wake(void)
{
    if (remote_nop())
    {
        rc_status |= RC_AWAKE;
        return;
    }

    rc_status &= ~RC_AWAKE;
    return;
}

static void remote_sleep(void)
{
    unsigned char data[2];

    if (rc_status & RC_AWAKE)
    {
        data[0] = 0x71;
        data[1] = 0x30;
        remote_tx(data, 2);

        udelay(25000);
    }

    rc_status &= ~RC_AWAKE;
}

static void remote_off(void)
{
    if (rc_status & RC_AWAKE)
        remote_sleep();

    if (rc_status & RC_DEV_INIT)
        remote_dev_enable(false);

    rc_status &= ~(RC_DETECTED | RC_ERROR_MASK | RC_UPDATE_MASK);
    remote_initialized = false;
}

static void remote_on(void)
{
    if (!(rc_status & RC_DEV_INIT))
        remote_dev_enable(true);

    remote_wake();

    if (rc_status & RC_AWAKE)
    {
        rc_status |= RC_DETECTED;
        remote_initialized = true;

        rc_status |= (RC_UPDATE_MASK | RC_SCREEN_ON | RC_BACKLIGHT_ON);
        //~ remote_update_icons(0xf0); /* show battery */
    }
    else
    {
        rc_status &= ~RC_DETECTED;
        remote_initialized = false;
    }
}

static void remote_thread(void)
{
    int rc_thread_sleep_count = 10;
    int rc_thread_wait_timeout = TIMEOUT_BLOCK;

    while (1)
    {
        semaphore_wait(&rc_thread_wakeup, rc_thread_wait_timeout);

        /* Error handling (most likely due to remote not present) */
        if (rc_status & RC_ERROR_MASK)
        {
            if (--rc_thread_sleep_count == 0)
                rc_status |= RC_POWER_OFF;
        }

        /* Power-off (thread sleeps) */
        if (rc_status & RC_POWER_OFF)
        {
            remote_off();

            rc_thread_sleep_count = 10;
            rc_thread_wait_timeout = TIMEOUT_BLOCK;

            continue;
        }

        /* Detection */
        if (!(rc_status & RC_DETECTED))
        {
            rc_thread_wait_timeout = HZ;

            if (headphones_inserted())
            {
                remote_on();

                if (rc_status & RC_AWAKE)
                {
                    rc_thread_sleep_count = 10;
                    rc_thread_wait_timeout = HZ/20; /* ~50ms for updates */
                }
            }
            else
            {
                if (--rc_thread_sleep_count == 0)
                    rc_status &= ~RC_POWER_OFF;
            }

            continue;
        }

        /* Update the remote (one per wakeup cycle) */
        if (headphones_inserted() && (rc_status & RC_AWAKE))
        {
            if (rc_status & RC_SCREEN_ON)
            {
                /* In order of importance */
                if (rc_status & RC_UPDATE_CONTROLLER)
                {
                    remote_update_controller();
                    rc_status &= ~RC_UPDATE_CONTROLLER;
                }
                else if (rc_status & RC_UPDATE_LCD)
                {
                    remote_update_lcd();
                    rc_status &= ~RC_UPDATE_LCD;
                }
                else
                {
                    remote_nop();
                }
            }
            else
            {
                remote_nop();
            }
        }
    }
}

/* ============================================= */
/* Public functions                              */
/*   These should only set the update flags that */
/*   will be executed in the remote thread.      */
/* ============================================= */
bool lcd_remote_read_device(unsigned char *data)
{
    if (!(rc_status & RC_AWAKE) || (rc_status & RC_ERROR_MASK))
        return false;

    /* Return the most recent data. While the remote is plugged,
       this is updated ~50ms */
    data[0] = rc_buf[0];
    data[1] = rc_buf[1];
    data[2] = rc_buf[2];
    data[3] = rc_buf[3];
    data[4] = rc_buf[4];

    return true;
}

void lcd_remote_set_invert_display(bool yesno)
{
    /* dummy function...need to introduce HAVE_LCD_REMOTE_INVERT */
    (void)yesno;
}

/* turn the display upside down (call lcd_remote_update() afterwards) */
void lcd_remote_set_flip(bool yesno)
{
    /* dummy function...need to introduce HAVE_LCD_REMOTE_FLIP */
    (void)yesno;
}

int lcd_remote_default_contrast(void)
{
    return DEFAULT_REMOTE_CONTRAST_SETTING;
}

void lcd_remote_set_contrast(int val)
{
    rc_status = (rc_status & ~RC_CONTRAST_MASK) | (val & RC_CONTRAST_MASK);
    rc_status |= RC_UPDATE_CONTROLLER;
}

void lcd_remote_off(void)
{
    /* should only be used to power off at shutdown */
    rc_status |= RC_POWER_OFF;
    semaphore_release(&rc_thread_wakeup);

    /* wait until the things are powered off */
    while (rc_status & RC_DEV_INIT)
        sleep(HZ/10);
}

void lcd_remote_on(void)
{
    /* Only wake the remote thread if it's in the blocked state. */
    struct thread_entry *rc_thread = thread_id_entry(remote_thread_id);
    if (rc_thread->state == STATE_BLOCKED || (rc_status & RC_FORCE_DETECT))
    {
        rc_status &= ~RC_FORCE_DETECT;
        rc_status &= ~RC_POWER_OFF;
        semaphore_release(&rc_thread_wakeup);
    }
}

bool remote_detect(void)
{
    return (rc_status & RC_DETECTED);
}

void lcd_remote_init_device(void)
{
    /* reset */
    remote_dev_enable(false);
    rc_status |= RC_FORCE_DETECT; /* force detection at startup */

    /* unknown */
    GPIO_SET_BITWISE(GPIOL_ENABLE, 0x80);
    GPIO_CLEAR_BITWISE(GPIOL_OUTPUT_VAL, 0x80);
    GPIO_SET_BITWISE(GPIOL_OUTPUT_EN, 0x80);

    /* a thread is required to poll & update the remote */
    semaphore_init(&rc_thread_wakeup, 1, 0);
    remote_thread_id = create_thread(remote_thread, remote_stack,
                            sizeof(remote_stack), 0, remote_thread_name
                            IF_PRIO(, PRIORITY_SYSTEM)
                            IF_COP(, CPU));
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_remote_update(void)
{
    rc_status |= RC_UPDATE_LCD;
}

/* Update a fraction of the display. */
void lcd_remote_update_rect(int x, int y, int width, int height)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;

    rc_status |= RC_UPDATE_LCD;
}

void _remote_backlight_on(void)
{
    rc_status |= RC_BACKLIGHT_ON;
    rc_status |= RC_UPDATE_CONTROLLER;
}

void _remote_backlight_off(void)
{
    rc_status &= ~RC_BACKLIGHT_ON;
    rc_status |= RC_UPDATE_CONTROLLER;
}
