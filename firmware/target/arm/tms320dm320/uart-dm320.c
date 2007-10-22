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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "cpu.h"
#include "system.h"

/* UART 0/1 */

#define CONFIG_UART_BRSR    87
#define MAX_UART_BUFFER     31
unsigned char uart1buffer[MAX_UART_BUFFER];
int uart1read = 0, uart1write = 0, uart1count = 0;

/*
static void do_checksums(char *data, int len, char *xor, char *add)
{
    int i;
    *xor = data[0];
    *add = data[0];
    for(i=1;i<len;i++)
    {
        *xor ^= data[i];
        *add += data[i];
    }
}
*/

void uart_init(void)
{
    // 8-N-1
    IO_UART1_MSR=0x8000;
    IO_UART1_BRSR=CONFIG_UART_BRSR;
    IO_UART1_RFCR = 0x8010; /* Trigger later */
    /* gio 27 is input, uart1 rx
       gio 28 is output, uart1 tx */
    IO_GIO_DIR1 |=  (1<<11); /* gio 27 */
    IO_GIO_DIR1 &= ~(1<<12); /* gio 28 */

    /* init the recieve buffer */
    uart1read = 0;
    uart1write = 0;
    uart1count = 0;
    
    /* Enable the interrupt */
    IO_INTC_EINT0 |= (1<<IRQ_UART1);
}

void uart1_putc(char ch)
{
    /* Wait for room in FIFO */
    while ((IO_UART1_TFCR & 0x3f) >= 0x20);
    
    /* Write character */
    IO_UART1_DTRR=ch;
}

/* Unsigned integer to ASCII hexadecimal conversion */
void uart1_putHex(unsigned int n)
{
    unsigned int i;

    for (i = 8; i != 0; i--) {
        unsigned int digit = n >> 28;
        uart1_putc(digit >= 10 ? digit - 10 + 'A' : digit + '0');
        n <<= 4;
    }
}

void uart1_puts(const char *str)
{
    char ch;
    while ((ch = *str++) != '\0') {
        uart1_putc(ch);
    }
}

void uart1_gets(char *str, unsigned int size)
{
    for (;;) {
        char ch;
        
        /* Wait for FIFO to contain something */
        while ((IO_UART1_RFCR & 0x3f) == 0);
        
        /* Read character */
        ch = (char)IO_UART1_DTRR;

        /* If CR, also echo LF, null-terminate, and return */
        if (ch == '\r') {
            IO_UART1_DTRR='\n';
            if (size) {
                *str++ = '\0';
            }
            return;
        }
        
        /* Append to buffer */
        if (size) {
            *str++ = ch;
            --size;
        }
    }
}

bool uart1_getch(char *c)
{
    if (uart1count > 0)
    {
        if(uart1read>MAX_UART_BUFFER)
            uart1read=0;

        *c = uart1buffer[uart1read++];
        uart1count--;
        return true;
    }
    return false;
}

/* UART1 receive intterupt handler */
void UART1(void)
{
    while (IO_UART1_RFCR & 0x3f)
    {
        if (uart1count > MAX_UART_BUFFER)
            panicf("UART1 buffer overflow");
        else
        {
            if(uart1write>MAX_UART_BUFFER)
                uart1write=0;

            uart1buffer[uart1write++] = IO_UART1_DTRR & 0xff;
            uart1count++;
        }
    }

    IO_INTC_IRQ0 = (1<<IRQ_UART1);
}
