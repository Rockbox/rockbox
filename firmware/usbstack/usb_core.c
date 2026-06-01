/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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

#ifdef USB_ENABLE_IAP
#include "usb_iap.h"
#endif

/* include order matters, include driver header before usb_drv.h */
#if CONFIG_USBOTG == USBOTG_DESIGNWARE
#include "usb-designware.h"
#endif
#include "usb_drv.h"

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

#ifdef USB_ENABLE_IAP
#define NUM_CONFIGS 2
#else
#define NUM_CONFIGS 1
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
    .bNumConfigurations = NUM_CONFIGS
} ;

static struct usb_config_descriptor __attribute__((aligned(2)))
                                    config_descriptor =
{
    .bLength             = sizeof(struct usb_config_descriptor),
    .bDescriptorType     = USB_DT_CONFIG,
    .wTotalLength        = 0, /* will be filled in later */
    .bNumInterfaces      = 1,
    .bConfigurationValue = 0, /* will be filled in later */
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

static struct usb_ctrlrequest handling_request;
static struct usb_ctrlrequest pending_request;
static volatile bool have_pending_request;

/* control endpoint typical state flow
 * READY -setup_received(IN, wLength==0)->
 *           HANDLING_TX_CONTROL -control_response(size==0)->
 *           EXPECT_TX_STATUS_COMP -transfer_complete(OUT)->
 *           READY
 *       -setup_received(IN, wLength>0)->
 *           HANDLING_TX_CONTROL -control_response(size>0)->
 *           EXPECT_TX_DATA_STATUS_COMP
 *           -transfer_complete(OUT)->
 *               EXPECT_TX_DATA_COMP -transfer_complete(IN)->
 *               READY
 *           -transfer_complete(IN)->
 *               EXPECT_TX_STATUS_COMP -transfer_complete(OUT)->
 *               READY
 *       -setup_received(OUT, wLength==0)->
 *           HANDLING_RX_CONTROL -control_response(size==0)->
 *           EXPECT_RX_STATUS_COMP -transfer_complete(IN)->
 *           READY
 *       -setup_received(OUT, wLength>0)->
 *           HANDLING_RX_CONTROL ->
 *           EXPECT_RX_DATA_COMP -transfer_complete(OUT)->
 *           HANDLING_TX_CONTROL -control_response(size==0)->
 *           EXPECT_RX_STATUS_COMP -transfer_complete()->
 *           READY
 * */
enum {
    EP0_READY,                      /* waiting for new setup packet */
    /* IN request */
    EP0_HANDLING_TX_CONTROL,        /* control in received, usb thread is processing it */
    EP0_EXPECT_TX_DATA_STATUS_COMP, /* sending control in data phase */
    EP0_EXPECT_TX_DATA_COMP,        /* status packet received earlier than tx completion */
    EP0_EXPECT_TX_STATUS_COMP,      /* receiving status */
    /* OUT request */
    EP0_HANDLING_RX_CONTROL,        /* control out received, usb thread is processing it */
    EP0_EXPECT_RX_DATA_COMP,        /* receiving control out data phase */
    EP0_EXPECT_RX_STATUS_COMP,      /* sending status */
};
static volatile int ep0_state;

#define TRACE_EP0_STATE 0
#if TRACE_EP0_STATE == 1
#define set_ep0_state(new) \
    { \
        logf("usb_core:%d ep0_state %d -> %d", __LINE__, ep0_state, new); \
        ep0_state = new; \
    }
#else
#define set_ep0_state(new) {ep0_state = new;}
#endif

struct usb_transfer_completion_event_data
{
    struct usb_ctrlrequest* req;
    unsigned char ep;
    int status;
    int length;
};

typedef void (*completion_handler_t)(int ep, int dir, int status, int length);
typedef bool (*fast_completion_handler_t)(int ep, int dir, int status, int length);
typedef bool (*control_handler_t)(struct usb_ctrlrequest* req, uint8_t* reqdata, size_t reqdata_size);

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

struct config_state {
    struct usb_drv_ep_alloc_ctx ep_alloc_ctx;
    struct ep_alloc_state ep_alloc_states[USB_NUM_ENDPOINTS];
    uint8_t num_interfaces;
};

struct config_state config_states[NUM_CONFIGS];

static struct usb_class_driver* drivers[USB_NUM_DRIVERS] =
{
#ifdef USB_ENABLE_STORAGE
    [USB_DRIVER_MASS_STORAGE] = &usb_cdrv_storage,
#endif
#ifdef USB_ENABLE_SERIAL
    [USB_DRIVER_SERIAL] = &usb_cdrv_serial,
#endif
#ifdef USB_ENABLE_CHARGING_ONLY
    [USB_DRIVER_CHARGING_ONLY] = &usb_cdrv_charging_only,
#endif
#ifdef USB_ENABLE_HID
    [USB_DRIVER_HID] = &usb_cdrv_hid,
#endif
#ifdef USB_ENABLE_AUDIO
    [USB_DRIVER_AUDIO] = &usb_cdrv_audio,
#endif
#ifdef USB_ENABLE_IAP
    [USB_DRIVER_IAP] = &usb_cdrv_iap,
#endif
};

static int usb_core_do_set_config(uint8_t new_config);
static void usb_core_control_request_handler(struct usb_ctrlrequest* req, uint8_t* reqdata, size_t reqdata_size);

#define is_active(driver) ((driver)->enabled && !(driver)->error && (driver)->config == usb_config)
#define has_if(driver, interface) ((interface) >= (driver)->first_interface && (interface) < (driver)->last_interface)

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
    for(i = 0; i < USB_NUM_DRIVERS; i++) {
        drivers[i]->enabled = false;
        drivers[i]->error = false;
        drivers[i]->first_interface = 0;
        drivers[i]->last_interface = 0;
        if(drivers[i]->init != NULL) {
            drivers[i]->init();
        }
    }

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

/* for sending/receiving control data */
static uint8_t usb_control_data[256] USB_DEVBSS_ATTR;

void usb_core_handle_transfer_completion(
    struct usb_transfer_completion_event_data* event)
{
    int num = EP_NUM(event->ep);
    int dir = EP_DIR(event->ep);

    if(num == EP_CONTROL) {
        logf("ctrl handled %ld req=0x%x", current_tick,event->req->bRequest);
        usb_core_control_request_handler(event->req, usb_control_data, sizeof(usb_control_data));
        return;
    }

    completion_handler_t handler = ep_data[num].completion_handler[dir];
    if(handler != NULL)
        handler(num, dir == DIR_IN ? USB_DIR_IN : USB_DIR_OUT, event->status, event->length);
}

void usb_core_enable_driver(int driver, bool enabled)
{
    drivers[driver]->enabled = enabled;
}

bool usb_core_driver_enabled(int driver)
{
    return drivers[driver]->enabled;
}

#ifdef HAVE_HOTSWAP
void usb_core_hotswap_event(int volume, bool inserted)
{
    int i;
    for(i = 0; i < USB_NUM_DRIVERS; i++)
        if(is_active(drivers[i]) && drivers[i]->notify_hotswap != NULL)
            drivers[i]->notify_hotswap(volume, inserted);
}
#endif


#ifdef USB_BATCH_NON_NATIVE
static uint8_t batch_ep = 0;
static bool batch_stopped;
static usb_drv_batch_get_more batch_get_more;

int usb_drv_batch_init(int ep, usb_drv_batch_get_more get_more)
{
    if(batch_ep != 0) {
        logf("usb_core: batch api in use user=0x%02X", batch_ep);
        return -1;
    }
    batch_ep = ep;
    batch_get_more = get_more;
    batch_stopped  = false;
    return 0;
}

int usb_drv_batch_deinit(void)
{
    logf("usb_core: batch deinit");
    usb_drv_batch_stop();
    batch_ep = 0;
    return 0;
}

static int batch_get_and_send(void)
{
    const void* ptr;
    size_t len;
    batch_get_more(&ptr, &len);
    if(len == 0 || batch_stopped) {
        return 0;
    }
    return usb_drv_send_nonblocking(EP_NUM(batch_ep), (void*)ptr, len);
}

int usb_drv_batch_start(void)
{
    logf("usb_core: batch start");
    batch_stopped = false;
    return batch_get_and_send();
}

int usb_drv_batch_stop(void)
{
    batch_stopped = true;
    return 0;
}

static void batch_xfer_complete(void) {
    if(batch_stopped) {
        return;
    }
    batch_get_and_send();
}
#endif

static void usb_core_set_serial_function_id(void)
{
    int i, id = 0;

    for(i = 0; i < USB_NUM_DRIVERS; i++)
        if(drivers[i]->enabled)
            id |= 1 << i;

    usb_string_iSerial.wString[0] = hex[id];
}

/* synchronize endpoint initialization state to allocation state */
static void init_deinit_endpoints(int config, bool init) {
    for(int epnum = 0; epnum < USB_NUM_ENDPOINTS; epnum += 1) {
        for(int dir = 0; dir < 2; dir += 1) {
            struct config_state* cstate = &config_states[config - 1];
            struct ep_alloc_state* astate = &cstate->ep_alloc_states[epnum];
            struct usb_class_driver* driver = astate->owner[dir];
            if(driver == NULL) {
                continue;
            }
            int ep = epnum | (dir == DIR_OUT ? USB_DIR_OUT : USB_DIR_IN);
            if(init) {
                usb_drv_ep_init(&cstate->ep_alloc_ctx, ep);
                ep_data[epnum].completion_handler[dir] = driver->transfer_complete;
                ep_data[epnum].fast_completion_handler[dir] = driver->fast_transfer_complete;
                ep_data[epnum].control_handler[dir] = driver->control_request;
            } else {
                usb_drv_ep_deinit(&cstate->ep_alloc_ctx, ep);
            }
        }
    }
}

#ifndef usb_drv_ep_alloc_ctx
/* default endpoint allocator using usb_drv_ep_specs table */
static void usb_drv_ep_reset_alloc_ctx(struct usb_drv_ep_alloc_ctx* ctx) {
    for(int i = 0; i < USB_NUM_ENDPOINTS; i += 1) {
        ctx->type[i][0] = -1;
        ctx->type[i][1] = -1;
    }
}

static bool usb_drv_ep_allocate(struct usb_drv_ep_alloc_ctx* ctx, int ep, int type, int max_packet_size) {
    const uint8_t epnum = EP_NUM(ep);
    const uint8_t epdir = EP_DIR(ep);

    struct usb_drv_ep_spec* spec = &usb_drv_ep_specs[epnum];

    const int8_t spec_type = spec->type[epdir];
    if(spec_type != type && spec_type != USB_ENDPOINT_TYPE_ANY) {
        return false;
    }

    const int8_t other_type = ctx->type[epnum][!epdir];
    if(usb_drv_ep_specs_flags & USB_ENDPOINT_SPEC_IO_EXCLUSIVE && other_type != -1) {
        /* the other side is allocated */
        return false;
    }

    if(usb_drv_ep_specs_flags & USB_ENDPOINT_SPEC_FORCE_IO_TYPE_MATCH && other_type != -1 && other_type != type) {
        /* the other side is allocated with another type */
        return false;
    }

    ctx->type[epnum][epdir] = type;
    ctx->max_packet_size[epnum][epdir] = max_packet_size;

    return true;
}
#endif

static void allocate_interfaces_and_endpoints(void)
{
    if(usb_config != 0) {
        /* deinit currently used endpoints */
        init_deinit_endpoints(usb_config, false);
    }

retry:
    /* reset allocations */
    for(int i = 0; i < NUM_CONFIGS; i += 1) {
        config_states[i].num_interfaces = 0;
        memset(config_states[i].ep_alloc_states, 0, sizeof(config_states[i].ep_alloc_states));
        usb_drv_ep_reset_alloc_ctx(&config_states[i].ep_alloc_ctx);
    }

    for(int i = 0; i < USB_NUM_DRIVERS; i++) {
        struct usb_class_driver* driver = drivers[i];
        struct config_state* cstate = &config_states[driver->config - 1];

        if(!driver->enabled || driver->error) {
            continue;
        }

        /* assign endpoints */
        for(int reqnum = 0; reqnum < driver->ep_allocs_size; reqnum += 1) {
            /* find matching ep */
            struct usb_class_driver_ep_allocation* req = &driver->ep_allocs[reqnum];
            req->ep = 0;
            for(int epnum = 1; epnum < USB_NUM_ENDPOINTS; epnum += 1) {
                /* free check */
                struct ep_alloc_state* alloc = &cstate->ep_alloc_states[epnum];
                if(alloc->owner[req->dir] != NULL) {
                    continue;
                }

                /* driver specific check */
                const int ep = epnum | (req->dir == DIR_OUT ? USB_DIR_OUT : USB_DIR_IN);
                if(!usb_drv_ep_allocate(&cstate->ep_alloc_ctx, ep, req->type, req->mps)) {
                    continue;
                }

                /* all checks passed, assign it */
                req->ep = ep;
                alloc->owner[req->dir] = driver;
                alloc->type[req->dir] = req->type;
                break;
            }
            if(req->ep == 0 && !req->optional) {
                /* no matching ep found, retry allocation excluding this driver */
                logf("usb_core: no endpoint allocated for driver %d", i);
                driver->enabled = false;
                goto retry;
            }
        }

        /* assign interfaces */
        driver->first_interface = cstate->num_interfaces;
        cstate->num_interfaces = driver->set_first_interface(cstate->num_interfaces);
        driver->last_interface = cstate->num_interfaces;
    }
}

static void control_request_handler_drivers(struct usb_ctrlrequest* req, uint8_t* reqdata, size_t reqdata_size)
{
    int i, interface = req->wIndex & 0xff;
    bool handled = false;

    for(i = 0; i < USB_NUM_DRIVERS; i++) {
        struct usb_class_driver* driver = drivers[i];
        if(!is_active(driver) || !has_if(driver, interface) || driver->control_request == NULL) {
            continue;
        }

        /* Check for SET_INTERFACE and GET_INTERFACE */
        if((req->bRequestType & USB_RECIP_MASK) == USB_RECIP_INTERFACE &&
                (req->bRequestType & USB_TYPE_MASK) == USB_TYPE_STANDARD) {
            if(req->bRequest == USB_REQ_SET_INTERFACE) {
                logf("usb_core: SET INTERFACE 0x%x 0x%x", req->wValue, req->wIndex);
                if(driver->set_interface && driver->set_interface(req->wIndex, req->wValue) >= 0) {
                    usb_core_control_response(USB_CONTROL_ACK, NULL, 0);
                    handled = true;
                }
                break;
            } else if(req->bRequest == USB_REQ_GET_INTERFACE) {
                int alt = -1;
                logf("usb_core: GET INTERFACE 0x%x", req->wIndex);

                if(driver->get_interface)
                    alt = driver->get_interface(req->wIndex);

                if(alt >= 0 && alt < 255) {
                    reqdata[0] = alt;
                    usb_core_control_response(USB_CONTROL_ACK, reqdata, 1);
                    handled = true;
                }
                break;
            }
        }

        handled = driver->control_request(req, reqdata, reqdata_size);
        break; /* no other driver can handle it because it's interface specific */
    }
    if(!handled) {
        /* nope. flag error */
        logf("bad req 0x%x:0x%x:0x%x:0x%x:0x%x", req->bRequestType,req->bRequest,
            req->wValue, req->wIndex, req->wLength);
        usb_core_control_response(USB_CONTROL_STALL, NULL, 0);
    }
}

static void request_handler_device_get_descriptor(struct usb_ctrlrequest* req, uint8_t* reqdata, size_t reqdata_size)
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
                if(drivers[i]->enabled && drivers[i]->config == index + 1 && drivers[i]->get_config_descriptor) {
                    size += drivers[i]->get_config_descriptor(reqdata + size, max_packet_size);
                }
            }

            config_descriptor.bNumInterfaces = config_states[index].num_interfaces;
            config_descriptor.bConfigurationValue = index + 1;
            config_descriptor.wTotalLength = (uint16_t)size;
            memcpy(reqdata, &config_descriptor, sizeof(struct usb_config_descriptor));

            ptr = reqdata;
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
            control_request_handler_drivers(req, reqdata, reqdata_size);
            return;
    }

    if(ptr) {
        logf("data %d (%d)", size, length);
        length = MIN(size, length);

        if (ptr != reqdata)
            memcpy(reqdata, ptr, length);

        usb_core_control_response(USB_CONTROL_ACK, reqdata, length);
    } else {
        usb_core_control_response(USB_CONTROL_STALL, NULL, 0);
    }
}

static void usb_core_do_set_addr(uint8_t address)
{
    logf("usb_core: SET_ADR %d", address);
    usb_address = address;
    usb_state = ADDRESS;
}

#if !defined(HAVE_PRIORITY_SCHEDULING)
#define thread_set_priority(...)
#define thread_get_priority(...)
#endif /* HAVE_PRIORITY_SCHEDULING */

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
            if(is_active(drivers[i]) && drivers[i]->disconnect != NULL) {
                drivers[i]->disconnect();
            }
        }
        init_deinit_endpoints(usb_config, false);

        /* clear any pending transfer completions,
         * because they are depend on contents of ep_data */
        usb_clear_pending_transfer_completion_events();
        /* reset endpoint states */
        memset(ep_data, 0, sizeof(ep_data));
    }

    usb_config = new_config;
    usb_state = usb_config == 0 ? ADDRESS : CONFIGURED;

    bool require_exclusive = false;
    bool require_cpu_boost = false;

    /* activate new config */
    if(usb_config != 0) {
        init_deinit_endpoints(usb_config, true);
        for(int i = 0; i < USB_NUM_DRIVERS; i++) {
            if(!is_active(drivers[i])) {
                continue;
            }
            if(drivers[i]->init_connection != NULL && drivers[i]->init_connection() < 0) {
                drivers[i]->error = true;
                continue;
            }
            require_exclusive |= drivers[i]->needs_exclusive_storage;
            require_cpu_boost |= drivers[i]->needs_cpu_boost;
        }
    }

    if(require_exclusive) {
        if(!usb_exclusive_storage()) {
            usb_release_exclusive_storage();
            usb_request_exclusive_storage();
        }
    } else {
        usb_release_exclusive_storage();
    }
    if(require_cpu_boost) {
        trigger_cpu_boost();
        thread_set_priority(thread_self(), PRIORITY_REALTIME);
    } else {
        thread_set_priority(thread_self(), PRIORITY_SYSTEM);
        cancel_cpu_boost();
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

static void request_handler_device(struct usb_ctrlrequest* req, uint8_t* reqdata, size_t reqdata_size)
{
    unsigned address;

    switch(req->bRequest) {
        case USB_REQ_GET_CONFIGURATION:
            logf("usb_core: GET_CONFIG");
            reqdata[0] = usb_config;
            usb_core_control_response(USB_CONTROL_ACK, reqdata, 1);
            break;
        case USB_REQ_SET_CONFIGURATION:
            usb_drv_cancel_all_transfers();
            if(usb_core_do_set_config(req->wValue) == 0) {
                usb_core_control_response(USB_CONTROL_ACK, NULL, 0);
            } else {
                usb_core_control_response(USB_CONTROL_STALL, NULL, 0);
            }
            break;
        case USB_REQ_SET_ADDRESS:
            /* NOTE: We really have no business handling this and drivers
             * should just handle it themselves. We don't care beyond
             * knowing if we've been assigned an address yet, or not. */
            address = req->wValue;
            usb_drv_cancel_all_transfers();
            usb_core_control_response(USB_CONTROL_ACK, NULL, 0);
            usb_drv_set_address(address);
            usb_core_do_set_addr(address);
            break;
        case USB_REQ_GET_DESCRIPTOR:
            logf("usb_core: GET_DESC %d", req->wValue >> 8);
            request_handler_device_get_descriptor(req, reqdata, reqdata_size);
            break;
        case USB_REQ_SET_FEATURE:
            if(req->wValue==USB_DEVICE_TEST_MODE) {
                int mode = req->wIndex >> 8;
                usb_core_control_response(USB_CONTROL_ACK, NULL, 0);
                usb_drv_set_test_mode(mode);
            } else {
                usb_core_control_response(USB_CONTROL_STALL, NULL, 0);
            }
            break;
        case USB_REQ_GET_STATUS:
            reqdata[0] = 0;
            reqdata[1] = 0;
            usb_core_control_response(USB_CONTROL_ACK, reqdata, 2);
            break;
        #ifdef USB_ENABLE_IAP
        case USB_REQ_APPLE_SET_AVAIL_CURRENT:
            usb_core_control_response(USB_CONTROL_ACK, NULL, 0);
            break;
        #endif
        default:
            logf("bad req:desc %d:%d", req->bRequest, req->wValue);
            usb_core_control_response(USB_CONTROL_STALL, NULL, 0);
            break;
    }
}

static void request_handler_interface_standard(struct usb_ctrlrequest* req, uint8_t* reqdata, size_t reqdata_size)
{
    switch (req->bRequest)
    {
        case USB_REQ_SET_INTERFACE:
            logf("usb_core: SET_INTERFACE");
        case USB_REQ_GET_INTERFACE:
            control_request_handler_drivers(req, reqdata, reqdata_size);
            break;
        case USB_REQ_GET_STATUS:
            reqdata[0] = 0;
            reqdata[1] = 0;
            usb_core_control_response(USB_CONTROL_ACK, reqdata, 2);
            break;
        case USB_REQ_CLEAR_FEATURE:
        case USB_REQ_SET_FEATURE:
            /* TODO: These used to be ignored (erroneously).
             * Should they be passed to the drivers instead? */
            usb_core_control_response(USB_CONTROL_STALL, NULL, 0);
            break;
        default:
            control_request_handler_drivers(req, reqdata, reqdata_size);
            break;
    }
}

static void request_handler_interface(struct usb_ctrlrequest* req, uint8_t* reqdata, size_t reqdata_size)
{
    switch(req->bRequestType & USB_TYPE_MASK) {
        case USB_TYPE_STANDARD:
            request_handler_interface_standard(req, reqdata, reqdata_size);
            break;
        case USB_TYPE_CLASS:
            control_request_handler_drivers(req, reqdata, reqdata_size);
            break;
        case USB_TYPE_VENDOR:
        default:
            logf("bad req:desc %d", req->bRequest);
            usb_core_control_response(USB_CONTROL_STALL, NULL, 0);
            break;
    }
}

static void request_handler_endpoint_drivers(struct usb_ctrlrequest* req, uint8_t* reqdata, size_t reqdata_size)
{
    bool handled = false;
    control_handler_t control_handler = NULL;

    if(EP_NUM(req->wIndex) < USB_NUM_ENDPOINTS)
        control_handler =
            ep_data[EP_NUM(req->wIndex)].control_handler[EP_DIR(req->wIndex)];

    if(control_handler)
        handled = control_handler(req, reqdata, reqdata_size);

    if(!handled) {
        /* nope. flag error */
        logf("bad req 0x%x:0x%x:0x%x:0x%x:0x%x", req->bRequestType,req->bRequest,
             req->wValue, req->wIndex, req->wLength);
        usb_core_control_response(USB_CONTROL_STALL, NULL, 0);
    }
}

static void request_handler_endpoint_standard(struct usb_ctrlrequest* req, uint8_t* reqdata, size_t reqdata_size)
{
    switch (req->bRequest) {
        case USB_REQ_CLEAR_FEATURE:
            usb_core_do_clear_feature(USB_RECIP_ENDPOINT,
                                      req->wIndex,
                                      req->wValue);
            usb_core_control_response(USB_CONTROL_ACK, NULL, 0);
            break;
        case USB_REQ_SET_FEATURE:
            logf("usb_core: SET FEATURE (%d)", req->wValue);
            if(req->wValue == USB_ENDPOINT_HALT)
               usb_drv_stall(EP_NUM(req->wIndex), true, EP_DIR(req->wIndex));

            usb_core_control_response(USB_CONTROL_ACK, NULL, 0);
            break;
        case USB_REQ_GET_STATUS:
            reqdata[0] = 0;
            reqdata[1] = 0;
            logf("usb_core: GET_STATUS");
            if(req->wIndex > 0)
                reqdata[0] = usb_drv_stalled(EP_NUM(req->wIndex),
                                             EP_DIR(req->wIndex));

            usb_core_control_response(USB_CONTROL_ACK, reqdata, 2);
            break;
        default:
            request_handler_endpoint_drivers(req, reqdata, reqdata_size);
            break;
    }
}

static void request_handler_endpoint(struct usb_ctrlrequest* req, uint8_t* reqdata, size_t reqdata_size)
{
    switch(req->bRequestType & USB_TYPE_MASK) {
        case USB_TYPE_STANDARD:
            request_handler_endpoint_standard(req, reqdata, reqdata_size);
            break;
        case USB_TYPE_CLASS:
            request_handler_endpoint_drivers(req, reqdata, reqdata_size);
            break;
        case USB_TYPE_VENDOR:
        default:
            logf("bad req:desc %d", req->bRequest);
            usb_core_control_response(USB_CONTROL_STALL, NULL, 0);
            break;
    }
}

/* Handling USB requests starts here */
static void usb_core_control_request_handler(struct usb_ctrlrequest* req, uint8_t* reqdata, size_t reqdata_size)
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
            request_handler_device(req, reqdata, reqdata_size);
            break;
        case USB_RECIP_INTERFACE:
            request_handler_interface(req, reqdata, reqdata_size);
            break;
        case USB_RECIP_ENDPOINT:
            request_handler_endpoint(req, reqdata, reqdata_size);
            break;
        default:
            logf("unsupported recipient");
            usb_core_control_response(USB_CONTROL_STALL, NULL, 0);
            break;
    }
}

static void do_bus_reset(void) {
    usb_address = 0;
    usb_state = DEFAULT;
    bus_reset_pending = false;
    set_ep0_state(EP0_READY);
    have_pending_request = false;
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

static void signal_xfer_complete(int ep, struct usb_ctrlrequest* req, int status, int length) {
    struct usb_transfer_completion_event_data* completion_event =
        &ep_data[EP_NUM(ep)].completion_event[EP_DIR(ep)];

    completion_event->req = req;
    completion_event->ep = ep;
    completion_event->status = status;
    completion_event->length = length;
    usb_signal_transfer_completion(completion_event);
}

static void process_setup_request(struct usb_ctrlrequest* req) {
    set_ep0_state(req->bRequestType & USB_DIR_IN ? EP0_HANDLING_TX_CONTROL : EP0_HANDLING_RX_CONTROL);

    if(ep0_state == EP0_HANDLING_TX_CONTROL || req->wLength == 0) {
        signal_xfer_complete(EP_CONTROL | USB_DIR_IN, req, 0, 0);
        return;
    }

    /* start control out data phase without usb thread interaction */
    if(req->wLength > sizeof(usb_control_data)) {
        logf("usb_core: control write too large %u > %u", req->wLength, sizeof(usb_control_data));
        usb_drv_stall(EP_CONTROL, true, false);
        return;
    }
    set_ep0_state(EP0_EXPECT_RX_DATA_COMP);
    usb_drv_recv_nonblocking(EP_CONTROL, usb_control_data, req->wLength);
    return;
}

static bool check_for_new_setup(void) {
    int oldlevel = disable_irq_save();
    if(!have_pending_request) {
        restore_irq(oldlevel);
        return false;
    }
    have_pending_request = false;
    handling_request = pending_request;
    process_setup_request(&handling_request);
    restore_irq(oldlevel);
    return true;
}

/* called by usb_drv_transfer_completed() */
void usb_core_transfer_complete(int endpoint, int dir, int status, int length) {
#ifdef USB_BATCH_NON_NATIVE
    /* batch api */
    if(batch_ep != 0 && (endpoint | dir) == batch_ep) {
        batch_xfer_complete();
        return;
    }
#endif

    /* Fast notification */
    fast_completion_handler_t handler = ep_data[endpoint].fast_completion_handler[EP_DIR(dir)];
    if(handler != NULL && handler(endpoint, dir, status, length)) {
        return; /* do not dispatch to the queue if handled */
    }

    /* Non-control packet handling */
    if(endpoint != EP_CONTROL) {
        signal_xfer_complete(endpoint | dir, NULL, status, length);
        return;
    }

    /* Control packet handling */
    switch(dir | ep0_state) {
    /* EXPECT_TX_DATA_STATUS_COMP -(status comp)-> EXPECT_TX_DATA_COMP -(data comp)-> READY 
     *                            -(data comp)-> EXPECT_TX_STATUS_COMP -(status comp)-> READY */
    case USB_DIR_OUT | EP0_EXPECT_TX_DATA_STATUS_COMP:
        logf("usb_core: control-in done success=%d", status == 0 && length == 0);
        set_ep0_state(EP0_EXPECT_TX_DATA_COMP);
        break;
    case USB_DIR_IN | EP0_EXPECT_TX_DATA_STATUS_COMP:
        set_ep0_state(EP0_EXPECT_TX_STATUS_COMP);
        break;
    case USB_DIR_OUT | EP0_EXPECT_TX_STATUS_COMP:
        logf("usb_core: control-in done success=%d", status == 0 && length == 0);
        set_ep0_state(EP0_READY);
        break;
    case USB_DIR_IN | EP0_EXPECT_TX_DATA_COMP:
        set_ep0_state(EP0_READY);
        break;
    /* EXPECT_RX_DATA_COMP -(data comp)-> HANDLING_RX_CONTROL [-(send status)-> EXPECT_RX_STATUS_COMP] -(status comp)-> READY
     *                                                        ^ done in control_response() */
    case USB_DIR_OUT | EP0_EXPECT_RX_DATA_COMP:
        set_ep0_state(EP0_HANDLING_RX_CONTROL);
        signal_xfer_complete(EP_CONTROL | USB_DIR_OUT, &handling_request, status, length);
        break;
    case USB_DIR_IN | EP0_EXPECT_RX_STATUS_COMP:
        logf("usb_core: control-out done success=%d", status == 0 && length == 0);
        set_ep0_state(EP0_READY);
        break;
    default:
        panicf("unhandled endpoint xfer completion ep0_state=%d dir=%d", ep0_state, dir);
        break;
    }

    if(ep0_state == EP0_READY) {
        check_for_new_setup();
    }
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
        case USB_NOTIFY_CLASS_DRIVER: {
            /* HACK: index is uint8 but promoted to int to avoid a compiler
               warning when USB_NUM_DRIVERS is 0, mainly in bootloaders.
               This hack can be removed once usb_core is no longer built
               for BOOTLOADER && !HAVE_BOOTLOADER_USB_MODE */
            int index = data >> 24;
            if(index < 0 || index >= USB_NUM_DRIVERS) {
                logf("usb_core: invalid notification destination index=%u", index);
                return;
            }
            if(is_active(drivers[index]) && drivers[index]->notify_event != NULL) {
                drivers[index]->notify_event(data & 0x00ffffff);
            }
        } break;
        default:
            break;
    }
}

void usb_core_setup_received(struct usb_ctrlrequest* req) {
    if(bus_reset_pending) {
        logf("usb_core: bus resetting tick=%lu", current_tick);
        return;
    }
    if(ep0_state != EP0_READY) {
        logf("usb_core: control pending tick=%lu", current_tick);
        pending_request = *req;
        have_pending_request = true;
        return;
    }
    handling_request = *req;
    process_setup_request(&handling_request);
}

void usb_core_control_response(enum usb_control_response response, const void* data, size_t size) {
    logf("usb_core: response ack=%d size=%u ep0_state=%d tick=%lu", response, size, ep0_state, current_tick);

    if((ep0_state == EP0_HANDLING_TX_CONTROL || ep0_state == EP0_HANDLING_RX_CONTROL) && check_for_new_setup()) {
        return;
    }

    if(response == USB_CONTROL_STALL) {
        set_ep0_state(EP0_READY);
        usb_drv_stall(EP_CONTROL, true, true);
        return;
    }

    switch(ep0_state) {
    case EP0_HANDLING_TX_CONTROL:
        if(size == 0) {
            /* non-data control-in */
            set_ep0_state(EP0_EXPECT_TX_STATUS_COMP);
            usb_drv_recv_nonblocking(EP_CONTROL, NULL, 0);
        } else {
            /* control-in data phase */
            set_ep0_state(EP0_EXPECT_TX_DATA_STATUS_COMP);
            /* prepare for a status packet before sending data.
             * note that it is driver and host-dependent that which completion
             * of the following two commands is notified first. */
            usb_drv_recv_nonblocking(EP_CONTROL, NULL, 0);
            /* send data packets */
            usb_drv_send_nonblocking(EP_CONTROL, (void*)data, size);
        }
        break;
    case EP0_HANDLING_RX_CONTROL:
        if(size == 0) {
            /* non-data control-out */
            set_ep0_state(EP0_EXPECT_RX_STATUS_COMP);
            /* send status packet*/
            usb_drv_send_nonblocking(EP_CONTROL, NULL, 0);
        } else {
            /* control-out data phase */
            /* should not happen, we've received it internally */
            logf("usb_core: receiving control data by class drivers is not allowed");
        }
        break;
    default:
        panicf("usb_core: invalid control response ep_state=%d", ep0_state);
        break;
    }
}

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
