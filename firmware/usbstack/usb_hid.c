/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Tomer Shalev
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
#include "string.h"
#include "system.h"
#include "usb_core.h"
#include "usb_drv.h"
#include "kernel.h"
#include "usb_hid.h"
#include "usb_class_driver.h"
/*#define LOGF_ENABLE*/
#include "logf.h"

/* Documents avaiable here: http://www.usb.org/developers/devclass_docs/ */

#define HID_VER                         0x0110 /* 1.1 */
/* Subclass Codes (HID1_11.pdf, page 18) */
#define SUBCLASS_NONE                   0
#define SUBCLASS_BOOT_INTERFACE         1
/* Protocol Codes (HID1_11.pdf, page 19) */
#define PROTOCOL_CODE_NONE              0
#define PROTOCOL_CODE_KEYBOARD          1
#define PROTOCOL_CODE_MOUSE             2
/* HID main items (HID1_11.pdf, page 38) */
#define INPUT                           0x80
#define OUTPUT                          0x90
#define COLLECTION                      0xA0
#define COLLECTION_PHYSICAL             0x00
#define COLLECTION_APPLICATION          0x01
#define END_COLLECTION                  0xC0
/* Parts (HID1_11.pdf, page 40) */
#define MAIN_ITEM_CONSTANT              BIT_N(0) /* 0x01 */
#define MAIN_ITEM_VARIABLE              BIT_N(1) /* 0x02 */
#define MAIN_ITEM_RELATIVE              BIT_N(2) /* 0x04 */
#define MAIN_ITEM_WRAP                  BIT_N(3) /* 0x08 */
#define MAIN_ITEM_NONLINEAR             BIT_N(4) /* 0x10 */
#define MAIN_ITEM_NO_PREFERRED          BIT_N(5) /* 0x20 */
#define MAIN_ITEM_NULL_STATE            BIT_N(6) /* 0x40 */
#define MAIN_ITEM_VOLATILE              BIT_N(7) /* 0x80 */
#define MAIN_ITEM_BUFFERED_BYTES        BIT_N(8) /* 0x0100 */
/* HID global items (HID1_11.pdf, page 45) */
#define USAGE_PAGE                      0x04
#define LOGICAL_MINIMUM                 0x14
#define LOGICAL_MAXIMUM                 0x24
#define REPORT_SIZE                     0x74
#define REPORT_ID                       0x84
#define REPORT_COUNT                    0x94
/* HID local items (HID1_11.pdf, page 50) */
#define USAGE_MINIMUM                   0x18
#define USAGE_MAXIMUM                   0x28
/* Types of class descriptors (HID1_11.pdf, page 59) */
#define USB_DT_HID                      0x21
#define USB_DT_REPORT                   0x22

#define CONSUMER_USAGE                  0x09

/* HID-only class specific requests (HID1_11.pdf, page 61) */
#define USB_HID_GET_REPORT              0x01
#define USB_HID_GET_IDLE                0x02
#define USB_HID_GET_PROTOCOL            0x03
#define USB_HID_SET_REPORT              0x09
#define USB_HID_SET_IDLE                0x0A
#define USB_HID_SET_PROTOCOL            0x0B

/* Get_Report and Set_Report Report Type field (HID1_11.pdf, page 61) */
#define REPORT_TYPE_INPUT               0x01
#define REPORT_TYPE_OUTPUT              0x02
#define REPORT_TYPE_FEATURE             0x03

#define HID_BUF_SIZE_MSG                16
#define HID_BUF_SIZE_CMD                16
#define HID_BUF_SIZE_REPORT             160
#define HID_NUM_BUFFERS                 5
#define SET_REPORT_BUF_LEN 2

#ifdef LOGF_ENABLE

#define BUF_DUMP_BUF_LEN       HID_BUF_SIZE_REPORT
#define BUF_DUMP_PREFIX_SIZE   5
#define BUF_DUMP_ITEMS_IN_LINE 8
#define BUF_DUMP_LINE_SIZE     (BUF_DUMP_ITEMS_IN_LINE * 3)
#define BUF_DUMP_NUM_LINES     (BUF_DUMP_BUF_LEN / BUF_DUMP_ITEMS_IN_LINE)
#endif

#define HID_BUF_INC(i) do { (i) = ((i) + 1) % HID_NUM_BUFFERS; } while (0)
#define PACK_VAL(dest, val) *(dest)++ = (val) & 0xff

typedef enum
{
    REPORT_ID_KEYBOARD = 1,
    REPORT_ID_CONSUMER,
#ifdef HAVE_USB_HID_MOUSE
    REPORT_ID_MOUSE,
#endif
    REPORT_ID_COUNT,
} report_id_t;

/* hid interface */
static struct usb_interface_descriptor __attribute__((aligned(2)))
                                interface_descriptor =
{
    .bLength            = sizeof(struct usb_interface_descriptor),
    .bDescriptorType    = USB_DT_INTERFACE,
    .bInterfaceNumber   = 0,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 1,
    .bInterfaceClass    = USB_CLASS_HID,
    .bInterfaceSubClass = SUBCLASS_BOOT_INTERFACE,
    .bInterfaceProtocol = PROTOCOL_CODE_KEYBOARD,
    .iInterface         = 0
};

struct usb_hid_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t wBcdHID;
    uint8_t  bCountryCode;
    uint8_t  bNumDescriptors;
    uint8_t  bDescriptorType0;
    uint16_t wDescriptorLength0;
} __attribute__ ((packed));

static struct usb_hid_descriptor __attribute__((aligned(2))) hid_descriptor =
{
    .bLength            = sizeof(struct usb_hid_descriptor),
    .bDescriptorType    = USB_DT_HID,
    .wBcdHID            = HID_VER,
    .bCountryCode       = 0,
    .bNumDescriptors    = 1,
    .bDescriptorType0   = USB_DT_REPORT,
    .wDescriptorLength0 = 0
};

static struct usb_endpoint_descriptor __attribute__((aligned(2)))
    endpoint_descriptor =
{
    .bLength          = sizeof(struct usb_endpoint_descriptor),
    .bDescriptorType  = USB_DT_ENDPOINT,
    .bEndpointAddress = 0,
    .bmAttributes     = USB_ENDPOINT_XFER_INT,
    .wMaxPacketSize   = 0,
    .bInterval        = 0
};

typedef struct
{
    uint8_t usage_page;
    /* Write the id into the buffer in the appropriate location, and returns the
     * buffer length */
    uint8_t (*buf_set)(unsigned char *buf, int id);
    bool is_key_released;
} usb_hid_report_t;

static usb_hid_report_t usb_hid_reports[REPORT_ID_COUNT];

static unsigned char report_descriptor[HID_BUF_SIZE_REPORT]
    USB_DEVBSS_ATTR __attribute__((aligned(32)));

static unsigned char send_buffer[HID_NUM_BUFFERS][HID_BUF_SIZE_MSG]
    USB_DEVBSS_ATTR __attribute__((aligned(32)));
static size_t send_buffer_len[HID_NUM_BUFFERS];
static int cur_buf_prepare;
static int cur_buf_send;

static bool active = false;
static bool currently_sending = false;
static int ep_in;
static int usb_interface;

static void usb_hid_try_send_drv(void);

static void pack_parameter(unsigned char **dest, bool is_signed, bool mark_size,
        uint8_t parameter, uint32_t value)
{
    uint8_t size_value = 1; /* # of bytes */
    uint32_t v = value;

    while (v >>= 8)
        size_value++;

    if (is_signed)
    {
        bool is_negative = 0;

        switch (size_value)
        {
            case 1:
                is_negative = (int8_t)value < 0;
                break;
            case 2:
                is_negative = (int16_t)value < 0;
                break;
            case 3:
            case 4:
                is_negative = (int32_t)value < 0;
                break;
            default:
                break;
        }

        if (is_negative)
            size_value++;
    }

    **dest = parameter | (mark_size ? size_value : 0);
    (*dest)++;

    while (size_value--)
    {
        **dest = value & 0xff;
        (*dest)++;
        value >>= 8;
    }
}

int usb_hid_request_endpoints(struct usb_class_driver *drv)
{
    ep_in = usb_core_request_endpoint(USB_ENDPOINT_XFER_INT, USB_DIR_IN, drv);
    if (ep_in < 0)
        return -1;

    return 0;
}

int usb_hid_set_first_interface(int interface)
{
    usb_interface = interface;

    return interface + 1;
}

#ifdef LOGF_ENABLE
static unsigned char buf_dump_ar[BUF_DUMP_NUM_LINES][BUF_DUMP_LINE_SIZE + 1]
    USB_DEVBSS_ATTR __attribute__((aligned(32)));

void buf_dump(unsigned char *buf, size_t size, char *msg)
{
    size_t i;
    int line;
    static const char v[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'A', 'B', 'C', 'D', 'E', 'F' };

    for (i = 0, line = 0; i < size; line++)
    {
        int j, i0 = i;
        char *b = buf_dump_ar[line];

        for (j = 0; j < BUF_DUMP_ITEMS_IN_LINE && i < size; j++, i++)
        {
            *b++ = v[buf[i] >> 4];
            *b++ = v[buf[i] & 0xF];
            *b++ = ' ';
        }
        *b = 0;

        /* Prefix must be of len BUF_DUMP_PREFIX_SIZE */
        logf("%03d: %s %s", i0, buf_dump_ar[line], msg);
    }
}
#else
#undef buf_dump
#define buf_dump(...)
#endif

#define BUF_LEN_KEYBOARD 7
static uint8_t buf_set_keyboard(unsigned char *buf, int id)
{
    memset(buf, 0, BUF_LEN_KEYBOARD);
    int key, i = 1;

    /* Each key is a word in id (up to 4 keys supported) */
    while ((key = id & 0xff))
    {
        /* Each modifier key is a bit in the first byte */
        if (HID_KEYBOARD_LEFT_CONTROL <= key && key <= HID_KEYBOARD_RIGHT_GUI)
            buf[0] |= (1 << (key - HID_KEYBOARD_LEFT_CONTROL));
        else /* Any other key should be set in a separate byte */
            buf[i++] = (uint8_t)key;

        id >>= 8;
    }

    return BUF_LEN_KEYBOARD;
}

#define BUF_LEN_CONSUMER 4
static uint8_t buf_set_consumer(unsigned char *buf, int id)
{
    memset(buf, 0, BUF_LEN_CONSUMER);
    buf[0] = (uint8_t)id;

    return BUF_LEN_CONSUMER;
}

#ifdef HAVE_USB_HID_MOUSE
#define MOUSE_WHEEL_STEP       1
#define MOUSE_STEP             8
#define MOUSE_BUTTON_LEFT    0x1
#define MOUSE_BUTTON_RIGHT   0x2
//#define MOUSE_BUTTON_MIDDLE  0x4
#define MOUSE_BUTTON           0
#define MOUSE_X                1
#define MOUSE_Y                2
#define MOUSE_WHEEL            3
#define BUF_LEN_MOUSE          4
#define MOUSE_ACCEL_FACTOR(count) ((count) / 2)
#define MOUSE_DO(item, step) (buf[(item)] = ((uint8_t)(step)))

static uint8_t buf_set_mouse(unsigned char *buf, int id)
{
    static int count = 0;
    int step = MOUSE_STEP;

    memset(buf, 0, BUF_LEN_MOUSE);

    /* Set proper button */
    switch (id)
    {
        case HID_MOUSE_BUTTON_LEFT:
        case HID_MOUSE_LDRAG_UP:
        case HID_MOUSE_LDRAG_UP_REP:
        case HID_MOUSE_LDRAG_DOWN:
        case HID_MOUSE_LDRAG_DOWN_REP:
        case HID_MOUSE_LDRAG_LEFT:
        case HID_MOUSE_LDRAG_LEFT_REP:
        case HID_MOUSE_LDRAG_RIGHT:
        case HID_MOUSE_LDRAG_RIGHT_REP:
            MOUSE_DO(MOUSE_BUTTON, MOUSE_BUTTON_LEFT);
            break;
        case HID_MOUSE_BUTTON_RIGHT:
        case HID_MOUSE_RDRAG_UP:
        case HID_MOUSE_RDRAG_UP_REP:
        case HID_MOUSE_RDRAG_DOWN:
        case HID_MOUSE_RDRAG_DOWN_REP:
        case HID_MOUSE_RDRAG_LEFT:
        case HID_MOUSE_RDRAG_LEFT_REP:
        case HID_MOUSE_RDRAG_RIGHT:
        case HID_MOUSE_RDRAG_RIGHT_REP:
            MOUSE_DO(MOUSE_BUTTON, MOUSE_BUTTON_RIGHT);
            break;
        case HID_MOUSE_BUTTON_LEFT_REL:
        case HID_MOUSE_BUTTON_RIGHT_REL:
            /* Keep buf empty, to mark mouse button release */
            break;
        default:
            break;
    }

    /* Handle mouse accelarated movement */
    switch (id)
    {
        case HID_MOUSE_UP:
        case HID_MOUSE_DOWN:
        case HID_MOUSE_LEFT:
        case HID_MOUSE_RIGHT:
        case HID_MOUSE_LDRAG_UP:
        case HID_MOUSE_LDRAG_DOWN:
        case HID_MOUSE_LDRAG_LEFT:
        case HID_MOUSE_LDRAG_RIGHT:
        case HID_MOUSE_RDRAG_UP:
        case HID_MOUSE_RDRAG_DOWN:
        case HID_MOUSE_RDRAG_LEFT:
        case HID_MOUSE_RDRAG_RIGHT:
            count = 0;
            break;
        case HID_MOUSE_UP_REP:
        case HID_MOUSE_DOWN_REP:
        case HID_MOUSE_LEFT_REP:
        case HID_MOUSE_RIGHT_REP:
        case HID_MOUSE_LDRAG_UP_REP:
        case HID_MOUSE_LDRAG_DOWN_REP:
        case HID_MOUSE_LDRAG_LEFT_REP:
        case HID_MOUSE_LDRAG_RIGHT_REP:
        case HID_MOUSE_RDRAG_UP_REP:
        case HID_MOUSE_RDRAG_DOWN_REP:
        case HID_MOUSE_RDRAG_LEFT_REP:
        case HID_MOUSE_RDRAG_RIGHT_REP:
            count++;
            /* TODO: Find a better mouse accellaration algorithm */
            step *= 1 + MOUSE_ACCEL_FACTOR(count);
            if (step > 255)
                step = 255;
            break;
        default:
            break;
    }

    /* Move/scroll mouse */
    switch (id)
    {
        case HID_MOUSE_UP:
        case HID_MOUSE_UP_REP:
        case HID_MOUSE_LDRAG_UP:
        case HID_MOUSE_LDRAG_UP_REP:
        case HID_MOUSE_RDRAG_UP:
        case HID_MOUSE_RDRAG_UP_REP:
            MOUSE_DO(MOUSE_Y, -step);
            break;
        case HID_MOUSE_DOWN:
        case HID_MOUSE_DOWN_REP:
        case HID_MOUSE_LDRAG_DOWN:
        case HID_MOUSE_LDRAG_DOWN_REP:
        case HID_MOUSE_RDRAG_DOWN:
        case HID_MOUSE_RDRAG_DOWN_REP:
            MOUSE_DO(MOUSE_Y, step);
            break;
        case HID_MOUSE_LEFT:
        case HID_MOUSE_LEFT_REP:
        case HID_MOUSE_LDRAG_LEFT:
        case HID_MOUSE_LDRAG_LEFT_REP:
        case HID_MOUSE_RDRAG_LEFT:
        case HID_MOUSE_RDRAG_LEFT_REP:
            MOUSE_DO(MOUSE_X, -step);
            break;
        case HID_MOUSE_RIGHT:
        case HID_MOUSE_RIGHT_REP:
        case HID_MOUSE_LDRAG_RIGHT:
        case HID_MOUSE_LDRAG_RIGHT_REP:
        case HID_MOUSE_RDRAG_RIGHT:
        case HID_MOUSE_RDRAG_RIGHT_REP:
            MOUSE_DO(MOUSE_X, step);
            break;
        case HID_MOUSE_SCROLL_UP:
            MOUSE_DO(MOUSE_WHEEL, MOUSE_WHEEL_STEP);
            break;
        case HID_MOUSE_SCROLL_DOWN:
            MOUSE_DO(MOUSE_WHEEL, -MOUSE_WHEEL_STEP);
            break;
        default:
            break;
    }

    return BUF_LEN_MOUSE;
}
#endif /* HAVE_USB_HID_MOUSE */

static size_t descriptor_report_get(unsigned char *dest)
{
    unsigned char *report = dest;
    usb_hid_report_t *usb_hid_report;

    /* Keyboard control */
    usb_hid_report = &usb_hid_reports[REPORT_ID_KEYBOARD];
    usb_hid_report->usage_page = HID_USAGE_PAGE_KEYBOARD_KEYPAD;
    usb_hid_report->buf_set = buf_set_keyboard;
    usb_hid_report->is_key_released = 1;

    pack_parameter(&report, 0, 1, USAGE_PAGE,
            HID_USAGE_PAGE_GENERIC_DESKTOP_CONTROLS);
    pack_parameter(&report, 0, 0, CONSUMER_USAGE, HID_GENERIC_DESKTOP_KEYBOARD);
    pack_parameter(&report, 0, 1, COLLECTION, COLLECTION_APPLICATION);
    pack_parameter(&report, 0, 1, REPORT_ID, REPORT_ID_KEYBOARD);
    pack_parameter(&report, 0, 1, USAGE_PAGE, HID_GENERIC_DESKTOP_KEYPAD);
    pack_parameter(&report, 0, 1, USAGE_MINIMUM, HID_KEYBOARD_LEFT_CONTROL);
    pack_parameter(&report, 0, 1, USAGE_MAXIMUM, HID_KEYBOARD_RIGHT_GUI);
    pack_parameter(&report, 1, 1, LOGICAL_MINIMUM, 0);
    pack_parameter(&report, 1, 1, LOGICAL_MAXIMUM, 1);
    pack_parameter(&report, 0, 1, REPORT_SIZE, 1);
    pack_parameter(&report, 0, 1, REPORT_COUNT, 8);
    pack_parameter(&report, 0, 1, INPUT, MAIN_ITEM_VARIABLE);
    pack_parameter(&report, 0, 1, REPORT_SIZE, 1);
    pack_parameter(&report, 0, 1, REPORT_COUNT, 5);
    pack_parameter(&report, 0, 1, USAGE_PAGE, HID_USAGE_PAGE_LEDS);
    pack_parameter(&report, 0, 1, USAGE_MINIMUM, HID_LED_NUM_LOCK);
    pack_parameter(&report, 0, 1, USAGE_MAXIMUM, HID_LED_KANA);
    pack_parameter(&report, 0, 1, OUTPUT, MAIN_ITEM_VARIABLE);
    pack_parameter(&report, 0, 1, REPORT_SIZE, 3);
    pack_parameter(&report, 0, 1, REPORT_COUNT, 1);
    pack_parameter(&report, 0, 1, OUTPUT, MAIN_ITEM_CONSTANT);
    pack_parameter(&report, 0, 1, REPORT_SIZE, 8);
    pack_parameter(&report, 0, 1, REPORT_COUNT, 6);
    pack_parameter(&report, 1, 1, LOGICAL_MINIMUM, 0);
    pack_parameter(&report, 1, 1, LOGICAL_MAXIMUM, HID_KEYBOARD_EX_SEL);
    pack_parameter(&report, 0, 1, USAGE_PAGE, HID_USAGE_PAGE_KEYBOARD_KEYPAD);
    pack_parameter(&report, 0, 1, USAGE_MINIMUM, 0);
    pack_parameter(&report, 0, 1, USAGE_MAXIMUM, HID_KEYBOARD_EX_SEL);
    pack_parameter(&report, 0, 1, INPUT, 0);
    PACK_VAL(report, END_COLLECTION);

    /* Consumer usage controls - play/pause, stop, etc. */
    usb_hid_report = &usb_hid_reports[REPORT_ID_CONSUMER];
    usb_hid_report->usage_page = HID_USAGE_PAGE_CONSUMER;
    usb_hid_report->buf_set = buf_set_consumer;
    usb_hid_report->is_key_released = 1;

    pack_parameter(&report, 0, 1, USAGE_PAGE, HID_USAGE_PAGE_CONSUMER);
    pack_parameter(&report, 0, 0, CONSUMER_USAGE,
            HID_CONSUMER_USAGE_CONSUMER_CONTROL);
    pack_parameter(&report, 0, 1, COLLECTION, COLLECTION_APPLICATION);
    pack_parameter(&report, 0, 1, REPORT_ID, REPORT_ID_CONSUMER);
    pack_parameter(&report, 0, 1, REPORT_SIZE, 16);
    pack_parameter(&report, 0, 1, REPORT_COUNT, 2);
    pack_parameter(&report, 1, 1, LOGICAL_MINIMUM, 1);
    pack_parameter(&report, 1, 1, LOGICAL_MAXIMUM, 652);
    pack_parameter(&report, 0, 1, USAGE_MINIMUM,
            HID_CONSUMER_USAGE_CONSUMER_CONTROL);
    pack_parameter(&report, 0, 1, USAGE_MAXIMUM, HID_CONSUMER_USAGE_AC_SEND);
    pack_parameter(&report, 0, 1, INPUT, MAIN_ITEM_NO_PREFERRED |
            MAIN_ITEM_NULL_STATE);
    PACK_VAL(report, END_COLLECTION);

#ifdef HAVE_USB_HID_MOUSE
    /* Mouse control */
    usb_hid_report = &usb_hid_reports[REPORT_ID_MOUSE];
    usb_hid_report->usage_page = HID_USAGE_PAGE_GENERIC_DESKTOP_CONTROLS;
    usb_hid_report->buf_set = buf_set_mouse;
    usb_hid_report->is_key_released = 0;

    pack_parameter(&report, 0, 1, USAGE_PAGE,
            HID_USAGE_PAGE_GENERIC_DESKTOP_CONTROLS);
    pack_parameter(&report, 0, 0, CONSUMER_USAGE, HID_GENERIC_DESKTOP_MOUSE);
    pack_parameter(&report, 0, 1, COLLECTION, COLLECTION_APPLICATION);
    pack_parameter(&report, 0, 1, REPORT_ID, REPORT_ID_MOUSE);
    pack_parameter(&report, 0, 0, CONSUMER_USAGE, HID_GENERIC_DESKTOP_POINTER);
    pack_parameter(&report, 0, 1, COLLECTION, COLLECTION_PHYSICAL);
    pack_parameter(&report, 0, 1, USAGE_PAGE, HID_USAGE_PAGE_BUTTON);
    pack_parameter(&report, 0, 1, USAGE_MINIMUM, 1);
    pack_parameter(&report, 0, 1, USAGE_MAXIMUM, 8);
    pack_parameter(&report, 1, 1, LOGICAL_MINIMUM, 0);
    pack_parameter(&report, 1, 1, LOGICAL_MAXIMUM, 1);
    pack_parameter(&report, 0, 1, REPORT_SIZE, 1);
    pack_parameter(&report, 0, 1, REPORT_COUNT, 8);
    pack_parameter(&report, 0, 1, INPUT, MAIN_ITEM_VARIABLE);
    pack_parameter(&report, 0, 1, USAGE_PAGE,
            HID_USAGE_PAGE_GENERIC_DESKTOP_CONTROLS);
    pack_parameter(&report, 0, 0, CONSUMER_USAGE, HID_GENERIC_DESKTOP_X);
    pack_parameter(&report, 0, 0, CONSUMER_USAGE, HID_GENERIC_DESKTOP_Y);
    pack_parameter(&report, 0, 0, CONSUMER_USAGE, HID_GENERIC_DESKTOP_WHEEL);
    pack_parameter(&report, 0, 1, LOGICAL_MINIMUM, -127 & 0xFF);
    pack_parameter(&report, 0, 1, LOGICAL_MAXIMUM, 127);
    pack_parameter(&report, 0, 1, REPORT_SIZE, 8);
    pack_parameter(&report, 0, 1, REPORT_COUNT, 3);
    pack_parameter(&report, 0, 1, INPUT, MAIN_ITEM_VARIABLE | MAIN_ITEM_RELATIVE);
    PACK_VAL(report, END_COLLECTION);
    PACK_VAL(report, END_COLLECTION);
#endif /* HAVE_USB_HID_MOUSE */

    return (size_t)(report - dest);
}

static void descriptor_hid_get(unsigned char **dest)
{
    hid_descriptor.wDescriptorLength0 =
        (uint16_t)descriptor_report_get(report_descriptor);

    logf("hid: desc len %u", hid_descriptor.wDescriptorLength0);
    buf_dump(report_descriptor, hid_descriptor.wDescriptorLength0, "desc");

    PACK_DATA(*dest, hid_descriptor);
}

int usb_hid_get_config_descriptor(unsigned char *dest, int max_packet_size)
{
    unsigned char *orig_dest = dest;

    logf("hid: config desc.");

    /* Ignore given max_packet_size */
    (void)max_packet_size;

    /* Interface descriptor */
    interface_descriptor.bInterfaceNumber = usb_interface;
    PACK_DATA(dest, interface_descriptor);

    /* HID descriptor */
    descriptor_hid_get(&dest);

    /* Endpoint descriptor */
    endpoint_descriptor.wMaxPacketSize = 8;
    endpoint_descriptor.bInterval = 8;
    endpoint_descriptor.bEndpointAddress = ep_in;
    PACK_DATA(dest, endpoint_descriptor);

    return (int)(dest - orig_dest);
}

void usb_hid_init_connection(void)
{
    logf("hid: init connection");

    active = true;
    currently_sending = false;
}

/* called by usb_core_init() */
void usb_hid_init(void)
{
    int i;

    logf("hid: init");

    for (i = 0; i < HID_NUM_BUFFERS; i++)
        send_buffer_len[i] = 0;

    cur_buf_prepare = 0;
    cur_buf_send = 0;

    active = true;
    currently_sending = false;
}

void usb_hid_disconnect(void)
{
    logf("hid: disconnect");
    active = false;
    currently_sending = false;
}

/* called by usb_core_transfer_complete() */
void usb_hid_transfer_complete(int ep, int dir, int status, int length)
{
    (void)ep;
    (void)dir;
    (void)status;
    (void)length;

    logf("HID: transfer complete: %d %d %d %d",ep,dir,status,length);
    switch (dir)
    {
        case USB_DIR_OUT:
            break;
        case USB_DIR_IN:
        {
            if (status)
                break;

            send_buffer_len[cur_buf_send] = 0;
            HID_BUF_INC(cur_buf_send);
            currently_sending = false;
            usb_hid_try_send_drv();
            break;
        }
    }
}

/* The DAP is registered as a keyboard with several LEDs, therefore the OS sends
 * LED report to notify the DAP whether Num Lock / Caps Lock etc. are enabled.
 * In order to allow sending info to the DAP, the Set Report mechanism can be
 * used by defining vendor specific output reports and send them from the host
 * to the DAP using the host's custom driver */
static int usb_hid_set_report(struct usb_ctrlrequest *req)
{
    static unsigned char buf[SET_REPORT_BUF_LEN] USB_DEVBSS_ATTR
        __attribute__((aligned(32)));
    int length;
    int rc = 0;

    if ((req->wValue >> 8) != REPORT_TYPE_OUTPUT)
    {
        logf("Unsopported report type");
        rc = 1;
        goto Exit;
    }
    if ((req->wValue & 0xff) != REPORT_ID_KEYBOARD)
    {
        logf("Wrong report id");
        rc = 2;
        goto Exit;
    }
    if (req->wIndex != (uint16_t)usb_interface)
    {
        logf("Wrong interface");
        rc = 3;
        goto Exit;
    }
    length = req->wLength;
    if (length != SET_REPORT_BUF_LEN)
    {
        logf("Wrong length");
        rc = 4;
        goto Exit;
    }

    memset(buf, 0, length);
    usb_drv_recv(EP_CONTROL, buf, length);

#ifdef LOGF_ENABLE
    if (buf[1] & 0x01)
        logf("Num Lock enabled");
    if (buf[1] & 0x02)
        logf("Caps Lock enabled");
    if (buf[1] & 0x04)
        logf("Scroll Lock enabled");
    if (buf[1] & 0x08)
        logf("Compose enabled");
    if (buf[1] & 0x10)
        logf("Kana enabled");
#endif

    /* Defining other LEDs and setting them from the USB host (OS) can be used
     * to send messages to the DAP */

Exit:
    return rc;
}

/* called by usb_core_control_request() */
bool usb_hid_control_request(struct usb_ctrlrequest *req, unsigned char *dest)
{
    bool handled = false;

    switch (req->bRequestType & USB_TYPE_MASK)
    {
        case USB_TYPE_STANDARD:
        {
            unsigned char *orig_dest = dest;

            switch (req->wValue>>8) /* type */
            {
                case USB_DT_HID:
                {
                    logf("hid: type hid");
                    descriptor_hid_get(&dest);
                    break;
                }
                case USB_DT_REPORT:
                {
                    int len;

                    logf("hid: type report");

                    len = descriptor_report_get(report_descriptor);
                    memcpy(dest, &report_descriptor, len);
                    dest += len;
                    break;
                }
                default:
                    logf("hid: unsup. std. req");
                    break;
            }
            if (dest != orig_dest)
            {
                usb_drv_recv(EP_CONTROL, NULL, 0); /* ack */
                usb_drv_send(EP_CONTROL, orig_dest, dest - orig_dest);
                handled = true;
            }
            break;
        }

        case USB_TYPE_CLASS:
        {
            switch (req->bRequest)
            {
                case USB_HID_SET_IDLE:
                    logf("hid: set idle");
                    usb_drv_send(EP_CONTROL, NULL, 0); /* ack */
                    handled = true;
                    break;
                case USB_HID_SET_REPORT:
                    logf("hid: set report");
                    if (!usb_hid_set_report(req))
                    {
                        usb_drv_send(EP_CONTROL, NULL, 0); /* ack */
                        handled = true;
                    }
                    break;
                default:
                    logf("%d: unsup. cls. req", req->bRequest);
                    break;
            }
            break;
        }

        case USB_TYPE_VENDOR:
            logf("hid: unsup. ven. req");
            break;
    }
    return handled;
}

static void usb_hid_try_send_drv(void)
{
    int rc;
    int length = send_buffer_len[cur_buf_send];

    if (!length)
        return;

    if (currently_sending)
    {
        logf("HID: Already sending");
        return;
    }

    logf("HID: Sending %d bytes",length);
    rc = usb_drv_send_nonblocking(ep_in, send_buffer[cur_buf_send], length);
    currently_sending = true;
    if (rc)
    {
        send_buffer_len[cur_buf_send] = 0;
        return;
    }
}

static void usb_hid_queue(unsigned char *data, int length)
{
    if (!active || length <= 0)
        return;

    /* Buffer overflow - item still in use */
    if (send_buffer_len[cur_buf_prepare])
        return;

    /* Prepare buffer for sending */
    if (length > HID_BUF_SIZE_MSG)
        length = HID_BUF_SIZE_MSG;
    memcpy(send_buffer[cur_buf_prepare], data, length);
    send_buffer_len[cur_buf_prepare] = length;

    HID_BUF_INC(cur_buf_prepare);
}

void usb_hid_send(usage_page_t usage_page, int id)
{
    uint8_t report_id, length;
    static unsigned char buf[HID_BUF_SIZE_CMD] USB_DEVBSS_ATTR
        __attribute__((aligned(32)));
    usb_hid_report_t *report = NULL;

    for (report_id = 1; report_id < REPORT_ID_COUNT; report_id++)
    {
        if (usb_hid_reports[report_id].usage_page == usage_page)
        {
            report = &usb_hid_reports[report_id];
            break;
        }
    }
    if (!report)
    {
        logf("Unsupported usage_page");
        return;
    }

    logf("Sending cmd 0x%x", id);
    buf[0] = report_id;
    length = report->buf_set(&buf[1], id) + 1;
    logf("length %u", length);

    if (!length)
        return;

    /* Key pressed */
    buf_dump(buf, length, "key press");
    usb_hid_queue(buf, length);

    if (report->is_key_released)
    {
        /* Key released */
        memset(buf, 0, length);
        buf[0] = report_id;

        buf_dump(buf, length, "key release");
        usb_hid_queue(buf, length);
    }

    usb_hid_try_send_drv();
}
