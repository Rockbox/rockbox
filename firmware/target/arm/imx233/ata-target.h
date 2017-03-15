/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 Amaury Pouly
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
#ifndef ATA_TARGET_H
#define ATA_TARGET_H

#include "config.h"

#ifdef HAVE_ATA_DMA
/* FIXME does this chips does MWDMA ? */
#define ATA_MAX_MWDMA   2
#define ATA_MAX_UDMA    4
#endif

uint8_t imx233_ata_inb(unsigned reg);
uint16_t imx233_ata_inw(unsigned reg);
void imx233_ata_outb(unsigned reg, uint8_t v);
void imx233_ata_outw(unsigned reg, uint16_t v);

#define IMX233_ATA_REG(cs,addr)     ((cs) << 3 | (addr))
#define IMX233_ATA_REG_CS(reg)      ((reg) >> 3)
#define IMX233_ATA_REG_ADDR(reg)    ((reg) & 0x7)
/* use register address (see ATA spec) */
#define ATA_DATA        IMX233_ATA_REG(0, 0)
#define ATA_ERROR       IMX233_ATA_REG(0, 1)
#define ATA_NSECTOR     IMX233_ATA_REG(0, 2)
#define ATA_SECTOR      IMX233_ATA_REG(0, 3)
#define ATA_LCYL        IMX233_ATA_REG(0, 4)
#define ATA_HCYL        IMX233_ATA_REG(0, 5)
#define ATA_SELECT      IMX233_ATA_REG(0, 6)
#define ATA_COMMAND     IMX233_ATA_REG(0, 7)
#define ATA_CONTROL     IMX233_ATA_REG(1, 6)

/* keep consistent with definition of IMX233_ATA_REG */
#define ATA_OUT8(reg, data) imx233_ata_outb(reg, data)
#define ATA_OUT16(reg, data) imx233_ata_outw(reg, data)
#define ATA_IN8(reg) imx233_ata_inb(reg)
#define ATA_IN16(reg) imx233_ata_inw(reg)

#define ATA_SET_PIO_TIMING

#define ATA_TARGET_POLLING

void ata_set_pio_timings(int mode);
void ata_reset(void);
void ata_enable(bool on);
bool ata_is_coldstart(void);
#ifdef HAVE_ATA_DMA
void ata_dma_set_mode(unsigned char mode);
bool ata_dma_setup(void *addr, unsigned long bytes, bool write);
bool ata_dma_finish(void);
#endif
int ata_wait_for_bsy(void);
int ata_wait_for_rdy(void);
void ata_device_init(void);

#endif /* ATA_TARGET_H */
