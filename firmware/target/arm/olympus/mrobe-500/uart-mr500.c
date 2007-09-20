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
#define IO_UART0_DTRR             0x0300
#define IO_UART0_BRSR             0x0302
#define IO_UART0_MSR              0x0304
#define IO_UART0_RFCR             0x0306
#define IO_UART0_TFCR             0x0308
#define IO_UART0_LCR              0x030A
#define IO_UART0_SR               0x030C

#define IO_UART1_DTRR             0x0380
#define IO_UART1_BRSR             0x0382
#define IO_UART1_MSR              0x0384
#define IO_UART1_RFCR             0x0386
#define IO_UART1_TFCR             0x0388
#define IO_UART1_LCR              0x038A
#define IO_UART1_SR               0x038C
#define CONFIG_UART_BRSR            87

void do_checksums(char *data, int len, char *xor, char *add)
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

void uartSetup(void) {
    // 8-N-1
    outw(0x8000, IO_UART1_MSR);
    outw(CONFIG_UART_BRSR, IO_UART1_BRSR);
}

void uartPutc(char ch) {
    // Wait for room in FIFO
    while ((inw(IO_UART1_TFCR) & 0x3f) >= 0x20);
    
    // Write character
    outw(ch, IO_UART1_DTRR);
}

// Unsigned integer to ASCII hexadecimal conversion
void uartPutHex(unsigned int n) {
    unsigned int i;

    for (i = 8; i != 0; i--) {
        unsigned int digit = n >> 28;
        uartPutc(digit >= 10 ? digit - 10 + 'A' : digit + '0');
        n <<= 4;
    }
}

void uartPuts(const char *str) {
    char ch;
    while ((ch = *str++) != '\0') {
        uartPutc(ch);
    }
}

void uartGets(char *str, unsigned int size) {
    for (;;) {
        char ch;
        
        // Wait for FIFO to contain something
        while ((inw(IO_UART1_RFCR) & 0x3f) == 0);
        
        // Read character
        ch = (char)inw(IO_UART1_DTRR);
        
        // Echo character back
        outw(ch, IO_UART1_DTRR);
        
        // If CR, also echo LF, null-terminate, and return
        if (ch == '\r') {
            outw('\n', IO_UART1_DTRR);
            if (size) {
                *str++ = '\0';
            }
            return;
        }
        
        // Append to buffer
        if (size) {
            *str++ = ch;
            --size;
        }
    }
}

int uartPollch(unsigned int ticks) {
    while (ticks--) {
        if (inw(IO_UART1_RFCR) & 0x3f) {
            return inw(IO_UART1_DTRR) & 0xff;
        }
    }
    
    return -1;
}

bool uartAvailable(void)
{
    return (inw(IO_UART1_RFCR) & 0x3f)?true:false;
}

void uartHeartbeat(void)
{
    char data[5] = {0x11, 0x30, 0x11^0x30, 0x11+0x30, '\0'};
    uartPuts(data);
}

void uartSendData(char* data, int len)
{
    int i;
    for(i=0;i<len;i++)
        uartPutc(data[i]);
}
