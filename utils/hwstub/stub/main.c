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
#include "config.h"
#include "protocol.h"
#include "logf.h"
#include "usb_ch9.h"
#include "usb_drv.h"
#include "memory.h"
#include "target.h"
#include "system.h"

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

static struct usb_device_descriptor device_descriptor=
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
    .iSerialNumber      = 0,
    .bNumConfigurations = 1
};

#define USB_MAX_CURRENT 200

static struct usb_config_descriptor config_descriptor =
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

#define USB_HWSTUB_INTF 0

static struct usb_interface_descriptor interface_descriptor =
{
    .bLength            = sizeof(struct usb_interface_descriptor),
    .bDescriptorType    = USB_DT_INTERFACE,
    .bInterfaceNumber   = USB_HWSTUB_INTF,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 0,
    .bInterfaceClass    = HWSTUB_CLASS,
    .bInterfaceSubClass = HWSTUB_SUBCLASS,
    .bInterfaceProtocol = HWSTUB_PROTOCOL,
    .iInterface         = 3
};

static const struct usb_string_descriptor usb_string_iManufacturer =
{
    24,
    USB_DT_STRING,
    {'R', 'o', 'c', 'k', 'b', 'o', 'x', '.', 'o', 'r', 'g'}
};

static const struct usb_string_descriptor usb_string_iProduct =
{
    44,
    USB_DT_STRING,
    {'R', 'o', 'c', 'k', 'b', 'o', 'x', ' ',
     'h', 'a', 'r', 'd', 'w', 'a', 'r', 'e', ' ',
     's', 't', 'u', 'b'}
};

static const struct usb_string_descriptor usb_string_iInterface =
{
    14,
    USB_DT_STRING,
    {'H', 'W', 'S', 't', 'u', 'b'}
};

/* this is stringid #0: languages supported */
static const struct usb_string_descriptor lang_descriptor =
{
    4,
    USB_DT_STRING,
    {0x0409} /* LANGID US English */
};

static struct hwstub_version_desc_t version_descriptor =
{
    sizeof(struct hwstub_version_desc_t),
    HWSTUB_DT_VERSION,
    HWSTUB_VERSION_MAJOR,
    HWSTUB_VERSION_MINOR,
    HWSTUB_VERSION_REV
};

static struct hwstub_layout_desc_t layout_descriptor =
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
   &usb_string_iInterface
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
            /* int max_packet_size; */

            /* config desc */
            if((req->wValue >> 8) == USB_DT_CONFIG)
            {
                /* max_packet_size = (usb_drv_port_speed() ? 512 : 64); */
                config_descriptor.bDescriptorType = USB_DT_CONFIG;
            }
            else
            {
                /* max_packet_size = (usb_drv_port_speed() ? 64 : 512); */
                config_descriptor.bDescriptorType = USB_DT_OTHER_SPEED_CONFIG;
            }
            size = sizeof(struct usb_config_descriptor);

            /* interface desc */
            memcpy(usb_buffer + size, (void *)&interface_descriptor,
                sizeof(interface_descriptor));
            size += sizeof(interface_descriptor);
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
            memcpy(usb_buffer, (void *)&config_descriptor,
                sizeof(config_descriptor));

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

static void handle_std_intf_desc(struct usb_ctrlrequest *req)
{
    int size;
    void* ptr = NULL;

    switch(req->wValue >> 8)
    {
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

static void handle_std_intf_req(struct usb_ctrlrequest *req)
{
    unsigned intf = req->wIndex & 0xff;
    if(intf != USB_HWSTUB_INTF)
        return usb_drv_stall(EP_CONTROL, true, true);

    switch(req->bRequest)
    {
        case USB_REQ_GET_DESCRIPTOR:
            handle_std_intf_desc(req);
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
        case USB_RECIP_INTERFACE:
            return handle_std_intf_req(req);
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

/* default implementation, relying on the compiler to produce correct code,
 * targets should reimplement this... */
uint8_t __attribute__((weak)) target_read8(const void *addr)
{
    return *(volatile uint8_t *)addr;
}

uint16_t __attribute__((weak)) target_read16(const void *addr)
{
    return *(volatile uint16_t *)addr;
}

uint32_t __attribute__((weak)) target_read32(const void *addr)
{
    return *(volatile uint32_t *)addr;
}

void __attribute__((weak)) target_write8(void *addr, uint8_t val)
{
    *(volatile uint8_t *)addr = val;
}

void __attribute__((weak)) target_write16(void *addr, uint16_t val)
{
    *(volatile uint16_t *)addr = val;
}

void __attribute__((weak)) target_write32(void *addr, uint32_t val)
{
    *(volatile uint32_t *)addr = val;
}

static bool read_atomic(void *dst, void *src, size_t sz)
{
    switch(sz)
    {
        case 1: *(uint8_t *)dst = target_read8(src); return true;
        case 2: *(uint16_t *)dst = target_read16(src); return true;
        case 4: *(uint32_t *)dst = target_read32(src); return true;
        default: return false;
    }
}

static void handle_read(struct usb_ctrlrequest *req)
{
    static uint32_t last_addr = 0;
    static uint16_t last_id = 0xffff;
    uint16_t id = req->wValue;

    if(req->bRequest == HWSTUB_READ)
    {
        int size = usb_drv_recv(EP_CONTROL, usb_buffer, req->wLength);
        if(size != sizeof(struct hwstub_read_req_t))
            return usb_drv_stall(EP_CONTROL, true, true);
        asm volatile("nop" : : : "memory");
        struct hwstub_read_req_t *read = (void *)usb_buffer;
        last_addr = read->dAddress;
        last_id = id;
        usb_drv_send(EP_CONTROL, NULL, 0);
    }
    else
    {
        if(id != last_id)
            return usb_drv_stall(EP_CONTROL, true, true);

        if(req->bRequest == HWSTUB_READ2_ATOMIC)
        {
            if(set_data_abort_jmp() == 0)
            {
                if(!read_atomic(usb_buffer, (void *)last_addr, req->wLength))
                    return usb_drv_stall(EP_CONTROL, true, true);
            }
            else
            {
                logf("trapped read data abort in [0x%x,0x%x]\n", last_addr,
                    last_addr + req->wLength);
                return usb_drv_stall(EP_CONTROL, true, true);
            }
        }
        else
        {
            if(set_data_abort_jmp() == 0)
            {
                memcpy(usb_buffer, (void *)last_addr, req->wLength);
                asm volatile("nop" : : : "memory");
            }
            else
            {
                logf("trapped read data abort in [0x%x,0x%x]\n", last_addr,
                    last_addr + req->wLength);
                return usb_drv_stall(EP_CONTROL, true, true);
            }

        }

        usb_drv_send(EP_CONTROL, usb_buffer, req->wLength);
        usb_drv_recv(EP_CONTROL, NULL, 0);
    }
}

static bool write_atomic(void *dst, void *src, size_t sz)
{
    switch(sz)
    {
        case 1: target_write8(dst, *(uint8_t *)src); return true;
        case 2: target_write16(dst, *(uint16_t *)src); return true;
        case 4: target_write32(dst, *(uint32_t *)src); return true;
        default: return false;
    }
}

static void handle_write(struct usb_ctrlrequest *req)
{
    int size = usb_drv_recv(EP_CONTROL, usb_buffer, req->wLength);
    asm volatile("nop" : : : "memory");
    struct hwstub_write_req_t *write = (void *)usb_buffer;
    int sz_hdr = sizeof(struct hwstub_write_req_t);
    if(size < sz_hdr)
        return usb_drv_stall(EP_CONTROL, true, true);

    if(req->bRequest == HWSTUB_WRITE_ATOMIC)
    {
        if(set_data_abort_jmp() == 0)
        {
            if(!write_atomic((void *)write->dAddress,
                             usb_buffer + sz_hdr, size - sz_hdr))
                return usb_drv_stall(EP_CONTROL, true, true);
        }
        else
        {
            logf("trapped write data abort in [0x%x,0x%x]\n", write->dAddress,
                 write->dAddress + size - sz_hdr);
            return usb_drv_stall(EP_CONTROL, true, true);
        }
    }
    else
    {
        if(set_data_abort_jmp() == 0)
        {
            memcpy((void *)write->dAddress,
                   usb_buffer + sz_hdr, size - sz_hdr);
        }
        else
        {
            logf("trapped write data abort in [0x%x,0x%x]\n", write->dAddress,
                 write->dAddress + size - sz_hdr);
            return usb_drv_stall(EP_CONTROL, true, true);
        }
    }

    usb_drv_send(EP_CONTROL, NULL, 0);
}

static void handle_exec(struct usb_ctrlrequest *req)
{
    int size = usb_drv_recv(EP_CONTROL, usb_buffer, req->wLength);
    asm volatile("nop" : : : "memory");
    struct hwstub_exec_req_t *exec = (void *)usb_buffer;
    if(size != sizeof(struct hwstub_exec_req_t))
        return usb_drv_stall(EP_CONTROL, true, true);
    uint32_t addr = exec->dAddress;

#if defined(CPU_ARM)
    if(exec->bmFlags & HWSTUB_EXEC_THUMB)
        addr |= 1;
    else
        addr &= ~1;
#endif

#ifdef CONFIG_FLUSH_CACHES
    target_flush_caches();
#endif

    if(exec->bmFlags & HWSTUB_EXEC_CALL)
    {
#if defined(CPU_ARM)
        /* in case of call, respond after return */
        asm volatile("blx %0\n" : : "r"(addr) : "memory");
        usb_drv_send(EP_CONTROL, NULL, 0);
#elif defined(CPU_MIPS)
        asm volatile("jalr %0\nnop\n" : : "r"(addr) : "memory");
        usb_drv_send(EP_CONTROL, NULL, 0);
#else
#warning call is unsupported on this platform
        usb_drv_stall(EP_CONTROL, true, true);
#endif
    }
    else
    {
        /* in case of jump, respond immediately and disconnect usb */
#if defined(CPU_ARM)
        usb_drv_send(EP_CONTROL, NULL, 0);
        usb_drv_exit();
        asm volatile("bx %0\n" : : "r" (addr) : "memory");
#elif defined(CPU_MIPS)
        usb_drv_send(EP_CONTROL, NULL, 0);
        usb_drv_exit();
        asm volatile("jr %0\nnop\n" : : "r" (addr) : "memory");
#else 
#warning jump is unsupported on this platform
        usb_drv_stall(EP_CONTROL, true, true);
#endif
    }
}

static void handle_class_intf_req(struct usb_ctrlrequest *req)
{
    unsigned intf = req->wIndex & 0xff;
    if(intf != USB_HWSTUB_INTF)
        return usb_drv_stall(EP_CONTROL, true, true);

    switch(req->bRequest)
    {
        case HWSTUB_GET_LOG:
            return handle_get_log(req);
        case HWSTUB_READ:
        case HWSTUB_READ2:
        case HWSTUB_READ2_ATOMIC:
            return handle_read(req);
        case HWSTUB_WRITE:
        case HWSTUB_WRITE_ATOMIC:
            return handle_write(req);
        case HWSTUB_EXEC:
            return handle_exec(req);
        default:
            usb_drv_stall(EP_CONTROL, true, true);
    }
}

static void handle_class_req(struct usb_ctrlrequest *req)
{
    switch(req->bRequestType & USB_RECIP_MASK)
    {
        case USB_RECIP_INTERFACE:
            return handle_class_intf_req(req);
        case USB_RECIP_DEVICE:
            //return handle_class_dev_req(req);
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
