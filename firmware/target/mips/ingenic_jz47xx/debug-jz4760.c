/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Vortex
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
#include "cpu.h"
#include <stdarg.h>
#include <stdio.h>
#include "lcd.h"
#include "kernel.h"
#include "font.h"
#include "button.h"
#include "timefuncs.h"

#define CFG_UART_BASE		UART1_BASE	/* Base of the UART channel */

void serial_putc (const char c)
{
	volatile u8 *uart_lsr = (volatile u8 *)(CFG_UART_BASE + OFF_LSR);
	volatile u8 *uart_tdr = (volatile u8 *)(CFG_UART_BASE + OFF_TDR);

	if (c == '\n') serial_putc ('\r');

	/* Wait for fifo to shift out some bytes */
	while ( !((*uart_lsr & (UARTLSR_TDRQ | UARTLSR_TEMT)) == 0x60) );

	*uart_tdr = (u8)c;
}

void serial_puts (const char *s)
{
	while (*s) {
		serial_putc (*s++);
	}
}

void serial_putsf(const char *format, ...)
{
    static char printfbuf[256];
    int len;
    unsigned char *ptr;
    va_list ap;
    va_start(ap, format);

    ptr = printfbuf;
    len = vsnprintf(ptr, sizeof(printfbuf), format, ap);
    va_end(ap);

    serial_puts(ptr);
    serial_putc('\n');
}

void serial_put_hex(unsigned int  d)
{
	char c[12];
	int i;
	for(i = 0; i < 8;i++)
	{
		c[i] = (d >> ((7 - i) * 4)) & 0xf;
		if(c[i] < 10)
			c[i] += 0x30;
		else
			c[i] += (0x41 - 10);
	}
	c[8] = '\n';
	c[9] = 0;
	serial_puts(c);

}

void serial_put_dec(unsigned int  d)
{
        char c[16];
        int i;
        int j = 0;
        int x = d;

        while (x /= 10)
                j++;

        for (i = j; i >= 0; i--) {
                c[i] = d % 10;
		c[i] += 0x30;
                d /= 10;
        }
        c[j + 1] = '\n';
        c[j + 2] = 0;
        serial_puts(c);
}

void serial_dump_data(unsigned char* data, int len)
{
	int i;
	for(i=0; i<len; i++)
	{
		unsigned char a = ((*data)>>4) & 0xf;
		if(a < 10)
			a += 0x30;
		else
			a += (0x41 - 10);
		serial_putc( a );
		
		a = (*data) & 0xf;
		if(a < 10)
			a += 0x30;
		else
			a += (0x41 - 10);
		serial_putc( a );

		serial_putc( ' ' );
		
		data++;
	}
	
	serial_putc( '\n' );
}

bool dbg_ports(void)
{
    serial_puts("dbg_ports\n");
    return false;
}

bool dbg_hw_info(void)
{
    serial_puts("dbg_hw_info\n");
    return false;
}
