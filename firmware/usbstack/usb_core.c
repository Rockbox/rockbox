/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:  $
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

#if defined(USB_STORAGE)
#include "usb_storage.h"
#endif

#if defined(USB_SERIAL)
#include "usb_serial.h"
#endif

/* TODO: Move this target-specific stuff somewhere else (serial number reading) */

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

static const struct usb_device_descriptor __attribute__((aligned(2))) device_descriptor= {
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

struct usb_config_descriptor __attribute__((aligned(2))) config_descriptor = 
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
struct usb_interface_descriptor __attribute__((aligned(2))) charging_interface_descriptor =
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

#ifdef USB_STORAGE
/* storage interface */
struct usb_interface_descriptor __attribute__((aligned(2))) mass_storage_interface_descriptor = 
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
};

struct usb_endpoint_descriptor __attribute__((aligned(2))) mass_storage_ep_in_descriptor =
{
    .bLength          = sizeof(struct usb_endpoint_descriptor),
    .bDescriptorType  = USB_DT_ENDPOINT,
    .bEndpointAddress = EP_MASS_STORAGE | USB_DIR_IN,
    .bmAttributes     = USB_ENDPOINT_XFER_BULK,
    .wMaxPacketSize   = 16,
    .bInterval        = 0
};
struct usb_endpoint_descriptor __attribute__((aligned(2))) mass_storage_ep_out_descriptor =
{
    .bLength          = sizeof(struct usb_endpoint_descriptor),
    .bDescriptorType  = USB_DT_ENDPOINT,
    .bEndpointAddress = EP_MASS_STORAGE | USB_DIR_OUT,
    .bmAttributes     = USB_ENDPOINT_XFER_BULK,
    .wMaxPacketSize   = 16,
    .bInterval        = 0
};
#endif

#ifdef USB_SERIAL
/* serial interface */
struct usb_interface_descriptor __attribute__((aligned(2))) serial_interface_descriptor =
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
};

struct usb_endpoint_descriptor __attribute__((aligned(2))) serial_ep_in_descriptor =
{
    .bLength          = sizeof(struct usb_endpoint_descriptor),
    .bDescriptorType  = USB_DT_ENDPOINT,
    .bEndpointAddress = EP_SERIAL | USB_DIR_IN,
    .bmAttributes     = USB_ENDPOINT_XFER_BULK,
    .wMaxPacketSize   = 16,
    .bInterval        = 0
};
struct usb_endpoint_descriptor __attribute__((aligned(2))) serial_ep_out_descriptor =
{
    .bLength          = sizeof(struct usb_endpoint_descriptor),
    .bDescriptorType  = USB_DT_ENDPOINT,
    .bEndpointAddress = EP_SERIAL | USB_DIR_OUT,
    .bmAttributes     = USB_ENDPOINT_XFER_BULK,
    .wMaxPacketSize   = 16,
    .bInterval        = 0
};
#endif

static const struct usb_qualifier_descriptor __attribute__((aligned(2))) qualifier_descriptor =
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

static struct usb_string_descriptor __attribute__((aligned(2))) usb_string_iManufacturer =
{
    24,
    USB_DT_STRING,
    {'R','o','c','k','b','o','x','.','o','r','g'}
};

static struct usb_string_descriptor __attribute__((aligned(2))) usb_string_iProduct =
{
    42,
    USB_DT_STRING,
    {'R','o','c','k','b','o','x',' ','m','e','d','i','a',' ','p','l','a','y','e','r'}
};

static struct usb_string_descriptor __attribute__((aligned(2))) usb_string_iSerial =
{
    84,
    USB_DT_STRING,
    {'0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0',
     '0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0',
     '0','0','0','0','0','0','0','0','0'}
};

/* Generic for all targets */

/* this is stringid #0: languages supported */
static struct usb_string_descriptor __attribute__((aligned(2))) lang_descriptor =
{
    4,
    USB_DT_STRING,
    {0x0409} /* LANGID US English */
};

static struct usb_string_descriptor __attribute__((aligned(2))) usb_string_charging_only =
{
    28,
    USB_DT_STRING,
    {'C','h','a','r','g','i','n','g',' ','o','n','l','y'}
};

static struct usb_string_descriptor* usb_strings[] =
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

#ifdef USB_STORAGE
static bool usb_core_storage_enabled = false;
#endif
/* Next one is non-static, to enable setting it from the debug menu */
bool usb_core_serial_enabled = false;
#ifdef USB_CHARGING_ONLY
static bool usb_core_charging_enabled = false;
#endif

static void usb_core_control_request_handler(struct usb_ctrlrequest* req);
static int ack_control(struct usb_ctrlrequest* req);

static unsigned char *response_data;
static unsigned char __response_data[CACHEALIGN_UP(256)] CACHEALIGN_ATTR;

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

    for (i = 0; i < 2; i++)
    {
       x = serial[i];
       for (j=0;j<8;j++)
       {
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

    i2c_readbytes(AS3514_I2C_ADDR, 0x30, 0x10, serial);
    for (i = 0; i < 16; i++)
    {
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

    for (i = 10; i < 20; i++)
    {
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
    if (initialized)
        return;

    response_data = (void*)UNCACHED_ADDR(&__response_data);

    usb_drv_init();

    /* class driver init functions should be safe to call even if the driver
     * won't be used. This simplifies other logic (i.e. we don't need to know
     * yet which drivers will be enabled */
#ifdef USB_STORAGE
    if(usb_core_storage_enabled)
        usb_storage_init();
#endif

#ifdef USB_SERIAL
    if(usb_core_serial_enabled)
        usb_serial_init();
#endif

    initialized = true;
    usb_state = DEFAULT;
    logf("usb_core_init() finished");
}

void usb_core_exit(void)
{
#ifdef USB_SERIAL
    if(usb_core_serial_enabled)
        usb_serial_exit();
#endif

    if (initialized) {
        usb_drv_exit();
    }
    initialized = false;
    logf("usb_core_exit() finished");
}

void usb_core_handle_transfer_completion(struct usb_transfer_completion_event_data* event)
{
    switch(event->endpoint) {
        case EP_CONTROL:
            logf("ctrl handled %ld",current_tick);
            usb_core_control_request_handler((struct usb_ctrlrequest*)event->data);
            break;
#ifdef USB_STORAGE
        case EP_MASS_STORAGE:
            usb_storage_transfer_complete(event->in,event->status,event->length);
            break;
#endif
#ifdef USB_SERIAL
        case EP_SERIAL:
            usb_serial_transfer_complete(event->in,event->status,event->length);
            break;
#endif
#ifdef USB_CHARGING_ONLY
        case EP_CHARGING_ONLY:
            break;
#endif
    }
}

void usb_core_enable_protocol(int driver,bool enabled)
{
    switch(driver) {
#ifdef USB_STORAGE
	case USB_DRIVER_MASS_STORAGE:
            usb_core_storage_enabled = enabled;
            break;
#endif
#ifdef USB_SERIAL
	case USB_DRIVER_SERIAL:
            usb_core_serial_enabled = enabled;
            break;
#endif
#ifdef USB_CHARGING_ONLY
	case USB_DRIVER_CHARGING_ONLY:
            usb_core_charging_enabled = enabled;
            break;
#endif
    }
}

static void usb_core_control_request_handler(struct usb_ctrlrequest* req)
{
    if(usb_state == DEFAULT) {
        set_serial_descriptor();
        
        int serial_function_id = 0;
#ifdef USB_STORAGE
        if(usb_core_storage_enabled) {
            usb_request_exclusive_ata();
            serial_function_id |= 1;
        }
#endif
#ifdef USB_SERIAL
        if(usb_core_serial_enabled)
            serial_function_id |= 2;
#endif
        usb_string_iSerial.wString[0] = hex[serial_function_id];
    }

    switch (req->bRequest) {
        case USB_REQ_SET_CONFIGURATION:
            logf("usb_core: SET_CONFIG");
            usb_drv_cancel_all_transfers();
            if (req->wValue){
                usb_state = CONFIGURED;
#ifdef USB_STORAGE
                if(usb_core_storage_enabled)
                    usb_storage_control_request(req);
#endif

#ifdef USB_SERIAL
                if(usb_core_serial_enabled)
                    usb_serial_control_request(req);
#endif
            }
            else {
                usb_state = ADDRESS;
            }
            ack_control(req);
            break;

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
        }

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
            logf("usb_core: CLEAR_FEATURE");
            if (req->wValue)
                usb_drv_stall(req->wIndex & 0xf, false,(req->wIndex & 0x80) !=0);
            else
                usb_drv_stall(req->wIndex & 0xf, false,(req->wIndex & 0x80) !=0);
            ack_control(req);
            break;

        case USB_REQ_SET_FEATURE:
            logf("usb_core: SET_FEATURE");
            switch(req->bRequestType & 0x0f){
                case 0: /* Device */
                    if(req->wValue == 2) { /* TEST_MODE */
                        int mode=req->wIndex>>8;
                        ack_control(req);
                        usb_drv_set_test_mode(mode);
                    }
                    break;
                case 2: /* Endpoint */
                    if (req->wValue)
                        usb_drv_stall(req->wIndex & 0xf, true,(req->wIndex & 0x80) !=0);
                    else
                        usb_drv_stall(req->wIndex & 0xf, false,(req->wIndex & 0x80) !=0);
                    ack_control(req);
                    break;
                default:
                    break;
            }
            break;

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

        case USB_REQ_GET_STATUS: {
            response_data[0]= 0;
            response_data[1]= 0;
            logf("usb_core: GET_STATUS"); 
            if(req->wIndex>0) {
                if(usb_drv_stalled(req->wIndex&0xf,(req->wIndex&0x80)!=0))
                    response_data[0] = 1;
            }
            logf("usb_core: %X %X",response_data[0],response_data[1]); 
            if(usb_drv_send(EP_CONTROL, response_data, 2)!=0)
                break;
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
                    size = sizeof(struct usb_device_descriptor);
                    break;

                case USB_DT_OTHER_SPEED_CONFIG:
                case USB_DT_CONFIG: {
                    int max_packet_size;
                    int interface_number=0;

                    if(req->wValue >> 8 == USB_DT_CONFIG) {
                        if(usb_drv_port_speed()) {
                            max_packet_size=512;
                        }
                        else {
                            max_packet_size=64;
                        }
                        config_descriptor.bDescriptorType=USB_DT_CONFIG;
                    }
                    else {
                        if(usb_drv_port_speed()) {
                            max_packet_size=64;
                        }
                        else {
                            max_packet_size=512;
                        }
                        config_descriptor.bDescriptorType=USB_DT_OTHER_SPEED_CONFIG;
                    }
                    size = sizeof(struct usb_config_descriptor);

#ifdef USB_STORAGE
                    if(usb_core_storage_enabled){
                        mass_storage_ep_in_descriptor.wMaxPacketSize=max_packet_size;
                        mass_storage_ep_out_descriptor.wMaxPacketSize=max_packet_size;
                        mass_storage_interface_descriptor.bInterfaceNumber=interface_number;
                        interface_number++;

                        memcpy(&response_data[size],&mass_storage_interface_descriptor,sizeof(struct usb_interface_descriptor));
                        size += sizeof(struct usb_interface_descriptor);
                        memcpy(&response_data[size],&mass_storage_ep_in_descriptor,sizeof(struct usb_endpoint_descriptor));
                        size += sizeof(struct usb_endpoint_descriptor);
                        memcpy(&response_data[size],&mass_storage_ep_out_descriptor,sizeof(struct usb_endpoint_descriptor));
                        size += sizeof(struct usb_endpoint_descriptor);
                    }
#endif
#ifdef USB_SERIAL
                    if(usb_core_serial_enabled){
                        serial_ep_in_descriptor.wMaxPacketSize=max_packet_size;
                        serial_ep_out_descriptor.wMaxPacketSize=max_packet_size;
                        serial_interface_descriptor.bInterfaceNumber=interface_number;
                        interface_number++;

                        memcpy(&response_data[size],&serial_interface_descriptor,sizeof(struct usb_interface_descriptor));
                        size += sizeof(struct usb_interface_descriptor);
                        memcpy(&response_data[size],&serial_ep_in_descriptor,sizeof(struct usb_endpoint_descriptor));
                        size += sizeof(struct usb_endpoint_descriptor);
                        memcpy(&response_data[size],&serial_ep_out_descriptor,sizeof(struct usb_endpoint_descriptor));
                        size += sizeof(struct usb_endpoint_descriptor);
                    }
#endif
#ifdef USB_CHARGING_ONLY
                    if(usb_core_charging_enabled && interface_number == 0){
                        charging_interface_descriptor.bInterfaceNumber=interface_number;
                        interface_number++;
                        memcpy(&response_data[size],&charging_interface_descriptor,sizeof(struct usb_interface_descriptor));
                        size += sizeof(struct usb_interface_descriptor);
                    }
#endif
                    config_descriptor.bNumInterfaces=interface_number;
                    config_descriptor.wTotalLength = size;
                    memcpy(&response_data[0],&config_descriptor,sizeof(struct usb_config_descriptor));

                    ptr = response_data;
                    break;
                    }

                case USB_DT_STRING:
                    logf("STRING %d",index);
                    if ((unsigned)index < (sizeof(usb_strings)/sizeof(struct usb_string_descriptor*))) {
                        size = usb_strings[index]->bLength;
                        memcpy(&response_data[0],usb_strings[index],size);
                        ptr = response_data;
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
                if(usb_drv_send(EP_CONTROL, (void*)UNCACHED_ADDR(ptr), length)!=0)
                    break;
            }
            ack_control(req);
            break;
        } /* USB_REQ_GET_DESCRIPTOR */

        default: {
            bool handled=false;
            if((req->bRequestType & 0x1f) == 1) /* Interface */
            {
#ifdef USB_STORAGE
                /* does usb_storage know this request? */
                if(req->wIndex == mass_storage_interface_descriptor.bInterfaceNumber)
                {
                    handled = usb_storage_control_request(req);
                }
#endif

#ifdef USB_SERIAL
                /* does usb_serial know this request? */
                if(req->wIndex == serial_interface_descriptor.bInterfaceNumber)
                {
                    handled = usb_serial_control_request(req);
                }
                    
#endif
            }
            if(!handled)
            {
                /* nope. flag error */
                logf("usb bad req %d", req->bRequest);
                usb_drv_stall(EP_CONTROL, true,true);
                ack_control(req);
            }
            break;
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
#if defined(USB_CHARGING_ONLY) || defined(USB_STORAGE)
    (void)in;
#endif

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

