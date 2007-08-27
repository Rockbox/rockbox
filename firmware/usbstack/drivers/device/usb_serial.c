/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2007 by Christian Gmeiner
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "usb_serial.h"
#include <string.h>

static struct usb_dcd_controller_ops* ops;

struct usb_device_driver usb_serial_driver = {
    .name =         "serial",
    .bind =         usb_serial_driver_bind,
    .unbind =       NULL,
    .request =      usb_serial_driver_request,
    .suspend =      NULL,
    .resume =       NULL,
    .speed =        usb_serial_driver_speed,
};

/*-------------------------------------------------------------------------*/
/* usb descriptors */

/* TODO: implement strings */
#define GS_MANUFACTURER_STR_ID  0
#define GS_PRODUCT_STR_ID       0
#define GS_SERIAL_STR_ID        0
#define GS_BULK_CONFIG_STR_ID   0
#define GS_DATA_STR_ID          0

#define GS_BULK_CONFIG_ID       1

static struct usb_device_descriptor serial_device_desc = {
    .bLength =              USB_DT_DEVICE_SIZE,
    .bDescriptorType =      USB_DT_DEVICE,
    .bcdUSB =               0x0200,
    .bDeviceClass =         USB_CLASS_COMM,
    .bDeviceSubClass =      0,
    .bDeviceProtocol =      0,
    .idVendor =             0x0525,
    .idProduct =            0xa4a6,
    .iManufacturer =        GS_MANUFACTURER_STR_ID,
    .iProduct =             GS_PRODUCT_STR_ID,
    .iSerialNumber =        GS_SERIAL_STR_ID,
    .bNumConfigurations =   1,
};

static struct usb_config_descriptor serial_bulk_config_desc = {
    .bLength =              USB_DT_CONFIG_SIZE,
    .bDescriptorType =      USB_DT_CONFIG,

    .bNumInterfaces =       1,
    .bConfigurationValue =  GS_BULK_CONFIG_ID,
    .iConfiguration =       GS_BULK_CONFIG_STR_ID,
    .bmAttributes =         USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
    .bMaxPower =            1,
};

static struct usb_interface_descriptor serial_bulk_interface_desc = {
    .bLength =              USB_DT_INTERFACE_SIZE,
    .bDescriptorType =      USB_DT_INTERFACE,
    .bInterfaceNumber =     0,
    .bNumEndpoints =        2,
    .bInterfaceClass =      USB_CLASS_CDC_DATA,
    .bInterfaceSubClass =   0,
    .bInterfaceProtocol =   0,
    .iInterface =           GS_DATA_STR_ID,
};

static struct usb_endpoint_descriptor serial_fullspeed_in_desc = {
    .bLength =              USB_DT_ENDPOINT_SIZE,
    .bDescriptorType =      USB_DT_ENDPOINT,
    .bEndpointAddress =     USB_DIR_IN,
    .bmAttributes =         USB_ENDPOINT_XFER_BULK,
    .wMaxPacketSize =       8,
};

static struct usb_endpoint_descriptor serial_fullspeed_out_desc = {
    .bLength =              USB_DT_ENDPOINT_SIZE,
    .bDescriptorType =      USB_DT_ENDPOINT,
    .bEndpointAddress =     USB_DIR_OUT,
    .bmAttributes =         USB_ENDPOINT_XFER_BULK,
    .wMaxPacketSize =       8,
};

static struct usb_debug_descriptor serial_debug_desc = {
    .bLength =              sizeof(struct usb_debug_descriptor),
    .bDescriptorType =      USB_DT_DEBUG,
};

static struct usb_qualifier_descriptor serial_qualifier_desc = {
    .bLength =             sizeof(struct usb_qualifier_descriptor),
    .bDescriptorType =     USB_DT_DEVICE_QUALIFIER,
    .bcdUSB =              0x0200,
    .bDeviceClass =        USB_CLASS_COMM,
    .bNumConfigurations =  1,
};

struct usb_descriptor_header *serial_bulk_fullspeed_function[] = {
    (struct usb_descriptor_header *) &serial_bulk_interface_desc,
    (struct usb_descriptor_header *) &serial_fullspeed_in_desc,
    (struct usb_descriptor_header *) &serial_fullspeed_out_desc,
    NULL,
};

#define BUFFER_SIZE 100
uint8_t buf[BUFFER_SIZE];

struct usb_response res;

/* helper functions */
static int config_buf(uint8_t *buf, uint8_t type, unsigned index);
static int set_config(int config);


struct device {
    struct usb_ep* in;
    struct usb_ep* out;
    uint32_t used_config;
};

static struct device dev;

/*-------------------------------------------------------------------------*/

void usb_serial_driver_init(void)
{
    logf("usb serial: register");
    usb_device_driver_register(&usb_serial_driver);
}

/*-------------------------------------------------------------------------*/

void usb_serial_driver_bind(void* controler_ops)
{
    logf("usb serial: bind");
    ops = controler_ops;

    /* serach and asign endpoints */
    usb_ep_autoconfig_reset();

    dev.in = usb_ep_autoconfig(&serial_fullspeed_in_desc);
    if (!dev.in) {
        goto autoconf_fail;
    }
    dev.in->claimed = true;
    logf("usb serial: in: %s", dev.in->name);

    dev.out = usb_ep_autoconfig(&serial_fullspeed_out_desc);
    if (!dev.out) {
        goto autoconf_fail;
    }
    dev.out->claimed = true;
    logf("usb serial: out: %s", dev.out->name);

    /* update device decsriptor */
    serial_device_desc.bMaxPacketSize0 = ops->ep0->maxpacket;

    /* update qualifie descriptor */
    serial_qualifier_desc.bMaxPacketSize0 = ops->ep0->maxpacket;

    /* update debug descriptor */
    serial_debug_desc.bDebugInEndpoint = dev.in->ep_num;
    serial_debug_desc.bDebugOutEndpoint = dev.out->ep_num;

    return;

autoconf_fail:
    logf("failed to find endpoiunts");
}

int usb_serial_driver_request(struct usb_ctrlrequest* request)
{
    int ret = -EOPNOTSUPP;
    logf("usb serial: request");

    res.length = 0;
    res.buf = NULL;

    switch (request->bRequestType & USB_TYPE_MASK) {
    case USB_TYPE_STANDARD:

        switch (request->bRequest) {
        case USB_REQ_GET_DESCRIPTOR:

            switch (request->wValue >> 8) {
            case USB_DT_DEVICE:
                logf("usb serial: sending device desc");
                ret = MIN(sizeof(struct usb_device_descriptor), request->wLength);
                res.buf = &serial_device_desc;
                break;

            case USB_DT_DEVICE_QUALIFIER:
                logf("usb serial: sending qualifier dec");
                ret = MIN(sizeof(struct usb_qualifier_descriptor), request->wLength);
                res.buf = &serial_qualifier_desc;

            case USB_DT_CONFIG:
                logf("usb serial: sending config desc");

                ret = config_buf(buf, request->wValue >> 8, request->wValue & 0xff);
                if (ret >= 0) {
                    logf("%d, vs %d", request->wLength, ret);
                    ret = MIN(request->wLength, (uint16_t)ret);
                }
                res.buf = buf;
                break;

            case USB_DT_DEBUG:
                logf("usb serial: sending debug desc");
                ret = MIN(sizeof(struct usb_debug_descriptor), request->wLength);
                res.buf = &serial_debug_desc;
                break;
            }
            break;

        case USB_REQ_SET_CONFIGURATION:
            logf("usb serial: set configuration %d", request->wValue);
            ret = set_config(request->wValue);
            break;

        case USB_REQ_GET_CONFIGURATION:
            logf("usb serial: get configuration");
            ret = 1;
            res.buf = &dev.used_config;
            break;
        }
    }

    if (ret >= 0) {
        res.length = ret;
        ret = ops->send(NULL, &res);
    }

    return ret;
}

void usb_serial_driver_speed(enum usb_device_speed speed)
{
    switch (speed) {
    case USB_SPEED_LOW:
    case USB_SPEED_FULL:
        logf("usb serial: using fullspeed");
        break;
    case USB_SPEED_HIGH:
        logf("usb serial: using highspeed");
        break;
    default:
        logf("speed: hmm");
        break;
    }
}

/*-------------------------------------------------------------------------*/
/* helper functions */

static int config_buf(uint8_t *buf, uint8_t type, unsigned index)
{
    int len;

    /* TODO check index*/

    len = usb_stack_configdesc(&serial_bulk_config_desc, buf, BUFFER_SIZE, serial_bulk_fullspeed_function);
    if (len < 0) {
        return len;
    }
    ((struct usb_config_descriptor *)buf)->bDescriptorType = type;
    return len;
}

static int set_config(int config)
{
    /* TODO check config*/

    /* enable endpoints */
    logf("setup %s", dev.in->name);
    ops->enable(dev.in);
    logf("setup %s", dev.out->name);
    ops->enable(dev.out);

    /* store config */
    logf("using config %d", config);
    dev.used_config = config;

    return 0;
}
