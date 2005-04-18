/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/*
 * Rockbox button functions
 */

#include <stdlib.h>
#include "config.h"
#include "cpu.h"
#include "system.h"
#include "button.h"
#include "kernel.h"
#include "backlight.h"
#include "adc.h"
#include "serial.h"
#include "power.h"
#include "system.h"
#include "powermgmt.h"

struct event_queue button_queue;

static long lastbtn;   /* Last valid button status */
static long last_read; /* Last button status, for debouncing/filtering */
#ifdef HAVE_LCD_BITMAP
static bool flipped;  /* buttons can be flipped to match the LCD flip */
#endif

/* how often we check to see if a button is pressed */
#define POLL_FREQUENCY    HZ/100

/* how long until repeat kicks in */
#define REPEAT_START      30

/* the speed repeat starts at */
#define REPEAT_INTERVAL_START   16

/* speed repeat finishes at */
#define REPEAT_INTERVAL_FINISH  5

/* Number of repeated keys before shutting off */
#define POWEROFF_COUNT 10

static int button_read(void);

static void button_tick(void)
{
    static int tick = 0;
    static int count = 0;
    static int repeat_speed = REPEAT_INTERVAL_START;
    static int repeat_count = 0;
    static bool repeat = false;
    int diff;
    int btn;

#if !defined(HAVE_MMC) && (CONFIG_KEYPAD != IRIVER_H100_PAD) \
  && (CONFIG_KEYPAD != GMINI100_PAD)

    /* Post events for the remote control */
    btn = remote_control_rx();
    if(btn)
    {
        queue_post(&button_queue, btn, NULL);
    }
#endif

    /* only poll every X ticks */
    if ( ++tick >= POLL_FREQUENCY )
    {
        bool post = false;
        btn = button_read();

        /* Find out if a key has been released */
        diff = btn ^ lastbtn;
        if(diff && (btn & diff) == 0)
        {
            queue_post(&button_queue, BUTTON_REL | diff, NULL);
        }
        else
        {
            if ( btn )
            {
                /* normal keypress */
                if ( btn != lastbtn )
                {
                    post = true;
                    repeat = false;
                    repeat_speed = REPEAT_INTERVAL_START;

                }
                else /* repeat? */
                {
                    if ( repeat )
                    {
                        count--;
                        if (count == 0) {
                            post = true;
                            /* yes we have repeat */
                            repeat_speed--;
                            if (repeat_speed < REPEAT_INTERVAL_FINISH)
                                repeat_speed = REPEAT_INTERVAL_FINISH;
                            count = repeat_speed;

                            repeat_count++;

                            /* Send a SYS_POWEROFF event if we have a device
                               which doesn't shut down easily with the OFF
                               key */
#ifdef HAVE_SW_POWEROFF
                            if (btn == BUTTON_OFF &&
#ifndef IRIVER_H100
                                !charger_inserted() &&
#endif
                                repeat_count > POWEROFF_COUNT)
                                queue_post(&button_queue, SYS_POWEROFF, NULL);
#endif
                        }
                    }
                    else
                    {
                        if (count++ > REPEAT_START)
                        {
                            post = true;
                            repeat = true;
                            repeat_count = 0;
                            /* initial repeat */
                            count = REPEAT_INTERVAL_START;
                        }
                    }
                }
                if ( post )
                {
                    if (repeat)
                        queue_post(&button_queue, BUTTON_REPEAT | btn, NULL);
                    else
                        queue_post(&button_queue, btn, NULL);
#ifdef HAVE_REMOTE_LCD
                    if(btn & BUTTON_REMOTE)
                        remote_backlight_on();
                    else
#endif
                        backlight_on();

                    reset_poweroff_timer();
                }
            }
            else
            {
                repeat = false;
                count = 0;
            }
        }
        lastbtn = btn & ~(BUTTON_REL | BUTTON_REPEAT);
        tick = 0;
    }

    backlight_tick();
}

long button_get(bool block)
{
    struct event ev;

    if ( block || !queue_empty(&button_queue) )
    {
        queue_wait(&button_queue, &ev);
        return ev.id;
    }
    return BUTTON_NONE;
}

long button_get_w_tmo(int ticks)
{
    struct event ev;
    queue_wait_w_tmo(&button_queue, &ev, ticks);
    return (ev.id != SYS_TIMEOUT)? ev.id: BUTTON_NONE;
}

void button_init(void)
{
    /* hardware inits */
#if CONFIG_KEYPAD == IRIVER_H100_PAD
    /* Set GPIO33, GPIO37, GPIO38  and GPIO52 as general purpose inputs */
    GPIO1_FUNCTION |= 0x00100062;
    GPIO1_ENABLE &= ~0x00100062;
#elif CONFIG_KEYPAD == RECORDER_PAD
    /* Set PB4 and PB8 as input pins */
    PBCR1 &= 0xfffc;  /* PB8MD = 00 */
    PBCR2 &= 0xfcff;  /* PB4MD = 00 */
    PBIOR &= ~0x0110; /* Inputs */
#elif CONFIG_KEYPAD == PLAYER_PAD
    /* set PA5 and PA11 as input pins */
    PACR1 &= 0xff3f;  /* PA11MD = 00 */
    PACR2 &= 0xfbff;  /* PA5MD = 0 */
    PAIOR &= ~0x0820; /* Inputs */
#elif CONFIG_KEYPAD == ONDIO_PAD
    /* nothing to initialize here */
#elif CONFIG_KEYPAD == GMINI100_PAD
    /* nothing to initialize here */
#endif /* CONFIG_KEYPAD */

    queue_init(&button_queue);
    lastbtn = 0;
    tick_add_task(button_tick);
    reset_poweroff_timer();

#ifdef HAVE_LCD_BITMAP
    flipped = false;
#endif
}

#ifdef HAVE_LCD_BITMAP /* only bitmap displays can be flipped */
/*
 * helper function to swap UP/DOWN, LEFT/RIGHT (and F1/F3 for Recorder)
 */
static int button_flip(int button)
{
    int newbutton;

    newbutton = button &
        ~(BUTTON_UP | BUTTON_DOWN
        | BUTTON_LEFT | BUTTON_RIGHT
#if CONFIG_KEYPAD == RECORDER_PAD
        | BUTTON_F1 | BUTTON_F3
#endif
        );

    if (button & BUTTON_UP)
        newbutton |= BUTTON_DOWN;
    if (button & BUTTON_DOWN)
        newbutton |= BUTTON_UP;
    if (button & BUTTON_LEFT)
        newbutton |= BUTTON_RIGHT;
    if (button & BUTTON_RIGHT)
        newbutton |= BUTTON_LEFT;
#if CONFIG_KEYPAD == RECORDER_PAD
    if (button & BUTTON_F1)
        newbutton |= BUTTON_F3;
    if (button & BUTTON_F3)
        newbutton |= BUTTON_F1;
#endif

    return newbutton;
}

/*
 * set the flip attribute
 * better only call this when the queue is empty
 */
void button_set_flip(bool flip)
{
    if (flip != flipped) /* not the current setting */
    {
        /* avoid race condition with the button_tick() */
        int oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
        lastbtn = button_flip(lastbtn);
        flipped = flip;
        set_irq_level(oldlevel);
    }
}
#endif /* HAVE_LCD_BITMAP */

/*
 Archos hardware button hookup
 =============================

 Recorder / Recorder FM/V2
 -------------------------
 F1, F2, F3, UP:           connected to AN4 through a resistor network
 DOWN, PLAY, LEFT, RIGHT:  likewise connected to AN5

 The voltage on AN4/ AN5 depends on which keys (or key combo) is pressed
 FM/V2 has PLAY and RIGHT switched compared to plain recorder

 ON:     PB8, low active (plain recorder) / AN3, low active (fm/v2)
 OFF:    PB4, low active (plain recorder) / AN2, high active (fm/v2)

 Player
 ------
 LEFT:   AN0
 MENU:   AN1
 RIGHT:  AN2
 PLAY:   AN3

 STOP:   PA11
 ON:     PA5

 All buttons are low active

 Ondio
 -----
 LEFT, RIGHT, UP, DOWN:    connected to AN4 through a resistor network

 The voltage on AN4 depends on which keys (or key combo) is pressed

 OPTION: AN2, high active (assigned as MENU)
 ON/OFF: AN3, low active (assigned as OFF)

*/

#if CONFIG_KEYPAD == RECORDER_PAD

#ifdef HAVE_FMADC
/* FM Recorder super-special levels */
#define LEVEL1        150
#define LEVEL2        385
#define LEVEL3        545
#define LEVEL4        700
#define ROW2_BUTTON1  BUTTON_PLAY
#define ROW2_BUTTON3  BUTTON_RIGHT

#else
/* plain bog standard Recorder levels */
#define LEVEL1        250
#define LEVEL2        500
#define LEVEL3        700
#define LEVEL4        900
#define ROW2_BUTTON1  BUTTON_RIGHT
#define ROW2_BUTTON3  BUTTON_PLAY
#endif /* HAVE_FMADC */

#elif CONFIG_KEYPAD == ONDIO_PAD
/* Ondio levels */
#define LEVEL1        165
#define LEVEL2        415
#define LEVEL3        585
#define LEVEL4        755

#endif /* CONFIG_KEYPAD */

/*
 * Get button pressed from hardware
 */
static int button_read(void)
{
    int btn = BUTTON_NONE;
    int retval;

    int data;

#if CONFIG_KEYPAD == IRIVER_H100_PAD

    data = adc_scan(ADC_BUTTONS);

    if (data < 0x80)
        if (data < 0x30)
            if (data < 0x18)
                btn = BUTTON_SELECT;
            else
                btn = BUTTON_UP;
        else
            if (data < 0x50)
                btn = BUTTON_LEFT;
            else
                btn = BUTTON_DOWN;
    else
        if (data < 0xb0)
            if (data < 0xa0)
                btn = BUTTON_RIGHT;
            else
                btn = BUTTON_OFF;
        else
            if (data < 0xd0)
                btn = BUTTON_MODE;
            else
                if (data < 0xf0)
                    btn = BUTTON_REC;

    data = adc_scan(ADC_REMOTE);

    if (data < 0x74)
        if (data < 0x40)
            if (data < 0x20)
                if(data < 0x10)
                    btn = BUTTON_RC_STOP;
                else
                    btn = BUTTON_RC_VOL_DOWN;
            else
                btn = BUTTON_RC_VOL;
        else
            if (data < 0x58)
                btn = BUTTON_RC_VOL_UP;
            else
                btn = BUTTON_RC_BITRATE;
    else
        if (data < 0xb0)
            if (data < 0x88)
                btn = BUTTON_RC_REC;
            else
                btn = BUTTON_RC_SOURCE;
        else
            if (data < 0xd8)
                if(data < 0xc0)
                    btn = BUTTON_RC_FF;
                else
                    btn = BUTTON_RC_MENU;
            else
                if (data < 0xf0)
                    btn = BUTTON_RC_REW;

    data = GPIO1_READ;
    if ((data & 0x20) == 0)
        btn |= BUTTON_ON;

    if ((data & 0x40) == 0)
        btn |= BUTTON_RC_ON;
    
#elif CONFIG_KEYPAD == RECORDER_PAD

#ifdef HAVE_FMADC
    if ( adc_read(ADC_BUTTON_ON) < 512 )
        btn |= BUTTON_ON;
    if ( adc_read(ADC_BUTTON_OFF) > 512 )
        btn |= BUTTON_OFF;
#else
    /* check port B pins for ON and OFF */
    data = PBDR;
    if ((data & 0x0100) == 0)
        btn |= BUTTON_ON;
    if ((data & 0x0010) == 0)
        btn |= BUTTON_OFF;
#endif

    /* check F1..F3 and UP */
    data = adc_read(ADC_BUTTON_ROW1);

    if (data >= LEVEL4)
        btn |= BUTTON_F3;
    else if (data >= LEVEL3)
        btn |= BUTTON_UP;
    else if (data >= LEVEL2)
        btn |= BUTTON_F2;
    else if (data >= LEVEL1)
        btn |= BUTTON_F1;

    /* Some units have mushy keypads, so pressing UP also activates
       the Left/Right buttons. Let's combat that by skipping the AN5
       checks when UP is pressed. */
    if(!(btn & BUTTON_UP))
    {
        /* check DOWN, PLAY, LEFT, RIGHT */
        data = adc_read(ADC_BUTTON_ROW2);

        if (data >= LEVEL4)
            btn |= BUTTON_DOWN;
        else if (data >= LEVEL3)
            btn |= ROW2_BUTTON3;
        else if (data >= LEVEL2)
            btn |= BUTTON_LEFT;
        else if (data >= LEVEL1)
            btn |= ROW2_BUTTON1;
    }

#elif CONFIG_KEYPAD == PLAYER_PAD

    /* buttons are active low */
    if (adc_read(0) < 0x180)
        btn |= BUTTON_LEFT;
    if (adc_read(1) < 0x180)
        btn |= BUTTON_MENU;
    if(adc_read(2) < 0x180)
        btn |= BUTTON_RIGHT;
    if(adc_read(3) < 0x180)
        btn |= BUTTON_PLAY;

    /* check port A pins for ON and STOP */
    data = PADR;
    if ( !(data & 0x0020) )
        btn |= BUTTON_ON;
    if ( !(data & 0x0800) )
        btn |= BUTTON_STOP;

#elif CONFIG_KEYPAD == ONDIO_PAD

    if(adc_read(ADC_BUTTON_OPTION) > 0x200) /* active high */
        btn |= BUTTON_MENU;
    if(adc_read(ADC_BUTTON_ONOFF) < 0x120) /* active low */
        btn |= BUTTON_OFF;

    /* Check the 4 direction keys */
    data = adc_read(ADC_BUTTON_ROW1);

    if (data >= LEVEL4)
        btn |= BUTTON_LEFT;
    else if (data >= LEVEL3)
        btn |= BUTTON_RIGHT;
    else if (data >= LEVEL2)
        btn |= BUTTON_UP;
    else if (data >= LEVEL1)
        btn |= BUTTON_DOWN;

#elif CONFIG_KEYPAD == GMINI100_PAD

    if (adc_read(7) < 0xE3)
        btn |= BUTTON_LEFT;
    else if (adc_read(7) < 0x1c5)
        btn |= BUTTON_DOWN;
    else if (adc_read(7) < 0x2a2)
        btn |= BUTTON_RIGHT;
    else if (adc_read(7) < 0x38a)
        btn |= BUTTON_UP;

    if (adc_read(6) < 0x233)
        btn |= BUTTON_OFF;
    else if (adc_read(6) < 0x288)
        btn |= BUTTON_PLAY;
    else if (adc_read(6) < 0x355)
        btn |= BUTTON_MENU;

    data = P7;
    if (data & 0x01)
        btn |= BUTTON_ON;
    
#endif /* CONFIG_KEYPAD */


#ifdef HAVE_LCD_BITMAP
    if (btn && flipped)
        btn = button_flip(btn); /* swap upside down */
#endif

    /* Filter the button status. It is only accepted if we get the same
       status twice in a row. */
    if (btn != last_read)
        retval = lastbtn;
    else
        retval = btn;
    last_read = btn;

    return retval;
}

#if CONFIG_KEYPAD == IRIVER_H100_PAD
bool button_hold(void)
{
    return (GPIO1_READ & 0x00000002)?true:false;
}

bool remote_button_hold(void)
{
    return (GPIO1_READ & 0x00100000)?true:false;
}
#endif

int button_status(void)
{
    return lastbtn;
}

void button_clear_queue(void)
{
    queue_clear(&button_queue);
}

