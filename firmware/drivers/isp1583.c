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
 * Copyright (C) 2008 by Maurus Cuelenaere
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
 
#include "config.h"
#include "usb_ch9.h"
#include "usb_drv.h"
#include "usb_core.h"
#include "isp1583.h"
#include "thread.h"
#include "logf.h"
#include <stdio.h>

#define DIR_RX                      0
#define DIR_TX                      1
 
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
    short type;
    char allocation;
};

static unsigned char setup_pkt_buf[8];
static struct usb_endpoint endpoints[NUM_ENDPOINTS];

static bool high_speed_mode = false;

static inline void or_int_value(volatile unsigned short *a, volatile unsigned short *b, unsigned long r, unsigned long value)
{
    set_int_value(*a, *b, (r | value));
}
static inline void bc_int_value(volatile unsigned short *a, volatile unsigned short *b, unsigned long r, unsigned long value)
{
    set_int_value(*a, *b, (r & ~value));
}

static inline void nop_f(void)
{
    yield();
}

#define NOP     asm volatile("nop\n");

static inline int ep_index(int n, bool dir)
{
    return (n << 1) | dir;
}

static inline bool epidx_dir(int idx)
{
    return idx & 1;
}

static inline int epidx_n(int idx)
{
    return idx >> 1;
}

static inline void usb_select_endpoint(int idx)
{
    /* Select the endpoint */
    ISP1583_DFLOW_EPINDEX = idx;
    /* The delay time from the Write Endpoint Index register to the Read Data Port register must be at least 190 ns.
    * The delay time from the Write Endpoint Index register to the Write Data Port register must be at least 100 ns.
    */
    NOP;
}

static inline void usb_select_setup_endpoint(void)
{
    /* Select the endpoint */
    ISP1583_DFLOW_EPINDEX = DFLOW_EPINDEX_EP0SETUP;
    /* The delay time from the Write Endpoint Index register to the Read Data Port register must be at least 190 ns.
    * The delay time from the Write Endpoint Index register to the Write Data Port register must be at least 100 ns.
    */
    NOP;
}

static void usb_setup_endpoint(int idx, int max_pkt_size, int type)
{
    if(epidx_n(idx)!=0)
    {
        usb_select_endpoint(idx);
        ISP1583_DFLOW_MAXPKSZ = max_pkt_size & 0x7FF;
        ISP1583_DFLOW_EPTYPE = (DFLOW_EPTYPE_NOEMPKT | DFLOW_EPTYPE_DBLBUF | (type & 0x3));
        
        /* clear buffer ... */
        ISP1583_DFLOW_CTRLFUN |= DFLOW_CTRLFUN_CLBUF;
        /* ... twice because of double buffering */
        usb_select_endpoint(idx);
        ISP1583_DFLOW_CTRLFUN |= DFLOW_CTRLFUN_CLBUF;
    }
    
    struct usb_endpoint *ep;
    ep = &(endpoints[epidx_n(idx)]);
    ep->halt[epidx_dir(idx)] = 0;
    ep->enabled[epidx_dir(idx)] = 0;
    ep->out_in_progress = 0;
    ep->in_min_len = -1;
    ep->in_ack = 0;
    ep->type = type;
    ep->max_pkt_size[epidx_dir(idx)] = max_pkt_size;
}

static void usb_enable_endpoint(int idx)
{
    if(epidx_n(idx)!=0)
    {
        usb_select_endpoint(idx);
        /* Enable interrupt */
        or_int_value(&ISP1583_INIT_INTEN_A, &ISP1583_INIT_INTEN_B, ISP1583_INIT_INTEN_READ, 1 << (10 + idx));
        /* Enable endpoint */
        ISP1583_DFLOW_EPTYPE |= DFLOW_EPTYPE_ENABLE;
    }
    
    endpoints[epidx_n(idx)].enabled[epidx_dir(idx)] = 1;
}
/*
static void usb_disable_endpoint(int idx, bool set_struct)
{
    usb_select_endpoint(idx);
    ISP1583_DFLOW_EPTYPE &= ~DFLOW_EPTYPE_ENABLE;
    bc_int_value(&ISP1583_INIT_INTEN_A, &ISP1583_INIT_INTEN_B, ISP1583_INIT_INTEN_READ, 1 << (10 + idx));
    
    if(set_struct)
        endpoints[epidx_n(idx)].enabled[epidx_dir(idx)] = 0;
}
*/
static int usb_get_packet(unsigned char *buf, int max_len)
{
    int len, i;
    len = ISP1583_DFLOW_BUFLEN;

    if (max_len < 0 || max_len > len)
        max_len = len;

    i = 0;
    while (i < len)
    {
        unsigned short d = ISP1583_DFLOW_DATA;
        if (i < max_len)
            buf[i] = d & 0xff;
        i++;
        if (i < max_len)
            buf[i] = (d >> 8) & 0xff;
        i++;
    }
    return max_len;
}

static int usb_receive(int n)
{
    logf("usb_receive(%d)", n);
    int len;

    if (endpoints[n].halt[DIR_RX]
        || !endpoints[n].enabled[DIR_RX]
        || endpoints[n].in_min_len < 0
        || !endpoints[n].in_ack)
        return -1;

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
    logf("receive_end");
    return 0;
}

static bool usb_out_buffer_full(int ep)
{
    usb_select_endpoint(ep_index(ep, DIR_TX));
    if (ISP1583_DFLOW_EPTYPE & 4)   /* Check if type=bulk and double buffering is set */
        return (ISP1583_DFLOW_BUFSTAT & 3) == 3; /* Return true if both buffers are filled */
    else
        return (ISP1583_DFLOW_BUFSTAT & 3) != 0; /* Return true if one of the buffers are filled */
}

static int usb_send(int n)
{
    logf("usb_send(%d)", n);
    int max_pkt_size, len;
    int i;
    unsigned char *p;

    if (endpoints[n].halt[DIR_TX]
        || !endpoints[n].enabled[DIR_TX]
        || !endpoints[n].out_in_progress)
        {
        logf("NOT SEND TO EP!");
        return -1;
        }

    if (endpoints[n].out_ptr < 0)
    {
        endpoints[n].out_in_progress = 0;
        if (endpoints[n].out_done)
            (*(endpoints[n].out_done))(n, endpoints[n].out_buf,
                                       endpoints[n].out_len);
        logf("ALREADY SENT TO EP!");
        return -1;
    }
    
    if (usb_out_buffer_full(n))
    {
    logf("BUFFER FULL!");
        return -1;
        }
    
    usb_select_endpoint(ep_index(n, DIR_TX));
    max_pkt_size = endpoints[n].max_pkt_size[DIR_TX];
    len = endpoints[n].out_len - endpoints[n].out_ptr;
    if (len > max_pkt_size)
        len = max_pkt_size;
    
    if(len < max_pkt_size)
        ISP1583_DFLOW_BUFLEN = len;
    
    p = endpoints[n].out_buf + endpoints[n].out_ptr;
    i = 0;
    while (len - i >= 2) {
        ISP1583_DFLOW_DATA = p[i] | (p[i + 1] << 8);
        i += 2;
    }
    if (i < len)
        ISP1583_DFLOW_DATA = p[i];

    endpoints[n].out_ptr += len;

/*
    if (endpoints[n].out_ptr == endpoints[n].out_len
        && len < max_pkt_size)
*/
    if (endpoints[n].out_ptr == endpoints[n].out_len)
        endpoints[n].out_ptr = -1;
    
    logf("send_end");
    return 0;
}

static void usb_stall_endpoint(int idx)
{
    usb_select_endpoint(idx);
    ISP1583_DFLOW_CTRLFUN |= DFLOW_CTRLFUN_STALL;
    endpoints[epidx_n(idx)].halt[epidx_dir(idx)] = 1;
}

static void usb_unstall_endpoint(int idx)
{
    usb_select_endpoint(idx);
    ISP1583_DFLOW_CTRLFUN &= ~DFLOW_CTRLFUN_STALL;
    ISP1583_DFLOW_EPTYPE &= ~DFLOW_EPTYPE_ENABLE;
    ISP1583_DFLOW_EPTYPE |= DFLOW_EPTYPE_ENABLE;
    ISP1583_DFLOW_CTRLFUN |= DFLOW_CTRLFUN_CLBUF;
    if (epidx_dir(idx) == DIR_TX)
        endpoints[epidx_n(idx)].out_in_progress = 0;
    else
    {
        endpoints[epidx_n(idx)].in_min_len = -1;
        endpoints[epidx_n(idx)].in_ack = 0;
    }
    endpoints[epidx_n(idx)].halt[epidx_dir(idx)] = 0;
}

static void usb_status_ack(int ep, int dir)
{
    logf("usb_status_ack(%d)", dir);
    usb_select_endpoint(ep_index(ep, dir));
    ISP1583_DFLOW_CTRLFUN |= DFLOW_CTRLFUN_STATUS;
}

static void usb_data_stage_enable(int ep, int dir)
{
    logf("usb_data_stage_enable(%d)", dir);
    usb_select_endpoint(ep_index(ep, dir));
    ISP1583_DFLOW_CTRLFUN |= DFLOW_CTRLFUN_DSEN;
}

static void usb_handle_setup_rx(void)
{
    int len;
    usb_select_setup_endpoint();
    len = usb_get_packet(setup_pkt_buf, 8);

    if (len == 8)
        usb_core_control_request((struct usb_ctrlrequest*)setup_pkt_buf);
    else
    {
        usb_drv_stall(0, true, false);
        usb_drv_stall(0, true, true);
        logf("usb_handle_setup_rx() failed");
        return;
    }
    
    logf("usb_handle_setup_rx(): %02x %02x %02x %02x %02x %02x %02x %02x", setup_pkt_buf[0], setup_pkt_buf[1], setup_pkt_buf[2], setup_pkt_buf[3], setup_pkt_buf[4], setup_pkt_buf[5], setup_pkt_buf[6], setup_pkt_buf[7]);
}

static void usb_handle_data_int(int ep, int dir)
{
    int len;
    if (dir == DIR_TX)
        len = usb_send(ep);
    else
    {
        len = usb_receive(ep);
        endpoints[ep].in_ack = 1;
    }
    logf("usb_handle_data_int(%d, %d) finished", ep, dir);
}

bool usb_drv_powered(void)
{
#if 0
    return (ISP1583_INIT_OTG & INIT_OTG_BSESS_VALID) ? true : false;
#else
    return (ISP1583_INIT_MODE & INIT_MODE_VBUSSTAT) ? true : false;
#endif
}

static void setup_endpoints(void)
{
    usb_setup_endpoint(ep_index(0, DIR_RX), 64, 0);
    usb_setup_endpoint(ep_index(0, DIR_TX), 64, 0);
    
    int i;
    for(i = 1; i < NUM_ENDPOINTS-1; i++)
    {
        usb_setup_endpoint(ep_index(i, DIR_RX), (high_speed_mode ? 512 : 64), 2); /* 2 = TYPE_BULK */
        usb_setup_endpoint(ep_index(i, DIR_TX), (high_speed_mode ? 512 : 64), 2);
    }
    
    usb_enable_endpoint(ep_index(0, DIR_RX));
    usb_enable_endpoint(ep_index(0, DIR_TX));
    
    for (i = 1; i < NUM_ENDPOINTS-1; i++)
    {
        usb_enable_endpoint(ep_index(i, DIR_RX));
        usb_enable_endpoint(ep_index(i, DIR_TX));
    }
    
    ZVM_SPECIFIC;
}

void usb_helper(void)
{
    if(ISP1583_GEN_INT_READ & ISP1583_INIT_INTEN_READ)
    {
    #ifdef DEBUG
        logf("Helper detected interrupt... [%d]", current_tick);
    #endif
        usb_drv_int();
    }
    return;
}

void usb_drv_init(void)
{
    /* Disable interrupt at CPU level */
    DIS_INT_CPU_TARGET;

    /* Unlock the device's registers */
    ISP1583_GEN_UNLCKDEV = ISP1583_UNLOCK_CODE;

    /* Soft reset the device */
    ISP1583_INIT_MODE = INIT_MODE_SFRESET;
    sleep(10);
    /* Enable CLKAON & GLINTENA */
    ISP1583_INIT_MODE = STANDARD_INIT_MODE;
    
    /* Disable all OTG functions */
    ISP1583_INIT_OTG = 0;

#if 0
    #ifdef USE_HIGH_SPEED
    /* Force device to high speed */
    ISP1583_GEN_TSTMOD = GEN_TSTMOD_FORCEHS;
    high_speed_mode = true;
    #endif
#endif

    #ifdef DEBUG
    logf("BUS_CONF/DA0:%d MODE0/DA1: %d MODE1: %d", (bool)(ISP1583_INIT_MODE & INIT_MODE_TEST0), (bool)(ISP1583_INIT_MODE & INIT_MODE_TEST1), (bool)(ISP1583_INIT_MODE & INIT_MODE_TEST2));
    logf("Chip ID: 0x%x", ISP1583_GEN_CHIPID);
    //logf("INV0: 0x% IRQEDGE: 0x%x IRQPORT: 0x%x", IO_GIO_INV0, IO_GIO_IRQEDGE, IO_GIO_IRQPORT);
    #endif

    /*Set interrupt generation to target-specific mode +
    * Set the control pipe to ACK only interrupt +
    * Set the IN pipe to ACK only interrupt +
    * Set OUT pipe to ACK and NYET interrupt
    */
    
    ISP1583_INIT_INTCONF = 0x54 | INT_CONF_TARGET;
    /* Clear all interrupts */
    set_int_value(ISP1583_GEN_INT_A, ISP1583_GEN_INT_B, 0xFFFFFFFF);
    /* Enable USB interrupts */
    set_int_value(ISP1583_INIT_INTEN_A, ISP1583_INIT_INTEN_B, STANDARD_INTEN);
    
    ZVM_SPECIFIC;
    
    /* Enable interrupt at CPU level */
    EN_INT_CPU_TARGET;
    
    setup_endpoints();
    
    /* Clear device address and disable it */
    ISP1583_INIT_ADDRESS = 0;
    
    /* Turn SoftConnect on */
    ISP1583_INIT_MODE |= INIT_MODE_SOFTCT;
    
    ZVM_SPECIFIC;
    
    tick_add_task(usb_helper);
    
    logf("usb_init_device() finished");
}

int usb_drv_port_speed(void)
{
    return (int)high_speed_mode;
}
 
void usb_drv_exit(void)
{
    logf("usb_drv_exit()");

    /* Disable device */
    ISP1583_INIT_MODE &= ~INIT_MODE_SOFTCT;
    ISP1583_INIT_ADDRESS = 0;
    
    /* Disable interrupts */
    set_int_value(ISP1583_INIT_INTEN_A, ISP1583_INIT_INTEN_B, 0);
    /* and the CPU's one... */
    DIS_INT_CPU_TARGET;


    /* Send usb controller to suspend mode */
    ISP1583_INIT_MODE = INIT_MODE_GOSUSP;
    ISP1583_INIT_MODE = 0;
    
    tick_remove_task(usb_helper);
    
    ZVM_SPECIFIC;
}

void usb_drv_stall(int endpoint, bool stall, bool in)
{
    logf("%sstall EP%d %s", (stall ? "" : "un"), endpoint, (in ? "RX" : "TX" ));
    if (stall)
        usb_stall_endpoint(ep_index(endpoint, (int)in));
    else
        usb_unstall_endpoint(ep_index(endpoint, (int)in));
}

bool usb_drv_stalled(int endpoint, bool in)
{
    return (endpoints[endpoint].halt[(int)in] == 1);
}

static void out_callback(int ep, unsigned char *buf, int len)
{
    (void)buf;
    logf("out_callback(%d, 0x%x, %d)", ep, &buf, len);
    usb_status_ack(ep, DIR_RX);
    usb_core_transfer_complete(ep, true, 0, len); /* 0=>status succeeded, haven't worked out status failed yet... */
}

static void in_callback(int ep, unsigned char *buf, int len)
{
    (void)buf;
    logf("in_callback(%d, 0x%x, %d)", ep, &buf, len);
    usb_status_ack(ep, DIR_TX);
    usb_core_transfer_complete(ep, false, 0, len);
}

int usb_drv_recv(int ep, void* ptr, int length)
{
    logf("usb_drv_recv(%d, 0x%x, %d)", ep, &ptr, length);
    if(ep == 0 && length == 0 && ptr == NULL)
    {
        usb_status_ack(ep, DIR_TX);
        return 0;
    }
    endpoints[ep].in_done = in_callback;
    endpoints[ep].in_buf = ptr;
    endpoints[ep].in_max_len = length;
    endpoints[ep].in_min_len = length;
    endpoints[ep].in_ptr = 0;
    if(ep == 0)
    {
        usb_data_stage_enable(ep, DIR_RX);
        return usb_receive(ep);
    }
    else
        return usb_receive(ep);
}

int usb_drv_send_nonblocking(int ep, void* ptr, int length)
{
    /* First implement DMA... */
    return usb_drv_send(ep, ptr, length);
}

static void usb_drv_wait(int ep, bool send)
{
    logf("usb_drv_wait(%d, %d)", ep, send);
    if(send)
    {
        while (endpoints[ep].out_in_progress)
            nop_f();
    }
    else
    {
        while (endpoints[ep].in_ack)
            nop_f();
    }
}

int usb_drv_send(int ep, void* ptr, int length)
{
    logf("usb_drv_send_nb(%d, 0x%x, %d)", ep, &ptr, length);
    if(ep == 0 && length == 0 && ptr == NULL)
    {
        usb_status_ack(ep, DIR_RX);
        return 0;
    }
    if(endpoints[ep].out_in_progress == 1)
        return -1;
    endpoints[ep].out_done = out_callback;
    endpoints[ep].out_buf = ptr;
    endpoints[ep].out_len = length;
    endpoints[ep].out_ptr = 0;
    endpoints[ep].out_in_progress = 1;
    if(ep == 0)
    {
        int rc = usb_send(ep);
        usb_data_stage_enable(ep, DIR_TX);
        usb_drv_wait(ep, DIR_TX);
        return rc;
    }
    else
        return usb_send(ep);
}

void usb_drv_reset_endpoint(int ep, bool send)
{
    logf("reset endpoint(%d, %d)", ep, send);
    usb_setup_endpoint(ep_index(ep, (int)send), endpoints[ep].max_pkt_size[(int)send], endpoints[ep].type);
    usb_enable_endpoint(ep_index(ep, (int)send));
}

void usb_drv_cancel_all_transfers(void)
{
    logf("usb_drv_cancel_all_tranfers()");
    int i;

    for(i=0;i<NUM_ENDPOINTS-1;i++)
        endpoints[i].halt[0] = endpoints[i].halt[1] = 1;
}

int usb_drv_request_endpoint(int dir)
{
    int i, bit;

    bit=(dir & USB_DIR_IN)? 2:1;

    for (i=1; i < NUM_ENDPOINTS; i++) {
        if((endpoints[i].allocation & bit)!=0)
            continue;
        endpoints[i].allocation |= bit;
        return i | dir;
    }

    return -1;
}

void usb_drv_release_endpoint(int ep)
{
    int mask = (ep & USB_DIR_IN)? ~2:~1;
    endpoints[ep & 0x7f].allocation &= mask;
}



static void bus_reset(void)
{
    /* Enable CLKAON & GLINTENA */
    ISP1583_INIT_MODE = STANDARD_INIT_MODE;
    /* Enable USB interrupts */
    ISP1583_INIT_INTCONF = 0x54 | INT_CONF_TARGET;
    set_int_value(ISP1583_INIT_INTEN_A, ISP1583_INIT_INTEN_B, STANDARD_INTEN);
    
    /* Disable all OTG functions */
    ISP1583_INIT_OTG = 0;
    
    /* Clear device address and enable it */
    ISP1583_INIT_ADDRESS = INIT_ADDRESS_DEVEN;
    
    ZVM_SPECIFIC;

    /* Reset endpoints to default */
    setup_endpoints();

    logf("bus reset->done");
}

/* Method for handling interrupts, must be called from usb-<target>.c */
void IRAM_ATTR usb_drv_int(void)
{
    unsigned long ints;
    ints = ISP1583_GEN_INT_READ & ISP1583_INIT_INTEN_READ;
    
    if(!ints)
        return;
        
    /* Unlock the device's registers */
    ISP1583_GEN_UNLCKDEV = ISP1583_UNLOCK_CODE;
    
    #if 0
    logf(" handling int [0x%x & 0x%x = 0x%x]", ISP1583_GEN_INT_READ, ISP1583_INIT_INTEN_READ, ints);
    #endif

    if(ints & INT_IEBRST) /* Bus reset */
    {
        logf("BRESET");
        high_speed_mode = false;
        bus_reset();
        usb_core_bus_reset();
        /* Mask bus reset interrupt */
        set_int_value(ISP1583_GEN_INT_A, ISP1583_GEN_INT_B, INT_IEBRST);
        return;
    }
    if(ints & INT_IEP0SETUP) /* EP0SETUP interrupt */
    {
        logf("EP0SETUP");
        usb_handle_setup_rx();
    }
    if(ints & INT_IEHS_STA) /* change from full-speed to high-speed mode -> endpoints need to get reconfigured!! */
    {
        logf("HS_STA");
        high_speed_mode = true;
        setup_endpoints();
    }
    if(ints & INT_EP_MASK) /* Endpoints interrupt */
    {
        unsigned long ep_event;
        unsigned short i = 10;
        ep_event = ints & INT_EP_MASK;
        while(ep_event)
        {
            if(i>25)
                break;
            
            if(ep_event & (1 << i))
            {
                logf("EP%d %s interrupt", (i - 10) / 2, i % 2 ? "RX" : "TX");
                usb_handle_data_int((i - 10) / 2, i % 2);
                ep_event &= ~(1 << i);
            }
            i++;
        }
    }
    if(ints & INT_IERESM && !(ints & INT_IESUSP)) /* Resume status: status change from suspend to resume (active) */
    {
        logf("RESM");
    }
    if(ints & INT_IESUSP && !(ints & INT_IERESM)) /* Suspend status: status change from active to suspend */
    {
        logf("SUSP");
    }
    if(ints & INT_IEDMA) /* change in the DMA Interrupt Reason register */
    {
        logf("DMA");
    }
    if(ints & INT_IEVBUS) /* transition from LOW to HIGH on VBUS */
    {
        logf("VBUS");
    }
    /* Mask all (enabled) interrupts */
    set_int_value(ISP1583_GEN_INT_A, ISP1583_GEN_INT_B, ints);
    
    ZVM_SPECIFIC;
}

void usb_drv_set_address(int address)
{
    logf("usb_drv_set_address(0x%x)", address);
    ISP1583_INIT_ADDRESS = (address & 0x7F) | INIT_ADDRESS_DEVEN;
    
    ZVM_SPECIFIC;
    
    usb_status_ack(0, DIR_TX);
}

int dbg_usb_num_items(void)
{
    return 2+NUM_ENDPOINTS*2;
}

char* dbg_usb_item(int selected_item, void *data, char *buffer, size_t buffer_len)
{
    if(selected_item < 2)
    {
        switch(selected_item)
        {
            case 0:
                snprintf(buffer, buffer_len, "USB connected: %s", (usb_drv_connected() ? "Yes" : "No"));
                return buffer;
            case 1:
                snprintf(buffer, buffer_len, "HS mode: %s", (high_speed_mode ? "Yes" : "No"));
                return buffer;
        }
    }
    else
    {
        int n = ep_index((selected_item - 2) / 2, (selected_item - 2) % 2);
        if(endpoints[n].enabled == false)
            snprintf(buffer, buffer_len, "EP%d[%s]: DISABLED", epidx_n(n), (epidx_dir(n) ? "TX" : "RX"));
        else
        {
            if(epidx_dir(n))
            {
                if(endpoints[n].out_in_progress)
                    snprintf(buffer, buffer_len, "EP%d[TX]: TRANSFERRING DATA -> %d bytes/%d bytes", epidx_n(n), (endpoints[n].out_len - endpoints[n].out_ptr), endpoints[n].out_len);
                else
                    snprintf(buffer, buffer_len, "EP%d[TX]: STANDBY", epidx_n(n));
            }
            else
            {
                if(endpoints[n].in_buf && !endpoints[n].in_ack)
                    snprintf(buffer, buffer_len, "EP%d[RX]: RECEIVING DATA -> %d bytes/%d bytes", epidx_n(n), endpoints[n].in_ptr, endpoints[n].in_max_len);
                else
                    snprintf(buffer, buffer_len, "EP%d[RX]: STANDBY", epidx_n(n));
            }
        }
        return buffer;
    }
    return NULL;
    (void)data;
}

void usb_drv_set_test_mode(int mode)
{
    logf("usb_drv_set_test_mode(%d)", mode);
    switch(mode){
        case 0:
            ISP1583_GEN_TSTMOD = 0;
            /* Power cycle... */
            break;
        case 1:
            ISP1583_GEN_TSTMOD = GEN_TSTMOD_JSTATE;
            break;
        case 2:
            ISP1583_GEN_TSTMOD = GEN_TSTMOD_KSTATE;
            break;
        case 3:
            ISP1583_GEN_TSTMOD = GEN_TSTMOD_SE0_NAK;
            break;
        case 4:
            //REG_PORTSC1 |= PORTSCX_PTC_PACKET;
            break;
        case 5:
            //REG_PORTSC1 |= PORTSCX_PTC_FORCE_EN;
            break;
    }
}
