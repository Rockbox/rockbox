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

#include "config.h"

#include <serial.h>
#include <lcd.h>

#define TDRE 7 /* transmit data register empty */
#define RDRF 6 /* receive data register full   */
#define ORER 5 /* overrun error                */
#define FER  4 /* frame   error                */
#define PER  3 /* parity  error                */

static int serial_byte,serial_flag;

void serial_putc (char byte) 
	{
    static int i = 0;
		while (!(QI(SCISSR1) & (1<<TDRE)));
		QI(SCITDR1) = byte;
		clear_bit (TDRE,SCISSR1);
    lcd_goto ((i++)%11,1); lcd_putc (byte);
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
  	QI(SCISCR1) =
  	QI(SCISSR1) =
  	QI(SCISMR1) = 0;
  	QI(SCIBRR1) = (PHI/(32*baudrate))-1;	
  	QI(SCISCR1) = 0x70; 
	}

#pragma interrupt
void REI1 (void)
  {
		clear_bit (FER,SCISSR1);
  }

#pragma interrupt
void RXI1 (void)
  {
    serial_byte = QI(SCIRDR1);
    serial_flag = 1;
		clear_bit (RDRF,SCISSR1);
    if (serial_byte == '0')
      lcd_turn_off_backlight ();
    if (serial_byte == '1')
      lcd_turn_on_backlight ();
  }
