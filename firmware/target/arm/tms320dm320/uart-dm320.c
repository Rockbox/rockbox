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
#define RECEIVE_RING_SIZE   20

char
    uart1_send_buffer_ring[SEND_RING_SIZE],
    uart1_receive_buffer_ring[RECEIVE_RING_SIZE];

static volatile int uart1_send_count, uart1_send_read, uart1_send_write;
static volatile int uart1_receive_count, uart1_receive_read, uart1_receive_write;

void uart_init(void)
{
    /* Setup UART 1 pins:
     *  27 - input, uart1 rx
     *  28 - output, uart1 tx */
    /*  27: input , non-inverted, no-irq, falling edge, no-chat, UART RX */
    dm320_set_io(27, true, false, false, false, false, 0x01);
    
    /*  28: output, non-inverted, no-irq, falling edge, no-chat, UART TX */
    dm320_set_io(28, false, false, false, false, false, 0x01);
    
    // 8-N-1
    IO_UART1_MSR = 0xC400;
    IO_UART1_BRSR = 0x0057;
    IO_UART1_RFCR = 0x8020; /* Trigger later */
    IO_UART1_TFCR = 0x0000; /* Trigger level */

    /* init the receive buffer */
    uart1_receive_count=0;
    uart1_receive_read=0;
    uart1_receive_write=0;
    
    /* init the send buffer */
    uart1_send_count=0;
    uart1_send_read=0;
    uart1_send_write=0;

    /* Enable the interrupt */
    IO_INTC_EINT0 |= INTR_EINT0_UART1;
}

/* This function is not interrupt driven */
void uart1_putc(char ch)
{
    /* Wait for the interupt driven puts to finish */
    while(uart1_send_count>0);

    /* Wait for room in FIFO */
    while ((IO_UART1_TFCR & 0x3f) >= 0x20);
    
    /* Write character */
    IO_UART1_DTRR=ch;
}

void uart1_puts(const char *str, int size)
{
    if(size>SEND_RING_SIZE)
        panicf("Too much data passed to uart1_puts");

    /* Wait for the previous transfer to finish */
    while(uart1_send_count>0);
        
    memcpy(uart1_send_buffer_ring, str, size);
    
    /* Disable interrupt while modifying the pointers */
    IO_INTC_EINT0 &= ~INTR_EINT0_UART1;
    
    uart1_send_count=size;
    uart1_send_read=0;
    
    /* prime the hardware buffer */
    while(((IO_UART1_TFCR & 0x3f) < 0x20) && (uart1_send_count > 0))
    {
        IO_UART1_DTRR=uart1_send_buffer_ring[uart1_send_read++];
        uart1_send_count--;
    }
    
    /* Enable interrupt */
    IO_INTC_EINT0 |= INTR_EINT0_UART1;
}

void uart1_clear_queue(void)
{
    /* Disable interrupt while modifying the pointers */
    IO_INTC_EINT0 &= ~INTR_EINT0_UART1;
    uart1_receive_write=0;
    uart1_receive_count=0;
    uart1_receive_read=0;
    /* Enable interrupt */
    IO_INTC_EINT0 |= INTR_EINT0_UART1;
}

/* This function returns the number of bytes left in the queue after a read is done (negative if fail)*/
int uart1_gets_queue(char *str, int size)
{
    /* Disable the interrupt while modifying the pointers */
    IO_INTC_EINT0 &= ~INTR_EINT0_UART1;
    int retval;
    
    if(uart1_receive_count<size)
    {
        retval= -1;
    }    
    else
    {
        if(uart1_receive_read+size<=RECEIVE_RING_SIZE)
        {
            memcpy(str,uart1_receive_buffer_ring+uart1_receive_read,size);
            
            uart1_receive_read+=size;
        }
        else
        {
            int tempcount=(RECEIVE_RING_SIZE-uart1_receive_read);
            memcpy(str,uart1_receive_buffer_ring+uart1_receive_read,tempcount);
            memcpy(str+tempcount,uart1_receive_buffer_ring,size-tempcount);
            
            uart1_receive_read=size-tempcount;
        }
    
        uart1_receive_count-=size;

        retval=uart1_receive_count;
    }

    /* Enable the interrupt */
    IO_INTC_EINT0 |= INTR_EINT0_UART1;

    return retval;
}

/* UART1 receive/transmit interupt handler */
void UART1(void)
{
    IO_INTC_IRQ0 = INTR_IRQ0_UART1; /* Clear the interrupt first */
    while (IO_UART1_RFCR & 0x3f)
    {
        if (uart1_receive_count > RECEIVE_RING_SIZE)
            panicf("UART1 receive buffer overflow");
        else
        {
            if(uart1_receive_write>=RECEIVE_RING_SIZE)
                uart1_receive_write=0;

            uart1_receive_buffer_ring[uart1_receive_write]=IO_UART1_DTRR & 0xff;
            uart1_receive_write++;
            uart1_receive_count++;
        }
    }

    while ( ((IO_UART1_TFCR & 0x3f) < 0x20) && (uart1_send_count > 0) )
    {
        IO_UART1_DTRR=uart1_send_buffer_ring[uart1_send_read++];
        uart1_send_count--;
    }
}
