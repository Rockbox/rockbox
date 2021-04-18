/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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
#include "usb.h"
#include "usb_core.h"
#include "usb_drv.h"
#include "usb-designware.h"
#include "irq-x1000.h"
#include "gpio-x1000.h"
#include "x1000/cpm.h"

#include "lcd.h"

const struct usb_dw_config usb_dw_config = {
    .phytype = DWC_PHYTYPE_UTMI_16,
    /* fifo sizes from Fiio's kernel */
    .rx_fifosz = 1024,
    .nptx_fifosz = 1024,
    .ptx_fifosz = 1024,
    .ahb_burst_len = HBSTLEN_INCR16,
    .ahb_threshold = 16,
};

void usb_enable(bool on)
{
    if(on)
        usb_core_init();
    else
        usb_core_exit();
}

void usb_init_device(void)
{
    /* Disable drvvbus pin */
    gpio_config(GPIO_B, 1 << 25, GPIO_OUTPUT(0));

    /* Power up the core clocks to allow writing
       to some registers needed to power it down */
    usb_dw_target_disable_irq();
    usb_dw_target_enable_clocks();

    usb_drv_exit();
}

static volatile int usb_status = USB_EXTRACTED;

int usb_detect(void)
{
    /* CPM_INTR (CPM_BASE + 0xb0)
     * - bit1 = VBUS_INTR (B device inserted)
     * - bit0 = ADEV_INTR (A device inserted)
     *
     * GPIOB07 [usb_id]   0 when plugged, 1 when unplugged
     * GPIOB11 [detect]   1 when plugged, 0 when unplugged
     * GPIOB25 [drvvbus]  (Charge pump enable, as device0 function of pin)
     */
    if(REG_GPIO_PIN(GPIO_B) & (1 << 11))
        return USB_INSERTED;
    else
        return USB_EXTRACTED;
}

void usb_charging_maxcurrent_change(int maxcurrent)
{
    /* TODO: this is target's job to talk with PMIC */
    (void)maxcurrent;
}

/* USB PHY init from Linux kernel code:
 * - arch/mips/xburst/soc-x1000/common/cpm_usb.c
 *   Copyright (C) 2005-2017 Ingenic Semiconductor
 */
void usb_dw_target_enable_clocks(void)
{
    /* Enable CPM clock */
    jz_writef(CPM_CLKGR, OTG(0));
#if X1000_EXCLK_FREQ == 24000000
    jz_writef(CPM_USBCDR, CLKSRC_V(EXCLK), CE(1), CLKDIV(0), PHY_GATE(0));
#else
# error "please add USB clock settings for 26 MHz EXCLK"
#endif
    while(jz_readf(CPM_USBCDR, BUSY));
    jz_writef(CPM_USBCDR, CE(0));

    /* PHY soft reset */
    jz_writef(CPM_SRBC, OTG_SR(1));
    udelay(10);
    jz_writef(CPM_SRBC, OTG_SR(0));

    /* Ungate PHY clock */
    jz_writef(CPM_OPCR, GATE_USBPHY_CLK(0));

    /* Exit suspend state */
    jz_writef(CPM_OPCR, SPENDN0(1));
    udelay(45);

    /* Program core configuration */
    jz_overwritef(CPM_USBVBFIL,
                  IDDIGFIL(0),
                  VBFIL(0));
    jz_overwritef(CPM_USBRDT,
                  HB_MASK(0),
                  VBFIL_LD_EN(1),
                  IDDIG_EN(0),
                  RDT(0x96));
    jz_overwritef(CPM_USBPCR,
                  OTG_DISABLE(1),
                  COMMONONN(1),
                  VBUSVLDEXT(1),
                  VBUSVLDEXTSEL(1),
                  SQRXTUNE(7),
                  TXPREEMPHTUNE(1),
                  TXHSXVTUNE(1),
                  TXVREFTUNE(7));
    jz_overwritef(CPM_USBPCR1,
                  BVLD_REG(1),
                  REFCLK_SEL_V(CLKCORE),
                  REFCLK_DIV_V(24MHZ), /* applies for 26 MHz EXCLK too */
                  WORD_IF_V(16BIT));

    /* Power on reset */
    jz_writef(CPM_USBPCR, POR(1));
    mdelay(1);
    jz_writef(CPM_USBPCR, POR(0));
    mdelay(1);
}

void usb_dw_target_disable_clocks(void)
{
    /* Suspend and power down PHY, then gate its clock */
    jz_writef(CPM_OPCR, SPENDN0(0));
    udelay(5);
    jz_writef(CPM_USBPCR, OTG_DISABLE(1), SIDDQ(1));
    jz_writef(CPM_OPCR, GATE_USBPHY_CLK(1));

    /* Disable CPM clock */
    jz_writef(CPM_USBCDR, CE(1), STOP(1), PHY_GATE(1));
    while(jz_readf(CPM_USBCDR, BUSY));
    jz_writef(CPM_USBCDR, CE(0));
    jz_writef(CPM_CLKGR, OTG(1));
}

void usb_dw_target_enable_irq(void)
{
    system_enable_irq(IRQ_OTG);
}

void usb_dw_target_disable_irq(void)
{
    system_disable_irq(IRQ_OTG);
}

void usb_dw_target_clear_irq(void)
{
    /* Handled automatically in system-x1000.c */
}
