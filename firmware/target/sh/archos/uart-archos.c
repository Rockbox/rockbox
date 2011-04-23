/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr & Nick Robinson
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
#include <stdlib.h>
#include <stdarg.h>

#include "config.h"
#include "button.h"
#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "serial.h"

/* FIX: this doesn't work on iRiver or iPod yet */
/* iFP7xx has no remote */

/* Received byte identifiers */
#define PLAY  0xC1
#define STOP  0xC2
#define PREV  0xC4
#define NEXT  0xC8
#define VOLUP 0xD0
#define VOLDN 0xE0

void serial_setup (void)
{
    /* Set PB10 function to serial Rx */
    PBCR1 = (PBCR1 & 0xffcf) | 0x0020;

    SMR1 = 0x00;
    SCR1 = 0;
    BRR1 = (FREQ/(32*9600))-1;
    and_b(0, &SSR1); /* The status bits must be read before they are cleared,
                        so we do an AND operation */

    /* Let the hardware settle. The serial port needs to wait "at least
       the interval required to transmit or receive one bit" before it
       can be used. */
    sleep(1);

    SCR1 = 0x10; /* Enable the receiver, no interrupt */
}

int tx_rdy(void)
{
    /* a dummy */
    return 1;
}

int rx_rdy(void)
{
    if(SSR1 & SCI_RDRF)
        return 1;
    else
        return 0;
}

void tx_writec(unsigned char c)
{
    /* a dummy */
    (void)c;
}

unsigned char rx_readc(void)
{
    char tmp;
    /* Read byte and clear the Rx Full bit */
    tmp = RDR1;
    and_b(~SCI_RDRF, &SSR1);
    return tmp;
}


/* This function returns the received remote control code only if it is
   received without errors before or after the reception.
   It therefore returns the received code on the second call after the
   code has been received. */
int remote_control_rx(void)
{
    static int last_valid_button = BUTTON_NONE;
    static int last_was_error = false;
    int btn;
    int ret = BUTTON_NONE;

    /* Errors? Just clear'em. The receiver stops if we don't */
    if(SSR1 & (SCI_ORER | SCI_FER | SCI_PER)) {
        and_b(~(SCI_ORER | SCI_FER | SCI_PER), &SSR1);
        last_valid_button = BUTTON_NONE;
        last_was_error = true;
        return BUTTON_NONE;
    }

    if(rx_rdy()) {
        btn = rx_readc();

        if(last_was_error)
        {
            last_valid_button = BUTTON_NONE;
            ret = BUTTON_NONE;
        }
        else
        {
            switch (btn)
            {
                case STOP:
                    last_valid_button = BUTTON_RC_STOP;
                    break;

                case PLAY:
                    last_valid_button = BUTTON_RC_PLAY;
                    break;

                case VOLUP:
                    last_valid_button = BUTTON_RC_VOL_UP;
                    break;

                case VOLDN:
                    last_valid_button = BUTTON_RC_VOL_DOWN;
                    break;

                case PREV:
                    last_valid_button = BUTTON_RC_LEFT;
                    break;

                case NEXT:
                    last_valid_button = BUTTON_RC_RIGHT;
                    break;

                default:
                    last_valid_button = BUTTON_NONE;
                    break;
            }
        }
    }
    else
    {
        /* This means that a valid remote control character was received
           the last time we were called, with no receiver errors either before
           or after. Then we can assume that there really is a remote control
           attached, and return the button code. */
        ret = last_valid_button;
        last_valid_button = BUTTON_NONE;
    }

    last_was_error = false;

    return ret;
}

