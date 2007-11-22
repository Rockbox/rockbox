/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:  $
 *
 * Copyright (C) 2007 by Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "string.h"
#include "system.h"
#include "usb_core.h"
#include "usb_drv.h"
//#define LOGF_ENABLE
#include "logf.h"
#include "ata.h"
#include "hotswap.h"

#define SECTOR_SIZE 512

/* bulk-only class specific requests */
#define USB_BULK_RESET_REQUEST   0xff
#define USB_BULK_GET_MAX_LUN     0xfe

#define DIRECT_ACCESS_DEVICE	 0x00    /* disks */
#define DEVICE_REMOVABLE	 0x80

#define CBW_SIGNATURE            0x43425355
#define CSW_SIGNATURE            0x53425355

#define SCSI_TEST_UNIT_READY      0x00
#define SCSI_INQUIRY              0x12
#define SCSI_MODE_SENSE           0x1a
#define SCSI_ALLOW_MEDIUM_REMOVAL 0x1e
#define SCSI_READ_CAPACITY        0x25
#define SCSI_READ_10              0x28
#define SCSI_WRITE_10             0x2a

#define SCSI_STATUS_GOOD            0x00
#define SCSI_STATUS_CHECK_CONDITION 0x02


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

struct capacity {
    unsigned int block_count;
    unsigned int block_size;
} __attribute__ ((packed));

/* the ARC USB controller can at most buffer 16KB unaligned data */
static unsigned char _transfer_buffer[16384];
static unsigned char* transfer_buffer;
static struct inquiry_data _inquiry;
static struct inquiry_data* inquiry;
static struct capacity _capacity_data;
static struct capacity* capacity_data;

//static unsigned char partial_sector[SECTOR_SIZE];

static struct {
    unsigned int sector;
    unsigned int offset; /* if partial sector */
    unsigned int count;
    unsigned int tag;
} current_cmd;

 void handle_scsi(struct command_block_wrapper* cbw);
 void send_csw(unsigned int tag, int status);
static void identify2inquiry(void);

static enum {
    IDLE,
    SENDING,
    RECEIVING
} state = IDLE;

/* called by usb_code_init() */
void usb_storage_init(void)
{
    inquiry = (void*)UNCACHED_ADDR(&_inquiry);
    transfer_buffer = (void*)UNCACHED_ADDR(&_transfer_buffer);
    capacity_data = (void*)UNCACHED_ADDR(&_capacity_data);
    identify2inquiry();
}

/* called by usb_core_transfer_complete() */
void usb_storage_transfer_complete(int endpoint)
{
    struct command_block_wrapper* cbw = (void*)transfer_buffer;

    switch (endpoint) {
        case EP_RX:
            //logf("ums: %d bytes in", length);
            switch (state) {
                case IDLE:
                    handle_scsi(cbw);
                    break;

                default:
                    break;
            }

            /* re-prime endpoint */
            usb_drv_recv(EP_RX, transfer_buffer, sizeof _transfer_buffer);
            break;

        case EP_TX:
            //logf("ums: %d bytes out", length);
            break;
    }
}

/* called by usb_core_control_request() */
bool usb_storage_control_request(struct usb_ctrlrequest* req)
{
    /* note: interrupt context */

    bool handled = false;

    switch (req->bRequest) {
        case USB_BULK_GET_MAX_LUN: {
            static char maxlun = 0;
            logf("ums: getmaxlun");
            usb_drv_send(EP_CONTROL, UNCACHED_ADDR(&maxlun), 1);
            usb_drv_recv(EP_CONTROL, NULL, 0); /* ack */
            handled = true;
            break;
        }

        case USB_BULK_RESET_REQUEST:
            logf("ums: bulk reset");
            usb_drv_reset_endpoint(EP_RX, false);
            usb_drv_reset_endpoint(EP_TX, true);
            usb_drv_send(EP_CONTROL, NULL, 0);  /* ack */
            handled = true;
            break;

        case USB_REQ_SET_CONFIGURATION:
            logf("ums: set config");
            /* prime rx endpoint */
            usb_drv_recv(EP_RX, transfer_buffer, sizeof _transfer_buffer);
            handled = true;
            break;
    }

    return handled;
}

/****************************************************************************/

void handle_scsi(struct command_block_wrapper* cbw)
{
    /* USB Mass Storage assumes LBA capability.
       TODO: support 48-bit LBA */

    unsigned int length = cbw->data_transfer_length;

    switch (cbw->command_block[0]) {
        case SCSI_TEST_UNIT_READY:
            logf("scsi test_unit_ready");
            send_csw(cbw->tag, SCSI_STATUS_GOOD);
            break;

        case SCSI_INQUIRY:
            logf("scsi inquiry");
            length = MIN(length, cbw->command_block[4]);
            usb_drv_send(EP_TX, inquiry, MIN(sizeof _inquiry, length));
            send_csw(cbw->tag, SCSI_STATUS_GOOD);
            break;

        case SCSI_MODE_SENSE: {
            static unsigned char sense_data[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
            logf("scsi mode_sense");
            usb_drv_send(EP_TX, UNCACHED_ADDR(&sense_data),
                         MIN(sizeof sense_data, length));
            send_csw(cbw->tag, SCSI_STATUS_GOOD);
            break;
        }

        case SCSI_ALLOW_MEDIUM_REMOVAL:
            logf("scsi allow_medium_removal");
            send_csw(cbw->tag, SCSI_STATUS_GOOD);
            break;

        case SCSI_READ_CAPACITY: {
            logf("scsi read_capacity");
#ifdef HAVE_FLASH_STORAGE
            tCardInfo* cinfo = card_get_info(0);
            capacity_data->block_count = htobe32(cinfo->numblocks);
            capacity_data->block_size = htobe32(cinfo->blocksize);
#else
            unsigned short* identify = ata_get_identify();
            capacity_data->block_count = htobe32(identify[60] << 16 | identify[61]);
            capacity_data->block_size = htobe32(SECTOR_SIZE);
#endif
            usb_drv_send(EP_TX, capacity_data,
                         MIN(sizeof _capacity_data, length));
            send_csw(cbw->tag, SCSI_STATUS_GOOD);
            break;
        }

        case SCSI_READ_10:
            current_cmd.sector =
                cbw->command_block[2] << 24 |
                cbw->command_block[3] << 16 |
                cbw->command_block[4] << 8  |
                cbw->command_block[5] ;
            current_cmd.count =
                cbw->command_block[7] << 16 |
                cbw->command_block[8];
            current_cmd.offset = 0;
            current_cmd.tag = cbw->tag;

            logf("scsi read %d %d", current_cmd.sector, current_cmd.count);

            /* too much? */
            if (current_cmd.count > (sizeof _transfer_buffer / SECTOR_SIZE)) {
                send_csw(current_cmd.tag, SCSI_STATUS_CHECK_CONDITION);
                break;
            }

            ata_read_sectors(IF_MV2(0,) current_cmd.sector, current_cmd.count,
                             &transfer_buffer);
            state = SENDING;
            usb_drv_send(EP_TX, transfer_buffer,
                         MIN(current_cmd.count * SECTOR_SIZE, length));
            break;

        case SCSI_WRITE_10:
            logf("scsi write10");
            break;

        default:
            logf("scsi unknown cmd %x", cbw->command_block[0]);
            break;
    }
}

void send_csw(unsigned int tag, int status)
{
    static struct command_status_wrapper _csw;
    struct command_status_wrapper* csw = UNCACHED_ADDR(&_csw);
    csw->signature = CSW_SIGNATURE;
    csw->tag = tag;
    csw->data_residue = 0;
    csw->status = status;

    //logf("csw %x %x", csw->tag, csw->signature);
    usb_drv_send(EP_TX, csw, sizeof _csw);
}

/* convert ATA IDENTIFY to SCSI INQUIRY */
static void identify2inquiry(void)
{
    unsigned int i;
#ifdef HAVE_FLASH_STORAGE
    for (i=0; i<8; i++)
        inquiry->VendorId[i] = i + 'A';

    for (i=0; i<8; i++)
        inquiry->ProductId[i] = i + 'm';
#else
    unsigned short* dest;
    unsigned short* src;
    unsigned short* identify = ata_get_identify();
    memset(inquiry, 0, sizeof _inquiry);
    
    if (identify[82] & 4)
        inquiry->DeviceTypeModifier = DEVICE_REMOVABLE;

    /* ATA only has a 'model' field, so we copy the 
       first 8 bytes to 'vendor' and the rest to 'product' */
    src = (unsigned short*)&identify[27];
    dest = (unsigned short*)&inquiry->VendorId;
    for (i=0;i<4;i++)
        dest[i] = src[i];

    src = (unsigned short*)&identify[27+8];
    dest = (unsigned short*)&inquiry->ProductId;
    for (i=0;i<8;i++)
        dest[i] = src[i];
    
    src = (unsigned short*)&identify[23];
    dest = (unsigned short*)&inquiry->ProductRevisionLevel;
    for (i=0;i<2;i++)
        dest[i] = src[i];
#endif

    inquiry->DeviceType = DIRECT_ACCESS_DEVICE;
    inquiry->AdditionalLength = 0x1f;
    inquiry->Versions = 3; /* ANSI SCSI level 2 */
    inquiry->Format   = 3; /* ANSI SCSI level 2 INQUIRY format */
}

