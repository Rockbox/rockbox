/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 by Jens Arnold
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdbool.h>
#include "ata.h"
#include "ata_mmc.h"
#include "kernel.h"
#include "thread.h"
#include "led.h"
#include "sh7034.h"
#include "system.h"
#include "debug.h"
#include "panic.h"
#include "usb.h"
#include "power.h"
#include "string.h"
#include "hwcompat.h"
#include "adc.h"

#include "bitswap.h"

#define SECTOR_SIZE     512

/* Command definitions */
#define CMD_GO_IDLE_STATE        0x40  /* R1 */
#define CMD_SEND_OP_COND         0x41  /* R1 */
#define CMD_SEND_CSD             0x49  /* R1 */
#define CMD_SEND_CID             0x4a  /* R1 */
#define CMD_STOP_TRANSMISSION    0x4c  /* R1 */
#define CMD_SEND_STATUS          0x4d  /* R2 */
#define CMD_READ_SINGLE_BLOCK    0x51  /* R1 */
#define CMD_READ_MULTIPLE_BLOCK  0x52  /* R1 */
#define CMD_WRITE_BLOCK          0x58  /* R1b */
#define CMD_WRITE_MULTIPLE_BLOCK 0x59  /* R1b */
#define CMD_READ_OCR             0x7a  /* R3 */

/* Response formats:
   R1  = single byte, msb=0, various error flags
   R1b = R1 + busy token(s)
   R2  = 2 bytes (1st byte identical to R1), additional flags
   R3  = 5 bytes (R1 + OCR register)
*/

#define R1_PARAMETER_ERR 0x40
#define R1_ADDRESS_ERR   0x20
#define R1_ERASE_SEQ_ERR 0x10
#define R1_COM_CRC_ERR   0x08
#define R1_ILLEGAL_CMD   0x04
#define R1_ERASE_RESET   0x02
#define R1_IN_IDLE_STATE 0x01

#define R2_OUT_OF_RANGE  0x80
#define R2_ERASE_PARAM   0x40
#define R2_WP_VIOLATION  0x20
#define R2_CARD_ECC_FAIL 0x10
#define R2_CC_ERROR      0x08
#define R2_ERROR         0x04
#define R2_ERASE_SKIP    0x02
#define R2_CARD_LOCKED   0x01

/* Data start tokens */

#define DT_START_BLOCK          0xfe
#define DT_START_WRITE_MULTIPLE 0xfc
#define DT_STOP_TRAN            0xfd

// DEBUG
#include "../../apps/screens.h"

/* for compatibility */
bool old_recorder = false; /* FIXME: get rid of this cross-dependency */
int ata_spinup_time = 0;
char ata_device = 0; /* device 0 (master) or 1 (slave) */
int ata_io_address = 0; /* 0x300 or 0x200, only valid on recorder */
long last_disk_activity = -1;

/* private variables */

static struct mutex mmc_mutex;

static char mmc_stack[DEFAULT_STACK_SIZE];
static const char mmc_thread_name[] = "mmc";
static struct event_queue mmc_queue;
static bool initialized = false;
static bool delayed_write = false;
static unsigned char delayed_sector[SECTOR_SIZE];
static int delayed_sector_num;

static int current_card = 0;

static const unsigned char dummy[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static tCardInfo card_info[2];

/* private function declarations */

static int select_card(int card_no);
static void deselect_card(void);
static void setup_sci1(int bitrate_register);
static void write_transfer(const unsigned char *buf, int len)
            __attribute__ ((section(".icode")));
static void read_transfer(unsigned char *buf, int len)
            __attribute__ ((section(".icode")));
static unsigned char poll_byte(int timeout);
static unsigned char poll_busy(int timeout);
static int send_cmd(int cmd, unsigned long parameter, unsigned char *response);
static int receive_data(unsigned char *buf, int len, int timeout);
static int send_data(char start_token, const unsigned char *buf, int len,
                     int timeout);
static int initialize_card(int card_no);

/* implementation */

static int select_card(int card_no)
{
    if (card_no == 0)             /* internal */
        or_b(0x10, &PADRH);       /* set clock gate PA12  CHECKME: mask? */
    else                          /* external */
        and_b(~0x10, &PADRH);     /* clear clock gate PA12  CHECKME: mask?*/
        
    last_disk_activity = current_tick;

    if (!card_info[card_no].initialized)
    {
        setup_sci1(7); /* Initial rate: 375 kbps (need <= 400 per mmc specs) */
        write_transfer(dummy, 10); /* allow the card to synchronize */
        while (!(SSR1 & SCI_TEND));
    }

    if (card_no == 0)             /* internal */
        and_b(~0x04, &PADRH);     /* assert CS */
    else                          /* external */
        and_b(~0x02, &PADRH);     /* assert CS */

    if (card_info[card_no].initialized)
    {
        setup_sci1(card_info[card_no].bitrate_register);
        return 0;
    }
    else
    {
        return initialize_card(card_no);
    }
}

static void deselect_card(void)
{
    while (!(SSR1 & SCI_TEND));   /* wait for end of transfer */
    or_b(0x06, &PADRH);           /* deassert CS (both cards) */

    last_disk_activity = current_tick;
}

static void setup_sci1(int bitrate_register)
{
    int i;

    while (!(SSR1 & SCI_TEND));   /* wait for end of transfer */

    SCR1 = 0;                     /* disable serial port */
    SMR1 = SYNC_MODE;             /* no prescale */
    BRR1 = bitrate_register;
    SCR1 = SCI_CKE0;
    SSR1 = 0;

    for (i = 0; i <= bitrate_register; i++); /* wait at least one bit time */
    
    or_b((SCI_TE|SCI_RE), &SCR1); /* enable transmitter & receiver */
}

static void write_transfer(const unsigned char *buf, int len)
{
    const unsigned char *buf_end = buf + len;
    register unsigned char data;

    /* TODO: DMA */
    
    while (buf < buf_end)
    {
        data = fliptable[(signed char)(*buf++)]; /* bitswap */
        while (!(SSR1 & SCI_TEND));              /* wait for end of transfer */
        TDR1 = data;                             /* write byte */
        SSR1 = 0;                                /* start transmitting */
    }
}

/* don't call this with len == 0 */
static void read_transfer(unsigned char *buf, int len)
{
    unsigned char *buf_end = buf + len - 1;
    register signed char data;

    /* TODO: DMA */

    while (!(SSR1 & SCI_TEND));   /* wait for end of transfer */
    TDR1 = 0xFF;                  /* send do-nothing data in parallel */
    
    SSR1 = 0;                     /* start receiving first byte */
    while (buf < buf_end)
    {
        while (!(SSR1 & SCI_RDRF)); /* wait for data */
        data = RDR1;                /* read byte */
        SSR1 = 0;                   /* start receiving */
        *buf++ = fliptable[data];   /* bitswap */
    }
    while (!(SSR1 & SCI_RDRF));     /* wait for last byte */
    *buf = fliptable[(signed char)(RDR1)]; /* read & bitswap */
}

static unsigned char poll_byte(int timeout) /* timeout is in bytes */
{
    int i;
    unsigned char data = 0;       /* stop the compiler complaining */

    while (!(SSR1 & SCI_TEND));   /* wait for end of transfer */
    TDR1 = 0xFF;                  /* send do-nothing data in parallel */

    i = 0;
    do {
        SSR1 = 0;                   /* start receiving */
        while (!(SSR1 & SCI_RDRF)); /* wait for data */
        data = RDR1;                /* read byte */
    } while ((data == 0xFF) && (++i < timeout));

    return fliptable[(signed char)data];
}

static unsigned char poll_busy(int timeout) /* timeout is in bytes */
{
    int i;
    unsigned char data, dummy;
    
    while (!(SSR1 &SCI_TEND));    /* wait for end of transfer */
    TDR1 = 0xFF;                  /* send do-nothing data in parallel */

    /* get data response */
    SSR1 = 0;                     /* start receiving */
    while (!(SSR1 & SCI_RDRF));   /* wait for data */
    data = fliptable[(signed char)(RDR1)];  /* read byte */

    /* wait until the card is ready again */
    i = 0;
    do {
        SSR1 = 0;                   /* start receiving */
        while (!(SSR1 & SCI_RDRF)); /* wait for data */
        dummy = RDR1;               /* read byte */
    } while ((dummy != 0xFF) && (++i < timeout));
    
    return data;
}

static int send_cmd(int cmd, unsigned long parameter, unsigned char *response)
{
    unsigned char command[] = {0x40, 0x00, 0x00, 0x00, 0x00, 0x95, 0xFF};

    command[0] = cmd;
    
    if (parameter != 0)
    {
        command[1] = (parameter >> 24) & 0xFF;
        command[2] = (parameter >> 16) & 0xFF;
        command[3] = (parameter >> 8) & 0xFF;
        command[4] = parameter & 0xFF;
    }
    
    write_transfer(command, 7);

    response[0] = poll_byte(20);
    
    if (response[0] != 0x00)
    {
        write_transfer(dummy, 1);
        return -1;
    }

    switch (cmd)
    {
        case CMD_SEND_CSD:        /* R1 response, leave open */
        case CMD_SEND_CID:
        case CMD_READ_SINGLE_BLOCK:
        case CMD_READ_MULTIPLE_BLOCK:
            break;
            
        case CMD_SEND_STATUS:     /* R2 response, close with dummy */
            read_transfer(response + 1, 1);
            write_transfer(dummy, 1);
            break;
            
        case CMD_READ_OCR:        /* R3 response, close with dummy */
            read_transfer(response + 1, 4);
            write_transfer(dummy, 1);
            break;

        default:                  /* R1 response, close with dummy */
            write_transfer(dummy, 1);
            break;                /* also catches block writes */
    }

    return 0;
}

static int receive_data(unsigned char *buf, int len, int timeout)
{
    unsigned char crc[2];         /* unused */

    if (poll_byte(timeout) != DT_START_BLOCK)
    {
        write_transfer(dummy, 1);
        return -1;                /* not start of data */
    }
        
    read_transfer(buf, len);
    read_transfer(crc, 2);        /* throw away */
    write_transfer(dummy, 1);

    return 0;
}

static int send_data(char start_token, const unsigned char *buf, int len,
                     int timeout)
{
    int ret = 0;

    write_transfer(&start_token, 1);
    write_transfer(buf, len);
    write_transfer(dummy, 2);     /* crc - dontcare */

    if ((poll_busy(timeout) & 0x1F) != 0x05) /* something went wrong */
        ret = -1;

    write_transfer(dummy, 1);
    
    return ret;
}

/* helper function to extract n (<=32) bits from an arbitrary position.
   counting from MSB to LSB */
unsigned long mmc_extract_bits(
    const unsigned long *p, /* the start of the bitfield array */
    unsigned int start,     /* bit no. to start reading  */
    unsigned int size)      /* how many bits to read */
{
    unsigned int bit_index;
    unsigned int bits_to_use;
    unsigned long mask;
    unsigned long result;

    if (size == 1)
    {   /* short cut */
        return ((p[start/32] >> (31 - (start % 32))) & 1);
    }

    result = 0;
    while (size)
    {
        bit_index = start % 32;
        bits_to_use = MIN(32 - bit_index, size);
        mask = 0xFFFFFFFF >> (32 - bits_to_use);

        result <<= bits_to_use; /* start last round */
        result |= (p[start/32] >> (32 - bits_to_use - bit_index)) & mask;

        start += bits_to_use;
        size -= bits_to_use;
    }

    return result;
}

static int initialize_card(int card_no)
{
    int i, temp;
    unsigned char response[5];
    tCardInfo *card = &card_info[card_no];

    static const char mantissa[] = {  /* *10 */
        0,  10, 12, 13, 15, 20, 25, 30,
        35, 40, 45, 50, 55, 60, 70, 80
    };
    static const int exponent[] = {  /* use varies */
        1, 10, 100, 1000, 10000, 100000, 1000000,
        10000000, 100000000, 1000000000
    };

    /* switch to SPI mode */
    send_cmd(CMD_GO_IDLE_STATE, 0, response);
    if (response[0] != 0x01)
        return -1;                /* error response */

    /* initialize card */
    i = 0;
    while (send_cmd(CMD_SEND_OP_COND, 0, response) && (++i < 200));
    if (response[0] != 0x00)
        return -2;                /* not ready */
        
    /* get OCR register */
    if (send_cmd(CMD_READ_OCR, 0, response))
        return -3;
    card->ocr = (response[1] << 24) + (response[2] << 16)
              + (response[3] << 8) + response[4];
        
    /* check voltage */
    if (!(card->ocr & 0x00100000)) /* 3.2 .. 3.3 V */
        return -4;
    
    /* get CSD register */
    if (send_cmd(CMD_SEND_CSD, 0, response))
        return -5;
    if (receive_data((unsigned char*)card->csd, 16, 20))
        return -6;

    /* check block size */
    if ((1 << mmc_extract_bits(card->csd, 44, 4)) != SECTOR_SIZE)
        return -7;

    /* max transmission speed, clock divider */
    temp = mmc_extract_bits(card->csd, 29, 3);
    temp = (temp > 3) ? 3 : temp;
    card->speed = mantissa[mmc_extract_bits(card->csd, 25, 4)]
                * exponent[temp + 4];
    card->bitrate_register = (FREQ/4-1) / card->speed;

    /* NSAC, TSAC, read timeout */
    card->nsac = 100 * mmc_extract_bits(card->csd, 16, 8);
    card->tsac = mantissa[mmc_extract_bits(card->csd, 9, 4)];
    temp = mmc_extract_bits(card->csd, 13, 3);
    card->read_timeout = ((FREQ/4) / (card->bitrate_register + 1)
                         * card->tsac / exponent[9 - temp]
                         + (10 * card->nsac));
    card->read_timeout /= 8;      /* clocks -> bytes */
    card->tsac *= exponent[temp];

    /* r2w_factor, write timeout */
    temp = mmc_extract_bits(card->csd, 99, 3);
    temp = (temp > 5) ? 5 : temp;
    card->r2w_factor = 1 << temp;
    card->write_timeout = card->read_timeout * card->r2w_factor;

    /* switch to full speed */
    setup_sci1(card->bitrate_register);
    
    /* get CID register */
    if (send_cmd(CMD_SEND_CID, 0, response))
        return -8;
    if (receive_data((unsigned char*)card->cid, 16, 20))
        return -9;

    card->initialized = true;
    return 0;
}

tCardInfo *mmc_card_info(int card_no)
{
    tCardInfo *card = &card_info[card_no];
    
    if (!card->initialized)
    {
        select_card(card_no);
        deselect_card();
    }
    return card;
}

int ata_read_sectors(unsigned long start,
                     int incount,
                     void* inbuf)
{
    int ret = 0;
    int i;
    unsigned long addr;
    unsigned char response;
    tCardInfo *card = &card_info[current_card];
    
    addr = start * SECTOR_SIZE;

    mutex_lock(&mmc_mutex);
    ret = select_card(current_card);

    if (ret == 0)
    {
        if (incount == 1)
        {   
            ret = send_cmd(CMD_READ_SINGLE_BLOCK, addr, &response);
            if (ret == 0)
                ret = receive_data(inbuf, SECTOR_SIZE, card->read_timeout);
                last_disk_activity = current_tick;
        }
        else
        {
            ret = send_cmd(CMD_READ_MULTIPLE_BLOCK, addr, &response);
            for (i = 0; (i < incount) && (ret == 0); i++)
            {
                ret = receive_data(inbuf, SECTOR_SIZE, card->read_timeout);
                inbuf += SECTOR_SIZE;
                last_disk_activity = current_tick;
            }
            if (ret == 0)
                ret = send_cmd(CMD_STOP_TRANSMISSION, 0, &response);
        }
    }

    deselect_card();
    mutex_unlock(&mmc_mutex);

    /* only flush if reading went ok */
    if ( (ret == 0) && delayed_write )
        ata_flush();

    return ret;
}

int ata_write_sectors(unsigned long start,
                      int count,
                      const void* buf)
{
    int ret = 0;
    int i;
    unsigned long addr;
    unsigned char response;
    tCardInfo *card = &card_info[current_card];

    if (start == 0)
        panicf("Writing on sector 0\n");

    addr = start * SECTOR_SIZE;
    
    mutex_lock(&mmc_mutex);
    ret = select_card(current_card);

    if (ret == 0)
    {
        if (count == 1)
        {
            ret = send_cmd(CMD_WRITE_BLOCK, addr, &response);
            if (ret == 0)
                ret = send_data(DT_START_BLOCK, buf, SECTOR_SIZE,
                                card->write_timeout);
            last_disk_activity = current_tick;
        }
        else
        {
            ret = send_cmd(CMD_WRITE_MULTIPLE_BLOCK, addr, &response);
            for (i = 0; (i < count) && (ret == 0); i++)
            {
                ret = send_data(DT_START_WRITE_MULTIPLE, buf, SECTOR_SIZE,
                                card->write_timeout);
                buf += SECTOR_SIZE;
                last_disk_activity = current_tick;
            }
            if (ret == 0)
            {
                response = DT_STOP_TRAN;
                write_transfer(&response, 1);
                poll_busy(card->write_timeout);
                last_disk_activity = current_tick;
            }
        }
    }

    deselect_card();
    mutex_unlock(&mmc_mutex);

    /* only flush if writing went ok */
    if ( (ret == 0) && delayed_write )
        ata_flush();

    return ret;
}

/* While there is no spinup, the delayed write is still here to avoid
   wearing the flash unnecessarily */
extern void ata_delayed_write(unsigned long sector, const void* buf)
{
    memcpy(delayed_sector, buf, SECTOR_SIZE);
    delayed_sector_num = sector;
    delayed_write = true;
}

extern void ata_flush(void)
{
    if ( delayed_write ) {
        DEBUGF("ata_flush()\n");
        delayed_write = false;
        ata_write_sectors(delayed_sector_num, 1, delayed_sector);
    }
}

void ata_spindown(int seconds)
{
    (void)seconds;
}

bool ata_disk_is_active(void)
{   
    /* this is correct unless early return from write gets implemented */
    return mmc_mutex.locked;
}

int ata_standby(int time)
{
    (void)time;

    return 0;
}

int ata_sleep(void)
{
    return 0;
}

void ata_spin(void)
{
}

static void mmc_thread(void)
{
    struct event ev;

    while (1) {
        while ( queue_empty( &mmc_queue ) ) {

            sleep(HZ/4);
        }
        queue_wait(&mmc_queue, &ev);
        switch ( ev.id ) {
#ifndef USB_NONE
            case SYS_USB_CONNECTED:
                /* Tell the USB thread that we are safe */
                DEBUGF("mmc_thread got SYS_USB_CONNECTED\n");
                usb_acknowledge(SYS_USB_CONNECTED_ACK);

                /* Wait until the USB cable is extracted again */
                usb_wait_for_disconnect(&mmc_queue);
                break;
#endif
        }
    }
}

int ata_soft_reset(void)
{
    int ret = 0;

    return ret;
}

void ata_enable(bool on)
{
    PBCR1 &= ~0x0CF0; /* PB13, PB11 and PB10 become GPIOs, if not modified below */
    PACR2 &= ~0x4000; /* use PA7 (bridge reset) as GPIO */
    if (on)
    {
        PBCR1 |= 0x08A0;    /* as SCK1, TxD1, RxD1 */
        IPRE &= 0x0FFF;     /* disable SCI1 interrupts for the CPU */
    }
    and_b(~0x80, &PADRL); /* assert reset */
    sleep(HZ/20);
    or_b(0x80, &PADRL);   /* de-assert reset */
    sleep(HZ/20);
    card_info[0].initialized = false;
    card_info[1].initialized = false;
}

int ata_init(void)
{
    int rc = 0;

    mutex_init(&mmc_mutex);

    led(false);

    /* Port setup */
    PACR1 &= ~0x0F00; /* GPIO function for PA12, /IRQ1 for PA13 */
    PACR1 |= 0x0400;
    PADR |= 0x0680;   /* set all the selects + reset high (=inactive) */
    PAIOR |= 0x1680;  /* make outputs for them and the PA12 clock gate */

    PBDR |= 0x2C00;   /* SCK1, TxD1 and RxD1 high when GPIO  CHECKME: mask */
    PBIOR |= 0x2000;  /* SCK1 output */
    PBIOR &= ~0x0C00; /* TxD1, RxD1 input */

    if(adc_read(ADC_MMC_SWITCH) < 0x200)
    {   /* MMC inserted */
        current_card = 1;
    }
    else
    {   /* no MMC, use internal memory */
        current_card = 0;
    }

    ata_enable(true);

    if ( !initialized ) {

        queue_init(&mmc_queue);

        create_thread(mmc_thread, mmc_stack,
                      sizeof(mmc_stack), mmc_thread_name);
        initialized = true;
    }

    return rc;
}

