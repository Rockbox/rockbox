/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: ata-meg-fx.c 27935 2010-08-28 23:12:11Z funman $
 *
 * Copyright (C) 2011 by Michael Sparmann
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
#include "cpu.h"
#include "kernel.h"
#include "thread.h"
#include "system.h"
#include "power.h"
#include "panic.h"
#include "pmu-target.h"
#include "ata.h"
#include "ata-target.h"
#include "s5l8702.h"


static struct wakeup ata_wakeup;

#ifdef HAVE_ATA_DMA
static uint32_t ata_dma_flags;
#endif


void ata_reset(void)
{
    ATA_SWRST = 1;
    sleep(HZ / 100);
    ATA_SWRST = 0;
    sleep(HZ / 10);
}

void ata_enable(bool on)
{
    if (on)
    {
        PWRCON(0) &= ~(1 << 5);
        ATA_CFG = 0x41;
        sleep(HZ / 100);
        ATA_CFG = 0x40;
        sleep(HZ / 20);
        ata_reset();
        ATA_CCONTROL = 1;
        sleep(HZ / 5);
        ATA_PIO_TIME = 0x191f7;
        *ATA_HCYL = 0;
        while (!(ATA_PIO_READY & 2)) yield();
    }
    else
    {
        ATA_CCONTROL = 0;
        while (!(ATA_CCONTROL & 2)) yield();
        PWRCON(1) |= 1 << 5;
    }
}

bool ata_is_coldstart(void)
{
    return false;
}

void ata_device_init(void)
{
    VIC0INTENABLE = 1 << IRQ_ATA;
}

uint16_t ata_read_cbr(uint32_t volatile* reg)
{
    while (!(ATA_PIO_READY & 2));
    volatile uint32_t __attribute__((unused)) dummy = *reg;
    while (!(ATA_PIO_READY & 1));
    return ATA_PIO_RDATA;
}

void ata_write_cbr(uint32_t volatile* reg, uint16_t data)
{
    while (!(ATA_PIO_READY & 2));
    *reg = data;
}

void ata_set_pio_timings(int mode)
{
    if (mode >= 4) ATA_PIO_TIME = 0x7083;
    if (mode >= 3) ATA_PIO_TIME = 0x2072;
    else ATA_PIO_TIME = 0x11f3;
}

#ifdef HAVE_ATA_DMA
static void ata_set_mdma_timings(unsigned int mode)
{
    if (mode >= 2) ATA_MDMA_TIME = 0x5072;
    if (mode >= 1) ATA_MDMA_TIME = 0x7083;
    else ATA_MDMA_TIME = 0x1c175;
}

static void ata_set_udma_timings(unsigned int mode)
{
    if (mode >= 4) ATA_UDMA_TIME = 0x2010a52;
    if (mode >= 3) ATA_UDMA_TIME = 0x2020a52;
    if (mode >= 2) ATA_UDMA_TIME = 0x3030a52;
    if (mode >= 1) ATA_UDMA_TIME = 0x3050a52;
    else ATA_UDMA_TIME = 0x5071152;
}

void ata_dma_set_mode(unsigned char mode)
{
    unsigned int modeidx = mode & 0x07;
    unsigned int dmamode = mode & 0xf8;

    if (dmamode == 0x40 && modeidx <= ATA_MAX_UDMA)
    {
        /* Using Ultra DMA */
        ata_set_udma_timings(dmamode);
        ata_dma_flags = 0x60c;
    }
    else if (dmamode == 0x20 && modeidx <= ATA_MAX_MWDMA)
    {
        /* Using Multiword DMA */
        ata_set_mdma_timings(dmamode);
        ata_dma_flags = 0x408;
    }
    else
    {
        /* Don't understand this - force PIO. */
        ata_dma_flags = 0;
    }
}

bool ata_dma_setup(void *addr, unsigned long bytes, bool write)
{
    if ((((int)addr) & 0xf) || (((int)bytes) & 0xf) || !ata_dma_flags)
        return false;

    if (write) clean_dcache();
    else invalidate_dcache();
    ATA_CCOMMAND = 2;

    if (write)
    {
        ATA_SBUF_START = addr;
        ATA_SBUF_SIZE = bytes;
        ATA_CFG |= 0x10;
    }
    else
    {
        ATA_TBUF_START = addr;
        ATA_TBUF_SIZE = bytes;
        ATA_CFG &= ~0x10;
    }
    ATA_XFR_NUM = bytes - 1;

    return true;
}

bool ata_dma_finish(void)
{
    ATA_CFG |= ata_dma_flags;
    ATA_CFG &= ~0x180;
    wakeup_wait(&ata_wakeup, TIMEOUT_NOBLOCK);
    ATA_IRQ = 0x1f;
    ATA_IRQ_MASK = 1;
    ATA_CCOMMAND = 1;
    if (wakeup_wait(&ata_wakeup, HZ / 2) != OBJ_WAIT_SUCCEEDED)
    {
        ATA_CCOMMAND = 2;
        ATA_CFG &= ~0x100c;
        return false;
    }
    ATA_CCOMMAND = 2;
    ATA_CFG &= ~0x100c;
    return true;
}
#endif /* HAVE_ATA_DMA */

void INT_ATA(void)
{
    uint32_t ata_irq = ATA_IRQ;
    ATA_IRQ = ata_irq;
    if (ata_irq & ATA_IRQ_MASK) wakeup_signal(&ata_wakeup);
    ATA_IRQ_MASK = 0;
}
