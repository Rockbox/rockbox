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

#ifdef USB_ENABLE_AUDIO
#include "usb_audio.h"
#include "usb_audio_def.h" // DEBUG
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

#define NUM_CONFIGS 1

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
    .bNumConfigurations = NUM_CONFIGS
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
    .bNumConfigurations = NUM_CONFIGS
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
static int usb_config = 0;
static bool initialized = false;
static volatile bool bus_reset_pending = false;
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

static int usb_core_num_interfaces[NUM_CONFIGS];

typedef void (*completion_handler_t)(int ep, int dir, int status, int length);
typedef bool (*fast_completion_handler_t)(int ep, int dir, int status, int length);
typedef bool (*control_handler_t)(struct usb_ctrlrequest* req, void* reqdata,
    unsigned char* dest);

static struct
{
    completion_handler_t completion_handler[2];
    fast_completion_handler_t fast_completion_handler[2];
    control_handler_t control_handler[2];
    struct usb_transfer_completion_event_data completion_event[2];
} ep_data[USB_NUM_ENDPOINTS];

struct ep_alloc_state {
    int8_t type[2];
    struct usb_class_driver* owner[2];
};

static struct ep_alloc_state ep_alloc_states[NUM_CONFIGS][USB_NUM_ENDPOINTS];

static struct usb_class_driver drivers[USB_NUM_DRIVERS] =
{
#ifdef USB_ENABLE_STORAGE
    [USB_DRIVER_MASS_STORAGE] = {
        .enabled = false,
        .needs_exclusive_storage = true,
        .config = 1,
        .first_interface = 0,
        .last_interface = 0,
        .ep_allocs_size = ARRAYLEN(usb_storage_ep_allocs),
        .ep_allocs = usb_storage_ep_allocs,
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
        .config = 1,
        .first_interface = 0,
        .last_interface = 0,
        .ep_allocs_size = ARRAYLEN(usb_serial_ep_allocs),
        .ep_allocs = usb_serial_ep_allocs,
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
        .config = 1,
        .first_interface = 0,
        .last_interface = 0,
        .ep_allocs_size = 0,
        .ep_allocs = NULL,
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
        .config = 1,
        .first_interface = 0,
        .last_interface = 0,
        .ep_allocs_size = ARRAYLEN(usb_hid_ep_allocs),
        .ep_allocs = usb_hid_ep_allocs,
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
#ifdef USB_ENABLE_AUDIO
    [USB_DRIVER_AUDIO] = {
        .enabled = false,
        .needs_exclusive_storage = false,
        .config = 1,
        .first_interface = 0,
        .last_interface = 0,
        .ep_allocs_size = ARRAYLEN(usb_audio_ep_allocs),
        .ep_allocs = usb_audio_ep_allocs,
        .set_first_interface = usb_audio_set_first_interface,
        .get_config_descriptor = usb_audio_get_config_descriptor,
        .init_connection = usb_audio_init_connection,
        .init = usb_audio_init,
        .disconnect = usb_audio_disconnect,
        .transfer_complete = usb_audio_transfer_complete,
        .fast_transfer_complete = usb_audio_fast_transfer_complete,
        .control_request = usb_audio_control_request,
#ifdef HAVE_HOTSWAP
        .notify_hotswap = NULL,
#endif
        .set_interface = usb_audio_set_interface,
        .get_interface = usb_audio_get_interface,
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

static int usb_core_do_set_config(uint8_t new_config);
static void usb_core_control_request_handler(struct usb_ctrlrequest* req, void* reqdata);

static unsigned char response_data[256] USB_DEVBSS_ATTR;

#define is_active(driver) ((driver).enabled && (driver).config == usb_config)
#define has_if(driver, interface) ((interface) >= (driver).first_interface && (interface) < (driver).last_interface)

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
        /* trim ALL spaces */
        int totallen = length;
        for (i = 0; i < length; i++) {
            if (sn[i] == ' ') {
                totallen--;
                continue;
            }
            *p++ = sn[i];
        }

        usb_string_iSerial.bLength = 2 + 2 * (1 + totallen);
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

    /* clear endpoint allocation state */
    memset(ep_alloc_states, 0, sizeof(ep_alloc_states));

    initialized = true;
    usb_state = DEFAULT;
    usb_config = 0;
#ifdef HAVE_USB_CHARGING_ENABLE
    usb_no_host = false;
    timeout_register(&usb_no_host_timeout, usb_no_host_callback, HZ*10, 0);
#endif
    logf("usb_core_init() finished");
}

void usb_core_exit(void)
{
    usb_core_do_set_config(0);
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

/* synchronize endpoint initialization state to allocation state */
static void init_deinit_endpoints(uint8_t conf_index, bool init) {
    for(int epnum = 0; epnum < USB_NUM_ENDPOINTS; epnum += 1) {
        for(int dir = 0; dir < 2; dir += 1) {
            struct ep_alloc_state* alloc = &ep_alloc_states[conf_index][epnum];
            if(alloc->owner[dir] == NULL) {
                continue;
            }
            int ep = epnum | (dir == DIR_OUT ? USB_DIR_OUT : USB_DIR_IN);
            int ret = init ?
                usb_drv_init_endpoint(ep, alloc->type[dir], -1) :
                usb_drv_deinit_endpoint(ep);
            if(ret) {
                logf("usb_core: usb_drv_%s_endpoint failed ep=%d dir=%d", init ? "init" : "deinit", epnum, dir);
                continue;
            }
            if(init) {
                ep_data[epnum].completion_handler[dir] = alloc->owner[dir]->transfer_complete;
                ep_data[epnum].fast_completion_handler[dir] = alloc->owner[dir]->fast_transfer_complete;
                ep_data[epnum].control_handler[dir] = alloc->owner[dir]->control_request;
            }
        }
    }
}

static void allocate_interfaces_and_endpoints(void)
{
    if(usb_config != 0) {
        /* deinit currently used endpoints */
        init_deinit_endpoints(usb_config - 1, false);
    }

    /* reset allocations */
    memset(ep_alloc_states, 0, sizeof(ep_alloc_states));

    int interface[NUM_CONFIGS] = {0};

    for(int i = 0; i < USB_NUM_DRIVERS; i++) {
        struct usb_class_driver* driver = &drivers[i];
        const uint8_t conf_index = driver->config - 1;

        if(!driver->enabled) {
            continue;
        }

        /* assign endpoints */
        for(int reqnum = 0; reqnum < driver->ep_allocs_size; reqnum += 1) {
            /* find matching ep */
            struct usb_class_driver_ep_allocation* req = &driver->ep_allocs[reqnum];
            req->ep = 0;
            for(int epnum = 1; epnum < USB_NUM_ENDPOINTS; epnum += 1) {
                struct usb_drv_ep_spec* spec = &usb_drv_ep_specs[epnum];
                /* ep type check */
                const int8_t spec_type = spec->type[req->dir];
                if(spec_type != req->type && spec_type != USB_ENDPOINT_TYPE_ANY) {
                    continue;
                }
                /* free check */
                struct ep_alloc_state* alloc = &ep_alloc_states[conf_index][epnum];
                if(alloc->owner[req->dir] != NULL) {
                    continue;
                }

                /* this ep's requested direction is free */

                /* another checks */
                if(usb_drv_ep_specs_flags & USB_ENDPOINT_SPEC_IO_EXCLUSIVE) {
                    /* check for the other direction type */
                    if(alloc->owner[!req->dir] != NULL) {
                        /* the other side is allocated */
                        continue;
                    }
                }
                if(usb_drv_ep_specs_flags & USB_ENDPOINT_SPEC_FORCE_IO_TYPE_MATCH) {
                    /* check for other direction type */
                    if(alloc->owner[!req->dir] != NULL && alloc->type[!req->dir] != req->type) {
                        /* the other side is allocated with another type */
                        continue;
                    }
                }

                /* all checks passed, assign it */
                const int ep = epnum | (req->dir == DIR_OUT ? USB_DIR_OUT : USB_DIR_IN);
                req->ep = ep;
                alloc->owner[req->dir] = driver;
                alloc->type[req->dir] = req->type;
                break;
            }
            if(req->ep == 0 && !req->optional) {
                /* no matching ep found, disable the driver */
                driver->enabled = false;
                /* also revert all allocations for this driver */
                for(reqnum = reqnum - 1; reqnum >= 0; reqnum -= 1) {
                    const uint8_t ep = driver->ep_allocs[reqnum].ep;
                    const uint8_t epnum = EP_NUM(ep);
                    const uint8_t epdir = EP_DIR(ep);
                    const uint8_t dir = epdir == USB_DIR_OUT ? DIR_OUT : DIR_IN;
                    ep_alloc_states[conf_index][epnum].owner[dir] = NULL;
                }
                break;
            }
        }

        if(!driver->enabled) {
            continue;
        }

        /* assign interfaces */
        driver->first_interface = interface[conf_index];
        interface[conf_index] = driver->set_first_interface(interface[conf_index]);
        driver->last_interface = interface[conf_index];
    }

    memcpy(usb_core_num_interfaces, interface, sizeof(interface));
}


static void control_request_handler_drivers(struct usb_ctrlrequest* req, void* reqdata)
{
    int i, interface = req->wIndex & 0xff;
    bool handled = false;

    for(i = 0; i < USB_NUM_DRIVERS; i++) {
        struct usb_class_driver* driver = &drivers[i];
        if(!is_active(*driver) || !has_if(*driver, interface) || driver->control_request == NULL) {
            continue;
        }

        /* Check for SET_INTERFACE and GET_INTERFACE */
        if((req->bRequestType & USB_RECIP_MASK) == USB_RECIP_INTERFACE &&
                (req->bRequestType & USB_TYPE_MASK) == USB_TYPE_STANDARD) {
            if(req->bRequest == USB_REQ_SET_INTERFACE) {
                logf("usb_core: SET INTERFACE 0x%x 0x%x", req->wValue, req->wIndex);
                if(driver->set_interface && driver->set_interface(req->wIndex, req->wValue) >= 0) {
                    usb_drv_control_response(USB_CONTROL_ACK, NULL, 0);
                    handled = true;
                }
                break;
            } else if(req->bRequest == USB_REQ_GET_INTERFACE) {
                int alt = -1;
                logf("usb_core: GET INTERFACE 0x%x", req->wIndex);

                if(drivers[i].get_interface)
                    alt = drivers[i].get_interface(req->wIndex);

                if(alt >= 0 && alt < 255) {
                    response_data[0] = alt;
                    usb_drv_control_response(USB_CONTROL_ACK, response_data, 1);
                    handled = true;
                }
                break;
            }
        }

        handled = driver->control_request(req, reqdata, response_data);
        break; /* no other driver can handle it because it's interface specific */
    }
    if(!handled) {
        /* nope. flag error */
        logf("bad req 0x%x:0x%x:0x%x:0x%x:0x%x", req->bRequestType,req->bRequest,
            req->wValue, req->wIndex, req->wLength);
        usb_drv_control_response(USB_CONTROL_STALL, NULL, 0);
    }
}

static void request_handler_device_get_descriptor(struct usb_ctrlrequest* req, void* reqdata)
{
    int size;
    const void* ptr = NULL;
    int length = req->wLength;
    int type = req->wValue >> 8;
    int index = req->wValue & 0xff;

    switch(type) {
        case USB_DT_DEVICE:
            ptr = &device_descriptor;
            size = sizeof(struct usb_device_descriptor);
            break;

        case USB_DT_OTHER_SPEED_CONFIG:
        case USB_DT_CONFIG: {
            if(index > NUM_CONFIGS) {
                logf("invalid config dt index %u", index);
                break;
            }
            int i, max_packet_size;

            if(type == USB_DT_CONFIG) {
                max_packet_size = (usb_drv_port_speed() ? 512 : 64);
                config_descriptor.bDescriptorType = USB_DT_CONFIG;
            }
            else {
                max_packet_size = (usb_drv_port_speed() ? 64 : 512);
                config_descriptor.bDescriptorType = USB_DT_OTHER_SPEED_CONFIG;
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

            for(i = 0; i < USB_NUM_DRIVERS; i++) {
                if(drivers[i].enabled && drivers[i].config == index + 1 && drivers[i].get_config_descriptor) {
                    size += drivers[i].get_config_descriptor(&response_data[size], max_packet_size);
                }
            }

            config_descriptor.bNumInterfaces = usb_core_num_interfaces[index];
            config_descriptor.bConfigurationValue = index + 1;
            config_descriptor.wTotalLength = (uint16_t)size;
            memcpy(&response_data[0], &config_descriptor, sizeof(struct usb_config_descriptor));

            ptr = response_data;
        } break;
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

static int usb_core_do_set_config(uint8_t new_config)
{
    logf("usb_core: SET_CONFIG %d to %d", usb_config, new_config);

    if(new_config > NUM_CONFIGS) {
        logf("usb_core: invalid config number");
        return -1;
    }

    /* deactivate old config */
    if(usb_config != 0) {
        for(int i = 0; i < USB_NUM_DRIVERS; i++) {
            if(is_active(drivers[i]) && drivers[i].disconnect != NULL) {
                drivers[i].disconnect();
            }
        }
        init_deinit_endpoints(usb_config - 1, false);

        /* clear any pending transfer completions,
         * because they are depend on contents of ep_data */
        usb_clear_pending_transfer_completion_events();
        /* reset endpoint states */
        memset(ep_data, 0, sizeof(ep_data));
    }

    usb_config = new_config;
    usb_state = usb_config == 0 ? ADDRESS : CONFIGURED;

    /* activate new config */
    if(usb_config != 0) {
        init_deinit_endpoints(usb_config - 1, true);
        for(int i = 0; i < USB_NUM_DRIVERS; i++) {
            if(is_active(drivers[i]) && drivers[i].init_connection != NULL) {
                drivers[i].init_connection();
            }
        }
    }

    #ifdef HAVE_USB_CHARGING_ENABLE
    usb_charging_maxcurrent_change(usb_charging_maxcurrent());
    #endif

    return 0;
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
            response_data[0] = usb_config;
            usb_drv_control_response(USB_CONTROL_ACK, response_data, 1);
            break;
        case USB_REQ_SET_CONFIGURATION:
            usb_drv_cancel_all_transfers();
            if(usb_core_do_set_config(req->wValue) == 0) {
                usb_drv_control_response(USB_CONTROL_ACK, NULL, 0);
            } else {
                usb_drv_control_response(USB_CONTROL_STALL, NULL, 0);
            }
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
        case USB_REQ_GET_INTERFACE:
            control_request_handler_drivers(req, reqdata);
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
        logf("bad req 0x%x:0x%x:0x%x:0x%x:0x%x", req->bRequestType,req->bRequest,
             req->wValue, req->wIndex, req->wLength);
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

static void do_bus_reset(void) {
    usb_address = 0;
    usb_state = DEFAULT;
#ifdef USB_LEGACY_CONTROL_API
    num_active_requests = 0;
#endif
    bus_reset_pending = false;
}

/* called by usb_drv_int() */
void usb_core_bus_reset(void)
{
    logf("usb_core: bus reset");
    if(bus_reset_pending) {
        return;
    }
    bus_reset_pending = true;
    if(usb_config == 0) {
        do_bus_reset();
    } else {
        /* need to disconnect class drivers, defer it to usb thread */
        usb_signal_notify(USB_NOTIFY_BUS_RESET, 0);
    }
}

/* called by usb_drv_transfer_completed() */
void usb_core_transfer_complete(int endpoint, int dir, int status, int length)
{
    struct usb_transfer_completion_event_data* completion_event =
        &ep_data[endpoint].completion_event[EP_DIR(dir)];
    /* Fast notification */
    fast_completion_handler_t handler = ep_data[endpoint].fast_completion_handler[EP_DIR(dir)];
    if(handler != NULL && handler(endpoint, dir, status, length))
        return; /* do not dispatch to the queue if handled */

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
        case USB_NOTIFY_BUS_RESET:
            usb_core_do_set_config(0);
            do_bus_reset();
#ifdef HAVE_USB_CHARGING_ENABLE
            usb_charging_maxcurrent_change(usb_charging_maxcurrent());
#endif
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

int usb_charging_maxcurrent(void)
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
