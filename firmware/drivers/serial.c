/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "serial.h"

#define TDRE 7 /* transmit data register empty */
#define RDRF 6 /* receive data register full   */
#define ORER 5 /* overrun error                */
#define FER  4 /* frame   error                */
#define PER  3 /* parity  error                */

static int serial_byte,serial_flag;

void serial_putc (char byte) 
{
    while (!(SSR1 & (1<<TDRE)));
    TDR1 = byte;
    clear_bit(TDRE,SSR1_ADDR);
}

void serial_puts (char const *string) 
{
    int byte;
    while ((byte = *string++))
	serial_putc (byte);
}

int serial_getc( void )
{
    int byte;
    while (!serial_flag);
    byte = serial_byte;
    serial_flag = 0;
    serial_putc (byte);
    return byte;
}

void serial_setup (int baudrate) 
{ 	
    SCR1 = 0;
    SSR1 = 0;
    SMR1 = 0;
    BRR1 = (FREQ/(32*baudrate))-1;
    SCR1 = 0x70;
}

#pragma interrupt
void REI1 (void)
{
    clear_bit (FER,SSR1_ADDR);
}

#pragma interrupt
void RXI1 (void)
{
    serial_byte = RDR1;
    serial_flag = 1;
    clear_bit(RDRF,SSR1_ADDR);
}
