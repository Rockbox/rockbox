/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:  $
 *
 * Copyright (C) 2007 by Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "system.h"
#include "thread.h"
#include "kernel.h"
#include "string.h"
//#define LOGF_ENABLE
#include "logf.h"

//#define USB_STORAGE
//#define USB_BENCHMARK
#define USB_CHARGING_ONLY

#include "usb_ch9.h"
#include "usb_drv.h"
#include "usb_core.h"

#if defined(USB_STORAGE)
#include "usb_storage.h"
#elif defined(USB_BENCHMARK)
#include "usb_benchmark.h"
#endif

/*-------------------------------------------------------------------------*/
/* USB protocol descriptors: */

#define USB_SC_SCSI      0x06            /* Transparent */
#define USB_PROT_BULK    0x50            /* bulk only */

int usb_max_pkt_size = 512;

static const struct usb_device_descriptor device_descriptor = {
    .bLength            = sizeof(struct usb_device_descriptor),
    .bDescriptorType    = USB_DT_DEVICE,
    .bcdUSB             = 0x0200, /* USB version 2.0 */
    .bDeviceClass       = USB_CLASS_PER_INTERFACE,
    .bDeviceSubClass    = 0,
    .bDeviceProtocol    = 0,
    .bMaxPacketSize0    = 64,
    .idVendor           = USB_VENDOR_ID,
    .idProduct          = USB_PRODUCT_ID,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = 0,
    .bNumConfigurations = 1
};

static const struct {
    struct usb_config_descriptor config_descriptor;
    struct usb_interface_descriptor interface_descriptor;
    struct usb_endpoint_descriptor ep1_hs_in_descriptor;
    struct usb_endpoint_descriptor ep1_hs_out_descriptor;
} config_data = 
{
    {
        .bLength             = sizeof(struct usb_config_descriptor),
        .bDescriptorType     = USB_DT_CONFIG,
        .wTotalLength        = sizeof config_data,
        .bNumInterfaces      = 1,
        .bConfigurationValue = 1,
        .iConfiguration      = 0,
        .bmAttributes        = USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
        .bMaxPower           = 250, /* 500mA in 2mA units */
    },
    
#ifdef USB_CHARGING_ONLY
    /* dummy interface for charging-only */
    {
        .bLength            = sizeof(struct usb_interface_descriptor),
        .bDescriptorType    = USB_DT_INTERFACE,
        .bInterfaceNumber   = 0,
        .bAlternateSetting  = 0,
        .bNumEndpoints      = 2,
        .bInterfaceClass    = USB_CLASS_VENDOR_SPEC,
        .bInterfaceSubClass = 0,
        .bInterfaceProtocol = 0,
        .iInterface         = 4
    },

    {
        .bLength          = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType  = USB_DT_ENDPOINT,
        .bEndpointAddress = EP_TX | USB_DIR_IN,
        .bmAttributes     = USB_ENDPOINT_XFER_BULK,
        .wMaxPacketSize   = 512,
        .bInterval        = 0
    },
    {
        .bLength          = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType  = USB_DT_ENDPOINT,
        .bEndpointAddress = EP_RX | USB_DIR_OUT,
        .bmAttributes     = USB_ENDPOINT_XFER_BULK,
        .wMaxPacketSize   = 512,
        .bInterval        = 0
    }
#endif

#ifdef USB_STORAGE
    /* storage interface */
    {
        .bLength            = sizeof(struct usb_interface_descriptor),
        .bDescriptorType    = USB_DT_INTERFACE,
        .bInterfaceNumber   = 0,
        .bAlternateSetting  = 0,
        .bNumEndpoints      = 2,
        .bInterfaceClass    = USB_CLASS_MASS_STORAGE,
        .bInterfaceSubClass = USB_SC_SCSI,
        .bInterfaceProtocol = USB_PROT_BULK,
        .iInterface         = 0
    },

    {
        .bLength          = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType  = USB_DT_ENDPOINT,
        .bEndpointAddress = EP_TX | USB_DIR_IN,
        .bmAttributes     = USB_ENDPOINT_XFER_BULK,
        .wMaxPacketSize   = 512,
        .bInterval        = 0
    },
    {
        .bLength          = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType  = USB_DT_ENDPOINT,
        .bEndpointAddress = EP_RX | USB_DIR_OUT,
        .bmAttributes     = USB_ENDPOINT_XFER_BULK,
        .wMaxPacketSize   = 512,
        .bInterval        = 0
    }
#endif

#ifdef USB_SERIAL
    /* serial interface */
    {
        .bLength            = sizeof(struct usb_interface_descriptor),
        .bDescriptorType    = USB_DT_INTERFACE,
        .bInterfaceNumber   = 0,
        .bAlternateSetting  = 0,
        .bNumEndpoints      = 2,
        .bInterfaceClass    = USB_CLASS_CDC_DATA,
        .bInterfaceSubClass = 0,
        .bInterfaceProtocol = 0,
        .iInterface         = 0
    },

    {
        .bLength          = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType  = USB_DT_ENDPOINT,
        .bEndpointAddress = EP_TX | USB_DIR_IN,
        .bmAttributes     = USB_ENDPOINT_XFER_BULK,
        .wMaxPacketSize   = 512,
        .bInterval        = 0
    },
    {
        .bLength          = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType  = USB_DT_ENDPOINT,
        .bEndpointAddress = EP_RX | USB_DIR_OUT,
        .bmAttributes     = USB_ENDPOINT_XFER_BULK,
        .wMaxPacketSize   = 512,
        .bInterval        = 0
    }
#endif

#ifdef USB_BENCHMARK
    /* bulk test interface */
    {
        .bLength            = sizeof(struct usb_interface_descriptor),
        .bDescriptorType    = USB_DT_INTERFACE,
        .bInterfaceNumber   = 0,
        .bAlternateSetting  = 0,
        .bNumEndpoints      = 2,
        .bInterfaceClass    = USB_CLASS_VENDOR_SPEC,
        .bInterfaceSubClass = 255,
        .bInterfaceProtocol = 255,
        .iInterface         = 3
    },

    {
        .bLength          = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType  = USB_DT_ENDPOINT,
        .bEndpointAddress = EP_RX | USB_DIR_OUT,
        .bmAttributes     = USB_ENDPOINT_XFER_BULK,
//        .wMaxPacketSize   = 64,
        .wMaxPacketSize   = 512,
        .bInterval        = 0
    },
    {
        .bLength          = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType  = USB_DT_ENDPOINT,
        .bEndpointAddress = EP_TX | USB_DIR_IN,
        .bmAttributes     = USB_ENDPOINT_XFER_BULK,
//        .wMaxPacketSize   = 64,
        .wMaxPacketSize   = 512,
        .bInterval        = 0
    }
#endif
};

static const struct usb_qualifier_descriptor qualifier_descriptor =
{
    .bLength            = sizeof(struct usb_qualifier_descriptor),
    .bDescriptorType    = USB_DT_DEVICE_QUALIFIER,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0,
    .bDeviceSubClass    = 0,
    .bDeviceProtocol    = 0,
    .bMaxPacketSize0    = 64,
    .bNumConfigurations = 1
};

/* full speed = 12 Mbit */
static const struct usb_endpoint_descriptor ep1_fs_in_descriptor =
{
    .bLength          = sizeof(struct usb_endpoint_descriptor),
    .bDescriptorType  = USB_DT_ENDPOINT,
    .bEndpointAddress = 1 | USB_DIR_IN,
    .bmAttributes     = USB_ENDPOINT_XFER_BULK,
    .wMaxPacketSize   = 64,
    .bInterval        = 0
};

static const struct usb_endpoint_descriptor ep1_fs_out_descriptor =
{
    .bLength          = sizeof(struct usb_endpoint_descriptor),
    .bDescriptorType  = USB_DT_ENDPOINT,
    .bEndpointAddress = 1 | USB_DIR_OUT,
    .bmAttributes     = USB_ENDPOINT_XFER_BULK,
    .wMaxPacketSize   = 64,
    .bInterval        = 0
};

static const struct usb_endpoint_descriptor* ep_descriptors[4] =
{
    &config_data.ep1_hs_in_descriptor,
    &config_data.ep1_hs_out_descriptor,
    &ep1_fs_in_descriptor,
    &ep1_fs_out_descriptor
};

/* this is stringid #0: languages supported */
static const struct usb_string_descriptor lang_descriptor =
{
    sizeof(struct usb_string_descriptor),
    USB_DT_STRING,
    {0x0409} /* LANGID US English */
};

/* this is stringid #1 and up: the actual strings */
static const struct {
    unsigned char size;
    unsigned char type;
    unsigned short string[32];
} usb_strings[] =
{
    {
        24,
        USB_DT_STRING,
        {'R','o','c','k','b','o','x','.','o','r','g'}
    },
    {
        42,
        USB_DT_STRING,
        {'R','o','c','k','b','o','x',' ','m','e','d','i','a',' ','p','l','a','y','e','r'}
    },
    {
        40,
        USB_DT_STRING,
        {'B','u','l','k',' ','t','e','s','t',' ','i','n','t','e','r','f','a','c','e'}
    },
    {
        28,
        USB_DT_STRING,
        {'C','h','a','r','g','i','n','g',' ','o','n','l','y'}
    }
};


static int usb_address = 0;
static bool initialized = false;
static bool data_connection = false;
static struct event_queue usbcore_queue;
static enum { DEFAULT, ADDRESS, CONFIGURED } usb_state;

#ifdef USB_STORAGE
static const char usbcore_thread_name[] = "usb_core";
static struct thread_entry* usbcore_thread;
static long usbcore_stack[DEFAULT_STACK_SIZE];
static void usb_core_thread(void);
#endif

static void ack_control(struct usb_ctrlrequest* req);

void usb_core_init(void)
{
    if (initialized)
        return;

    queue_init(&usbcore_queue, false);
    usb_drv_init();
#ifdef USB_STORAGE
    usb_storage_init();
    usbcore_thread =
        create_thread(usb_core_thread, usbcore_stack, sizeof(usbcore_stack), 0,
                      usbcore_thread_name
                      IF_PRIO(, PRIORITY_SYSTEM)
                      IF_COP(, CPU));
#endif

#ifdef USB_BENCHMARK
    usb_benchmark_init();
#endif
    initialized = true;
    usb_state = DEFAULT;
    logf("usb_core_init() finished");
}

void usb_core_exit(void)
{
    if (initialized) {
        usb_drv_exit();
        queue_delete(&usbcore_queue);
#ifdef USB_STORAGE
        remove_thread(usbcore_thread);
#endif
    }
    data_connection = false;
    initialized = false;
    logf("usb_core_exit() finished");
}

bool usb_core_data_connection(void)
{
    return data_connection;
}

#ifdef USB_STORAGE
void usb_core_thread(void)
{
    while (1) {
        struct queue_event ev;
    
        queue_wait(&usbcore_queue, &ev);

        usb_storage_transfer_complete(ev.id);
    }
}
#endif

/* called by usb_drv_int() */
void usb_core_control_request(struct usb_ctrlrequest* req)
{
    /* note: interrupt context */
    data_connection = true;

#ifdef USB_BENCHMARK
    if ((req->bRequestType & 0x60) == USB_TYPE_VENDOR) {
        usb_benchmark_control_request(req);
        return;
    }
#endif

    switch (req->bRequest) {
        case USB_REQ_SET_CONFIGURATION:
            logf("usb_core: SET_CONFIG");
#ifdef USB_STORAGE
            usb_storage_control_request(req);
#endif
            ack_control(req);
            usb_state = CONFIGURED;
            break;

        case USB_REQ_GET_CONFIGURATION: {
            static char confignum;
            char* tmp = (void*)UNCACHED_ADDR(&confignum);
            logf("usb_core: GET_CONFIG");
            if (usb_state == ADDRESS)
                *tmp = 0;
            else
                *tmp = 1;
            usb_drv_send(EP_CONTROL, tmp, 1);
            ack_control(req);
            break;
        }
            
        case USB_REQ_SET_INTERFACE:
            logf("usb_core: SET_INTERFACE");
            ack_control(req);
            break;

        case USB_REQ_CLEAR_FEATURE:
            logf("usb_core: CLEAR_FEATURE");
            if (req->wValue)
                usb_drv_stall(req->wIndex, true);
            else
                usb_drv_stall(req->wIndex, false);
            ack_control(req);
            break;

        case USB_REQ_SET_ADDRESS:
            usb_address = req->wValue;
            logf("usb_core: SET_ADR %d", usb_address);
            ack_control(req);
            usb_drv_set_address(usb_address);
            usb_state = ADDRESS;
            break;

        case USB_REQ_GET_STATUS: {
            static char tmp[2] = {0,0};
            tmp[0] = 0;
            tmp[1] = 0;
            logf("usb_core: GET_STATUS"); 
            usb_drv_send(EP_CONTROL, UNCACHED_ADDR(&tmp), 2);
            ack_control(req);
            break;
        }

        case USB_REQ_GET_DESCRIPTOR: {
            int index = req->wValue & 0xff;
            int length = req->wLength;
            int size;
            const void* ptr = NULL;
            logf("usb_core: GET_DESC %d", req->wValue >> 8);

            switch (req->wValue >> 8) { /* type */
                case USB_DT_DEVICE:
                    ptr = &device_descriptor;
                    size = sizeof device_descriptor;
                    break;

                case USB_DT_CONFIG:
                    ptr = &config_data;
                    size = sizeof config_data;
                    break;

                case USB_DT_STRING:
                    switch (index) {
                        case 0: /* lang descriptor */
                            ptr = &lang_descriptor;
                            size = sizeof lang_descriptor;
                            break;

                        default:
                            if ((unsigned)index <= (sizeof(usb_strings)/sizeof(usb_strings[0]))) {
                                index -= 1;
                                ptr = &usb_strings[index];
                                size = usb_strings[index].size;
                            }
                            else {
                                logf("bad string id %d", index);
                                usb_drv_stall(EP_CONTROL, true);
                            }
                            break;
                    }
                    break;

                case USB_DT_INTERFACE:
                    ptr = &config_data.interface_descriptor;
                    size = sizeof config_data.interface_descriptor;
                    break;

                case USB_DT_ENDPOINT:
                    if (index <= NUM_ENDPOINTS) {
                        ptr = &ep_descriptors[index];
                        size = sizeof ep_descriptors[index];
                    }
                    else {
                        logf("bad endpoint %d", index);
                        usb_drv_stall(EP_CONTROL, true);
                    }
                    break;

                case USB_DT_DEVICE_QUALIFIER:
                    ptr = &qualifier_descriptor;
                    size = sizeof qualifier_descriptor;
                    break;

/*
  case USB_DT_OTHER_SPEED_CONFIG:
  ptr = &other_speed_descriptor;
  size = sizeof other_speed_descriptor;
  break;
*/
                default:
                    logf("bad desc %d", req->wValue >> 8);
                    usb_drv_stall(EP_CONTROL, true);
                    break;
            }

            if (ptr) {
                length = MIN(size, length);
                usb_drv_send(EP_CONTROL, (void*)UNCACHED_ADDR(ptr), length);
            }
            ack_control(req);
            break;
        } /* USB_REQ_GET_DESCRIPTOR */

        default:
#ifdef USB_STORAGE
            /* does usb_storage know this request? */
            if (!usb_storage_control_request(req))
#endif
            {
                /* nope. flag error */
                logf("usb bad req %d", req->bRequest);
                usb_drv_stall(EP_CONTROL, true);
                ack_control(req);
            }
            break;
    }
}

/* called by usb_drv_int() */
void usb_core_bus_reset(void)
{
    usb_address = 0;
    data_connection = false;
    usb_state = DEFAULT;
}

/* called by usb_drv_transfer_completed() */
void usb_core_transfer_complete(int endpoint, bool in)
{
#ifdef USB_CHARGING_ONLY
    (void)in;
#endif

    switch (endpoint) {
        case EP_CONTROL:
            /* already handled */
            break;

        case EP_RX:
        case EP_TX:
#if defined(USB_BENCHMARK)
            usb_benchmark_transfer_complete(endpoint, in);
#elif defined(USB_STORAGE)
            queue_post(&usbcore_queue, endpoint, 0);
#endif
            break;
                
        default:
            break;
    }
}

static void ack_control(struct usb_ctrlrequest* req)
{
    if (req->bRequestType & 0x80)
        usb_drv_recv(EP_CONTROL, NULL, 0);
    else
        usb_drv_send(EP_CONTROL, NULL, 0);
}

