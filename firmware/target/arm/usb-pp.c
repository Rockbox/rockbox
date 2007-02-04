/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * iPod driver based on code from the ipodlinux project - http://ipodlinux.org
 * Adapted for Rockbox in January 2006
 * Original file: podzilla/usb.c
 * Copyright (C) 2005 Adam Johnston
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
#include "debug.h"
#include "ata.h"
#include "fat.h"
#include "disk.h"
#include "panic.h"
#include "lcd.h"
#include "adc.h"
#include "usb.h"
#include "button.h"
#include "sprintf.h"
#include "string.h"
#include "hwcompat.h"

#include "usb-target.h"
#include "mx31.h"

void usb_init_device(void)
{
    int r0;
    outl(inl(0x70000084) | 0x200, 0x70000084);

    outl(inl(0x7000002C) | 0x3000000, 0x7000002C);
    DEV_EN |= DEV_USB;

    DEV_RS |= DEV_USB; /* reset usb start */
    DEV_RS &=~DEV_USB;/* reset usb end */

    DEV_INIT |= INIT_USB;
    while ((inl(0x70000028) & 0x80) == 0);

    UOG_PORTSC1 |= PORTSCX_PORT_RESET;
    while ((UOG_PORTSC1 & PORTSCX_PORT_RESET) != 0);

    UOG_OTGSC |= 0x5F000000;
    if( (UOG_OTGSC & 0x100) == 0) {
        UOG_USBMODE &=~ USB_MODE_CTRL_MODE_HOST;
        UOG_USBMODE |= USB_MODE_CTRL_MODE_DEVICE;
        outl(inl(0x70000028) | 0x4000, 0x70000028);
        outl(inl(0x70000028) | 0x2, 0x70000028);
    } else {
        UOG_USBMODE |= USB_MODE_CTRL_MODE_DEVICE;
        outl(inl(0x70000028) &~0x4000, 0x70000028);
        outl(inl(0x70000028) | 0x2, 0x70000028);
    }
    
    
    UOG_USBCMD |= USB_CMD_CTRL_RESET;
    while((UOG_USBCMD & USB_CMD_CTRL_RESET) != 0);
    
    r0 = UOG_PORTSC1;

    /* Note from IPL source (referring to next 5 lines of code: 
       THIS NEEDS TO BE CHANGED ONCE THERE IS KERNEL USB */
    outl(inl(0x70000020) | 0x80000000, 0x70000020);
    DEV_EN |= DEV_USB;
    while ((inl(0x70000028) & 0x80) == 0);
    outl(inl(0x70000028) | 0x2, 0x70000028);

    udelay(0x186A0);
}

void usb_enable(bool on)
{
    /* This device specific code will eventually give way to proper USB
       handling, which should be the same for all PortalPlayer targets. */
    if (on)
    {
#if IPOD_ARCH || defined(IRIVER_H10) || defined (IRIVER_H10_5GB)
        /* For the H10 and iPod, we can only do one thing with USB mode - reboot
           into the flash-based disk-mode.  This does not return. */
       
        /* The following code is copied from ipodlinux */
#if defined(IPOD_COLOR) || defined(IPOD_3G) || \
    defined(IPOD_4G) || defined(IPOD_MINI)
        unsigned char* storage_ptr = (unsigned char *)0x40017F00;
#elif defined(IPOD_NANO) || defined(IPOD_VIDEO) || defined(IPOD_MINI2G)
        unsigned char* storage_ptr = (unsigned char *)0x4001FF00;
#endif

#if defined(IRIVER_H10) || defined (IRIVER_H10_5GB)
        if(button_status()==BUTTON_RIGHT)
#endif
        {
            ata_sleepnow(); /* Immediately spindown the disk. */
            sleep(HZ*2);
#ifdef IPOD_ARCH
            memcpy(storage_ptr, "diskmode\0\0hotstuff\0\0\1", 21);
#endif
            system_reboot(); /* Reboot */
        }
#endif
    }
}

/*------------------------------------------------------------------
    Internal Hardware related function
 ------------------------------------------------------------------*/

/* @qh_addr is the aligned virt addr of ep QH addr
 * it is used to set endpointlistaddr Reg */
static int dr_controller_setup(void/* *qh_addr, struct device *dev*/)
{
    int timeout = 0;
/*    struct arc_usb_config *config;

    config = udc_controller->config;
*/
    /* before here, make sure usb_slave_regs has been initialized */
/*    if (!qh_addr)
        return -EINVAL;
*/
    /* Stop and reset the usb controller */
    UOG_USBCMD &= ~USB_CMD_RUN_STOP;

    UOG_USBCMD |= USB_CMD_CTRL_RESET;

    /* Wait for reset to complete */
    timeout = 10000000;
    while ((UOG_USBCMD & USB_CMD_CTRL_RESET) &&
           --timeout) {
        continue;
    }
    if (timeout == 0) {
        //logf("%s: TIMEOUT", __FUNCTION__);
        return 1;
    }

    /* Set the controller as device mode and disable setup lockout */
    UOG_USBMODE |= (USB_MODE_CTRL_MODE_DEVICE | USB_MODE_SETUP_LOCK_OFF);

    /* Clear the setup status */
    UOG_USBSTS = 0;

/*    tmp = virt_to_phys(qh_addr);
    tmp &= USB_EP_LIST_ADDRESS_MASK;
    usb_slave_regs->endpointlistaddr = cpu_to_le32(tmp);
*/
    UOG_PORTSC1 = (UOG_PORTSC1 & ~PORTSCX_PHY_TYPE_SEL) | PORTSCX_PTS_UTMI;

/*    if (config->set_vbus_power)
        config->set_vbus_power(0);
*/
    return 0;
}

/* just Enable DR irq reg and Set Dr controller Run */
static void dr_controller_run(void/*struct arcotg_udc *udc*/)
{
    /*Enable DR irq reg */
    UOG_USBINTR = USB_INTR_INT_EN | USB_INTR_ERR_INT_EN |
          USB_INTR_PTC_DETECT_EN | USB_INTR_RESET_EN |
          USB_INTR_DEVICE_SUSPEND | USB_INTR_SYS_ERR_EN;

    /* Clear stopped bit */
    /*udc->stopped = 0;*/
    
    /* Set the controller as device mode */
    UOG_USBMODE |= USB_MODE_CTRL_MODE_DEVICE;

    /* Set controller to Run */
    UOG_USBCMD |= USB_CMD_RUN_STOP;
    
    return;
}

bool usb_detect(void)
{
    static bool prev_usbstatus1 = false;
    bool usbstatus1,usbstatus2;

    /* UOG_ID should have the bit format:
        [31:24] = 0x0
        [23:16] = 0x22 (Revision number)
        [15:14] = 0x3 (Reserved)
        [13:8]  = 0x3a (NID - 1's compliment of ID)
        [7:6]   = 0x0 (Reserved)
        [5:0]   = 0x05 (ID) */
    if (UOG_ID != 0x22FA05) {
        return false;
    }

    usbstatus1 = (UOG_OTGSC & 0x800) ? true : false;
    if ((usbstatus1 == true) && (prev_usbstatus1 == false)) {
        dr_controller_setup();
        dr_controller_run();
    }
    prev_usbstatus1 = usbstatus1;
    usbstatus2 = (UOG_PORTSC1 & PORTSCX_CURRENT_CONNECT_STATUS) ? true : false;

    if (usbstatus1 && usbstatus2) {
        return true;
    } else {
        return false;
    }
}
