/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Jens Arnold
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

#include "system.h"
#include "kernel.h"
#include "isp1362.h"

#define HC_DATA (*((volatile unsigned short*)0xc0000000))
#define HC_CMD  (*((volatile unsigned short*)0xc0000002))
#define DC_DATA (*((volatile unsigned short*)0xc0000004))
#define DC_CMD  (*((volatile unsigned short*)0xc0000006))

/* host controller access */

unsigned isp1362_read_hc_reg16(unsigned reg)
{
    HC_CMD = reg;

    asm ("nop\n nop\n nop\n nop\n");
    asm ("nop\n nop\n nop\n nop\n");
    asm ("nop\n nop\n nop\n nop\n");

    return HC_DATA;
}

unsigned isp1362_read_hc_reg32(unsigned reg)
{
    unsigned data;

    HC_CMD = reg;

    asm ("nop\n nop\n nop\n nop\n");
    asm ("nop\n nop\n nop\n nop\n");
    asm ("nop\n nop\n nop\n nop\n");

    data = HC_DATA;
    data |= HC_DATA << 16;
    return data;
}

void isp1362_write_hc_reg16(unsigned reg, unsigned data)
{
    HC_CMD = reg | 0x80;

    asm ("nop\n nop\n nop\n");

    HC_DATA = data;
}

void isp1362_write_hc_reg32(unsigned reg, unsigned data)
{
    HC_CMD = reg | 0x80;

    asm ("nop\n nop\n nop\n");

    HC_DATA = data;
    HC_DATA = data >> 16;
}

/* device controller access */

unsigned isp1362_read_dc_reg16(unsigned reg)
{
    DC_CMD = reg;

    asm ("nop\n nop\n nop\n nop\n");
    asm ("nop\n nop\n nop\n nop\n");
    asm ("nop\n nop\n nop\n nop\n");

    return DC_DATA;
}

unsigned isp1362_read_dc_reg32(unsigned reg)
{
    unsigned data;

    DC_CMD = reg;

    asm ("nop\n nop\n nop\n nop\n");
    asm ("nop\n nop\n nop\n nop\n");
    asm ("nop\n nop\n nop\n nop\n");

    data = DC_DATA;
    data |= DC_DATA << 16;
    return data;
}

void isp1362_write_dc_reg16(unsigned reg, unsigned data)
{
    DC_CMD = reg;

    asm ("nop\n nop\n nop\n");

    DC_DATA = data;
}

void isp1362_write_dc_reg32(unsigned reg, unsigned data)
{
    DC_CMD = reg;

    asm ("nop\n nop\n nop\n");

    DC_DATA = data;
    DC_DATA = data >> 16;
}

static void isp1362_suspend(void)
{
    unsigned data;

    data = isp1362_read_hc_reg16(ISP1362_OTG_CONTROL);
    data &= ~0x0001;  /* DRV_VBUS = 0 */
    isp1362_write_hc_reg16(ISP1362_OTG_CONTROL, data);

    /* prepare the DC */
    data = isp1362_read_dc_reg16(ISP1362_DC_HARDWARE_CONFIG_R);
    data &= ~0x1008;  /* CLKRUN = WKUPCS = 0. Wakeup is still possible via /D_WAKEUP */
    isp1362_write_dc_reg16(ISP1362_DC_HARDWARE_CONFIG_W, data);

    /* send the DC to sleep */
    data = isp1362_read_dc_reg16(ISP1362_DC_MODE_R);
    data |=  0x20;  /* GOSUSP = 1 */
    isp1362_write_dc_reg16(ISP1362_DC_MODE_W, data);
    data &= ~0x20;  /* GOSUSP = 0 */
    isp1362_write_dc_reg16(ISP1362_DC_MODE_W, data);

    /* prepare the HC */
    data = isp1362_read_hc_reg16(ISP1362_HC_HARDWARE_CONFIG);
    data &= ~0x0800;  /* SuspendClkNotStop = 0 */
    data |=  0x4001;  /* GlobalPowerDown = InterruptPinEnable = 1 */
    isp1362_write_hc_reg16(ISP1362_HC_HARDWARE_CONFIG, data);

    /* TODO: OTG wake-up cfg */
    /* TODO: Interrupt setup */
    
    /* set the HC to operational */
    isp1362_write_hc_reg32(ISP1362_HC_CONTROL, 0x0680);
                           /* RWE = RWC = 1, HCFS = 0b10 (USBOperational) */
    /* ..then send it to sleep */
    isp1362_write_hc_reg32(ISP1362_HC_CONTROL, 0x06c0);
                           /* RWE = RWC = 1, HCFS = 0b11 (USBSuspend) */
}

/* init */

void isp1362_init(void)
{
    and_l(~0x00200080, &GPIO1_OUT);     /* disable 5V USB host power and ??? */
    or_l(  0x00200080, &GPIO1_ENABLE);
    or_l(  0x00200080, &GPIO1_FUNCTION);

    or_l(  0x20600000, &GPIO_OUT);      /* ID = D_SUSPEND = /OTGMODE = 1 */
    and_l(~0x04000000, &GPIO_OUT);      /* ?R26? = 0 */
    or_l(  0x24600000, &GPIO_ENABLE);   /* ID, ?R26?, D_SUSPEND, /OTGMODE outputs */
    and_l(~0x000000a8, &GPIO_ENABLE);   /* /INT2, /INT1, /RESET inputs */
    or_l(  0x246000a8, &GPIO_FUNCTION); /* GPIO for these pins */

    sleep(HZ/5);

    isp1362_suspend();
}
