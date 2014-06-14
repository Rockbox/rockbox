#ifndef __USB_DESIGNWARE_H__
#define __USB_DESIGNWARE_H__

#include <inttypes.h>
#include "usb-designware-regs.h"
#include "kernel.h"

struct usb_instance;

union __attribute__((packed,aligned(4))) usb_ep0_buffer
{
    struct __attribute__((packed,aligned(4)))
    {
        struct __attribute__((packed))
        {
            enum
            {
                USB_SETUP_BMREQUESTTYPE_RECIPIENT_DEVICE = 0,
                USB_SETUP_BMREQUESTTYPE_RECIPIENT_INTERFACE = 1,
                USB_SETUP_BMREQUESTTYPE_RECIPIENT_ENDPOINT = 2,
                USB_SETUP_BMREQUESTTYPE_RECIPIENT_OTHER = 3,
            } recipient : 5;
            enum
            {
                USB_SETUP_BMREQUESTTYPE_TYPE_STANDARD = 0,
                USB_SETUP_BMREQUESTTYPE_TYPE_CLASS = 1,
                USB_SETUP_BMREQUESTTYPE_TYPE_VENDOR = 2,
            } type : 2;
            enum
            {
                USB_SETUP_BMREQUESTTYPE_DIRECTION_OUT = 0,
                USB_SETUP_BMREQUESTTYPE_DIRECTION_IN = 1,
            } direction : 1;
        } bmRequestType;
        union
        {
            enum __attribute__((packed))
            {
                USB_SETUP_BREQUEST_GET_STATUS = 0,
                USB_SETUP_BREQUEST_CLEAR_FEATURE = 1,
                USB_SETUP_BREQUEST_SET_FEATURE = 3,
                USB_SETUP_BREQUEST_SET_ADDRESS = 5,
                USB_SETUP_BREQUEST_GET_DESCRIPTOR = 6,
                USB_SETUP_BREQUEST_SET_DESCRIPTOR = 7,
                USB_SETUP_BREQUEST_GET_CONFIGURATION = 8,
                USB_SETUP_BREQUEST_SET_CONFIGURATION = 9,
                USB_SETUP_BREQUEST_GET_INTERFACE = 10,
                USB_SETUP_BREQUEST_SET_INTERFACE = 11,
                USB_SETUP_BREQUEST_SYNCH_FRAME = 12,
            } req;
            uint8_t raw;
        } bRequest;
        uint16_t wValue;
        uint16_t wIndex;
        uint16_t wLength;
    } setup;
    uint8_t raw[64];
};

enum __attribute__((packed)) usb_descriptor_type
{
    USB_DESCRIPTOR_TYPE_DEVICE = 1,
    USB_DESCRIPTOR_TYPE_CONFIGURATION = 2,
    USB_DESCRIPTOR_TYPE_STRING = 3,
    USB_DESCRIPTOR_TYPE_INTERFACE = 4,
    USB_DESCRIPTOR_TYPE_ENDPOINT = 5,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER = 6,
    USB_DESCRIPTOR_TYPE_OTHER_SPEED_CONFIG = 7,
    USB_DESCRIPTOR_TYPE_INTERFACE_POWER = 8,
    USB_DESCRIPTOR_TYPE_OTG = 9,
    USB_DESCRIPTOR_TYPE_DEBUG = 10,
    USB_DESCRIPTOR_TYPE_INTERFACE_ASSOCIATION = 11,
};

enum usb_endpoint_direction
{
    USB_ENDPOINT_DIRECTION_OUT = 0,
    USB_ENDPOINT_DIRECTION_IN = 1,
};

//union __attribute__((packed)) usb_endpoint_number
struct usb_endpoint_number
{
    int number;
    enum usb_endpoint_direction direction;
};

enum usb_endpoint_type
{
    USB_ENDPOINT_TYPE_CONTROL = 0,
    USB_ENDPOINT_TYPE_ISOCHRONOUS = 1,
    USB_ENDPOINT_TYPE_BULK = 2,
    USB_ENDPOINT_TYPE_INTERRUPT = 3,
};

struct usb_dw_config //__attribute__((packed,aligned(4))) usb_dw_config
{
    bool phy_16bit;
    bool phy_ulpi;
    bool use_dma;
    bool shared_txfifo;
    bool disable_double_buffering;
    uint16_t fifosize;
    uint16_t txfifosize[16];
};

extern void usb_dw_irq(void);
extern void usb_dw_target_enable_clocks(void);
extern void usb_dw_target_disable_clocks(void);
extern void usb_dw_target_enable_irq(void);
extern void usb_dw_target_disable_irq(void);
extern void usb_dw_target_clear_irq(void);

extern const struct usb_dw_config usb_dw_config;

#endif
