/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Amaury Pouly
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
#include "stddef.h"
#include "protocol.h"
#include "logf.h"
#include "usb_ch9.h"
#include "usb_drv.h"
#include "memory.h"
#include "target.h"

extern unsigned char oc_codestart[];
extern unsigned char oc_codeend[];
extern unsigned char oc_stackstart[];
extern unsigned char oc_stackend[];
extern unsigned char oc_bufferstart[];
extern unsigned char oc_bufferend[];

#define oc_codesize ((size_t)(oc_codeend - oc_codestart))
#define oc_stacksize ((size_t)(oc_stackend - oc_stackstart))
#define oc_buffersize ((size_t)(oc_bufferend - oc_bufferstart))

static bool g_exit = false;

/**
 * 
 * USB stack
 * 
 */

static struct usb_device_descriptor __attribute__((aligned(2)))
    device_descriptor=
{
    .bLength            = sizeof(struct usb_device_descriptor),
    .bDescriptorType    = USB_DT_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = HWSTUB_CLASS,
    .bDeviceSubClass    = HWSTUB_SUBCLASS,
    .bDeviceProtocol    = HWSTUB_PROTOCOL,
    .bMaxPacketSize0    = 64,
    .idVendor           = HWSTUB_USB_VID,
    .idProduct          = HWSTUB_USB_PID,
    .bcdDevice          = HWSTUB_VERSION_MAJOR << 8 | HWSTUB_VERSION_MINOR,
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = 0,
    .bNumConfigurations = 1
};

#define USB_MAX_CURRENT 200

static struct usb_config_descriptor __attribute__((aligned(2)))
                                    config_descriptor =
{
    .bLength             = sizeof(struct usb_config_descriptor),
    .bDescriptorType     = USB_DT_CONFIG,
    .wTotalLength        = 0, /* will be filled in later */
    .bNumInterfaces      = 0,
    .bConfigurationValue = 1,
    .iConfiguration      = 0,
    .bmAttributes        = USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
    .bMaxPower           = (USB_MAX_CURRENT + 1) / 2, /* In 2mA units */
};

static const struct usb_string_descriptor __attribute__((aligned(2)))
    usb_string_iManufacturer =
{
    24,
    USB_DT_STRING,
    {'R', 'o', 'c', 'k', 'b', 'o', 'x', '.', 'o', 'r', 'g'}
};

static const struct usb_string_descriptor __attribute__((aligned(2)))
    usb_string_iProduct =
{
    44,
    USB_DT_STRING,
    {'R', 'o', 'c', 'k', 'b', 'o', 'x', ' ',
     'h', 'a', 'r', 'd', 'w', 'a', 'r', 'e', ' ',
     's', 't', 'u', 'b'}
};

/* this is stringid #0: languages supported */
static const struct usb_string_descriptor __attribute__((aligned(2)))
    lang_descriptor =
{
    4,
    USB_DT_STRING,
    {0x0409} /* LANGID US English */
};

static struct hwstub_version_desc_t __attribute__((aligned(2)))
    version_descriptor =
{
    sizeof(struct hwstub_version_desc_t),
    HWSTUB_DT_VERSION,
    HWSTUB_VERSION_MAJOR,
    HWSTUB_VERSION_MINOR,
    HWSTUB_VERSION_REV
};

static struct hwstub_layout_desc_t __attribute__((aligned(2)))
    layout_descriptor =
{
    sizeof(struct hwstub_layout_desc_t),
    HWSTUB_DT_LAYOUT,
    0, 0, 0, 0, 0, 0
};

#define USB_NUM_STRINGS 5

static const struct usb_string_descriptor* const usb_strings[USB_NUM_STRINGS] =
{
   &lang_descriptor,
   &usb_string_iManufacturer,
   &usb_string_iProduct,
};

uint8_t *usb_buffer = oc_bufferstart;
uint32_t usb_buffer_size = 0;

static void fill_layout_info(void)
{
    layout_descriptor.dCodeStart = (uint32_t)oc_codestart;
    layout_descriptor.dCodeSize = oc_codesize;
    layout_descriptor.dStackStart = (uint32_t)oc_stackstart;
    layout_descriptor.dStackSize = oc_stacksize;
    layout_descriptor.dBufferStart = (uint32_t)oc_bufferstart;
    layout_descriptor.dBufferSize = oc_buffersize;
}

static void handle_std_dev_desc(struct usb_ctrlrequest *req)
{
    int size;
    void* ptr = NULL;
    unsigned index = req->wValue & 0xff;

    switch(req->wValue >> 8)
    {
        case USB_DT_DEVICE:
            ptr = &device_descriptor;
            size = sizeof(struct usb_device_descriptor);
            break;
        case USB_DT_OTHER_SPEED_CONFIG:
        case USB_DT_CONFIG:
        {
            int max_packet_size;

            /* config desc */
            if((req->wValue >> 8) ==USB_DT_CONFIG)
            {
                max_packet_size = (usb_drv_port_speed() ? 512 : 64);
                config_descriptor.bDescriptorType = USB_DT_CONFIG;
            }
            else
            {
                max_packet_size = (usb_drv_port_speed() ? 64 : 512);
                config_descriptor.bDescriptorType = USB_DT_OTHER_SPEED_CONFIG;
            }
            size = sizeof(struct usb_config_descriptor);

            /* hwstub version */
            memcpy(usb_buffer + size, (void *)&version_descriptor,
                sizeof(version_descriptor));
            size += sizeof(version_descriptor);
            /* hwstub layout */
            fill_layout_info();
            memcpy(usb_buffer + size, (void *)&layout_descriptor,
                sizeof(layout_descriptor));
            size += sizeof(layout_descriptor);
            /* hwstub target */
            fill_layout_info();
            memcpy(usb_buffer + size, (void *)&target_descriptor,
                sizeof(target_descriptor));
            size += sizeof(target_descriptor);
            /* target specific descriptors */
            target_get_config_desc(usb_buffer + size, &size);
            /* fix config descriptor */
            config_descriptor.wTotalLength = size;
            memcpy(usb_buffer, (void *)&config_descriptor, sizeof(config_descriptor));

            ptr = usb_buffer;
            break;
        }
        case USB_DT_STRING:
            if(index < USB_NUM_STRINGS)
            {
                size = usb_strings[index]->bLength;
                ptr = (void *)usb_strings[index];
            }
            else
                usb_drv_stall(EP_CONTROL, true, true);
            break;
        case HWSTUB_DT_VERSION:
            ptr = &version_descriptor;
            size = sizeof(version_descriptor);
            break;
        case HWSTUB_DT_LAYOUT:
            ptr = &layout_descriptor;
            size = sizeof(layout_descriptor);
            break;
        case HWSTUB_DT_TARGET:
            ptr = &target_descriptor;
            size = sizeof(target_descriptor);
            break;
        default:
            target_get_desc(req->wValue >> 8, &ptr);
            if(ptr != 0)
                size = ((struct usb_descriptor_header *)ptr)->bLength;
            break;
    }

    if(ptr)
    {
        int length = MIN(size, req->wLength);

        if(ptr != usb_buffer)
            memcpy(usb_buffer, ptr, length);

        usb_drv_send(EP_CONTROL, usb_buffer, length);
        usb_drv_recv(EP_CONTROL, NULL, 0);
    }
    else
        usb_drv_stall(EP_CONTROL, true, true);
}

static void handle_std_dev_req(struct usb_ctrlrequest *req)
{
    switch(req->bRequest)
    {
        case USB_REQ_GET_CONFIGURATION:
            usb_buffer[0] = 1;
            usb_drv_send(EP_CONTROL, usb_buffer, 1);
            usb_drv_recv(EP_CONTROL, NULL, 0);
            break;
        case USB_REQ_SET_CONFIGURATION:
            usb_drv_send(EP_CONTROL, NULL, 0);
            break;
        case USB_REQ_GET_DESCRIPTOR:
            handle_std_dev_desc(req);
            break;
        case USB_REQ_SET_ADDRESS:
            usb_drv_send(EP_CONTROL, NULL, 0);
            usb_drv_set_address(req->wValue);
            break;
        case USB_REQ_GET_STATUS:
            usb_buffer[0] = 0;
            usb_buffer[1] = 0;
            usb_drv_send(EP_CONTROL, usb_buffer, 2);
            usb_drv_recv(EP_CONTROL, NULL, 0);
            break;
        default:
            usb_drv_stall(EP_CONTROL, true, true);
    }
}

static void handle_std_req(struct usb_ctrlrequest *req)
{
    switch(req->bRequestType & USB_RECIP_MASK)
    {
        case USB_RECIP_DEVICE:
            return handle_std_dev_req(req);
        default:
            usb_drv_stall(EP_CONTROL, true, true);
    }
}

static void handle_get_log(struct usb_ctrlrequest *req)
{
    enable_logf(false);
    int length = logf_readback(usb_buffer, MIN(req->wLength, usb_buffer_size));
    usb_drv_send(EP_CONTROL, usb_buffer, length);
    usb_drv_recv(EP_CONTROL, NULL, 0);
    enable_logf(true);
}

static void handle_rw_mem(struct usb_ctrlrequest *req)
{
    uint32_t addr = req->wValue | req->wIndex << 16;
    uint16_t length = req->wLength;
    
    if(req->bRequestType & USB_DIR_IN)
    {
        memcpy(usb_buffer, (void *)addr, length);
        asm volatile("nop" : : : "memory");
        usb_drv_send(EP_CONTROL, usb_buffer, length);
        usb_drv_recv(EP_CONTROL, NULL, 0);
    }
    else
    {
        int size = usb_drv_recv(EP_CONTROL, usb_buffer, length);
        asm volatile("nop" : : : "memory");
        if(size != length)
            usb_drv_stall(EP_CONTROL, true, true);
        else
        {
            memcpy((void *)addr, usb_buffer, length);
            usb_drv_send(EP_CONTROL, NULL, 0);
        }
    }
}

static void handle_call_jump(struct usb_ctrlrequest *req)
{
    uint32_t addr = req->wValue | req->wIndex << 16;

    if(req->bRequest == HWSTUB_CALL)
        ((void (*)(void))addr)();
    else
    {
        /* disconnect to make sure usb/dma won't interfere */
        usb_drv_exit();
        asm volatile("bx %0\n" : : "r" (addr) : "memory");
    }
}

static void handle_class_dev_req(struct usb_ctrlrequest *req)
{
    switch(req->bRequest)
    {
        case HWSTUB_GET_LOG:
            handle_get_log(req);
            break;
        case HWSTUB_RW_MEM:
            handle_rw_mem(req);
            break;
        case HWSTUB_CALL:
        case HWSTUB_JUMP:
            handle_call_jump(req);
            break;
            break;
        default:
            usb_drv_stall(EP_CONTROL, true, true);
    }
}

static void handle_class_req(struct usb_ctrlrequest *req)
{
    switch(req->bRequestType & USB_RECIP_MASK)
    {
        case USB_RECIP_DEVICE:
            return handle_class_dev_req(req);
        default:
            usb_drv_stall(EP_CONTROL, true, true);
    }
}

/**
 * 
 * Main
 * 
 */

void main(uint32_t arg)
{
    usb_buffer_size = oc_buffersize;

    logf("hwstub %d.%d.%d\n", HWSTUB_VERSION_MAJOR, HWSTUB_VERSION_MINOR,
         HWSTUB_VERSION_REV);
    logf("argument: 0x%08x\n", arg);

    target_init();
    usb_drv_init();

    while(!g_exit)
    {
        struct usb_ctrlrequest req;
        usb_drv_recv_setup(&req);

        switch(req.bRequestType & USB_TYPE_MASK)
        {
            case USB_TYPE_STANDARD:
                handle_std_req(&req);
                break;
            case USB_TYPE_CLASS:
                handle_class_req(&req);
                break;
            default:
                usb_drv_stall(EP_CONTROL, true, true);
        }
    }
    usb_drv_exit();
}
