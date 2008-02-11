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
#include "disk.h"

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
#define SCSI_REQUEST_SENSE        0x03
#define SCSI_ALLOW_MEDIUM_REMOVAL 0x1e
#define SCSI_READ_CAPACITY        0x25
#define SCSI_READ_FORMAT_CAPACITY 0x23
#define SCSI_READ_10              0x28
#define SCSI_WRITE_10             0x2a
#define SCSI_START_STOP_UNIT      0x1b

#define SCSI_STATUS_GOOD            0x00
#define SCSI_STATUS_FAIL            0x01
#define SCSI_STATUS_CHECK_CONDITION 0x02

#define SCSI_FORMAT_CAPACITY_FORMATTED_MEDIA 0x02000000


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

struct sense_data {
    unsigned char ResponseCode;
    unsigned char Obsolete;
    unsigned char filemark_eom_ili_sensekey;
    unsigned int Information;
    unsigned char AdditionalSenseLength;
    unsigned int  CommandSpecificInformation;
    unsigned char AdditionalSenseCode;
    unsigned char AdditionalSenseCodeQualifier;
    unsigned char FieldReplaceableUnitCode;
    unsigned char SKSV;
    unsigned short SenseKeySpecific;
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

struct format_capacity {
    unsigned int following_length;
    unsigned int block_count;
    unsigned int block_size;
} __attribute__ ((packed));

/* the ARC USB controller can at most buffer 16KB unaligned data */
static unsigned char _transfer_buffer[16384*8] __attribute((aligned (4096)));
static unsigned char* transfer_buffer;
static struct inquiry_data _inquiry CACHEALIGN_ATTR;
static struct inquiry_data* inquiry;
static struct capacity _capacity_data CACHEALIGN_ATTR;
static struct capacity* capacity_data;
static struct format_capacity _format_capacity_data CACHEALIGN_ATTR;
static struct format_capacity* format_capacity_data;
static struct sense_data _sense_data CACHEALIGN_ATTR;
static struct sense_data *sense_data;

static struct {
    unsigned int sector;
    unsigned int count;
    unsigned int tag;
    unsigned int lun;
} current_cmd;

static void handle_scsi(struct command_block_wrapper* cbw);
static void send_csw(unsigned int tag, int status);
static void identify2inquiry(int lun);

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
    format_capacity_data = (void*)UNCACHED_ADDR(&_format_capacity_data);
    sense_data = (void*)UNCACHED_ADDR(&_sense_data);
    state = IDLE;
    logf("usb_storage_init done");
}

/* called by usb_core_transfer_complete() */
void usb_storage_transfer_complete(int endpoint)
{
    struct command_block_wrapper* cbw = (void*)transfer_buffer;

    switch (endpoint) {
        case EP_RX:
            //logf("ums: %d bytes in", length);
            if(state == RECEIVING)
            {
                int receive_count=usb_drv_get_last_transfer_length();
                logf("scsi write %d %d", current_cmd.sector, current_cmd.count);
                if(usb_drv_get_last_transfer_status()==0)
                {
                    if((unsigned int)receive_count!=(SECTOR_SIZE*current_cmd.count))
                    {
                        logf("%d >= %d",SECTOR_SIZE*current_cmd.count,receive_count);
                    }
                    ata_write_sectors(IF_MV2(current_cmd.lun,)
                                      current_cmd.sector, current_cmd.count,
                                      transfer_buffer);
                    send_csw(current_cmd.tag, SCSI_STATUS_GOOD);
                }
                else
                {
                    logf("Transfer failed %X",usb_drv_get_last_transfer_status());
                    send_csw(current_cmd.tag, SCSI_STATUS_CHECK_CONDITION);
                }
            }
            else
            {
                state = SENDING;
                handle_scsi(cbw);
            }
            
            break;

        case EP_TX:
            //logf("ums: out complete");
            if(state != IDLE)
            {
                /* re-prime endpoint. We only need room for commands */
                state = IDLE;
                usb_drv_recv(EP_RX, transfer_buffer, 1024);
            }

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
            static char maxlun = NUM_VOLUMES - 1;
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
            /* prime rx endpoint. We only need room for commands */
            state = IDLE;
            usb_drv_recv(EP_RX, transfer_buffer, 1024);
            handled = true;
            break;
    }

    return handled;
}

/****************************************************************************/

static void handle_scsi(struct command_block_wrapper* cbw)
{
    /* USB Mass Storage assumes LBA capability.
       TODO: support 48-bit LBA */

    unsigned int sectors_per_transfer=0;
    unsigned int length = cbw->data_transfer_length;
    unsigned int block_size;
    unsigned char lun = cbw->lun;
    unsigned int block_size_mult = 1;
#ifdef HAVE_HOTSWAP
    tCardInfo* cinfo = card_get_info(lun);
    block_size = cinfo->blocksize;
    if(cinfo->initialized==1)
    {
       sectors_per_transfer=(sizeof _transfer_buffer/ block_size);
    }
#else
    block_size = SECTOR_SIZE;
    sectors_per_transfer=(sizeof _transfer_buffer/ block_size);
#endif

#ifdef MAX_LOG_SECTOR_SIZE
    block_size_mult = disk_sector_multiplier;
#endif

    switch (cbw->command_block[0]) {
        case SCSI_TEST_UNIT_READY:
            logf("scsi test_unit_ready %d",lun);
#ifdef HAVE_HOTSWAP
            if(cinfo->initialized==1)
                send_csw(cbw->tag, SCSI_STATUS_GOOD);
            else
                send_csw(cbw->tag, SCSI_STATUS_FAIL);
#else
            send_csw(cbw->tag, SCSI_STATUS_GOOD);
#endif
            break;

        case SCSI_INQUIRY:
            logf("scsi inquiry %d",lun);
            identify2inquiry(lun);
            length = MIN(length, cbw->command_block[4]);
            usb_drv_send(EP_TX, inquiry, MIN(sizeof _inquiry, length));
            send_csw(cbw->tag, SCSI_STATUS_GOOD);
            break;

        case SCSI_REQUEST_SENSE: {
            sense_data->ResponseCode=0x70;
            sense_data->filemark_eom_ili_sensekey=2;
            sense_data->Information=2;
            sense_data->AdditionalSenseLength=10;
            sense_data->CommandSpecificInformation=0;
            sense_data->AdditionalSenseCode=0x3a;
            sense_data->AdditionalSenseCodeQualifier=0;
            sense_data->FieldReplaceableUnitCode=0;
            sense_data->SKSV=0;
            sense_data->SenseKeySpecific=0;
            logf("scsi request_sense %d",lun);
            usb_drv_send(EP_TX, sense_data,
                         sizeof(_sense_data));
            send_csw(cbw->tag, SCSI_STATUS_GOOD);
            break;
        }

        case SCSI_MODE_SENSE: {
            static unsigned char sense_data[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
            logf("scsi mode_sense %d",lun);
            usb_drv_send(EP_TX, UNCACHED_ADDR(&sense_data),
                         MIN(sizeof sense_data, length));
            send_csw(cbw->tag, SCSI_STATUS_GOOD);
            break;
        }

        case SCSI_START_STOP_UNIT:
            logf("scsi start_stop unit %d",lun);
            send_csw(cbw->tag, SCSI_STATUS_GOOD);
            break;

        case SCSI_ALLOW_MEDIUM_REMOVAL:
            logf("scsi allow_medium_removal %d",lun);
            send_csw(cbw->tag, SCSI_STATUS_GOOD);
            break;

        case SCSI_READ_FORMAT_CAPACITY: {
            logf("scsi read_format_capacity %d",lun);
            format_capacity_data->following_length=htobe32(8);
#ifdef HAVE_HOTSWAP
            /* Careful: "block count" actually means "number of last block" */
            if(cinfo->initialized==1)
            {
                format_capacity_data->block_count = htobe32(cinfo->numblocks - 1);
                format_capacity_data->block_size = htobe32(cinfo->blocksize);
            }
            else
            {
                format_capacity_data->block_count = htobe32(0);
                format_capacity_data->block_size = htobe32(0);
            }
#else
            unsigned short* identify = ata_get_identify();
            /* Careful: "block count" actually means "number of last block" */
            format_capacity_data->block_count = htobe32((identify[61] << 16 | identify[60]) / block_size_mult - 1);
            format_capacity_data->block_size = htobe32(block_size * block_size_mult);
#endif
            format_capacity_data->block_size |= SCSI_FORMAT_CAPACITY_FORMATTED_MEDIA;

            usb_drv_send(EP_TX, format_capacity_data,
                         MIN(sizeof _format_capacity_data, length));
            send_csw(cbw->tag, SCSI_STATUS_GOOD);
            break;
        }

        case SCSI_READ_CAPACITY: {
            logf("scsi read_capacity %d",lun);
#ifdef HAVE_HOTSWAP
            /* Careful: "block count" actually means "number of last block" */
            if(cinfo->initialized==1)
            {
                capacity_data->block_count = htobe32(cinfo->numblocks - 1);
                capacity_data->block_size = htobe32(cinfo->blocksize);
            }
            else
            {
                capacity_data->block_count = htobe32(0);
                capacity_data->block_size = htobe32(0);
            }
#else
            unsigned short* identify = ata_get_identify();
            /* Careful : "block count" actually means the number of the last block */
            capacity_data->block_count = htobe32((identify[61] << 16 | identify[60]) / block_size_mult - 1);
            capacity_data->block_size = htobe32(block_size * block_size_mult);
#endif
            usb_drv_send(EP_TX, capacity_data,
                         MIN(sizeof _capacity_data, length));
            send_csw(cbw->tag, SCSI_STATUS_GOOD);
            break;
        }

        case SCSI_READ_10:
            current_cmd.sector = block_size_mult *
               (cbw->command_block[2] << 24 |
                cbw->command_block[3] << 16 |
                cbw->command_block[4] << 8  |
                cbw->command_block[5] );
            current_cmd.count = block_size_mult *
               (cbw->command_block[7] << 16 |
                cbw->command_block[8]);
            current_cmd.tag = cbw->tag;
            current_cmd.lun = cbw->lun;

            //logf("scsi read %d %d", current_cmd.sector, current_cmd.count);

            //logf("Asked for %d sectors",current_cmd.count);
            if(current_cmd.count > sectors_per_transfer)
            {
                current_cmd.count = sectors_per_transfer;
            }
            //logf("Sending %d sectors",current_cmd.count);

            if(current_cmd.count*block_size > sizeof(_transfer_buffer)) {
                send_csw(current_cmd.tag, SCSI_STATUS_CHECK_CONDITION);
            }
            else {
                ata_read_sectors(IF_MV2(lun,) current_cmd.sector,
                                 current_cmd.count, transfer_buffer);
                usb_drv_send(EP_TX, transfer_buffer,
                             current_cmd.count*block_size);
                send_csw(current_cmd.tag, SCSI_STATUS_GOOD);
            }
            break;

        case SCSI_WRITE_10:
            //logf("scsi write10");
            current_cmd.sector = block_size_mult *
               (cbw->command_block[2] << 24 |
                cbw->command_block[3] << 16 |
                cbw->command_block[4] << 8  |
                cbw->command_block[5] );
            current_cmd.count = block_size_mult *
               (cbw->command_block[7] << 16 |
                cbw->command_block[8]);
            current_cmd.tag = cbw->tag;
            current_cmd.lun = cbw->lun;
            /* expect data */
            if(current_cmd.count*block_size > sizeof(_transfer_buffer)) {
                send_csw(current_cmd.tag, SCSI_STATUS_CHECK_CONDITION);
            }
            else {
                usb_drv_recv(EP_RX, transfer_buffer,
                             current_cmd.count*block_size);
                state = RECEIVING;
            }

            break;

        default:
            logf("scsi unknown cmd %x",cbw->command_block[0x0]);
            usb_drv_stall(EP_TX, true);
            send_csw(current_cmd.tag, SCSI_STATUS_GOOD);
            break;
    }
}

static void send_csw(unsigned int tag, int status)
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
static void identify2inquiry(int lun)
{
#ifdef HAVE_FLASH_STORAGE
    if(lun==0)
    {
        memcpy(&inquiry->VendorId,"Rockbox ",8);
        memcpy(&inquiry->ProductId,"Internal Storage",16);
        memcpy(&inquiry->ProductRevisionLevel,"0.00",4);
    }
    else
    {
        memcpy(&inquiry->VendorId,"Rockbox ",8);
        memcpy(&inquiry->ProductId,"SD Card Slot    ",16);
        memcpy(&inquiry->ProductRevisionLevel,"0.00",4);
    }
#else
    unsigned int i;
    unsigned short* dest;
    unsigned short* src;
    unsigned short* identify = ata_get_identify();
    (void)lun;
    memset(inquiry, 0, sizeof _inquiry);
    
    if (identify[82] & 4)
        inquiry->DeviceTypeModifier = DEVICE_REMOVABLE;

    /* ATA only has a 'model' field, so we copy the 
       first 8 bytes to 'vendor' and the rest to 'product' */
    src = (unsigned short*)&identify[27];
    dest = (unsigned short*)&inquiry->VendorId;
    for (i=0;i<4;i++)
        dest[i] = htobe16(src[i]);

    src = (unsigned short*)&identify[27+8];
    dest = (unsigned short*)&inquiry->ProductId;
    for (i=0;i<8;i++)
        dest[i] = htobe16(src[i]);
    
    src = (unsigned short*)&identify[23];
    dest = (unsigned short*)&inquiry->ProductRevisionLevel;
    for (i=0;i<2;i++)
        dest[i] = htobe16(src[i]);
#endif

    inquiry->DeviceType = DIRECT_ACCESS_DEVICE;
    inquiry->AdditionalLength = 0x1f;
    inquiry->Versions = 3; /* ANSI SCSI level 2 */
    inquiry->Format   = 3; /* ANSI SCSI level 2 INQUIRY format */

#ifdef HAVE_HOTSWAP
    inquiry->DeviceTypeModifier = DEVICE_REMOVABLE;
#endif

}

