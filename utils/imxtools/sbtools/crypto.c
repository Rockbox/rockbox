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
#include "crypto.h"
#include <stdio.h>
#include <stdbool.h>
#ifdef CRYPTO_LIBUSB
#include "libusb.h"
#endif
#include "misc.h"

static enum crypto_method_t cur_method = CRYPTO_NONE;
static byte key[16];
static uint16_t usb_vid, usb_pid;

void crypto_setup(enum crypto_method_t method, void *param)
{
    cur_method = method;
    switch(method)
    {
        case CRYPTO_KEY:
            memcpy(key, param, sizeof(key));
            break;
        case CRYPTO_USBOTP:
        {
            uint32_t value = *(uint32_t *)param;
            usb_vid = value >> 16;
            usb_pid = value & 0xffff;
            break;
        }
        default:
            break;
    }
}

int crypto_apply(
    byte *in_data, /* Input data */
    byte *out_data, /* Output data (or NULL) */
    int nr_blocks, /* Number of blocks (one block=16 bytes) */
    byte iv[16], /* Key */
    byte (*out_cbc_mac)[16], /* CBC-MAC of the result (or NULL) */
    int encrypt)
{
    if(cur_method == CRYPTO_KEY)
    {
        cbc_mac(in_data, out_data, nr_blocks, key, iv, out_cbc_mac, encrypt);
        return CRYPTO_ERROR_SUCCESS;
    }
    #ifdef CRYPTO_LIBUSB
    else if(cur_method == CRYPTO_USBOTP)
    {
        if(out_cbc_mac && !encrypt)
            memcpy(*out_cbc_mac, in_data + 16 * (nr_blocks - 1), 16);
        
        libusb_device_handle *handle = NULL;
        libusb_context *ctx;
        /* init library */
        libusb_init(&ctx);
        libusb_set_debug(NULL,3);
        /* open device */
        handle = libusb_open_device_with_vid_pid(ctx, usb_vid, usb_pid);
        if(handle == NULL)
        {
            printf("usbotp: cannot open device %04x:%04x\n", usb_vid, usb_pid);
            return CRYPTO_ERROR_NODEVICE;
        }
        /* get device pointer */
        libusb_device *mydev = libusb_get_device(handle);
        if(g_debug)
            printf("usbotp: device found at %d:%d\n", libusb_get_bus_number(mydev),
                libusb_get_device_address(mydev));
        int config_id;
        /* explore configuration */
        libusb_get_configuration(handle, &config_id);
        struct libusb_config_descriptor *config;
        libusb_get_active_config_descriptor(mydev, &config);

        if(g_debug)
        {
            printf("usbotp: configuration: %d\n", config_id);
            printf("usbotp: interfaces: %d\n", config->bNumInterfaces);
        }
        
        const struct libusb_endpoint_descriptor *endp = NULL;
        int intf, intf_alt;
        for(intf = 0; intf < config->bNumInterfaces; intf++)
            for(intf_alt = 0; intf_alt < config->interface[intf].num_altsetting; intf_alt++)
                for(int ep = 0; ep < config->interface[intf].altsetting[intf_alt].bNumEndpoints; ep++)
                {
                    endp = &config->interface[intf].altsetting[intf_alt].endpoint[ep];
                    if((endp->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_TRANSFER_TYPE_INTERRUPT &&
                            (endp->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN)
                        goto Lfound;
                }
        libusb_close(handle);
        printf("usbotp: No suitable endpoint found\n");
        return CRYPTO_ERROR_BADENDP;

        if(g_debug)
        {
            printf("usbotp: use interface %d, alt %d\n", intf, intf_alt);
            printf("usbotp: use endpoint %d\n", endp->bEndpointAddress);
        }
        Lfound:
        if(libusb_claim_interface(handle, intf) != 0)
        {
            if(g_debug)
                printf("usbotp: claim error\n");
            return CRYPTO_ERROR_CLAIMFAIL;
        }

        int buffer_size = 16 + 16 * nr_blocks;
        unsigned char *buffer = xmalloc(buffer_size);
        memcpy(buffer, iv, 16);
        memcpy(buffer + 16, in_data, 16 * nr_blocks);
        int  ret = libusb_control_transfer(handle,
            LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_DEVICE,
            0xaa, encrypt ? 0xeeee : 0xdddd, 0, buffer, buffer_size, 1000);
        if(ret < 0)
        {
            if(g_debug)
                printf("usbotp: control transfer failed: %d\n", ret);
            libusb_release_interface(handle, intf);
            libusb_close(handle);
            return CRYPTO_ERROR_DEVREJECT;
        }

        int recv_size;
        ret = libusb_interrupt_transfer(handle, endp->bEndpointAddress, buffer,
            buffer_size, &recv_size, 1000);
        libusb_release_interface(handle, intf);
        libusb_close(handle);
        
        if(ret < 0)
        {
            if(g_debug)
                printf("usbotp: interrupt transfer failed: %d\n", ret);
            return CRYPTO_ERROR_DEVSILENT;
        }
        if(recv_size != buffer_size)
        {
            if(g_debug)
                printf("usbotp: device returned %d bytes, expected %d\n", recv_size,
                    buffer_size);
            return CRYPTO_ERROR_DEVERR;
        }

        if(out_data)
            memcpy(out_data, buffer + 16, 16 * nr_blocks);
        if(out_cbc_mac && encrypt)
            memcpy(*out_cbc_mac, buffer + buffer_size - 16, 16);
        
        return CRYPTO_ERROR_SUCCESS;
    }
    #endif
    else
        return CRYPTO_ERROR_BADSETUP;
}

int crypto_cbc(
    byte *in_data, /* Input data */
    byte *out_data, /* Output data (or NULL) */
    int nr_blocks, /* Number of blocks (one block=16 bytes) */
    struct crypto_key_t *key, /* Key */
    byte iv[16], /* IV */
    byte (*out_cbc_mac)[16], /* CBC-MAC of the result (or NULL) */
    int encrypt)
{
    crypto_setup(key->method, (void *)key->u.param);
    return crypto_apply(in_data, out_data, nr_blocks, iv, out_cbc_mac, encrypt);
}
