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
#include "string.h"
#include "system.h"
#include "usb_core.h"
#include "usb_drv.h"
#include "usb_class_driver.h"
/*#define LOGF_ENABLE*/
#include "logf.h"
#include "storage.h"
#include "disk.h"
#include "fs_defines.h"
/* Needed to get at the audio buffer */
#include "audio.h"
#include "usb_storage.h"
#if CONFIG_RTC
#include "timefuncs.h"
#endif
#include "core_alloc.h"
#include "panic.h"

#ifdef USB_USE_RAMDISK
#define RAMDISK_SIZE 2048
#endif

/* These defaults allow the operation */
#ifndef USBSTOR_READ_SECTORS_FILTER
#define USBSTOR_READ_SECTORS_FILTER() ({ 0; })
#endif

#ifndef USBSTOR_WRITE_SECTORS_FILTER
#define USBSTOR_WRITE_SECTORS_FILTER() ({ 0; })
#endif

/* the ARC driver currently supports up to 64k USB transfers. This is
 * enough for efficient mass storage support, as commonly host OSes
 * don't do larger SCSI transfers anyway, so larger USB transfers
 * wouldn't buy us anything.
 * Due to being the double-buffering system used, using a smaller write buffer
 * ends up being more efficient. Measurements have shown that 24k to 28k is
 * optimal, except for sd devices that apparently don't gain anything from
 * double-buffering
 */
#ifdef USB_READ_BUFFER_SIZE
#define READ_BUFFER_SIZE USB_READ_BUFFER_SIZE
#else
#if CONFIG_USBOTG == USBOTG_AS3525
/* We'd need to implement multidescriptor dma for sizes >65535 */
#define READ_BUFFER_SIZE (1024*63)
#else
#define READ_BUFFER_SIZE (1024*64)
#endif /* CONFIG_CPU == AS3525 */
#endif /* USB_READ_BUFFER_SIZE */

/* We don't use sizeof() here, because we *need* a multiple of 32 */
#define MAX_CBW_SIZE 512

#ifdef USB_WRITE_BUFFER_SIZE
#define WRITE_BUFFER_SIZE USB_WRITE_BUFFER_SIZE
#else
#if (CONFIG_STORAGE & STORAGE_SD)
#if CONFIG_USBOTG == USBOTG_AS3525
/* We'd need to implement multidescriptor dma for sizes >65535 */
#define WRITE_BUFFER_SIZE (1024*63)
#else
#define WRITE_BUFFER_SIZE (1024*64)
#endif /* CONFIG_CPU == AS3525 */
#else
#define WRITE_BUFFER_SIZE (1024*24)
#endif /* (CONFIG_STORAGE & STORAGE_SD) */
#endif /* USB_WRITE_BUFFER_SIZE */

#define ALLOCATE_BUFFER_SIZE (2*MAX(READ_BUFFER_SIZE,WRITE_BUFFER_SIZE))

/* bulk-only class specific requests */
#define USB_BULK_RESET_REQUEST   0xff
#define USB_BULK_GET_MAX_LUN     0xfe

#define DIRECT_ACCESS_DEVICE     0x00    /* disks */
#define DEVICE_REMOVABLE         0x80

#define CBW_SIGNATURE            0x43425355
#define CSW_SIGNATURE            0x53425355

#define SCSI_TEST_UNIT_READY      0x00
#define SCSI_INQUIRY              0x12
#define SCSI_MODE_SENSE_6         0x1a
#define SCSI_MODE_SENSE_10        0x5a
#define SCSI_REQUEST_SENSE        0x03
#define SCSI_ALLOW_MEDIUM_REMOVAL 0x1e
#define SCSI_READ_CAPACITY_10     0x25
#define SCSI_READ_CAPACITY_16     0x9e
#define USB_UFI_READ_FORMAT_CAPACITY 0x23
#define SCSI_READ_10              0x28
#define SCSI_WRITE_10             0x2a
#define SCSI_START_STOP_UNIT      0x1b
#define SCSI_REPORT_LUNS          0xa0
#define SCSI_WRITE_BUFFER         0x3b

#define SCSI_READ_16              0x88
#define SCSI_WRITE_16             0x8a

#define UMS_STATUS_GOOD            0x00
#define UMS_STATUS_FAIL            0x01

#define SENSE_NOT_READY             0x02
#define SENSE_MEDIUM_ERROR          0x03
#define SENSE_ILLEGAL_REQUEST       0x05
#define SENSE_UNIT_ATTENTION        0x06

#define ASC_MEDIUM_NOT_PRESENT      0x3a
#define ASC_INVALID_FIELD_IN_CBD    0x24
#define ASC_LBA_OUT_OF_RANGE        0x21
#define ASC_WRITE_ERROR             0x0C
#define ASC_READ_ERROR              0x11
#define ASC_NOT_READY               0x04
#define ASC_INVALID_COMMAND         0x20

#define ASCQ_BECOMING_READY         0x01

#define SCSI_FORMAT_CAPACITY_FORMATTED_MEDIA 0x02000000

/* storage interface */

#define USB_SC_SCSI      0x06            /* Transparent */
#define USB_PROT_BULK    0x50            /* bulk only */

static struct usb_interface_descriptor __attribute__((aligned(2)))
                                       interface_descriptor =
{
    .bLength            = sizeof(struct usb_interface_descriptor),
    .bDescriptorType    = USB_DT_INTERFACE,
    .bInterfaceNumber   = 0,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 2,
    .bInterfaceClass    = USB_CLASS_MASS_STORAGE,
    .bInterfaceSubClass = USB_SC_SCSI,
    .bInterfaceProtocol = USB_PROT_BULK,
    .iInterface         = 0
};

static struct usb_endpoint_descriptor __attribute__((aligned(2)))
                                      endpoint_descriptor =
{
    .bLength          = sizeof(struct usb_endpoint_descriptor),
    .bDescriptorType  = USB_DT_ENDPOINT,
    .bEndpointAddress = 0,
    .bmAttributes     = USB_ENDPOINT_XFER_BULK,
    .wMaxPacketSize   = 0,
    .bInterval        = 0
};

struct inquiry_data {
    unsigned char DeviceType;
    unsigned char DeviceTypeModifier;
    unsigned char Versions;
    unsigned char Format;
    unsigned char AdditionalLength;
    unsigned char Reserved[2];
    unsigned char Capability;
    unsigned char VendorId[8];
    unsigned char ProductId[16];
    unsigned char ProductRevisionLevel[4];
} __attribute__ ((packed));

struct report_lun_data {
    unsigned int lun_list_length;
    unsigned int reserved1;
    unsigned char luns[NUM_DRIVES][8];
} __attribute__ ((packed));

struct sense_data {
    unsigned char ResponseCode;
    unsigned char Obsolete;
    unsigned char fei_sensekey;
    unsigned int Information;
    unsigned char AdditionalSenseLength;
    unsigned int  CommandSpecificInformation;
    unsigned char AdditionalSenseCode;
    unsigned char AdditionalSenseCodeQualifier;
    unsigned char FieldReplaceableUnitCode;
    unsigned char SKSV;
    unsigned short SenseKeySpecific;
} __attribute__ ((packed));

struct mode_sense_bdesc_longlba {
    unsigned char num_blocks[8];
    unsigned char reserved[4];
    unsigned char block_size[4];
} __attribute__ ((packed));

struct mode_sense_bdesc_shortlba {
    unsigned char density_code;
    unsigned char num_blocks[3];
    unsigned char reserved;
    unsigned char block_size[3];
} __attribute__ ((packed));

struct mode_sense_data_10 {
    unsigned short mode_data_length;
    unsigned char medium_type;
    unsigned char device_specific;
    unsigned char longlba;
    unsigned char reserved;
    unsigned short block_descriptor_length;
    struct mode_sense_bdesc_longlba block_descriptor;
} __attribute__ ((packed));

struct mode_sense_data_6 {
    unsigned char mode_data_length;
    unsigned char medium_type;
    unsigned char device_specific;
    unsigned char block_descriptor_length;
    struct mode_sense_bdesc_shortlba block_descriptor;
} __attribute__ ((packed));

struct command_block_wrapper {
    unsigned int signature;
    unsigned int tag;
    unsigned int data_transfer_length;
    unsigned char flags;
    unsigned char lun;
    unsigned char command_length;
    unsigned char command_block[16];
} __attribute__ ((packed));

struct command_status_wrapper {
    unsigned int signature;
    unsigned int tag;
    unsigned int data_residue;
    unsigned char status;
} __attribute__ ((packed));

struct capacity_10 {
    unsigned char block_count[4];
    unsigned char block_size[4];
} __attribute__ ((packed));

struct capacity_16 {
    unsigned char block_count[8];
    unsigned char block_size[4];
    unsigned char flags[2];
    unsigned char lowest_aligned_lba[2];
} __attribute__ ((packed));

struct format_capacity {
    unsigned int following_length;
    unsigned int block_count;
    unsigned int block_size;
} __attribute__ ((packed));


static union {
    unsigned char* transfer_buffer;
    struct inquiry_data* inquiry;
    struct capacity_10* capacity_data_10;
    struct capacity_16* capacity_data_16;
    struct format_capacity* format_capacity_data;
    struct sense_data *sense_data;
    struct mode_sense_data_6 *ms_data_6;
    struct mode_sense_data_10 *ms_data_10;
    struct report_lun_data *lun_data;
    struct command_status_wrapper* csw;
    char *max_lun;
} tb;

static char *cbw_buffer;

static struct {
    sector_t sector;
    unsigned int count;
    unsigned int cur_cmd;
    unsigned int block_size;
    unsigned int tag;
    unsigned int lun;
    unsigned char *data[2];
    unsigned char data_select;
    unsigned int last_result;
} cur_cmd;

static struct {
    unsigned char sense_key;
    unsigned char information;
    unsigned char asc;
    unsigned char ascq;
} cur_sense_data;

static void handle_scsi(struct command_block_wrapper* cbw);
static void send_csw(int status);
static void send_command_result(void *data,int size);
static void send_command_failed_result(void);
static void send_block_data(void *data,int size);
static void receive_block_data(void *data,int size);
#if CONFIG_RTC
static void receive_time(void);
#endif
static void fill_inquiry(IF_MD_NONVOID(int lun));
static void send_and_read_next(void);
static bool ejected[NUM_DRIVES];
static bool locked[NUM_DRIVES];

static int usb_interface;
static int ep_in, ep_out;

#if defined(HAVE_MULTIDRIVE)
static bool skip_first = 0;
#endif

#ifdef USB_USE_RAMDISK
static unsigned char* ramdisk_buffer;
#endif

static enum {
    WAITING_FOR_COMMAND,
    SENDING_BLOCKS,
    SENDING_RESULT,
    SENDING_FAILED_RESULT,
    RECEIVING_BLOCKS,
#if CONFIG_RTC
    RECEIVING_TIME,
#endif
    WAITING_FOR_CSW_COMPLETION_OR_COMMAND,
    WAITING_FOR_CSW_COMPLETION
} state = WAITING_FOR_COMMAND;

#if CONFIG_RTC
static void yearday_to_daymonth(int yd, int y, int *d, int *m)
{
    static char t[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    bool leap = (y%4 == 0 && y%100 != 0) || y%400 == 0;
    t[1] = leap ? 29 : 28;

    int i;
    for (i = 0; i < 12 && yd >= t[i]; i++)
        yd -= t[i];

    *d = yd+1;
    *m = i;
}
#endif /* CONFIG_RTC */

static bool check_disk_present(IF_MD_NONVOID(int volume))
{
#ifdef USB_USE_RAMDISK
    return true;
#else
    return disk_present(IF_MD(volume));
#endif
}

#ifdef HAVE_HOTSWAP
void usb_storage_notify_hotswap(int volume,bool inserted)
{
    logf("notify %d",inserted);
    if(inserted && check_disk_present(IF_MD(volume))) {
        ejected[volume] = false;
    }
    else {
        ejected[volume] = true;
        /* If this happens while the device is locked, weird things may happen.
           At least try to keep our state consistent */
        locked[volume]=false;
    }
}
#endif

#ifdef HAVE_MULTIDRIVE
void usb_set_skip_first_drive(bool skip)
{
    skip_first = skip;
}
#endif

/* called by usb_core_init() */
void usb_storage_init(void)
{
    logf("usb_storage_init done");
}

int usb_storage_request_endpoints(struct usb_class_driver *drv)
{
    ep_in = usb_core_request_endpoint(USB_ENDPOINT_XFER_BULK, USB_DIR_IN, drv);

    if(ep_in<0)
        return -1;

    ep_out = usb_core_request_endpoint(USB_ENDPOINT_XFER_BULK, USB_DIR_OUT,
            drv);

    if(ep_out<0) {
        usb_core_release_endpoint(ep_in);
        return -1;
    }

    return 0;
}

int usb_storage_set_first_interface(int interface)
{
    usb_interface = interface;
    return interface + 1;
}

int usb_storage_get_config_descriptor(unsigned char *dest,int max_packet_size)
{
    unsigned char *orig_dest = dest;

    interface_descriptor.bInterfaceNumber = usb_interface;
    PACK_DATA(&dest, interface_descriptor);

    endpoint_descriptor.wMaxPacketSize = max_packet_size;

    endpoint_descriptor.bEndpointAddress = ep_in;
    PACK_DATA(&dest, endpoint_descriptor);

    endpoint_descriptor.bEndpointAddress = ep_out;
    PACK_DATA(&dest, endpoint_descriptor);

    return (dest - orig_dest);
}
#if (CONFIG_CPU == IMX31L || defined(CPU_TCC780X) || \
     CONFIG_CPU == S5L8702 || CONFIG_CPU == S5L8701 || CONFIG_CPU == AS3525v2 || \
     defined(BOOTLOADER) || CONFIG_CPU == DM320) && !defined(CPU_PP502x)
#define USB_STATIC_ALLOC
#else
static int usb_handle = 0;
#endif
void usb_storage_init_connection(void)
{
    logf("ums: set config");
    /* prime rx endpoint. We only need room for commands */
    state = WAITING_FOR_COMMAND;

#ifdef USB_STATIC_ALLOC
    static unsigned char _cbw_buffer[MAX_CBW_SIZE]
        USB_DEVBSS_ATTR __attribute__((aligned(32)));
    cbw_buffer = (void *)_cbw_buffer;

    static unsigned char _transfer_buffer[ALLOCATE_BUFFER_SIZE]
        USB_DEVBSS_ATTR __attribute__((aligned(32)));
    tb.transfer_buffer = (void *)_transfer_buffer;
#ifdef USB_USE_RAMDISK
    static unsigned char _ramdisk_buffer[RAMDISK_SIZE*SECTOR_SIZE];
    ramdisk_buffer = _ramdisk_buffer;
#endif
#else
    unsigned char * buffer;

    // Add 31 to handle worst-case misalignment
    usb_handle = core_alloc_ex(ALLOCATE_BUFFER_SIZE + MAX_CBW_SIZE + 31,
                               &buflib_ops_locked);
    if (usb_handle < 0)
        panicf("%s(): OOM", __func__);

    buffer = core_get_data(usb_handle);
#if defined(UNCACHED_ADDR) && CONFIG_CPU != AS3525
    cbw_buffer = (void *)UNCACHED_ADDR((unsigned int)(buffer+31) & 0xffffffe0);
#else
    cbw_buffer = (void *)((unsigned int)(buffer+31) & 0xffffffe0);
#endif
    tb.transfer_buffer = cbw_buffer + MAX_CBW_SIZE;
    commit_discard_dcache();
#ifdef USB_USE_RAMDISK
    ramdisk_buffer = tb.transfer_buffer + ALLOCATE_BUFFER_SIZE;
#endif
#endif
    usb_drv_recv_nonblocking(ep_out, cbw_buffer, MAX_CBW_SIZE);

    int i;
    for(i=0;i<storage_num_drives();i++) {
        locked[i] = false;
        ejected[i] = !check_disk_present(IF_MD(i));
        queue_broadcast(SYS_USB_LUN_LOCKED, (i<<16)+0);
    }
}

void usb_storage_disconnect(void)
{
#ifndef USB_STATIC_ALLOC
    usb_handle = core_free(usb_handle);
#endif
}

/* called by usb_core_transfer_complete() */
void usb_storage_transfer_complete(int ep,int dir,int status,int length)
{
    (void)ep;
    struct command_block_wrapper* cbw = (void*)cbw_buffer;
#if CONFIG_RTC
    struct tm tm;
#endif

    logf("transfer result for ep %d/%d %X %d", ep,dir,status, length);

    switch(state) {
        case RECEIVING_BLOCKS:
            if(dir==USB_DIR_IN) {
                logf("IN received in RECEIVING");
            }
            logf("scsi write %llu %d", cur_cmd.sector, cur_cmd.count);
            if(status==0) {
                if((unsigned int)length!=(cur_cmd.block_size* cur_cmd.count)
                  && (unsigned int)length!=WRITE_BUFFER_SIZE) {
                    logf("unexpected length :%d",length);
                    break;
                }

                sector_t next_sector = cur_cmd.sector +
                             (WRITE_BUFFER_SIZE/cur_cmd.block_size);
                unsigned int next_count = cur_cmd.count -
                             MIN(cur_cmd.count,WRITE_BUFFER_SIZE/cur_cmd.block_size);
                int next_select = !cur_cmd.data_select;

                if(next_count!=0) {
                    /* Ask the host to send more, to the other buffer */
                    receive_block_data(cur_cmd.data[next_select],
                                       MIN(WRITE_BUFFER_SIZE,next_count*cur_cmd.block_size));
                }

                /* Now write the data that just came in, while the host is
                   sending the next bit */
#ifdef USB_USE_RAMDISK
                memcpy(ramdisk_buffer + cur_cmd.sector*cur_cmd.block_size,
                        cur_cmd.data[cur_cmd.data_select],
                        MIN(WRITE_BUFFER_SIZE/cur_cmd.block_size, cur_cmd.count)*cur_cmd.block_size);
#else
                int result = USBSTOR_WRITE_SECTORS_FILTER();

                if (result == 0) {
                    result = storage_write_sectors(IF_MD(cur_cmd.lun,)
                        cur_cmd.sector,
                        MIN(WRITE_BUFFER_SIZE/cur_cmd.block_size, cur_cmd.count),
                        cur_cmd.data[cur_cmd.data_select]);
                }

                if(result != 0) {
                    send_csw(UMS_STATUS_FAIL);
                    cur_sense_data.sense_key=SENSE_MEDIUM_ERROR;
                    cur_sense_data.asc=ASC_WRITE_ERROR;
                    cur_sense_data.ascq=0;
                    break;
                }
#endif
                if(next_count==0) {
                    send_csw(UMS_STATUS_GOOD);
                }

                /* Switch buffers for the next one */
                cur_cmd.data_select=!cur_cmd.data_select;

                cur_cmd.sector = next_sector;
                cur_cmd.count = next_count;
            }
            else {
                logf("Transfer failed %X",status);
                send_csw(UMS_STATUS_FAIL);
                /* TODO fill in cur_sense_data */
                cur_sense_data.sense_key=0;
                cur_sense_data.information=0;
                cur_sense_data.asc=0;
                cur_sense_data.ascq=0;
            }
            break;
        case WAITING_FOR_CSW_COMPLETION_OR_COMMAND:
            if(dir==USB_DIR_IN) {
                /* This was the CSW */
                state = WAITING_FOR_COMMAND;
            }
            else {
                /* This was the command */
                state = WAITING_FOR_CSW_COMPLETION;
                /* We now have the CBW, but we won't execute it yet to avoid
                 * issues with the still-pending CSW */
            }
            break;
        case WAITING_FOR_COMMAND:
            if(dir==USB_DIR_IN) {
                logf("IN received in WAITING_FOR_COMMAND");
            }
            handle_scsi(cbw);
            break;
        case WAITING_FOR_CSW_COMPLETION:
            if(dir==USB_DIR_OUT) {
                logf("OUT received in WAITING_FOR_CSW_COMPLETION");
            }
            handle_scsi(cbw);
            break;
        case SENDING_RESULT:
            if(dir==USB_DIR_OUT) {
                logf("OUT received in SENDING");
            }
            if(status==0) {
                //logf("data sent, now send csw");
                send_csw(UMS_STATUS_GOOD);
            }
            else {
                logf("Transfer failed %X",status);
                send_csw(UMS_STATUS_FAIL);
                /* TODO fill in cur_sense_data */
                cur_sense_data.sense_key=0;
                cur_sense_data.information=0;
                cur_sense_data.asc=0;
                cur_sense_data.ascq=0;
            }
            break;
        case SENDING_FAILED_RESULT:
            if(dir==USB_DIR_OUT) {
                logf("OUT received in SENDING");
            }
            send_csw(UMS_STATUS_FAIL);
            break;
        case SENDING_BLOCKS:
            if(dir==USB_DIR_OUT) {
                logf("OUT received in SENDING");
            }
            if(status==0) {
                if(cur_cmd.count==0) {
                    //logf("data sent, now send csw");
                    if(cur_cmd.last_result!=0) {
                        /* The last read failed. */
                        send_csw(UMS_STATUS_FAIL);
                        cur_sense_data.sense_key=SENSE_MEDIUM_ERROR;
                        cur_sense_data.asc=ASC_READ_ERROR;
                        cur_sense_data.ascq=0;
                        return;
                    }
                    else
                        send_csw(UMS_STATUS_GOOD);
                }
                else {
                    send_and_read_next();
                }
            }
            else {
                logf("Transfer failed %X",status);
                send_csw(UMS_STATUS_FAIL);
                /* TODO fill in cur_sense_data */
                cur_sense_data.sense_key=0;
                cur_sense_data.information=0;
                cur_sense_data.asc=0;
                cur_sense_data.ascq=0;
            }
            break;
#if CONFIG_RTC
        case RECEIVING_TIME:
            tm.tm_year=(tb.transfer_buffer[0]<<8)+tb.transfer_buffer[1] - 1900;
            tm.tm_yday=(tb.transfer_buffer[2]<<8)+tb.transfer_buffer[3];
            tm.tm_hour=tb.transfer_buffer[5];
            tm.tm_min=tb.transfer_buffer[6];
            tm.tm_sec=tb.transfer_buffer[7];
            yearday_to_daymonth(tm.tm_yday,tm.tm_year + 1900,&tm.tm_mday,&tm.tm_mon);
            set_day_of_week(&tm);
            set_time(&tm);
            send_csw(UMS_STATUS_GOOD);
            break;
#endif /* CONFIG_RTC */
    }
}

/* called by usb_core_control_request() */
bool usb_storage_control_request(struct usb_ctrlrequest* req, void* reqdata, unsigned char* dest)
{
    bool handled = false;

    (void)dest;
    (void)reqdata;

    switch (req->bRequest) {
        case USB_BULK_GET_MAX_LUN: {
            *tb.max_lun = storage_num_drives() - 1;
#if defined(HAVE_MULTIDRIVE)
            if(skip_first) (*tb.max_lun) --;
#endif
            logf("ums: getmaxlun");
            usb_drv_control_response(USB_CONTROL_ACK, tb.max_lun, 1);
            handled = true;
            break;
        }

        case USB_BULK_RESET_REQUEST:
            logf("ums: bulk reset");
            state = WAITING_FOR_COMMAND;
            /* UMS BOT 3.1 says The device shall preserve the value of its bulk
               data toggle bits and endpoint STALL conditions despite
               the Bulk-Only Mass Storage Reset. */
#if 0
            usb_drv_reset_endpoint(ep_in, false);
            usb_drv_reset_endpoint(ep_out, true);
#endif
            usb_drv_control_response(USB_CONTROL_ACK, NULL, 0);
            handled = true;
            break;
    }

    return handled;
}

static void send_and_read_next(void)
{
    int result = USBSTOR_READ_SECTORS_FILTER();

    if(result != 0 && cur_cmd.last_result == 0)
        cur_cmd.last_result = result;

    send_block_data(cur_cmd.data[cur_cmd.data_select],
                    MIN(READ_BUFFER_SIZE,cur_cmd.count*cur_cmd.block_size));

    /* Switch buffers for the next one */
    cur_cmd.data_select=!cur_cmd.data_select;

    cur_cmd.sector+=(READ_BUFFER_SIZE/cur_cmd.block_size);
    cur_cmd.count-=MIN(cur_cmd.count,READ_BUFFER_SIZE/cur_cmd.block_size);

    if(cur_cmd.count!=0) {
        /* already read the next bit, so we can send it out immediately when the
         * current transfer completes.  */
#ifdef USB_USE_RAMDISK
        memcpy(cur_cmd.data[cur_cmd.data_select],
                ramdisk_buffer + cur_cmd.sector*cur_cmd.block_size,
                MIN(READ_BUFFER_SIZE/cur_cmd.block_size, cur_cmd.count)*cur_cmd.block_size);
#else
        result = storage_read_sectors(IF_MD(cur_cmd.lun,)
                cur_cmd.sector,
                MIN(READ_BUFFER_SIZE/cur_cmd.block_size, cur_cmd.count),
                cur_cmd.data[cur_cmd.data_select]);
        if(cur_cmd.last_result == 0)
            cur_cmd.last_result = result;

#endif
    }
}
/****************************************************************************/

static void handle_scsi(struct command_block_wrapper* cbw)
{
    struct storage_info info;
    unsigned int length = cbw->data_transfer_length;
    sector_t block_count;
    unsigned int block_size;
    bool lun_present=true;
    unsigned char lun = cbw->lun;

    if(letoh32(cbw->signature) != CBW_SIGNATURE) {
        logf("ums: bad cbw signature (%x)", cbw->signature);
        usb_drv_stall(ep_in, true,true);
        usb_drv_stall(ep_out, true,false);
        return;
    }
    /* Clear the signature to prevent possible bugs elsewhere
     * to trigger a second execution of the same command with
     * bogus data */
    cbw->signature=0;

#if defined(HAVE_MULTIDRIVE)
    if(skip_first) lun++;
#endif

    storage_get_info(lun,&info);
#ifdef USB_USE_RAMDISK
    block_size = SECTOR_SIZE;
    block_count = RAMDISK_SIZE;
#else
    block_size=info.sector_size;
    block_count=info.num_sectors;
#endif

#ifdef HAVE_HOTSWAP
    if(storage_removable(lun) && !storage_present(lun)) {
        ejected[lun] = true;
    }
#endif

    if(ejected[lun])
        lun_present = false;

    unsigned int block_size_mult = 1; /* Number of LOGICAL storage device blocks in each USB block */
#ifdef MAX_LOG_SECTOR_SIZE
    block_size_mult = disk_get_sector_multiplier(IF_MD(lun));
#endif

    uint32_t bsize = block_size*block_size_mult;
    sector_t bcount = block_count/block_size_mult;

    cur_cmd.tag = cbw->tag;
    cur_cmd.lun = lun;
    cur_cmd.cur_cmd = cbw->command_block[0];

    switch (cbw->command_block[0]) {
        case SCSI_TEST_UNIT_READY:
            logf("scsi test_unit_ready %d",lun);
            if(!usb_exclusive_storage()) {
                send_csw(UMS_STATUS_FAIL);
                cur_sense_data.sense_key=SENSE_NOT_READY;
                cur_sense_data.asc=ASC_MEDIUM_NOT_PRESENT;
                cur_sense_data.ascq=0;
                break;
            }
            if(lun_present) {
                send_csw(UMS_STATUS_GOOD);
            }
            else {
                send_csw(UMS_STATUS_FAIL);
                cur_sense_data.sense_key=SENSE_NOT_READY;
                cur_sense_data.asc=ASC_MEDIUM_NOT_PRESENT;
                cur_sense_data.ascq=0;
            }
            break;

        case SCSI_REPORT_LUNS: {
            logf("scsi report luns %d",lun);
            unsigned int allocation_length=0;
            int i;
            unsigned int response_length = 8+8*storage_num_drives();
            allocation_length|=(cbw->command_block[6]<<24);
            allocation_length|=(cbw->command_block[7]<<16);
            allocation_length|=(cbw->command_block[8]<<8);
            allocation_length|=(cbw->command_block[9]);
            memset(tb.lun_data,0,sizeof(struct report_lun_data));
            tb.lun_data->lun_list_length=htobe32(8*storage_num_drives());
            for(i=0;i<storage_num_drives();i++)
            {
#ifdef HAVE_HOTSWAP
                if(storage_removable(i))
                    tb.lun_data->luns[i][1]=1;
                else
#endif
                    tb.lun_data->luns[i][1]=0;
            }
            length = MIN(length, allocation_length);
            send_command_result(tb.lun_data,
                                MIN(response_length, length));
            break;
        }

        case SCSI_INQUIRY:
            logf("scsi inquiry %d",lun);
            fill_inquiry(IF_MD(lun));
            length = MIN(length, cbw->command_block[4]);
            send_command_result(tb.inquiry,
                                MIN(sizeof(struct inquiry_data), length));
            break;

        case SCSI_REQUEST_SENSE: {
            tb.sense_data->ResponseCode=0x70;/*current error*/
            tb.sense_data->Obsolete=0;
            tb.sense_data->fei_sensekey=cur_sense_data.sense_key&0x0f;
            tb.sense_data->Information=cur_sense_data.information;
            tb.sense_data->AdditionalSenseLength=10;
            tb.sense_data->CommandSpecificInformation=0;
            tb.sense_data->AdditionalSenseCode=cur_sense_data.asc;
            tb.sense_data->AdditionalSenseCodeQualifier=cur_sense_data.ascq;
            tb.sense_data->FieldReplaceableUnitCode=0;
            tb.sense_data->SKSV=0;
            tb.sense_data->SenseKeySpecific=0;
            logf("scsi request_sense %d",lun);
            send_command_result(tb.sense_data,
                                MIN(sizeof(struct sense_data), length));
            break;
        }

        case SCSI_MODE_SENSE_10: {
            if(!lun_present) {
                send_command_failed_result();
                cur_sense_data.sense_key=SENSE_NOT_READY;
                cur_sense_data.asc=ASC_MEDIUM_NOT_PRESENT;
                cur_sense_data.ascq=0;
                break;
            }
            /*unsigned char pc = (cbw->command_block[2] & 0xc0) >>6;*/
            unsigned char page_code = cbw->command_block[2] & 0x3f;
            logf("scsi mode_sense_10 %d %X",lun,page_code);
            switch(page_code) {
                case 0x3f:
                    tb.ms_data_10->mode_data_length =
                        htobe16(sizeof(struct mode_sense_data_10)-2);
                    tb.ms_data_10->medium_type = 0;
                    tb.ms_data_10->device_specific = 0;
                    tb.ms_data_10->reserved = 0;
                    tb.ms_data_10->longlba = 1;
                    tb.ms_data_10->block_descriptor_length =
                        htobe16(sizeof(struct mode_sense_bdesc_longlba));

                    memset(tb.ms_data_10->block_descriptor.reserved,0,4);

#ifdef STORAGE_64BIT_SECTOR
                    tb.ms_data_10->block_descriptor.num_blocks[2] =
                        (bcount & 0xff00000000ULL)>>40;
                    tb.ms_data_10->block_descriptor.num_blocks[3] =
                        (bcount & 0x00ff000000ULL)>>32;
#else
                    tb.ms_data_10->block_descriptor.num_blocks[2] = 0;
                    tb.ms_data_10->block_descriptor.num_blocks[3] = 0;
#endif
                    tb.ms_data_10->block_descriptor.num_blocks[4] =
                        (bcount & 0xff000000)>>24;
                    tb.ms_data_10->block_descriptor.num_blocks[5] =
                        (bcount & 0x00ff0000)>>16;
                    tb.ms_data_10->block_descriptor.num_blocks[6] =
                        (bcount & 0x0000ff00)>>8;
                    tb.ms_data_10->block_descriptor.num_blocks[7] =
                        (bcount & 0x000000ff);

                    tb.ms_data_10->block_descriptor.block_size[0] =
                        (bsize & 0xff000000)>>24;
                    tb.ms_data_10->block_descriptor.block_size[1] =
                        (bsize & 0x00ff0000)>>16;
                    tb.ms_data_10->block_descriptor.block_size[2] =
                        (bsize & 0x0000ff00)>>8;
                    tb.ms_data_10->block_descriptor.block_size[3] =
                        (bsize & 0x000000ff);
                    send_command_result(tb.ms_data_10,
                            MIN(sizeof(struct mode_sense_data_10), length));
                    break;
                default:
                    send_command_failed_result();
                    cur_sense_data.sense_key=SENSE_ILLEGAL_REQUEST;
                    cur_sense_data.asc=ASC_INVALID_FIELD_IN_CBD;
                    cur_sense_data.ascq=0;
                    break;
            }
            break;
        }

        case SCSI_MODE_SENSE_6: {
            if(!lun_present) {
                send_command_failed_result();
                cur_sense_data.sense_key=SENSE_NOT_READY;
                cur_sense_data.asc=ASC_MEDIUM_NOT_PRESENT;
                cur_sense_data.ascq=0;
                break;
            }
            /*unsigned char pc = (cbw->command_block[2] & 0xc0) >>6;*/
            unsigned char page_code = cbw->command_block[2] & 0x3f;
            logf("scsi mode_sense_6 %d %X",lun,page_code);
            switch(page_code) {
                case 0x3f:
                    /* All supported pages. */
                    tb.ms_data_6->mode_data_length =
                        sizeof(struct mode_sense_data_6)-1;
                    tb.ms_data_6->medium_type = 0;
                    tb.ms_data_6->device_specific = 0;
                    tb.ms_data_6->block_descriptor_length =
                        sizeof(struct mode_sense_bdesc_shortlba);
                    tb.ms_data_6->block_descriptor.density_code = 0;
                    tb.ms_data_6->block_descriptor.reserved = 0;
                    if(bcount > 0xffffff) {
                        tb.ms_data_6->block_descriptor.num_blocks[0] = 0xff;
                        tb.ms_data_6->block_descriptor.num_blocks[1] = 0xff;
                        tb.ms_data_6->block_descriptor.num_blocks[2] = 0xff;
                    } else {
                        tb.ms_data_6->block_descriptor.num_blocks[0] =
                            (bcount & 0xff0000)>>16;
                        tb.ms_data_6->block_descriptor.num_blocks[1] =
                            (bcount & 0x00ff00)>>8;
                        tb.ms_data_6->block_descriptor.num_blocks[2] =
                            (bcount & 0x0000ff);
                    }
                    tb.ms_data_6->block_descriptor.block_size[0] =
                        (bsize & 0xff0000)>>16;
                    tb.ms_data_6->block_descriptor.block_size[1] =
                        (bsize & 0x00ff00)>>8;
                    tb.ms_data_6->block_descriptor.block_size[2] =
                        (bsize & 0x0000ff);
                    send_command_result(tb.ms_data_6,
                        MIN(sizeof(struct mode_sense_data_6), length));
                    break;
                default:
                    send_command_failed_result();
                    cur_sense_data.sense_key=SENSE_ILLEGAL_REQUEST;
                    cur_sense_data.asc=ASC_INVALID_FIELD_IN_CBD;
                    cur_sense_data.ascq=0;
                    break;
            }
            break;
        }

        case SCSI_START_STOP_UNIT:
            logf("scsi start_stop unit %d",lun);
            if((cbw->command_block[4] & 0xf0) == 0) /*load/eject bit is valid*/
            { /* Process start and eject bits */
                logf("scsi load/eject");
                if((cbw->command_block[4] & 0x01) == 0) /* Don't start */
                {
                    if((cbw->command_block[4] & 0x02) != 0) /* eject */
                    {
                        logf("scsi eject");
                        ejected[lun]=true;
                    }
                }
            }
            send_csw(UMS_STATUS_GOOD);
            break;

        case SCSI_ALLOW_MEDIUM_REMOVAL:
            logf("scsi allow_medium_removal %d",lun);
            if((cbw->command_block[4] & 0x03) == 0) {
                locked[lun]=false;
                queue_broadcast(SYS_USB_LUN_LOCKED, (lun<<16)+0);
            }
            else {
                locked[lun]=true;
                queue_broadcast(SYS_USB_LUN_LOCKED, (lun<<16)+1);
            }
            send_csw(UMS_STATUS_GOOD);
            break;

        case USB_UFI_READ_FORMAT_CAPACITY: {
            logf("usb ufi read_format_capacity %d",lun);
            if(lun_present) {
                tb.format_capacity_data->following_length=htobe32(8);
                /* "block count" actually means "number of last block" */
                if (bcount > 0xffffffff)
                    tb.format_capacity_data->block_count = 0xffffffff;
                else
                    tb.format_capacity_data->block_count = htobe32(bcount - 1);
                tb.format_capacity_data->block_size = htobe32(bsize);
                tb.format_capacity_data->block_size |=
                    htobe32(SCSI_FORMAT_CAPACITY_FORMATTED_MEDIA);

                send_command_result(tb.format_capacity_data,
                    MIN(sizeof(struct format_capacity), length));
           }
           else {
               send_command_failed_result();
               cur_sense_data.sense_key=SENSE_NOT_READY;
               cur_sense_data.asc=ASC_MEDIUM_NOT_PRESENT;
               cur_sense_data.ascq=0;
           }
           break;
        }

        case SCSI_READ_CAPACITY_10: {
            logf("scsi read_capacity10 %d",lun);

            if(lun_present) {
                /* "block count" actually means "number of last block" */
                if (bcount >= 0xffffffff) {
                    tb.capacity_data_10->block_count[0] = 0xff;
                    tb.capacity_data_10->block_count[1] = 0xff;
                    tb.capacity_data_10->block_count[2] = 0xff;
                    tb.capacity_data_10->block_count[3] = 0xff;
                } else {
                    tb.capacity_data_10->block_count[0] = ((bcount-1) & 0xff000000)>>24;
                    tb.capacity_data_10->block_count[1] = ((bcount-1) & 0x00ff0000)>>16;
                    tb.capacity_data_10->block_count[2] = ((bcount-1) & 0x0000ff00)>>8;
                    tb.capacity_data_10->block_count[3] = ((bcount-1) & 0x000000ff);
                }
                tb.capacity_data_10->block_size[0] = (bsize & 0xff000000)>>24;
                tb.capacity_data_10->block_size[1] = (bsize & 0x00ff0000)>>16;
                tb.capacity_data_10->block_size[2] = (bsize & 0x0000ff00)>>8;
                tb.capacity_data_10->block_size[3] = (bsize & 0x000000ff);

                send_command_result(tb.capacity_data_10,
                    MIN(sizeof(struct capacity_10), length));
            } else {
                send_command_failed_result();
                cur_sense_data.sense_key=SENSE_NOT_READY;
                cur_sense_data.asc=ASC_MEDIUM_NOT_PRESENT;
                cur_sense_data.ascq=0;
            }
            break;
        }
        case SCSI_READ_CAPACITY_16: {
            logf("scsi read_capacity16 %d",lun);

            if(lun_present) {
                /* "block count" actually means "number of last block" */
#ifdef STORAGE_64BIT_SECTOR
                    tb.capacity_data_16->block_count[2] =
                        ((bcount-1) & 0xff00000000ULL)>>40;
                    tb.capacity_data_16->block_count[3] =
                        ((bcount-1) & 0x00ff000000ULL)>>32;
#else
                    tb.capacity_data_16->block_count[2] = 0;
                    tb.capacity_data_16->block_count[3] = 0;
#endif
                    tb.capacity_data_16->block_count[4] =
                        ((bcount-1) & 0xff000000)>>24;
                    tb.capacity_data_16->block_count[5] =
                        ((bcount-1) & 0x00ff0000)>>16;
                    tb.capacity_data_16->block_count[6] =
                        ((bcount-1) & 0x0000ff00)>>8;
                    tb.capacity_data_16->block_count[7] =
                        ((bcount-1) & 0x000000ff);

                    tb.ms_data_10->block_descriptor.block_size[0] =
                        (bsize & 0xff000000)>>24;
                    tb.ms_data_10->block_descriptor.block_size[1] =
                        (bsize & 0x00ff0000)>>16;
                    tb.ms_data_10->block_descriptor.block_size[2] =
                        (bsize & 0x0000ff00)>>8;
                    tb.ms_data_10->block_descriptor.block_size[3] =
                        (bsize & 0x000000ff);

                tb.capacity_data_16->block_size[0] = (bsize & 0xff000000)>>24;
                tb.capacity_data_16->block_size[1] = (bsize & 0x00ff0000)>>16;
                tb.capacity_data_16->block_size[2] = (bsize & 0x0000ff00)>>8;
                tb.capacity_data_16->block_size[3] = (bsize & 0x000000ff);

                uint16_t flags = htobe16((1<<12) | find_first_set_bit(block_size_mult));
                memcpy(tb.capacity_data_16->flags, &flags, sizeof(flags));

                tb.capacity_data_16->lowest_aligned_lba[0] = 0;
                tb.capacity_data_16->lowest_aligned_lba[1] = 1;

                send_command_result(tb.capacity_data_16,
                    MIN(sizeof(struct capacity_16), length));
            } else {
                send_command_failed_result();
                cur_sense_data.sense_key=SENSE_NOT_READY;
                cur_sense_data.asc=ASC_MEDIUM_NOT_PRESENT;
                cur_sense_data.ascq=0;
            }
            break;
        }
        case SCSI_READ_10:
            logf("scsi read10 %d",lun);
            if(!lun_present) {
                send_command_failed_result();
                cur_sense_data.sense_key=SENSE_NOT_READY;
                cur_sense_data.asc=ASC_MEDIUM_NOT_PRESENT;
                cur_sense_data.ascq=0;
                break;
            }
            cur_cmd.data[0] = tb.transfer_buffer;
            cur_cmd.data[1] = &tb.transfer_buffer[READ_BUFFER_SIZE];
            cur_cmd.data_select=0;
            cur_cmd.sector = block_size_mult *
                (cbw->command_block[2] << 24 |
                 cbw->command_block[3] << 16 |
                 cbw->command_block[4] << 8  |
                 cbw->command_block[5] );
            cur_cmd.count = block_size_mult *
                (cbw->command_block[7] << 8 |
                 cbw->command_block[8]);
            cur_cmd.block_size = block_size;

            logf("scsi read %llu %d", cur_cmd.sector, cur_cmd.count);

            if((cur_cmd.sector + cur_cmd.count) > block_count) {
                send_csw(UMS_STATUS_FAIL);
                cur_sense_data.sense_key=SENSE_ILLEGAL_REQUEST;
                cur_sense_data.asc=ASC_LBA_OUT_OF_RANGE;
                cur_sense_data.ascq=0;
            }
            else {
#ifdef USB_USE_RAMDISK
                memcpy(cur_cmd.data[cur_cmd.data_select],
                        ramdisk_buffer + cur_cmd.sector*cur_cmd.block_size,
                        MIN(READ_BUFFER_SIZE/cur_cmd.block_size,cur_cmd.count)*cur_cmd.block_size);
#else
                cur_cmd.last_result = storage_read_sectors(IF_MD(cur_cmd.lun,)
                        cur_cmd.sector,
                        MIN(READ_BUFFER_SIZE/cur_cmd.block_size, cur_cmd.count),
                        cur_cmd.data[cur_cmd.data_select]);
#endif
                send_and_read_next();
            }
            break;
#ifdef STORAGE_64BIT_SECTOR
        case SCSI_READ_16:
            logf("scsi read16 %d",lun);
            if(!lun_present) {
                send_command_failed_result();
                cur_sense_data.sense_key=SENSE_NOT_READY;
                cur_sense_data.asc=ASC_MEDIUM_NOT_PRESENT;
                cur_sense_data.ascq=0;
                break;
            }
            cur_cmd.data[0] = tb.transfer_buffer;
            cur_cmd.data[1] = &tb.transfer_buffer[READ_BUFFER_SIZE];
            cur_cmd.data_select=0;
            cur_cmd.sector = block_size_mult *
                 ((uint64_t)cbw->command_block[2] << 56 |
                 (uint64_t)cbw->command_block[3] << 48 |
                 (uint64_t)cbw->command_block[4] << 40 |
                 (uint64_t)cbw->command_block[5] << 32 |
                 cbw->command_block[6] << 24 |
                 cbw->command_block[7] << 16 |
                 cbw->command_block[8] << 8  |
                 cbw->command_block[9]);
            cur_cmd.count = block_size_mult *
                (cbw->command_block[10] << 24 |
                 cbw->command_block[11] << 16 |
                 cbw->command_block[12] << 8 |
                 cbw->command_block[13]);
            cur_cmd.block_size = block_size;

            logf("scsi read %llu %d", cur_cmd.sector, cur_cmd.count);

            if((cur_cmd.sector + cur_cmd.count) > block_count) {
                send_csw(UMS_STATUS_FAIL);
                cur_sense_data.sense_key=SENSE_ILLEGAL_REQUEST;
                cur_sense_data.asc=ASC_LBA_OUT_OF_RANGE;
                cur_sense_data.ascq=0;
            }
            else {
#ifdef USB_USE_RAMDISK
                memcpy(cur_cmd.data[cur_cmd.data_select],
                        ramdisk_buffer + cur_cmd.sector*cur_cmd.block_size,
                        MIN(READ_BUFFER_SIZE/cur_cmd.block_size,cur_cmd.count)*cur_cmd.block_size);
#else
                cur_cmd.last_result = storage_read_sectors(IF_MD(cur_cmd.lun,)
                        cur_cmd.sector,
                        MIN(READ_BUFFER_SIZE/cur_cmd.block_size, cur_cmd.count),
                        cur_cmd.data[cur_cmd.data_select]);
#endif
                send_and_read_next();
            }
            break;
#endif
        case SCSI_WRITE_10:
            logf("scsi write10 %d",lun);
            if(!lun_present) {
                send_csw(UMS_STATUS_FAIL);
                cur_sense_data.sense_key=SENSE_NOT_READY;
                cur_sense_data.asc=ASC_MEDIUM_NOT_PRESENT;
                cur_sense_data.ascq=0;
                break;
            }
            cur_cmd.data[0] = tb.transfer_buffer;
            cur_cmd.data[1] = &tb.transfer_buffer[WRITE_BUFFER_SIZE];
            cur_cmd.data_select=0;
            cur_cmd.sector = block_size_mult *
                (cbw->command_block[2] << 24 |
                 cbw->command_block[3] << 16 |
                 cbw->command_block[4] << 8  |
                 cbw->command_block[5] );
            cur_cmd.count = block_size_mult *
                (cbw->command_block[7] << 8 |
                 cbw->command_block[8]);
            cur_cmd.block_size = block_size;

            /* expect data */
            if((cur_cmd.sector + cur_cmd.count) > block_count) {
                send_csw(UMS_STATUS_FAIL);
                cur_sense_data.sense_key=SENSE_ILLEGAL_REQUEST;
                cur_sense_data.asc=ASC_LBA_OUT_OF_RANGE;
                cur_sense_data.ascq=0;
            }
            else {
                receive_block_data(cur_cmd.data[0],
                        MIN(WRITE_BUFFER_SIZE, cur_cmd.count*cur_cmd.block_size));
            }
            break;
#ifdef STORAGE_64BIT_SECTOR
        case SCSI_WRITE_16:
            logf("scsi write16 %d",lun);
            if(!lun_present) {
                send_csw(UMS_STATUS_FAIL);
                cur_sense_data.sense_key=SENSE_NOT_READY;
                cur_sense_data.asc=ASC_MEDIUM_NOT_PRESENT;
                cur_sense_data.ascq=0;
                break;
            }
            cur_cmd.data[0] = tb.transfer_buffer;
            cur_cmd.data[1] = &tb.transfer_buffer[WRITE_BUFFER_SIZE];
            cur_cmd.data_select=0;
            cur_cmd.sector = block_size_mult *
                ((uint64_t)cbw->command_block[2] << 56 |
                 (uint64_t)cbw->command_block[3] << 48 |
                 (uint64_t)cbw->command_block[4] << 40 |
                 (uint64_t)cbw->command_block[5] << 32 |
                 cbw->command_block[6] << 24 |
                 cbw->command_block[7] << 16 |
                 cbw->command_block[8] << 8  |
                 cbw->command_block[9]);
            cur_cmd.count = block_size_mult *
                (cbw->command_block[10] << 24 |
                 cbw->command_block[11] << 16 |
                 cbw->command_block[12] << 8 |
                 cbw->command_block[13]);
            cur_cmd.block_size = block_size;

            /* expect data */
            if((cur_cmd.sector + cur_cmd.count) > block_count) {
                send_csw(UMS_STATUS_FAIL);
                cur_sense_data.sense_key=SENSE_ILLEGAL_REQUEST;
                cur_sense_data.asc=ASC_LBA_OUT_OF_RANGE;
                cur_sense_data.ascq=0;
            }
            else {
                receive_block_data(cur_cmd.data[0],
                        MIN(WRITE_BUFFER_SIZE, cur_cmd.count*cur_cmd.block_size));
            }
            break;
#endif

#if CONFIG_RTC
        case SCSI_WRITE_BUFFER:
            if(cbw->command_block[1]==1 /* mode = vendor specific */
            && cbw->command_block[2]==0 /* buffer id = 0 */

            && cbw->command_block[3]==0x0c /* offset (3 bytes) */
            && cbw->command_block[4]==0
            && cbw->command_block[5]==0

            /* Some versions of itunes set the parameter list length to 0.
             * Technically it should be 0x0c, which is what libgpod sends */
            && cbw->command_block[6]==0 /* parameter list (3 bytes) */
            && cbw->command_block[7]==0
            && (cbw->command_block[8]==0 || cbw->command_block[8]==0x0c)

            && cbw->command_block[9]==0)
            receive_time();
            break;
#endif /* CONFIG_RTC */

        default:
            logf("scsi unknown cmd %x",cbw->command_block[0x0]);
            send_csw(UMS_STATUS_FAIL);
            cur_sense_data.sense_key=SENSE_ILLEGAL_REQUEST;
            cur_sense_data.asc=ASC_INVALID_COMMAND;
            cur_sense_data.ascq=0;
            break;
    }
}

static void send_block_data(void *data,int size)
{
    usb_drv_send_nonblocking(ep_in, data,size);
    state = SENDING_BLOCKS;
}

static void send_command_result(void *data,int size)
{
    usb_drv_send_nonblocking(ep_in, data,size);
    state = SENDING_RESULT;
}

static void send_command_failed_result(void)
{
    usb_drv_send_nonblocking(ep_in, NULL, 0);
    state = SENDING_FAILED_RESULT;
}

#if CONFIG_RTC
static void receive_time(void)
{
    usb_drv_recv_nonblocking(ep_out, tb.transfer_buffer, 12);
    state = RECEIVING_TIME;
}
#endif /* CONFIG_RTC */

static void receive_block_data(void *data,int size)
{
    usb_drv_recv_nonblocking(ep_out, data, size);
    state = RECEIVING_BLOCKS;
}

static void send_csw(int status)
{
    tb.csw->signature = htole32(CSW_SIGNATURE);
    tb.csw->tag = cur_cmd.tag;
    tb.csw->data_residue = 0;
    tb.csw->status = status;

    usb_drv_send_nonblocking(ep_in, tb.csw,
            sizeof(struct command_status_wrapper));
    state = WAITING_FOR_CSW_COMPLETION_OR_COMMAND;
    //logf("CSW: %X",status);
    /* Already start waiting for the next command */
    usb_drv_recv_nonblocking(ep_out, cbw_buffer, MAX_CBW_SIZE);
    /* The next completed transfer will be either the CSW one
     * or the new command */

    if(status == UMS_STATUS_GOOD) {
        cur_sense_data.sense_key=0;
        cur_sense_data.information=0;
        cur_sense_data.asc=0;
        cur_sense_data.ascq=0;
    }
}

static void copy_padded(char *dest, char *src, int len)
{
    for (int i = 0; i < len; i++) {
        if (src[i] == '\0') {
            memset(&dest[i], ' ', len - i);
            return;
        }
        dest[i] = src[i];
    }
}

/* build SCSI INQUIRY */
static void fill_inquiry(IF_MD_NONVOID(int lun))
{
    struct storage_info info;
    memset(tb.inquiry, 0, sizeof(struct inquiry_data));
    storage_get_info(lun,&info);
    copy_padded(tb.inquiry->VendorId,info.vendor,sizeof(tb.inquiry->VendorId));
    copy_padded(tb.inquiry->ProductId,info.product,sizeof(tb.inquiry->ProductId));
    copy_padded(tb.inquiry->ProductRevisionLevel,info.revision,sizeof(tb.inquiry->ProductRevisionLevel));

    tb.inquiry->DeviceType = DIRECT_ACCESS_DEVICE;
    tb.inquiry->AdditionalLength = 0x1f;
//    memset(tb.inquiry->Reserved, 0, sizeof(tb.inquiry->Reserved)); // Redundant
    tb.inquiry->Versions = 4; /* SPC-2 */
    tb.inquiry->Format = 2; /* SPC-2/3 inquiry format */

    tb.inquiry->DeviceTypeModifier = DEVICE_REMOVABLE;
}
