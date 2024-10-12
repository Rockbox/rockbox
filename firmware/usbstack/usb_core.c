/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Björn Stenberg
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
#include "system.h"
#include "thread.h"
#include "kernel.h"
#include "string.h"
#include "panic.h"
/*#define LOGF_ENABLE*/
#include "logf.h"

#include "usb.h"
#include "usb_ch9.h"
#include "usb_drv.h"
#include "usb_core.h"
#include "usb_class_driver.h"

#if defined(USB_ENABLE_STORAGE)
#include "usb_storage.h"
#endif

#if defined(USB_ENABLE_SERIAL)
#include "usb_serial.h"
#endif

#if defined(USB_ENABLE_CHARGING_ONLY)
#include "usb_charging_only.h"
#endif

#ifdef USB_ENABLE_HID
#include "usb_hid.h"
#endif

/* TODO: Move target-specific stuff somewhere else (serial number reading) */

#if defined(IPOD_ARCH) && defined(CPU_PP)
// no need to include anything
#elif defined(HAVE_AS3514)
#include "ascodec.h"
#include "as3514.h"
#elif (CONFIG_CPU == IMX233) && IMX233_SUBTARGET >= 3700
#include "ocotp-imx233.h"
#elif defined(SANSA_CONNECT)
#include "cryptomem-sansaconnect.h"
#elif (CONFIG_STORAGE & STORAGE_ATA)
#include "ata.h"
#endif

#ifndef USB_MAX_CURRENT
#define USB_MAX_CURRENT 500
#endif

/*-------------------------------------------------------------------------*/
/* USB protocol descriptors: */

static struct usb_device_descriptor __attribute__((aligned(2)))
                                          device_descriptor=
{
    .bLength            = sizeof(struct usb_device_descriptor),
    .bDescriptorType    = USB_DT_DEVICE,
#ifndef USB_NO_HIGH_SPEED
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
    .iManufacturer      = USB_STRING_INDEX_MANUFACTURER,
    .iProduct           = USB_STRING_INDEX_PRODUCT,
    .iSerialNumber      = USB_STRING_INDEX_SERIAL,
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
    .bMaxPower           = (USB_MAX_CURRENT + 1) / 2, /* In 2mA units */
};

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

static const struct usb_string_descriptor usb_string_iManufacturer =
USB_STRING_INITIALIZER(u"Rockbox.org");

static const struct usb_string_descriptor usb_string_iProduct =
USB_STRING_INITIALIZER(u"Rockbox media player");

static struct usb_string_descriptor usb_string_iSerial =
USB_STRING_INITIALIZER(u"00000000000000000000000000000000000000000");

/* Generic for all targets */

/* this is stringid #0: languages supported */
static const struct usb_string_descriptor __attribute__((aligned(2)))
                                    lang_descriptor =
USB_STRING_INITIALIZER(u"\x0409"); /* LANGID US English */

static const struct usb_string_descriptor* const usb_strings[USB_STRING_INDEX_MAX] =
{
    [USB_STRING_INDEX_LANGUAGE] = &lang_descriptor,
    [USB_STRING_INDEX_MANUFACTURER] = &usb_string_iManufacturer,
    [USB_STRING_INDEX_PRODUCT] = &usb_string_iProduct,
    [USB_STRING_INDEX_SERIAL] = &usb_string_iSerial,
};

static int usb_address = 0;
static bool initialized = false;
static bool drivers_connected = false;
static enum { DEFAULT, ADDRESS, CONFIGURED } usb_state;

#ifdef HAVE_USB_CHARGING_ENABLE
static int usb_charging_mode = USB_CHARGING_DISABLE;
static int usb_charging_current_requested = 500;
static struct timeout usb_no_host_timeout;
static bool usb_no_host = false;

static int usb_no_host_callback(struct timeout *tmo)
{
    (void)tmo;
    usb_no_host = true;
    usb_charger_update();
    return 0;
}
#endif

static int usb_core_num_interfaces;

typedef void (*completion_handler_t)(int ep, int dir, int status, int length);
typedef bool (*control_handler_t)(struct usb_ctrlrequest* req, void* reqdata,
    unsigned char* dest);

static struct
{
    completion_handler_t completion_handler[2];
    control_handler_t control_handler[2];
    struct usb_transfer_completion_event_data completion_event[2];
} ep_data[USB_NUM_ENDPOINTS];

static struct usb_class_driver drivers[USB_NUM_DRIVERS] =
{
#ifdef USB_ENABLE_STORAGE
    [USB_DRIVER_MASS_STORAGE] = {
        .enabled = false,
        .needs_exclusive_storage = true,
        .first_interface = 0,
        .last_interface = 0,
        .request_endpoints = usb_storage_request_endpoints,
        .set_first_interface = usb_storage_set_first_interface,
        .get_config_descriptor = usb_storage_get_config_descriptor,
        .init_connection = usb_storage_init_connection,
        .init = usb_storage_init,
        .disconnect = usb_storage_disconnect,
        .transfer_complete = usb_storage_transfer_complete,
        .control_request = usb_storage_control_request,
#ifdef HAVE_HOTSWAP
        .notify_hotswap = usb_storage_notify_hotswap,
#endif
    },
#endif
#ifdef USB_ENABLE_SERIAL
    [USB_DRIVER_SERIAL] = {
        .enabled = false,
        .needs_exclusive_storage = false,
        .first_interface = 0,
        .last_interface = 0,
        .request_endpoints = usb_serial_request_endpoints,
        .set_first_interface = usb_serial_set_first_interface,
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
#ifdef USB_ENABLE_CHARGING_ONLY
    [USB_DRIVER_CHARGING_ONLY] = {
        .enabled = false,
        .needs_exclusive_storage = false,
        .first_interface = 0,
        .last_interface = 0,
        .request_endpoints = usb_charging_only_request_endpoints,
        .set_first_interface = usb_charging_only_set_first_interface,
        .get_config_descriptor = usb_charging_only_get_config_descriptor,
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
#ifdef USB_ENABLE_HID
    [USB_DRIVER_HID] = {
        .enabled = false,
        .needs_exclusive_storage = false,
        .first_interface = 0,
        .last_interface = 0,
        .request_endpoints = usb_hid_request_endpoints,
        .set_first_interface = usb_hid_set_first_interface,
        .get_config_descriptor = usb_hid_get_config_descriptor,
        .init_connection = usb_hid_init_connection,
        .init = usb_hid_init,
        .disconnect = usb_hid_disconnect,
        .transfer_complete = usb_hid_transfer_complete,
        .control_request = usb_hid_control_request,
#ifdef HAVE_HOTSWAP
        .notify_hotswap = NULL,
#endif
    },
#endif
};

#ifdef USB_LEGACY_CONTROL_API
static struct usb_ctrlrequest buffered_request;
static struct usb_ctrlrequest* volatile active_request = NULL;
static volatile unsigned int num_active_requests = 0;
static void* volatile control_write_data = NULL;
static volatile bool control_write_data_done = false;
#endif

static void usb_core_control_request_handler(struct usb_ctrlrequest* req, void* reqdata);

static unsigned char response_data[256] USB_DEVBSS_ATTR;

/** NOTE Serial Number
 * The serial number string is split into two parts:
 * - the first character indicates the set of interfaces enabled
 * - the other characters form a (hopefully) unique device-specific number
 * The implementation of set_serial_descriptor should left the first character
 * of usb_string_iSerial unused, ie never write to
 * usb_string_iSerial.wString[0] but should take it into account when
 * computing the length of the descriptor
 */

static const short hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
#if defined(IPOD_ARCH) && defined(CPU_PP)
static void set_serial_descriptor(void)
{
#ifdef IPOD_VIDEO
    uint32_t* serial = (uint32_t*)0x20004034;
#else
    uint32_t* serial = (uint32_t*)0x20002034;
#endif

    /* We need to convert from a little-endian 64-bit int
       into a utf-16 string of hex characters */
    short* p = &usb_string_iSerial.wString[24];
    uint32_t x;
    int i, j;

    for(i = 0; i < 2; i++) {
       x = serial[i];
       for(j = 0; j < 8; j++) {
          *p-- = hex[x & 0xf];
          x >>= 4;
       }
    }
    usb_string_iSerial.bLength = 52;
}
#elif defined(HAVE_AS3514)
static void set_serial_descriptor(void)
{
    unsigned char serial[AS3514_UID_LEN];
    /* Align 32 digits right in the 40-digit serial number */
    short* p = &usb_string_iSerial.wString[1];
    int i;

    ascodec_readbytes(AS3514_UID_0, AS3514_UID_LEN, serial);
    for(i = 0; i < AS3514_UID_LEN; i++) {
        *p++ = hex[(serial[i] >> 4) & 0xF];
        *p++ = hex[(serial[i] >> 0) & 0xF];
    }
    usb_string_iSerial.bLength = 36 + (2 * AS3514_UID_LEN);
}
#elif (CONFIG_CPU == IMX233) && IMX233_SUBTARGET >= 3700
// FIXME where is the STMP3600 serial number stored ?
static void set_serial_descriptor(void)
{
    short* p = &usb_string_iSerial.wString[1];
    for(int i = 0; i < IMX233_NUM_OCOTP_OPS; i++) {
        uint32_t ops = imx233_ocotp_read(&HW_OCOTP_OPSn(i));
        for(int j = 0; j < 8; j++) {
            *p++ = hex[(ops >> 28) & 0xF];
            ops <<= 4;
        }
    }
    usb_string_iSerial.bLength = 2 + 2 * (1 + IMX233_NUM_OCOTP_OPS * 8);
}
#elif defined(SANSA_CONNECT)
static void set_serial_descriptor(void)
{
    char deviceid[32];
    short* p = &usb_string_iSerial.wString[1];
    int i;

    if(!cryptomem_read_deviceid(deviceid)) {
        for(i = 0; i < 32; i++) {
            *p++ = deviceid[i];
        }
        usb_string_iSerial.bLength = 2 + 2 * (1 + 32);
    } else {
        device_descriptor.iSerialNumber = 0;
    }
}
#elif (CONFIG_STORAGE & STORAGE_ATA)
/* If we don't know the device serial number, use the one
 * from the disk */
static void set_serial_descriptor(void)
{
    short* p = &usb_string_iSerial.wString[1];
    unsigned short* identify = ata_get_identify();
    char sn[20];
    char length = 20;
    int i;

    for (i = 0; i < length / 2; i++) {
        ((unsigned short*)sn)[i] = htobe16(identify[i + 10]);
    }

    char is_printable = 1;
    for (i = 0; i < length; i++) {
        if (sn[i] < 32 || sn[i] > 126) {
            is_printable = 0;
            break;
        }
    }

    if (is_printable) {
        /* trim trailing spaces */
        while (length > 0 && sn[length - 1] == ' ') {
            length--;
        }

        for (i = 0; i < length; i++) {
            *p++ = sn[i];
        }

        usb_string_iSerial.bLength = 2 + 2 * (1 + length);
    }
    else {
        for (i = 0; i < length; i++) {
            char x = sn[i];
            *p++ = hex[(x >> 4) & 0xF];
            *p++ = hex[x & 0xF];
        }

        usb_string_iSerial.bLength = 2 + 2 * (1 + length * 2);
    }
}
#elif (CONFIG_STORAGE & STORAGE_RAMDISK)
/* This "serial number" isn't unique, but it should never actually
   appear in non-testing use */
static void set_serial_descriptor(void)
{
    short* p = &usb_string_iSerial.wString[1];
    int i;
    for(i = 0; i < 16; i++) {
        *p++ = hex[(2 * i) & 0xF];
        *p++ = hex[(2 * i + 1) & 0xF];
    }
    usb_string_iSerial.bLength = 68;
}
#else
static void set_serial_descriptor(void)
{
    device_descriptor.iSerialNumber = 0;
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
    for(i = 0; i < USB_NUM_DRIVERS; i++)
        if(drivers[i].init != NULL)
            drivers[i].init();

    initialized = true;
    usb_state = DEFAULT;
#ifdef HAVE_USB_CHARGING_ENABLE
    usb_no_host = false;
    timeout_register(&usb_no_host_timeout, usb_no_host_callback, HZ*10, 0);
#endif
    logf("usb_core_init() finished");
}

void usb_core_exit(void)
{
    int i;
    if(drivers_connected)
    {
        for(i = 0; i < USB_NUM_DRIVERS; i++)
            if(drivers[i].enabled && drivers[i].disconnect != NULL)
            {
                drivers[i].disconnect();
                drivers[i].enabled = false;
            }
        drivers_connected = false;
    }

    if(initialized) {
        usb_drv_exit();
        initialized = false;
    }
    usb_state = DEFAULT;
#ifdef HAVE_USB_CHARGING_ENABLE
    usb_no_host = false;
    usb_charging_maxcurrent_change(usb_charging_maxcurrent());
#endif
    logf("usb_core_exit() finished");
}

void usb_core_handle_transfer_completion(
    struct usb_transfer_completion_event_data* event)
{
    completion_handler_t handler;
    int ep = event->endpoint;

    switch(ep) {
        case EP_CONTROL:
            logf("ctrl handled %ld req=0x%x",
                 current_tick,
                 ((struct usb_ctrlrequest*)event->data[0])->bRequest);

            usb_core_control_request_handler(
                (struct usb_ctrlrequest*)event->data[0], event->data[1]);
            break;
        default:
            handler = ep_data[ep].completion_handler[EP_DIR(event->dir)];
            if(handler != NULL)
                handler(ep, event->dir, event->status, event->length);
            break;
    }
}

void usb_core_enable_driver(int driver, bool enabled)
{
    drivers[driver].enabled = enabled;
}

bool usb_core_driver_enabled(int driver)
{
    return drivers[driver].enabled;
}

bool usb_core_any_exclusive_storage(void)
{
    int i;
    for(i = 0; i < USB_NUM_DRIVERS; i++)
        if(drivers[i].enabled && drivers[i].needs_exclusive_storage)
            return true;

    return false;
}

#ifdef HAVE_HOTSWAP
void usb_core_hotswap_event(int volume, bool inserted)
{
    int i;
    for(i = 0; i < USB_NUM_DRIVERS; i++)
        if(drivers[i].enabled && drivers[i].notify_hotswap != NULL)
            drivers[i].notify_hotswap(volume, inserted);
}
#endif

static void usb_core_set_serial_function_id(void)
{
    int i, id = 0;

    for(i = 0; i < USB_NUM_DRIVERS; i++)
        if(drivers[i].enabled)
            id |= 1 << i;

    usb_string_iSerial.wString[0] = hex[id];
}

int usb_core_request_endpoint(int type, int dir, struct usb_class_driver* drv)
{
    int ret, ep;

    ret = usb_drv_request_endpoint(type, dir);

    if(ret == -1)
        return -1;

    dir = EP_DIR(ret);
    ep = EP_NUM(ret);

    ep_data[ep].completion_handler[dir] = drv->transfer_complete;
    ep_data[ep].control_handler[dir] = drv->control_request;

    return ret;
}

void usb_core_release_endpoint(int ep)
{
    int dir;

    usb_drv_release_endpoint(ep);

    dir = EP_DIR(ep);
    ep = EP_NUM(ep);

    ep_data[ep].completion_handler[dir] = NULL;
    ep_data[ep].control_handler[dir] = NULL;
}

static void allocate_interfaces_and_endpoints(void)
{
    int i;
    int interface = 0;

    memset(ep_data, 0, sizeof(ep_data));

    for(i = 0; i < USB_NUM_ENDPOINTS; i++) {
        usb_drv_release_endpoint(i | USB_DIR_OUT);
        usb_drv_release_endpoint(i | USB_DIR_IN);
    }

    for(i = 0; i < USB_NUM_DRIVERS; i++) {
        if(drivers[i].enabled) {
            drivers[i].first_interface = interface;

            if(drivers[i].request_endpoints(&drivers[i])) {
                drivers[i].enabled = false;
                continue;
            }

            interface = drivers[i].set_first_interface(interface);
            drivers[i].last_interface = interface;
        }
    }
    usb_core_num_interfaces = interface;
}


static void control_request_handler_drivers(struct usb_ctrlrequest* req, void* reqdata)
{
    int i, interface = req->wIndex & 0xff;
    bool handled = false;

    for(i = 0; i < USB_NUM_DRIVERS; i++) {
        if(drivers[i].enabled &&
                drivers[i].control_request &&
                drivers[i].first_interface <= interface &&
                drivers[i].last_interface > interface)
        {
            handled = drivers[i].control_request(req, reqdata, response_data);
            if(handled)
                break;
        }
    }
    if(!handled) {
        /* nope. flag error */
        logf("bad req:desc %d:%d", req->bRequest, req->wValue >> 8);
        usb_drv_control_response(USB_CONTROL_STALL, NULL, 0);
    }
}

static void request_handler_device_get_descriptor(struct usb_ctrlrequest* req, void* reqdata)
{
    int size;
    const void* ptr = NULL;
    int length = req->wLength;
    int index = req->wValue & 0xff;

    switch(req->wValue >> 8) { /* type */
        case USB_DT_DEVICE:
            ptr = &device_descriptor;
            size = sizeof(struct usb_device_descriptor);
            break;

        case USB_DT_OTHER_SPEED_CONFIG:
        case USB_DT_CONFIG: {
                int i, max_packet_size;

                if(req->wValue>>8==USB_DT_CONFIG) {
                    max_packet_size = (usb_drv_port_speed() ? 512 : 64);
                    config_descriptor.bDescriptorType = USB_DT_CONFIG;
                }
                else {
                    max_packet_size=(usb_drv_port_speed() ? 64 : 512);
                    config_descriptor.bDescriptorType =
                        USB_DT_OTHER_SPEED_CONFIG;
                }
#ifdef HAVE_USB_CHARGING_ENABLE
                if (usb_charging_mode == USB_CHARGING_DISABLE) {
                    config_descriptor.bMaxPower = (100+1)/2;
                    usb_charging_current_requested = 100;
                }
                else {
                    config_descriptor.bMaxPower = (500+1)/2;
                    usb_charging_current_requested = 500;
                }
#endif
                size = sizeof(struct usb_config_descriptor);

                for(i = 0; i < USB_NUM_DRIVERS; i++)
                    if(drivers[i].enabled && drivers[i].get_config_descriptor)
                        size += drivers[i].get_config_descriptor(
                                    &response_data[size], max_packet_size);

                config_descriptor.bNumInterfaces = usb_core_num_interfaces;
                config_descriptor.wTotalLength = (uint16_t)size;
                memcpy(&response_data[0], &config_descriptor,
                        sizeof(struct usb_config_descriptor));

                ptr = response_data;
                break;
            }

        case USB_DT_STRING:
            logf("STRING %d", index);
            if((unsigned)index < USB_STRING_INDEX_MAX) {
                size = usb_strings[index]->bLength;
                ptr = usb_strings[index];
            }
            else if(index == 0xee) {
                /* We don't have a real OS descriptor, and we don't handle
                 * STALL correctly on some devices, so we return any valid
                 * string (we arbitrarily pick the manufacturer name)
                 */
                size = usb_string_iManufacturer.bLength;
                ptr = &usb_string_iManufacturer;
            }
            else {
                logf("bad string id %d", index);
                ptr = NULL;
            }
            break;

        case USB_DT_DEVICE_QUALIFIER:
            ptr = &qualifier_descriptor;
            size = sizeof(struct usb_qualifier_descriptor);
            break;

        default:
            logf("ctrl desc.");
            control_request_handler_drivers(req, reqdata);
            return;
    }

    if(ptr) {
        logf("data %d (%d)", size, length);
        length = MIN(size, length);

        if (ptr != response_data)
            memcpy(response_data, ptr, length);

        usb_drv_control_response(USB_CONTROL_ACK, response_data, length);
    } else {
        usb_drv_control_response(USB_CONTROL_STALL, NULL, 0);
    }
}

static void usb_core_do_set_addr(uint8_t address)
{
    logf("usb_core: SET_ADR %d", address);
    usb_address = address;
    usb_state = ADDRESS;
}

static void usb_core_do_set_config(uint8_t config)
{
    logf("usb_core: SET_CONFIG %d",config);
    if(config) {
        usb_state = CONFIGURED;

        if(drivers_connected)
            for(int i = 0; i < USB_NUM_DRIVERS; i++)
                if(drivers[i].enabled && drivers[i].disconnect != NULL)
                    drivers[i].disconnect();

        for(int i = 0; i < USB_NUM_DRIVERS; i++)
            if(drivers[i].enabled && drivers[i].init_connection)
                drivers[i].init_connection();
        drivers_connected = true;
    }
    else
        usb_state = ADDRESS;
    #ifdef HAVE_USB_CHARGING_ENABLE
    usb_charging_maxcurrent_change(usb_charging_maxcurrent());
    #endif
}

static void usb_core_do_clear_feature(int recip, int recip_nr, int feature)
{
    logf("usb_core: CLEAR FEATURE (%d,%d,%d)", recip, recip_nr, feature);
    if(recip == USB_RECIP_ENDPOINT)
    {
        if(feature == USB_ENDPOINT_HALT)
            usb_drv_stall(EP_NUM(recip_nr), false, EP_DIR(recip_nr));
    }
}

static void request_handler_device(struct usb_ctrlrequest* req, void* reqdata)
{
    unsigned address;

    switch(req->bRequest) {
        case USB_REQ_GET_CONFIGURATION:
            logf("usb_core: GET_CONFIG");
            response_data[0] = (usb_state == ADDRESS ? 0 : 1);
            usb_drv_control_response(USB_CONTROL_ACK, response_data, 1);
            break;
        case USB_REQ_SET_CONFIGURATION:
            usb_drv_cancel_all_transfers();
            usb_core_do_set_config(req->wValue);
            usb_drv_control_response(USB_CONTROL_ACK, NULL, 0);
            break;
        case USB_REQ_SET_ADDRESS:
            /* NOTE: We really have no business handling this and drivers
             * should just handle it themselves. We don't care beyond
             * knowing if we've been assigned an address yet, or not. */
            address = req->wValue;
            usb_drv_control_response(USB_CONTROL_ACK, NULL, 0);
            usb_drv_cancel_all_transfers();
            usb_drv_set_address(address);
            usb_core_do_set_addr(address);
            break;
        case USB_REQ_GET_DESCRIPTOR:
            logf("usb_core: GET_DESC %d", req->wValue >> 8);
            request_handler_device_get_descriptor(req, reqdata);
            break;
        case USB_REQ_SET_FEATURE:
            if(req->wValue==USB_DEVICE_TEST_MODE) {
                int mode = req->wIndex >> 8;
                usb_drv_control_response(USB_CONTROL_ACK, NULL, 0);
                usb_drv_set_test_mode(mode);
            } else {
                usb_drv_control_response(USB_CONTROL_STALL, NULL, 0);
            }
            break;
        case USB_REQ_GET_STATUS:
            response_data[0] = 0;
            response_data[1] = 0;
            usb_drv_control_response(USB_CONTROL_ACK, response_data, 2);
            break;
        default:
            logf("bad req:desc %d:%d", req->bRequest, req->wValue);
            usb_drv_control_response(USB_CONTROL_STALL, NULL, 0);
            break;
    }
}

static void request_handler_interface_standard(struct usb_ctrlrequest* req, void* reqdata)
{
    switch (req->bRequest)
    {
        case USB_REQ_SET_INTERFACE:
            logf("usb_core: SET_INTERFACE");
            usb_drv_control_response(USB_CONTROL_ACK, NULL, 0);
            break;

        case USB_REQ_GET_INTERFACE:
            logf("usb_core: GET_INTERFACE");
            response_data[0] = 0;
            usb_drv_control_response(USB_CONTROL_ACK, response_data, 1);
            break;
        case USB_REQ_GET_STATUS:
            response_data[0] = 0;
            response_data[1] = 0;
            usb_drv_control_response(USB_CONTROL_ACK, response_data, 2);
            break;
        case USB_REQ_CLEAR_FEATURE:
        case USB_REQ_SET_FEATURE:
            /* TODO: These used to be ignored (erroneously).
             * Should they be passed to the drivers instead? */
            usb_drv_control_response(USB_CONTROL_STALL, NULL, 0);
            break;
        default:
            control_request_handler_drivers(req, reqdata);
            break;
    }
}

static void request_handler_interface(struct usb_ctrlrequest* req, void* reqdata)
{
    switch(req->bRequestType & USB_TYPE_MASK) {
        case USB_TYPE_STANDARD:
            request_handler_interface_standard(req, reqdata);
            break;
        case USB_TYPE_CLASS:
            control_request_handler_drivers(req, reqdata);
            break;
        case USB_TYPE_VENDOR:
        default:
            logf("bad req:desc %d", req->bRequest);
            usb_drv_control_response(USB_CONTROL_STALL, NULL, 0);
            break;
    }
}

static void request_handler_endpoint_drivers(struct usb_ctrlrequest* req, void* reqdata)
{
    bool handled = false;
    control_handler_t control_handler = NULL;

    if(EP_NUM(req->wIndex) < USB_NUM_ENDPOINTS)
        control_handler =
            ep_data[EP_NUM(req->wIndex)].control_handler[EP_DIR(req->wIndex)];

    if(control_handler)
        handled = control_handler(req, reqdata, response_data);

    if(!handled) {
        /* nope. flag error */
        logf("usb bad req %d", req->bRequest);
        usb_drv_control_response(USB_CONTROL_STALL, NULL, 0);
    }
}

static void request_handler_endpoint_standard(struct usb_ctrlrequest* req, void* reqdata)
{
    switch (req->bRequest) {
        case USB_REQ_CLEAR_FEATURE:
            usb_core_do_clear_feature(USB_RECIP_ENDPOINT,
                                      req->wIndex,
                                      req->wValue);
            usb_drv_control_response(USB_CONTROL_ACK, NULL, 0);
            break;
        case USB_REQ_SET_FEATURE:
            logf("usb_core: SET FEATURE (%d)", req->wValue);
            if(req->wValue == USB_ENDPOINT_HALT)
               usb_drv_stall(EP_NUM(req->wIndex), true, EP_DIR(req->wIndex));

            usb_drv_control_response(USB_CONTROL_ACK, NULL, 0);
            break;
        case USB_REQ_GET_STATUS:
            response_data[0] = 0;
            response_data[1] = 0;
            logf("usb_core: GET_STATUS");
            if(req->wIndex > 0)
                response_data[0] = usb_drv_stalled(EP_NUM(req->wIndex),
                                                    EP_DIR(req->wIndex));

            usb_drv_control_response(USB_CONTROL_ACK, response_data, 2);
            break;
        default:
            request_handler_endpoint_drivers(req, reqdata);
            break;
    }
}

static void request_handler_endpoint(struct usb_ctrlrequest* req, void* reqdata)
{
    switch(req->bRequestType & USB_TYPE_MASK) {
        case USB_TYPE_STANDARD:
            request_handler_endpoint_standard(req, reqdata);
            break;
        case USB_TYPE_CLASS:
            request_handler_endpoint_drivers(req, reqdata);
            break;
        case USB_TYPE_VENDOR:
        default:
            logf("bad req:desc %d", req->bRequest);
            usb_drv_control_response(USB_CONTROL_STALL, NULL, 0);
            break;
    }
}

/* Handling USB requests starts here */
static void usb_core_control_request_handler(struct usb_ctrlrequest* req, void* reqdata)
{
#ifdef HAVE_USB_CHARGING_ENABLE
    timeout_cancel(&usb_no_host_timeout);
    if(usb_no_host) {
        usb_no_host = false;
        usb_charging_maxcurrent_change(usb_charging_maxcurrent());
    }
#endif
    if(usb_state == DEFAULT) {
        set_serial_descriptor();
        usb_core_set_serial_function_id();

        allocate_interfaces_and_endpoints();
    }

    switch(req->bRequestType & USB_RECIP_MASK) {
        case USB_RECIP_DEVICE:
            request_handler_device(req, reqdata);
            break;
        case USB_RECIP_INTERFACE:
            request_handler_interface(req, reqdata);
            break;
        case USB_RECIP_ENDPOINT:
            request_handler_endpoint(req, reqdata);
            break;
        default:
            logf("unsupported recipient");
            usb_drv_control_response(USB_CONTROL_STALL, NULL, 0);
            break;
    }
}

/* called by usb_drv_int() */
void usb_core_bus_reset(void)
{
    logf("usb_core: bus reset");
    usb_address = 0;
    usb_state = DEFAULT;
#ifdef HAVE_USB_CHARGING_ENABLE
#ifdef HAVE_USB_CHARGING_IN_THREAD
    /* On some targets usb_charging_maxcurrent_change() cannot be called
     * from an interrupt handler; get the USB thread to do it instead. */
    usb_charger_update();
#else
    usb_charging_maxcurrent_change(usb_charging_maxcurrent());
#endif
#endif
}

/* called by usb_drv_transfer_completed() */
void usb_core_transfer_complete(int endpoint, int dir, int status, int length)
{
    struct usb_transfer_completion_event_data* completion_event =
        &ep_data[endpoint].completion_event[EP_DIR(dir)];

    void* data0 = NULL;
    void* data1 = NULL;

#ifdef USB_LEGACY_CONTROL_API
    if(endpoint == EP_CONTROL) {
        bool cwdd = control_write_data_done;
        struct usb_ctrlrequest* req = active_request;

        if(dir == USB_DIR_OUT && req && cwdd) {
            data0 = req;
            data1 = control_write_data;
        } else {
            return;
        }
    }
#endif

    completion_event->endpoint = endpoint;
    completion_event->dir = dir;
    completion_event->data[0] = data0;
    completion_event->data[1] = data1;
    completion_event->status = status;
    completion_event->length = length;

    usb_signal_transfer_completion(completion_event);
}

void usb_core_handle_notify(long id, intptr_t data)
{
    switch(id)
    {
        case USB_NOTIFY_SET_ADDR:
            usb_core_do_set_addr(data);
            break;
        case USB_NOTIFY_SET_CONFIG:
            usb_core_do_set_config(data);
            break;
        default:
            break;
    }
}

void usb_core_control_request(struct usb_ctrlrequest* req, void* reqdata)
{
    struct usb_transfer_completion_event_data* completion_event =
        &ep_data[EP_CONTROL].completion_event[EP_DIR(USB_DIR_IN)];

    completion_event->endpoint = EP_CONTROL;
    completion_event->dir = 0;
    completion_event->data[0] = (void*)req;
    completion_event->data[1] = reqdata;
    completion_event->status = 0;
    completion_event->length = 0;
    logf("ctrl received %ld, req=0x%x", current_tick, req->bRequest);
    usb_signal_transfer_completion(completion_event);
}

void usb_core_control_complete(int status)
{
    /* We currently don't use this, it's here to make the API look good ;)
     * It makes sense to #define it away on normal builds.
     */
    (void)status;
    logf("ctrl complete %ld, %d", current_tick, status);
}

#ifdef USB_LEGACY_CONTROL_API
/* Only needed if the driver does not support the new API yet */
void usb_core_legacy_control_request(struct usb_ctrlrequest* req)
{
    /* Only submit non-overlapping requests */
    if (num_active_requests++ == 0)
    {
        buffered_request = *req;
        active_request = &buffered_request;
        control_write_data = NULL;
        control_write_data_done = false;

        usb_core_control_request(req, NULL);
    }
}

void usb_drv_control_response(enum usb_control_response resp,
                              void* data, int length)
{
    struct usb_ctrlrequest* req = active_request;
    unsigned int num_active = num_active_requests--;

    /*
     * There should have been a prior request submission, at least.
     * FIXME: It seems the iPod video can get here and ignoring it
     * allows the connection to succeed??
     */
    if (num_active == 0)
    {
        //panicf("null ctrl req");
        return;
    }

    /*
     * This can happen because an active request was already pending when
     * the driver submitted a new one in usb_core_legacy_control_request().
     * This could mean two things: (a) a driver bug; or (b) the host sent
     * another request because we were too slow in handling an earlier one.
     *
     * The USB spec requires we respond to the latest request and drop any
     * earlier ones, but that's not easy to do with the current design of
     * the USB stack. Thus, the host will be expecting a response for the
     * latest request, but this response is for the _earliest_ request.
     *
     * Play it safe and return a STALL. At this point we've recovered from
     * the error on our end and will be ready to handle the next request.
     */
    if (num_active > 1)
    {
        active_request = NULL;
        num_active_requests = 0;
        usb_drv_stall(EP_CONTROL, true, true);
        return;
    }

    if(req->wLength == 0)
    {
        active_request = NULL;

        /* No-data request */
        if(resp == USB_CONTROL_ACK)
            usb_drv_send(EP_CONTROL, data, length);
        else if(resp == USB_CONTROL_STALL)
            usb_drv_stall(EP_CONTROL, true, true);
        else
            panicf("RECEIVE on non-data req");
    }
    else if(req->bRequestType & USB_DIR_IN)
    {
        /* Control read request */
        if(resp == USB_CONTROL_ACK)
        {
            active_request = NULL;
            usb_drv_recv_nonblocking(EP_CONTROL, NULL, 0);
            usb_drv_send(EP_CONTROL, data, length);
        }
        else if(resp == USB_CONTROL_STALL)
        {
            active_request = NULL;
            usb_drv_stall(EP_CONTROL, true, true);
        }
        else
        {
            panicf("RECEIVE on ctrl read req");
        }
    }
    else if(!control_write_data_done)
    {
        /* Control write request, data phase */
        if(resp == USB_CONTROL_RECEIVE)
        {
            control_write_data = data;
            control_write_data_done = true;
            usb_drv_recv_nonblocking(EP_CONTROL, data, length);
        }
        else if(resp == USB_CONTROL_STALL)
        {
            /* We should stall the OUT endpoint here, but the old code did
             * not do so and some drivers may not handle it correctly. */
            active_request = NULL;
            usb_drv_stall(EP_CONTROL, true, true);
        }
        else
        {
            panicf("ACK on ctrl write data");
        }
    }
    else
    {
        active_request = NULL;
        control_write_data = NULL;
        control_write_data_done = false;

        /* Control write request, status phase */
        if(resp == USB_CONTROL_ACK)
            usb_drv_send(EP_CONTROL, NULL, 0);
        else if(resp == USB_CONTROL_STALL)
            usb_drv_stall(EP_CONTROL, true, true);
        else
            panicf("RECEIVE on ctrl write status");
    }
}
#endif

void usb_core_notify_set_address(uint8_t addr)
{
    logf("notify set addr received %ld", current_tick);
    usb_signal_notify(USB_NOTIFY_SET_ADDR, addr);
}

void usb_core_notify_set_config(uint8_t config)
{
    logf("notify set config received %ld", current_tick);
    usb_signal_notify(USB_NOTIFY_SET_CONFIG, config);
}

#ifdef HAVE_USB_CHARGING_ENABLE
void usb_charging_enable(int state)
{
    usb_charging_mode = state;
    usb_charging_maxcurrent_change(usb_charging_maxcurrent());
}

int usb_charging_maxcurrent()
{
    if (!initialized || usb_charging_mode == USB_CHARGING_DISABLE)
        return 100;
    if (usb_state == CONFIGURED)
        return usb_charging_current_requested;
    if (usb_charging_mode == USB_CHARGING_FORCE && usb_no_host)
        return 500;
    return 100;
}
#endif
