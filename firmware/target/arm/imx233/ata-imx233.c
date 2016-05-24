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
#ifndef ATA_IMX233_H
#define ATA_IMX233_H

#include "config.h"
#include "system.h"
#include "pinctrl-imx233.h"
#include "clkctrl-imx233.h"
#include "ata-target.h"
#include "ata-defines.h"

#include "regs/gpmi.h"

struct pio_timing_t
{
    /** all values are in ns */
    int addr_setup; /* "Address valid to DIOR-/DIOW-setup" */
    int data_hold; /* "DIOR-/DIOW-recovery time" */
    int data_setup; /* "DIOR-/DIOW-" */
};

static struct pio_timing_t pio_timing[] =
{
    /* FIXME: OF uses 290, 290, 290, 80, 70 for data_setup */
    {70, 100, 165},
    {50, 100, 125},
    {30, 100, 100},
    {30,  70,  80},
    {25,  25,  70},
};

static void imx233_ata_wait_ready(void)
{
    while(BF_RD(GPMI_CTRL0, RUN))
        yield();
}

static uint16_t imx233_ata_read_reg(unsigned reg)
{
    /* wait ready */
    imx233_ata_wait_ready();

    /* setup command */
    BF_WR_ALL(GPMI_CTRL0, RUN(1), COMMAND_MODE_V(READ), WORD_LENGTH_V(16_BIT),
        CS(IMX233_ATA_REG_CS(reg)), ADDRESS(IMX233_ATA_REG_ADDR(reg)), XFER_COUNT(1));

    /* wait for completion */
    while(BF_RD(GPMI_STAT, FIFO_EMPTY));

    /* get data */
    return HW_GPMI_DATA & 0xffff;
}

static void imx233_ata_write_reg(unsigned reg, uint16_t data)
{
    /* wait ready */
    imx233_ata_wait_ready();

    /* setup command */
    BF_WR_ALL(GPMI_CTRL0, RUN(1), COMMAND_MODE_V(WRITE), WORD_LENGTH_V(16_BIT),
        CS(IMX233_ATA_REG_CS(reg)), ADDRESS(IMX233_ATA_REG_ADDR(reg)), XFER_COUNT(1));

    /* send data */
    HW_GPMI_DATA = data;
}

uint8_t imx233_ata_inb(unsigned reg)
{
    return imx233_ata_read_reg(reg) & 0xff;
}

uint16_t imx233_ata_inw(unsigned reg)
{
    return imx233_ata_read_reg(reg);
}

void imx233_ata_outb(unsigned reg, uint8_t v)
{
    imx233_ata_write_reg(reg, v);
}

void imx233_ata_outw(unsigned reg, uint16_t v)
{
    imx233_ata_write_reg(reg, v);
}

void ata_set_pio_timings(int mode)
{
    /* load timing */
    struct pio_timing_t t = pio_timing[mode > 3 ? 3 : mode];
    /* adjust to the clock */
    unsigned clock_freq = 80 * 1000;
#define adjust_to_clock(val) \
    val = (val * clock_freq) / 1000 / 1000

    adjust_to_clock(t.addr_setup);
    adjust_to_clock(t.data_hold);
    adjust_to_clock(t.data_setup);
    /* write */
    imx233_ata_wait_ready();
    BF_WR_ALL(GPMI_TIMING0, ADDRESS_SETUP(t.addr_setup), DATA_HOLD(t.data_hold),
        DATA_SETUP(t.data_setup));
}

void ata_reset(void)
{
    /* reset device */
    BF_WR(GPMI_CTRL1, DEV_RESET_V(ENABLED));
    sleep(HZ / 10);
    BF_WR(GPMI_CTRL1, DEV_RESET_V(DISABLED));
}

void ata_enable(bool on)
{
}

bool ata_is_coldstart(void)
{
    return false;
}

#ifdef HAVE_ATA_DMA
void ata_dma_set_mode(unsigned char mode);
bool ata_dma_setup(void *addr, unsigned long bytes, bool write);
bool ata_dma_finish(void);
#endif

static int ata_wait_status(unsigned status, unsigned mask, int timeout)
{
    long end_tick = current_tick + timeout;

    while(TIME_BEFORE(current_tick, end_tick))
    {
        if((ATA_IN8(ATA_STATUS) & mask) == status)
            return 1;
        sleep(0);
    }

    return 0;
}

int ata_wait_for_bsy(void)
{
    /* BSY = 0 */
    return ata_wait_status(0, STATUS_BSY, HZ);
}

int ata_wait_for_rdy(void)
{
    /* RDY = 1 && BSY = 0 */
    return ata_wait_status(STATUS_RDY, STATUS_RDY | STATUS_BSY, HZ);
}

void ata_device_init(void)
{
    /* reset block */
    imx233_reset_block(&HW_GPMI_CTRL0);
    /* setup pins */
    imx233_pinctrl_setup_vpin(VPIN_GPMI_D0, "ata d0", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_GPMI_D1, "ata d1", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_GPMI_D2, "ata d2", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_GPMI_D3, "ata d3", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_GPMI_D4, "ata d4", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_GPMI_D5, "ata d5", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_GPMI_D6, "ata d6", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_GPMI_D7, "ata d7", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_GPMI_D8, "ata d8", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_GPMI_D9, "ata d9", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_GPMI_D10, "ata d10", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_GPMI_D11, "ata d11", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_GPMI_D12, "ata d12", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_GPMI_D13, "ata d13", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_GPMI_D14, "ata d14", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_GPMI_D15, "ata d15", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_GPMI_D15, "ata d15", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_GPMI_CE0n, "ata cs0", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_GPMI_CE1n, "ata cs1", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_GPMI_A0, "ata a0", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_GPMI_A1, "ata a1", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_GPMI_A2, "ata a2", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_GPMI_IRQ, "ata irq", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_GPMI_RDY, "ata rdy", PINCTRL_DRIVE_4mA, false);
#ifdef HAVE_ATA_DMA
    imx233_pinctrl_setup_vpin(VPIN_GPMI_RDY2, "ata dmack", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_GPMI_RDY3, "ata dmarq", PINCTRL_DRIVE_4mA, false);
#endif
    imx233_pinctrl_setup_vpin(VPIN_GPMI_RDn, "ata rd", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_GPMI_WRn, "ata wr", PINCTRL_DRIVE_4mA, false);
    /* setup ata mode */
    BF_WR(GPMI_CTRL1, GPMI_MODE_V(ATA));
    /* reset device */
    ata_reset();
    ata_enable(true);

    /* setup mode 0 for all until identification */
    ata_set_pio_timings(0);

#ifdef HAVE_ATA_DMA
    ata_set_mdma_timings(0);
    ata_set_udma_timings(0);
#endif
}

#endif /* ATA_IMX233_H */


 
