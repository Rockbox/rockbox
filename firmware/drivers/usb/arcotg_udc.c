/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Barry Wardell
 *
 * Based on code from the Linux Target Image Builder from Freescale
 * available at http://www.bitshrine.org/ and
 * http://www.bitshrine.org/gpp/linux-2.6.16-mx31-usb-2.patch
 * Adapted for Rockbox in January 2007
 * Original file: drivers/usb/gadget/arcotg_udc.c
 *
 * USB Device Controller Driver
 * Driver for ARC OTG USB module in the i.MX31 platform, etc.
 *
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Based on mpc-udc.h
 * Author: Li Yang (leoli@freescale.com)
 *         Jiang Bo (Tanya.jiang@freescale.com)
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "arcotg_udc.h"
#include "logf.h"

static int timeout;

/* @qh_addr is the aligned virt addr of ep QH addr
 * it is used to set endpointlistaddr Reg */
/* was static int dr_controller_setup(void *qh_addr) */
int dr_controller_setup(void)
{
#if 0
    struct arc_usb_config *config;

    config = udc_controller->config;

    /* before here, make sure usb_slave_regs has been initialized */
    if (!qh_addr)
        return -EINVAL;
#endif

    /* Stop and reset the usb controller */
    UDC_USBCMD &= ~USB_CMD_RUN_STOP;

    UDC_USBCMD |= USB_CMD_CTRL_RESET;

    /* Wait for reset to complete */
    timeout = 10000000;
    while ((UDC_USBCMD & USB_CMD_CTRL_RESET) &&
           --timeout) {
        continue;
    }
    if (timeout == 0) {
        logf("%s: TIMEOUT", __FUNCTION__);
        return -ETIMEDOUT;
    }

    /* Set the controller as device mode and disable setup lockout */
    UDC_USBMODE |= (USB_MODE_CTRL_MODE_DEVICE | USB_MODE_SETUP_LOCK_OFF);

    /* Clear the setup status */
    UDC_USBSTS = 0;
#if 0
    UDC_ENDPOINTLISTADDR = (unsigned int)qh_addr & USB_EP_LIST_ADDRESS_MASK;
    
    VDBG("qh_addr=0x%x epla_reg=0x%8x", qh_addr, UOG_ASYNCLISTADDR);
#endif
    UDC_PORTSC1 = (UDC_PORTSC1 & ~PORTSCX_PHY_TYPE_SEL) | PORTSCX_PTS_UTMI;
#if 0
    if (config->set_vbus_power)
        config->set_vbus_power(0);
#endif

    return 0;
}

/* just Enable DR irq reg and Set Dr controller Run */
/* was static void dr_controller_run(struct arcotg_udc *udc) */
void dr_controller_run(void)
{
    /*Enable DR irq reg */
    UDC_USBINTR = USB_INTR_INT_EN | USB_INTR_ERR_INT_EN |
          USB_INTR_PTC_DETECT_EN | USB_INTR_RESET_EN |
          USB_INTR_DEVICE_SUSPEND | USB_INTR_SYS_ERR_EN;
#if 0
    /* Clear stopped bit */
    udc->stopped = 0;
#endif
    /* Set the controller as device mode */
    UDC_USBMODE |= USB_MODE_CTRL_MODE_DEVICE;

    /* Set controller to Run */
    UDC_USBCMD |= USB_CMD_RUN_STOP;

    return;
}

/* just Enable DR irq reg and Set Dr controller Run */
/* was static void dr_controller_stop(struct arcotg_udc *udc) */
void dr_controller_stop(void)
{
#if 0
   /* if we're in OTG mode, and the Host is currently using the port,
    * stop now and don't rip the controller out from under the
    * ehci driver
    */
   if (udc->gadget.is_otg) {
       if (!(UDC_OTGSC & OTGSC_STS_USB_ID)) {
           logf("Leaving early");
           return;
       }
   }
#endif

    /* disable all INTR */
    UDC_USBINTR = 0;
#if 0
    /* Set stopped bit */
    udc->stopped = 1;
#endif
    /* Set controller to Stop */
    UDC_USBCMD &= ~USB_CMD_RUN_STOP;
}
