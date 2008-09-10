/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Catalin Patulea <cat@vv.carleton.ca>
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
#include "cpu.h"
#include "system.h"
#include "string.h"
#include "panic.h"
#include "uart-target.h"

#define MAX_UART_BUFFER     31
#define SEND_RING_SIZE      256
#define RECIEVE_RING_SIZE   20

char
//    uart1_send_buffer_ring[SEND_RING_SIZE],
    uart1_recieve_buffer_ring[RECIEVE_RING_SIZE];

//static unsigned int uart1_send_count, uart1_send_read, uart1_send_write;
static unsigned int uart1_recieve_count, uart1_recieve_read, uart1_recieve_write;

void uart_init(void)
{
    // 8-N-1
    IO_UART1_MSR=0x8000;
    IO_UART1_BRSR=0x0057;
    IO_UART1_RFCR = 0x8010; /* Trigger later */
    /* gio 27 is input, uart1 rx
       gio 28 is output, uart1 tx */
    IO_GIO_DIR1 |=  (1<<11); /* gio 27 */
    IO_GIO_DIR1 &= ~(1<<12); /* gio 28 */

    /* init the recieve buffer */
    uart1_recieve_count=0;
    uart1_recieve_read=0;
    uart1_recieve_write=0;

    /* Enable the interrupt */
    IO_INTC_EINT0 |= INTR_EINT0_UART1;
}

void uart1_putc(char ch)
{
    /* Wait for room in FIFO */
    while ((IO_UART1_TFCR & 0x3f) >= 0x20);
    
    /* Write character */
    IO_UART1_DTRR=ch;
}

void uart1_puts(const char *str, int size)
{
    int count=0;
    while (count<size) 
    {
        uart1_putc(str[count]);
        count++;
    }
}

/* This function returns the number of bytes left in the queue after a read is done (negative if fail)*/
int uart1_gets_queue(char *str, unsigned int size)
{
    IO_INTC_EINT0 &= ~INTR_EINT0_UART1;
    int retval;
    
    if(uart1_recieve_count<size)
    {
        retval= -1;
    }    
    else
    {
        if(uart1_recieve_read+size<RECIEVE_RING_SIZE)
        {
            memcpy(str,uart1_recieve_buffer_ring+uart1_recieve_read,size);
        }
        else
        {
            int tempcount=(RECIEVE_RING_SIZE-uart1_recieve_read);
            memcpy(str,uart1_recieve_buffer_ring+uart1_recieve_read,tempcount);
            memcpy(str+tempcount,uart1_recieve_buffer_ring,size-tempcount);
        }
    
        uart1_recieve_count-=size;
    
        if(uart1_recieve_read+size<RECIEVE_RING_SIZE)
            uart1_recieve_read+=size;
        else
            uart1_recieve_read=size-(RECIEVE_RING_SIZE-uart1_recieve_read);
            
        retval=uart1_recieve_count;
    }

    /* Enable the interrupt */
    IO_INTC_EINT0 |= INTR_EINT0_UART1;

    return retval;
}

/* UART1 receive interupt handler */
void UART1(void)
{
    while (IO_UART1_RFCR & 0x3f)
    {
        if (uart1_recieve_count > RECIEVE_RING_SIZE)
            panicf("UART1 buffer overflow");
        else
        {
            if(uart1_recieve_write==RECIEVE_RING_SIZE)
                uart1_recieve_write=0;

            uart1_recieve_buffer_ring[uart1_recieve_write] = IO_UART1_DTRR & 0xff;
            uart1_recieve_write++;
            uart1_recieve_count++;
        }
    }

    IO_INTC_IRQ0 = INTR_IRQ0_UART1;
}
