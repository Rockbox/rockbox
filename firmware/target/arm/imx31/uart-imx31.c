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

#include "serial-imx31.h"

void serial_setup(void)
{
#ifdef UART_INT /*enable UART Interrupts */
    UCR1_1 |= (EUARTUCR1_TRDYEN | EUARTUCR1_RRDYEN | EUARTUCR1_TXMPTYEN);
    UCR4_1 |= (EUARTUCR4_TCEN);
#else /*disable UART Interrupts*/
    UCR1_1 &= ~(EUARTUCR1_TRDYEN | EUARTUCR1_RRDYEN | EUARTUCR1_TXMPTYEN);
    UCR4_1 &= ~(EUARTUCR4_TCEN);
#endif
    UCR1_1 |= EUARTUCR1_UARTEN;
    UCR2_1 |= (EUARTUCR2_TXEN  | EUARTUCR2_RXEN | EUARTUCR2_IRTS);

    /* Tx,Rx Interrupt Trigger levels, Disable for now*/
    /*UFCR1 |= (UFCR1_TXTL_32 | UFCR1_RXTL_32);*/
}

int tx_rdy(void)
{
    if((UTS1 & EUARTUTS_TXEMPTY))
        return 1;
    else
        return 0;
}

/*Not ready...After first Rx, UTS1 & UTS1_RXEMPTY
  keeps returning true*/
int rx_rdy(void)
{
    if(!(UTS1 & EUARTUTS_RXEMPTY))
        return 1;
    else
        return 0;
}

void tx_writec(unsigned char c)
{
    UTXD1=(int) c;
}

