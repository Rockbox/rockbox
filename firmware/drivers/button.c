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
 * iPod driver based on code from the ipodlinux project - http://ipodlinux.org
 * Adapted for Rockbox in December 2005
 * Original file: linux/arch/armnommu/mach-ipod/keyboard.c
 * Copyright (c) 2003-2005 Bernard Leach (leachbj@bouncycastle.org)
 *
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

#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif

struct event_queue button_queue;

static long lastbtn;   /* Last valid button status */
static long last_read; /* Last button status, for debouncing/filtering */
#ifdef HAVE_LCD_BITMAP
static bool flipped;  /* buttons can be flipped to match the LCD flip */
#endif
#ifdef CONFIG_BACKLIGHT
static bool filter_first_keypress;
#ifdef HAVE_REMOTE_LCD
static bool remote_filter_first_keypress;
#endif
#endif

/* how long until repeat kicks in, in ticks */
#define REPEAT_START      30

/* the speed repeat starts at, in ticks */
#define REPEAT_INTERVAL_START   16

/* speed repeat finishes at, in ticks */
#define REPEAT_INTERVAL_FINISH  5

/* the power-off button and number of repeated keys before shutting off */
#if (CONFIG_KEYPAD == IPOD_3G_PAD) || (CONFIG_KEYPAD == IPOD_4G_PAD) ||\
    (CONFIG_KEYPAD == IRIVER_IFP7XX_PAD)
#define POWEROFF_BUTTON BUTTON_PLAY
#define POWEROFF_COUNT 40
#elif (CONFIG_KEYPAD == IAUDIO_X5_PAD)
#define POWEROFF_BUTTON BUTTON_POWER
#define POWEROFF_COUNT 10
#else
#define POWEROFF_BUTTON BUTTON_OFF
#define POWEROFF_COUNT 10
#endif

static int button_read(void);

#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
static bool remote_button_hold_only(void);
#endif

#if CONFIG_KEYPAD == IPOD_4G_PAD
/* Variable to use for setting button status in interrupt handler */
int int_btn = BUTTON_NONE;
#endif

#if (CONFIG_KEYPAD == IPOD_4G_PAD) && !defined(IPOD_MINI)
static void opto_i2c_init(void)
{
    int i, curr_value;

    /* wait for value to settle */
    i = 1000;
    curr_value = (inl(0x7000c104) << 16) >> 24;
    while (i > 0)
    {
        int new_value = (inl(0x7000c104) << 16) >> 24;

        if (new_value != curr_value) {
            i = 10000;
            curr_value = new_value;
        }
        else {
            i--;
        }
    }

    GPIOB_OUTPUT_VAL |= 0x10;
    DEV_EN |= 0x10000;
    DEV_RS |= 0x10000;
    udelay(5);
    DEV_RS &= ~0x10000; /* finish reset */

    outl(0xffffffff, 0x7000c120);
    outl(0xffffffff, 0x7000c124);
    outl(0xc00a1f00, 0x7000c100);
    outl(0x1000000, 0x7000c104);
}

static inline int ipod_4g_button_read(void)
{
    /* The ipodlinux source had a udelay(250) here, but testing has shown that
       it is not needed - tested on Nano, Color/Photo and Video. */
    /* udelay(250);*/

    int btn = BUTTON_NONE;
    unsigned reg = 0x7000c104;
    if ((inl(0x7000c104) & 0x4000000) != 0) {
        unsigned status = inl(0x7000c140);

        reg = reg + 0x3C;      /* 0x7000c140 */
        outl(0x0, 0x7000c140); /* clear interrupt status? */

        if ((status & 0x800000ff) == 0x8000001a) {
            static int old_wheel_value IDATA_ATTR = -1;
            static int wheel_repeat = 0;

            if (status & 0x100)
                btn |= BUTTON_SELECT;
            if (status & 0x200)
                btn |= BUTTON_RIGHT;
            if (status & 0x400)
                btn |= BUTTON_LEFT;
            if (status & 0x800)
                btn |= BUTTON_PLAY;
            if (status & 0x1000)
                btn |= BUTTON_MENU;
            if (status & 0x40000000) {
                /* NB: highest wheel = 0x5F, clockwise increases */
                int new_wheel_value = (status << 9) >> 25;
                backlight_on();
                /* The queue should have no other events when scrolling */
                if (queue_empty(&button_queue) && old_wheel_value >= 0) {

                    /* This is for later = BUTTON_SCROLL_TOUCH;*/
                    int wheel_delta = new_wheel_value - old_wheel_value;
                    unsigned long data;
                    int wheel_keycode;

                    if (wheel_delta < -48)
                        wheel_delta += 96; /* Forward wrapping case */
                    else if (wheel_delta > 48)
                        wheel_delta -= 96; /* Backward wrapping case */

                    if (wheel_delta > 4) {
                        wheel_keycode = BUTTON_SCROLL_FWD;
                    } else if (wheel_delta < -4) {
                        wheel_keycode = BUTTON_SCROLL_BACK;
                    } else goto wheel_end;

                    data = (wheel_delta << 16) | new_wheel_value;
                    queue_post(&button_queue, wheel_keycode | wheel_repeat,
                            (void *)data);

                    if (!wheel_repeat) wheel_repeat = BUTTON_REPEAT;
                }

                old_wheel_value = new_wheel_value;
            } else if (old_wheel_value >= 0) {
                /* scroll wheel up */
                old_wheel_value = -1;
                wheel_repeat = 0;
            }

        } else if (status == 0xffffffff) {
            opto_i2c_init();
        }
    }

wheel_end:

    if ((inl(reg) & 0x8000000) != 0) {
        outl(0xffffffff, 0x7000c120);
        outl(0xffffffff, 0x7000c124);
    }
    return btn;
}

void ipod_4g_button_int(void)
{
    CPU_HI_INT_CLR = I2C_MASK;
    /* The following delay was 250 in the ipodlinux source, but 50 seems to 
       work fine - tested on Nano, Color/Photo and Video. */
    udelay(50); 
    outl(0x0, 0x7000c140); 
    int_btn = ipod_4g_button_read();
    outl(inl(0x7000c104) | 0xC000000, 0x7000c104);
    outl(0x400a1f00, 0x7000c100);

    GPIOB_OUTPUT_VAL |= 0x10;
    CPU_INT_EN = 0x40000000;
    CPU_HI_INT_EN = I2C_MASK;
}
#endif 
#if (CONFIG_KEYPAD == IPOD_3G_PAD) || defined(IPOD_MINI)
/* iPod 3G and mini 1G, mini 2G uses iPod 4G code */
void handle_scroll_wheel(int new_scroll, int was_hold, int reverse)
{
    int wheel_keycode = BUTTON_NONE;
    static int prev_scroll = -1;
    static int direction = 0;
    static int count = 0;
    static int scroll_state[4][4] = {
        {0, 1, -1, 0},
        {-1, 0, 0, 1},
        {1, 0, 0, -1},
        {0, -1, 1, 0}
    };

    if ( prev_scroll == -1 ) {
        prev_scroll = new_scroll;
    }
    else if (direction != scroll_state[prev_scroll][new_scroll]) {
        direction = scroll_state[prev_scroll][new_scroll];
        count = 0;
    }
    else if (!was_hold) {
        backlight_on();
        if (++count == 6) { /* reduce sensitivity */
            count = 0;
            switch (direction) {
                case 1:
                    if (reverse) {
                        /* 'r' keypress */
                        wheel_keycode = BUTTON_SCROLL_FWD;
                    }
                    else {
                        /* 'l' keypress */
                        wheel_keycode = BUTTON_SCROLL_BACK;
                    }
                    break;
                case -1:
                    if (reverse) {
                        /* 'l' keypress */
                        wheel_keycode = BUTTON_SCROLL_BACK;
                    }
                    else {
                        /* 'r' keypress */
                        wheel_keycode = BUTTON_SCROLL_FWD;
                    }
                    break;
                default:
                    /* only happens if we get out of sync */
                    break;
            }
        }
    }
    if (wheel_keycode != BUTTON_NONE && queue_empty(&button_queue))
        queue_post(&button_queue, wheel_keycode, NULL);
    prev_scroll = new_scroll;
}
#endif
#if (CONFIG_KEYPAD == IPOD_4G_PAD) && defined(IPOD_MINI)
/* mini 1 only, mini 2G uses iPod 4G code */
static int ipod_mini_button_read(void)
{
    unsigned char source, wheel_source, state, wheel_state;
    static bool was_hold = false;
    int btn = BUTTON_NONE;

    /* The ipodlinux source had a udelay(250) here, but testing has shown that
       it is not needed - tested on mini 1g. */
    /* udelay(250);*/

    /* get source(s) of interupt */
    source = GPIOA_INT_STAT & 0x3f;
    wheel_source = GPIOB_INT_STAT & 0x30;

    if (source == 0 && wheel_source == 0) {
        return BUTTON_NONE; /* not for us */
    }

    /* get current keypad & wheel status */
    state = GPIOA_INPUT_VAL & 0x3f;
    wheel_state = GPIOB_INPUT_VAL & 0x30;

    /* toggle interrupt level */
    GPIOA_INT_LEV = ~state;
    GPIOB_INT_LEV = ~wheel_state;

    /* hold switch causes all outputs to go low    */
    /* we shouldn't interpret these as key presses */
    if ((state & 0x20)) {
        if (!(state & 0x1))
            btn |= BUTTON_SELECT;
        if (!(state & 0x2))
            btn |= BUTTON_MENU;
        if (!(state & 0x4))
            btn |= BUTTON_PLAY;
        if (!(state & 0x8))
            btn |= BUTTON_RIGHT;
        if (!(state & 0x10))
            btn |= BUTTON_LEFT;

        if (wheel_source & 0x30) {
            handle_scroll_wheel((wheel_state & 0x30) >> 4, was_hold, 1);
        }
    }

    was_hold = button_hold();

    /* ack any active interrupts */
    if (source)
        GPIOA_INT_CLR = source;
    if (wheel_source)
        GPIOB_INT_CLR = wheel_source;

    return btn;
}

void ipod_mini_button_int(void)
{
    CPU_HI_INT_CLR = GPIO_MASK;
    int_btn = ipod_mini_button_read();
    //CPU_INT_EN = 0x40000000;
    CPU_HI_INT_EN = GPIO_MASK;
}
#endif
#if CONFIG_KEYPAD == IPOD_3G_PAD
static int ipod_3g_button_read(void)
{
    unsigned char source, state;
    static int was_hold = 0;
    int btn = BUTTON_NONE;
    /*
     * we need some delay for g3, cause hold generates several interrupts,
     * some of them delayed
     */
    udelay(250);

    /* get source of interupts */
    source = GPIOA_INT_STAT;


    /* get current keypad status */
    state = GPIOA_INPUT_VAL;
    GPIOA_INT_LEV = ~state;

    if (was_hold && source == 0x40 && state == 0xbf) {
        /* ack any active interrupts */
        GPIOA_INT_CLR = source;
        return BUTTON_NONE;
    }
    was_hold = 0;


    if ((state & 0x20) == 0) {
        /* 3g hold switch is active low */
        was_hold = 1;
        /* hold switch on 3g causes all outputs to go low */
        /* we shouldn't interpret these as key presses */
        GPIOA_INT_CLR = source;
        return BUTTON_NONE;
    }
    if ((state & 0x1) == 0) {
        btn |= BUTTON_RIGHT;
    }
    if ((state & 0x2) == 0) {
        btn |= BUTTON_SELECT;
    }
    if ((state & 0x4) == 0) {
        btn |= BUTTON_PLAY;
    }
    if ((state & 0x8) == 0) {
        btn |= BUTTON_LEFT;
    }
    if ((state & 0x10) == 0) {
        btn |= BUTTON_MENU;
    }

    if (source & 0xc0) {
        handle_scroll_wheel((state & 0xc0) >> 6, was_hold, 0);
    }

    /* ack any active interrupts */
    GPIOA_INT_CLR = source;

    return btn;
}
#endif

static void button_tick(void)
{
    static int count = 0;
    static int repeat_speed = REPEAT_INTERVAL_START;
    static int repeat_count = 0;
    static bool repeat = false;
    static bool post = false;
#ifdef CONFIG_BACKLIGHT
    static bool skip_release = false;
#ifdef HAVE_REMOTE_LCD
    static bool skip_remote_release = false;
#endif
#endif
    int diff;
    int btn;

#if (CONFIG_KEYPAD == PLAYER_PAD) || (CONFIG_KEYPAD == RECORDER_PAD)

    /* Post events for the remote control */
    btn = remote_control_rx();
    if(btn)
    {
        queue_post(&button_queue, btn, NULL);
    }
#endif

    btn = button_read();

    /* Find out if a key has been released */
    diff = btn ^ lastbtn;
    if(diff && (btn & diff) == 0)
    {
#ifdef CONFIG_BACKLIGHT
#ifdef HAVE_REMOTE_LCD
        if(diff & BUTTON_REMOTE)
            if(!skip_remote_release)
                queue_post(&button_queue, BUTTON_REL | diff, NULL);
            else
                skip_remote_release = false;
        else
#endif
            if(!skip_release)
                queue_post(&button_queue, BUTTON_REL | diff, NULL);
            else
                skip_release = false;
#else
        queue_post(&button_queue, BUTTON_REL | diff, NULL);
#endif
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
                    if (!post)
                        count--;
                    if (count == 0) {
                        post = true;
                        /* yes we have repeat */
                        if (repeat_speed > REPEAT_INTERVAL_FINISH)
                            repeat_speed--;
                        count = repeat_speed;

                        repeat_count++;

                        /* Send a SYS_POWEROFF event if we have a device
                           which doesn't shut down easily with the OFF
                           key */
#ifdef HAVE_SW_POWEROFF
                        if ((btn == POWEROFF_BUTTON
#ifdef BUTTON_RC_STOP
                                    || btn == BUTTON_RC_STOP
#endif
                                    ) &&
#if defined(HAVE_CHARGING) && !defined(HAVE_POWEROFF_WHILE_CHARGING)
                                !charger_inserted() &&
#endif
                                repeat_count > POWEROFF_COUNT)
                        {
                            /* Tell the main thread that it's time to
                               power off */
                            sys_poweroff();

                            /* Safety net for players without hardware
                               poweroff */
                            if(repeat_count > POWEROFF_COUNT * 10)
                                power_off();
                        }
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
                {
                    /* Only post repeat events if the queue is empty,
                     * to avoid afterscroll effects. */
                    if (queue_empty(&button_queue))
                    {
                        queue_post(&button_queue, BUTTON_REPEAT | btn, NULL);
#ifdef CONFIG_BACKLIGHT
#ifdef HAVE_REMOTE_LCD
                        skip_remote_release = false;
#endif
                        skip_release = false;
#endif
                        post = false;
                    }
                }
                else
                {
#ifdef CONFIG_BACKLIGHT
#ifdef HAVE_REMOTE_LCD
                    if (btn & BUTTON_REMOTE) {
                        if (!remote_filter_first_keypress || is_remote_backlight_on()
#if defined(IRIVER_H100_SERIES) || defined(IRIVER_H300_SERIES)
                           || (remote_type()==REMOTETYPE_H300_NONLCD)
#endif
                            )
                            queue_post(&button_queue, btn, NULL);
                        else
                            skip_remote_release = true;
                    }
                    else
#endif
                        if (!filter_first_keypress || is_backlight_on())
                            queue_post(&button_queue, btn, NULL);
                        else
                            skip_release = true;
#else /* no backlight, nothing to skip */
                    queue_post(&button_queue, btn, NULL);
#endif
                    post = false;
                }
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
    GPIO1_ENABLE &= ~0x00100060;
#elif CONFIG_KEYPAD == IRIVER_H300_PAD
    /* Set GPIO9 and GPIO15 as general purpose inputs */
    GPIO_ENABLE &= ~0x00008200;
    GPIO_FUNCTION |= 0x00008200;
    /* Set GPIO33, GPIO37, GPIO38  and GPIO52 as general purpose inputs */
    GPIO1_ENABLE &= ~0x00100060;
    GPIO1_FUNCTION |= 0x00100062;
#elif CONFIG_KEYPAD == IAUDIO_X5_PAD
    /* Power, Remote Play & Hold switch */
    GPIO_FUNCTION |= 0x0e000000;
    GPIO_ENABLE &= ~0x0e000000;

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
#elif (CONFIG_KEYPAD == IPOD_4G_PAD) && !defined(IPOD_MINI)
    opto_i2c_init();
    /* hold button - enable as input */
    GPIOA_ENABLE |= 0x20;
    GPIOA_OUTPUT_EN &= ~0x20; 
    /* hold button - set interrupt levels */
    GPIOA_INT_LEV = ~(GPIOA_INPUT_VAL & 0x20);
    GPIOA_INT_CLR = GPIOA_INT_STAT & 0x20;
    /* enable interrupts */
    GPIOA_INT_EN = 0x20;
    /* unmask interrupt */
    CPU_INT_EN = 0x40000000;
    CPU_HI_INT_EN = I2C_MASK;

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) && defined(IPOD_MINI)
    /* iPod Mini G1 */
    /* buttons - enable as input */
    GPIOA_ENABLE |= 0x3f;
    GPIOA_OUTPUT_EN &= ~0x3f;
    /* scroll wheel- enable as input */
    GPIOB_ENABLE |= 0x30; /* port b 4,5 */
    GPIOB_OUTPUT_EN &= ~0x30; /* port b 4,5 */
    /* buttons - set interrupt levels */
    GPIOA_INT_LEV = ~(GPIOA_INPUT_VAL & 0x3f);
    GPIOA_INT_CLR = GPIOA_INT_STAT & 0x3f;
    /* scroll wheel - set interrupt levels */
    GPIOB_INT_LEV = ~(GPIOB_INPUT_VAL & 0x30);
    GPIOB_INT_CLR = GPIOB_INT_STAT & 0x30;
    /* enable interrupts */
    GPIOA_INT_EN = 0x3f;
    GPIOB_INT_EN = 0x30;
    /* unmask interrupt */
    CPU_INT_EN = 0x40000000;
    CPU_HI_INT_EN = GPIO_MASK;

#elif CONFIG_KEYPAD == IPOD_3G_PAD
    GPIOA_INT_LEV = ~GPIOA_INPUT_VAL;
    GPIOA_INT_CLR = GPIOA_INT_STAT;
    GPIOA_INT_EN  = 0xff;
#endif /* CONFIG_KEYPAD */
    queue_init(&button_queue);
    button_read();
    lastbtn = button_read();
    tick_add_task(button_tick);
    reset_poweroff_timer();

#ifdef HAVE_LCD_BITMAP
    flipped = false;
#endif
#ifdef CONFIG_BACKLIGHT
    filter_first_keypress = false;
#ifdef HAVE_REMOTE_LCD
    remote_filter_first_keypress = false;
#endif    
#endif
}

#ifdef HAVE_LCD_BITMAP /* only bitmap displays can be flipped */
/*
 * helper function to swap LEFT/RIGHT, UP/DOWN (if present), and F1/F3 (Recorder)
 */
static int button_flip(int button)
{
    int newbutton;

    newbutton = button &
        ~(BUTTON_LEFT | BUTTON_RIGHT
#if defined(BUTTON_UP) && defined(BUTTON_DOWN)
        | BUTTON_UP | BUTTON_DOWN
#endif
#if CONFIG_KEYPAD == RECORDER_PAD
        | BUTTON_F1 | BUTTON_F3
#endif
        );

    if (button & BUTTON_LEFT)
        newbutton |= BUTTON_RIGHT;
    if (button & BUTTON_RIGHT)
        newbutton |= BUTTON_LEFT;
#if defined(BUTTON_UP) && defined(BUTTON_DOWN)
    if (button & BUTTON_UP)
        newbutton |= BUTTON_DOWN;
    if (button & BUTTON_DOWN)
        newbutton |= BUTTON_UP;
#endif
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

#ifdef CONFIG_BACKLIGHT
void set_backlight_filter_keypress(bool value)
{
    filter_first_keypress = value;
}
#ifdef HAVE_REMOTE_LCD
void set_remote_backlight_filter_keypress(bool value)
{
    remote_filter_first_keypress = value;
}
#endif
#endif

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

#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
    static bool hold_button = false;
    static bool remote_hold_button = false;
    static int prev_data = 0xff;
    static int last_valid = 0xff;

    /* light handling */
    if (hold_button && !button_hold())
    {
        backlight_on();
    }
    if (remote_hold_button && !remote_button_hold_only())
    {
        remote_backlight_on();
    }

    hold_button = button_hold();
    remote_hold_button = remote_button_hold_only();

    /* normal buttons */
    if (!hold_button)
    {
        data = adc_scan(ADC_BUTTONS);

        /* ADC debouncing: Only accept new reading if it's
         * stable (+/-1). Use latest stable value otherwise. */
        if ((unsigned)(data - prev_data + 1) <= 2)
            last_valid = data;
        prev_data = data;
        data = last_valid;

#if CONFIG_KEYPAD == IRIVER_H100_PAD
        if (data < 0xf0)
        {
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
                        btn = BUTTON_REC;
        }
#else /* H300 */
        if (data < 0xba)
        {
            if (data < 0x54)
                if (data < 0x30)
                    if (data < 0x10)
                        btn = BUTTON_SELECT;
                    else
                        btn = BUTTON_UP;
                else
                    btn = BUTTON_LEFT;
            else
                if (data < 0x98)
                    if (data < 0x76)
                        btn = BUTTON_DOWN;
                    else
                        btn = BUTTON_RIGHT;
                else
                    btn = BUTTON_OFF;
        }
#endif
    }

    /* remote buttons */
    if (!remote_hold_button)
    {
        data = adc_scan(ADC_REMOTE);
        switch (remote_type())
        {
            case REMOTETYPE_H100_LCD:
                if (data < 0xf5)
                {
                    if (data < 0x73)
                        if (data < 0x3f)
                            if (data < 0x25)
                                if(data < 0x0c)
                                    btn |= BUTTON_RC_STOP;
                                else
                                    btn |= BUTTON_RC_VOL_DOWN;
                            else
                                btn |= BUTTON_RC_MODE;
                        else
                            if (data < 0x5a)
                                btn |= BUTTON_RC_VOL_UP;
                            else
                                btn |= BUTTON_RC_BITRATE;
                    else
                        if (data < 0xa8)
                            if (data < 0x8c)
                                btn |= BUTTON_RC_REC;
                            else
                                btn |= BUTTON_RC_SOURCE;
                        else
                            if (data < 0xdf)
                                if(data < 0xc5)
                                    btn |= BUTTON_RC_FF;
                                else
                                    btn |= BUTTON_RC_MENU;
                            else
                                btn |= BUTTON_RC_REW;
                }
                break;
            case REMOTETYPE_H300_LCD:
                if (data < 0xf5)
                {
                    if (data < 0x73)
                        if (data < 0x42)
                            if (data < 0x27)
                                if(data < 0x0c)
                                    btn |= BUTTON_RC_VOL_DOWN;
                                else
                                    btn |= BUTTON_RC_FF;
                            else
                                btn |= BUTTON_RC_STOP;
                        else
                            if (data < 0x5b)
                                btn |= BUTTON_RC_MODE;
                            else
                                btn |= BUTTON_RC_REC;
                    else
                        if (data < 0xab)
                            if (data < 0x8e)
                                btn |= BUTTON_RC_ON;
                            else
                                btn |= BUTTON_RC_BITRATE;
                        else
                            if (data < 0xde)
                                if(data < 0xc5)
                                    btn |= BUTTON_RC_SOURCE;
                                else
                                    btn |= BUTTON_RC_VOL_UP;
                            else
                                btn |= BUTTON_RC_REW;
                }
                break;
            case REMOTETYPE_H300_NONLCD:
                if (data < 0xf1)
                {
                    if (data < 0x7d)
                        if (data < 0x25)
                            btn |= BUTTON_RC_FF;
                        else
                            btn |= BUTTON_RC_REW;
                    else
                        if (data < 0xd5)
                            btn |= BUTTON_RC_VOL_DOWN;
                        else
                            btn |= BUTTON_RC_VOL_UP;
                }
                break;
        }
    }

    /* special buttons */
#if CONFIG_KEYPAD == IRIVER_H300_PAD
    if (!hold_button)
    {
        data = GPIO_READ;
        if ((data & 0x0200) == 0)
            btn |= BUTTON_MODE;
        if ((data & 0x8000) == 0)
            btn |= BUTTON_REC;
    }
#endif

    data = GPIO1_READ;
    if (!hold_button && ((data & 0x20) == 0))
        btn |= BUTTON_ON;
    if (!remote_hold_button && ((data & 0x40) == 0))
        switch(remote_type())
        {
            case REMOTETYPE_H100_LCD:
            case REMOTETYPE_H300_NONLCD:
                btn |= BUTTON_RC_ON;
                break;
            case REMOTETYPE_H300_LCD:
                btn |= BUTTON_RC_MENU;
                break;
        }
   
#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
    static bool hold_button = false;

    /* light handling */
    if (hold_button && !button_hold())
    {
        backlight_on();
    }
    hold_button = button_hold();

    /* normal buttons */
    if (!button_hold())
    {          
        data = adc_read(ADC_BUTTONS);
        if (data < 0x35c)
        {
            if (data < 0x151)
                if (data < 0xc7)
                    if (data < 0x41)
                        btn = BUTTON_LEFT;
                    else
                        btn = BUTTON_RIGHT;
                else
                    btn = BUTTON_SELECT;
            else
                if (data < 0x268)
                    if (data < 0x1d7)
                        btn = BUTTON_UP;
                    else
                        btn = BUTTON_DOWN;
                else
                    if (data < 0x2f9)
                        btn = BUTTON_EQ;
                    else
                        btn = BUTTON_MODE;
        }

        if (adc_read(ADC_BUTTON_PLAY) < 0x64)
            btn |= BUTTON_PLAY;
    }

#elif CONFIG_KEYPAD == RECORDER_PAD
#ifndef HAVE_FMADC
    static int off_button_count = 0;
#endif

    /* check F1..F3 and UP */
    data = adc_read(ADC_BUTTON_ROW1);
    if (data >= LEVEL1)
    {
        if (data >= LEVEL3)
            if (data >= LEVEL4)
                btn = BUTTON_F3;
            else
                btn = BUTTON_UP;
        else
            if (data >= LEVEL2)
                btn = BUTTON_F2;
            else
                btn = BUTTON_F1;
    }

    /* Some units have mushy keypads, so pressing UP also activates
       the Left/Right buttons. Let's combat that by skipping the AN5
       checks when UP is pressed. */
    if(!(btn & BUTTON_UP))
    {
        /* check DOWN, PLAY, LEFT, RIGHT */
        data = adc_read(ADC_BUTTON_ROW2);
        if (data >= LEVEL1)
        {
            if (data >= LEVEL3)
                if (data >= LEVEL4)
                    btn |= BUTTON_DOWN;
                else
                    btn |= ROW2_BUTTON3;
            else
                if (data >= LEVEL2)
                    btn |= BUTTON_LEFT;
                else
                    btn |= ROW2_BUTTON1;
        }
    }

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
    {
        /* When the batteries are low, the low-battery shutdown logic causes
         * spurious OFF events due to voltage fluctuation on some units.
         * Only accept OFF when read several times in sequence. */
        if (++off_button_count > 3)
            btn |= BUTTON_OFF;
    }
    else
        off_button_count = 0;
#endif

#elif CONFIG_KEYPAD == PLAYER_PAD
    /* buttons are active low */
    if (adc_read(0) < 0x180)
        btn = BUTTON_LEFT;
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
    /* Check the 4 direction keys */
    data = adc_read(ADC_BUTTON_ROW1);
    if (data >= LEVEL1)
    {
        if (data >= LEVEL3)
            if (data >= LEVEL4)
                btn = BUTTON_LEFT;
            else
                btn = BUTTON_RIGHT;
        else
            if (data >= LEVEL2)
                btn = BUTTON_UP;
            else
                btn = BUTTON_DOWN;
    }

    if(adc_read(ADC_BUTTON_OPTION) > 0x200) /* active high */
        btn |= BUTTON_MENU;
    if(adc_read(ADC_BUTTON_ONOFF) < 0x120) /* active low */
        btn |= BUTTON_OFF;

#elif CONFIG_KEYPAD == GMINI100_PAD
    data = adc_read(7);
    if (data < 0x38a)
    {
        if (data < 0x1c5)
            if (data < 0xe3)
                btn = BUTTON_LEFT;
            else
                btn = BUTTON_DOWN;
        else
            if (data < 0x2a2)
                btn = BUTTON_RIGHT;
            else
                btn = BUTTON_UP;
    }

    data = adc_read(6);
    if (data < 0x355)
    {
        if (data < 0x288)
            if (data < 0x233)
                btn |= BUTTON_OFF;
            else
                btn |= BUTTON_PLAY;
        else
            btn |= BUTTON_MENU;
    }

    data = P7;
    if (data & 0x01)
        btn |= BUTTON_ON;

#elif (CONFIG_KEYPAD == IPOD_4G_PAD)
    (void)data;
    /* The int_btn variable is set in the button interrupt handler */
    btn = int_btn;

#elif (CONFIG_KEYPAD == IPOD_3G_PAD)
    (void)data;
    btn = ipod_3g_button_read();

#elif (CONFIG_KEYPAD == IAUDIO_X5_PAD)
    static bool hold_button = false;
    static bool remote_hold_button = false;

    hold_button = button_hold();
    remote_hold_button = remote_button_hold();

    /* normal buttons */
    if (!hold_button)
    {
        data = adc_scan(ADC_BUTTONS);
        if (data < 0xf0)
        {
            if(data < 0x7c)
                if(data < 0x42)
                    btn = BUTTON_LEFT;
                else
                    if(data < 0x62)
                        btn = BUTTON_RIGHT;
                    else
                        btn = BUTTON_SELECT;
            else
                if(data < 0xb6)
                    if(data < 0x98)
                        btn = BUTTON_REC;
                    else
                        btn = BUTTON_PLAY;
                else
                    if(data < 0xd3)
                        btn = BUTTON_DOWN;
                    else
                        btn = BUTTON_UP;
        }
    }

    /* remote buttons */
    data = adc_scan(ADC_REMOTE);
    if(data < 0x17)
        remote_hold_button = true;

    if(!remote_hold_button)
    {
        if (data < 0xee)
        {
            if(data < 0x7a)
                if(data < 0x41)
                    btn |= BUTTON_RC_REW;
                else
                    if(data < 0x61)
                        btn |= BUTTON_RC_FF;
                    else
                        btn |= BUTTON_RC_MODE;
            else
                if(data < 0xb4)
                    if(data < 0x96)
                        btn |= BUTTON_RC_REC;
                    else
                        btn |= BUTTON_RC_MENU;
                else
                    if(data < 0xd1)
                        btn |= BUTTON_RC_VOL_UP;
                    else
                        btn |= BUTTON_RC_VOL_DOWN;
        }
    }

    data = GPIO_READ;
    if (!(data & 0x04000000))
        btn |= BUTTON_POWER;

    if (!(data & 0x02000000))
        btn |= BUTTON_RC_PLAY;

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

#if (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD)
bool button_hold(void)
{
    return (GPIOA_INPUT_VAL & 0x20)?false:true;
}
#endif

#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
bool button_hold(void)
{
    return (GPIO1_READ & 0x00000002)?true:false;
}

static bool remote_button_hold_only(void)
{        
    if(remote_type() == REMOTETYPE_H300_NONLCD)
        return adc_scan(ADC_REMOTE)<0x0d; /* hold should be 0x00 */
    else
        return (GPIO1_READ & 0x00100000)?true:false;
}

/* returns true only if there is remote present */
bool remote_button_hold(void)
{
    /* H300's NON-LCD remote doesn't set the "remote present" bit */
    if(remote_type() == REMOTETYPE_H300_NONLCD)
        return remote_button_hold_only();
    else
        return ((GPIO_READ & 0x40000000) == 0)?remote_button_hold_only():false;
}
#endif

#if CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
bool button_hold(void)
{
    return (GPIO5_READ & 4) ? false : true;
}
#endif

#if (CONFIG_KEYPAD == IAUDIO_X5_PAD)
bool button_hold(void)
{
    return (GPIO_READ & 0x08000000)?false:true;
}

bool remote_button_hold(void)
{
    return false; /* TODO X5 */
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

