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

/*
 * USB-Designware driver API
 */

const struct usb_dw_config usb_dw_config = {
    .phytype = DWC_PHYTYPE_UTMI_16,

    /* Available FIFO memory: 3576 words
     * Number of endpoints: 9
     * Max packet size: 512 bytes
     */
    .rx_fifosz   = 816, /* shared RxFIFO */
    .nptx_fifosz = 32,  /* only used for EP0 IN */
    .ptx_fifosz  = 384, /* room for 7 IN EPs */

#ifndef USB_DW_ARCH_SLAVE
    .ahb_burst_len = HBSTLEN_INCR16,
    /* Disable Rx FIFO thresholding. It appears to cause problems,
     * apparently a known issue -- Synopsys recommends disabling it
     * because it can cause issues during certain error conditions.
     */
    .ahb_threshold = 0,
#else
    .disable_double_buffering = false,
#endif
};

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
}

/*
 * Rockbox API
 */

#ifdef USB_STATUS_BY_EVENT
static volatile int usb_status = USB_EXTRACTED;
static void usb_detect_interrupt(void);
#endif

static int __usb_detect(void)
{
    /* XXX: Do we need an active level define for this? */
    if(gpio_get_level(GPIO_USB_DETECT))
        return USB_INSERTED;
    else
        return USB_EXTRACTED;
}

void usb_enable(bool on)
{
    if(on)
        usb_core_init();
    else
        usb_core_exit();
}

void usb_init_device(void)
{
    /* Disable drvvbus pin -- it is only used when acting as a host,
     * which Rockbox does not support */
    gpio_set_function(GPIO_USB_DRVVBUS, GPIOF_OUTPUT(0));

    /* Power up the core clocks to allow writing
       to some registers needed to power it down */
    usb_dw_target_disable_irq();
    usb_dw_target_enable_clocks();
    usb_drv_exit();

#ifdef USB_STATUS_BY_EVENT
    /* Setup USB detect pin IRQ */
    usb_status = __usb_detect();
    system_set_irq_handler(GPIO_TO_IRQ(GPIO_USB_DETECT), usb_detect_interrupt);
    gpio_set_function(GPIO_USB_DETECT, GPIOF_IRQ_EDGE(1));
    gpio_flip_edge_irq(GPIO_USB_DETECT);
    gpio_enable_irq(GPIO_USB_DETECT);
#endif
}

#ifndef USB_STATUS_BY_EVENT
int usb_detect(void)
{
    return __usb_detect();
}
#else
int usb_detect(void)
{
    return usb_status;
}

static void usb_detect_interrupt(void)
{
    /* Update status and flip the IRQ trigger edge */
    usb_status = __usb_detect();
    gpio_flip_edge_irq(GPIO_USB_DETECT);

    /* Notify Rockbox of event */
    usb_status_event(usb_status);
}
#endif
