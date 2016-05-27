/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2014 Michael Sparmann
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
#ifndef __USB_DESIGNWARE_H__
#define __USB_DESIGNWARE_H__

#include <inttypes.h>
#include "usb-designware-regs.h"


/* select HS PHY/interface configuration */
#define DWC_PHYTYPE_UTMI_8       (0)
#define DWC_PHYTYPE_UTMI_16      (PHYIF16)
#define DWC_PHYTYPE_ULPI_SDR     (ULPI_UTMI_SEL)
#define DWC_PHYTYPE_ULPI_DDR     (ULPI_UTMI_SEL|DDRSEL)


union __attribute__((packed,aligned(4))) usb_ep0_buffer
{
    struct __attribute__((packed,aligned(4)))
    {
        struct __attribute__((packed))
        {
            enum
            {
                USB_SETUP_BMREQUESTTYPE_RECIPIENT_DEVICE = 0,
                USB_SETUP_BMREQUESTTYPE_RECIPIENT_INTERFACE = 1,
                USB_SETUP_BMREQUESTTYPE_RECIPIENT_ENDPOINT = 2,
                USB_SETUP_BMREQUESTTYPE_RECIPIENT_OTHER = 3,
            } recipient : 5;
            enum
            {
                USB_SETUP_BMREQUESTTYPE_TYPE_STANDARD = 0,
                USB_SETUP_BMREQUESTTYPE_TYPE_CLASS = 1,
                USB_SETUP_BMREQUESTTYPE_TYPE_VENDOR = 2,
            } type : 2;
            enum
            {
                USB_SETUP_BMREQUESTTYPE_DIRECTION_OUT = 0,
                USB_SETUP_BMREQUESTTYPE_DIRECTION_IN = 1,
            } direction : 1;
        } bmRequestType;
        union
        {
            enum __attribute__((packed))
            {
                USB_SETUP_BREQUEST_GET_STATUS = 0,
                USB_SETUP_BREQUEST_CLEAR_FEATURE = 1,
                USB_SETUP_BREQUEST_SET_FEATURE = 3,
                USB_SETUP_BREQUEST_SET_ADDRESS = 5,
                USB_SETUP_BREQUEST_GET_DESCRIPTOR = 6,
                USB_SETUP_BREQUEST_SET_DESCRIPTOR = 7,
                USB_SETUP_BREQUEST_GET_CONFIGURATION = 8,
                USB_SETUP_BREQUEST_SET_CONFIGURATION = 9,
                USB_SETUP_BREQUEST_GET_INTERFACE = 10,
                USB_SETUP_BREQUEST_SET_INTERFACE = 11,
                USB_SETUP_BREQUEST_SYNCH_FRAME = 12,
            } req;
            uint8_t raw;
        } bRequest;
        uint16_t wValue;
        uint16_t wIndex;
        uint16_t wLength;
    } setup;
    uint8_t raw[64];
};

/* internal EP state/info */
struct usb_dw_ep
{
    struct semaphore complete;
    uint32_t* req_addr;
    uint32_t req_size;
    uint32_t* addr;
    uint32_t sizeleft;
    uint32_t size;
    int8_t status;
    uint8_t active;
    uint8_t busy;
};

enum usb_dw_epdir
{
    USB_DW_EPDIR_IN = 0,
    USB_DW_EPDIR_OUT = 1,
};

/* configure USB OTG capabilities on SoC */
struct usb_dw_config
{
    uint8_t phytype;  /* DWC_PHYTYPE_ */

    uint16_t rx_fifosz;
    uint16_t nptx_fifosz;
    uint16_t ptx_fifosz;
#ifdef USB_DW_SHARED_FIFO
    bool use_ptxfifo_as_plain_buffer;
#endif
#ifdef USB_DW_ARCH_SLAVE
    bool disable_double_buffering;
#else
    uint8_t ahb_burst_len;  /* HBSTLEN_ */
#ifndef USB_DW_SHARED_FIFO
    uint8_t ahb_threshold;
#endif
#endif
};

extern void usb_dw_target_enable_clocks(void);
extern void usb_dw_target_disable_clocks(void);
extern void usb_dw_target_enable_irq(void);
extern void usb_dw_target_disable_irq(void);
extern void usb_dw_target_clear_irq(void);

extern const struct usb_dw_config usb_dw_config;

#endif /* __USB_DESIGNWARE_H__ */
