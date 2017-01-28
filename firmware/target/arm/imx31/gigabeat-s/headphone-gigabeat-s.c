/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2009 by Michael Sevakis
 *
 * Driver to handle headphone jack events
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
#include "system.h"
#include "kernel.h"
#include "thread.h"
#include "mc13783.h"
#include "adc.h"
#include "button.h"

static struct semaphore headphone_wakeup;
static unsigned int headphone_thread_id;
static unsigned int headphone_stack[176/sizeof(int)]; /* Little stack needed */
static const char * const headphone_thread_name = "headphone";
static bool headphones_detect = false;

/* Convert ADC reading into a button value. */
static int adc_data_to_button(unsigned int data)
{
    /*               _______370_______
     *           ___149___       ___675___
     *        ___64__ __252__ __505__ __870__
     *        x  PLAY DSP REW FF VOL+ VOL-  x
     *
     * Child nodes are at 2*n and 2*n+1 per usual bintree array representation
     */
    static const unsigned int button_tree[16] =
    {
        [ 0] =   0,
        [ 1] = 370,
        [ 2] = 149,
        [ 3] = 675,
        [ 4] =  64,
        [ 5] = 252,
        [ 6] = 505,
        [ 7] = 870,
        [ 8] = BUTTON_NONE,
        [ 9] = BUTTON_RC_PLAY,
        [10] = BUTTON_RC_DSP,
        [11] = BUTTON_RC_REW,
        [12] = BUTTON_RC_FF,
        [13] = BUTTON_RC_VOL_UP,
        [14] = BUTTON_RC_VOL_DOWN,
        [15] = BUTTON_NONE,
    };

    int i, button;

    asm volatile (
        "ldr    %0, [%2, #1*4]       \n" /* button = button_tree[1] */
        "mov    %1, #1               \n" /* i = 1 */
        "cmp    %3, %0               \n" /* C=1 if data > button */
        "adc    %1, %1, %1           \n" /* i = 2*n + C */
        "ldr    %0, [%2, %1, lsl #2] \n" /* button = button_tree[i] */
        "cmp    %3, %0               \n" /* C=1 if data > button */
        "adc    %1, %1, %1           \n" /* i = 2*n + C */
        "ldr    %0, [%2, %1, lsl #2] \n" /* button = button_tree[i] */
        "cmp    %3, %0               \n" /* C=1 if data > button */
        "adc    %1, %1, %1           \n" /* i = 2*n + C */
        "ldr    %0, [%2, %1, lsl #2] \n" /* button = button_tree[i] */
        : "=&r"(button), "=&r"(i)
        : "r"(button_tree), "r"(data));

    return button;
}

static void NORETURN_ATTR headphone_thread(void)
{
    int headphone_sleep_countdown = 0;
    int headphone_wait_timeout = TIMEOUT_BLOCK;
    int last_btn = BUTTON_NONE;

    while (1)
    {
        int rc = semaphore_wait(&headphone_wakeup, headphone_wait_timeout);
        unsigned int data = adc_read(ADC_HPREMOTE);

        if (rc == OBJ_WAIT_TIMEDOUT)
        {
            if (headphone_sleep_countdown <= 0)
            {
                /* Polling ADC */
                int btn = adc_data_to_button(data);
                if (btn != last_btn)
                {
                    last_btn = btn;
                    /* If the buttons dont agree twice in a row, then it's
                     * none (from meg-fx remote reader). */
                    btn = BUTTON_NONE;
                }

                button_headphone_set(btn);
                continue;
            }

            if (--headphone_sleep_countdown == 0)
            {
                /* Nothing has changed and remote is not present -
                 * go to sleep. */
                headphone_wait_timeout = TIMEOUT_BLOCK;
                continue;
            }
        }

        headphones_detect = data <= 951; /* Max remote value */

        /* Cancel any buttons if jack readings are unstable. */
        button_headphone_set(BUTTON_NONE);
        last_btn = BUTTON_NONE;

        if (data >= 64 && data <= 951)
        {
            /* Should be a remote control - accelerate */
            headphone_wait_timeout = HZ/25;
            headphone_sleep_countdown = 0;
        }
        else if (rc == OBJ_WAIT_SUCCEEDED)
        {
            /* Got signaled - something is being plugged/unplugged. Set
             * countdown until we just give up and go to sleep (~10s). */
            headphone_wait_timeout = HZ/2;
            headphone_sleep_countdown = 10*2;
        }
    }
}

/* HP plugged/unplugged event - called from PMIC ISR */
void MC13783_EVENT_CB_ONOFD2(void)
{
    /* Trigger the thread immediately. */
    semaphore_release(&headphone_wakeup);
}

/* Tell if anything is in the jack. */
bool headphones_inserted(void)
{
    return headphones_detect;
}

void INIT_ATTR headphone_init(void)
{
    /* A thread is required to monitor the remote ADC and jack state. */
    semaphore_init(&headphone_wakeup, 1, 1);
    headphone_thread_id = create_thread(headphone_thread, 
                                        headphone_stack,
                                        sizeof(headphone_stack),
                                        0, headphone_thread_name
                                        IF_PRIO(, PRIORITY_REALTIME)
                                        IF_COP(, CPU));

    /* Enable PMIC event */
    mc13783_enable_event(MC13783_INT_ID_ONOFD2, true);
}
