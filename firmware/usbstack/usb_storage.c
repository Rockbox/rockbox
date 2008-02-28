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
/* Needed to get at the audio buffer */
#include "audio.h"

#ifdef USB_STORAGE

/* Enable the following define to export only the SD card slot. This
 * is useful for USBCV MSC tests, as those are destructive.
 * This won't work right if the device doesn't have a card slot.
 */
//#define ONLY_EXPOSE_CARD_SLOT

#define SECTOR_SIZE 512

/* We can currently use up to 20k buffer size. More than that requires
 * transfer chaining in the driver. Tests on sansa c200 show that the 16k
 * limitation causes no more than 2% slowdown.
 */
#define BUFFER_SIZE 16384

/* bulk-only class specific requests */
#define USB_BULK_RESET_REQUEST   0xff
#define USB_BULK_GET_MAX_LUN     0xfe

#define DIRECT_ACCESS_DEVICE	 0x00    /* disks */
#define DEVICE_REMOVABLE	 0x80

#define CBW_SIGNATURE            0x43425355
#define CSW_SIGNATURE            0x53425355

#define SCSI_TEST_UNIT_READY      0x00
#define SCSI_INQUIRY              0x12
#define SCSI_MODE_SENSE_6         0x1a
#define SCSI_MODE_SENSE_10        0x5a
#define SCSI_REQUEST_SENSE        0x03
#define SCSI_ALLOW_MEDIUM_REMOVAL 0x1e
#define SCSI_READ_CAPACITY        0x25
#define SCSI_READ_FORMAT_CAPACITY 0x23
#define SCSI_READ_10              0x28
#define SCSI_WRITE_10             0x2a
#define SCSI_START_STOP_UNIT      0x1b
#define SCSI_REPORT_LUNS          0xa0

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

struct report_lun_data {
    unsigned int lun_list_length;
    unsigned int reserved1;
    unsigned char lun0[8];
#ifdef HAVE_HOTSWAP
    unsigned char lun1[8];
#endif
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

struct mode_sense_header_10 {
    unsigned short mode_data_length;
    unsigned char medium_type;
    unsigned char device_specific;
    unsigned char reserved1[2];
    unsigned short block_descriptor_length;
} __attribute__ ((packed));

struct mode_sense_header_6 {
    unsigned char mode_data_length;
    unsigned char medium_type;
    unsigned char device_specific;
    unsigned char block_descriptor_length;
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

static unsigned char* transfer_buffer;

static struct inquiry_data* inquiry;
static struct capacity* capacity_data;
static struct format_capacity* format_capacity_data;
static struct sense_data *sense_data;
static struct mode_sense_header_6 *mode_sense_data_6;
static struct mode_sense_header_10 *mode_sense_data_10;
static struct report_lun_data *lun_data;
static struct command_status_wrapper* csw;
static char *max_lun;

static struct {
    unsigned int sector;
    unsigned int count;
    unsigned int tag;
    unsigned int lun;
    unsigned char *data[2];
    unsigned char data_select;
    unsigned int last_result;
} current_cmd;

static struct {
    unsigned char sense_key;
    unsigned char information;
    unsigned char asc;
} cur_sense_data;

static void handle_scsi(struct command_block_wrapper* cbw);
static void send_csw(int status);
static void send_command_result(void *data,int size);
static void send_block_data(void *data,int size);
static void receive_block_data(void *data,int size);
static void identify2inquiry(int lun);
static void send_and_read_next(void);

static enum {
    WAITING_FOR_COMMAND,
    SENDING_BLOCKS,
    SENDING_RESULT,
    RECEIVING_BLOCKS,
    SENDING_CSW
} state = WAITING_FOR_COMMAND;

/* called by usb_code_init() */
void usb_storage_init(void)
{
    size_t bufsize;
    unsigned char * audio_buffer = audio_get_buffer(false,&bufsize);
    /* TODO : check if bufsize is at least 32K ? */
    transfer_buffer = (void *)UNCACHED_ADDR((unsigned int)(audio_buffer + 31) & 0xffffffe0);
    inquiry = (void*)transfer_buffer;
    capacity_data = (void*)transfer_buffer;
    format_capacity_data = (void*)transfer_buffer;
    sense_data = (void*)transfer_buffer;
    mode_sense_data_6 = (void*)transfer_buffer;
    mode_sense_data_10 = (void*)transfer_buffer;
    lun_data = (void*)transfer_buffer;
    max_lun = (void*)transfer_buffer;
    csw = (void*)transfer_buffer;
    logf("usb_storage_init done");
}

/* called by usb_core_transfer_complete() */
void usb_storage_transfer_complete(bool in,int status,int length)
{
    struct command_block_wrapper* cbw = (void*)transfer_buffer;

    //logf("transfer result %X %d", status, length);
    switch(state) {
        case RECEIVING_BLOCKS:
            if(in==true) {
                logf("IN received in RECEIVING");
            }
            logf("scsi write %d %d", current_cmd.sector, current_cmd.count);
            if(status==0) {
                if((unsigned int)length!=(SECTOR_SIZE*current_cmd.count)
                  && (unsigned int)length!=BUFFER_SIZE) {
                    logf("unexpected length :%d",length);
                }

                unsigned int next_sector = current_cmd.sector + (BUFFER_SIZE/SECTOR_SIZE);
                unsigned int next_count = current_cmd.count - MIN(current_cmd.count,BUFFER_SIZE/SECTOR_SIZE);

                if(next_count!=0) {
                    /* Ask the host to send more, to the other buffer */
                    receive_block_data(current_cmd.data[!current_cmd.data_select],
                                       MIN(BUFFER_SIZE,next_count*SECTOR_SIZE));
                }

                /* Now write the data that just came in, while the host is sending the next bit */
                int result = ata_write_sectors(IF_MV2(current_cmd.lun,)
                                               current_cmd.sector, MIN(BUFFER_SIZE/SECTOR_SIZE,current_cmd.count),
                                               current_cmd.data[current_cmd.data_select]);
                if(result != 0) {
                    send_csw(UMS_STATUS_FAIL);
                    cur_sense_data.sense_key=SENSE_MEDIUM_ERROR;
                    cur_sense_data.asc=ASC_WRITE_ERROR;
                    break;
                }

                if(next_count==0) {
                    send_csw(UMS_STATUS_GOOD);
                }

                /* Switch buffers for the next one */
                current_cmd.data_select=!current_cmd.data_select;

                current_cmd.sector = next_sector;
                current_cmd.count = next_count;

            }
            else {
                logf("Transfer failed %X",status);
                send_csw(UMS_STATUS_FAIL);
                /* TODO fill in cur_sense_data */
                cur_sense_data.sense_key=0;
                cur_sense_data.information=0;
                cur_sense_data.asc=0;
            }
            break;
        case WAITING_FOR_COMMAND:
            if(in==true) {
                logf("IN received in WAITING_FOR_COMMAND");
            }
            //logf("command received");
            if(letoh32(cbw->signature) == CBW_SIGNATURE){
                handle_scsi(cbw);
            }
            else {
                usb_drv_stall(EP_MASS_STORAGE, true,true);
                usb_drv_stall(EP_MASS_STORAGE, true,false);
            }
            break;
        case SENDING_CSW:
            if(in==false) {
                logf("OUT received in SENDING_CSW");
            }
            //logf("csw sent, now go back to idle");
            state = WAITING_FOR_COMMAND;
            usb_drv_recv(EP_MASS_STORAGE, transfer_buffer, 1024);
            break;
        case SENDING_RESULT:
            if(in==false) {
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
            }
            break;
        case SENDING_BLOCKS:
            if(in==false) {
                logf("OUT received in SENDING");
            }
            if(status==0) {
                if(current_cmd.count==0) {
                    //logf("data sent, now send csw");
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
            }
            break;
    }
}

/* called by usb_core_control_request() */
bool usb_storage_control_request(struct usb_ctrlrequest* req)
{
    bool handled = false;

    switch (req->bRequest) {
        case USB_BULK_GET_MAX_LUN: {
#ifdef ONLY_EXPOSE_CARD_SLOT
            *max_lun = 0;
#else
            *max_lun = NUM_VOLUMES - 1;
#endif
            logf("ums: getmaxlun");
            usb_drv_send(EP_CONTROL, UNCACHED_ADDR(max_lun), 1);
            usb_drv_recv(EP_CONTROL, NULL, 0); /* ack */
            handled = true;
            break;
        }

        case USB_BULK_RESET_REQUEST:
            logf("ums: bulk reset");
            state = WAITING_FOR_COMMAND;
            /* UMS BOT 3.1 says The device shall preserve the value of its bulk
             * data toggle bits and endpoint STALL conditions despite the Bulk-Only
             * Mass Storage Reset. */
#if 0
            usb_drv_reset_endpoint(EP_MASS_STORAGE, false);
            usb_drv_reset_endpoint(EP_MASS_STORAGE, true);
#endif
            
            usb_drv_send(EP_CONTROL, NULL, 0);  /* ack */
            handled = true;
            break;

        case USB_REQ_SET_CONFIGURATION:
            logf("ums: set config");
            /* prime rx endpoint. We only need room for commands */
            state = WAITING_FOR_COMMAND;
            usb_drv_recv(EP_MASS_STORAGE, transfer_buffer, 1024);
            handled = true;
            break;
    }

    return handled;
}

static void send_and_read_next(void)
{
    if(current_cmd.last_result!=0) {
        /* The last read failed. */
        send_csw(UMS_STATUS_FAIL);
        cur_sense_data.sense_key=SENSE_MEDIUM_ERROR;
        cur_sense_data.asc=ASC_READ_ERROR;
        return;
    }
    send_block_data(current_cmd.data[current_cmd.data_select],
                    MIN(BUFFER_SIZE,current_cmd.count*SECTOR_SIZE));

    /* Switch buffers for the next one */
    current_cmd.data_select=!current_cmd.data_select;

    current_cmd.sector+=(BUFFER_SIZE/SECTOR_SIZE);
    current_cmd.count-=MIN(current_cmd.count,BUFFER_SIZE/SECTOR_SIZE);

    if(current_cmd.count!=0){
        /* already read the next bit, so we can send it out immediately when the
         * current transfer completes.  */
        current_cmd.last_result = ata_read_sectors(IF_MV2(current_cmd.lun,) current_cmd.sector,
                                  MIN(BUFFER_SIZE/SECTOR_SIZE,current_cmd.count),
                                  current_cmd.data[current_cmd.data_select]);
    }
}
/****************************************************************************/

static void handle_scsi(struct command_block_wrapper* cbw)
{
    /* USB Mass Storage assumes LBA capability.
       TODO: support 48-bit LBA */

    unsigned int length = cbw->data_transfer_length;
    unsigned int block_size;
    unsigned int block_count;
    bool lun_present=true;
#ifdef ONLY_EXPOSE_CARD_SLOT
    unsigned char lun = cbw->lun+1;
#else
    unsigned char lun = cbw->lun;
#endif
    unsigned int block_size_mult = 1;
#ifdef HAVE_HOTSWAP
    tCardInfo* cinfo = card_get_info(lun);
    if(cinfo->initialized==1 && cinfo->numblocks > 0) {
        block_size = cinfo->blocksize;
        block_count = cinfo->numblocks;
    }
    else {
        lun_present=false;
        block_size = 0;
        block_count = 0;
    }
#else
    unsigned short* identify = ata_get_identify();
    block_size = SECTOR_SIZE;
    block_count = (identify[61] << 16 | identify[60]);
#endif

#ifdef MAX_LOG_SECTOR_SIZE
    block_size_mult = disk_sector_multiplier;
#endif

    current_cmd.tag = cbw->tag;
    current_cmd.lun = lun;

    switch (cbw->command_block[0]) {
        case SCSI_TEST_UNIT_READY:
            logf("scsi test_unit_ready %d",lun);
            if(lun_present) {
                send_csw(UMS_STATUS_GOOD);
            }
            else {
                send_csw(UMS_STATUS_FAIL);
                cur_sense_data.sense_key=SENSE_NOT_READY;
                cur_sense_data.asc=ASC_MEDIUM_NOT_PRESENT;
            }
            break;

        case SCSI_REPORT_LUNS: {
            logf("scsi inquiry %d",lun);
            int allocation_length=0;
            allocation_length|=(cbw->command_block[6]<<24);
            allocation_length|=(cbw->command_block[7]<<16);
            allocation_length|=(cbw->command_block[8]<<8);
            allocation_length|=(cbw->command_block[9]);
            memset(lun_data,0,sizeof(struct report_lun_data));
#ifdef HAVE_HOTSWAP
            lun_data->lun_list_length=htobe32(16);
            lun_data->lun1[1]=1;
#else
            lun_data->lun_list_length=htobe32(8);
#endif
            lun_data->lun0[1]=0;
            
            send_command_result(lun_data, MIN(sizeof(struct report_lun_data), length));
            break;
        }

        case SCSI_INQUIRY:
            logf("scsi inquiry %d",lun);
            identify2inquiry(lun);
            length = MIN(length, cbw->command_block[4]);
            send_command_result(inquiry, MIN(sizeof(struct inquiry_data), length));
            break;

        case SCSI_REQUEST_SENSE: {
            sense_data->ResponseCode=0x70;/*current error*/
            sense_data->filemark_eom_ili_sensekey=cur_sense_data.sense_key&0x0f;
            sense_data->Information=cur_sense_data.information;
            sense_data->AdditionalSenseLength=10;
            sense_data->CommandSpecificInformation=0;
            sense_data->AdditionalSenseCode=cur_sense_data.asc;
            sense_data->AdditionalSenseCodeQualifier=0;
            sense_data->FieldReplaceableUnitCode=0;
            sense_data->SKSV=0;
            sense_data->SenseKeySpecific=0;
            logf("scsi request_sense %d",lun);
            send_command_result(sense_data, sizeof(struct sense_data));
            break;
        }

        case SCSI_MODE_SENSE_10: {
            if(! lun_present) {
                usb_drv_stall(EP_MASS_STORAGE, true,true);
                send_csw(UMS_STATUS_FAIL);
                cur_sense_data.sense_key=SENSE_NOT_READY;
                cur_sense_data.asc=ASC_MEDIUM_NOT_PRESENT;
                break;
            }
            /*unsigned char pc = (cbw->command_block[2] & 0xc0) >>6;*/
            unsigned char page_code = cbw->command_block[2] & 0x3f;
            logf("scsi mode_sense_10 %d %X",lun,page_code);
            switch(page_code) {
                case 0x3f:
                    mode_sense_data_10->mode_data_length=0;
                    mode_sense_data_10->medium_type=0;
                    mode_sense_data_10->device_specific=0;
                    mode_sense_data_10->block_descriptor_length=0;
                    send_command_result(mode_sense_data_10,
                              MIN(sizeof(struct mode_sense_header_10), length));
                    break;
                default:
                    usb_drv_stall(EP_MASS_STORAGE, true,true);
                    send_csw(UMS_STATUS_FAIL);
                    cur_sense_data.sense_key=SENSE_ILLEGAL_REQUEST;
                    cur_sense_data.asc=ASC_INVALID_FIELD_IN_CBD;
                    break;
                }
            break;
        }
        case SCSI_MODE_SENSE_6: {
            if(! lun_present) {
                usb_drv_stall(EP_MASS_STORAGE, true,true);
                send_csw(UMS_STATUS_FAIL);
                cur_sense_data.sense_key=SENSE_NOT_READY;
                cur_sense_data.asc=ASC_MEDIUM_NOT_PRESENT;
                break;
            }
            /*unsigned char pc = (cbw->command_block[2] & 0xc0) >>6;*/
            unsigned char page_code = cbw->command_block[2] & 0x3f;
            logf("scsi mode_sense_6 %d %X",lun,page_code);
            switch(page_code) {
                case 0x3f:
                    /* All supported pages Since we support only one this is easy*/
                    mode_sense_data_6->mode_data_length=0;
                    mode_sense_data_6->medium_type=0;
                    mode_sense_data_6->device_specific=0;
                    mode_sense_data_6->block_descriptor_length=0;
                    send_command_result(mode_sense_data_6,
                              MIN(sizeof(struct mode_sense_header_6), length));
                    break;
                    /* TODO : windows does 1A, Power Condition. Do we need that ? */
                default:
                    usb_drv_stall(EP_MASS_STORAGE, true,true);
                    send_csw(UMS_STATUS_FAIL);
                    cur_sense_data.sense_key=SENSE_ILLEGAL_REQUEST;
                    cur_sense_data.asc=ASC_INVALID_FIELD_IN_CBD;
                    break;
            }
            break;
        }

        case SCSI_START_STOP_UNIT:
            logf("scsi start_stop unit %d",lun);
            send_csw(UMS_STATUS_GOOD);
            break;

        case SCSI_ALLOW_MEDIUM_REMOVAL:
            logf("scsi allow_medium_removal %d",lun);
            /* TODO: use this to show the connect screen ? */
            send_csw(UMS_STATUS_GOOD);
            break;
        case SCSI_READ_FORMAT_CAPACITY: {
            logf("scsi read_format_capacity %d",lun);
            if(lun_present) {
                format_capacity_data->following_length=htobe32(8);
                /* Careful: "block count" actually means "number of last block" */
                format_capacity_data->block_count = htobe32(block_count/block_size_mult - 1);
                format_capacity_data->block_size = htobe32(block_size*block_size_mult);
                format_capacity_data->block_size |= SCSI_FORMAT_CAPACITY_FORMATTED_MEDIA;

                send_command_result(format_capacity_data,
                          MIN(sizeof(struct format_capacity), length));
            }
            else
            {
                usb_drv_stall(EP_MASS_STORAGE, true,true);
                send_csw(UMS_STATUS_FAIL);
                cur_sense_data.sense_key=SENSE_NOT_READY;
                cur_sense_data.asc=ASC_MEDIUM_NOT_PRESENT;
            }
            break;
        }
        case SCSI_READ_CAPACITY: {
            logf("scsi read_capacity %d",lun);

            if(lun_present) {
                /* Careful: "block count" actually means "number of last block" */
                capacity_data->block_count = htobe32(block_count/block_size_mult - 1);
                capacity_data->block_size = htobe32(block_size*block_size_mult);

                send_command_result(capacity_data, MIN(sizeof(struct capacity), length));
            }
            else
            {
                usb_drv_stall(EP_MASS_STORAGE, true,true);
                send_csw(UMS_STATUS_FAIL);
                cur_sense_data.sense_key=SENSE_NOT_READY;
                cur_sense_data.asc=ASC_MEDIUM_NOT_PRESENT;
            }
            break;
        }

        case SCSI_READ_10:
            logf("scsi read10 %d",lun);
            if(! lun_present) {
                usb_drv_stall(EP_MASS_STORAGE, true,true);
                send_csw(UMS_STATUS_FAIL);
                cur_sense_data.sense_key=SENSE_NOT_READY;
                cur_sense_data.asc=ASC_MEDIUM_NOT_PRESENT;
                break;
            }
            current_cmd.data[0] = transfer_buffer;
            current_cmd.data[1] = &transfer_buffer[BUFFER_SIZE];
            current_cmd.data_select=0;
            current_cmd.sector = block_size_mult *
               (cbw->command_block[2] << 24 |
                cbw->command_block[3] << 16 |
                cbw->command_block[4] << 8  |
                cbw->command_block[5] );
            current_cmd.count = block_size_mult *
               (cbw->command_block[7] << 16 |
                cbw->command_block[8]);

            //logf("scsi read %d %d", current_cmd.sector, current_cmd.count);

            if((current_cmd.sector + current_cmd.count) > block_count) {
                usb_drv_stall(EP_MASS_STORAGE, true,true);
                send_csw(UMS_STATUS_FAIL);
                cur_sense_data.sense_key=SENSE_ILLEGAL_REQUEST;
                cur_sense_data.asc=ASC_LBA_OUT_OF_RANGE;
            }
            else {
                current_cmd.last_result = ata_read_sectors(IF_MV2(current_cmd.lun,) current_cmd.sector,
                                              MIN(BUFFER_SIZE/SECTOR_SIZE,current_cmd.count),
                                              current_cmd.data[current_cmd.data_select]);
                send_and_read_next();
            }
            break;

        case SCSI_WRITE_10:
            logf("scsi write10 %d",lun);
            if(! lun_present) {
                usb_drv_stall(EP_MASS_STORAGE, true,true);
                send_csw(UMS_STATUS_FAIL);
                cur_sense_data.sense_key=SENSE_NOT_READY;
                cur_sense_data.asc=ASC_MEDIUM_NOT_PRESENT;
                break;
            }
            current_cmd.data[0] = transfer_buffer;
            current_cmd.data[1] = &transfer_buffer[BUFFER_SIZE];
            current_cmd.data_select=0;
            current_cmd.sector = block_size_mult *
               (cbw->command_block[2] << 24 |
                cbw->command_block[3] << 16 |
                cbw->command_block[4] << 8  |
                cbw->command_block[5] );
            current_cmd.count = block_size_mult *
               (cbw->command_block[7] << 16 |
                cbw->command_block[8]);
            /* expect data */
            if((current_cmd.sector + current_cmd.count) > block_count) {
                usb_drv_stall(EP_MASS_STORAGE, true,true);
                send_csw(UMS_STATUS_FAIL);
                cur_sense_data.sense_key=SENSE_ILLEGAL_REQUEST;
                cur_sense_data.asc=ASC_LBA_OUT_OF_RANGE;
            }
            else {
                receive_block_data(current_cmd.data[0],
                                   MIN(BUFFER_SIZE,current_cmd.count*SECTOR_SIZE));
            }

            break;

        default:
            logf("scsi unknown cmd %x",cbw->command_block[0x0]);
            usb_drv_stall(EP_MASS_STORAGE, true,true);
            send_csw(UMS_STATUS_FAIL);
            break;
    }
}

static void send_block_data(void *data,int size)
{
    usb_drv_send_nonblocking(EP_MASS_STORAGE, data,size);
    state = SENDING_BLOCKS;
}

static void send_command_result(void *data,int size)
{
    usb_drv_send_nonblocking(EP_MASS_STORAGE, data,size);
    state = SENDING_RESULT;
}

static void receive_block_data(void *data,int size)
{
    usb_drv_recv(EP_MASS_STORAGE, data, size);
    state = RECEIVING_BLOCKS;
}

static void send_csw(int status)
{
    csw->signature = htole32(CSW_SIGNATURE);
    csw->tag = current_cmd.tag;
    csw->data_residue = 0;
    csw->status = status;

    usb_drv_send_nonblocking(EP_MASS_STORAGE, csw, sizeof(struct command_status_wrapper));
    state = SENDING_CSW;
    //logf("CSW: %X",status);

    if(status == UMS_STATUS_GOOD) {
        cur_sense_data.sense_key=0;
        cur_sense_data.information=0;
        cur_sense_data.asc=0;
    }
}

/* convert ATA IDENTIFY to SCSI INQUIRY */
static void identify2inquiry(int lun)
{
#ifdef HAVE_FLASH_STORAGE
    if(lun==0) {
        memcpy(&inquiry->VendorId,"Rockbox ",8);
        memcpy(&inquiry->ProductId,"Internal Storage",16);
        memcpy(&inquiry->ProductRevisionLevel,"0.00",4);
    }
    else {
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
    memset(inquiry, 0, sizeof(struct inquiry_data));
    
    if (identify[82] & 4)
        inquiry->DeviceTypeModifier = DEVICE_REMOVABLE;

    /* ATA only has a 'model' field, so we copy the 
       first 8 bytes to 'vendor' and the rest to 'product' (they are
       consecutive in the inquiry struct) */
    src = (unsigned short*)&identify[27];
    dest = (unsigned short*)&inquiry->VendorId;
    for (i=0;i<12;i++)
        dest[i] = htobe16(src[i]);
    
    src = (unsigned short*)&identify[23];
    dest = (unsigned short*)&inquiry->ProductRevisionLevel;
    for (i=0;i<2;i++)
        dest[i] = htobe16(src[i]);
#endif

    inquiry->DeviceType = DIRECT_ACCESS_DEVICE;
    inquiry->AdditionalLength = 0x1f;
    inquiry->Versions = 4; /* SPC-2 */
    inquiry->Format   = 2; /* SPC-2/3 inquiry format */

#ifdef HAVE_HOTSWAP
    if(lun>0)
        inquiry->DeviceTypeModifier = DEVICE_REMOVABLE;
#endif

}

#endif /* USB_STORAGE */
