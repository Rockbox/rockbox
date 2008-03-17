/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Jens Arnold
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"

#include "adc.h"
#include "i2c-coldfire.h"
#include "system.h"

#define ADC_I2C_ADDR 0xa0

/* The M3 ADC is hooked exclusively to the secondary I²C bus, and requires
 * very slow transfers (I²C clock <= 16kHz). So we start one 4-byte read
 * transfer each tick, and handle it via an ISR. At 11MHz, one transfer
 * takes too long to be started every tick, but it seems we have to live 
 * with that. */

static unsigned char adc_data[NUM_ADC_CHANNELS] IBSS_ATTR;
static volatile bool data_ready = false;


static void adc_tick(void)
{
    if ((MBSR2 & IBB) == 0)   /* Bus is free */
    {
        MBCR2 |= (MSTA|TXAK|MTX); /* Generate START and prepare for write */
        MBDR2 = ADC_I2C_ADDR|1;   /* Send address */
    }
}

void IIC2(void) __attribute__((interrupt_handler));
void IIC2(void)
{
    static int bytenum = 0;
    
    MBSR2 &= ~IFF;            /* Clear interrupt flag */

    if (MBSR2 & IAL)          /* Arbitration lost - shouldn't happen */
    {                         /* normally, but CPU freq change might induce it */
        MBSR2 &= ~IAL;        /* Clear flag */
        MBCR2 &= ~MSTA;       /* STOP */
    }
    else if (MBCR2 & MTX)     /* Address phase */
    {
        if (MBSR2 & RXAK)     /* No ACK received */
        {
            MBCR2 &= ~MSTA;   /* STOP */
            return;
        }
        MBCR2 &= ~(MTX|TXAK); /* Switch to RX mode, enable TX ack generation */
        MBDR2;                /* Dummy read */
        bytenum = 0;
    }
    else
    {
        switch (bytenum)
        {
          case 2:
            MBCR2 |= TXAK;    /* Don't ACK the last byte */
            break;
            
          case 3:
            MBCR2 &= ~MSTA;   /* STOP before reading the last value */
            data_ready = true; /* Simplification - the last byte is a dummy. */
            break;
        }
        adc_data[bytenum++] = MBDR2;
    }

}

unsigned short adc_read(int channel)
{
    return adc_data[channel];
}

void adc_init(void)
{
    MFDR2 = 0x1f;             /* I²C clock = SYSCLK / 3840 */
    MBCR2 = IEN;              /* Enable interface */
    MBSR2 = 0;                /* Clear flags */
    MBCR2 = (IEN|IIEN);       /* Enable interrupts */

    and_l(~0x0f000000, &INTPRI8);
    or_l(  0x04000000, &INTPRI8); /* INT62 - Priority 4 */

    tick_add_task(adc_tick);
    
    while (!data_ready)
        sleep(1);             /* Ensure valid readings when adc_init returns */
}
