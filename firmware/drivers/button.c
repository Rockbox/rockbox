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

/* the power-off button and number of repeated keys before shutting off */
#if (CONFIG_KEYPAD == IPOD_3G_PAD) || (CONFIG_KEYPAD == IPOD_4G_PAD) ||\
    (CONFIG_KEYPAD == IRIVER_IFP7XX_PAD)
#define POWEROFF_BUTTON BUTTON_PLAY
#define POWEROFF_COUNT 40
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

static int ipod_4g_button_read(void)
{
    unsigned reg, status;
    static int clickwheel_down = 0;
    static int old_wheel_value = -1;
    int wheel_keycode = BUTTON_NONE;
    int wheel_delta, wheel_delta_abs;
    int new_wheel_value;
    int btn = BUTTON_NONE;
    
    /* The ipodlinux source had a udelay(250) here, but testing has shown that
       it is not needed - tested on Nano, Color/Photo and Video. */
    /* udelay(250);*/

    reg = 0x7000c104;
    if ((inl(0x7000c104) & 0x4000000) != 0) {
        reg = reg + 0x3C;   /* 0x7000c140 */

        status = inl(0x7000c140);
        outl(0x0, 0x7000c140);  /* clear interrupt status? */

        if ((status & 0x800000ff) == 0x8000001a) {
            /* NB: highest wheel = 0x5F, clockwise increases */
            new_wheel_value = ((status << 9) >> 25) & 0xff;

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
                /* scroll wheel down */
                clickwheel_down = 1;
                backlight_on();
                if (old_wheel_value != -1) {
                    wheel_delta = new_wheel_value - old_wheel_value;
                    wheel_delta_abs = wheel_delta < 0 ? -wheel_delta 
                                                      : wheel_delta;
                    
                    if (wheel_delta_abs > 48) {
                        if (old_wheel_value > new_wheel_value)
                            /* wrapped around the top going clockwise */
                            wheel_delta += 96;
                        else if (old_wheel_value < new_wheel_value)
                            /* wrapped around the top going counterclockwise */                             wheel_delta -= 96;
                    }
                    /* TODO: these thresholds should most definitely be
                       settings, and we're probably going to want a more
                       advanced scheme than this anyway. */
                    if (wheel_delta > 4) {
                        wheel_keycode = BUTTON_SCROLL_FWD;
                        old_wheel_value = new_wheel_value;
                    } else if (wheel_delta < -4) {
                        wheel_keycode = BUTTON_SCROLL_BACK;
                        old_wheel_value = new_wheel_value;
                    }

                    if (wheel_keycode != BUTTON_NONE)
                        queue_post(&button_queue, wheel_keycode, NULL);
                }
                else {
                    old_wheel_value = new_wheel_value;
                }
            }
            else if (clickwheel_down) {
                /* scroll wheel up */
                old_wheel_value = -1;
                clickwheel_down = 0;
            }
        }
        /* 
       Don't know why this should be needed, let me know if you do. 
       else if ((status & 0x800000FF) == 0x8000003A) {
            wheel_value = status & 0x800000FF;
        }
        */
        else if (status == 0xffffffff) {
            opto_i2c_init();
        }
    }

    if ((inl(reg) & 0x8000000) != 0) {
        outl(0xffffffff, 0x7000c120);
        outl(0xffffffff, 0x7000c124);
    }
    return btn;
}

void ipod_4g_button_int(void)
{
    CPU_HI_INT_CLR = I2C_MASK;
    /* The following delay was 250 in the ipodlinux source, but 10 seems to 
       work fine - tested on Nano, Color/Photo and Video. */
    udelay(10); 
    outl(0x0, 0x7000c140); 
    int_btn = ipod_4g_button_read();
    outl(inl(0x7000c104) | 0xC000000, 0x7000c104);
    outl(0x400a1f00, 0x7000c100);

    GPIOB_OUTPUT_VAL |= 0x10;
    CPU_INT_EN = 0x40000000;
    CPU_HI_INT_EN = I2C_MASK;
}
#endif 
#if CONFIG_KEYPAD == IPOD_3G_PAD
/* Variable to use for setting button status in interrupt handler */
int int_btn = BUTTON_NONE;

/**
 * 
 * 
 */
void handle_scroll_wheel(int new_scroll, int was_hold, int reverse)
{
    int wheel_keycode = BUTTON_NONE;
    static int prev_scroll = -1;
    static int scroll_state[4][4] = {
        {0, 1, -1, 0},
        {-1, 0, 0, 1},
        {1, 0, 0, -1},
        {0, -1, 1, 0}
    };

    if ( prev_scroll == -1 ) {
        prev_scroll = new_scroll;
    }
    else if (!was_hold) {
        switch (scroll_state[prev_scroll][new_scroll]) {
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
            break;
        default:
            /* only happens if we get out of sync */
            break;
        }
    }
    }
    if (wheel_keycode != BUTTON_NONE)
              queue_post(&button_queue, wheel_keycode, NULL);
    prev_scroll = new_scroll;
}

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
    source = inb(0xcf000040);
    if (source) {
        
        /* get current keypad status */
        state = inb(0xcf000030);
        outb(~state, 0xcf000060);
    
            if (was_hold && source == 0x40 && state == 0xbf) {
                /* ack any active interrupts */
                outb(source, 0xcf000070);
                return BUTTON_NONE;
            }
            was_hold = 0;
    
    
        if ( source & 0x20 ) {
                /* 3g hold switch is active low */
                    btn |= BUTTON_HOLD;
                    was_hold = 1;
                /* hold switch on 3g causes all outputs to go low */
                /* we shouldn't interpret these as key presses */
                goto done;
        }
        if (source & 0x1) {
             btn |= BUTTON_RIGHT;
        }
        if (source & 0x2) {
            btn |= BUTTON_SELECT;
        }
        if (source & 0x4) {
            btn |= BUTTON_PLAY;
        }
        if (source & 0x8) {
            btn |= BUTTON_LEFT;
        }
        if (source & 0x10) {
            btn |= BUTTON_MENU;
        }
    
        if (source & 0xc0) {
            handle_scroll_wheel((state & 0xc0) >> 6, was_hold, 0);
        }
    done:
    
        /* ack any active interrupts */
        outb(source, 0xcf000070);
    }
    return btn;
}

void ipod_3g_button_int(void)
{
    /**
     *  Theire is other things to do but for now ...
     * TODO: implement this function in a better way 
     **/
   int_btn = ipod_3g_button_read();
   
}
#endif 
static void button_tick(void)
{
    static int tick = 0;
    static int count = 0;
    static int repeat_speed = REPEAT_INTERVAL_START;
    static int repeat_count = 0;
    static bool repeat = false;
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
#ifdef BUTTON_RC_STOP
                            if ((btn == POWEROFF_BUTTON || btn == BUTTON_RC_STOP) &&
#else
                            if (btn == POWEROFF_BUTTON &&
#endif
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
#elif CONFIG_KEYPAD == IPOD_4G_PAD
    opto_i2c_init();
    /* hold button - enable as input */
    GPIOA_ENABLE |= 0x20;
    GPIOA_OUTPUT_EN &= ~0x20; 
    /* hold button - set interrupt levels */
    GPIOA_INT_LEV = ~(GPIOA_INPUT_VAL & 0x20);
    GPIOA_INT_CLR = GPIOA_INT_STAT & 0x20;
    /* enable interrupts */
    GPIOA_INT_EN = 0x20;
    CPU_INT_EN = 0x40000000;
    CPU_HI_INT_EN = I2C_MASK;

#elif CONFIG_KEYPAD == IPOD_3G_PAD
    outb(~inb(GPIOA_INPUT_VAL), GPIOA_INT_LEV);
    outb(inb(GPIOA_INT_STAT), GPIOA_INT_CLR);
    outb(0xff, GPIOA_INT_EN);

#endif /* CONFIG_KEYPAD */
    queue_init(&button_queue);
    button_read();
    lastbtn = button_read();
    tick_add_task(button_tick);
    reset_poweroff_timer();

#ifdef HAVE_LCD_BITMAP
    flipped = false;
#endif
}

#ifdef HAVE_LCD_BITMAP /* only bitmap displays can be flipped */
#if (CONFIG_KEYPAD != IPOD_3G_PAD) && (CONFIG_KEYPAD != IPOD_4G_PAD)
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
#else
/* We don't flip the iPod's keypad yet*/
#define button_flip(x) (x)
#endif

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

    static bool hold_button = false;
    static bool remote_hold_button = false;

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
    }
    
    /* remote buttons */
    if (!remote_hold_button)
    {
        data = adc_scan(ADC_REMOTE);

        if (data < 0x74)
            if (data < 0x40)
                if (data < 0x20)
                    if(data < 0x10)
                        btn = BUTTON_RC_STOP;
                    else
                        btn = BUTTON_RC_VOL_DOWN;
                else
                    btn = BUTTON_RC_MODE;
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
    }
    
    /* special buttons */
    data = GPIO1_READ;
    if (!hold_button && ((data & 0x20) == 0))
        btn |= BUTTON_ON;
    if (!remote_hold_button && ((data & 0x40) == 0))
        btn |= BUTTON_RC_ON;
    
#elif CONFIG_KEYPAD == IRIVER_H300_PAD

    static bool hold_button = false;
    static bool remote_hold_button = false;

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
        
        if (data < 0x50)
            if (data < 0x30)
                if (data < 0x10)
                    btn = BUTTON_SELECT;
                else
                    btn = BUTTON_UP;
            else
                btn = BUTTON_LEFT;
        else
            if (data < 0x90)
                if (data < 0x70)
                    btn = BUTTON_DOWN;
                else
                    btn = BUTTON_RIGHT;
            else
                if(data < 0xc0)
                    btn = BUTTON_OFF;
    }
        
    /* remote buttons */
    if (!remote_hold_button)
    {
        data = adc_scan(ADC_REMOTE);

        if (data < 0x74)
            if (data < 0x40)
                if (data < 0x20)
                    if(data < 0x10)
                        btn = BUTTON_RC_STOP;
                    else
                        btn = BUTTON_RC_VOL_DOWN;
                else
                    btn = BUTTON_RC_MODE;
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
    }

    /* special buttons */
    if (!hold_button)
    {
        data = GPIO_READ;
        if ((data & 0x0200) == 0)
            btn |= BUTTON_MODE;
        if ((data & 0x8000) == 0)
            btn |= BUTTON_REC;
    }
    
    data = GPIO1_READ;
    if (!hold_button && ((data & 0x20) == 0))
        btn |= BUTTON_ON;
    if (!remote_hold_button && ((data & 0x40) == 0))
        btn |= BUTTON_RC_ON;
    
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
                    if (data < 0x35c)
                        btn = BUTTON_MODE;
    }
    
    if (!button_hold() && (adc_read(ADC_BUTTON_PLAY) < 0x64))
        btn |= BUTTON_PLAY;

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

#elif (CONFIG_KEYPAD == IPOD_3G_PAD) || (CONFIG_KEYPAD == IPOD_4G_PAD)
    (void)data;
    /* The int_btn variable is set in the button interrupt handler */
    btn = int_btn;
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

#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
bool button_hold(void)
{
    return (GPIO1_READ & 0x00000002)?true:false;
}

static bool remote_button_hold_only(void)
{
    return (GPIO1_READ & 0x00100000)?true:false;
}

bool remote_button_hold(void)
{
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
    return (GPIO_READ & 0x08000000)?true:false;
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

