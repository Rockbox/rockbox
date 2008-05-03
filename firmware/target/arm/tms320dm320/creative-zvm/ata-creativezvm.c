/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
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
#include "kernel.h"
#include "thread.h"
#include "system.h"
#include "power.h"
#include "panic.h"
#include "ata-target.h"
#include "dm320.h"

void sleep_ms(int ms)
{
    sleep(ms*HZ/1000);
}

void ide_power_enable(bool on)
{
/* Disabled until figured out what's wrong */
#if 0
    int old_level = disable_irq_save();
    if(on)
    {
        IO_GIO_BITSET0 = (1 << 14);
        ata_reset();
    }
    else
        IO_GIO_BITCLR0 = (1 << 14);
    restore_irq(old_level);
#else
    (void)on;
#endif
}

inline bool ide_powered()
{
#if 0
    return (IO_GIO_BITSET0 & (1 << 14));
#else
    return true;
#endif
}

void ata_reset(void)
{
    int old_level = disable_irq_save();
    if(!ide_powered())
    {
        ide_power_enable(true);
        sleep_ms(150);
    }
    else
    {
        IO_GIO_BITSET0 = (1 << 5);
        IO_GIO_BITCLR0 = (1 << 3);
        sleep_ms(1);
    }
    IO_GIO_BITCLR0 = (1 << 5);
    sleep_ms(10);
    IO_GIO_BITSET0 = (1 << 3);
    while(!(ATA_COMMAND & STATUS_RDY))
        sleep_ms(10);
    restore_irq(old_level);
}

void ata_enable(bool on)
{
    (void)on;
    return;
}

bool ata_is_coldstart(void)
{
    return true;
}

#if 0 /* Disabled as device crashes; probably due to SDRAM addresses aren't 32-bit aligned */
#define CS1_START       0x50000000
#define DEST_ADDR       (ATA_IOBASE-CS1_START)
static struct wakeup transfer_completion_signal;

void MTC0(void)
{
    IO_INTC_IRQ1 = 1 << IRQ_MTC0;
    wakeup_signal(&transfer_completion_signal);
}

void copy_read_sectors(unsigned char* buf, int wordcount)
{
    bool lasthalfword = false;
    unsigned short tmp;
    if(wordcount < 16)
    {
        _copy_read_sectors(buf, wordcount);
        return;
    }
    else if((unsigned long)buf % 32) /* Not 32-byte aligned */
    {
        unsigned char* bufend = buf + ((unsigned long)buf % 32);
        if( ((unsigned long)buf % 32) % 2 )
            lasthalfword = true;
        wordcount -= ((unsigned long)buf % 32) / 2;
        do
        {
            tmp = ATA_DATA;
            *buf++ = tmp >> 8;
            *buf++ = tmp & 0xff;
        } while (buf < bufend); /* tail loop is faster */
    }
    IO_SDRAM_SDDMASEL = 0x0830; /* 32-byte burst mode transfer */
    IO_EMIF_AHBADDH = ((unsigned)buf >> 16) & ~(1 << 15); /* Set variable address */
    IO_EMIF_AHBADDL = (unsigned)buf & 0xFFFF;
    IO_EMIF_DMAMTCSEL = 1; /* Select CS1 */
    IO_EMIF_MTCADDH = ( (1 << 15) | (DEST_ADDR >> 16) ); /* Set fixed address */
    IO_EMIF_MTCADDL = DEST_ADDR & 0xFFFF;
    IO_EMIF_DMASIZE = wordcount*2;
    IO_EMIF_DMACTL = 3; /* Select MTC->AHB and start transfer */
    //wakeup_wait(&transfer_completion_signal, TIMEOUT_BLOCK);
    while(IO_EMIF_DMACTL & 1)
        nop;
    if(lasthalfword)
    {
        *buf += wordcount * 2;
        tmp = ATA_DATA;
        *buf++ = tmp >> 8;
        *buf++ = tmp & 0xff;
    }
}
void copy_write_sectors(const unsigned char* buf, int wordcount)
{
    IO_EMIF_DMAMTCSEL = 1; /* Select CS1 */
    IO_SDRAM_SDDMASEL = 0x0820; /* Temporarily set to standard value */
    IO_EMIF_AHBADDH = ((int)buf >> 16) & ~(1 << 15); /* Set variable address */
    IO_EMIF_AHBADDL = (int)buf & 0xFFFF;
    IO_EMIF_MTCADDH = ( (1 << 15) | (DEST_ADDR >> 16) ); /* Set fixed address */
    IO_EMIF_MTCADDL = DEST_ADDR & 0xFFFF;
    IO_EMIF_DMASIZE = wordcount;
    IO_EMIF_DMACTL = 1; /* Select AHB->MTC and start transfer */
    wakeup_wait(&transfer_completion_signal, TIMEOUT_BLOCK);
}
#endif

void ata_device_init(void)
{
    IO_INTC_EINT1 |= INTR_EINT1_EXT2;   /* enable GIO2 interrupt */
#if 0
    IO_INTC_EINT1 |= 1 << IRQ_MTC0;     /* enable MTC interrupt */
    wakeup_init(&transfer_completion_signal);
#endif
    //TODO: mimic OF inits...
    return;
}

void GIO2(void)
{
#ifdef DEBUG
    logf("GIO2 interrupt...");
#endif
    IO_INTC_IRQ1 = INTR_IRQ1_EXT2; /* Mask GIO2 interrupt */
    return;
}
