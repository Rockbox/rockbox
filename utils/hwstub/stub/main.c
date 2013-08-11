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
    .bDeviceClass       = USB_CLASS_PER_INTERFACE,
    .bDeviceSubClass    = 0,
    .bDeviceProtocol    = 0,
    .bMaxPacketSize0    = 64,
    .idVendor           = HWSTUB_USB_VID,
    .idProduct          = HWSTUB_USB_PID,
    .bcdDevice          = HWSTUB_VERSION_MAJOR << 8 | HWSTUB_VERSION_MINOR,
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = 3,
    .bNumConfigurations = 1
};

#define USB_MAX_CURRENT 200

static struct usb_config_descriptor __attribute__((aligned(2)))
                                    config_descriptor =
{
    .bLength             = sizeof(struct usb_config_descriptor),
    .bDescriptorType     = USB_DT_CONFIG,
    .wTotalLength        = 0, /* will be filled in later */
    .bNumInterfaces      = 1,
    .bConfigurationValue = 1,
    .iConfiguration      = 0,
    .bmAttributes        = USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
    .bMaxPower           = (USB_MAX_CURRENT + 1) / 2, /* In 2mA units */
};

/* main interface */
static struct usb_interface_descriptor __attribute__((aligned(2)))
    interface_descriptor =
{
    .bLength            = sizeof(struct usb_interface_descriptor),
    .bDescriptorType    = USB_DT_INTERFACE,
    .bInterfaceNumber   = 0,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 3,
    .bInterfaceClass    = HWSTUB_CLASS,
    .bInterfaceSubClass = HWSTUB_SUBCLASS,
    .bInterfaceProtocol = HWSTUB_PROTOCOL,
    .iInterface         = 4
};


static struct usb_endpoint_descriptor __attribute__((aligned(2)))
    endpoint_descriptor =
{
    .bLength          = sizeof(struct usb_endpoint_descriptor),
    .bDescriptorType  = USB_DT_ENDPOINT,
    .bEndpointAddress = 0,
    .bmAttributes     = USB_ENDPOINT_XFER_BULK,
    .wMaxPacketSize   = 0,
    .bInterval        = 0
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
    52,
    USB_DT_STRING,
    {'R', 'o', 'c', 'k', 'b', 'o', 'x', ' ',
     'h', 'a', 'r', 'd', 'w', 'a', 'r', 'e', ' ',
     'e', 'm', 'u', 'l', 'a', 't', 'e', 'r'}
};

static struct usb_string_descriptor __attribute__((aligned(2)))
    usb_string_iSerial =
{
    84,
    USB_DT_STRING,
    {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
     '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
     '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
     '0', '0', '0', '0', '0', '0', '0', '0'}
};

static struct usb_string_descriptor __attribute__((aligned(2)))
    usb_string_iInterface =
{
    28,
    USB_DT_STRING,
    {'A', 'c', 'i', 'd', ' ',
     '0' + (HWSTUB_VERSION_MAJOR >> 4), '0' + (HWSTUB_VERSION_MAJOR & 0xf), '.',
     '0' + (HWSTUB_VERSION_MINOR >> 4), '0' + (HWSTUB_VERSION_MINOR & 0xf), '.',
     '0' + (HWSTUB_VERSION_REV >> 4), '0' + (HWSTUB_VERSION_REV & 0xf) }
};

/* this is stringid #0: languages supported */
static const struct usb_string_descriptor __attribute__((aligned(2)))
    lang_descriptor =
{
    4,
    USB_DT_STRING,
    {0x0409} /* LANGID US English */
};

#define USB_NUM_STRINGS 5

static const struct usb_string_descriptor* const usb_strings[USB_NUM_STRINGS] =
{
   &lang_descriptor,
   &usb_string_iManufacturer,
   &usb_string_iProduct,
   &usb_string_iSerial,
   &usb_string_iInterface
};

uint8_t *usb_buffer = oc_bufferstart;
uint32_t usb_buffer_size = 0;

#define EP_BULK 1
#define EP_INT  2

static void set_config(void)
{
    usb_drv_configure_endpoint(EP_BULK, USB_ENDPOINT_XFER_BULK);
    usb_drv_configure_endpoint(EP_INT, USB_ENDPOINT_XFER_INT);
}

static void handle_std_dev_desc(struct usb_ctrlrequest *req)
{
    int size;
    const void* ptr = NULL;
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
                max_packet_size=(usb_drv_port_speed() ? 64 : 512);
                config_descriptor.bDescriptorType = USB_DT_OTHER_SPEED_CONFIG;
            }
            size = sizeof(struct usb_config_descriptor);

            /* interface */
            memcpy(usb_buffer + size, (void *)&interface_descriptor,
                sizeof(interface_descriptor));
            size += sizeof(interface_descriptor);
            /* endpoint 1: bulk out */
            endpoint_descriptor.bEndpointAddress = EP_BULK | USB_DIR_OUT;
            endpoint_descriptor.bmAttributes = USB_ENDPOINT_XFER_BULK;
            endpoint_descriptor.wMaxPacketSize = 512;
            memcpy(usb_buffer + size, (void *)&endpoint_descriptor,
                sizeof(endpoint_descriptor));
            size += sizeof(endpoint_descriptor);
            /* endpoint 2: bulk in */
            endpoint_descriptor.bEndpointAddress = EP_BULK | USB_DIR_IN;
            endpoint_descriptor.bmAttributes = USB_ENDPOINT_XFER_BULK;
            endpoint_descriptor.wMaxPacketSize = 512;
            memcpy(usb_buffer + size, (void *)&endpoint_descriptor,
                sizeof(endpoint_descriptor));
            size += sizeof(endpoint_descriptor);
            /* endpoint 3: int in */
            endpoint_descriptor.bEndpointAddress = EP_INT | USB_DIR_IN;
            endpoint_descriptor.bmAttributes = USB_ENDPOINT_XFER_INT;
            endpoint_descriptor.wMaxPacketSize = 1024;
            memcpy(usb_buffer + size, (void *)&endpoint_descriptor,
                sizeof(endpoint_descriptor));
            size += sizeof(endpoint_descriptor);

            /* fix config descriptor */
            config_descriptor.bNumInterfaces = 1;
            config_descriptor.wTotalLength = size;
            memcpy(usb_buffer, (void *)&config_descriptor, sizeof(config_descriptor));

            ptr = usb_buffer;
            break;
        }
        case USB_DT_STRING:
            if(index < USB_NUM_STRINGS)
            {
                size = usb_strings[index]->bLength;
                ptr = usb_strings[index];
            }
            else
                usb_drv_stall(EP_CONTROL, true, true);
            break;
        default:
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
            set_config();
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

struct usb_resp_info_version_t g_version =
{
    .major = HWSTUB_VERSION_MAJOR,
    .minor = HWSTUB_VERSION_MINOR,
    .revision = HWSTUB_VERSION_REV
};

struct usb_resp_info_layout_t g_layout;

struct usb_resp_info_features_t g_features =
{
    .feature_mask = HWSTUB_FEATURE_LOG | HWSTUB_FEATURE_MEM |
        HWSTUB_FEATURE_CALL | HWSTUB_FEATURE_JUMP
};

static void fill_layout_info(void)
{
    g_layout.oc_code_start = (uint32_t)oc_codestart;
    g_layout.oc_code_size = oc_codesize;
    g_layout.oc_stack_start = (uint32_t)oc_stackstart;
    g_layout.oc_stack_size = oc_stacksize;
    g_layout.oc_buffer_start = (uint32_t)oc_bufferstart;
    g_layout.oc_buffer_size = oc_buffersize;
}

static void handle_get_info(struct usb_ctrlrequest *req)
{
    void *ptr = NULL;
    int size = 0;
    switch(req->wIndex)
    {
        case HWSTUB_INFO_VERSION:
            ptr = &g_version;
            size = sizeof(g_version);
            break;
        case HWSTUB_INFO_LAYOUT:
            fill_layout_info();
            ptr = &g_layout;
            size = sizeof(g_layout);
            break;
        case HWSTUB_INFO_FEATURES:
            ptr = &g_features;
            size = sizeof(g_features);
            break;
        default:
            size = target_get_info(req->wIndex, &ptr);
            if(size < 0)
                usb_drv_stall(EP_CONTROL, true, true);
    }

    if(ptr)
    {
        int length = MIN(size, req->wLength);
        
        if(ptr != usb_buffer)
            memcpy(usb_buffer, ptr, length);
        usb_drv_send(EP_CONTROL, usb_buffer, length);
        usb_drv_recv(EP_CONTROL, NULL, 0);
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

static void handle_atexit(struct usb_ctrlrequest *req)
{
    if(target_atexit(req->wIndex) < 0)
        usb_drv_stall(EP_CONTROL, true, true);
    else
        usb_drv_send(EP_CONTROL, NULL, 0);
}

static void handle_exit(struct usb_ctrlrequest *req)
{
    (void)req;
    usb_drv_send(EP_CONTROL, NULL, 0);
    g_exit = true;
}

static void handle_class_dev_req(struct usb_ctrlrequest *req)
{
    switch(req->bRequest)
    {
        case HWSTUB_GET_INFO:
            handle_get_info(req);
            break;
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
        case HWSTUB_ATEXIT:
            handle_atexit(req);
            break;
        case HWSTUB_EXIT:
            handle_exit(req);
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
    target_exit();
}
