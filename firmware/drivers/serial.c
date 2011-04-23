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
#include "button.h"
#include "config.h"
#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "lcd.h"
#include "serial.h"
#include "iap.h"

#if defined(IPOD_ACCESSORY_PROTOCOL)
static int autobaud = 0;
void serial_setup (void)
{
    int tmp;

#if defined(IPOD_COLOR) || defined(IPOD_4G)
    /* Route the Tx/Rx pins.  4G Ipod??? */
    outl(0x70000018, inl(0x70000018) & ~0xc00);
#elif defined(IPOD_NANO) || defined(IPOD_VIDEO)
    /* Route the Tx/Rx pins.  5G Ipod */
    (*(volatile unsigned long *)(0x7000008C)) &= ~0x0C;
    GPO32_ENABLE &= ~0x0C;
#endif

    DEV_EN = DEV_EN | DEV_SER0;
    CPU_HI_INT_DIS = SER0_MASK;

    DEV_RS |= DEV_SER0;
    sleep(1);
    DEV_RS &= ~DEV_SER0;

    SER0_LCR = 0x80; /* Divisor latch enable */
    SER0_DLM = 0x00;
    SER0_LCR = 0x03; /* Divisor latch disable, 8-N-1 */
    SER0_IER = 0x01;

    SER0_FCR = 0x07; /* Tx+Rx FIFO reset and FIFO enable */

    CPU_INT_EN |= HI_MASK;
    CPU_HI_INT_EN |= SER0_MASK;
    tmp = SER0_RBR;

    serial_bitrate(0);
}

void serial_bitrate(int rate)
{
    if(rate == 0)
    {
        autobaud = 2;
        SER0_LCR = 0x80; /* Divisor latch enable */
        SER0_DLL = 0x0D; /* 24000000/13/16 = 115384 baud */
        SER0_LCR = 0x03; /* Divisor latch disable, 8-N-1 */
        return;
    }

    autobaud = 0;
    SER0_LCR = 0x80; /* Divisor latch enable */
    SER0_DLL = 24000000L / rate / 16;
    SER0_LCR = 0x03; /* Divisor latch disable, 8-N-1 */
}

int tx_rdy(void)
{
    if((SER0_LSR & 0x20))
        return 1;
    else
        return 0;
}

int rx_rdy(void)
{
    if((SER0_LSR & 0x1))
        return 1;
    else
        return 0;
}

void tx_writec(unsigned char c)
{
    SER0_THR =(int) c;
}

unsigned char rx_readc(void)
{
    return (SER0_RBR & 0xFF);
}

void SERIAL0(void)
{
    static int badbaud = 0;
    static bool newpkt = true;
    char temp;

    while(rx_rdy())
    {
        temp = rx_readc();
        if (newpkt && autobaud > 0)
        {
            if (autobaud == 1)
            {
                switch (temp)
                {
                    case 0xFF:
                    case 0x55:
                        break;
                    case 0xFC:
                        SER0_LCR = 0x80; /* Divisor latch enable */
                        SER0_DLL = 0x4E; /* 24000000/78/16 = 19230 baud */
                        SER0_LCR = 0x03; /* Divisor latch disable, 8-N-1 */
                        temp = 0xFF;
                        break;
                    case 0xE0:
                        SER0_LCR = 0x80; /* Divisor latch enable */
                        SER0_DLL = 0x9C; /* 24000000/156/16 = 9615 baud */
                        SER0_LCR = 0x03; /* Divisor latch disable, 8-N-1 */
                        temp = 0xFF;
                        break;
                    default:
                        badbaud++;
                        if (badbaud >= 6) /* Switch baud detection mode */
                        {
                            autobaud = 2;
                            SER0_LCR = 0x80; /* Divisor latch enable */
                            SER0_DLL = 0x0D; /* 24000000/13/16 = 115384 baud */
                            SER0_LCR = 0x03; /* Divisor latch disable, 8-N-1 */
                            badbaud = 0;
                        } else {
                            SER0_LCR = 0x80; /* Divisor latch enable */
                            SER0_DLL = 0x1A; /* 24000000/26/16 = 57692 baud */
                            SER0_LCR = 0x03; /* Divisor latch disable, 8-N-1 */
                        }
                        continue;
                }
            } else {
                switch (temp)
                {
                    case 0xFF:
                    case 0x55:
                        break;
                    case 0xFE:
                        SER0_LCR = 0x80; /* Divisor latch enable */
                        SER0_DLL = 0x1A; /* 24000000/26/16 = 57692 baud */
                        SER0_LCR = 0x03; /* Divisor latch disable, 8-N-1 */
                        temp = 0xFF;
                        break;
                    case 0xFC:
                            SER0_LCR = 0x80; /* Divisor latch enable */
                        SER0_DLL = 0x27; /* 24000000/39/16 = 38461 baud */
                        SER0_LCR = 0x03; /* Divisor latch disable, 8-N-1 */
                        temp = 0xFF;
                        break;
                    case 0xE0:
                        SER0_LCR = 0x80; /* Divisor latch enable */
                        SER0_DLL = 0x4E; /* 24000000/78/16 = 19230 baud */
                        SER0_LCR = 0x03; /* Divisor latch disable, 8-N-1 */
                        temp = 0xFF;
                        break;
                    default:
                        badbaud++;
                        if (badbaud >= 6) /* Switch baud detection */
                        {
                            autobaud = 1;
                            SER0_LCR = 0x80; /* Divisor latch enable */
                            SER0_DLL = 0x1A; /* 24000000/26/16 = 57692 baud */
                            SER0_LCR = 0x03; /* Divisor latch disable, 8-N-1 */
                            badbaud = 0;
                        } else {
                            SER0_LCR = 0x80; /* Divisor latch enable */
                            SER0_DLL = 0x0D; /* 24000000/13/16 = 115384 baud */
                            SER0_LCR = 0x03; /* Divisor latch disable, 8-N-1 */
                        }
                        continue;
                }
            }
        }
        bool pkt = iap_getc(temp);
        if(newpkt && !pkt)
            autobaud = 0; /* Found good baud */
        newpkt = pkt;
    }
}
#endif

void dprintf(const char * str, ... )
{
    char dprintfbuff[256];
    char * ptr;

    va_list ap;
    va_start(ap, str);

    ptr = dprintfbuff;
    vsnprintf(ptr,sizeof(dprintfbuff),str,ap);
    va_end(ap);

    serial_tx((unsigned char *)ptr);
}

void serial_tx(const unsigned char * buf)
{
    /*Tx*/
    for(;;) {
        if(tx_rdy()) {
            if(*buf == '\0')
                return;
            if(*buf == '\n')
                tx_writec('\r');
            tx_writec(*buf);
            buf++;
        }
    }
}
