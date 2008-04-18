/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Bj�rn Stenberg
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

#include "usb.h"
#include "usb_ch9.h"
#include "usb_drv.h"
#include "usb_core.h"
#include "usb_class_driver.h"

#if defined(USB_STORAGE)
#include "usb_storage.h"
#endif

#if defined(USB_SERIAL)
#include "usb_serial.h"
#endif

/* TODO: Move target-specific stuff somewhere else (serial number reading) */

#ifdef HAVE_AS3514
#include "i2c-pp.h"
#include "as3514.h"
#endif

#if !defined(HAVE_AS3514) && !defined(IPOD_ARCH)
#include "ata.h"
#endif


/*-------------------------------------------------------------------------*/
/* USB protocol descriptors: */

#define USB_SC_SCSI      0x06            /* Transparent */
#define USB_PROT_BULK    0x50            /* bulk only */

static const struct usb_device_descriptor __attribute__((aligned(2)))
                                          device_descriptor=
{
    .bLength            = sizeof(struct usb_device_descriptor),
    .bDescriptorType    = USB_DT_DEVICE,
#ifdef USE_HIGH_SPEED
    .bcdUSB             = 0x0200,
#else
    .bcdUSB             = 0x0110,
#endif
    .bDeviceClass       = USB_CLASS_PER_INTERFACE,
    .bDeviceSubClass    = 0,
    .bDeviceProtocol    = 0,
    .bMaxPacketSize0    = 64,
    .idVendor           = USB_VENDOR_ID,
    .idProduct          = USB_PRODUCT_ID,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = 3,
    .bNumConfigurations = 1
} ;

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
    .bMaxPower           = 250, /* 500mA in 2mA units */
};

#ifdef USB_CHARGING_ONLY
/* dummy interface for charging-only */
static struct usb_interface_descriptor __attribute__((aligned(2)))
                                       charging_interface_descriptor =
{
    .bLength            = sizeof(struct usb_interface_descriptor),
    .bDescriptorType    = USB_DT_INTERFACE,
    .bInterfaceNumber   = 0,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 0,
    .bInterfaceClass    = USB_CLASS_VENDOR_SPEC,
    .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0,
    .iInterface         = 4
};
#endif

static const struct usb_qualifier_descriptor __attribute__((aligned(2)))
                                             qualifier_descriptor =
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

static const struct usb_string_descriptor __attribute__((aligned(2)))
                                    usb_string_iManufacturer =
{
    24,
    USB_DT_STRING,
    {'R','o','c','k','b','o','x','.','o','r','g'}
};

static const struct usb_string_descriptor __attribute__((aligned(2)))
                                    usb_string_iProduct =
{
    42,
    USB_DT_STRING,
    {'R','o','c','k','b','o','x',' ',
     'm','e','d','i','a',' ',
     'p','l','a','y','e','r'}
};

static struct usb_string_descriptor __attribute__((aligned(2)))
                                    usb_string_iSerial =
{
    84,
    USB_DT_STRING,
    {'0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0',
     '0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0',
     '0','0','0','0','0','0','0','0','0'}
};

/* Generic for all targets */

/* this is stringid #0: languages supported */
static const struct usb_string_descriptor __attribute__((aligned(2)))
                                    lang_descriptor =
{
    4,
    USB_DT_STRING,
    {0x0409} /* LANGID US English */
};

static const struct usb_string_descriptor __attribute__((aligned(2)))
                                    usb_string_charging_only =
{
    28,
    USB_DT_STRING,
    {'C','h','a','r','g','i','n','g',' ','o','n','l','y'}
};

static const struct usb_string_descriptor* const usb_strings[] =
{
   &lang_descriptor,
   &usb_string_iManufacturer,
   &usb_string_iProduct,
   &usb_string_iSerial,
   &usb_string_charging_only
};

static int usb_address = 0;
static bool initialized = false;
static enum { DEFAULT, ADDRESS, CONFIGURED } usb_state;

static int usb_core_num_interfaces;

static int usb_charging_get_config_descriptor(unsigned char *dest,int max_packet_size,
                                              int interface_number,int endpoint);

static struct usb_class_driver drivers[USB_NUM_DRIVERS] =
{
#ifdef USB_STORAGE
    [USB_DRIVER_MASS_STORAGE] = {
        .enabled = false,
        .needs_exclusive_ata = true,
        .usb_endpoint = 0,
        .usb_interface = 0,
        .get_config_descriptor = usb_storage_get_config_descriptor,
        .init_connection = usb_storage_init_connection,
        .init = usb_storage_init,
        .disconnect = NULL,
        .transfer_complete = usb_storage_transfer_complete,
        .control_request = usb_storage_control_request,
#ifdef HAVE_HOTSWAP
        .notify_hotswap = usb_storage_notify_hotswap,
#endif
    },
#endif
#ifdef USB_SERIAL
    [USB_DRIVER_SERIAL] = {
        .enabled = false,
        .needs_exclusive_ata = false,
        .usb_endpoint = 0,
        .usb_interface = 0,
        .get_config_descriptor = usb_serial_get_config_descriptor,
        .init_connection = usb_serial_init_connection,
        .init = usb_serial_init,
        .disconnect = usb_serial_disconnect,
        .transfer_complete = usb_serial_transfer_complete,
        .control_request = usb_serial_control_request,
#ifdef HAVE_HOTSWAP
        .notify_hotswap = NULL,
#endif
    },
#endif
#ifdef USB_CHARGING_ONLY
    [USB_DRIVER_CHARGING_ONLY] = {
        .enabled = false,
        .needs_exclusive_ata = false,
        .usb_endpoint = 0,
        .usb_interface = 0,
        .get_config_descriptor = usb_charging_get_config_descriptor,
        .init_connection = NULL,
        .init = NULL,
        .disconnect = NULL,
        .transfer_complete = NULL,
        .control_request = NULL,
#ifdef HAVE_HOTSWAP
        .notify_hotswap = NULL,
#endif
    },
#endif
};

static void usb_core_control_request_handler(struct usb_ctrlrequest* req);
static int ack_control(struct usb_ctrlrequest* req);

static unsigned char response_data[256] USBDEVBSS_ATTR;

static struct usb_transfer_completion_event_data events[NUM_ENDPOINTS];

static short hex[16] = {'0','1','2','3','4','5','6','7',
                        '8','9','A','B','C','D','E','F'};
#ifdef IPOD_ARCH
static void set_serial_descriptor(void)
{
#ifdef IPOD_VIDEO
    uint32_t* serial = (uint32_t*)(0x20004034);
#else
    uint32_t* serial = (uint32_t*)(0x20002034);
#endif

    /* We need to convert from a little-endian 64-bit int
       into a utf-16 string of hex characters */
    short* p = &usb_string_iSerial.wString[24];
    uint32_t x;
    int i,j;

    for (i = 0; i < 2; i++) {
       x = serial[i];
       for (j=0;j<8;j++) {
          *p-- = hex[x & 0xf];
          x >>= 4;
       }
    }
    usb_string_iSerial.bLength=52;
}
#elif defined(HAVE_AS3514)
static void set_serial_descriptor(void)
{
    unsigned char serial[16];
    /* Align 32 digits right in the 40-digit serial number */
    short* p = &usb_string_iSerial.wString[1];
    int i;

    i2c_readbytes(AS3514_I2C_ADDR, AS3514_UID_0, 0x10, serial);
    for (i = 0; i < 16; i++) {
        *p++ = hex[(serial[i] >> 4) & 0xF];
        *p++ = hex[(serial[i] >> 0) & 0xF];
    }
    usb_string_iSerial.bLength=68;
}
#else 
/* If we don't know the device serial number, use the one
 * from the disk */
static void set_serial_descriptor(void)
{
    short* p = &usb_string_iSerial.wString[1];
    unsigned short* identify = ata_get_identify();
    unsigned short x;
    int i;

    for (i = 10; i < 20; i++) {
       x = identify[i];
       *p++ = hex[(x >> 12) & 0xF];
       *p++ = hex[(x >> 8) & 0xF];
       *p++ = hex[(x >> 4) & 0xF];
       *p++ = hex[(x >> 0) & 0xF];
    }
    usb_string_iSerial.bLength=84;
}
#endif

void usb_core_init(void)
{
    int i;
    if (initialized)
        return;

    usb_drv_init();

    /* class driver init functions should be safe to call even if the driver
     * won't be used. This simplifies other logic (i.e. we don't need to know
     * yet which drivers will be enabled */
    for(i=0;i<USB_NUM_DRIVERS;i++) {
        if(drivers[i].enabled && drivers[i].init != NULL)
            drivers[i].init();
    }

    initialized = true;
    usb_state = DEFAULT;
    logf("usb_core_init() finished");
}

static int usb_charging_get_config_descriptor(unsigned char *dest,int max_packet_size,
                                            int interface_number,int endpoint)
{
    (void) max_packet_size;
    (void) endpoint;
    charging_interface_descriptor.bInterfaceNumber=interface_number;
    memcpy(dest,&charging_interface_descriptor,
           sizeof(struct usb_interface_descriptor));
    return sizeof(struct usb_interface_descriptor);
}

void usb_core_exit(void)
{
    int i;
    for(i=0;i<USB_NUM_DRIVERS;i++) {
        if(drivers[i].enabled && drivers[i].disconnect != NULL)
            drivers[i].disconnect ();
    }

    if (initialized) {
        usb_drv_exit();
    }
    initialized = false;
    logf("usb_core_exit() finished");
}

void usb_core_handle_transfer_completion(
              struct usb_transfer_completion_event_data* event)
{
    int i;
    switch(event->endpoint) {
        case EP_CONTROL:
            logf("ctrl handled %ld",current_tick);
            usb_core_control_request_handler(
                                    (struct usb_ctrlrequest*)event->data);
            break;
        default:
            for(i=0;i<USB_NUM_DRIVERS;i++) {
                if(drivers[i].enabled &&
                   drivers[i].usb_endpoint == event->endpoint &&
                   drivers[i].transfer_complete != NULL)
                {
                    drivers[i].transfer_complete(event->in,
                                              event->status,event->length);
                    break;
                }
            }
            break;
    }
}

void usb_core_enable_driver(int driver,bool enabled)
{
    drivers[driver].enabled = enabled;
}

bool usb_core_driver_enabled(int driver)
{
    return drivers[driver].enabled;
}

#ifdef HAVE_HOTSWAP
void usb_core_hotswap_event(int volume,bool inserted)
{
    int i;
    for(i=0;i<USB_NUM_DRIVERS;i++) {
        if(drivers[i].enabled &&
           drivers[i].notify_hotswap!=NULL)
        {
            drivers[i].notify_hotswap(volume,inserted);
        }
    }
}
#endif

static void usb_core_set_serial_function_id(void)
{
    int id = 0;
    int i;
    for(i=0;i<USB_NUM_DRIVERS;i++) {
        if(drivers[i].enabled)
            id |= 1<<i;
    }
    usb_string_iSerial.wString[0] = hex[id];
}

static void allocate_interfaces_and_endpoints(void)
{
    int i;
    int interface=0;
    int endpoint=1;
    for(i=0;i<USB_NUM_DRIVERS;i++) {
        if(drivers[i].enabled) {
            drivers[i].usb_endpoint = endpoint++;
            drivers[i].usb_interface = interface++;
        }
        if(endpoint>NUM_ENDPOINTS) {
            drivers[i].enabled = false;
        }
    }
    usb_core_num_interfaces = interface;
}

static void usb_core_control_request_handler(struct usb_ctrlrequest* req)
{
    int i;
    if(usb_state == DEFAULT) {
        set_serial_descriptor();
        usb_core_set_serial_function_id();

        allocate_interfaces_and_endpoints();

        for(i=0;i<USB_NUM_DRIVERS;i++) {
            if(drivers[i].enabled &&
               drivers[i].needs_exclusive_ata) {
                usb_request_exclusive_ata();
                break;
            }
        }
    }

    switch(req->bRequestType & 0x1f) {
        case 0: /* Device */
            switch (req->bRequest) {
                case USB_REQ_GET_CONFIGURATION: {
                    logf("usb_core: GET_CONFIG");
                    if (usb_state == ADDRESS)
                        response_data[0] = 0;
                    else
                        response_data[0] = 1;
                    if(usb_drv_send(EP_CONTROL, response_data, 1)!= 0)
                        break;
                    ack_control(req);
                    break;
                case USB_REQ_SET_CONFIGURATION:
                    logf("usb_core: SET_CONFIG");
                    usb_drv_cancel_all_transfers();
                    if (req->wValue) {
                        usb_state = CONFIGURED;
                        for(i=0;i<USB_NUM_DRIVERS;i++) {
                            if(drivers[i].enabled &&
                               drivers[i].init_connection!=NULL)
                            {
                                drivers[i].init_connection(
                                            drivers[i].usb_interface,
                                            drivers[i].usb_endpoint);
                            }
                        }
                    }
                    else {
                        usb_state = ADDRESS;
                    }
                    ack_control(req);
                    break;
                }
                case USB_REQ_SET_ADDRESS: {
                    unsigned char address = req->wValue;
                    logf("usb_core: SET_ADR %d", address);
                    if(ack_control(req)!=0)
                        break;
                    usb_drv_cancel_all_transfers();
                    usb_address = address;
                    usb_drv_set_address(usb_address);
                    usb_state = ADDRESS;
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
                            size = sizeof(struct usb_device_descriptor);
                            break;

                        case USB_DT_OTHER_SPEED_CONFIG:
                        case USB_DT_CONFIG: {
                            int max_packet_size;

                            if(req->wValue >> 8 == USB_DT_CONFIG) {
                                if(usb_drv_port_speed())
                                    max_packet_size=512;
                                else
                                    max_packet_size=64;
                                config_descriptor.bDescriptorType=USB_DT_CONFIG;
                            }
                            else {
                                if(usb_drv_port_speed())
                                    max_packet_size=64;
                                else
                                    max_packet_size=512;
                                config_descriptor.bDescriptorType =
                                                 USB_DT_OTHER_SPEED_CONFIG;
                            }
                            size = sizeof(struct usb_config_descriptor);

                            for(i=0;i<USB_NUM_DRIVERS;i++) {
                                if(drivers[i].enabled &&
                                   drivers[i].get_config_descriptor)
                                {
                                    size+=drivers[i].get_config_descriptor(
                                             &response_data[size],
                                             max_packet_size,
                                             drivers[i].usb_interface,
                                             drivers[i].usb_endpoint);
                                }
                            }
                            config_descriptor.bNumInterfaces =
                                                     usb_core_num_interfaces;
                            config_descriptor.wTotalLength = size;
                            memcpy(&response_data[0],&config_descriptor,
                                   sizeof(struct usb_config_descriptor));

                            ptr = response_data;
                            break;
                        }

                        case USB_DT_STRING:
                            logf("STRING %d",index);
                            if ((unsigned)index < (sizeof(usb_strings)/
                                         sizeof(struct usb_string_descriptor*)))
                            {
                                size = usb_strings[index]->bLength;
                                ptr = usb_strings[index];
                            }
                            else {
                                logf("bad string id %d", index);
                                usb_drv_stall(EP_CONTROL, true,true);
                            }
                            break;

                        case USB_DT_DEVICE_QUALIFIER:
                            ptr = &qualifier_descriptor;
                            size = sizeof (struct usb_qualifier_descriptor);
                            break;

                        default:
                            logf("bad desc %d", req->wValue >> 8);
                            usb_drv_stall(EP_CONTROL, true,true);
                            break;
                    }

                    if (ptr) {
                        length = MIN(size, length);

                        if (ptr != response_data) {
                            memcpy(response_data, ptr, length);
                        }

                        if(usb_drv_send(EP_CONTROL, response_data, length)!=0)
                            break;
                    }
                    ack_control(req);
                    break;
                } /* USB_REQ_GET_DESCRIPTOR */
                case USB_REQ_CLEAR_FEATURE:
                    break;
                case USB_REQ_SET_FEATURE:
                    if(req->wValue == 2) { /* TEST_MODE */
                        int mode=req->wIndex>>8;
                        ack_control(req);
                        usb_drv_set_test_mode(mode);
                    }
                    break;
                case USB_REQ_GET_STATUS:
                    response_data[0]= 0;
                    response_data[1]= 0;
                    if(usb_drv_send(EP_CONTROL, response_data, 2)!=0)
                        break;
                    ack_control(req);
                    break;
                default:
                    break;
            }
            break;
        case 1: /* Interface */
            switch (req->bRequest) {
                case USB_REQ_SET_INTERFACE:
                    logf("usb_core: SET_INTERFACE");
                    ack_control(req);
                    break;

                case USB_REQ_GET_INTERFACE:
                    logf("usb_core: GET_INTERFACE");
                    response_data[0] = 0;
                    if(usb_drv_send(EP_CONTROL, response_data, 1)!=0)
                        break;
                    ack_control(req);
                    break;
                case USB_REQ_CLEAR_FEATURE:
                    break;
                case USB_REQ_SET_FEATURE:
                    break;
                case USB_REQ_GET_STATUS:
                    response_data[0]= 0;
                    response_data[1]= 0;
                    if(usb_drv_send(EP_CONTROL, response_data, 2)!=0)
                        break;
                    ack_control(req);
                    break;
                default: {
                    bool handled=false;
                    for(i=0;i<USB_NUM_DRIVERS;i++) {
                        if(drivers[i].enabled &&
                           drivers[i].control_request &&
                           drivers[i].usb_interface == (req->wIndex))
                        {
                            handled = drivers[i].control_request(req);
                        }
                    }
                    if(!handled) {
                        /* nope. flag error */
                        logf("usb bad req %d", req->bRequest);
                        usb_drv_stall(EP_CONTROL, true,true);
                        ack_control(req);
                    }
                    break;
                }
            }
            break;
        case 2: /* Endpoint */
            switch (req->bRequest) {
                case USB_REQ_CLEAR_FEATURE:
                    if (req->wValue == 0 ) /* ENDPOINT_HALT */
                        usb_drv_stall(req->wIndex & 0xf, false,
                                      (req->wIndex & 0x80) !=0);
                    ack_control(req);
                    break;
                case USB_REQ_SET_FEATURE:
                    if (req->wValue == 0 ) /* ENDPOINT_HALT */
                        usb_drv_stall(req->wIndex & 0xf, true,
                                      (req->wIndex & 0x80) !=0);
                    ack_control(req);
                    break;
                case USB_REQ_GET_STATUS:
                    response_data[0]= 0;
                    response_data[1]= 0;
                    logf("usb_core: GET_STATUS"); 
                    if(req->wIndex>0)
                        response_data[0] = usb_drv_stalled(req->wIndex&0xf,
                                                       (req->wIndex&0x80)!=0);
                    if(usb_drv_send(EP_CONTROL, response_data, 2)!=0)
                        break;
                    ack_control(req);
                    break;
                default: {
                    bool handled=false;
                    for(i=0;i<USB_NUM_DRIVERS;i++) {
                        if(drivers[i].enabled &&
                           drivers[i].control_request &&
                           drivers[i].usb_endpoint == (req->wIndex & 0xf))
                        {
                            handled = drivers[i].control_request(req);
                        }
                    }
                    if(!handled) {
                        /* nope. flag error */
                        logf("usb bad req %d", req->bRequest);
                        usb_drv_stall(EP_CONTROL, true,true);
                        ack_control(req);
                    }
                    break;
                }
            }
    }
    logf("control handled");
}

/* called by usb_drv_int() */
void usb_core_bus_reset(void)
{
    usb_address = 0;
    usb_state = DEFAULT;
}

/* called by usb_drv_transfer_completed() */
void usb_core_transfer_complete(int endpoint, bool in, int status,int length)
{
    switch (endpoint) {
        case EP_CONTROL:
            /* already handled */
            break;

        default:
            events[endpoint].endpoint=endpoint;
            events[endpoint].in=in;
            events[endpoint].data=0;
            events[endpoint].status=status;
            events[endpoint].length=length;
            /* All other endoints. Let the thread deal with it */
            usb_signal_transfer_completion(&events[endpoint]);
            break;
    }
}

/* called by usb_drv_int() */
void usb_core_control_request(struct usb_ctrlrequest* req)
{
    events[0].endpoint=0;
    events[0].in=0;
    events[0].data=(void *)req;
    events[0].status=0;
    events[0].length=0;
    logf("ctrl received %ld",current_tick);
    usb_signal_transfer_completion(&events[0]);
}

static int ack_control(struct usb_ctrlrequest* req)
{
    if (req->bRequestType & 0x80)
        return usb_drv_recv(EP_CONTROL, NULL, 0);
    else
        return usb_drv_send(EP_CONTROL, NULL, 0);
}

