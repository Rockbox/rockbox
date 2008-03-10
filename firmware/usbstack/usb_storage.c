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
#define ASC_NOT_READY               0x04
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
    unsigned char lun0[8];
#ifdef HAVE_HOTSWAP
    unsigned char lun1[8];
#endif
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

struct capacity {
    unsigned int block_count;
    unsigned int block_size;
} __attribute__ ((packed));

struct format_capacity {
    unsigned int following_length;
    unsigned int block_count;
    unsigned int block_size;
} __attribute__ ((packed));


static union {
    unsigned char* transfer_buffer;
    struct inquiry_data* inquiry;
    struct capacity* capacity_data;
    struct format_capacity* format_capacity_data;
    struct sense_data *sense_data;
    struct mode_sense_data_6 *ms_data_6;
    struct mode_sense_data_10 *ms_data_10;
    struct report_lun_data *lun_data;
    struct command_status_wrapper* csw;
    char *max_lun;
} tb;

static struct {
    unsigned int sector;
    unsigned int count;
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
static void send_block_data(void *data,int size);
static void receive_block_data(void *data,int size);
static void identify2inquiry(int lun);
static void send_and_read_next(void);
static bool ejected[NUM_VOLUMES];

static int usb_endpoint;
static int usb_interface;

static enum {
    WAITING_FOR_COMMAND,
    SENDING_BLOCKS,
    SENDING_RESULT,
    RECEIVING_BLOCKS,
    SENDING_CSW
} state = WAITING_FOR_COMMAND;

static bool check_disk_present(int volume)
{
    unsigned char sector[512];
    return ata_read_sectors(IF_MV2(volume,)0,1,sector) == 0;
}

static void try_release_ata(void)
{
    /* Check if there is a connected drive left. If not, 
       release excusive access */
    bool canrelease=true;
    int i;
    for(i=0;i<NUM_VOLUMES;i++) {
        if(ejected[i]==false){
            canrelease=false;
            break;
        }
    }
    if(canrelease) {
        logf("scsi release ata");
        usb_release_exclusive_ata();
    }
}

#ifdef HAVE_HOTSWAP
void usb_storage_notify_hotswap(int volume,bool inserted)
{
    logf("notify %d",inserted);
    if(inserted  && check_disk_present(volume)) {
        ejected[volume] = false;
    }
    else {
        ejected[volume] = true;
        try_release_ata();
    }

}
#endif

void usb_storage_reconnect(void)
{
    int i;
    for(i=0;i<NUM_VOLUMES;i++)
        ejected[i] = !check_disk_present(i);

    usb_request_exclusive_ata();
}

/* called by usb_code_init() */
void usb_storage_init(void)
{
    int i;
    for(i=0;i<NUM_VOLUMES;i++) {
        ejected[i] = !check_disk_present(i);
    }
    logf("usb_storage_init done");
}


int usb_storage_get_config_descriptor(unsigned char *dest,int max_packet_size,
                                      int interface_number,int endpoint)
{
    endpoint_descriptor.wMaxPacketSize=max_packet_size;
    interface_descriptor.bInterfaceNumber=interface_number;


    memcpy(dest,&interface_descriptor,
           sizeof(struct usb_interface_descriptor));
    dest+=sizeof(struct usb_interface_descriptor);

    endpoint_descriptor.bEndpointAddress = endpoint | USB_DIR_IN,
    memcpy(dest,&endpoint_descriptor,
           sizeof(struct usb_endpoint_descriptor));
    dest+=sizeof(struct usb_endpoint_descriptor);

    endpoint_descriptor.bEndpointAddress = endpoint | USB_DIR_OUT,
    memcpy(dest,&endpoint_descriptor,
           sizeof(struct usb_endpoint_descriptor));

    return sizeof(struct usb_interface_descriptor) + 
           2*sizeof(struct usb_endpoint_descriptor);
}

void usb_storage_init_connection(int interface,int endpoint)
{
    size_t bufsize;
    unsigned char * audio_buffer;

    usb_interface = interface;
    usb_endpoint = endpoint;

    logf("ums: set config");
    /* prime rx endpoint. We only need room for commands */
    state = WAITING_FOR_COMMAND;

    /* TODO : check if bufsize is at least 32K ? */
    audio_buffer = audio_get_buffer(false,&bufsize);
    tb.transfer_buffer =
        (void *)UNCACHED_ADDR((unsigned int)(audio_buffer + 31) & 0xffffffe0);
    usb_drv_recv(usb_endpoint, tb.transfer_buffer, 1024);
}

/* called by usb_core_transfer_complete() */
void usb_storage_transfer_complete(bool in,int status,int length)
{
    struct command_block_wrapper* cbw = (void*)tb.transfer_buffer;

    //logf("transfer result %X %d", status, length);
    switch(state) {
        case RECEIVING_BLOCKS:
            if(in==true) {
                logf("IN received in RECEIVING");
            }
            logf("scsi write %d %d", cur_cmd.sector, cur_cmd.count);
            if(status==0) {
                if((unsigned int)length!=(SECTOR_SIZE*cur_cmd.count)
                  && (unsigned int)length!=BUFFER_SIZE) {
                    logf("unexpected length :%d",length);
                }

                unsigned int next_sector = cur_cmd.sector +
                             (BUFFER_SIZE/SECTOR_SIZE);
                unsigned int next_count = cur_cmd.count -
                             MIN(cur_cmd.count,BUFFER_SIZE/SECTOR_SIZE);

                if(next_count!=0) {
                    /* Ask the host to send more, to the other buffer */
                    receive_block_data(cur_cmd.data[!cur_cmd.data_select],
                                       MIN(BUFFER_SIZE,next_count*SECTOR_SIZE));
                }

                /* Now write the data that just came in, while the host is
                   sending the next bit */
                int result = ata_write_sectors(IF_MV2(cur_cmd.lun,)
                                         cur_cmd.sector,
                                         MIN(BUFFER_SIZE/SECTOR_SIZE,
                                             cur_cmd.count),
                                         cur_cmd.data[cur_cmd.data_select]);
                if(result != 0) {
                    send_csw(UMS_STATUS_FAIL);
                    cur_sense_data.sense_key=SENSE_MEDIUM_ERROR;
                    cur_sense_data.asc=ASC_WRITE_ERROR;
                    cur_sense_data.ascq=0;
                    break;
                }

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
        case WAITING_FOR_COMMAND:
            if(in==true) {
                logf("IN received in WAITING_FOR_COMMAND");
            }
            //logf("command received");
            if(letoh32(cbw->signature) == CBW_SIGNATURE){
                handle_scsi(cbw);
            }
            else {
                usb_drv_stall(usb_endpoint, true,true);
                usb_drv_stall(usb_endpoint, true,false);
            }
            break;
        case SENDING_CSW:
            if(in==false) {
                logf("OUT received in SENDING_CSW");
            }
            //logf("csw sent, now go back to idle");
            state = WAITING_FOR_COMMAND;
            usb_drv_recv(usb_endpoint, tb.transfer_buffer, 1024);
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
                cur_sense_data.ascq=0;
            }
            break;
        case SENDING_BLOCKS:
            if(in==false) {
                logf("OUT received in SENDING");
            }
            if(status==0) {
                if(cur_cmd.count==0) {
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
                cur_sense_data.ascq=0;
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
            *tb.max_lun = 0;
#else
            *tb.max_lun = NUM_VOLUMES - 1;
#endif
            logf("ums: getmaxlun");
            usb_drv_send(EP_CONTROL, UNCACHED_ADDR(tb.max_lun), 1);
            usb_drv_recv(EP_CONTROL, NULL, 0); /* ack */
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
            usb_drv_reset_endpoint(usb_endpoint, false);
            usb_drv_reset_endpoint(usb_endpoint, true);
#endif
            
            usb_drv_send(EP_CONTROL, NULL, 0);  /* ack */
            handled = true;
            break;
    }

    return handled;
}

static void send_and_read_next(void)
{
    if(cur_cmd.last_result!=0) {
        /* The last read failed. */
        send_csw(UMS_STATUS_FAIL);
        cur_sense_data.sense_key=SENSE_MEDIUM_ERROR;
        cur_sense_data.asc=ASC_READ_ERROR;
        cur_sense_data.ascq=0;
        return;
    }
    send_block_data(cur_cmd.data[cur_cmd.data_select],
                    MIN(BUFFER_SIZE,cur_cmd.count*SECTOR_SIZE));

    /* Switch buffers for the next one */
    cur_cmd.data_select=!cur_cmd.data_select;

    cur_cmd.sector+=(BUFFER_SIZE/SECTOR_SIZE);
    cur_cmd.count-=MIN(cur_cmd.count,BUFFER_SIZE/SECTOR_SIZE);

    if(cur_cmd.count!=0){
        /* already read the next bit, so we can send it out immediately when the
         * current transfer completes.  */
        cur_cmd.last_result = ata_read_sectors(IF_MV2(cur_cmd.lun,)
                                           cur_cmd.sector,
                                           MIN(BUFFER_SIZE/SECTOR_SIZE,
                                               cur_cmd.count),
                                           cur_cmd.data[cur_cmd.data_select]);
    }
}
/****************************************************************************/

static void handle_scsi(struct command_block_wrapper* cbw)
{
    /* USB Mass Storage assumes LBA capability.
       TODO: support 48-bit LBA */

    unsigned int length = cbw->data_transfer_length;
    unsigned int block_size = 0;
    unsigned int block_count = 0;
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
        ejected[lun] = true;
        try_release_ata();
    }
#else
    unsigned short* identify = ata_get_identify();
    block_size = SECTOR_SIZE;
    block_count = (identify[61] << 16 | identify[60]);
#endif

    if(ejected[lun])
        lun_present = false;

#ifdef MAX_LOG_SECTOR_SIZE
    block_size_mult = disk_sector_multiplier;
#endif

    cur_cmd.tag = cbw->tag;
    cur_cmd.lun = lun;

    switch (cbw->command_block[0]) {
        case SCSI_TEST_UNIT_READY:
            logf("scsi test_unit_ready %d",lun);
            if(!usb_exclusive_ata()) {
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
            logf("scsi inquiry %d",lun);
            int allocation_length=0;
            allocation_length|=(cbw->command_block[6]<<24);
            allocation_length|=(cbw->command_block[7]<<16);
            allocation_length|=(cbw->command_block[8]<<8);
            allocation_length|=(cbw->command_block[9]);
            memset(tb.lun_data,0,sizeof(struct report_lun_data));
#ifdef HAVE_HOTSWAP
            tb.lun_data->lun_list_length=htobe32(16);
            tb.lun_data->lun1[1]=1;
#else
            tb.lun_data->lun_list_length=htobe32(8);
#endif
            tb.lun_data->lun0[1]=0;
            
            send_command_result(tb.lun_data,
                                MIN(sizeof(struct report_lun_data), length));
            break;
        }

        case SCSI_INQUIRY:
            logf("scsi inquiry %d",lun);
            identify2inquiry(lun);
            length = MIN(length, cbw->command_block[4]);
            send_command_result(tb.inquiry,
                                MIN(sizeof(struct inquiry_data), length));
            break;

        case SCSI_REQUEST_SENSE: {
            tb.sense_data->ResponseCode=0x70;/*current error*/
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
            send_command_result(tb.sense_data, sizeof(struct sense_data));
            break;
        }

        case SCSI_MODE_SENSE_10: {
            if(! lun_present) {
                send_csw(UMS_STATUS_FAIL);
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
                    memset(tb.ms_data_10->block_descriptor.num_blocks,0,8);

                    tb.ms_data_10->block_descriptor.num_blocks[4] =
                               ((block_count/block_size_mult) & 0xff000000)>>24;
                    tb.ms_data_10->block_descriptor.num_blocks[5] =
                               ((block_count/block_size_mult) & 0x00ff0000)>>16;
                    tb.ms_data_10->block_descriptor.num_blocks[6] =
                               ((block_count/block_size_mult) & 0x0000ff00)>>8;
                    tb.ms_data_10->block_descriptor.num_blocks[7] =
                               ((block_count/block_size_mult) & 0x000000ff);

                    tb.ms_data_10->block_descriptor.block_size[0] =
                                ((block_size*block_size_mult) & 0xff000000)>>24;
                    tb.ms_data_10->block_descriptor.block_size[1] =
                                ((block_size*block_size_mult) & 0x00ff0000)>>16;
                    tb.ms_data_10->block_descriptor.block_size[2] =
                                ((block_size*block_size_mult) & 0x0000ff00)>>8;
                    tb.ms_data_10->block_descriptor.block_size[3] =
                                ((block_size*block_size_mult) & 0x000000ff);
                    send_command_result(tb.ms_data_10,
                              MIN(sizeof(struct mode_sense_data_10), length));
                    break;
                default:
                    send_csw(UMS_STATUS_FAIL);
                    cur_sense_data.sense_key=SENSE_ILLEGAL_REQUEST;
                    cur_sense_data.asc=ASC_INVALID_FIELD_IN_CBD;
                    cur_sense_data.ascq=0;
                    break;
                }
            break;
        }
        case SCSI_MODE_SENSE_6: {
            if(! lun_present) {
                send_csw(UMS_STATUS_FAIL);
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
                    if(block_count/block_size_mult > 0xffffff){
                        tb.ms_data_6->block_descriptor.num_blocks[0] = 0xff;
                        tb.ms_data_6->block_descriptor.num_blocks[1] = 0xff;
                        tb.ms_data_6->block_descriptor.num_blocks[2] = 0xff;
                    }
                    else {
                        tb.ms_data_6->block_descriptor.num_blocks[0] =
                             ((block_count/block_size_mult) & 0xff0000)>>16;
                        tb.ms_data_6->block_descriptor.num_blocks[1] =
                             ((block_count/block_size_mult) & 0x00ff00)>>8;
                        tb.ms_data_6->block_descriptor.num_blocks[2] =
                             ((block_count/block_size_mult) & 0x0000ff);
                    }
                    tb.ms_data_6->block_descriptor.block_size[0] =
                         ((block_size*block_size_mult) & 0xff0000)>>16;
                    tb.ms_data_6->block_descriptor.block_size[1] =
                         ((block_size*block_size_mult) & 0x00ff00)>>8;
                    tb.ms_data_6->block_descriptor.block_size[2] =
                         ((block_size*block_size_mult) & 0x0000ff);
                    send_command_result(tb.ms_data_6,
                              MIN(sizeof(struct mode_sense_data_6), length));
                    break;
                default:
                    send_csw(UMS_STATUS_FAIL);
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
                        try_release_ata();
                    }
                }
            }
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
                tb.format_capacity_data->following_length=htobe32(8);
                /* "block count" actually means "number of last block" */
                tb.format_capacity_data->block_count =
                     htobe32(block_count/block_size_mult - 1);
                tb.format_capacity_data->block_size =
                     htobe32(block_size*block_size_mult);
                tb.format_capacity_data->block_size |=
                     SCSI_FORMAT_CAPACITY_FORMATTED_MEDIA;

                send_command_result(tb.format_capacity_data,
                          MIN(sizeof(struct format_capacity), length));
            }
            else
            {
                send_csw(UMS_STATUS_FAIL);
                cur_sense_data.sense_key=SENSE_NOT_READY;
                cur_sense_data.asc=ASC_MEDIUM_NOT_PRESENT;
                cur_sense_data.ascq=0;
            }
            break;
        }
        case SCSI_READ_CAPACITY: {
            logf("scsi read_capacity %d",lun);

            if(lun_present) {
                /* "block count" actually means "number of last block" */
                tb.capacity_data->block_count =
                     htobe32(block_count/block_size_mult - 1);
                tb.capacity_data->block_size =
                     htobe32(block_size*block_size_mult);

                send_command_result(tb.capacity_data,
                     MIN(sizeof(struct capacity), length));
            }
            else
            {
                send_csw(UMS_STATUS_FAIL);
                cur_sense_data.sense_key=SENSE_NOT_READY;
                cur_sense_data.asc=ASC_MEDIUM_NOT_PRESENT;
                cur_sense_data.ascq=0;
            }
            break;
        }

        case SCSI_READ_10:
            logf("scsi read10 %d",lun);
            if(! lun_present) {
                send_csw(UMS_STATUS_FAIL);
                cur_sense_data.sense_key=SENSE_NOT_READY;
                cur_sense_data.asc=ASC_MEDIUM_NOT_PRESENT;
                cur_sense_data.ascq=0;
                break;
            }
            cur_cmd.data[0] = tb.transfer_buffer;
            cur_cmd.data[1] = &tb.transfer_buffer[BUFFER_SIZE];
            cur_cmd.data_select=0;
            cur_cmd.sector = block_size_mult *
               (cbw->command_block[2] << 24 |
                cbw->command_block[3] << 16 |
                cbw->command_block[4] << 8  |
                cbw->command_block[5] );
            cur_cmd.count = block_size_mult *
               (cbw->command_block[7] << 8 |
                cbw->command_block[8]);

            //logf("scsi read %d %d", cur_cmd.sector, cur_cmd.count);

            if((cur_cmd.sector + cur_cmd.count) > block_count) {
                send_csw(UMS_STATUS_FAIL);
                cur_sense_data.sense_key=SENSE_ILLEGAL_REQUEST;
                cur_sense_data.asc=ASC_LBA_OUT_OF_RANGE;
                cur_sense_data.ascq=0;
            }
            else {
                cur_cmd.last_result = ata_read_sectors(IF_MV2(cur_cmd.lun,)
                                             cur_cmd.sector,
                                             MIN(BUFFER_SIZE/SECTOR_SIZE,
                                                 cur_cmd.count),
                                             cur_cmd.data[cur_cmd.data_select]);
                send_and_read_next();
            }
            break;

        case SCSI_WRITE_10:
            logf("scsi write10 %d",lun);
            if(! lun_present) {
                send_csw(UMS_STATUS_FAIL);
                cur_sense_data.sense_key=SENSE_NOT_READY;
                cur_sense_data.asc=ASC_MEDIUM_NOT_PRESENT;
                cur_sense_data.ascq=0;
                break;
            }
            cur_cmd.data[0] = tb.transfer_buffer;
            cur_cmd.data[1] = &tb.transfer_buffer[BUFFER_SIZE];
            cur_cmd.data_select=0;
            cur_cmd.sector = block_size_mult *
               (cbw->command_block[2] << 24 |
                cbw->command_block[3] << 16 |
                cbw->command_block[4] << 8  |
                cbw->command_block[5] );
            cur_cmd.count = block_size_mult *
               (cbw->command_block[7] << 8 |
                cbw->command_block[8]);
            /* expect data */
            if((cur_cmd.sector + cur_cmd.count) > block_count) {
                send_csw(UMS_STATUS_FAIL);
                cur_sense_data.sense_key=SENSE_ILLEGAL_REQUEST;
                cur_sense_data.asc=ASC_LBA_OUT_OF_RANGE;
                cur_sense_data.ascq=0;
            }
            else {
                receive_block_data(cur_cmd.data[0],
                                   MIN(BUFFER_SIZE,
                                   cur_cmd.count*SECTOR_SIZE));
            }

            break;

        default:
            logf("scsi unknown cmd %x",cbw->command_block[0x0]);
            usb_drv_stall(usb_endpoint, true,true);
            send_csw(UMS_STATUS_FAIL);
            break;
    }
}

static void send_block_data(void *data,int size)
{
    usb_drv_send_nonblocking(usb_endpoint, data,size);
    state = SENDING_BLOCKS;
}

static void send_command_result(void *data,int size)
{
    usb_drv_send_nonblocking(usb_endpoint, data,size);
    state = SENDING_RESULT;
}

static void receive_block_data(void *data,int size)
{
    usb_drv_recv(usb_endpoint, data, size);
    state = RECEIVING_BLOCKS;
}

static void send_csw(int status)
{
    tb.csw->signature = htole32(CSW_SIGNATURE);
    tb.csw->tag = cur_cmd.tag;
    tb.csw->data_residue = 0;
    tb.csw->status = status;

    usb_drv_send_nonblocking(usb_endpoint, tb.csw,
                             sizeof(struct command_status_wrapper));
    state = SENDING_CSW;
    //logf("CSW: %X",status);

    if(status == UMS_STATUS_GOOD) {
        cur_sense_data.sense_key=0;
        cur_sense_data.information=0;
        cur_sense_data.asc=0;
        cur_sense_data.ascq=0;
    }
}

/* convert ATA IDENTIFY to SCSI INQUIRY */
static void identify2inquiry(int lun)
{
#ifdef HAVE_FLASH_STORAGE
    if(lun==0) {
        memcpy(&tb.inquiry->VendorId,"Rockbox ",8);
        memcpy(&tb.inquiry->ProductId,"Internal Storage",16);
        memcpy(&tb.inquiry->ProductRevisionLevel,"0.00",4);
    }
    else {
        memcpy(&tb.inquiry->VendorId,"Rockbox ",8);
        memcpy(&tb.inquiry->ProductId,"SD Card Slot    ",16);
        memcpy(&tb.inquiry->ProductRevisionLevel,"0.00",4);
    }
#else
    unsigned int i;
    unsigned short* dest;
    unsigned short* src;
    unsigned short* identify = ata_get_identify();
    (void)lun;
    memset(tb.inquiry, 0, sizeof(struct inquiry_data));
    
#if 0
    if (identify[82] & 4)
        tb.inquiry->DeviceTypeModifier = DEVICE_REMOVABLE;
#endif

    /* ATA only has a 'model' field, so we copy the 
       first 8 bytes to 'vendor' and the rest to 'product' (they are
       consecutive in the inquiry struct) */
    src = (unsigned short*)&identify[27];
    dest = (unsigned short*)&tb.inquiry->VendorId;
    for (i=0;i<12;i++)
        dest[i] = htobe16(src[i]);
    
    src = (unsigned short*)&identify[23];
    dest = (unsigned short*)&tb.inquiry->ProductRevisionLevel;
    for (i=0;i<2;i++)
        dest[i] = htobe16(src[i]);
#endif

    tb.inquiry->DeviceType = DIRECT_ACCESS_DEVICE;
    tb.inquiry->AdditionalLength = 0x1f;
    tb.inquiry->Versions = 4; /* SPC-2 */
    tb.inquiry->Format   = 2; /* SPC-2/3 inquiry format */

#if 0
#ifdef HAVE_HOTSWAP
    if(lun>0)
        tb.inquiry->DeviceTypeModifier = DEVICE_REMOVABLE;
#endif
#endif
    /* Mac OSX 10.5 doesn't like this driver if DEVICE_REMOVABLE is not set.
       TODO : this can probably be solved by providing caching mode page */
    tb.inquiry->DeviceTypeModifier = DEVICE_REMOVABLE;
}

#endif /* USB_STORAGE */
