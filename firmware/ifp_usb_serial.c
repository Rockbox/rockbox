/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Tomasz Malesinski
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

/*
#define LCD_DEBUG
#define BUTTONS
*/

/* #include "config.h" */
#include <stdlib.h>
#include "pnx0101.h"
#include "ifp_usb_serial.h"

#ifdef BUTTONS
#include "kernel.h"
#include "button.h"
#include "system.h"
#endif

#ifdef LCD_DEBUG
#include "lcd.h"
#include "sprintf.h"
#endif


#define ISP1582_BASE (0x24100000)
#define ISP1582_ADDRESS (*(volatile unsigned char *)ISP1582_BASE)
#define ISP1582_MODE    (*(volatile unsigned short *)(ISP1582_BASE + 0xc))
#define ISP1582_INTCONF (*(volatile unsigned char *)(ISP1582_BASE + 0x10))
#define ISP1582_OTG     (*(volatile unsigned char *)(ISP1582_BASE + 0x12))
#define ISP1582_INTEN   (*(volatile unsigned long *)(ISP1582_BASE + 0x14))

#define ISP1582_EPINDEX (*(volatile unsigned char *)(ISP1582_BASE + 0x2c))
#define ISP1582_CTRLFUN (*(volatile unsigned char *)(ISP1582_BASE + 0x28))
#define ISP1582_DATA    (*(volatile unsigned short *)(ISP1582_BASE + 0x20))
#define ISP1582_BUFLEN  (*(volatile unsigned short *)(ISP1582_BASE + 0x1c))
#define ISP1582_BUFSTAT (*(volatile unsigned char *)(ISP1582_BASE + 0x1e))
#define ISP1582_MAXPKSZ (*(volatile unsigned short *)(ISP1582_BASE + 0x04))
#define ISP1582_EPTYPE  (*(volatile unsigned short *)(ISP1582_BASE + 0x08))

#define ISP1582_INT     (*(volatile unsigned long *)(ISP1582_BASE + 0x18))
#define ISP1582_CHIPID  (*(volatile unsigned long *)(ISP1582_BASE + 0x70))
#define ISP1582_FRAMENO (*(volatile unsigned short *)(ISP1582_BASE + 0x74))
#define ISP1582_UNLOCK  (*(volatile unsigned short *)(ISP1582_BASE + 0x7c))

#define ISP1582_UNLOCK_CODE 0xaa37

#define DIR_RX 0
#define DIR_TX 1

#define TYPE_BULK 2

#define STATE_DEFAULT 0
#define STATE_ADDRESS 1
#define STATE_CONFIGURED 2

#define N_ENDPOINTS 2

struct usb_endpoint
{
    unsigned char *out_buf;
    short out_len;
    short out_ptr;
    void (*out_done)(int, unsigned char *, int);
    unsigned char out_in_progress;

    unsigned char *in_buf;
    short in_min_len;
    short in_max_len;
    short in_ptr;
    void (*in_done)(int, unsigned char *, int);
    unsigned char in_ack;

    unsigned char halt[2];
    unsigned char enabled[2];
    short max_pkt_size[2];
};

static char usb_connect_state;

static struct usb_endpoint endpoints[N_ENDPOINTS];

static unsigned char setup_pkt_buf[8];
static unsigned char setup_out_buf[8];
static unsigned char usb_state;
static unsigned char usb_remote_wakeup;

#ifdef LCD_DEBUG
static unsigned char int_count[32];

static int log_pos_x = 0;
static int log_pos_y = 3;
#endif

static void nop_f(void)
{
}

#ifdef LCD_DEBUG
static void log_char(char c)
{
    char s[2];

    s[0] = c;
    s[1] = 0;

    lcd_puts(log_pos_x, log_pos_y, s);
    lcd_update();
    log_pos_x++;
    if (log_pos_x >= 16)
    {
        log_pos_x = 0;
        log_pos_y++;
        if (log_pos_y > 5)
            log_pos_y = 3;
    }
}
#else
#define log_char(c)
#endif

#define SERIAL_BUF_SIZE 1024

struct serial_fifo
{
    unsigned char buf[SERIAL_BUF_SIZE];
    short head, tail;
};

static struct serial_fifo serial_in_fifo;
static struct serial_fifo serial_out_fifo;
static unsigned char serial_in_pkt[64];

static unsigned char device_descriptor[18] = {
    0x12,       /* length */
    0x01,       /* descriptor type */
    0x10, 0x01, /* USB version (1.1) */
    0xff, 0xff, /* class and subclass */
    0xff,       /* protocol */
    0x40,       /* max packet size 0 */
    0x02, 0x41, /* vendor (iRiver) */
    0x07, 0xee, /* product (0xee07) */
    0x01, 0x00, /* device version */
    0x01,       /* manufacturer string */
    0x02,       /* product string */
    0x00,       /* serial number string */
    0x01        /* number of configurations */
};

static unsigned char cfg_descriptor[32] = {
    0x09,       /* length */
    0x02,       /* descriptor type */
    0x20, 0x00, /* total length */
    0x01,       /* number of interfaces */
    0x01,       /* configuration value */
    0x00,       /* configuration string */
    0x80,       /* attributes (none) */
    0x32,       /* max power (100 mA) */
    /* interface descriptor */
    0x09,       /* length */
    0x04,       /* descriptor type */
    0x00,       /* interface number */
    0x00,       /* alternate setting */
    0x02,       /* number of endpoints */
    0xff,       /* interface class */
    0xff,       /* interface subclass */
    0xff,       /* interface protocol */
    0x00,       /* interface string */
    /* endpoint IN */
    0x07,       /* length */
    0x05,       /* descriptor type */
    0x81,       /* endpoint 1 IN */
    0x02,       /* attributes (bulk) */
    0x40, 0x00, /* max packet size */
    0x00,       /* interval */
    /* endpoint OUT */
    0x07,       /* length */
    0x05,       /* descriptor type */
    0x01,       /* endpoint 1 IN */
    0x02,       /* attributes (bulk) */
    0x40, 0x00, /* max packet size */
    0x00        /* interval */
};

static unsigned char lang_descriptor[4] = {
    0x04,       /* length */
    0x03,       /* descriptor type */
    0x09, 0x04  /* English (US) */
};

#define N_STRING_DESCRIPTORS 2

static unsigned char string_descriptor_vendor[] = {
    0x2e, 0x03,
    'i', 0, 'R', 0, 'i', 0, 'v', 0, 'e', 0, 'r', 0, ' ', 0, 'L', 0,
    't', 0, 'd', 0, ' ', 0, 'a', 0, 'n', 0, 'd', 0, ' ', 0, 'R', 0,
    'o', 0, 'c', 0, 'k', 0, 'b', 0, 'o', 0, 'x', 0};
    
static unsigned char string_descriptor_product[] = {
    0x1c, 0x03,
    'i', 0, 'R', 0, 'i', 0, 'v', 0, 'e', 0, 'r', 0, ' ', 0, 'i', 0,
    'F', 0, 'P', 0, '7', 0, '0', 0, '0', 0};

static unsigned char *string_descriptor[N_STRING_DESCRIPTORS] = {
    string_descriptor_vendor,
    string_descriptor_product
};

static inline int ep_index(int n, int dir)
{
    return (n << 1) | dir;
}

static inline int epidx_dir(int idx)
{
    return idx & 1;
}

static inline int epidx_n(int idx)
{
    return idx >> 1;
}

int usb_connected(void)
{
    return GPIO7_READ & 1;
}

static inline void usb_select_endpoint(int idx)
{
    ISP1582_EPINDEX = idx;
}

static inline void usb_select_setup_endpoint(void)
{
    ISP1582_EPINDEX = 0x20;
}

void usb_setup_endpoint(int idx, int max_pkt_size, int type)
{
    struct usb_endpoint *ep;

    usb_select_endpoint(idx);
    ISP1582_MAXPKSZ = max_pkt_size;
    /* |= is in the original firmware */
    ISP1582_EPTYPE |= 0x1c | type;
    /* clear buffer */
    ISP1582_CTRLFUN |= 0x10;
    ISP1582_INTEN |= (1 << (10 + idx));

    ep = &(endpoints[epidx_n(idx)]);
    ep->halt[epidx_dir(idx)] = 0;
    ep->enabled[epidx_dir(idx)] = 1;
    ep->out_in_progress = 0;
    ep->in_min_len = -1;
    ep->in_ack = 0;
    ep->max_pkt_size[epidx_dir(idx)] = max_pkt_size;
}

void usb_disable_endpoint(int idx)
{
    usb_select_endpoint(idx);
    ISP1582_EPTYPE &= 8;
    ISP1582_INTEN &= ~(1 << (10 + idx));
    endpoints[epidx_n(idx)].enabled[epidx_dir(idx)] = 1;
}

void usb_reconnect(void)
{
    int i;
    ISP1582_MODE &= ~1; /* SOFTCT off */
    for (i = 0; i < 10000; i++)
        nop_f();
    ISP1582_MODE |= 1; /* SOFTCT on */
}

void usb_cleanup(void)
{
    ISP1582_MODE &= ~1; /* SOFTCT off */
}

void usb_setup(int reset)
{
    int i;

    for (i = 0; i < N_ENDPOINTS; i++)
        endpoints[i].enabled[0] = endpoints[i].enabled[1] = 0;

    ISP1582_UNLOCK = ISP1582_UNLOCK_CODE;
    if (!reset)
        ISP1582_MODE = 0x88; /* CLKAON | GLINTENA */
    ISP1582_INTCONF = 0x57;
    ISP1582_INTEN = 0xd39;

    ISP1582_ADDRESS = reset ? 0x80: 0;

    usb_setup_endpoint(ep_index(0, DIR_RX), 64, 0);
    usb_setup_endpoint(ep_index(0, DIR_TX), 64, 0);
    
    ISP1582_MODE |= 1; /* SOFTCT on */

    usb_state = STATE_DEFAULT;
    usb_remote_wakeup = 0;
}

static int usb_get_packet(unsigned char *buf, int max_len)
{
    int len, i;
    len = ISP1582_BUFLEN;

    if (max_len < 0 || max_len > len)
        max_len = len;

    i = 0;
    while (i < len)
    {
        unsigned short d = ISP1582_DATA;
        if (i < max_len)
            buf[i] = d & 0xff;
        i++;
        if (i < max_len)
            buf[i] = (d >> 8) & 0xff;
        i++;
    }
    return max_len;
}

static void usb_receive(int n)
{
    int len;

    if (endpoints[n].halt[DIR_RX]
        || !endpoints[n].enabled[DIR_RX]
        || endpoints[n].in_min_len < 0
        || !endpoints[n].in_ack)
        return;

    endpoints[n].in_ack = 0;

    usb_select_endpoint(ep_index(n, DIR_RX));

    len = usb_get_packet(endpoints[n].in_buf + endpoints[n].in_ptr,
                         endpoints[n].in_max_len - endpoints[n].in_ptr);
    endpoints[n].in_ptr += len;
    if (endpoints[n].in_ptr >= endpoints[n].in_min_len) {
        endpoints[n].in_min_len = -1;
        if (endpoints[n].in_done)
            (*(endpoints[n].in_done))(n, endpoints[n].in_buf,
                                      endpoints[n].in_ptr);
    }
}

static int usb_out_buffer_full(int ep)
{
    usb_select_endpoint(ep_index(ep, DIR_TX));
    if (ISP1582_EPTYPE & 4)
        return (ISP1582_BUFSTAT & 3) == 3;
    else
        return (ISP1582_BUFSTAT & 3) != 0;
}

static void usb_send(int n)
{
    int max_pkt_size, len;
    int i;
    unsigned char *p;

#ifdef LCD_DEBUG
    if (endpoints[n].halt[DIR_TX])
        log_char('H');
    if (!endpoints[n].out_in_progress)
        log_char('$');
#endif

    if (endpoints[n].halt[DIR_TX]
        || !endpoints[n].enabled[DIR_TX]
        || !endpoints[n].out_in_progress)
        return;

    if (endpoints[n].out_ptr < 0)
    {
        endpoints[n].out_in_progress = 0;
        if (endpoints[n].out_done)
            (*(endpoints[n].out_done))(n, endpoints[n].out_buf,
                                       endpoints[n].out_len);
        return;
    }
    
    if (usb_out_buffer_full(n))
    {
        log_char('F');
        return;
    }
    
    usb_select_endpoint(ep_index(n, DIR_TX));
    max_pkt_size = endpoints[n].max_pkt_size[DIR_TX];
    len = endpoints[n].out_len - endpoints[n].out_ptr;
    if (len > max_pkt_size)
        len = max_pkt_size;

    log_char('0' + (len % 10));
    ISP1582_BUFLEN = len;
    p = endpoints[n].out_buf + endpoints[n].out_ptr;
    i = 0;
    while (len - i >= 2) {
        ISP1582_DATA = p[i] | (p[i + 1] << 8);
        i += 2;
    }
    if (i < len)
        ISP1582_DATA = p[i];

    endpoints[n].out_ptr += len;

/*
    if (endpoints[n].out_ptr == endpoints[n].out_len
        && len < max_pkt_size)
*/
    if (endpoints[n].out_ptr == endpoints[n].out_len)
        endpoints[n].out_ptr = -1;
}

static void usb_stall_endpoint(int idx)
{
    usb_select_endpoint(idx);
    ISP1582_CTRLFUN |= 1;
    endpoints[epidx_n(idx)].halt[epidx_dir(idx)] = 1;
}

static void usb_unstall_endpoint(int idx)
{
    usb_select_endpoint(idx);
    ISP1582_CTRLFUN &= ~1;
    ISP1582_EPTYPE &= ~8;
    ISP1582_EPTYPE |= 8;
    ISP1582_CTRLFUN |= 0x10;
    if (epidx_dir(idx) == DIR_TX)
        endpoints[epidx_n(idx)].out_in_progress = 0;
    else
    {
        endpoints[epidx_n(idx)].in_min_len = -1;
        endpoints[epidx_n(idx)].in_ack = 0;
    }
    endpoints[epidx_n(idx)].halt[epidx_dir(idx)] = 0;
}

static void usb_status_ack(int dir)
{
    log_char(dir ? '@' : '#');
    usb_select_endpoint(ep_index(0, dir));
    ISP1582_CTRLFUN |= 2;
}

static void usb_set_address(int adr)
{
    ISP1582_ADDRESS = adr | 0x80;
}

static void usb_data_stage_enable(int dir)
{
    usb_select_endpoint(ep_index(0, dir));
    ISP1582_CTRLFUN |= 4;
}

static void usb_request_error(void)
{
    usb_stall_endpoint(ep_index(0, DIR_TX));
    usb_stall_endpoint(ep_index(0, DIR_RX));
}

static void usb_receive_block(unsigned char *buf, int min_len,
                              int max_len,
                              void (*in_done)(int, unsigned char *, int),
                              int ep)
{
    endpoints[ep].in_done = in_done;
    endpoints[ep].in_buf = buf;
    endpoints[ep].in_max_len = max_len;
    endpoints[ep].in_min_len = min_len;
    endpoints[ep].in_ptr = 0;
    usb_receive(ep);
}

static void usb_send_block(unsigned char *buf, int len,
                           void (*done)(int, unsigned char *, int),
                           int ep)
{
    endpoints[ep].out_done = done;
    endpoints[ep].out_buf = buf;
    endpoints[ep].out_len = len;
    endpoints[ep].out_ptr = 0;
    endpoints[ep].out_in_progress = 1;
    usb_send(ep);
}

static void out_send_status(int n, unsigned char *buf, int len)
{
    (void)n;
    (void)buf;
    (void)len;
    usb_status_ack(DIR_RX);
}

static void usb_send_block_and_status(unsigned char *buf, int len, int ep)
{
    usb_send_block(buf, len, out_send_status, ep);
}

static void usb_setup_set_address(int adr)
{
    usb_set_address(adr);
    usb_state = adr ? STATE_ADDRESS : STATE_DEFAULT;
    usb_status_ack(DIR_TX);
}

static void usb_send_descriptor(unsigned char *device_descriptor,
                                int descriptor_len, int buffer_len)
{
    int len = descriptor_len < buffer_len ? descriptor_len : buffer_len;
    usb_send_block_and_status(device_descriptor, len, 0);
}

static void usb_setup_get_descriptor(int type, int index, int lang, int len)
{
    (void)lang;
    usb_data_stage_enable(DIR_TX);
    switch (type)
    {
        case 1:
            if (index == 0)
                usb_send_descriptor(device_descriptor,
                                    sizeof(device_descriptor), len);
            else
                usb_request_error();
            break;
        case 2:
            if (index == 0)
                usb_send_descriptor(cfg_descriptor,
                                    sizeof(cfg_descriptor), len);
            else
                usb_request_error();
            break;
        case 3:
            if (index == 0)
                usb_send_descriptor(lang_descriptor,
                                    sizeof(lang_descriptor), len);
            else if (index <= N_STRING_DESCRIPTORS)
                usb_send_descriptor(string_descriptor[index - 1],
                                    string_descriptor[index - 1][0],
                                    len);
            else
                usb_request_error();
            break;
        default:
            usb_request_error();
    }
}

static void usb_setup_get_configuration(void)
{
    setup_out_buf[0] = (usb_state == STATE_CONFIGURED) ? 1 : 0;
    usb_data_stage_enable(DIR_TX);
    usb_send_block_and_status(setup_out_buf, 1, 0);
}

static void usb_setup_interface(void)
{
    usb_setup_endpoint(ep_index(1, DIR_RX), 64, TYPE_BULK);
    usb_setup_endpoint(ep_index(1, DIR_TX), 64, TYPE_BULK);
}

static void usb_setup_set_configuration(int value)
{
    switch (value)
    {
        case 0:
            usb_disable_endpoint(ep_index(1, DIR_RX));
            usb_disable_endpoint(ep_index(1, DIR_TX));
            usb_state = STATE_ADDRESS;
            usb_status_ack(DIR_TX);
            break;
        case 1:
            usb_setup_interface();
            usb_state = STATE_CONFIGURED;
            usb_status_ack(DIR_TX);
            break;
        default:
            usb_request_error();
    }
}

static void usb_send_status(void)
{
    usb_data_stage_enable(DIR_TX);
    usb_send_block_and_status(setup_out_buf, 2, 0);
}

static void usb_setup_get_device_status(void)
{
    setup_out_buf[0] = (usb_remote_wakeup != 0) ? 2 : 0;
    setup_out_buf[1] = 0;
    usb_send_status();
}

static void usb_setup_clear_device_feature(int feature)
{
    if (feature == 1) {
        usb_remote_wakeup = 0;
        usb_status_ack(DIR_TX);
    } else
        usb_request_error();
}

static void usb_setup_set_device_feature(int feature)
{
    if (feature == 1) {
        usb_remote_wakeup = 1;
        usb_status_ack(DIR_TX);
    } else
        usb_request_error();
}

static void usb_setup_clear_endpoint_feature(int endpoint, int feature)
{
    if (usb_state != STATE_CONFIGURED || feature != 0)
        usb_request_error();
    else if ((endpoint & 0xf) == 1)
    {
        usb_unstall_endpoint(ep_index(endpoint & 0xf, endpoint >> 7));
        usb_status_ack(DIR_TX);
    }
    else
        usb_request_error();
}

static void usb_setup_set_endpoint_feature(int endpoint, int feature)
{
    if (usb_state != STATE_CONFIGURED || feature != 0)
        usb_request_error();
    else if ((endpoint & 0xf) == 1)
    {
        usb_stall_endpoint(ep_index(endpoint & 0xf, endpoint >> 7));
        usb_status_ack(DIR_TX);
    }
    else
        usb_request_error();
}

static void usb_setup_get_interface_status(int interface)
{
    if (usb_state != STATE_CONFIGURED || interface != 0)
        usb_request_error();
    else
    {
        setup_out_buf[0] = setup_out_buf[1] = 0;
        usb_send_status();
    }
}

static void usb_setup_get_endpoint_status(int endpoint)
{
    if ((usb_state == STATE_CONFIGURED && (endpoint & 0xf) <= 1)
        || (usb_state == STATE_ADDRESS && (endpoint & 0xf) == 0))
    {
        setup_out_buf[0] = endpoints[endpoint & 0xf].halt[endpoint >> 7];
        setup_out_buf[1] = 0;
        usb_send_status();
    }
    else
        usb_request_error();
}

static void usb_setup_get_interface(int interface)
{
    if (usb_state != STATE_CONFIGURED || interface != 0)
        usb_request_error();
    else
    {
        setup_out_buf[0] = 0;
        usb_data_stage_enable(DIR_TX);
        usb_send_block_and_status(setup_out_buf, 1, 0);
    }
}

static void usb_setup_set_interface(int interface, int setting)
{
    if (usb_state != STATE_CONFIGURED || interface != 0 || setting != 0)
        usb_request_error();
    else
    {
        usb_setup_interface();
        usb_status_ack(DIR_TX);
    }
}

static void usb_handle_setup_pkt(unsigned char *pkt)
{
    switch ((pkt[0] << 8) | pkt[1])
    {
        case 0x0005:
            log_char('A');
            usb_setup_set_address(pkt[2]);
            break;
        case 0x8006:
            log_char('D');
            usb_setup_get_descriptor(pkt[3], pkt[2], (pkt[5] << 8) | pkt[4],
                                     (pkt[7] << 8) | pkt[6]);
            break;
        case 0x8008:
            usb_setup_get_configuration();
            break;
        case 0x0009:
            usb_setup_set_configuration(pkt[2]);
            break;
        case 0x8000:
            usb_setup_get_device_status();
            break;
        case 0x8100:
            usb_setup_get_interface_status(pkt[4]);
            break;
        case 0x8200:
            usb_setup_get_endpoint_status(pkt[4]);
            break;
        case 0x0001:
            usb_setup_clear_device_feature(pkt[2]);
            break;
        case 0x0201:
            usb_setup_clear_endpoint_feature(pkt[4], pkt[2]);
            break;
        case 0x0003:
            usb_setup_set_device_feature(pkt[2]);
            break;
        case 0x0203:
            usb_setup_set_endpoint_feature(pkt[4], pkt[2]);
            break;
        case 0x810a:
            usb_setup_get_interface(pkt[4]);
            break;
        case 0x010b:
            usb_setup_set_interface(pkt[4], pkt[2]);
            break;
        case 0x0103:
            /* set interface feature */
        case 0x0101:
            /* clear interface feature */
        case 0x0007:
            /* set descriptor */
        case 0x820c:
            /* synch frame */
        default:
            usb_request_error();
    }
}

static void usb_handle_setup_rx(void)
{
    int len;
#ifdef LCD_DEBUG
    char s[20];
    int i;
#endif
    usb_select_setup_endpoint();
    len = usb_get_packet(setup_pkt_buf, 8);

    if (len == 8)
        usb_handle_setup_pkt(setup_pkt_buf);

#ifdef LCD_DEBUG
/*
    snprintf(s, 10, "l%02x", len);
    lcd_puts(0, 5, s);
*/
    for (i = 0; i < 8; i++)
        snprintf(s + i * 2, 3, "%02x", setup_pkt_buf[i]);
    lcd_puts(0, 0, s);
    lcd_update();
#endif
}

static void usb_handle_data_int(int ep, int dir)
{
    if (dir == DIR_TX)
        usb_send(ep);
    else
    {
        endpoints[ep].in_ack = 1;
        usb_receive(ep);
    }
}

static void usb_handle_int(int i)
{
#ifdef LCD_DEBUG
/*
    char s[10];
    snprintf(s, sizeof(s), "%02d", i);
    lcd_puts(0, 2, s);
    lcd_update();
*/
    int_count[i]++;
    if (i == 10)
        log_char('o');
    if (i == 11)
        log_char('i');
    if (i == 12)
        log_char('O');
    if (i == 13)
        log_char('I');
#endif

    if (i >= 10)
        usb_handle_data_int((i - 10) / 2, i % 2);
    else
    {
        switch (i)
        {
            case 0:
                log_char('r');
                usb_setup(1);
                break;
            case 8:
                log_char('s');
                usb_handle_setup_rx();
                break;
        }
    }
        
}

void usb_handle_interrupts(void)
{
#ifdef LCD_DEBUG
    char s[20];
#endif

    while (1)
    {
        unsigned long ints;
        int i;

#ifdef LCD_DEBUG
        /*
        snprintf(s, sizeof(s), "i%08lx", ISP1582_INT);
        lcd_puts(0, 2, s);
        */
#endif

        ints = ISP1582_INT & ISP1582_INTEN;
        if (!ints) break;

        i = 0;
        while (!(ints & (1 << i)))
            i++;
        ISP1582_INT = 1 << i;
        usb_handle_int(i);

#ifdef LCD_DEBUG
        for (i = 0; i < 8; i++)
            snprintf(s + i * 2, 3, "%02x", int_count[i]);
        lcd_puts(0, 6, s);
        for (i = 0; i < 8; i++)
            snprintf(s + i * 2, 3, "%02x", int_count[i + 8]);
        lcd_puts(0, 7, s);
#endif
    }
#ifdef LCD_DEBUG
/*
    lcd_puts(0, 3, usb_connected() ? "C" : "N");
    lcd_update();
*/
#endif
}

static inline int fifo_mod(int n)
{
    return (n >= SERIAL_BUF_SIZE) ? n - SERIAL_BUF_SIZE : n;
}

static void fifo_init(struct serial_fifo *fifo)
{
    fifo->head = fifo->tail = 0;
}

static int fifo_empty(struct serial_fifo *fifo)
{
    return fifo->head == fifo->tail;
}

static int fifo_full(struct serial_fifo *fifo)
{
    return fifo_mod(fifo->head + 1) == fifo->tail;
}

static void fifo_remove(struct serial_fifo *fifo, int n)
{
    fifo->tail = fifo_mod(fifo->tail + n);
}

/*
  Not used:
static void fifo_add(struct serial_fifo *fifo, int n)
{
    fifo->head = fifo_mod(fifo->head + n);
}

static void fifo_free_block(struct serial_fifo *fifo,
                           unsigned char **ptr, int *len)
{
    *ptr = fifo->buf + fifo->head;
    if (fifo->head >= fifo->tail)
    {
        int l = SERIAL_BUF_SIZE - fifo->head;
        if (fifo->tail == 0)
            l--;
        *len = l;
    }
    else
        *len = fifo->tail - fifo->head - 1;
}
*/

static int fifo_free_space(struct serial_fifo *fifo)
{
    if (fifo->head >= fifo->tail)
        return SERIAL_BUF_SIZE - (fifo->head - fifo->tail) - 1;
    else
        return fifo->tail - fifo->head - 1;
}

static int fifo_get_byte(struct serial_fifo *fifo)
{
    int r = fifo->buf[fifo->tail];
    fifo->tail = fifo_mod(fifo->tail + 1);
    return r;
}

static void fifo_put_byte(struct serial_fifo *fifo, int b)
{
    fifo->buf[fifo->head] = b;
    fifo->head = fifo_mod(fifo->head + 1);
}

static void fifo_full_block(struct serial_fifo *fifo,
                            unsigned char **ptr, int *len)
{
    *ptr = fifo->buf + fifo->tail;
    if (fifo->head >= fifo->tail)
        *len = fifo->head - fifo->tail;
    else
        *len = SERIAL_BUF_SIZE - fifo->tail;
}

static void serial_fill_in_fifo(int ep, unsigned char *buf, int len);
static void serial_free_out_fifo(int ep, unsigned char *buf, int len);

static void serial_restart_input(int ep)
{
    if (fifo_free_space(&serial_in_fifo) >= 64)
        usb_receive_block(serial_in_pkt, 1, 64, serial_fill_in_fifo, ep);
}

static void serial_fill_in_fifo(int ep, unsigned char *buf, int len)
{
    int i;
    for (i = 0; i < len; i++)
        fifo_put_byte(&serial_in_fifo, buf[i]);
    serial_restart_input(ep);
}

static void serial_restart_output(int ep)
{
    unsigned char *block;
    int blen;
    fifo_full_block(&serial_out_fifo, &block, &blen);
    if (blen)
    {
#ifdef LCD_DEBUG
        char s[20];
        snprintf(s, sizeof(s), "o%03lx/%03x", block - serial_out_fifo.buf,
                 blen);
        lcd_puts(0, 2, s);
        lcd_update();
#endif
        usb_send_block(block, blen, serial_free_out_fifo, ep);
    }
}

static void serial_free_out_fifo(int ep, unsigned char *buf, int len)
{
    (void)buf;
    fifo_remove(&serial_out_fifo, len);
    serial_restart_output(ep);
}

void usb_serial_handle(void)
{
#ifdef BUTTONS
    static int t = 0;

    t++;
    if (t >= 1000)
    {
        int b;
        t = 0;
        yield();
        b = button_get(false);
        if (b == BUTTON_PLAY)
            system_reboot();
        else if (b & BUTTON_REL)
            usb_reconnect();
    }
#endif
    
    
    if (!usb_connect_state)
    {
        if (usb_connected())
        {
            int i;
            GPIO3_SET = 4;
            (*(volatile unsigned long *)0x80005004) = 2;
            (*(volatile unsigned long *)0x80005008) = 0;
            for (i = 0; i < 100000; i++)
                nop_f();
            usb_setup(0);
            usb_connect_state = 1;
        }
    }
    else
    {
        if (!usb_connected())
        {
            usb_connect_state = 0;
            usb_cleanup();
        }
        else
        {
            usb_handle_interrupts();
            
            if (usb_state == STATE_CONFIGURED)
            {
                if (endpoints[1].in_min_len < 0)
                    serial_restart_input(1);
                if (!endpoints[1].out_in_progress)
                    serial_restart_output(1);
            }
        }
    }
}


/*
  Not used:
static int usb_serial_in_empty(void)
{
    return fifo_empty(&serial_in_fifo);
}
*/

int usb_serial_get_byte(void)
{
    while (fifo_empty(&serial_in_fifo))
        usb_serial_handle();
    return fifo_get_byte(&serial_in_fifo);
}

int usb_serial_try_get_byte(void)
{
    int r;
    if (fifo_empty(&serial_in_fifo))
        r = -1;
    else
        r = fifo_get_byte(&serial_in_fifo);
    usb_serial_handle();
    return r;
}

/*
  Not used:
static int usb_serial_out_full(void)
{
    return fifo_full(&serial_out_fifo);
}
*/

void usb_serial_put_byte(int b)
{
    while (fifo_full(&serial_out_fifo))
        usb_serial_handle();
    fifo_put_byte(&serial_out_fifo, b);
    usb_serial_handle();
}

int usb_serial_try_put_byte(int b)
{
    int r = -1;
    if (!fifo_full(&serial_out_fifo))
    {
        fifo_put_byte(&serial_out_fifo, b);
        r = 0;
    }
    usb_serial_handle();
    return r;
}

void usb_serial_init(void)
{
    fifo_init(&serial_in_fifo);
    fifo_init(&serial_out_fifo);
    usb_connect_state = 0;
}
