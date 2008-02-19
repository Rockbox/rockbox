/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr & Nick Robinson
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdlib.h>
#include "button.h"
#include "config.h"
#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "backlight.h"
#include "adc.h"
#include "lcd.h"
#include "serial.h"

#if CONFIG_CPU == IMX31L
#include "serial-imx31.h"
#endif

#if CONFIG_CPU == SH7034

/* FIX: this doesn't work on iRiver or iPod yet */
/* iFP7xx has no remote */

#if !defined(HAVE_FMADC) /* Recorder FM/V2 has no remote control pin */ \
 && !defined(HAVE_MMC)   /* MMC takes serial port 1, so don't mess with it */

/* Received byte identifiers */
#define PLAY  0xC1
#define STOP  0xC2
#define PREV  0xC4
#define NEXT  0xC8
#define VOLUP 0xD0
#define VOLDN 0xE0

void serial_setup (void) 
{
    /* Set PB10 function to serial Rx */
    PBCR1 = (PBCR1 & 0xffcf) | 0x0020;
    
    SMR1 = 0x00;
    SCR1 = 0;
    BRR1 = (FREQ/(32*9600))-1;
    and_b(0, &SSR1); /* The status bits must be read before they are cleared,
                        so we do an AND operation */

    /* Let the hardware settle. The serial port needs to wait "at least
       the interval required to transmit or receive one bit" before it
       can be used. */
    sleep(1);

    SCR1 = 0x10; /* Enable the receiver, no interrupt */
}

/* This function returns the received remote control code only if it is
   received without errors before or after the reception.
   It therefore returns the received code on the second call after the
   code has been received. */
int remote_control_rx(void)
{
    static int last_valid_button = BUTTON_NONE;
    static int last_was_error = false;
    int btn;
    int ret = BUTTON_NONE;
    
    /* Errors? Just clear'em. The receiver stops if we don't */
    if(SSR1 & (SCI_ORER | SCI_FER | SCI_PER)) {
        and_b(~(SCI_ORER | SCI_FER | SCI_PER), &SSR1);
        last_valid_button = BUTTON_NONE;
        last_was_error = true;
        return BUTTON_NONE;
    }

    if(SSR1 & SCI_RDRF) {
        /* Read byte and clear the Rx Full bit */
        btn = RDR1;
        and_b(~SCI_RDRF, &SSR1);

        if(last_was_error)
        {
            last_valid_button = BUTTON_NONE;
            ret = BUTTON_NONE;
        }
        else
        {
            switch (btn)
            {
                case STOP:
                    last_valid_button = BUTTON_RC_STOP;
                    break;

                case PLAY:
                    last_valid_button = BUTTON_RC_PLAY;
                    break;

                case VOLUP:
                    last_valid_button = BUTTON_RC_VOL_UP;
                    break;

                case VOLDN:
                    last_valid_button = BUTTON_RC_VOL_DOWN;
                    break;

                case PREV:
                    last_valid_button = BUTTON_RC_LEFT;
                    break;

                case NEXT:
                    last_valid_button = BUTTON_RC_RIGHT;
                    break;

                default:
                    last_valid_button = BUTTON_NONE;
                    break;
            }
        }
    }
    else
    {
        /* This means that a valid remote control character was received
           the last time we were called, with no receiver errors either before
           or after. Then we can assume that there really is a remote control
           attached, and return the button code. */
        ret = last_valid_button;
        last_valid_button = BUTTON_NONE;
    }
    
    last_was_error = false;

    return ret;
}

#endif /* !HAVE_FMADC && !HAVE_MMC */
#elif defined(CPU_COLDFIRE) && defined(HAVE_SERIAL)

void serial_tx(const unsigned char *buf)
{
    while(*buf) {
        while(!(USR0 & 0x04))
        {
        };
        UTB0 = *buf++;
    }
}

void serial_setup (void) 
{
    UCR0 = 0x30; /* Reset transmitter */
    UCSR0 = 0xdd; /* Timer mode */

    UCR0 = 0x10;  /* Reset pointer */
    UMR0 = 0x13; /* No parity, 8 bits */
    UMR0 = 0x07; /* 1 stop bit */

    UCR0 = 0x04; /* Tx enable */
}

#elif (CONFIG_CPU == IMX31L)

void serial_setup(void)
{
#ifdef UART_INT /*enable UART Interrupts */
    UCR1_1 |= (EUARTUCR1_TRDYEN | EUaRTUCR1_RRDYEN | EUARTUCR1_TXMPTYEN);
    UCR4_1 |= (EUARTUCR4_TCEN);
#else /*disable UART Interrupts*/
    UCR1_1 &= ~(EUARTUCR1_TRDYEN | EUARTUCR1_RRDYEN | EUARTUCR1_TXMPTYEN);
    UCR4_1 &= ~(EUARTUCR4_TCEN);
#endif
    UCR1_1 |= EUARTUCR1_UARTEN;
    UCR2_1 |= (EUARTUCR2_TXEN  | EUARTUCR2_RXEN | EUARTUCR2_IRTS);

    /* Tx,Rx Interrupt Trigger levels, Disable for now*/
    /*UFCR1 |= (UFCR1_TXTL_32 | UFCR1_RXTL_32);*/
}

int tx_rdy(void)
{
    if((UTS1 & EUARTUTS_TXEMPTY))
        return 1;
    else
        return 0;
}

/*Not ready...After first Rx, UTS1 & UTS1_RXEMPTY 
  keeps returning true*/
int rx_rdy(void) 
{
    if(!(UTS1 & EUARTUTS_RXEMPTY))
        return 1;
    else
        return 0;
}

void tx_writec(char c)
{
    UTXD1=(int) c;
}

void dprintf(const char * str, ... )
{
    char dprintfbuff[256];
    unsigned char * ptr;
	
    va_list ap;
    va_start(ap, str);

    ptr = dprintfbuff;
    vsnprintf(ptr,sizeof(dprintfbuff),str,ap);
    va_end(ap);

    serial_tx(ptr);
}

void serial_tx(const unsigned char * buf)
{
    /*Tx*/
    for(;;) {
        if(tx_rdy()) {
            if(*buf == '\0')
                return;
            if(*buf == '\n')
                tx_writec('\r');
            tx_writec(*buf);
            buf++;
        }
    }
}

#else /* Other targets */
void serial_setup (void) 
{
    /* a dummy */
}
#endif
