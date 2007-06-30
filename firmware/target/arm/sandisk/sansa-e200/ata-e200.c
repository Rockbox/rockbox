/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Daniel Ankers
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "ata.h"
#include "hotswap-target.h"
#include "ata-target.h"
#include "ata_idle_notify.h"
#include "system.h"
#include <string.h>
#include "thread.h"
#include "led.h"
#include "disk.h"
#include "pp5024.h"
#include "panic.h"

#define BLOCK_SIZE      (512)
#define SECTOR_SIZE     (512)
#define BLOCKS_PER_BANK 0x7a7800

#define STATUS_REG      (*(volatile unsigned int *)(0x70008204))
#define REG_1           (*(volatile unsigned int *)(0x70008208))
#define UNKNOWN         (*(volatile unsigned int *)(0x70008210))
#define BLOCK_SIZE_REG  (*(volatile unsigned int *)(0x7000821c))
#define BLOCK_COUNT_REG (*(volatile unsigned int *)(0x70008220))
#define REG_5           (*(volatile unsigned int *)(0x70008224))
#define CMD_REG0        (*(volatile unsigned int *)(0x70008228))
#define CMD_REG1        (*(volatile unsigned int *)(0x7000822c))
#define CMD_REG2        (*(volatile unsigned int *)(0x70008230))
#define RESPONSE_REG    (*(volatile unsigned int *)(0x70008234))
#define SD_STATE_REG    (*(volatile unsigned int *)(0x70008238))
#define REG_11          (*(volatile unsigned int *)(0x70008240))
#define REG_12          (*(volatile unsigned int *)(0x70008244))
#define DATA_REG        (*(volatile unsigned int *)(0x70008280))

#define DATA_DONE       (1 << 12)
#define CMD_DONE        (1 << 13)
#define ERROR_BITS      (0x3f)
#define READY_FOR_DATA  (1 << 8)
#define FIFO_FULL       (1 << 7)
#define FIFO_EMPTY      (1 << 6)

/* SD States */
#define IDLE            0
#define READY           1
#define IDENT           2
#define STBY            3
#define TRAN            4
#define DATA            5
#define RCV             6
#define PRG             7
#define DIS             8

#define FIFO_SIZE       16          /* FIFO is 16 words deep */

/* SD Commands */
#define GO_IDLE_STATE         0
#define ALL_SEND_CID          2
#define SEND_RELATIVE_ADDR    3
#define SET_DSR               4
#define SWITCH_FUNC           6
#define SELECT_CARD           7
#define DESELECT_CARD         7
#define SEND_CSD              9
#define SEND_CID             10
#define STOP_TRANSMISSION    12
#define SEND_STATUS          13
#define GO_INACTIVE_STATE    15
#define SET_BLOCKLEN         16
#define READ_SINGLE_BLOCK    17
#define READ_MULTIPLE_BLOCK  18
#define SEND_NUM_WR_BLOCKS   22
#define WRITE_BLOCK          24
#define WRITE_MULTIPLE_BLOCK 25
#define ERASE_WR_BLK_START   32
#define ERASE_WR_BLK_END     33
#define ERASE                38
#define APP_CMD              55

#define EC_POWER_UP             1 /* error code */
#define EC_READ_TIMEOUT         2 /* error code */
#define EC_WRITE_TIMEOUT        3 /* error code */
#define EC_TRAN_SEL_BANK        4 /* error code */
#define EC_TRAN_READ_ENTRY      5 /* error code */
#define EC_TRAN_READ_EXIT       6 /* error code */
#define EC_TRAN_WRITE_ENTRY     7 /* error code */
#define EC_TRAN_WRITE_EXIT      8 /* error code */
#define DO_PANIC               32 /* marker     */
#define NO_PANIC                0 /* marker     */
#define EC_COMMAND             10 /* error code */
#define EC_FIFO_SEL_BANK_EMPTY 11 /* error code */
#define EC_FIFO_SEL_BANK_DONE  12 /* error code */
#define EC_FIFO_ENA_BANK_EMPTY 13 /* error code */
#define EC_FIFO_READ_FULL      14 /* error code */
#define EC_FIFO_WR_EMPTY       15 /* error code */
#define EC_FIFO_WR_DONE        16 /* error code */

/* Application Specific commands */
#define SET_BUS_WIDTH   6
#define SD_APP_OP_COND  41

/* for compatibility */
int ata_spinup_time = 0;

long last_disk_activity = -1;

static bool initialized = false;
static int  sd1_status  = 0x00;  /* 0x00:inserted, 0x80:not inserted */

static tSDCardInfo card_info[2];
static tSDCardInfo *currcard; /* current active card */

/* Shoot for around 75% usage */
static long sd_stack [(DEFAULT_STACK_SIZE*2 + 0x1c0)/sizeof(long)];
static const char         sd_thread_name[] = "ata/sd";
static struct mutex       sd_mtx;
static struct event_queue sd_queue;

/* Posted when card plugged status has changed */
#define SD_HOTSWAP    1

/* Private Functions */

static unsigned int check_time[10];

static inline void sd_check_timeout(unsigned int timeout, int id)
{
    if (USEC_TIMER > check_time[id] + timeout)
        panicf("Error SDCard: %d", id);
}

static inline bool sd_poll_status(unsigned int trigger, unsigned int timeout,
                                  int id)
{
    unsigned int t = USEC_TIMER;

    while ((STATUS_REG & trigger) == 0)
    {
        if (USEC_TIMER > t + timeout)
        {
            if(id & DO_PANIC)
                panicf("Error SDCard: %d", id & 31);

            return false;
        }
    }

    return true;
}

static bool sd_command(unsigned int cmd, unsigned long arg1,
                       unsigned int *response, unsigned int type)
{
    int i, words; /* Number of 16 bit words to read from RESPONSE_REG */
    unsigned int data[9];

    while (1)
    {
        CMD_REG0 = cmd;
        CMD_REG1 = (unsigned int)((arg1 & 0xffff0000) >> 16);
        CMD_REG2 = (unsigned int)((arg1 & 0xffff));
        UNKNOWN  = type;

        sd_poll_status(CMD_DONE, 100000, EC_COMMAND | DO_PANIC);

        if ((STATUS_REG & ERROR_BITS) == 0)
            break;

        priority_yield();
    }

    if (cmd == GO_IDLE_STATE)  return true; /* no response here */

    words = (type == 2) ? 9 : 3;

    for (i = 0; i < words; i++) /* RESPONSE_REG is read MSB first */
    {
        data[i] = RESPONSE_REG; /* Read most significant 16-bit word */
    }

    if (type == 2)
    {
        /* Response type 2 has the following structure:
         * [135:135] Start Bit - '0'
         * [134:134] Transmission bit - '0'
         * [133:128] Reserved - '111111'
         * [127:001] CID or CSD register including internal CRC7
         * [000:000] End Bit - '1'
         */
        response[3] = (data[0]<<24) + (data[1]<<8) + ((data[2]&0xff00)>>8);
        response[2] = (data[2]<<24) + (data[3]<<8) + ((data[4]&0xff00)>>8);
        response[1] = (data[4]<<24) + (data[5]<<8) + ((data[6]&0xff00)>>8);
        response[0] = (data[6]<<24) + (data[7]<<8) + ((data[8]&0xff00)>>8);
    }
    else
    {
        /* Response types 1, 1b, 3, 6 have the following structure:
         * Types 4 and 5 are not supported.
         *
         *     [47] Start bit - '0'
         *     [46] Transmission bit - '0'
         *  [45:40] R1, R1b, R6: Command index
         *          R3: Reserved - '111111'
         *   [39:8] R1, R1b: Card Status
         *          R3: OCR Register
         *          R6: [31:16] RCA
         *              [15: 0] Card Status Bits 23, 22, 19, 12:0
         *                     [23] COM_CRC_ERROR
         *                     [22] ILLEGAL_COMMAND
         *                     [19] ERROR
         *                   [12:9] CURRENT_STATE
         *                      [8] READY_FOR_DATA
         *                    [7:6]
         *                      [5] APP_CMD
         *                      [4]
         *                      [3] AKE_SEQ_ERROR
         *                      [2] Reserved
         *                    [1:0] Reserved for test mode
         *    [7:1] R1, R1b: CRC7
         *          R3: Reserved - '1111111'
         *      [0] End Bit - '1'
         */
        response[0] = (data[0]<<24) + (data[1]<<8) + ((data[2]&0xff00)>>8);
    }

    return true;
}

static void sd_wait_for_state(unsigned int state, unsigned int id)
{
    unsigned int response = 0;

    check_time[id] = USEC_TIMER;

    while (1)
    {
        sd_command(SEND_STATUS, currcard->rca, &response, 1);
        sd_check_timeout(0x80000, id);

        if (((response >> 9) & 0xf) == state)
            break;

        priority_yield();
    }

    SD_STATE_REG = state;
}

static inline void copy_read_sectors_fast(unsigned char **buf)
{
    /* Copy one chunk of 16 words using best method for start alignment */
    switch ( (intptr_t)*buf & 3 )
    {
    case 0:
        asm volatile (
            "ldmia  %[data], { r2-r9 }          \r\n"
            "orr    r2, r2, r3, lsl #16         \r\n"
            "orr    r4, r4, r5, lsl #16         \r\n"
            "orr    r6, r6, r7, lsl #16         \r\n"
            "orr    r8, r8, r9, lsl #16         \r\n"
            "stmia  %[buf]!, { r2, r4, r6, r8 } \r\n"
            "ldmia  %[data], { r2-r9 }          \r\n"
            "orr    r2, r2, r3, lsl #16         \r\n"
            "orr    r4, r4, r5, lsl #16         \r\n"
            "orr    r6, r6, r7, lsl #16         \r\n"
            "orr    r8, r8, r9, lsl #16         \r\n"
            "stmia  %[buf]!, { r2, r4, r6, r8 } \r\n"
            : [buf]"+&r"(*buf)
            : [data]"r"(&DATA_REG)
            : "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9"
        );
        break;
    case 1:
        asm volatile (
            "ldmia  %[data], { r2-r9 }          \r\n"
            "orr    r3, r2, r3, lsl #16         \r\n"
            "strb   r3, [%[buf]], #1            \r\n"
            "mov    r3, r3, lsr #8              \r\n"
            "strh   r3, [%[buf]], #2            \r\n"
            "mov    r3, r3, lsr #16             \r\n"
            "orr    r3, r3, r4, lsl #8          \r\n"
            "orr    r3, r3, r5, lsl #24         \r\n"
            "mov    r5, r5, lsr #8              \r\n"
            "orr    r5, r5, r6, lsl #8          \r\n"
            "orr    r5, r5, r7, lsl #24         \r\n"
            "mov    r7, r7, lsr #8              \r\n"
            "orr    r7, r7, r8, lsl #8          \r\n"
            "orr    r7, r7, r9, lsl #24         \r\n"
            "mov    r2, r9, lsr #8              \r\n"
            "stmia  %[buf]!, { r3, r5, r7 }     \r\n"
            "ldmia  %[data], { r3-r10 }         \r\n"
            "orr    r2, r2, r3, lsl #8          \r\n"
            "orr    r2, r2, r4, lsl #24         \r\n"
            "mov    r4, r4, lsr #8              \r\n"
            "orr    r4, r4, r5, lsl #8          \r\n"
            "orr    r4, r4, r6, lsl #24         \r\n"
            "mov    r6, r6, lsr #8              \r\n"
            "orr    r6, r6, r7, lsl #8          \r\n"
            "orr    r6, r6, r8, lsl #24         \r\n"
            "mov    r8, r8, lsr #8              \r\n"
            "orr    r8, r8, r9, lsl #8          \r\n"
            "orr    r8, r8, r10, lsl #24        \r\n"
            "mov    r10, r10, lsr #8            \r\n"
            "stmia  %[buf]!, { r2, r4, r6, r8 } \r\n"
            "strb   r10, [%[buf]], #1           \r\n"
            : [buf]"+&r"(*buf)
            : [data]"r"(&DATA_REG)
            : "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10"
        );
        break;
    case 2:
        asm volatile (
            "ldmia  %[data], { r2-r9 }          \r\n"
            "strh   r2, [%[buf]], #2            \r\n"
            "orr    r3, r3, r4, lsl #16         \r\n"
            "orr    r5, r5, r6, lsl #16         \r\n"
            "orr    r7, r7, r8, lsl #16         \r\n"
            "stmia  %[buf]!, { r3, r5, r7 }     \r\n"
            "ldmia  %[data], { r2-r8, r10 }     \r\n"
            "orr    r2, r9, r2, lsl #16         \r\n"
            "orr    r3, r3, r4, lsl #16         \r\n"
            "orr    r5, r5, r6, lsl #16         \r\n"
            "orr    r7, r7, r8, lsl #16         \r\n"
            "stmia  %[buf]!, { r2, r3, r5, r7 } \r\n"
            "strh   r10, [%[buf]], #2           \r\n"
            : [buf]"+&r"(*buf)
            : [data]"r"(&DATA_REG)
            : "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10"
        );
        break;
    case 3:
        asm volatile (
            "ldmia  %[data], { r2-r9 }          \r\n"
            "orr    r3, r2, r3, lsl #16         \r\n"
            "strb   r3, [%[buf]], #1            \r\n"
            "mov    r3, r3, lsr #8              \r\n"
            "orr    r3, r3, r4, lsl #24         \r\n"
            "mov    r4, r4, lsr #8              \r\n"
            "orr    r5, r4, r5, lsl #8          \r\n"
            "orr    r5, r5, r6, lsl #24         \r\n"
            "mov    r6, r6, lsr #8              \r\n"
            "orr    r7, r6, r7, lsl #8          \r\n"
            "orr    r7, r7, r8, lsl #24         \r\n"
            "mov    r8, r8, lsr #8              \r\n"
            "orr    r2, r8, r9, lsl #8          \r\n"
            "stmia  %[buf]!, { r3, r5, r7 }     \r\n"
            "ldmia  %[data], { r3-r10 }         \r\n"
            "orr    r2, r2, r3, lsl #24         \r\n"
            "mov    r3, r3, lsr #8              \r\n"
            "orr    r4, r3, r4, lsl #8          \r\n"
            "orr    r4, r4, r5, lsl #24         \r\n"
            "mov    r5, r5, lsr #8              \r\n"
            "orr    r6, r5, r6, lsl #8          \r\n"
            "orr    r6, r6, r7, lsl #24         \r\n"
            "mov    r7, r7, lsr #8              \r\n"
            "orr    r8, r7, r8, lsl #8          \r\n"
            "orr    r8, r8, r9, lsl #24         \r\n"
            "mov    r9, r9, lsr #8              \r\n"
            "orr    r10, r9, r10, lsl #8        \r\n"
            "stmia  %[buf]!, { r2, r4, r6, r8 } \r\n"
            "strh   r10, [%[buf]], #2           \r\n"
            "mov    r10, r10, lsr #16           \r\n"
            "strb   r10, [%[buf]], #1           \r\n"
            : [buf]"+&r"(*buf)
            : [data]"r"(&DATA_REG)
            : "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10"
        );
        break;
    }
}

static inline void copy_read_sectors_slow(unsigned char** buf)
{
    int cnt = FIFO_SIZE;
    int t;

    /* Copy one chunk of 16 words */
    asm volatile (
    "1:                                     \r\n"
        "ldrh   %[t], [%[data]]             \r\n"
        "strb   %[t], [%[buf]], #1          \r\n"
        "mov    %[t], %[t], lsr #8          \r\n"
        "strb   %[t], [%[buf]], #1          \r\n"
        "subs   %[cnt], %[cnt], #1          \r\n"
        "bgt    1b                          \r\n"
        : [cnt]"+&r"(cnt), [buf]"+&r"(*buf),
          [t]"=&r"(t)
        : [data]"r"(&DATA_REG)
    );
}

/* Writes have to be kept slow for now */
static inline void copy_write_sectors(const unsigned char* buf)
{
    unsigned short tmp = 0;
    const unsigned char* bufend = buf + FIFO_SIZE*2;

    do
    {
        tmp  = (unsigned short) *buf++;
        tmp |= (unsigned short) *buf++ << 8;
        DATA_REG = tmp;
    } while (buf < bufend); /* tail loop is faster */
}

static void sd_select_bank(unsigned char bank)
{
    unsigned int response;
    unsigned char card_data[512];
    unsigned char* write_buf;
    int i;

    memset(card_data, 0, 512);
    sd_wait_for_state(TRAN, EC_TRAN_SEL_BANK);
    BLOCK_SIZE_REG = 512;
    BLOCK_COUNT_REG = 1;
    sd_command(35, 0, &response, 0x1c0d); /* CMD35 is vendor specific */
    SD_STATE_REG = PRG;

    card_data[0] = bank;

    /* Write the card data */
    write_buf = card_data;
    for (i = 0; i < BLOCK_SIZE / 2; i += FIFO_SIZE)
    {
        /* Wait for the FIFO to empty */
        sd_poll_status(FIFO_EMPTY, 10000, EC_FIFO_SEL_BANK_EMPTY | DO_PANIC);

        copy_write_sectors(write_buf); /* Copy one chunk of 16 words */

        write_buf += FIFO_SIZE*2; /* Advance one chunk of 16 words */
    }

    sd_poll_status(DATA_DONE, 10000, EC_FIFO_SEL_BANK_DONE | DO_PANIC);

    currcard->current_bank = bank;
}

/* lock must already be aquired */
static void sd_init_device(int card_no)
{
/* SD Protocol registers */
    unsigned int  i, dummy;
    unsigned int  c_size = 0;
    unsigned long c_mult = 0;
    unsigned char carddata[512];
    unsigned char *dataptr;

/* Enable and initialise controller */
    REG_1 = 6;

    currcard = &card_info[card_no];

/* Initialise card data as blank */
    memset(currcard, 0, sizeof(*currcard));

    if (card_no == 0)
    {
        outl(inl(0x70000080) | 0x4, 0x70000080);

        GPIOA_ENABLE     &= ~0x7a;
        GPIOA_OUTPUT_EN  &= ~0x7a;
        GPIOD_ENABLE     |=  0x1f;
        GPIOD_OUTPUT_VAL |=  0x1f;
        GPIOD_OUTPUT_EN  |=  0x1f;

        outl((inl(0x70000014) & ~(0x3ffff)) | 0x255aa, 0x70000014);
    }
    else
    {
        outl(inl(0x70000080) & ~0x4, 0x70000080);

        GPIOD_ENABLE     &= ~0x1f;
        GPIOD_OUTPUT_EN  &= ~0x1f;
        GPIOA_ENABLE     |=  0x7a;
        GPIOA_OUTPUT_VAL |=  0x7a;
        GPIOA_OUTPUT_EN  |=  0x7a;

        outl(inl(0x70000014) & ~(0x3ffff), 0x70000014);
    }

    /* Init NAND */
    REG_11 |=  (1 << 15);
    REG_12 |=  (1 << 15);
    REG_12 &= ~(3 << 12);
    REG_12 |=  (1 << 13);
    REG_11 &= ~(3 << 12);
    REG_11 |=  (1 << 13);

    DEV_EN |= DEV_ATA; /* Enable controller */
    DEV_RS |= DEV_ATA; /* Reset controller */
    DEV_RS &=~DEV_ATA; /* Clear Reset */

    SD_STATE_REG = TRAN;

    REG_5 = 0xf;
    sd_command(GO_IDLE_STATE, 0, &dummy, 256);
    check_time[EC_POWER_UP] = USEC_TIMER;
    while ((currcard->ocr & (1 << 31)) == 0) /* until card is powered up */
    {
        sd_command(APP_CMD, currcard->rca, &dummy, 1);
        sd_command(SD_APP_OP_COND, 0x100000, &currcard->ocr, 3);
        sd_check_timeout(5000000, EC_POWER_UP);
    }

    sd_command(ALL_SEND_CID,         0, currcard->cid, 2);
    sd_command(SEND_RELATIVE_ADDR,  0, &currcard->rca, 1);
    sd_command(SEND_CSD, currcard->rca, currcard->csd, 2);

    /* These calculations come from the Sandisk SD card product manual */
    c_size = ((currcard->csd[2] & 0x3ff) << 2) + (currcard->csd[1] >> 30) + 1;
    c_mult = 4 << ((currcard->csd[1] >> 15) & 7);
    currcard->max_read_bl_len = 1 << ((currcard->csd[2] >> 16) & 15);
    currcard->block_size = BLOCK_SIZE;     /* Always use 512 byte blocks */
    currcard->numblocks = c_size * c_mult * (currcard->max_read_bl_len / 512);
    currcard->capacity = currcard->numblocks * currcard->block_size;

    REG_1 = 0;

    sd_command(SELECT_CARD,   currcard->rca       , &dummy, 129);
    sd_command(APP_CMD,       currcard->rca       , &dummy, 1);
    sd_command(SET_BUS_WIDTH, currcard->rca | 2   , &dummy, 1); /* 4 bit */
    sd_command(SET_BLOCKLEN,  currcard->block_size, &dummy, 1);
    BLOCK_SIZE_REG = currcard->block_size;

    /* If this card is > 4Gb, then we need to enable bank switching */
    if(currcard->numblocks >= BLOCKS_PER_BANK)
    {
        SD_STATE_REG = TRAN;
        BLOCK_COUNT_REG = 1;
        sd_command(SWITCH_FUNC, 0x80ffffef, &dummy, 0x1c05);
        /* Read 512 bytes from the card.
        The first 512 bits contain the status information
        TODO: Do something useful with this! */
        dataptr = carddata;
        for (i = 0; i < BLOCK_SIZE / 2; i += FIFO_SIZE)
        {
            /* Wait for the FIFO to be full */
            sd_poll_status(FIFO_FULL, 100000,
                           EC_FIFO_ENA_BANK_EMPTY | DO_PANIC);
            copy_read_sectors_slow(&dataptr);
        }
    }

    currcard->initialized = true;
}

/* API Functions */

void ata_led(bool onoff)
{
    led(onoff);
}

int ata_read_sectors(IF_MV2(int drive,) unsigned long start, int incount,
                     void* inbuf)
{
    int  ret = 0;
    unsigned char *buf, *buf_end;
    unsigned int dummy;
    int bank;
    
    /* TODO: Add DMA support. */

    spinlock_lock(&sd_mtx);

    ata_led(true);

    if (drive != 0 && (GPIOA_INPUT_VAL & 0x80) != 0)
    {
        /* no external sd-card inserted */
        ret = -9;
        goto ata_read_error;
    }

    if (&card_info[drive] != currcard || !card_info[drive].initialized)
        sd_init_device(drive);

    last_disk_activity = current_tick;

    bank = start / BLOCKS_PER_BANK;

    if (currcard->current_bank != bank)
        sd_select_bank(bank);

    start -= bank * BLOCKS_PER_BANK;

    sd_wait_for_state(TRAN, EC_TRAN_READ_ENTRY);
    BLOCK_COUNT_REG = incount;
    sd_command(READ_MULTIPLE_BLOCK, start * BLOCK_SIZE, &dummy, 0x1c25);
    /* TODO: Don't assume BLOCK_SIZE == SECTOR_SIZE */

    buf_end = (unsigned char *)inbuf + incount * currcard->block_size;
    for (buf = inbuf; buf < buf_end;)
    {
        /* Wait for the FIFO to be full */
        sd_poll_status(FIFO_FULL, 0x80000, EC_FIFO_READ_FULL | DO_PANIC);
        copy_read_sectors_fast(&buf); /* Copy one chunk of 16 words */

        /* TODO: Switch bank if necessary */
    }

    last_disk_activity = current_tick;
#if 0
    udelay(75);
#endif
    sd_command(STOP_TRANSMISSION, 0, &dummy, 1);
    sd_wait_for_state(TRAN, EC_TRAN_READ_EXIT);

ata_read_error:
    ata_led(false);

    spinlock_unlock(&sd_mtx);

    return ret;
}

int ata_write_sectors(IF_MV2(int drive,) unsigned long start, int count,
                      const void* outbuf)
{
/* Write support is not finished yet */
/* TODO: The standard suggests using ACMD23 prior to writing multiple blocks
   to improve performance */
    unsigned int response;
    void const* buf, *buf_end;
    int ret = 0;
    int bank;

    spinlock_lock(&sd_mtx);

    ata_led(true);

    if (drive != 0 && (GPIOA_INPUT_VAL & 0x80) != 0)
    {
        /* no external sd-card inserted */
        ret = -9;
        goto error;
    }

    if (&card_info[drive] != currcard || !card_info[drive].initialized)
        sd_init_device(drive);

    bank = start / BLOCKS_PER_BANK;

    if (currcard->current_bank != bank)
        sd_select_bank(bank);

    start -= bank * BLOCKS_PER_BANK;

    check_time[EC_WRITE_TIMEOUT] = USEC_TIMER;
    sd_wait_for_state(TRAN, EC_TRAN_WRITE_ENTRY);
    BLOCK_COUNT_REG = count;
    sd_command(WRITE_MULTIPLE_BLOCK, start * SECTOR_SIZE, &response, 0x1c2d);

    buf_end = outbuf + count * currcard->block_size;
    for (buf = outbuf; buf < buf_end; buf += 2 * FIFO_SIZE)
    {
        if (buf >= buf_end - 2 * FIFO_SIZE)
        {
            /* Set SD_STATE_REG to PRG for the last buffer fill */
            SD_STATE_REG = PRG;
        }

        udelay(2); /* needed here (loop is too fast :-) */

        /* Wait for the FIFO to empty */
        sd_poll_status(FIFO_EMPTY, 0x80000, EC_FIFO_WR_EMPTY | DO_PANIC);

        copy_write_sectors(buf); /* Copy one chunk of 16 words */

        /* TODO: Switch bank if necessary */
    }

    last_disk_activity = current_tick;

    sd_poll_status(DATA_DONE, 0x80000, EC_FIFO_WR_DONE | DO_PANIC);
    sd_check_timeout(0x80000, EC_WRITE_TIMEOUT);

    sd_command(STOP_TRANSMISSION, 0, &response, 1);
    sd_wait_for_state(TRAN, EC_TRAN_WRITE_EXIT);

  error:
    ata_led(false);
    spinlock_unlock(&sd_mtx);

    return ret;
}

static void sd_thread(void) __attribute__((noreturn));
static void sd_thread(void)
{
    struct event ev;
    bool idle_notified = false;
    
    while (1)
    {
        queue_wait_w_tmo(&sd_queue, &ev, HZ);

        switch ( ev.id ) 
        {
        case SD_HOTSWAP:
        {
            int status = 0;
            enum { SD_UNMOUNTED = 0x1, SD_MOUNTED = 0x2 };

            /* Delay on insert and remove to prevent reading state if it is
               just bouncing back and forth while card is sliding - delay on
               insert is also required for the card to stabilize and accept
               commands */
            sleep(HZ/10);

            /* Lock to keep us from messing with this variable while an init
               may be in progress */
            spinlock_lock(&sd_mtx);
            card_info[1].initialized = false;
            spinlock_unlock(&sd_mtx);

            /* Either unmount because the card was pulled or unmount and
               remount if already mounted since multiple messages may be
               generated for the same event - like someone inserting a new
               card before anything detects the old one pulled :) */
            if (disk_unmount(1) != 0)  /* release "by force" */
                status |= SD_UNMOUNTED;

            if (card_detect_target() && disk_mount(1) != 0) /* mount SD-CARD */
                status |= SD_MOUNTED;

            if (status & SD_UNMOUNTED)
                queue_broadcast(SYS_HOTSWAP_EXTRACTED, 0);

            if (status & SD_MOUNTED)
                queue_broadcast(SYS_HOTSWAP_INSERTED, 0);

            if (status)
                queue_broadcast(SYS_FS_CHANGED, 0);
            break;
            } /* SD_HOTSWAP */
        case SYS_TIMEOUT:
            if (TIME_BEFORE(current_tick, last_disk_activity+(3*HZ)))
            {
                idle_notified = false;
            }
            else if (!idle_notified)
            {
                call_ata_idle_notifys(false);
                idle_notified = true;
            }
            break;
        }
    }
}


void ata_spindown(int seconds)
{
    (void)seconds;
}

bool ata_disk_is_active(void)
{
    return 0;
}

void ata_sleep(void)
{
}

void ata_spin(void)
{
}

/* Hardware reset protocol as specified in chapter 9.1, ATA spec draft v5 */
int ata_hard_reset(void)
{
    return 0;
}

int ata_soft_reset(void)
{
    return 0;
}

void ata_enable(bool on)
{
    if(on)
    {
        DEV_EN |= DEV_ATA; /* Enable controller */
    }
    else
    {
        DEV_EN &= ~DEV_ATA; /* Disable controller */
    }
}

int ata_init(void)
{
    ata_led(false);

    /* NOTE: This init isn't dual core safe */
    if (!initialized)
    {
        initialized = true;

        spinlock_init(&sd_mtx);

        spinlock_lock(&sd_mtx);

        /* init controller */
        outl(inl(0x70000088) & ~(0x4), 0x70000088);
        outl(inl(0x7000008c) & ~(0x4), 0x7000008c);
        outl(inl(0x70000084) | 0x4, 0x70000084);
        outl(0x1010, 0x70000034);

        GPIOG_ENABLE     |= (0x3 << 5);
        GPIOG_OUTPUT_EN  |= (0x3 << 5);
        GPIOG_OUTPUT_VAL |= (0x3 << 5);

        /* enable card detection port - mask interrupt first */
        GPIOA_INT_EN     &= ~0x80;

        GPIOA_OUTPUT_EN  &= ~0x80;
        GPIOA_ENABLE     |=  0x80;

        sd_init_device(0);

        queue_init(&sd_queue, true);
        create_thread(sd_thread, sd_stack, sizeof(sd_stack),
            sd_thread_name IF_PRIO(, PRIORITY_SYSTEM) IF_COP(, CPU, false));

        /* enable interupt for the mSD card */
        sleep(HZ/10);

        CPU_INT_EN = HI_MASK;
        CPU_HI_INT_EN = GPIO0_MASK;

        sd1_status = GPIOA_INPUT_VAL & 0x80;
        GPIOA_INT_LEV = (GPIOA_INT_LEV & ~0x80) | (sd1_status ^ 0x80);

        GPIOA_INT_CLR = 0x80;
        GPIOA_INT_EN |= 0x80;

        spinlock_unlock(&sd_mtx);
    }

    return 0;
}

/* move the sd-card info to mmc struct */
tCardInfo *card_get_info_target(int card_no)
{
    int i, temp;
    static tCardInfo card;
    static const char mantissa[] = {  /* *10 */
        0,  10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80 };
    static const int exponent[] = {  /* use varies */
      1,10,100,1000,10000,100000,1000000,10000000,100000000,1000000000 };

    card.initialized  = card_info[card_no].initialized;
    card.ocr          = card_info[card_no].ocr;
    for(i=0; i<4; i++)  card.csd[i] = card_info[card_no].csd[3-i];
    for(i=0; i<4; i++)  card.cid[i] = card_info[card_no].cid[3-i];
    card.numblocks    = card_info[card_no].numblocks;
    card.blocksize    = card_info[card_no].block_size;
    card.size         = card_info[card_no].capacity < 0xffffffff ?
                        card_info[card_no].capacity : 0xffffffff;
    card.block_exp    = card_info[card_no].block_exp;
    temp              = card_extract_bits(card.csd, 29, 3);
    card.speed        = mantissa[card_extract_bits(card.csd, 25, 4)]
                      * exponent[temp > 2 ? 7 : temp + 4];
    card.nsac         = 100 * card_extract_bits(card.csd, 16, 8);
    temp              = card_extract_bits(card.csd, 13, 3);
    card.tsac         = mantissa[card_extract_bits(card.csd, 9, 4)]
                      * exponent[temp] / 10;
    card.cid[0]       = htobe32(card.cid[0]); /* ascii chars here */
    card.cid[1]       = htobe32(card.cid[1]); /* ascii chars here */
    temp = *((char*)card.cid+13); /* adjust year<=>month, 1997 <=> 2000 */
    *((char*)card.cid+13) = (unsigned char)((temp >> 4) | (temp << 4)) + 3;

    return &card;
}

bool card_detect_target(void)
{
    /* 0x00:inserted, 0x80:not inserted */
    return (GPIOA_INPUT_VAL & 0x80) == 0;
}

/* called on insertion/removal interrupt */
void microsd_int(void)
{
    int status = GPIOA_INPUT_VAL & 0x80;
    
    GPIOA_INT_LEV = (GPIOA_INT_LEV & ~0x80) | (status ^ 0x80);
    GPIOA_INT_CLR = 0x80;

    if (status == sd1_status)
        return;

    sd1_status = status;

    /* Take final state only - insert/remove is bouncy */
    queue_remove_from_head(&sd_queue, SD_HOTSWAP);
    queue_post(&sd_queue, SD_HOTSWAP, status);
}
