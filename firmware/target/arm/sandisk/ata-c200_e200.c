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
#include "fat.h"
#include "hotswap-target.h"
#include "ata-target.h"
#include "ata_idle_notify.h"
#include "system.h"
#include <string.h>
#include "thread.h"
#include "led.h"
#include "disk.h"
#include "cpu.h"
#include "panic.h"
#include "usb.h"

#define BLOCK_SIZE      512
#define SECTOR_SIZE     512
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

/* STATUS_REG bits */
#define DATA_DONE       (1 << 12)
#define CMD_DONE        (1 << 13)
#define ERROR_BITS      (0x3f)
#define READY_FOR_DATA  (1 << 8)
#define FIFO_FULL       (1 << 7)
#define FIFO_EMPTY      (1 << 6)

#define CMD_OK          0x0 /* Command was successful */
#define CMD_ERROR_2     0x2 /* SD did not respond to command (either it doesn't
                               understand the command or is not inserted) */

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

#define FIFO_LEN        16          /* FIFO is 16 words deep */

/* SD Commands */
#define GO_IDLE_STATE         0
#define ALL_SEND_CID          2
#define SEND_RELATIVE_ADDR    3
#define SET_DSR               4
#define SWITCH_FUNC           6
#define SELECT_CARD           7
#define DESELECT_CARD         7
#define SEND_IF_COND          8
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

#define EC_OK                    0
#define EC_FAILED                1
#define EC_NOCARD                2
#define EC_WAIT_STATE_FAILED     3
#define EC_CHECK_TIMEOUT_FAILED  4
#define EC_POWER_UP              5
#define EC_READ_TIMEOUT          6
#define EC_WRITE_TIMEOUT         7
#define EC_TRAN_SEL_BANK         8
#define EC_TRAN_READ_ENTRY       9
#define EC_TRAN_READ_EXIT       10
#define EC_TRAN_WRITE_ENTRY     11
#define EC_TRAN_WRITE_EXIT      12
#define EC_FIFO_SEL_BANK_EMPTY  13
#define EC_FIFO_SEL_BANK_DONE   14
#define EC_FIFO_ENA_BANK_EMPTY  15
#define EC_FIFO_READ_FULL       16
#define EC_FIFO_WR_EMPTY        17
#define EC_FIFO_WR_DONE         18
#define EC_COMMAND              19
#define NUM_EC                  20

/* Application Specific commands */
#define SET_BUS_WIDTH   6
#define SD_APP_OP_COND  41

/** global, exported variables **/
#ifdef HAVE_HOTSWAP
#define NUM_VOLUMES 2
#else
#define NUM_VOLUMES 1
#endif

/* for compatibility */
int ata_spinup_time = 0;

long last_disk_activity = -1;

/** static, private data **/ 
static bool initialized = false;

static long next_yield = 0;
#define MIN_YIELD_PERIOD 1000

static tSDCardInfo card_info[2];
static tSDCardInfo *currcard = NULL; /* current active card */

struct sd_card_status
{
    int retry;
    int retry_max;
};

static struct sd_card_status sd_status[NUM_VOLUMES] =
{
    { 0, 1  },
#ifdef HAVE_HOTSWAP
    { 0, 10 }
#endif
};

/* Shoot for around 75% usage */
static long sd_stack [(DEFAULT_STACK_SIZE*2 + 0x1c0)/sizeof(long)];
static const char         sd_thread_name[] = "ata/sd";
static struct mutex       sd_mtx NOCACHEBSS_ATTR;
static struct event_queue sd_queue;

/* Posted when card plugged status has changed */
#define SD_HOTSWAP    1
/* Actions taken by sd_thread when card status has changed */
enum sd_thread_actions
{
    SDA_NONE      = 0x0,
    SDA_UNMOUNTED = 0x1,
    SDA_MOUNTED   = 0x2
};

/* Private Functions */

static unsigned int check_time[NUM_EC];

static inline bool sd_check_timeout(long timeout, int id)
{
    return !TIME_AFTER(USEC_TIMER, check_time[id] + timeout);
}

static bool sd_poll_status(unsigned int trigger, long timeout)
{
    long t = USEC_TIMER;

    while ((STATUS_REG & trigger) == 0)
    {
        long time = USEC_TIMER;

        if (TIME_AFTER(time, next_yield))
        {
            long ty = USEC_TIMER;
            priority_yield();
            timeout += USEC_TIMER - ty;
            next_yield = ty + MIN_YIELD_PERIOD;
        }

        if (TIME_AFTER(time, t + timeout))
            return false;
    }

    return true;
}

static int sd_command(unsigned int cmd, unsigned long arg1,
                      unsigned int *response, unsigned int type)
{
    int i, words; /* Number of 16 bit words to read from RESPONSE_REG */
    unsigned int data[9];

    CMD_REG0 = cmd;
    CMD_REG1 = (unsigned int)((arg1 & 0xffff0000) >> 16);
    CMD_REG2 = (unsigned int)((arg1 & 0xffff));
    UNKNOWN  = type;

    if (!sd_poll_status(CMD_DONE, 100000))
        return -EC_COMMAND;

    if ((STATUS_REG & ERROR_BITS) != CMD_OK)
        /* Error sending command */
        return -EC_COMMAND - (STATUS_REG & ERROR_BITS)*100;

    if (cmd == GO_IDLE_STATE)
        return 0; /* no response here */

    words = (type == 2) ? 9 : 3;

    for (i = 0; i < words; i++) /* RESPONSE_REG is read MSB first */
        data[i] = RESPONSE_REG; /* Read most significant 16-bit word */

    if (response == NULL)
    {
        /* response discarded */
    }
    else if (type == 2)
    {
        /* Response type 2 has the following structure:
         * [135:135] Start Bit - '0'
         * [134:134] Transmission bit - '0'
         * [133:128] Reserved - '111111'
         * [127:001] CID or CSD register including internal CRC7
         * [000:000] End Bit - '1'
         */
        response[3] = (data[0]<<24) + (data[1]<<8) + (data[2]>>8);
        response[2] = (data[2]<<24) + (data[3]<<8) + (data[4]>>8);
        response[1] = (data[4]<<24) + (data[5]<<8) + (data[6]>>8);
        response[0] = (data[6]<<24) + (data[7]<<8) + (data[8]>>8);
    }
    else
    {
        /* Response types 1, 1b, 3, 6, 7 have the following structure:
         * Types 4 and 5 are not supported.
         *
         *     [47] Start bit - '0'
         *     [46] Transmission bit - '0'
         *  [45:40] R1, R1b, R6, R7: Command index
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
         *          R7: [19:16] Voltage accepted
         *              [15:8]  echo-back of check pattern
         *    [7:1] R1, R1b: CRC7
         *          R3: Reserved - '1111111'
         *      [0] End Bit - '1'
         */
        response[0] = (data[0]<<24) + (data[1]<<8) + (data[2]>>8);
    }

    return 0;
}

static int sd_wait_for_state(unsigned int state, int id)
{
    unsigned int response = 0;
    unsigned int timeout = 0x80000;

    check_time[id] = USEC_TIMER;

    while (1)
    {
        int ret = sd_command(SEND_STATUS, currcard->rca, &response, 1);
        long us;

        if (ret < 0)
            return ret*100 - id;

        if (((response >> 9) & 0xf) == state)
        {
            SD_STATE_REG = state;
            return 0;
        }

        if (!sd_check_timeout(timeout, id))
            return -EC_WAIT_STATE_FAILED*100 - id;

        us = USEC_TIMER;
        if (TIME_AFTER(us, next_yield))
        {
            priority_yield();
            timeout += USEC_TIMER - us;
            next_yield = us + MIN_YIELD_PERIOD;
        }
    }
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
    int cnt = FIFO_LEN;
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
static inline void copy_write_sectors(const unsigned char** buf)
{
    int cnt = FIFO_LEN;
    unsigned t;

    do
    {
        t  = *(*buf)++;
        t |= *(*buf)++ << 8;
        DATA_REG = t;
    } while (--cnt > 0); /* tail loop is faster */
}

static int sd_select_bank(unsigned char bank)
{
    unsigned char card_data[512];
    const unsigned char* write_buf;
    int i, ret;

    memset(card_data, 0, 512);

    ret = sd_wait_for_state(TRAN, EC_TRAN_SEL_BANK);
    if (ret < 0)
        return ret;

    BLOCK_SIZE_REG = 512;
    BLOCK_COUNT_REG = 1;

    ret = sd_command(35, 0, NULL, 0x1c0d); /* CMD35 is vendor specific */
    if (ret < 0)
        return ret;

    SD_STATE_REG = PRG;

    card_data[0] = bank;

    /* Write the card data */
    write_buf = card_data;
    for (i = 0; i < BLOCK_SIZE/2; i += FIFO_LEN)
    {
        /* Wait for the FIFO to empty */
        if (sd_poll_status(FIFO_EMPTY, 10000))
        {
            copy_write_sectors(&write_buf); /* Copy one chunk of 16 words */
            continue;
        }

        return -EC_FIFO_SEL_BANK_EMPTY;
    }

    if (!sd_poll_status(DATA_DONE, 10000))
        return -EC_FIFO_SEL_BANK_DONE;

    currcard->current_bank = bank;

    return 0;
}

static void sd_card_mux(int card_no)
{
/* Set the current card mux */
#ifdef SANSA_E200
    if (card_no == 0)
    {
        GPO32_VAL |= 0x4;

        GPIO_CLEAR_BITWISE(GPIOA_ENABLE, 0x7a);
        GPIO_CLEAR_BITWISE(GPIOA_OUTPUT_EN, 0x7a);
        GPIO_SET_BITWISE(GPIOD_ENABLE,  0x1f);
        GPIO_SET_BITWISE(GPIOD_OUTPUT_VAL, 0x1f);
        GPIO_SET_BITWISE(GPIOD_OUTPUT_EN,  0x1f);

        outl((inl(0x70000014) & ~(0x3ffff)) | 0x255aa, 0x70000014);
    }
    else
    {
        GPO32_VAL &= ~0x4;

        GPIO_CLEAR_BITWISE(GPIOD_ENABLE, 0x1f);
        GPIO_CLEAR_BITWISE(GPIOD_OUTPUT_EN, 0x1f);
        GPIO_SET_BITWISE(GPIOA_ENABLE, 0x7a);
        GPIO_SET_BITWISE(GPIOA_OUTPUT_VAL, 0x7a);
        GPIO_SET_BITWISE( GPIOA_OUTPUT_EN, 0x7a);

        outl(inl(0x70000014) & ~(0x3ffff), 0x70000014);
    }
#else /* SANSA_C200 */
    if (card_no == 0)
    {
        GPO32_VAL |= 0x4;

        GPIO_CLEAR_BITWISE(GPIOD_ENABLE, 0x1f);
        GPIO_CLEAR_BITWISE(GPIOD_OUTPUT_EN, 0x1f);
        GPIO_SET_BITWISE(GPIOA_ENABLE, 0x7a);
        GPIO_SET_BITWISE(GPIOA_OUTPUT_VAL, 0x7a);
        GPIO_SET_BITWISE( GPIOA_OUTPUT_EN, 0x7a);

        outl(inl(0x70000014) & ~(0x3ffff), 0x70000014);
    }
    else
    {
        GPO32_VAL &= ~0x4;

        GPIO_CLEAR_BITWISE(GPIOA_ENABLE, 0x7a);
        GPIO_CLEAR_BITWISE(GPIOA_OUTPUT_EN, 0x7a);
        GPIO_SET_BITWISE(GPIOD_ENABLE,  0x1f);
        GPIO_SET_BITWISE(GPIOD_OUTPUT_VAL, 0x1f);
        GPIO_SET_BITWISE(GPIOD_OUTPUT_EN,  0x1f);

        outl((inl(0x70000014) & ~(0x3ffff)) | 0x255aa, 0x70000014);
    }
#endif
}

static void sd_init_device(int card_no)
{
/* SD Protocol registers */
#ifdef HAVE_HOTSWAP
    unsigned int response = 0;
#endif
    unsigned int  i;
    unsigned int  c_size;
    unsigned long c_mult;
    unsigned char carddata[512];
    unsigned char *dataptr;
    int ret;

/* Enable and initialise controller */
    REG_1 = 6;

/* Initialise card data as blank */
    memset(currcard, 0, sizeof(*currcard));

/* Switch card mux to card to initialize */
    sd_card_mux(card_no);

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

    ret = sd_command(GO_IDLE_STATE, 0, NULL, 256);
    if (ret < 0)
        goto card_init_error;

    check_time[EC_POWER_UP] = USEC_TIMER;

#ifdef HAVE_HOTSWAP
    /* Check for SDHC:
       - non-SDHC cards simply ignore SEND_IF_COND (CMD8) and we get error -219,
         which we can just ignore and assume we're dealing with standard SD.
       - SDHC cards echo back the argument into the response. This is how we
         tell if the card is SDHC.
     */
    ret = sd_command(SEND_IF_COND,0x1aa, &response,7);
    if ( (ret < 0) && (ret!=-219) )
            goto card_init_error;
#endif

    while ((currcard->ocr & (1 << 31)) == 0) /* until card is powered up */
    {
        ret = sd_command(APP_CMD, currcard->rca, NULL, 1);
        if (ret < 0)
            goto card_init_error;

#ifdef HAVE_HOTSWAP
        if(response == 0x1aa)
        {
            /* SDHC */
            ret = sd_command(SD_APP_OP_COND, (1<<30)|0x100000,
                             &currcard->ocr, 3);
        }
        else
#endif /* HAVE_HOTSWAP */
        {
            /* SD Standard */
            ret = sd_command(SD_APP_OP_COND, 0x100000, &currcard->ocr, 3);
        }

        if (ret < 0)
            goto card_init_error;

        if (!sd_check_timeout(5000000, EC_POWER_UP))
        {
            ret = -EC_POWER_UP;
            goto card_init_error;
        }
    }

    ret = sd_command(ALL_SEND_CID, 0, currcard->cid, 2);
    if (ret < 0)
        goto card_init_error;

    ret = sd_command(SEND_RELATIVE_ADDR, 0, &currcard->rca, 1);
    if (ret < 0)
        goto card_init_error;

    ret = sd_command(SEND_CSD, currcard->rca, currcard->csd, 2);
    if (ret < 0)
        goto card_init_error;

    /* These calculations come from the Sandisk SD card product manual */
    if( (currcard->csd[3]>>30) == 0)
    {
        /* CSD version 1.0 */
        c_size = ((currcard->csd[2] & 0x3ff) << 2) + (currcard->csd[1]>>30) + 1;
        c_mult = 4 << ((currcard->csd[1] >> 15) & 7);
        currcard->max_read_bl_len = 1 << ((currcard->csd[2] >> 16) & 15);
        currcard->block_size = BLOCK_SIZE;     /* Always use 512 byte blocks */
        currcard->numblocks = c_size * c_mult * (currcard->max_read_bl_len/512);
        currcard->capacity = currcard->numblocks * currcard->block_size;
    }
#ifdef HAVE_HOTSWAP
    else if( (currcard->csd[3]>>30) == 1)
    {
        /* CSD version 2.0 */
        c_size = ((currcard->csd[2] & 0x3f) << 16) + (currcard->csd[1]>>16) + 1;
        currcard->max_read_bl_len = 1 << ((currcard->csd[2] >> 16) & 0xf);
        currcard->block_size = BLOCK_SIZE;     /* Always use 512 byte blocks */
        currcard->numblocks = c_size << 10;
        currcard->capacity = currcard->numblocks * currcard->block_size;
    }
#endif /* HAVE_HOTSWAP */
    
    REG_1 = 0;

    ret = sd_command(SELECT_CARD, currcard->rca, NULL, 129);
    if (ret < 0)
        goto card_init_error;

    ret = sd_command(APP_CMD, currcard->rca, NULL, 1);
    if (ret < 0)
        goto card_init_error;

    ret = sd_command(SET_BUS_WIDTH, currcard->rca | 2, NULL, 1); /* 4 bit */
    if (ret < 0)
        goto card_init_error;

    ret = sd_command(SET_BLOCKLEN, currcard->block_size, NULL, 1);
    if (ret < 0)
        goto card_init_error;

    BLOCK_SIZE_REG = currcard->block_size;

    /* If this card is >4GB & not SDHC, then we need to enable bank switching */
    if( (currcard->numblocks >= BLOCKS_PER_BANK) &&
        ((currcard->ocr & (1<<30)) == 0) )
    {
        SD_STATE_REG = TRAN;
        BLOCK_COUNT_REG = 1;

        ret = sd_command(SWITCH_FUNC, 0x80ffffef, NULL, 0x1c05);
        if (ret < 0)
            goto card_init_error;

        /* Read 512 bytes from the card.
        The first 512 bits contain the status information
        TODO: Do something useful with this! */
        dataptr = carddata;
        for (i = 0; i < BLOCK_SIZE/2; i += FIFO_LEN)
        {
            /* Wait for the FIFO to be full */
            if (sd_poll_status(FIFO_FULL, 100000))
            {
                copy_read_sectors_slow(&dataptr);
                continue;
            }

            ret = -EC_FIFO_ENA_BANK_EMPTY;
            goto card_init_error;
        }
    }

    currcard->initialized = 1;
    return;

    /* Card failed to initialize so disable it */
card_init_error:
    currcard->initialized = ret;
}

/* lock must already be aquired */
static void sd_select_device(int card_no)
{
    currcard = &card_info[card_no];

    if (card_no == 0)
    {
        /* Main card always gets a chance */
        sd_status[0].retry = 0;
    }

    if (currcard->initialized > 0)
    {
        /* This card is already initialized - switch to it */
        sd_card_mux(card_no);
        return;
    }

    if (currcard->initialized == 0)
    {
        /* Card needs (re)init */
        sd_init_device(card_no);
    }
}

/* API Functions */

void ata_led(bool onoff)
{
    led(onoff);
}

int ata_read_sectors(IF_MV2(int drive,) unsigned long start, int incount,
                     void* inbuf)
{
#ifndef HAVE_HOTSWAP
    const int drive = 0;
#endif
    int ret;
    unsigned char *buf, *buf_end;
    int bank;
    
    /* TODO: Add DMA support. */

    mutex_lock(&sd_mtx);

    ata_led(true);

ata_read_retry:
    if (drive != 0 && !card_detect_target())
    {
        /* no external sd-card inserted */
        ret = -EC_NOCARD;
        goto ata_read_error;
    }

    sd_select_device(drive);

    if (currcard->initialized < 0)
    {
        ret = currcard->initialized;
        goto ata_read_error;
    }

    last_disk_activity = current_tick;

    /* Only switch banks with non-SDHC cards */
    if((currcard->ocr & (1<<30))==0)
    {
        bank = start / BLOCKS_PER_BANK;

        if (currcard->current_bank != bank)
        {
            ret = sd_select_bank(bank);
            if (ret < 0)
                goto ata_read_error;
        }
    
        start -= bank * BLOCKS_PER_BANK;
    }

    ret = sd_wait_for_state(TRAN, EC_TRAN_READ_ENTRY);
    if (ret < 0)
        goto ata_read_error;

    BLOCK_COUNT_REG = incount;

#ifdef HAVE_HOTSWAP
    if(currcard->ocr & (1<<30) )
    {
        /* SDHC */
        ret = sd_command(READ_MULTIPLE_BLOCK, start, NULL, 0x1c25);
    }
    else
#endif
    {
        ret = sd_command(READ_MULTIPLE_BLOCK, start * BLOCK_SIZE, NULL, 0x1c25);
    }
    if (ret < 0)
        goto ata_read_error;

    /* TODO: Don't assume BLOCK_SIZE == SECTOR_SIZE */

    buf_end = (unsigned char *)inbuf + incount * currcard->block_size;
    for (buf = inbuf; buf < buf_end;)
    {
        /* Wait for the FIFO to be full */
        if (sd_poll_status(FIFO_FULL, 0x80000))
        {
            copy_read_sectors_fast(&buf); /* Copy one chunk of 16 words */
            /* TODO: Switch bank if necessary */
            continue;
        }

        ret = -EC_FIFO_READ_FULL;
        goto ata_read_error;
    }

    last_disk_activity = current_tick;

    ret = sd_command(STOP_TRANSMISSION, 0, NULL, 1);
    if (ret < 0)
        goto ata_read_error;

    ret = sd_wait_for_state(TRAN, EC_TRAN_READ_EXIT);
    if (ret < 0)
        goto ata_read_error;

    while (1)
    {
        ata_led(false);
        mutex_unlock(&sd_mtx);

        return ret;

ata_read_error:
        if (sd_status[drive].retry < sd_status[drive].retry_max
            && ret != -EC_NOCARD)
        {
            sd_status[drive].retry++;
            currcard->initialized = 0;
            goto ata_read_retry;
        }
    }
}

int ata_write_sectors(IF_MV2(int drive,) unsigned long start, int count,
                      const void* outbuf)
{
/* Write support is not finished yet */
/* TODO: The standard suggests using ACMD23 prior to writing multiple blocks
   to improve performance */
#ifndef HAVE_HOTSWAP
    const int drive = 0;
#endif
    int ret;
    const unsigned char *buf, *buf_end;
    int bank;

    mutex_lock(&sd_mtx);

    ata_led(true);

ata_write_retry:
    if (drive != 0 && !card_detect_target())
    {
        /* no external sd-card inserted */
        ret = -EC_NOCARD;
        goto ata_write_error;
    }

    sd_select_device(drive);

    if (currcard->initialized < 0)
    {
        ret = currcard->initialized;
        goto ata_write_error;
    }

    /* Only switch banks with non-SDHC cards */
    if((currcard->ocr & (1<<30))==0)
    {
        bank = start / BLOCKS_PER_BANK;

        if (currcard->current_bank != bank)
        {
            ret = sd_select_bank(bank);
            if (ret < 0)
                goto ata_write_error;
        }
    
        start -= bank * BLOCKS_PER_BANK;
    }

    check_time[EC_WRITE_TIMEOUT] = USEC_TIMER;

    ret = sd_wait_for_state(TRAN, EC_TRAN_WRITE_ENTRY);
    if (ret < 0)
        goto ata_write_error;

    BLOCK_COUNT_REG = count;

#ifdef HAVE_HOTSWAP
    if(currcard->ocr & (1<<30) )
    {
        /* SDHC */
        ret = sd_command(WRITE_MULTIPLE_BLOCK, start, NULL, 0x1c2d);
    }
    else
#endif
    {
        ret = sd_command(WRITE_MULTIPLE_BLOCK, start*BLOCK_SIZE, NULL, 0x1c2d);
    }
    if (ret < 0)
        goto ata_write_error;

    buf_end = outbuf + count * currcard->block_size - 2*FIFO_LEN;

    for (buf = outbuf; buf <= buf_end;)
    {
        if (buf == buf_end)
        {
            /* Set SD_STATE_REG to PRG for the last buffer fill */
            SD_STATE_REG = PRG;
        }

        udelay(2); /* needed here (loop is too fast :-) */

        /* Wait for the FIFO to empty */
        if (sd_poll_status(FIFO_EMPTY, 0x80000))
        {
            copy_write_sectors(&buf); /* Copy one chunk of 16 words */
            /* TODO: Switch bank if necessary */
            continue;
        }

        ret = -EC_FIFO_WR_EMPTY;
        goto ata_write_error;
    }

    last_disk_activity = current_tick;

    if (!sd_poll_status(DATA_DONE, 0x80000))
    {
        ret = -EC_FIFO_WR_DONE;
        goto ata_write_error;
    }

    ret = sd_command(STOP_TRANSMISSION, 0, NULL, 1);
    if (ret < 0)
        goto ata_write_error;

    ret = sd_wait_for_state(TRAN, EC_TRAN_WRITE_EXIT);
    if (ret < 0)
        goto ata_write_error;

    while (1)
    {
        ata_led(false);
        mutex_unlock(&sd_mtx);

        return ret;

ata_write_error:
        if (sd_status[drive].retry < sd_status[drive].retry_max
            && ret != -EC_NOCARD)
        {
            sd_status[drive].retry++;
            currcard->initialized = 0;
            goto ata_write_retry;
        }
    }
}

static void sd_thread(void) __attribute__((noreturn));
static void sd_thread(void)
{
    struct queue_event ev;
    bool idle_notified = false;
    
    while (1)
    {
        queue_wait_w_tmo(&sd_queue, &ev, HZ);

        switch ( ev.id ) 
        {
#ifdef HAVE_HOTSWAP
        case SYS_HOTSWAP_INSERTED:
        case SYS_HOTSWAP_EXTRACTED:
            fat_lock();          /* lock-out FAT activity first -
                                    prevent deadlocking via disk_mount that
                                    would cause a reverse-order attempt with
                                    another thread */
            mutex_lock(&sd_mtx); /* lock-out card activity - direct calls
                                    into driver that bypass the fat cache */

            /* We now have exclusive control of fat cache and ata */

            disk_unmount(1);     /* release "by force", ensure file
                                    descriptors aren't leaked and any busy
                                    ones are invalid if mounting */

            /* Force card init for new card, re-init for re-inserted one or
             * clear if the last attempt to init failed with an error. */
            card_info[1].initialized = 0;
            sd_status[1].retry = 0; 

            if (ev.id == SYS_HOTSWAP_INSERTED)
                disk_mount(1);

            queue_broadcast(SYS_FS_CHANGED, 0);

            /* Access is now safe */
            mutex_unlock(&sd_mtx);
            fat_unlock();
            break;
#endif
        case SYS_TIMEOUT:
            if (TIME_BEFORE(current_tick, last_disk_activity+(3*HZ)))
            {
                idle_notified = false;
            }
            else
            {
                /* never let a timer wrap confuse us */
                next_yield = USEC_TIMER;

                if (!idle_notified)
                {
                    call_ata_idle_notifys(false);
                    idle_notified = true;
                }
            }
            break;
        case SYS_USB_CONNECTED:
            usb_acknowledge(SYS_USB_CONNECTED_ACK);
            /* Wait until the USB cable is extracted again */
            usb_wait_for_disconnect(&sd_queue);

            break;
        case SYS_USB_DISCONNECTED:
            usb_acknowledge(SYS_USB_DISCONNECTED_ACK);
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

#ifdef HAVE_HOTSWAP
void card_enable_monitoring(bool on)
{
    if (on)
    {
#ifdef SANSA_E200
        GPIO_SET_BITWISE(GPIOA_INT_EN, 0x80);
#elif defined(SANSA_C200)
        GPIO_SET_BITWISE(GPIOL_INT_EN, 0x08);
#endif
    }
    else
    {
#ifdef SANSA_E200
        GPIO_CLEAR_BITWISE(GPIOA_INT_EN, 0x80);
#elif defined(SANSA_C200)
        GPIO_CLEAR_BITWISE(GPIOL_INT_EN, 0x08);
#endif
    }
}
#endif

int ata_init(void)
{
    int ret = 0;

    if (!initialized)
        mutex_init(&sd_mtx);

    mutex_lock(&sd_mtx);

    ata_led(false);

    if (!initialized)
    {
        initialized = true;

        /* init controller */
        outl(inl(0x70000088) & ~(0x4), 0x70000088);
        outl(inl(0x7000008c) & ~(0x4), 0x7000008c);
        GPO32_ENABLE |= 0x4;

        GPIO_SET_BITWISE(GPIOG_ENABLE, (0x3 << 5));
        GPIO_SET_BITWISE(GPIOG_OUTPUT_EN, (0x3 << 5));
        GPIO_SET_BITWISE(GPIOG_OUTPUT_VAL, (0x3 << 5));

#ifdef HAVE_HOTSWAP
        /* enable card detection port - mask interrupt first */
#ifdef SANSA_E200
        GPIO_CLEAR_BITWISE(GPIOA_INT_EN, 0x80);

        GPIO_CLEAR_BITWISE(GPIOA_OUTPUT_EN, 0x80);
        GPIO_SET_BITWISE(GPIOA_ENABLE, 0x80);
#elif defined SANSA_C200
        GPIO_CLEAR_BITWISE(GPIOL_INT_EN, 0x08);

        GPIO_CLEAR_BITWISE(GPIOL_OUTPUT_EN, 0x08);
        GPIO_SET_BITWISE(GPIOL_ENABLE, 0x08);
#endif
#endif
        sd_select_device(0);

        if (currcard->initialized < 0)
            ret = currcard->initialized;

        queue_init(&sd_queue, true);
        create_thread(sd_thread, sd_stack, sizeof(sd_stack), 0,
            sd_thread_name IF_PRIO(, PRIORITY_USER_INTERFACE)
            IF_COP(, CPU));

        /* enable interupt for the mSD card */
        sleep(HZ/10);
#ifdef HAVE_HOTSWAP
#ifdef SANSA_E200
        CPU_INT_EN = HI_MASK;
        CPU_HI_INT_EN = GPIO0_MASK;

        GPIOA_INT_LEV = (0x80 << 8) | (~GPIOA_INPUT_VAL & 0x80);

        GPIOA_INT_CLR = 0x80;
#elif defined SANSA_C200
        CPU_INT_EN = HI_MASK;
        CPU_HI_INT_EN = GPIO2_MASK;

        GPIOL_INT_LEV = (0x08 << 8) | (~GPIOL_INPUT_VAL & 0x08);

        GPIOL_INT_CLR = 0x08;
#endif
#endif
    }

    mutex_unlock(&sd_mtx);

    return ret;
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

#ifdef HAVE_HOTSWAP
bool card_detect_target(void)
{
#ifdef SANSA_E200
    return (GPIOA_INPUT_VAL & 0x80) == 0; /* low active */
#elif defined SANSA_C200
    return (GPIOL_INPUT_VAL & 0x08) != 0; /* high active */
#endif
}

static bool sd1_oneshot_callback(struct timeout *tmo)
{
    (void)tmo;

    /* This is called only if the state was stable for 300ms - check state
     * and post appropriate event. */
    if (card_detect_target())
        queue_broadcast(SYS_HOTSWAP_INSERTED, 0);
    else
        queue_broadcast(SYS_HOTSWAP_EXTRACTED, 0);

    return false;
}

/* called on insertion/removal interrupt */
void microsd_int(void)
{
    static struct timeout sd1_oneshot;

#ifdef SANSA_E200
    GPIO_CLEAR_BITWISE(GPIOA_INT_EN, 0x80);
    GPIOA_INT_LEV = (0x80 << 8) | (~GPIOA_INPUT_VAL & 0x80);
    GPIOA_INT_CLR = 0x80;
    GPIO_SET_BITWISE(GPIOA_INT_EN, 0x80);

#elif defined SANSA_C200
    GPIO_CLEAR_BITWISE(GPIOL_INT_EN, 0x08);
    GPIOL_INT_LEV = (0x08 << 8) | (~GPIOL_INPUT_VAL & 0x08);
    GPIOL_INT_CLR = 0x08;
    GPIO_SET_BITWISE(GPIOL_INT_EN, 0x08);
#endif
    timeout_register(&sd1_oneshot, sd1_oneshot_callback, (3*HZ/10), 0);

}
#endif /* HAVE_HOTSWAP */
