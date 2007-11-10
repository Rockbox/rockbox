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
#ifndef UART_H
#define UART_H

void uart_init(void);
bool uart1_getch(char *c);
void uart1_heartbeat(void);
bool uart1_available(void);

int uart1_gets_queue(char *, unsigned int);
void uart1_puts(const char *str);
void uart1_gets(char *str, unsigned int size);
int  uart1_pollch(unsigned int ticks);
void uart1_putc(char ch);
void uart1_putHex(unsigned int n);

#endif
