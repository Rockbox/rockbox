/*
 * (C) Copyright 2007 Catalin Patulea <cat@vv.carleton.ca>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */
#include "config.h"
#include "cpu.h"
#include "system.h"

/* UART 0/1 */

#define CONFIG_UART_BRSR    87
#define MAX_UART_BUFFER     32
static unsigned char uart1buffer[MAX_UART_BUFFER];
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
    IO_UART1_RFCR = 0x8000;
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
        
        /* Echo character back */
        IO_UART1_DTRR=ch;
        
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

int uart1_pollch(unsigned int ticks)
{
    while (ticks--) {
        if (IO_UART1_RFCR & 0x3f) {
            return IO_UART1_DTRR & 0xff;
        }
    }
    return -1;
}

bool uart1_available(void)
{
    return uart1count > 0;
}

void uart1_heartbeat(void)
{
    char data[5] = {0x11, 0x30, 0x11^0x30, 0x11+0x30, '\0'};
    uart1_puts(data);
}

bool uart1_getch(char *c)
{
    if (uart1count > 0)
    {
        *c = uart1buffer[uart1read];
        uart1read = (uart1read+1) % MAX_UART_BUFFER;
        uart1count--;
        return true;
    }
    return false;
}

/* UART1 receive intterupt handler */
void UART1(void)
{
    if (IO_UART1_RFCR & 0x3f)
    {
/*
        if (uart1count >= MAX_UART_BUFFER)
            panicf("UART1 buffer overflow");
*/
        uart1buffer[uart1write] = IO_UART1_DTRR & 0xff;
        uart1write = (uart1write+1) % MAX_UART_BUFFER;
        uart1count++;
    }

    IO_INTC_IRQ0 = (1<<IRQ_UART1);
}
