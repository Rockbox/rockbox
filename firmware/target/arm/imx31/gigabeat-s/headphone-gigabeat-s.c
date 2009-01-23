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
#include "mc13783-target.h"
#include "adc.h"
#include "button.h"

static struct wakeup headphone_wakeup;
static unsigned int headphone_thread_id;
static int headphone_stack[160/sizeof(int)]; /* Not much stack needed */
static const char * const headphone_thread_name = "headphone";
static bool headphones_detect = false;

/* Convert ADC reading into a button value. */
static int adc_data_to_button(unsigned int data)
{
    int btn = BUTTON_NONE;

    if (data < 505)
    {
        if (data < 252)
        {
            if (data < 149)
            {
                if (data >= 64)
                {
                    /* Play/Pause */
                    btn = BUTTON_RC_PLAY;
                }
                /* else headphone direct */
            }
            else
            {
                /* DSP */
                btn = BUTTON_RC_DSP;
            }
        }
        else
        {
            if (data < 370)
            {
                /* RW */
                btn = BUTTON_RC_REW;
            }
            else
            {
                /* FF */
                btn = BUTTON_RC_FF;
            }
        }
    }
    else
    {
        if (data < 870)
        {
            if (data < 675)
            {
                /* Vol + */
                btn = BUTTON_RC_VOL_UP;
            }
            else
            {
                /* Vol - */
                btn = BUTTON_RC_VOL_DOWN;
            }
        }
#if 0
        else
        {

            if (data < 951)
            {
                /* No buttons */                            
            }
            else
            {
                /* Not inserted */

            }
        }
#endif
    }

    return btn;
}

static void headphone_thread(void)
{
    int headphone_sleep_countdown = 0;
    int headphone_wait_timeout = TIMEOUT_BLOCK;

    while (1)
    {
        int rc = wakeup_wait(&headphone_wakeup, headphone_wait_timeout);
        unsigned int data = adc_read(ADC_HPREMOTE);

        if (rc == OBJ_WAIT_TIMEDOUT)
        {
            if (headphone_sleep_countdown <= 0)
            {
                /* Polling ADC */
                int btn, btn2;

                btn = adc_data_to_button(data);
                sleep(HZ/50);
                data = adc_read(ADC_HPREMOTE);
                btn2 = adc_data_to_button(data);

                if (btn != btn2)
                {
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

        if (data >= 64 && data <= 951)
        {
            /* Should be a remote control - accelerate */
            headphone_wait_timeout = HZ/20-HZ/50;
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

/* This is called from the mc13783 interrupt thread */
void headphone_detect_event(void)
{
    /* Trigger the thread immediately. */
    wakeup_signal(&headphone_wakeup);
}

/* Tell if anything is in the jack. */
bool headphones_inserted(void)
{
    return headphones_detect;
}

void headphone_init(void)
{
    /* A thread is required to monitor the remote ADC and jack state. */
    wakeup_init(&headphone_wakeup);
    headphone_thread_id = create_thread(headphone_thread, 
                                        headphone_stack,
                                        sizeof(headphone_stack),
                                        0, headphone_thread_name
                                        IF_PRIO(, PRIORITY_REALTIME)
                                        IF_COP(, CPU));

    /* Initially poll and then enable PMIC event */
    headphone_detect_event();
    mc13783_enable_event(MC13783_ONOFD2_EVENT);
}
