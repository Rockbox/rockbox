/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2025 by Sho Tanimoto
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
#include "audio.h"
#include "pcm_mixer.h"
#include "pcm_sink.h"
#include "playback.h"
#include "powermgmt.h"
#include "timefuncs.h"
#include "usb_drv.h"

#include "iap/audio.h"
#include "iap/libiap/iap.h"
#include "iap/libiap/platform.h"
#include "iap/macros.h"
#include "iap/platform-macros.h"
#include "iap/platform.h"
#include "usb_audio_def.h"
#include "usb_class_driver.h"
#include "usb_hid_def.h"
#include "usb_iap.h"

struct usb_class_driver_ep_allocation usb_iap_ep_allocs[2] = {
    /* uac input */
    {.type = USB_ENDPOINT_XFER_ISOC, .dir = DIR_IN, .optional = false},
    /* hid input */
    {.type = USB_ENDPOINT_XFER_INT, .dir = DIR_IN, .optional = false},
};

/* interface 0 (audio control) */
static struct usb_interface_descriptor ipod_audio_control_desc = {
    .bLength            = USB_DT_INTERFACE_SIZE,
    .bDescriptorType    = USB_DT_INTERFACE,
    .bInterfaceNumber   = -1, /* dynamic */
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 0,
    .bInterfaceClass    = USB_CLASS_AUDIO,
    .bInterfaceSubClass = USB_SUBCLASS_AUDIO_CONTROL,
};

static struct usb_ac_header ipod_audio_control_uac_header = {
    .bLength            = USB_AC_SIZEOF_HEADER(1),
    .bDescriptorType    = USB_DT_CS_INTERFACE,
    .bDescriptorSubType = USB_AC_HEADER,
    .bcdADC             = 0x0100, /* 1.00 */
    .wTotalLength       = -1,     /* dynamic */
    .bInCollection      = 1,
    .baInterfaceNr      = {-1}, /* dynamic */
};

static struct usb_ac_input_terminal ipod_audio_control_uac_input_terminal = {
    .bLength            = sizeof(struct usb_ac_input_terminal),
    .bDescriptorType    = USB_DT_CS_INTERFACE,
    .bDescriptorSubType = USB_AC_INPUT_TERMINAL,
    .bTerminalId        = 1,
    .wTerminalType      = USB_AC_INPUT_TERMINAL_MICROPHONE,
    .bAssocTerminal     = 2, /* ipod_audio_control_uac_output_terminal */
    .bNrChannels        = 2,
    .wChannelConfig     = USB_AC_CHANNELS_LEFT_RIGHT_FRONT,
};

static struct usb_ac_output_terminal ipod_audio_control_uac_output_terminal = {
    .bLength            = sizeof(struct usb_ac_output_terminal),
    .bDescriptorType    = USB_DT_CS_INTERFACE,
    .bDescriptorSubType = USB_AC_OUTPUT_TERMINAL,
    .bTerminalId        = 2,
    .wTerminalType      = USB_AC_TERMINAL_STREAMING,
    .bAssocTerminal     = 1, /* ipod_audio_control_uac_input_terminal */
    .bSourceId          = 1,
};

/* interface 1 (audio stream) */
/* interface 1 alt 0 */
static struct usb_interface_descriptor ipod_audio_stream_0_desc = {
    .bLength            = USB_DT_INTERFACE_SIZE,
    .bDescriptorType    = USB_DT_INTERFACE,
    .bInterfaceNumber   = -1, /* dynamic */
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 0,
    .bInterfaceClass    = USB_CLASS_AUDIO,
    .bInterfaceSubClass = USB_SUBCLASS_AUDIO_STREAMING,
};

/* interface 1 alt 1 */
static struct usb_interface_descriptor ipod_audio_stream_1_desc = {
    .bLength            = USB_DT_INTERFACE_SIZE,
    .bDescriptorType    = USB_DT_INTERFACE,
    .bInterfaceNumber   = -1, /* dynamic */
    .bAlternateSetting  = 1,
    .bNumEndpoints      = 1,
    .bInterfaceClass    = USB_CLASS_AUDIO,
    .bInterfaceSubClass = USB_SUBCLASS_AUDIO_STREAMING,
};

static struct usb_as_interface ipod_audio_stream_1_uac_header = {
    .bLength            = sizeof(struct usb_as_interface),
    .bDescriptorType    = USB_DT_CS_INTERFACE,
    .bDescriptorSubType = USB_AS_GENERAL,
    .bTerminalLink      = 2, /* ipod_audio_control_uac_output_terminal */
    .bDelay             = 1,
    .wFormatTag         = USB_AS_FORMAT_TYPE_I_PCM,
};

/* TODO: remove unsupported freqs */
static struct usb_as_format_type_i_discrete ipod_audio_stream_1_uac_discrete = {
    .bLength            = USB_AS_SIZEOF_FORMAT_TYPE_I_DISCRETE(9),
    .bDescriptorType    = USB_DT_CS_INTERFACE,
    .bDescriptorSubType = USB_AS_FORMAT_TYPE,
    .bFormatType        = USB_AS_FORMAT_TYPE_I,
    .bNrChannels        = 2,
    .bSubframeSize      = 2, /* bBitResolution / 8 */
    .bBitResolution     = 16,
    .bSamFreqType       = 9,
    .tSamFreq           = {
        {0x40, 0x1F, 0x00}, /* 8000  */
        {0x11, 0x2B, 0x00}, /* 11025 */
        {0xE0, 0x2E, 0x00}, /* 12000 */
        {0x80, 0x3E, 0x00}, /* 16000 */
        {0x22, 0x56, 0x00}, /* 22050 */
        {0xC0, 0x5D, 0x00}, /* 24000 */
        {0x00, 0x7D, 0x00}, /* 32000 */
        {0x44, 0xAC, 0x00}, /* 44100 */
        {0x80, 0xBB, 0x00}, /* 48000 */
    },
};

/* interface 1 endpoint 0 */
static struct usb_as_iso_audio_endpoint ipod_audio_stream_1_endpoint = {
    .bLength          = USB_DT_ENDPOINT_AUDIO_SIZE,
    .bDescriptorType  = USB_DT_ENDPOINT,
    .bEndpointAddress = -1, /* dynamic */
    .bmAttributes     = USB_ENDPOINT_XFER_ISOC,
    .wMaxPacketSize   = AS_PACKET_SIZE,
    .bInterval        = -1, /* dynamic */
    .bRefresh         = 0,
    .bSynchAddress    = 0,
};

static struct usb_as_iso_ctrldata_endpoint ipod_audio_stream_1_endpoint_uac = {
    .bLength            = sizeof(struct usb_as_iso_ctrldata_endpoint),
    .bDescriptorType    = USB_DT_CS_ENDPOINT,
    .bDescriptorSubType = USB_AS_EP_GENERAL,
    .bmAttributes       = USB_AS_EP_CS_SAMPLING_FREQ_CTL,
    .bLockDelayUnits    = 0,
    .wLockDelay         = 0,
};

/* interface 2 (hid) */
static struct usb_interface_descriptor ipod_hid_desc = {
    .bLength            = USB_DT_INTERFACE_SIZE,
    .bDescriptorType    = USB_DT_INTERFACE,
    .bInterfaceNumber   = -1, /* dynamic */
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 1,
    .bInterfaceClass    = USB_CLASS_HID,
    .bInterfaceSubClass = 0,
};

#define INPUT_REPORT(id, count) 0x09, 0x01,      /* Usage 0x01 */   \
                                0x85, id,        /* Report ID */    \
                                0x95, count,     /* Report Count */ \
                                0x82, 0x02, 0x01 /* Input 0x0102 (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Buffered Bytes) */

#define OUTPUT_REPORT(id, count) 0x09, 0x01,      /* Usage 0x01 */   \
                                 0x85, id,        /* Report ID */    \
                                 0x95, count,     /* Report Count */ \
                                 0x92, 0x02, 0x01 /* Output 0x0102 (...) */

#define INPUT_REPORT2(id, count1, count2) 0x09, 0x01,           /* Usage 0x01 */   \
                                          0x85, id,             /* Report ID */    \
                                          0x96, count1, count2, /* Report Count */ \
                                          0x82, 0x02, 0x01      /* Input 0x0102 (...) */

#define OUTPUT_REPORT2(id, count1, count2) 0x09, 0x01,           /* Usage 0x01 */   \
                                           0x85, id,             /* Report ID */    \
                                           0x96, count1, count2, /* Report Count */ \
                                           0x92, 0x02, 0x01      /* Output 0x0102 (...) */

// clang-format off
static const uint8_t ipod_hid_report_fs[] = {
    0x06, 0x00, 0xFF, /* Usage Page 0xFF00 (Vendor-defined) */
    0x09, 0x01,       /* Usage 0x01 */
    0xA1, 0x01,       /* Collection 0x01 (Application) */
    0x75, 0x08,       /* Report Size 0x08 */
    0x26, 0x80, 0x00, /* Logical Maximum 0x0081 (128) */
    0x15, 0x00,       /* Logical Minumum 0x0000 (0) */

    INPUT_REPORT(0x01, 0x0C),
    INPUT_REPORT(0x02, 0x0E),
    INPUT_REPORT(0x03, 0x14),
    INPUT_REPORT(0x04, 0x3F),

    OUTPUT_REPORT(0x05, 0x08),
    OUTPUT_REPORT(0x06, 0x0A),
    OUTPUT_REPORT(0x07, 0x0E),
    OUTPUT_REPORT(0x08, 0x14),
    OUTPUT_REPORT(0x09, 0x3F),

    0xC0, /* End Collection */
};

static const uint8_t ipod_hid_report_hs[] = {
    0x06, 0x00, 0xFF, /* Usage Page 0xFF00 (Vendor-defined) */
    0x09, 0x01,       /* Usage 0x01 */
    0xA1, 0x01,       /* Collection 0x01 (Application) */
    0x75, 0x08,       /* Report Size 0x08 */
    0x26, 0x80, 0x00, /* Logical Maximum 0x0081 (128) */
    0x15, 0x00,       /* Logical Minumum 0x0000 (0) */

    INPUT_REPORT(0x01, 0x05),
    INPUT_REPORT(0x02, 0x09),
    INPUT_REPORT(0x03, 0x0D),
    INPUT_REPORT(0x04, 0x11),
    INPUT_REPORT(0x05, 0x19),
    INPUT_REPORT(0x06, 0x31),
    INPUT_REPORT(0x07, 0x5F),
    INPUT_REPORT(0x08, 0xC1),
    INPUT_REPORT2(0x09, 0x01, 0x01),
    INPUT_REPORT2(0x0A, 0x81, 0x01),
    INPUT_REPORT2(0x0B, 0x01, 0x02),
    INPUT_REPORT2(0x0C, 0xFF, 0x02),

    OUTPUT_REPORT(0x0D, 0x05),
    OUTPUT_REPORT(0x0E, 0x09),
    OUTPUT_REPORT(0x1F, 0x0D),
    OUTPUT_REPORT(0x10, 0x11),
    OUTPUT_REPORT(0x11, 0x19),
    OUTPUT_REPORT(0x12, 0x31),
    OUTPUT_REPORT(0x13, 0x5F),
    OUTPUT_REPORT(0x14, 0xC1),
    OUTPUT_REPORT(0x15, 0xFF),

    0xC0, /* End Collection */
};
// clang-format on

static struct usb_hid_descriptor ipod_hid_hid_desc = {
    .bLength            = sizeof(struct usb_hid_descriptor),
    .bDescriptorType    = USB_DT_HID,
    .wBcdHID            = 0x0111, /* 1.11 */
    .bCountryCode       = 0,
    .bNumDescriptors    = 1,
    .bDescriptorType0   = USB_DT_REPORT,
    .wDescriptorLength0 = -1, /* dynamic */
};

/* interface 2 endpoint 0 */
static struct usb_endpoint_descriptor ipod_hid_endpoint = {
    .bLength          = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType  = USB_DT_ENDPOINT,
    .bEndpointAddress = -1, /* dynamic */
    .bmAttributes     = USB_ENDPOINT_XFER_INT,
    .wMaxPacketSize   = -1, /* dynamic */
    .bInterval        = 1,
};

static struct {
    int interface;
} ctrl;

static struct {
    uint32_t sample_rate;
    int      interface;
    int8_t   alt;
} stream;

static struct {
    int interface;
} hid;

static struct mutex    iap_ctx_mutex;
static struct Platform platform;
static struct timeout  tick_tmo;
static bool            iap_ctx_mutex_initialized = false;

struct IAPContext* _iap_acquire_ctx(bool lock) {
    static struct IAPContext ctx;
    if(lock) {
        mutex_lock(&iap_ctx_mutex);
    }
    return &ctx;
}

void _iap_release_ctx() {
    mutex_unlock(&iap_ctx_mutex);
}

bool iap_initialized;

/* those notifications need pollling */
static enum charge_state_type last_charge_state;
static uint8_t                last_battery_level;
static int8_t                 last_minute;
static int8_t                 last_hold_switch_state;

enum Notify {
    Notify_Tick,
};

static int tick_callback(struct timeout* tmo) {
    (void)tmo;
    usb_signal_class_notify(USB_DRIVER_IAP, Notify_Tick);
    return HZ / 10;
}

int usb_iap_set_first_interface(int interface) {
    ctrl.interface   = interface + 0;
    stream.interface = interface + 1;
    hid.interface    = interface + 2;
    return interface + 3;
}

#define PACK_DESC(desc) pack_data(&dest, &desc, ((struct usb_descriptor_header*)&desc)->bLength)

int usb_iap_get_config_descriptor(unsigned char* dest, int max_packet_size) {
    (void)max_packet_size;

    unsigned char* orig_dest = dest;

    ipod_audio_control_desc.bInterfaceNumber = ctrl.interface;
    PACK_DESC(ipod_audio_control_desc);
    ipod_audio_control_uac_header.baInterfaceNr[0] = stream.interface;
    ipod_audio_control_uac_header.wTotalLength =
        sizeof(ipod_audio_control_uac_header) +
        sizeof(ipod_audio_control_uac_input_terminal) +
        sizeof(ipod_audio_control_uac_output_terminal);
    PACK_DESC(ipod_audio_control_uac_header);
    PACK_DESC(ipod_audio_control_uac_input_terminal);
    PACK_DESC(ipod_audio_control_uac_output_terminal);

    ipod_audio_stream_0_desc.bInterfaceNumber     = stream.interface;
    ipod_audio_stream_1_desc.bInterfaceNumber     = stream.interface;
    ipod_audio_stream_1_endpoint.bEndpointAddress = AS_EP_IN;
    ipod_audio_stream_1_endpoint.bInterval        = usb_drv_port_speed() ? 4 : 1;
    PACK_DESC(ipod_audio_stream_0_desc);
    PACK_DESC(ipod_audio_stream_1_desc);
    PACK_DESC(ipod_audio_stream_1_uac_header);
    PACK_DESC(ipod_audio_stream_1_uac_discrete);
    PACK_DESC(ipod_audio_stream_1_endpoint);
    PACK_DESC(ipod_audio_stream_1_endpoint_uac);

    ipod_hid_desc.bInterfaceNumber     = hid.interface;
    ipod_hid_endpoint.bEndpointAddress = HID_EP_IN;
    if(usb_drv_port_speed()) {
        ipod_hid_hid_desc.wDescriptorLength0 = sizeof(ipod_hid_report_hs);
        ipod_hid_endpoint.wMaxPacketSize     = 64;
    } else {
        ipod_hid_hid_desc.wDescriptorLength0 = sizeof(ipod_hid_report_fs);
        ipod_hid_endpoint.wMaxPacketSize     = 64;
    }
    PACK_DESC(ipod_hid_desc);
    PACK_DESC(ipod_hid_hid_desc);
    PACK_DESC(ipod_hid_endpoint);

    return dest - orig_dest;
}

void usb_iap_init_connection(void) {
    stream.sample_rate     = 48000;
    last_charge_state      = -1;
    last_minute            = -1;
    last_hold_switch_state = -1;

    iap_debug_reset_timestamp();

    /* TODO: disable iap on error */

    /* init audio sink */
    check_act(iap_audio_init(), return);

    /* init libiap */
    if(!iap_ctx_mutex_initialized) {
        iap_ctx_mutex_initialized = true;
        mutex_init(&iap_ctx_mutex);
    }

    struct IAPContext* ctx = _iap_acquire_ctx(true);

    const struct IAPOpts opts = {
        .usb_highspeed         = usb_drv_port_speed(),
        .ignore_hid_report_id  = iap_true,
        .artwork_single_report = iap_false,
        .enable_packet_dump    = iap_false,
    };
    check_act(iap_init_ctx(ctx, opts, &platform), goto cleanup_audio);
    _iap_release_ctx();

    /* prepare artwork */
    struct dim dim   = {IAP_ARTWORK_WIDTH, IAP_ARTWORK_HEIGHT};
    platform.aa_slot = playback_claim_aa_slot(&dim);
    if(platform.aa_slot < 0) {
        ERROR("failed to claim albumart slot");
    }
    platform.control_pending = false;

    /* register timer */
    timeout_register(&tick_tmo, tick_callback, HZ / 10, 0);

    iap_initialized = true;
    LOG("initialized");
    return;

cleanup_audio:
    _iap_release_ctx();
    iap_audio_deinit();
}

int usb_iap_set_interface(int intf, int alt) {
    LOG("set interface interface=%d alt=%d", intf, alt);
    check_act(intf == stream.interface, return -1);
    if(alt == 0) {
        check_act(iap_audio_disable(), return -1);
    } else if(alt == 1) {
        check_act(iap_audio_enable(), return -1);
    } else {
        ERROR("invalid alt %d", alt);
        return -1;
    }
    return 0;
}

int usb_iap_get_interface(int intf) {
    LOG("get interface interface=%d", intf);
    check_act(intf == stream.interface, return -1);
    return stream.alt;
}

int usb_iap_get_max_packet_size(int ep) {
    if(ep == AS_EP_IN) {
        return 1024;
    } else if(ep == HID_EP_IN) {
        return 64;
    } else {
        panicf("unexpected endpoint number %d", ep);
    }
}

void usb_iap_init(void) {
    LOG("init");
}

void usb_iap_disconnect(void) {
    iap_initialized = false;
    audio_pause();
    mixer_switch_sink(PCM_SINK_BUILTIN);
    timeout_cancel(&tick_tmo);
    if(platform.aa_slot >= 0) {
        playback_release_aa_slot(platform.aa_slot);
    }
    struct IAPContext* ctx = _iap_acquire_ctx(true);
    check_act(iap_deinit_ctx(ctx), );
    _iap_release_ctx();
    check_act(iap_audio_deinit(), );
    LOG("disconnected");
}

void usb_iap_transfer_complete(int ep, int dir, int status, int length) {
    (void)length;

    if((ep | dir) == HID_EP_IN) {
        check_act(status == 0, return);
#if DEBUG_DUMP_TX
        LOG("ep=%d dir=%d state=%d length=%d", ep, dir, status, length);
#endif
        struct IAPContext* ctx = _iap_acquire_ctx(true);
        check_act(iap_notify_send_complete(ctx), );
        _iap_release_ctx();
    }
}

bool usb_iap_fast_transfer_complete(int ep, int dir, int status, int length) {
    (void)status;
    (void)length;
    return (ep | dir) == AS_EP_IN;
}

static unsigned char ctrl_buf[256] USB_DEVBSS_ATTR;

static void respond_zero(struct usb_ctrlrequest* req) {
    if(req->wLength > sizeof(ctrl_buf)) {
        ERROR("required data too long %u > %u", req->wLength, sizeof(ctrl_buf));
        usb_drv_control_response(USB_CONTROL_STALL, NULL, 0);
    } else {
        memset(ctrl_buf, 0, req->wLength);
        usb_drv_control_response(USB_CONTROL_ACK, ctrl_buf, req->wLength);
    }
}

/* returns true when ctrl_buf has received data */
static bool receive_data(struct usb_ctrlrequest* req, void* reqdata) {
    if(reqdata == NULL) {
        /* setup */
        if(req->wLength > sizeof(ctrl_buf)) {
            ERROR("parameter too long");
            usb_drv_control_response(USB_CONTROL_STALL, NULL, 0);
        } else {
            usb_drv_control_response(USB_CONTROL_RECEIVE, ctrl_buf, req->wLength);
        }
        return false;
    } else {
        /* data */
        return true;
    }
}

static bool control_request_if_std(struct usb_ctrlrequest* req, void* reqdata, unsigned char* dest) {
    (void)reqdata;

    unsigned char* const orig_dest = dest;
    switch(req->bRequest) {
    case USB_REQ_GET_DESCRIPTOR: {
        const uint8_t desc_type  = req->wValue >> 8;
        const uint8_t desc_index = req->wValue & 0xff;
        LOG("descriptor request type=%x index=%x", desc_type, desc_index);
        (void)desc_index;
        switch(desc_type) {
        case USB_DT_HID:
            PACK_DATA(&dest, ipod_hid_hid_desc);
            break;
        case USB_DT_REPORT:
            if(usb_drv_port_speed()) {
                PACK_DATA(&dest, ipod_hid_report_hs);
            } else {
                PACK_DATA(&dest, ipod_hid_report_fs);
            }
            break;
        }
        if(dest != orig_dest) {
            usb_drv_control_response(USB_CONTROL_ACK, orig_dest, MIN(dest - orig_dest, req->wLength));
            return true;
        }
    } break;
    }
    return false;
}

static bool control_request_if_class(struct usb_ctrlrequest* req, void* reqdata, unsigned char* dest) {
    (void)dest;

    const uint8_t recip_interface = req->wIndex & 0xff;
    if(recip_interface == hid.interface) {
        switch(req->bRequest) {
        case USB_HID_GET_REPORT:
            respond_zero(req);
            return true;
        case USB_HID_SET_REPORT: {
            if(!receive_data(req, reqdata)) {
                return true;
            }
#if DEBUG_DUMP_RX == 1
            logf("==== acc: %u bytes ====", req->wLength);
            iap_platform_dump_hex(reqdata, req->wLength);
#endif

            struct IAPContext* ctx = _iap_acquire_ctx(true);
            const bool         ret = iap_feed_hid_report(ctx, reqdata, req->wLength);
            _iap_release_ctx();

            check_act(ret, return false);
            usb_drv_control_response(USB_CONTROL_ACK, NULL, 0);
            return true;
        }
        case USB_HID_SET_IDLE:
            usb_drv_control_response(USB_CONTROL_ACK, NULL, 0);
            return true;
        }
    }
    return false;
}

static bool control_request_if_endpoint(struct usb_ctrlrequest* req, void* reqdata, unsigned char* dest) {
    (void)dest;

    LOG("ctrl to endpoint %x (stream=%x, hid=%x)", req->wIndex, AS_EP_IN, HID_EP_IN);
    if(req->wIndex == AS_EP_IN) {
        const uint8_t recip_entity     = req->wIndex >> 8;
        const uint8_t control_selector = req->wValue >> 8;
        (void)recip_entity;
        switch(req->bRequest) {
        case USB_AC_SET_CUR:
            if(!receive_data(req, reqdata)) {
                return true;
            }
            LOG("audio ctrl to stream endpoint entity=0x%02X request=0x%02X length=%u", recip_entity, req->bRequest, req->wLength);
            switch(control_selector) {
            case USB_AS_EP_CS_SAMPLING_FREQ_CTL:
                check_act(req->wLength == 3, goto stall);
                stream.sample_rate = ctrl_buf[0] | (ctrl_buf[1] << 8) | (ctrl_buf[2] << 16);
                LOG("audio stream sampling rate %lu", stream.sample_rate);
                check_act(iap_audio_set_sampr(stream.sample_rate), goto stall);
                break;
            }
            usb_drv_control_response(USB_CONTROL_ACK, NULL, 0);
            return true;
        case USB_AC_GET_CUR:
            switch(control_selector) {
            case USB_AS_EP_CS_SAMPLING_FREQ_CTL:
                check_act(req->wLength == 3, goto stall);
                ctrl_buf[2] = (stream.sample_rate >> 16) & 0xff;
                ctrl_buf[1] = (stream.sample_rate >> 8) & 0xff;
                ctrl_buf[0] = (stream.sample_rate & 0xff);
                usb_drv_control_response(USB_CONTROL_ACK, ctrl_buf, req->wLength);
                return true;
            }
            /* fallthrough */
        case USB_AC_GET_MIN:
        case USB_AC_GET_MAX:
        case USB_AC_GET_RES:
            respond_zero(req);
            return true;
        stall:
            usb_drv_control_response(USB_CONTROL_STALL, NULL, 0);
            return true;
        }
    }
    return false;
}

bool usb_iap_control_request(struct usb_ctrlrequest* req, void* reqdata, unsigned char* dest) {
    const uint8_t req_recipient = req->bRequestType & USB_RECIP_MASK;
    const uint8_t req_type      = req->bRequestType & USB_TYPE_MASK;
#if 0
    LOG("bRequestType=%x, bRequest=%x, wValue=%x, wIndex=%x, wLength=%x", req->bRequestType, req->bRequest, req->wValue, req->wIndex, req->wLength);
    LOG("recip=%x type=%x", req_recipient, req_type);
#endif
    if(req_recipient == USB_RECIP_INTERFACE && req_type == USB_TYPE_STANDARD) {
        return control_request_if_std(req, reqdata, dest);
    } else if(req_recipient == USB_RECIP_INTERFACE && req_type == USB_TYPE_CLASS) {
        return control_request_if_class(req, reqdata, dest);
    } else if(req_recipient == USB_RECIP_ENDPOINT && req_type == USB_TYPE_CLASS) {
        return control_request_if_endpoint(req, reqdata, dest);
    }
    return false;
}

void usb_iap_notify_event(intptr_t data) {
    switch(data) {
    case Notify_Tick: {
        struct IAPContext* ctx = _iap_acquire_ctx(true);
        struct Platform*   plt = ctx->platform;
        if(plt->control_pending) {
            /* waiting for playback begins */
            _iap_release_ctx();
            return;
        }

        if(last_charge_state < 0 || last_charge_state != charge_state || last_battery_level != battery_level()) {
            last_charge_state  = charge_state;
            last_battery_level = battery_level();
            iap_notify_power_state(ctx, _iap_convert_charge_status(last_charge_state), _iap_convert_battery_level(last_battery_level));
        }

        struct tm* tm = get_time();
        if(last_minute == -1 || last_minute != tm->tm_min) {
            last_minute = tm->tm_min;
            struct IAPDateTime time;
            _iap_convert_datetime(get_time(), &time);
            iap_notify_time_setting(ctx, &time);
        }

#ifdef HAS_BUTTON_HOLD
        int8_t hold = button_hold() ? 1 : 0;
        if(hold != last_hold_switch_state) {
            last_hold_switch_state = hold;
            iap_notify_hold_switch_state(ctx, last_hold_switch_state);
        }
#endif

        check_act(iap_periodic_tick(ctx), );
        _iap_release_ctx();
    } break;
    }
}
