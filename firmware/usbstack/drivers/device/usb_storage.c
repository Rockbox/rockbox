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

#include "usb_storage.h"
#include <string.h>

/*-------------------------------------------------------------------------*/

static struct usb_dcd_controller_ops* ops;

struct usb_device_driver usb_storage_driver = {
    .name =         "storage",
    .bind =         usb_storage_driver_bind,
    .unbind =       NULL,
    .request =      usb_storage_driver_request,
    .suspend =      NULL,
    .resume =       NULL,
    .speed =        NULL,
};

struct device {
    struct usb_ep* in;
    struct usb_ep* out;
    struct usb_ep* intr;
};

static struct device dev;

/*-------------------------------------------------------------------------*/

#define PROTO_BULK 0x50    // Bulk only
#define SUBCL_SCSI 0x06    // Transparent SCSI

/* Bulk-only class specific requests */
#define USB_BULK_RESET_REQUEST          0xff
#define USB_BULK_GET_MAX_LUN_REQUEST    0xfe

/*-------------------------------------------------------------------------*/
/* usb descriptors */

static struct usb_device_descriptor storage_device_desc = {
    .bLength =              USB_DT_DEVICE_SIZE,
    .bDescriptorType =      USB_DT_DEVICE,
    .bcdUSB =               0x0200,
    .bDeviceClass =         0,
    .bDeviceSubClass =      0,
    .bDeviceProtocol =      0,
    .bMaxPacketSize0 =      64,
    .idVendor =             0xffff,
    .idProduct =            0x0001,
    .iManufacturer =        0,
    .iProduct =             0,
    .iSerialNumber =        0,
    .bNumConfigurations =   1,
};

static struct usb_config_descriptor storage_config_desc = {
    .bLength =              USB_DT_CONFIG_SIZE,
    .bDescriptorType =      USB_DT_CONFIG,

    .bNumInterfaces =       1,
    .bConfigurationValue =  1,
    .iConfiguration =       0,
    .bmAttributes =         USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
    .bMaxPower =            1,
};

static struct usb_interface_descriptor storage_interface_desc = {
    .bLength =              USB_DT_INTERFACE_SIZE,
    .bDescriptorType =      USB_DT_INTERFACE,
    .bInterfaceNumber =     0,
    .bNumEndpoints =        3,
    .bInterfaceClass =      USB_CLASS_MASS_STORAGE,
    .bInterfaceSubClass =   SUBCL_SCSI,
    .bInterfaceProtocol =   PROTO_BULK,
    .iInterface =           0,
};

/* endpoint I -> bulk in */
static struct usb_endpoint_descriptor storage_bulk_in_desc = {
    .bLength =              USB_DT_ENDPOINT_SIZE,
    .bDescriptorType =      USB_DT_ENDPOINT,
    .bEndpointAddress =     USB_DIR_IN,
    .bmAttributes =         USB_ENDPOINT_XFER_BULK,
    .wMaxPacketSize =       512,
};

/* endpoint II -> bulk out */
static struct usb_endpoint_descriptor storage_bulk_out_desc = {
    .bLength =              USB_DT_ENDPOINT_SIZE,
    .bDescriptorType =      USB_DT_ENDPOINT,
    .bEndpointAddress =     USB_DIR_OUT,
    .bmAttributes =         USB_ENDPOINT_XFER_BULK,
    .wMaxPacketSize =       512,
};

struct usb_descriptor_header *storage_fullspeed_function[] = {
    (struct usb_descriptor_header *) &storage_interface_desc,
    (struct usb_descriptor_header *) &storage_bulk_in_desc,
    (struct usb_descriptor_header *) &storage_bulk_out_desc,
    NULL,
};

#define BUFFER_SIZE 100
uint8_t buf[BUFFER_SIZE];

struct usb_response res;

/* helper functions */
static int config_buf(uint8_t *buf, uint8_t type, unsigned index);
static int set_config(int config);

/*-------------------------------------------------------------------------*/

void usb_storage_driver_init(void)
{
    logf("usb storage: register");
    usb_device_driver_register(&usb_storage_driver);
}

/*-------------------------------------------------------------------------*/
/* device driver ops */

void usb_storage_driver_bind(void* controler_ops)
{
    ops = controler_ops;

    /* serach and asign endpoints */
    usb_ep_autoconfig_reset();

    dev.in = usb_ep_autoconfig(&storage_bulk_in_desc);
    if (!dev.in) {
        goto autoconf_fail;
    }
    dev.in->claimed = true;
    logf("usb storage: in: %s", dev.in->name);

    dev.out = usb_ep_autoconfig(&storage_bulk_out_desc);
    if (!dev.out) {
        goto autoconf_fail;
    }
    dev.out->claimed = true;
    logf("usb storage: out: %s", dev.out->name);

    return;

autoconf_fail:
    logf("failed to find endpoints");
}

int usb_storage_driver_request(struct usb_ctrlrequest* request)
{
    int ret = -EOPNOTSUPP;
    logf("usb storage: request");

    res.length = 0;
    res.buf = NULL;

    switch (request->bRequestType & USB_TYPE_MASK) {
    case USB_TYPE_STANDARD:

        switch (request->bRequest) {
        case USB_REQ_GET_DESCRIPTOR:

            switch (request->wValue >> 8) {
            case USB_DT_DEVICE:
                logf("usb storage: sending device desc");
                ret = MIN(sizeof(struct usb_device_descriptor), request->wLength);
                res.buf = &storage_device_desc;
                break;

            case USB_DT_CONFIG:
                logf("usb storage: sending config desc");

                ret = config_buf(buf, request->wValue >> 8, request->wValue & 0xff);
                if (ret >= 0) {
                    logf("%d, vs %d", request->wLength, ret);
                    ret = MIN(request->wLength, (uint16_t)ret);
                }
                res.buf = buf;
                break;
            }
            break;

        case USB_REQ_SET_CONFIGURATION:
            logf("usb storage: set configuration %d", request->wValue);
            ret = set_config(request->wValue);
            break;

        case USB_REQ_SET_INTERFACE:
            logf("usb storage: set interface");
            ret = 0;
            break;
        }

    case USB_TYPE_CLASS:

        switch (request->bRequest) {
        case USB_BULK_RESET_REQUEST:
            logf("usb storage: bulk reset");
            break;

        case USB_BULK_GET_MAX_LUN_REQUEST:
            logf("usb storage: get max lun");
            /* we support no LUNs (Logical Unit Number) */
            buf[0] = 0;
            ret = 1;
            break;
        }
        break;
    }

    if (ret >= 0) {
        res.length = ret;
        ret = ops->send(NULL, &res);
    }

    return ret;
}

/*-------------------------------------------------------------------------*/
/* S/GET CONFIGURATION helpers */

static int config_buf(uint8_t *buf, uint8_t type, unsigned index)
{
    int len;

    /* only one configuration */
    if (index != 0) {
        return -EINVAL;
    }

    len = usb_stack_configdesc(&storage_config_desc, buf, BUFFER_SIZE, storage_fullspeed_function);
    if (len < 0) {
        return len;
    }
    ((struct usb_config_descriptor *)buf)->bDescriptorType = type;
    return len;
}

static int set_config(int config)
{
    /* enable endpoints */
    logf("setup %s", dev.in->name);
    ops->enable(dev.in);
    logf("setup %s", dev.out->name);
    ops->enable(dev.out);

    /* setup buffers */

    return 0;
}

