/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Amaury Pouly
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libusb.h>
#include <stdint.h>
#include <stdbool.h>
#include <getopt.h>

bool g_debug = false;

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

static void put32le(uint8_t *buf, uint32_t i)
{
    *buf++ = i & 0xff;
    *buf++ = (i >> 8) & 0xff;
    *buf++ = (i >> 16) & 0xff;
    *buf++ = (i >> 24) & 0xff;
}

static void put32be(uint8_t *buf, uint32_t i)
{
    *buf++ = (i >> 24) & 0xff;
    *buf++ = (i >> 16) & 0xff;
    *buf++ = (i >> 8) & 0xff;
    *buf++ = i & 0xff;
}

enum dev_type_t
{
    PROBE_DEVICE,
    HID_DEVICE,
    RECOVERY_DEVICE,
};

struct dev_info_t
{
    uint16_t vendor_id;
    uint16_t product_id;
    unsigned xfer_size;
    enum dev_type_t dev_type;
};

struct dev_info_t g_dev_info[] =
{
    {0x066f, 0x3780, 1024, HID_DEVICE}, /* i.MX233 / STMP3780 */
    {0x066f, 0x3770, 48, HID_DEVICE}, /* STMP3770 */
    {0x15A2, 0x004F, 1024, HID_DEVICE}, /* i.MX28 */
    {0x066f, 0x3600, 4096, RECOVERY_DEVICE}, /* STMP36xx */
};

static int send_hid(libusb_device_handle *dev, int xfer_size, uint8_t *data, int size, int nr_xfers)
{
    libusb_detach_kernel_driver(dev, 0);
    libusb_claim_interface(dev, 0);

    uint8_t *xfer_buf = malloc(1 + xfer_size);
    uint8_t *p = xfer_buf;

    *p++ = 0x01;         /* Report id */

    /* Command block wrapper */
    *p++ = 'B';          /* Signature */
    *p++ = 'L';
    *p++ = 'T';
    *p++ = 'C';
    put32le(p, 0x1);     /* Tag */
    p += 4;
    put32le(p, size);    /* Payload size */
    p += 4;
    *p++ = 0;            /* Flags (host to device) */
    p += 2;              /* Reserved */

    /* Command descriptor block */
    *p++ = 0x02;         /* Firmware download */
    put32be(p, size);    /* Download size */

    int ret = libusb_control_transfer(dev,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, 0x9, 0x201, 0,
        xfer_buf, xfer_size + 1, 1000);
    if(ret < 0)
    {
        printf("transfer error at init step\n");
        return 1;
    }

    for(int i = 0; i < nr_xfers; i++)
    {
        xfer_buf[0] = 0x2;
        memcpy(&xfer_buf[1], &data[i * xfer_size], xfer_size);

        ret = libusb_control_transfer(dev,
            LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
            0x9, 0x202, 0, xfer_buf, xfer_size + 1, 1000);
        if(ret < 0)
        {
            printf("transfer error at send step %d\n", i);
            return 1;
        }
    }

    int recv_size;
    ret = libusb_interrupt_transfer(dev, 0x81, xfer_buf, xfer_size, &recv_size,
        1000);
    if(ret < 0)
    {
        printf("transfer error at final stage\n");
        return 1;
    }

    return ret;
}

static int send_recovery(libusb_device_handle *dev, int xfer_size, uint8_t *data, int size, int nr_xfers)
{
    (void) nr_xfers;
    // there should be no kernel driver attached but in doubt...
    libusb_detach_kernel_driver(dev, 0);
    libusb_claim_interface(dev, 0);

    int sent = 0;
    while(sent < size)
    {
        int xfered;
        int len = MIN(size - sent, xfer_size);
        int ret = libusb_bulk_transfer(dev, 1, data + sent, len, &xfered, 1000);
        if(ret < 0)
        {
            printf("transfer error at send offset %d\n", sent);
            return 1;
        }
        if(xfered == 0)
        {
            printf("empty transfer at step offset %d\n", sent);
            return 2;
        }
        sent += xfered;
    }
    return 0;
}

static void usage(void)
{
    printf("sbloader [options] file\n");
    printf("options:\n");
    printf("  -h/-?/--help    Display this help\n");
    printf("  -d/--debug      Enable debug output\n");
    printf("  -x <size>       Force transfer size\n");
    printf("  -u <vid>:<pid>  Force USB PID and VID\n");
    printf("  -b <bus>:<dev>  Force USB bus and device\n");
    printf("  -p <protocol>   Force protocol ('hid' or 'recovery')\n");
    printf("The following devices are known to this tool:\n");
    for(unsigned i = 0; i < sizeof(g_dev_info) / sizeof(g_dev_info[0]); i++)
    {
        const char *type = "unk";
        if(g_dev_info[i].dev_type == HID_DEVICE)
            type = "hid";
        else if(g_dev_info[i].dev_type == RECOVERY_DEVICE)
            type = "recovery";
        else if(g_dev_info[i].dev_type == PROBE_DEVICE)
            type = "probe";
        printf("  %04x:%04x %s (%d bytes/xfer)\n", g_dev_info[i].vendor_id,
            g_dev_info[i].product_id, type, g_dev_info[i].xfer_size);
    }
    printf("You can select a particular device by USB PID and VID.\n");
    printf("In case this is ambiguous, use bus and device number.\n");
    printf("Protocol is infered if possible and unspecified.\n");
    printf("Transfer size is infered if possible.\n");
    exit(1);
}

static bool dev_match(libusb_device *dev, struct dev_info_t *arg_di,
    int usb_bus, int usb_dev, int *db_idx)
{
    // match bus/dev
    if(usb_bus != -1)
        return libusb_get_bus_number(dev) == usb_bus && libusb_get_device_address(dev) == usb_dev;
    // get device descriptor
    struct libusb_device_descriptor desc;
    if(libusb_get_device_descriptor(dev, &desc))
        return false;
    // match command line vid/pid if specified
    if(arg_di->vendor_id != 0)
        return desc.idVendor == arg_di->vendor_id && desc.idProduct == arg_di->product_id;
    // match known vid/pid
    for(unsigned i = 0; i < sizeof(g_dev_info) / sizeof(g_dev_info[0]); i++)
        if(desc.idVendor == g_dev_info[i].vendor_id && desc.idProduct == g_dev_info[i].product_id)
        {
            if(db_idx)
                *db_idx = i;
            return true;
        }
    return false;
}

static void print_match(libusb_device *dev)
{
    struct libusb_device_descriptor desc;
    if(libusb_get_device_descriptor(dev, &desc))
        printf("????:????");
    else
        printf("%04x:%04x", desc.idVendor, desc.idProduct);
    printf(" @ %d.%d\n", libusb_get_bus_number(dev), libusb_get_device_address(dev));
}

static bool is_hid_dev(struct libusb_config_descriptor *desc)
{
    if(desc->bNumInterfaces != 1)
        return false;
    if(desc->interface[0].num_altsetting != 1)
        return false;
    const struct libusb_interface_descriptor *intf = &desc->interface[0].altsetting[0];
    if(intf->bNumEndpoints != 1)
        return false;
    if(intf->bInterfaceClass != LIBUSB_CLASS_HID  || intf->bInterfaceSubClass != 0 ||
            intf->bInterfaceProtocol != 0)
        return false;
    return true;
}

static bool is_recovery_dev(struct libusb_config_descriptor *desc)
{
    return false;
}

static enum dev_type_t probe_protocol(libusb_device_handle *dev)
{
    struct libusb_config_descriptor *desc;
    if(libusb_get_config_descriptor(libusb_get_device(dev), 0, &desc))
        goto Lerr;
    if(is_hid_dev(desc))
        return HID_DEVICE;
    if(is_recovery_dev(desc))
        return RECOVERY_DEVICE;
    Lerr:
    printf("Cannot probe protocol, please specify it on command line.\n");
    exit(11);
    return PROBE_DEVICE;
}

struct hid_item_t
{
    int tag;
    int type;
    int total_size;
    int data_offset;
    int data_size;
};

static bool hid_parse_short_item(uint8_t *buf, int size, struct hid_item_t *item)
{
    if(size == 0)
        return false;
    item->tag = buf[0] >> 4;
    item->data_size = buf[0] & 3;
    item->type = (buf[0] >> 2) & 3;
    item->data_offset = 1;
    item->total_size = 1 + item->data_size;
    return size >= item->total_size;
}

static bool hid_parse_item(uint8_t *buf, int size, struct hid_item_t *item)
{
    if(!hid_parse_short_item(buf, size, item))
        return false;
    /* long item ? */
    if(item->data_size == 2 && item->type == 3 && item->tag == 15)
    {
        item->tag = buf[2];
        item->data_size = buf[1];
        item->total_size = 3 + item->data_size;
        return size >= item->total_size;
    }
    else
        return true;
}

static int probe_hid_xfer_size(libusb_device_handle *dev)
{
    // FIXME detahc kernel and claim interface here ?
    /* get HID descriptor */
    uint8_t buffer[1024];
    int ret = libusb_control_transfer(dev, LIBUSB_ENDPOINT_IN | LIBUSB_RECIPIENT_INTERFACE,
        LIBUSB_REQUEST_GET_DESCRIPTOR, (LIBUSB_DT_REPORT << 8) | 0, 0, buffer,
        sizeof(buffer), 1000);
    if(ret <= 0)
        goto Lerr;
    /* this is not a real parser, since the HID descriptor of the device is
     * is mostly trivial, we assume that all reports are made up of one item
     * and simply compute the maximum of report size * report count */
    int xfer_size = 0;
    int report_size = 0;
    int report_count = 0;
    uint8_t *buf = buffer;
    int size = ret;
    while(true)
    {
        struct hid_item_t item;
        if(!hid_parse_item(buf, size, &item))
            break;
        if(item.type == /*global*/1)
        {
            if(item.tag == /*report count*/9)
                report_count = buf[item.data_offset];
            if(item.tag == /*report size*/7)
                report_size = buf[item.data_offset];
        }
        else if(item.type == /*main*/0)
        {
            if(item.tag == /*output*/9)
                xfer_size = MAX(xfer_size, report_count * report_size);
        }
        buf += item.total_size;
        size -= item.total_size;
    }
    return xfer_size / 8;

    Lerr:
    printf("Cannot probe transfer size, please specify it on command line.\n");
    exit(11);
    return 0;
}

static int probe_xfer_size(enum dev_type_t prot, libusb_device_handle *dev)
{
    if(prot == HID_DEVICE)
        return probe_hid_xfer_size(dev);
    printf("Cannot probe transfer size, please specify it on command line.\n");
    exit(10);
    return 0;
}

int main(int argc, char **argv)
{
    if(argc <= 1)
        usage();
    struct dev_info_t di = {.vendor_id = 0, .product_id = 0, .xfer_size = 0,
        .dev_type = PROBE_DEVICE};
    int usb_bus = -1;
    int usb_dev = -1;
    int force_xfer_size = 0;
    /* parse command line */
    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, '?'},
            {"debug", no_argument, 0, 'd'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "?dx:u:b:p:", long_options, NULL);
        if(c == -1)
            break;
        switch(c)
        {
            case -1:
                break;
            case 'd':
                g_debug = true;
                break;
            case '?':
                usage();
                break;
            case 'x':
            {
                char *end;
                force_xfer_size = strtoul(optarg, &end, 0);
                if(*end)
                {
                    printf("Invalid transfer size!\n");
                    exit(2);
                }
                break;
            }
            case 'u':
            {
                char *end;
                di.vendor_id = strtoul(optarg, &end, 16);
                if(*end != ':')
                {
                    printf("Invalid USB PID!\n");
                    exit(3);
                }
                di.product_id = strtoul(end + 1, &end, 16);
                if(*end)
                {
                    printf("Invalid USB VID!\n");
                    exit(4);
                }
                break;
            }
            case 'b':
            {
                char *end;
                usb_bus = strtol(optarg, &end, 0);
                if(*end != ':')
                {
                    printf("Invalid USB bus!\n");
                    exit(5);
                }
                usb_dev = strtol(end, &end, 0);
                if(*end)
                {
                    printf("Invalid USB device!\n");
                    exit(6);
                }
                break;
            }
            case 'p':
                if(strcmp(optarg, "hid") == 0)
                    di.dev_type = HID_DEVICE;
                else if(strcmp(optarg, "recovery") == 0)
                    di.dev_type = RECOVERY_DEVICE;
                else
                {
                    printf("Invalid protocol!\n");
                    exit(7);
                }
                break;
            default:
                printf("Internal error: unknown option '%c'\n", c);
                abort();
        }
    }

    if(optind + 1 != argc)
        usage();
    const char *filename = argv[optind];
    /* lookup device */
    libusb_init(NULL);
    libusb_set_debug(NULL, 3);
    libusb_device **list;
    ssize_t list_size = libusb_get_device_list(NULL, &list);
    libusb_device_handle *dev = NULL;
    int db_idx = -1;

    {
        libusb_device *mdev = NULL;
        int nr_matches = 0;
        for(int i = 0; i < list_size; i++)
        {
            // match bus/dev if specified
            if(dev_match(list[i], &di, usb_bus, usb_dev, &db_idx))
            {
                mdev = list[i];
                nr_matches++;
            }
        }
        if(nr_matches == 0)
        {
            printf("No device found\n");
            exit(8);
        }
        if(nr_matches > 1)
        {
            printf("Several devices match the specified parameters:\n");
            for(int i = 0; i < list_size; i++)
            {
                // match bus/dev if specified
                if(dev_match(list[i], &di, usb_bus, usb_dev, NULL))
                {
                    printf("  ");
                    print_match(list[i]);
                }
            }
        }
        printf("Device: ");
        print_match(mdev);
        libusb_open(mdev, &dev);
    }
    if(dev == NULL)
    {
        printf("Cannot open device\n");
        return 1;
    }
    /* get protocol */
    enum dev_type_t dev_type = PROBE_DEVICE;
    int xfer_size = di.xfer_size;
    if(db_idx >= 0)
    {
        dev_type = g_dev_info[db_idx].dev_type;
        xfer_size = g_dev_info[db_idx].xfer_size;
    }
    if(force_xfer_size > 0)
        xfer_size = force_xfer_size;
    if(dev_type == PROBE_DEVICE)
        dev_type = probe_protocol(dev);
    if(xfer_size == 0)
        xfer_size = probe_xfer_size(dev_type, dev);
    /* open file */
    FILE *f = fopen(filename, "r");
    if(f == NULL)
    {
        perror("Cannot open file");
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    printf("Transfer size: %d\n", xfer_size);
    int nr_xfers = (size + xfer_size - 1) / xfer_size;
    uint8_t *file_buf = malloc(nr_xfers * xfer_size);
    memset(file_buf, 0xff, nr_xfers * xfer_size); // pad with 0xff
    if(fread(file_buf, size, 1, f) != 1)
    {
        perror("read error");
        fclose(f);
        return 1;
    }
    fclose(f);
    /* send file */
    switch(dev_type)
    {
        case HID_DEVICE:
            send_hid(dev, xfer_size, file_buf, size, nr_xfers);
            break;
        case RECOVERY_DEVICE:
            send_recovery(dev, xfer_size, file_buf, size, nr_xfers);
            break;
        default:
            printf("unknown device type\n");
            break;
    }

    return 0;
}

