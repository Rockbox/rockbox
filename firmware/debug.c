/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "sh7034.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "config.h"

#ifdef DEBUG
static char debugmembuf[100];
static char debugbuf[200];
#endif

#ifndef SIMULATOR /* allow non archos platforms to display output */
#ifdef ARCHOS_RECORDER
#include "kernel.h"
#include "button.h"
#include "system.h"
#include "lcd.h"
#include "adc.h"
#endif

void debug_init(void)
{
    /* Clear it all! */
    SSR1 &= ~(SCI_RDRF | SCI_ORER | SCI_PER | SCI_FER);

    /* This enables the serial Rx interrupt, to be able to exit into the
       debugger when you hit CTRL-C */
    SCR1 |= 0x40;
    SCR1 &= ~0x80;
    IPRE |= 0xf000; /* Set to highest priority */
}

#ifdef DEBUG
static int debug_tx_ready(void)
{
    return (SSR1 & SCI_TDRE);
}

static void debug_tx_char(char ch)
{
    while (!debug_tx_ready())
    {
        ;
    }
    
    /*
     * Write data into TDR and clear TDRE
     */
    TDR1 = ch;
    SSR1 &= ~SCI_TDRE;
}

static void debug_handle_error(char ssr)
{
    (void)ssr;
    SSR1 &= ~(SCI_ORER | SCI_PER | SCI_FER);
}

static int debug_rx_ready(void)
{
   char ssr;
  
   ssr = SSR1 & ( SCI_PER | SCI_FER | SCI_ORER );
   if ( ssr )
      debug_handle_error ( ssr );
   return SSR1 & SCI_RDRF;
}

static char debug_rx_char(void)
{
   char ch;
   char ssr;

   while (!debug_rx_ready())
   {
      ;
   }

   ch = RDR1;
   SSR1 &= ~SCI_RDRF;

   ssr = SSR1 & (SCI_PER | SCI_FER | SCI_ORER);

   if (ssr)
      debug_handle_error (ssr);

   return ch;
}

static const char hexchars[] = "0123456789abcdef";

static char highhex(int  x)
{
   return hexchars[(x >> 4) & 0xf];
}

static char lowhex(int  x)
{
   return hexchars[x & 0xf];
}

static void putpacket (char *buffer)
{
    register  int checksum;

    char *src = buffer;

   /* Special debug hack. Shut off the Rx IRQ during I/O to prevent the debug
      stub from interrupting the message */
    SCR1 &= ~0x40;
   
    debug_tx_char ('$');
    checksum = 0;

    while (*src)
    {
        int runlen;
        
        /* Do run length encoding */
        for (runlen = 0; runlen < 100; runlen ++) 
        {
            if (src[0] != src[runlen]) 
            {
                if (runlen > 3) 
                {
                    int encode;
                    /* Got a useful amount */
                    debug_tx_char (*src);
                    checksum += *src;
                    debug_tx_char ('*');
                    checksum += '*';
                    checksum += (encode = runlen + ' ' - 4);
                    debug_tx_char (encode);
                    src += runlen;
                }
                else
                {
                    debug_tx_char (*src);
                    checksum += *src;
                    src++;
                }
                break;
            }
        }
    }
    
    
    debug_tx_char ('#');
    debug_tx_char (highhex(checksum));
    debug_tx_char (lowhex(checksum));

    /* Wait for the '+' */
    debug_rx_char();

    /* Special debug hack. Enable the IRQ again */
    SCR1 |= 0x40;
}


/* convert the memory, pointed to by mem into hex, placing result in buf */
/* return a pointer to the last char put in buf (null) */
static char *mem2hex (char *mem, char *buf, int count)
{
    int i;
    int ch;
    for (i = 0; i < count; i++)
    {
        ch = *mem++;
        *buf++ = highhex (ch);
        *buf++ = lowhex (ch);
    }
    *buf = 0;
    return (buf);
}

static void debug(char *msg)
{
    debugbuf[0] = 'O';
    
    mem2hex(msg, &debugbuf[1], strlen(msg));
    putpacket(debugbuf);
}
#endif /* end of DEBUG section */

void debugf(char *fmt, ...)
{
#ifdef DEBUG
    va_list ap;
    
    va_start(ap, fmt);
    vsnprintf(debugmembuf, sizeof(debugmembuf), fmt, ap);
    va_end(ap);
    debug(debugmembuf);
#else
    (void)fmt;
#endif
}

/*---------------------------------------------------*/
/*    SPECIAL DEBUG STUFF                            */
/*---------------------------------------------------*/
#ifdef ARCHOS_RECORDER
extern int ata_device;
extern int ata_io_address;

/* Test code!!! */
void dbg_ports(void)
{
    unsigned short porta;
    unsigned short portb;
    unsigned char portc;
    char buf[32];
    int button;

    lcd_clear_display();

    while(1)
    {
	porta = PADR;
	portb = PBDR;
	portc = PCDR;

	snprintf(buf, 32, "PADR: %04x", porta);
	lcd_puts(0, 0, buf);
	snprintf(buf, 32, "PBDR: %04x", portb);
	lcd_puts(0, 1, buf);

	snprintf(buf, 32, "AN0: %03x AN4: %03x", adc_read(0), adc_read(4));
	lcd_puts(0, 3, buf);
	snprintf(buf, 32, "AN1: %03x AN5: %03x", adc_read(1), adc_read(5));
	lcd_puts(0, 4, buf);
	snprintf(buf, 32, "AN2: %03x AN6: %03x", adc_read(2), adc_read(6));
	lcd_puts(0, 5, buf);
	snprintf(buf, 32, "AN3: %03x AN7: %03x", adc_read(3), adc_read(7));
	lcd_puts(0, 6, buf);

	snprintf(buf, 32, "%s : 0x%x",
             ata_device?"slave":"master", ata_io_address);
	lcd_puts(0, 7, buf);
	
	lcd_update();
	sleep(HZ/10);

	button = button_get(false);

	switch(button)
	{
	case BUTTON_ON:
	    /* Toggle the charger */
	    PBDR ^= 0x20;
	    break;

	case BUTTON_OFF:
	    /* Disable the charger */
	    PBDR |= 0x20;
	    return;
	}
    }
}
#endif

#else /* SIMULATOR code coming up */

void debug_init(void)
{
}

void debugf(char *fmt, ...)
{
    va_list ap;
    va_start( ap, fmt );
    vfprintf( stderr, fmt, ap );
    va_end( ap );
}
#endif
