/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr & Nick Robinson
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "panic.h"
#include "iap.h"
#include "button.h"
#include "config.h"
#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "thread.h"
#include "serial.h"
#include "appevents.h"
#include "timefuncs.h"
#include "core_alloc.h"

#include "playlist.h"
#include "playback.h"
#include "audio.h"
#include "settings.h"
#include "metadata.h"
#include "wps.h"
#include "sound.h"
#include "action.h"
#include "powermgmt.h"

#include "tuner.h"
#include "ipod_remote_tuner.h"

#include "filetree.h"
#include "dir.h"

/*
 * This macro is meant to be used inside an IAP mode message handler.
 * It is passed the expected minimum length of the message buffer.
 * If the buffer does not have the required lenght an ACK
 * packet with a Bad Parameter error is generated.
 *
 * Used for Lingo 0x00
 */
#define CHECKLEN0(x) do { \
        if (len < (x)) { \
            cmd_ack_mode0(cmd, IAP_ACK_BAD_PARAM); \
            return; \
        }} while(0)

/* The CHECKLEN0() macro, for Lingo 0x02 */
#define CHECKLEN2(x) do { \
        if (len < (x)) { \
            cmd_ack_mode2(cmd, IAP_ACK_BAD_PARAM); \
            return; \
        }} while(0)

/* The CHECKLEN0() macro, for Lingo 0x03 */
#define CHECKLEN3(x) do { \
        if (len < (x)) { \
            cmd_ack_mode3(cmd, IAP_ACK_BAD_PARAM); \
            return; \
        }} while(0)

/* The CHECKLEN0() macro, for Lingo 0x04
 */
#define CHECKLEN4(x) do { \
        if (len < (x)) { \
            cmd_ack_mode4(cmd, IAP_ACK_BAD_PARAM); \
            return; \
        }} while(0)

/* Check for authenticated state, and return an ACK Not
 * Authenticated on failure.
 *
 * Used for Lingo 0x00
 */
#define CHECKAUTH0 do { \
        if (!DEVICE_AUTHENTICATED) { \
            cmd_ack_mode0(cmd, IAP_ACK_NO_AUTHEN); \
            return; \
        }} while(0)

/* The CHECKAUTH0 macro, for Lingo 0x03 */
#define CHECKAUTH3 do { \
        if (!DEVICE_AUTHENTICATED) { \
            cmd_ack_mode3(cmd, IAP_ACK_NO_AUTHEN); \
            return; \
        }} while(0)

/* MS_TO_TICKS converts a milisecond time period into the
 * corresponding amount of ticks. If the time period cannot
 * be accurately measured in ticks it will round up.
 */
#if (HZ>1000)
#error "HZ is >1000, please fix MS_TO_TICKS"
#endif
#define MS_PER_HZ (1000/HZ)
#define MS_TO_TICKS(x) (((x)+MS_PER_HZ-1)/MS_PER_HZ)
/* IAP specifies a timeout of 25ms for traffic from a device to the iPod.
 * Depending on HZ this cannot be accurately measured. Find out the next
 * best thing.
 */
#define IAP_PKT_TIMEOUT (MS_TO_TICKS(25))

#define IAP_ACK_OK          (0x00)  /* Success */
#define IAP_ACK_UNKNOWN_DB  (0x01)  /* Unknown Database Category */
#define IAP_ACK_CMD_FAILED  (0x02)  /* Command failed */
#define IAP_ACK_NO_RESOURCE (0x03)  /* Out of resources */
#define IAP_ACK_BAD_PARAM   (0x04)  /* Bad parameter */
#define IAP_ACK_UNKNOWN_ID  (0x05)  /* Unknown ID */
#define IAP_ACK_PENDING     (0x06)  /* Command pending */
#define IAP_ACK_NO_AUTHEN   (0x07)  /* Not authenticated */
#define IAP_ACK_BAD_AUTHEN  (0x08)  /* Bad authentication version */
/* 0x09 reserved */
#define IAP_ACK_CERT_INVAL  (0x0A)  /* Certificate invalid */
#define IAP_ACK_CERT_PERM   (0x0B)  /* Certificate permissions invalid */
/* 0x0C-0x10 reserved */
#define IAP_ACK_RES_INVAL   (0x11)  /* Invalid accessory resistor value */

/* Events in the iap_queue */
#define IAP_EV_TICK         (1)     /* The regular task timeout */
#define IAP_EV_MSG_RCVD     (2)     /* A complete message has been received from the device */
#define IAP_EV_MALLOC       (3)     /* Allocate memory for the RX/TX buffers */

/* Add a button to the remote button bitfield. Also set iap_repeatbtn=1
 * to ensure a button press is at least delivered once.
 */
#define REMOTE_BUTTON(x) do { \
        iap_remotebtn |= (x); \
        iap_timeoutbtn = 3; \
        iap_repeatbtn = 2; \
        } while(0)
        
static bool iap_started = false;
static bool iap_setupflag = false, iap_updateflag = false, iap_running = false;
/* This is set to true if a SYS_POWEROFF message is received,
 * signalling impending power off
 */
static bool iap_shutdown = false;
static struct timeout iap_task_tmo;

static unsigned long iap_remotebtn = 0;
/* Used to make sure a button press is delivered to the processing
 * backend. While this is !0, no new incoming messasges are processed.
 * Counted down by remote_control_rx()
 */
static int iap_repeatbtn = 0;
/* Used to time out button down events in case we miss the button up event
 * from the device somehow.
 * If a device sends a button down event it's required to repeat that event
 * every 30 to 100ms as long as the button is pressed, and send an explicit
 * button up event if the button is released.
 * In case the button up event is lost any down events will time out after
 * ~200ms.
 * iap_periodic() will count down this variable and reset all buttons if
 * it reaches 0
 */
static unsigned int iap_timeoutbtn = 0;
static bool iap_btnrepeat = false, iap_btnshuffle = false;

static long thread_stack[(DEFAULT_STACK_SIZE*4)/sizeof(long)];
static struct event_queue iap_queue;

/* These are pointer used to manage a dynamically allocated buffer which
 * will hold both the RX and TX side of things.
 *
 * iap_buffer_handle is the handle returned from core_alloc()
 * iap_buffers points to the start of the complete buffer
 *
 * The buffer is partitioned as follows:
 * - TX_BUFLEN+6 bytes for the TX buffer
 *   The 6 extra bytes are for the sync byte, the SOP byte, the length indicators
 *   (3 bytes) and the checksum byte.
 *   iap_txstart points to the beginning of the TX buffer
 *   iap_txpayload points to the beginning of the payload portion of the TX buffer
 *   iap_txnext points to the position where the next byte will be placed
 *
 * - RX_BUFLEN+2 bytes for the RX buffer
 *   The RX buffer can hold multiple packets at once, up to it's
 *   maximum capacity. Every packet consists of a two byte length
 *   indicator followed by the actual payload. The length indicator
 *   is two bytes for every length, even for packets with a length <256
 *   bytes.
 *
 *   Once a packet has been processed from the RX buffer the rest
 *   of the buffer (and the pointers below) are shifted to the front
 *   so that the next packet again starts at the beginning of the
 *   buffer. This happens with interrupts disabled, to prevent
 *   writing into the buffer during the move.
 *
 *   iap_rxstart points to the beginning of the RX buffer
 *   iap_rxpayload starts to the beginning of the currently recieved
 *   packet
 *   iap_rxnext points to the position where the next incoming byte
 *   will be placed
 *   iap_rxlen is not a pointer, but an indicator of the free
 *   space left in the RX buffer.
 *
 * The RX buffer is placed behind the TX buffer so that an eventual TX
 * buffer overflow has some place to spill into where it will not cause
 * immediate damage. See the comments for IAP_TX_* and iap_send_tx()
 */
#define IAP_MALLOC_SIZE (TX_BUFLEN+6+RX_BUFLEN+2)
#ifdef IAP_MALLOC_DYNAMIC
static int iap_buffer_handle;
#endif
static unsigned char* iap_buffers;
static unsigned char* iap_rxstart;
static unsigned char* iap_rxpayload;
static unsigned char* iap_rxnext;
static uint32_t iap_rxlen;
static unsigned char* iap_txstart;
static unsigned char* iap_txpayload;
static unsigned char* iap_txnext;

/* These are a number of helper macros to manage the dynamic TX buffer content
 * These macros DO NOT CHECK for buffer overflow. iap_send_tx() will, but
 * it might be too late at that point. See the current size of TX_BUFLEN
 */

/* Initialize the TX buffer with a lingo and command ID. This will reset the 
 * data pointer, effectively invalidating unsent information in the TX buffer.
 * There are two versions of this, one for 1 byte command IDs (all Lingoes except
 * 0x04) and one for two byte command IDs (Lingo 0x04)
 */
#define IAP_TX_INIT(lingo, command) do { \
        iap_txnext = iap_txpayload; \
        IAP_TX_PUT((lingo)); \
        IAP_TX_PUT((command)); \
        } while (0)

#define IAP_TX_INIT4(lingo, command) do { \
        iap_txnext = iap_txpayload; \
        IAP_TX_PUT((lingo)); \
        IAP_TX_PUT_U16((command)); \
        } while (0)

/* Put an unsigned char into the TX buffer */
#define IAP_TX_PUT(data) *(iap_txnext++) = (data)

/* Put a 16bit unsigned quantity into the TX buffer */
#define IAP_TX_PUT_U16(data) do { \
        put_u16(iap_txnext, (data)); \
        iap_txnext += 2; \
        } while (0)

/* Put a 32bit unsigned quantity into the TX buffer */
#define IAP_TX_PUT_U32(data) do { \
        put_u32(iap_txnext, (data)); \
        iap_txnext += 4; \
        } while (0)

/* Put an arbitrary amount of data (identified by a char pointer and 
 * a length) into the TX buffer
 */
#define IAP_TX_PUT_DATA(data, len) do { \
        memcpy(iap_txnext, (unsigned char *)(data), (len)); \
        iap_txnext += (len); \
        } while(0)

/* Put a NULL terminated string into the TX buffer, including the
 * NULL byte
 */
#define IAP_TX_PUT_STRING(str) IAP_TX_PUT_DATA((str), strlen((str))+1)

/* Put a NULL terminated string into the TX buffer, taking care not to
 * overflow the buffer. If the string does not fit into the TX buffer
 * it will be truncated, but always NULL terminated.
 *
 * This function is expensive compared to the other IAP_TX_PUT_*
 * functions
 */
#define IAP_TX_PUT_STRLCPY(str) iap_tx_strlcpy(str)

/* The Model ID of the iPod we emulate. Currently a 160GB classic */
#define IAP_IPOD_MODEL (0x00130200U)

/* The firmware version we emulate. Currently 2.0.3 */
#define IAP_IPOD_FIRMWARE_MAJOR (2)
#define IAP_IPOD_FIRMWARE_MINOR (0)
#define IAP_IPOD_FIRMWARE_REV   (3)

static char cur_dbrecord[5] = {0};

/* states of the iap de-framing state machine */
enum fsm_state {
    ST_SYNC,    /* wait for 0xFF sync byte */
    ST_SOF,     /* wait for 0x55 start-of-frame byte */
    ST_LEN,     /* receive length byte (small packet) */
    ST_LENH,    /* receive length high byte (large packet) */
    ST_LENL,    /* receive length low byte (large packet) */
    ST_DATA,    /* receive data */
    ST_CHECK    /* verify checksum */
};

static struct state_t {
    enum fsm_state state;   /* current fsm state */
    unsigned int len;       /* payload data length */
    unsigned int check;     /* running checksum over [len,payload,check] */
    unsigned int count;     /* playload bytes counter */
} frame_state = {
    .state = ST_SYNC
};

/* States of the extended command support */
enum interface_state {
    IST_STANDARD,    /* General state, support lingo 0x00 commands */
    IST_EXTENDED,   /* Extended Interface lingo (0x04) negotiated */
};
static enum interface_state interface_state = IST_STANDARD;

/* The versions of the various lingos we support. A major version
 * of 0 means unsupported
 */
static unsigned char lingo_versions[32][2] = {
    {1, 8},     /* General lingo, 0x00 */
    {0, 0},     /* Microphone lingo, 0x01, unsupported */
    {1, 2},     /* Simple remote lingo, 0x02 */
    {1, 5},     /* Display remote lingo, 0x03 */
    {1, 0},     /* Extended Interface lingo, 0x04 */
    {}          /* All others are unsupported */
};
#define LINGO_SUPPORTED(x) (LINGO_MAJOR((x)&0x1f) > 0)
#define LINGO_MAJOR(x) (lingo_versions[(x)&0x1f][0])
#define LINGO_MINOR(x) (lingo_versions[(x)&0x1f][1])

/* States of the authentication state machine */
enum authen_state {
    AUST_NONE,      /* Initial state, no message sent */
    AUST_INIT,      /* Remote side has requested authentication */
    AUST_CERTREQ,   /* Remote certificate requested */
    AUST_CERTBEG,   /* Certificate is being received */
    AUST_CERTDONE,  /* Certificate received */
    AUST_CHASENT,   /* Challenge sent */
    AUST_CHADONE,   /* Challenge response received */
    AUST_AUTH,      /* Authentication complete */
};
/* State of authentication */
struct auth_t {
    enum authen_state state;        /* Current state of authentication */
    unsigned char max_section;      /* The maximum number of certificate sections */
    unsigned char next_section;     /* The next expected section number */
};

/* State of GetAccessoryInfo */
enum accinfo_state {
    ACCST_NONE,     /* Initial state, no message sent */
    ACCST_INIT,     /* Send out initial GetAccessoryInfo */
    ACCST_SENT,     /* Wait for initial RetAccessoryInfo */
    ACCST_DATA,     /* Query device information, according to capabilities */
};

/* A struct describing an attached device and it's current
 * state
 */
static struct device_t {
    struct auth_t auth;             /* Authentication state */
    enum accinfo_state accinfo;     /* Accessory information state */
    uint32_t lingoes;               /* Negotiated lingoes */
    uint32_t notifications;         /* Requested notifications. These are just the
                                     * notifications explicitly requested by the
                                     * device
                                     */
    uint32_t changed_notifications; /* Tracks notifications that changed since the last
                                     * call to SetRemoteEventNotification or GetRemoteEventStatus
                                     */
    bool do_notify;                 /* Notifications enabled */
    bool do_power_notify;           /* Whether to send power change notifications.
                                     * These are sent automatically to all devices
                                     * that used IdentifyDeviceLingoes to identify
                                     * themselves, independent of other notifications
                                     */

    uint32_t trackpos_ms;           /* These fields are to save the current state */
    uint32_t track_index;           /* of various fields so we can send a notification */
    uint32_t chapter_index;         /* if they change */
    unsigned char play_status;
    bool mute;
    unsigned char volume;
    unsigned char power_state;
    unsigned char battery_level;
    uint32_t equalizer_index;
    unsigned char shuffle;
    unsigned char repeat;
    struct tm datetime;
    unsigned char alarm_state;
    unsigned char alarm_hour;
    unsigned char alarm_minute;
    unsigned char backlight;
    bool hold;
    unsigned char soundcheck;
    unsigned char audiobook;
    uint16_t trackpos_s;
    uint32_t capabilities;          /* Capabilities of the device, as returned by type 0
                                     * of GetAccessoryInfo
                                     */
    uint32_t capabilities_queried;  /* Capabilities already queried */
} device;
#define DEVICE_AUTHENTICATED (device.auth.state == AUST_AUTH)
#define DEVICE_AUTH_RUNNING ((device.auth.state != AUST_NONE) && (device.auth.state != AUST_AUTH))
#define DEVICE_LINGO_SUPPORTED(x) (device.lingoes & BIT_N((x)&0x1f))

#ifdef IAP_MALLOC_DYNAMIC
static int iap_move_callback(int handle, void* current, void* new);

static struct buflib_callbacks iap_buflib_callbacks = {
    iap_move_callback,
    NULL
};
#endif

static void iap_malloc(void);
static void iap_shuffle_state(bool state);
static void iap_repeat_state(unsigned char state);
static void iap_repeat_next(void);
static void iap_fill_power_state(void);

static void put_u16(unsigned char *buf, const uint16_t data)
{
    buf[0] = (data >>  8) & 0xFF;
    buf[1] = (data >>  0) & 0xFF;
}

static void put_u32(unsigned char *buf, const uint32_t data)
{
    buf[0] = (data >> 24) & 0xFF;
    buf[1] = (data >> 16) & 0xFF;
    buf[2] = (data >>  8) & 0xFF;
    buf[3] = (data >>  0) & 0xFF;
}

static uint32_t get_u32(const unsigned char *buf)
{
    return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

static uint16_t get_u16(const unsigned char *buf)
{
    return (buf[0] << 8) | buf[1];
}

static void iap_tx_strlcpy(const unsigned char *str)
{
    ptrdiff_t txfree;
    size_t r;

    txfree = TX_BUFLEN - (iap_txnext - iap_txstart);
    r = strlcpy(iap_txnext, str, txfree);

    if (r < txfree)
    {
        /* No truncation occured
         * Account for the terminating \0
         */
        iap_txnext += (r+1);
    } else {
        /* Truncation occured, the TX buffer is now full. */
        iap_txnext = iap_txstart + TX_BUFLEN;
    }
}

static void reset_auth(struct auth_t* auth)
{
    auth->state = AUST_NONE;
    auth->max_section = 0;
    auth->next_section = 0;
}

static void reset_device(struct device_t* device)
{
    reset_auth(&(device->auth));
    device->lingoes = 0;
    device->notifications = 0;
    device->changed_notifications = 0;
    device->do_notify = false;
    device->do_power_notify = false;
    device->accinfo = ACCST_NONE;
    device->capabilities = 0;
    device->capabilities_queried = 0;
}

static int iap_task(struct timeout *tmo)
{
    (void) tmo;

    queue_post(&iap_queue, IAP_EV_TICK, 0);
    return MS_TO_TICKS(100);
}

/* This thread is waiting for events posted to iap_queue and calls
 * the appropriate subroutines in response
 */
static void iap_thread(void)
{
    struct queue_event ev;
    while(1) {
        queue_wait(&iap_queue, &ev);
        switch (ev.id)
        {
            /* Handle the regular 100ms tick used for driving the
             * authentication state machine and notifications
             */
            case IAP_EV_TICK:
            {
                iap_periodic();
                break;
            }

            /* Handle a newly received message from the device */
            case IAP_EV_MSG_RCVD:
            {
                iap_handlepkt();
                break;
            }

            /* Handle memory allocation. This is used only once, during
             * startup
             */
            case IAP_EV_MALLOC:
            {
                iap_malloc();
                break;
            }

            /* Handle poweroff message */
            case SYS_POWEROFF:
            {
                iap_shutdown = true;
                break;
            }
        }
    }
}

/* called by playback when the next track starts */
static void iap_track_changed(void *ignored)
{
    (void)ignored;
}

/* Do general setup of the needed infrastructure.
 *
 * Please note that a lot of additional work is done by iap_start()
 */
void iap_setup(const int ratenum)
{
    iap_bitrate_set(ratenum);
    iap_updateflag = false;
    iap_remotebtn = BUTTON_NONE;
    iap_setupflag = true;
    iap_started = false;
    iap_running = false;
}

/* Actually bring up the message queue, message handler thread and
 * notification timer
 *
 * NOTE: This is running in interrupt context
 */
static void iap_start(void)
{
    unsigned int tid;

    if (iap_started)
        return;

    reset_device(&device);
    queue_init(&iap_queue, true);
    tid = create_thread(iap_thread, thread_stack, sizeof(thread_stack),
            0, "iap"
            IF_PRIO(, PRIORITY_SYSTEM)
            IF_COP(, CPU));
    if (!tid)
        panicf("Could not create iap thread");
    timeout_register(&iap_task_tmo, iap_task, MS_TO_TICKS(100), (intptr_t)NULL);
    add_event(PLAYBACK_EVENT_TRACK_CHANGE, false, iap_track_changed);

    /* Since we cannot allocate memory while in interrupt context
     * post a message to our own queue to get that done
     */
    queue_post(&iap_queue, IAP_EV_MALLOC, 0);
    iap_started = true;
}

static void iap_malloc(void)
{
#ifndef IAP_MALLOC_DYNAMIC
    static unsigned char serbuf[IAP_MALLOC_SIZE];
#endif

    if (iap_running)
        return;

#ifdef IAP_MALLOC_DYNAMIC
    iap_buffer_handle = core_alloc_ex("iap", IAP_MALLOC_SIZE, &iap_buflib_callbacks);
    if (iap_buffer_handle < 0)
        panicf("Could not allocate buffer memory");
    iap_buffers = core_get_data(iap_buffer_handle);
#else
    iap_buffers = serbuf;
#endif
    iap_txstart = iap_buffers;
    iap_txpayload = iap_txstart+5;
    iap_txnext = iap_txpayload;
    iap_rxstart = iap_buffers+(TX_BUFLEN+6);
    iap_rxpayload = iap_rxstart;
    iap_rxnext = iap_rxpayload;
    iap_rxlen = RX_BUFLEN+2;
    iap_running = true;
}

void iap_bitrate_set(const int ratenum)
{
    switch(ratenum)
    {
        case 0:
            serial_bitrate(0);
            break;
        case 1:
            serial_bitrate(9600);
            break;
        case 2:
            serial_bitrate(19200);
            break;
        case 3:
            serial_bitrate(38400);
            break;
        case 4:
            serial_bitrate(57600);
            break;
    }
}

/* Message format:
   0xff
   0x55
   length
   mode
   command (2 bytes)
   parameters (0-n bytes)
   checksum (length+mode+parameters+checksum == 0)
*/

/* Send the current content of the TX buffer.
 * This will check for TX buffer overflow and panic, but it might
 * be too late by then (although one would have to overflow the complete
 * RX buffer as well)
 */
static void iap_send_tx(void)
{
    int i, chksum;
    ptrdiff_t txlen;
    unsigned char* txstart;

    txlen = iap_txnext - iap_txpayload;

    if (txlen <= 0)
        return;

    if (txlen > TX_BUFLEN)
        panicf("IAP: TX buffer overflow");

    if (txlen < 256)
    {
        /* Short packet */
        txstart = iap_txstart+2;
        *(txstart+2) = txlen;
        chksum = txlen;
    } else {
        /* Long packet */
        txstart = iap_txstart;
        *(txstart+2) = 0x00;
        *(txstart+3) = (txlen >> 8) & 0xFF;
        *(txstart+4) = (txlen) & 0xFF;
        chksum = *(txstart+3) + *(txstart+4);
    }
    *(txstart) = 0xFF;
    *(txstart+1) = 0x55;

    for (i=0; i<txlen; i++)
    {
        chksum += iap_txpayload[i];
    }
    *(iap_txnext) = 0x100 - (chksum & 0xFF);

    for (i=0; i <= (iap_txnext - txstart); i++)
    {
        while(!tx_rdy()) ;
        tx_writec(txstart[i]);
    }
}

/* This is just a compatibility wrapper around the new TX buffer
 * infrastructure
 */
void iap_send_pkt(const unsigned char * data, const int len)
{
    if (!iap_running)
        return;

    iap_txnext = iap_txpayload;
    IAP_TX_PUT_DATA(data, len);
    iap_send_tx();
}    

bool iap_getc(const unsigned char x)
{
    struct state_t *s = &frame_state;
    static long pkt_timeout;

    if (!iap_setupflag)
        return false;

    /* Check the time since the last packet arrived. */
    if ((s->state != ST_SYNC) && TIME_AFTER(current_tick, pkt_timeout)) {
        /* Packet timeouts only make sense while not waiting for the
         * sync byte */
         s->state = ST_SYNC;
         return iap_getc(x);
    }

    
    /* run state machine to detect and extract a valid frame */
    switch (s->state) {
    case ST_SYNC:
        if (x == 0xFF) {
            /* The IAP infrastructure is started by the first received sync
             * byte. It takes a while to spin up, so do not advance the state
             * machine until it has started.
             */
            if (!iap_running)
            {
                iap_start();
                break;
            }
            iap_rxnext = iap_rxpayload;
            s->state = ST_SOF;
        }
        break;
    case ST_SOF:
        if (x == 0x55) {
            /* received a valid sync/SOF pair */
            s->state = ST_LEN;
        } else {
            s->state = ST_SYNC;
            return iap_getc(x);
        }
        break;
    case ST_LEN:
        s->check = x;
        s->count = 0;
        if (x == 0) {
            /* large packet */
            s->state = ST_LENH;
        } else {
            /* small packet */
            if (x > (iap_rxlen-2))
            {
                /* Packet too long for buffer */
                s->state = ST_SYNC;
                break;
            }
            s->len = x;
            s->state = ST_DATA;
            put_u16(iap_rxnext, s->len);
            iap_rxnext += 2;
        }
        break;
    case ST_LENH:
        s->check += x;
        s->len = x << 8;
        s->state = ST_LENL;
        break;
    case ST_LENL:
        s->check += x;
        s->len += x;
        if ((s->len == 0) || (s->len > (iap_rxlen-2))) {
            /* invalid length */
            s->state = ST_SYNC;
            break;
        } else {
            s->state = ST_DATA;
            put_u16(iap_rxnext, s->len);
            iap_rxnext += 2;
        }
        break;
    case ST_DATA:
        s->check += x;
        *(iap_rxnext++) = x;
        s->count += 1;
        if (s->count == s->len) {
            s->state = ST_CHECK;
        }
        break;
    case ST_CHECK:
        s->check += x;
        if ((s->check & 0xFF) == 0) {
            /* done, received a valid frame */
            iap_rxpayload = iap_rxnext;
            queue_post(&iap_queue, IAP_EV_MSG_RCVD, 0);
        } else {
            /* Invalid frame */
        }
        s->state = ST_SYNC;
        break;
    default:
        panicf("Unhandled iap state %d", (int) s->state);
        break;
    }

    pkt_timeout = current_tick + IAP_PKT_TIMEOUT;
    
    /* return true while still hunting for the sync and start-of-frame byte */
    return (s->state == ST_SYNC) || (s->state == ST_SOF);
}

void iap_get_trackinfo(const unsigned int track, struct mp3entry* id3)
{
    int tracknum;
    int fd;
    struct playlist_track_info info;

    tracknum = track;

    tracknum += playlist_get_first_index(NULL);
    if(tracknum >= playlist_amount())
        tracknum -= playlist_amount();

    /* If the tracknumber is not the current one,
       read id3 from disk */
    if(playlist_next(0) != tracknum)
    {
        playlist_get_track_info(NULL, tracknum, &info);
        fd = open(info.filename, O_RDONLY);
        memset(id3, 0, sizeof(*id3));
        get_metadata(id3, fd, info.filename);
        close(fd);
    } else {
        memcpy(id3, audio_current_track(), sizeof(*id3));
    }
}

static uint32_t iap_get_trackpos(void)
{
    struct mp3entry *id3 = audio_current_track();

    return id3->elapsed;
}

static uint32_t iap_get_trackindex(void)
{
    struct playlist_info* playlist = playlist_get_current();

    return (playlist->index - playlist->first_index);
}

void iap_periodic(void)
{
    static int count;

    if(!iap_setupflag) return;

    /* Handle pending authentication tasks */
    switch (device.auth.state)
    {
        case AUST_INIT:
        {
            /* Send out GetDevAuthenticationInfo */
            IAP_TX_INIT(0x00, 0x14);

            iap_send_tx();
            device.auth.state = AUST_CERTREQ;
            break;
        }

        case AUST_CERTDONE:
        {
            /* Send out GetDevAuthenticationSignature, with
             * 20 bytes of challenge and a retry counter of 1.
             * Since we do not really care about the content of the
             * challenge we just use the first 20 bytes of whatever
             * is in the RX buffer right now.
             */
            IAP_TX_INIT(0x00, 0x17);
            IAP_TX_PUT_DATA(iap_rxstart, 20);
            IAP_TX_PUT(0x01);

            iap_send_tx();
            device.auth.state = AUST_CHASENT;
            break;
        }

        default:
        {
            break;
        }
    }

    /* Time out button down events */
    if (iap_timeoutbtn)
        iap_timeoutbtn -= 1;

    if (!iap_timeoutbtn)
    {
        iap_remotebtn = BUTTON_NONE;
        iap_repeatbtn = 0;
        iap_btnshuffle = false;
        iap_btnrepeat = false;
    }

    /* Handle power down messages. */
    if (iap_shutdown && device.do_power_notify)
    {
        /* NotifyiPodStateChange */
        IAP_TX_INIT(0x00, 0x23);
        IAP_TX_PUT(0x01);

        iap_send_tx();

        /* No further actions, we're going down */
        reset_device(&device);
        return;
    }

    /* Handle GetAccessoryInfo messages */
    if (device.accinfo == ACCST_INIT)
    {
        /* GetAccessoryInfo */
        IAP_TX_INIT(0x00, 0x27);
        IAP_TX_PUT(0x00);

        iap_send_tx();
        device.accinfo = ACCST_SENT;
    }

    /* Do not send requests for device information while
     * an authentication is still running, this seems to
     * confuse some devices
     */
    if (!DEVICE_AUTH_RUNNING && (device.accinfo == ACCST_DATA))
    {
        int first_set;

        /* Find the first bit set in the capabilities field,
         * ignoring those we already asked for
         */
        first_set = find_first_set_bit(device.capabilities & (~device.capabilities_queried));

        if (first_set != 32)
        {
            /* Add bit to queried cababilities */
            device.capabilities_queried |= BIT_N(first_set);

            switch (first_set)
            {
                /* Name */
                case 0x01:
                /* Firmware version */
                case 0x04:
                /* Hardware version */
                case 0x05:
                /* Manufacturer */
                case 0x06:
                /* Model number */
                case 0x07:
                /* Serial number */
                case 0x08:
                /* Maximum payload size */
                case 0x09:
                {
                    IAP_TX_INIT(0x00, 0x27);
                    IAP_TX_PUT(first_set);

                    iap_send_tx();
                    break;
                }

                /* Minimum supported iPod firmware version */
                case 0x02:
                {
                    IAP_TX_INIT(0x00, 0x27);
                    IAP_TX_PUT(2);
                    IAP_TX_PUT_U32(IAP_IPOD_MODEL);
                    IAP_TX_PUT(IAP_IPOD_FIRMWARE_MAJOR);
                    IAP_TX_PUT(IAP_IPOD_FIRMWARE_MINOR);
                    IAP_TX_PUT(IAP_IPOD_FIRMWARE_REV);

                    iap_send_tx();
                    break;
                }

                /* Minimum supported lingo version. Queries Lingo 0 */
                case 0x03:
                {
                    IAP_TX_INIT(0x00, 0x27);
                    IAP_TX_PUT(3);
                    IAP_TX_PUT(0);

                    iap_send_tx();
                    break;
                }
            }

            device.accinfo = ACCST_SENT;
        }
    }

    if (!device.do_notify) return;
    if (device.notifications == 0) return;

    /* Volume change notifications are sent every 100ms */
    if (device.notifications & (BIT_N(4) | BIT_N(16))) {
        /* Currently we do not track volume changes, so this is
         * never sent.
         *
         * TODO: Fix volume tracking
         */
    }

    /* All other events are sent every 500ms */
    count += 1;
    if (count < 5) return;

    count = 0;
    
    /* RemoteEventNotification */
    
    /* Track position (ms)  or Track position (s) */
    if (device.notifications & (BIT_N(0) | BIT_N(15)))
    {
        uint32_t t;
        uint16_t ts;
        bool changed;

        t = iap_get_trackpos();
        ts = (t / 1000) & 0xFFFF;

        if ((device.notifications & BIT_N(0)) && (device.trackpos_ms != t))
        {
            IAP_TX_INIT(0x03, 0x09);
            IAP_TX_PUT(0x00);
            IAP_TX_PUT_U32(t);
            device.changed_notifications |= BIT_N(0);
            changed = true;

            iap_send_tx();
        }

        if ((device.notifications & BIT_N(15)) && (device.trackpos_s != ts)) {
            IAP_TX_INIT(0x03, 0x09);
            IAP_TX_PUT(0x0F);
            IAP_TX_PUT_U16(ts);
            device.changed_notifications |= BIT_N(15);
            changed = true;

            iap_send_tx();
        }

        if (changed)
        {
            device.trackpos_ms = t;
            device.trackpos_s = ts;
        }
    }

    /* Track index */
    if (device.notifications & BIT_N(1))
    {
        uint32_t index;

        index = iap_get_trackindex();

        if (device.track_index != index) {
            IAP_TX_INIT(0x03, 0x09);
            IAP_TX_PUT(0x01);
            IAP_TX_PUT_U32(index);
            device.changed_notifications |= BIT_N(1);

            iap_send_tx();

            device.track_index = index;
        }
    }

    /* Chapter index */
    if (device.notifications & BIT_N(2))
    {
        uint32_t index;

        index = iap_get_trackindex();

        if (device.track_index != index)
        {
            IAP_TX_INIT(0x03, 0x09);
            IAP_TX_PUT(0x02);
            IAP_TX_PUT_U32(index);
            IAP_TX_PUT_U16(0);
            IAP_TX_PUT_U16(0xFFFF);
            device.changed_notifications |= BIT_N(2);

            iap_send_tx();

            device.track_index = index;
        }
    }

    /* Play status */
    if (device.notifications & BIT_N(3))
    {
        unsigned char play_status;

        play_status = audio_status();

        if (device.play_status != play_status)
        {
            IAP_TX_INIT(0x03, 0x09);
            IAP_TX_PUT(0x03);
            if (play_status & AUDIO_STATUS_PLAY) {
                /* Playing or paused */
                if (play_status & AUDIO_STATUS_PAUSE) {
                    /* Paused */
                    IAP_TX_PUT(0x02);
                } else {
                    /* Playing */
                    IAP_TX_PUT(0x01);
                }
            } else {
                IAP_TX_PUT(0x00);
            }
            device.changed_notifications |= BIT_N(3);

            iap_send_tx();

            device.play_status = play_status;
        }
    }

    /* Power/Battery */
    if (device.notifications & BIT_N(5))
    {
        unsigned char power_state;
        unsigned char battery_l;

        power_state = charger_input_state;
        battery_l = battery_level();

        if ((device.power_state != power_state) || (device.battery_level != battery_l))
        {
            IAP_TX_INIT(0x03, 0x09);
            IAP_TX_PUT(0x05);

            iap_fill_power_state();
            device.changed_notifications |= BIT_N(5);

            iap_send_tx();

            device.power_state = power_state;
            device.battery_level = battery_l;
        }
    }

    /* Equalizer state
     * This is not handled yet.
     *
     * TODO: Fix equalizer handling
     */

    /* Shuffle */
    if (device.notifications & BIT_N(7))
    {
        unsigned char shuffle;

        shuffle = global_settings.playlist_shuffle;

        if (device.shuffle != shuffle)
        {
            IAP_TX_INIT(0x03, 0x09);
            IAP_TX_PUT(0x07);
            IAP_TX_PUT(shuffle?0x01:0x00);
            device.changed_notifications |= BIT_N(7);

            iap_send_tx();

            device.shuffle = shuffle;
        }
    }

    /* Repeat */
    if (device.notifications & BIT_N(8))
    {
        unsigned char repeat;

        repeat = global_settings.repeat_mode;

        if (device.repeat != repeat)
        {
            IAP_TX_INIT(0x03, 0x09);
            IAP_TX_PUT(0x08);
            switch (repeat)
            {
                case REPEAT_OFF:
                {
                    IAP_TX_PUT(0x00);
                    break;
                }

                case REPEAT_ONE:
                {
                    IAP_TX_PUT(0x01);
                    break;
                }

                case REPEAT_ALL:
                {
                    IAP_TX_PUT(0x02);
                    break;
                }
            }
            device.changed_notifications |= BIT_N(8);

            iap_send_tx();

            device.repeat = repeat;
        }
    }

    /* Date/Time */
    if (device.notifications & BIT_N(9))
    {
        struct tm* tm;

        tm = get_time();

        if (memcmp(tm, &(device.datetime), sizeof(struct tm)))
        {
            IAP_TX_INIT(0x03, 0x09);
            IAP_TX_PUT(0x09);
            IAP_TX_PUT_U16(tm->tm_year);

            /* Month */
            IAP_TX_PUT(tm->tm_mon+1);

            /* Day */
            IAP_TX_PUT(tm->tm_mday);

            /* Hour */
            IAP_TX_PUT(tm->tm_hour);

            /* Minute */
            IAP_TX_PUT(tm->tm_min);

            device.changed_notifications |= BIT_N(9);

            iap_send_tx();

            memcpy(&(device.datetime), tm, sizeof(struct tm));
        }
    }

    /* Alarm
     * This is not supported yet.
     *
     * TODO: Fix alarm handling
     */

    /* Backlight
     * This is not supported yet.
     *
     * TODO: Fix backlight handling
     */

    /* Hold switch */
    if (device.notifications & BIT_N(0x0C))
    {
        unsigned char hold;

        hold = button_hold();
        if (device.hold != hold) {
            IAP_TX_INIT(0x03, 0x09);
            IAP_TX_PUT(0x0C);
            IAP_TX_PUT(hold?0x01:0x00);

            device.changed_notifications |= BIT_N(0x0C);

            iap_send_tx();

            device.hold = hold;
        }
    }

    /* Sound check
     * This is not supported yet.
     *
     * TODO: Fix sound check handling
     */

    /* Audiobook check
     * This is not supported yet.
     *
     * TODO: Fix audiobook handling
     */
}

/* Change the current interface state.
 * On a change from IST_EXTENDED to IST_STANDARD, or from IST_STANDARD
 * to IST_EXTENDED, pause playback, if playing
 */
static void interface_state_change(const enum interface_state new)
{
    if (((interface_state == IST_EXTENDED) && (new == IST_STANDARD)) ||
        ((interface_state == IST_STANDARD) && (new == IST_EXTENDED))) {
        if (audio_status() == AUDIO_STATUS_PLAY)
        {
            REMOTE_BUTTON(BUTTON_RC_PLAY);
        }
    }

    interface_state = new;
}

static void cmd_ack_mode0(const unsigned char cmd, const unsigned char status)
{
    IAP_TX_INIT(0x00, 0x02);
    IAP_TX_PUT(status);
    IAP_TX_PUT(cmd);
    iap_send_tx();
}

#define cmd_ok_mode0(cmd) cmd_ack_mode0((cmd), IAP_ACK_OK)

static void cmd_pending_mode0(const unsigned char cmd, const uint32_t msdelay)
{
    IAP_TX_INIT(0x00, 0x02);
    IAP_TX_PUT(0x06);
    IAP_TX_PUT(cmd);
    IAP_TX_PUT_U32(msdelay);
    iap_send_tx();
}


static void iap_handlepkt_mode0(const unsigned int len, const unsigned char *buf)
{
    unsigned int cmd = buf[1];

    /* We expect at least two bytes in the buffer, one for the
     * lingo, one for the command
     */
    CHECKLEN0(2);

    switch (cmd) {
        /* RequestIdentify (0x00)
         *
         * Sent from the iPod to the device
         */

        /* Identify (0x01)
         * This command is deprecated.
         *
         * It is used by a device to inform the iPod of the devices
         * presence and of the lingo the device supports.
         *
         * Also, it is used to negotiate power for RF transmitters
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: General Lingo, always 0x00
         * 0x01: Command, always 0x01
         * 0x02: Lingo supported by the device
         *
         * Some RF transmitters use an extended version of this
         * command:
         *
         * 0x00: Lingo ID: General Lingo, always 0x00
         * 0x01: Command, always 0x01
         * 0x02: Lingo supported by the device, always 0x05 (RF Transmitter)
         * 0x03: Reserved, always 0x00
         * 0x04: Number of valid bits in the following fields
         * 0x05-N: Datafields holding the number of bits specified in 0x04
         *
         * Returns: (none)
         * 
         * TODO:
         * BeginHighPower/EndHighPower should be send in the periodic handler,
         * depending on the current play status
         */
        case 0x01:
        {
            unsigned char lingo = buf[2];

            /* This is sufficient even for Lingo 0x05, as we are
             * not actually reading from the extended bits for now
             */
            CHECKLEN0(3);

            /* Issuing this command exits any extended interface states
             * and resets authentication
             */
            interface_state_change(IST_STANDARD);
            reset_device(&device);

            switch (lingo) {
                case 0x04:
                {
                    /* A single lingo device negotiating the
                     * extended interface lingo. This causes an interface
                     * state change.
                     */
                    interface_state_change(IST_EXTENDED);
                    break;
                }

                case 0x05: 
                {
                    /* FM transmitter sends this: */
                    /* FF 55 06 00 01 05 00 02 01 F1 (mode switch) */
                    sleep(HZ/3);
                    /* RF Transmitter: Begin transmission */
                    IAP_TX_INIT(0x05, 0x02);

                    iap_send_tx();
                    break;
                }
            }

            if (lingo < 32) {
                /* All devices that Identify get access to Lingoes 0x00 and 0x02 */
                device.lingoes = BIT_N(0x00) | BIT_N(0x02);

                device.lingoes |= BIT_N(lingo);

                /* Devices that Identify with Lingo 0x04 also gain access
                 * to Lingo 0x03
                 */
                if (lingo == 0x04)
                    device.lingoes |= BIT_N(0x03);
            } else {
                device.lingoes = 0;
            }
            break;
        }

        /* ACK (0x02)
         *
         * Sent from the iPod to the device
         */

        /* RequestRemoteUIMode (0x03)
         *
         * Request the current Extended Interface Mode state
         * This command may be used only if the accessory requests Lingo 0x04
         * during its identification process.
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: General Lingo, always 0x00
         * 0x01: Command, always 0x03
         *
         * Returns on success:
         * ReturnRemoteUIMode
         *
         * Packet format (offset in data[]: Description)
         * 0x00: Lingo ID: General Lingo, always 0x00
         * 0x01: Command, always 0x04
         * 0x02: Current Extended Interface Mode (zero: false, non-zero: true)
         *
         * Returns on failure:
         * IAP_ACK_BAD_PARAM
         */
        case 0x03:
        {
            if (!DEVICE_LINGO_SUPPORTED(0x04)) {
                cmd_ack_mode0(cmd, IAP_ACK_BAD_PARAM);
                break;
            }

            IAP_TX_INIT(0x00, 0x04);
            if (interface_state == IST_EXTENDED)
                IAP_TX_PUT(0x01);
            else
                IAP_TX_PUT(0x00);

            iap_send_tx();
            break;
        }

        /* ReturnRemoteUIMode (0x04)
         *
         * Sent from the iPod to the device
         */
    
        /* EnterRemoteUIMode (0x05)
         *
         * Request Extended Interface Mode
         * This command may be used only if the accessory requests Lingo 0x04
         * during its identification process.
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: General Lingo, always 0x00
         * 0x01: Command, always 0x05
         *
         * Returns on success:
         * IAP_ACK_PENDING
         * IAP_ACK_OK
         *
         * Returns on failure:
         * IAP_ACK_BAD_PARAM
         */
        case 0x05:
        {
            if (!DEVICE_LINGO_SUPPORTED(0x04)) {
                cmd_ack_mode0(cmd, IAP_ACK_BAD_PARAM);
                break;
            }

            cmd_pending_mode0(cmd, 1000);
            interface_state_change(IST_EXTENDED);
            cmd_ok_mode0(cmd);
            break;
        }

        /* ExitRemoteUIMode (0x06)
         *
         * Leave Extended Interface Mode
         * This command may be used only if the accessory requests Lingo 0x04
         * during its identification process.
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: General Lingo, always 0x00
         * 0x01: Command, always 0x06
         *
         * Returns on success:
         * IAP_ACK_PENDING
         * IAP_ACK_OK
         *
         * Returns on failure:
         * IAP_ACK_BAD_PARAM
         */
        case 0x06:
        {
            if (!DEVICE_LINGO_SUPPORTED(0x04)) {
                cmd_ack_mode0(cmd, IAP_ACK_BAD_PARAM);
                break;
            }

            cmd_pending_mode0(cmd, 1000);
            interface_state_change(IST_STANDARD);
            cmd_ok_mode0(cmd);
            break;
        }

        /* RequestiPodName (0x07)
         *
         * Retrieves the name of the iPod
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: General Lingo, always 0x00
         * 0x01: Command, always 0x07
         *
         * Returns:
         * ReturniPodName
         *
         * Packet format (offset in data[]: Description)
         * 0x00: Lingo ID: General Lingo, always 0x00
         * 0x01: Command, always 0x08
         * 0x02-0xNN: iPod name as NULL-terminated UTF8 string
         */
        case 0x07:
        {
            IAP_TX_INIT(0x00, 0x08);
            IAP_TX_PUT_STRING("ROCKBOX");

            iap_send_tx();
            break;
        }

        /* ReturniPodName (0x08)
         *
         * Sent from the iPod to the device
         */

        /* RequestiPodSoftwareVersion (0x09)
         *
         * Returns the major, minor and revision numbers of the iPod
         * software version. This not any Lingo protocol version.
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: General Lingo, always 0x00
         * 0x01: Command, always 0x09
         *
         * Returns:
         * ReturniPodSoftwareVersion
         *
         * Packet format (offset in data[]: Description)
         * 0x00: Lingo ID: General Lingo, always 0x00
         * 0x01: Command, always 0x0A
         * 0x02: iPod major software version
         * 0x03: iPod minor software version
         * 0x04: iPod revision software version
         */
        case 0x09:
        {
            IAP_TX_INIT(0x00, 0x0A);
            IAP_TX_PUT(IAP_IPOD_FIRMWARE_MAJOR);
            IAP_TX_PUT(IAP_IPOD_FIRMWARE_MINOR);
            IAP_TX_PUT(IAP_IPOD_FIRMWARE_REV);

            iap_send_tx();
            break;
        } 

        /* ReturniPodSoftwareVersion (0x0A)
         *
         * Sent from the iPod to the device
         */

        /* RequestiPodSerialNum (0x0B)
         *
         * Returns the iPod serial number
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: General Lingo, always 0x00
         * 0x01: Command, always 0x0B
         *
         * Returns:
         * ReturniPodSerialNumber
         *
         * Packet format (offset in data[]: Description)
         * 0x00: Lingo ID: General Lingo, always 0x00
         * 0x01: Command, always 0x0C
         * 0x02-0xNN: Serial number as NULL-terminated UTF8 string
         */
        case 0x0B:
        {
            IAP_TX_INIT(0x00, 0x0C);
            IAP_TX_PUT_STRING("0123456789");

            iap_send_tx();
            break;
        }

        /* ReturniPodSerialNum (0x0C)
         *
         * Sent from the iPod to the device
         */

        /* RequestiPodModelNum (0x0D)
         *
         * Returns the model number as a 32bit unsigned integer and
         * as a string.
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: General Lingo, always 0x00
         * 0x01: Command, always 0x0D
         *
         * Returns:
         * ReturniPodModelNum
         *
         * Packet format (offset in data[]: Description)
         * 0x00: Lingo ID: General Lingo, always 0x00
         * 0x01: Command, always 0x0E
         * 0x02-0x05: Model number as 32bit integer
         * 0x06-0xNN: Model number as NULL-terminated UTF8 string
         */
        case 0x0D:
        {
            IAP_TX_INIT(0x00, 0x0E);
            IAP_TX_PUT_U32(IAP_IPOD_MODEL);
            IAP_TX_PUT_STRING("ROCKBOX");

            iap_send_tx();
            break;
        }

        /* ReturniPodSerialNum (0x0E)
         *
         * Sent from the iPod to the device
         */

        /* RequestLingoProtocolVersion (0x0F)
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: General Lingo, always 0x00
         * 0x01: Command, always 0x0F
         * 0x02: Lingo for which to request version information
         *
         * Returns on success:
         * ReturnLingoProtocolVersion
         *
         * Packet format (offset in data[]: Description)
         * 0x00: Lingo ID: General Lingo, always 0x00
         * 0x01: Command, always 0x10
         * 0x02: Lingo for which version information is returned
         * 0x03: Major protocol version for the given lingo
         * 0x04: Minor protocol version for the given lingo
         * 
         * Returns on failure:
         * IAP_ACK_BAD_PARAM
         */
        case 0x0F:
        {
            unsigned char lingo = buf[2];

            CHECKLEN0(3);

            /* Supported lingos and versions are read from the lingo_versions
             * array
             */
            if (LINGO_SUPPORTED(lingo)) {
                IAP_TX_INIT(0x00, 0x10);
                IAP_TX_PUT(lingo);
                IAP_TX_PUT(LINGO_MAJOR(lingo));
                IAP_TX_PUT(LINGO_MINOR(lingo));

                iap_send_tx();
            } else {
                cmd_ack_mode0(cmd, IAP_ACK_BAD_PARAM);
            }
            break;
        }

        /* ReturnLingoProtocolVersion (0x10)
         *
         * Sent from the iPod to the device
         */

        /* IdentifyDeviceLingoes (0x13);
         *
         * Used by a device to inform the iPod of the devices
         * presence and of the lingoes the device supports.
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: General Lingo, always 0x00
         * 0x01: Command, always 0x13
         * 0x02-0x05: Device lingoes spoken
         * 0x06-0x09: Device options
         * 0x0A-0x0D: Device ID. Only important for authentication
         *
         * Returns on success:
         * IAP_ACK_OK
         *
         * Returns on failure:
         * IAP_ACK_CMD_FAILED
         */
        case 0x13:
        {
            uint32_t lingoes = get_u32(&buf[2]);
            uint32_t options = get_u32(&buf[6]);
            uint32_t deviceid = get_u32(&buf[0x0A]);
            bool seen_unsupported = false;
            unsigned char i;

            CHECKLEN0(14);

            /* Issuing this command exits any extended interface states */
            interface_state_change(IST_STANDARD);

            /* Loop through the lingoes advertised by the device.
             * If it tries to use a lingo we do not support, return
             * a Command Failed ACK.
             */
            for(i=0; i<32; i++) {
                if (lingoes & BIT_N(i)) {
                    /* Bit set by device */
                    if (!LINGO_SUPPORTED(i)) {
                        seen_unsupported = true;
                    }
                }
            }

            /* Bit 0 _must_ be set by the device */
            if (!(lingoes & 1)) {
                seen_unsupported = true;
            }

            /* Specifying a deviceid without requesting authentication is
             * an error
             */
            if (deviceid && !(options & 0x03))
                seen_unsupported = true;

            /* Specifying authentication without a deviceid is an error */
            if (!deviceid && (options & 0x03))
                seen_unsupported = true;

            device.lingoes = 0;
            if (seen_unsupported) {
                cmd_ack_mode0(cmd, IAP_ACK_CMD_FAILED);
                break;
            }
            reset_device(&device);
            device.lingoes = lingoes;

            /* Devices using IdentifyDeviceLingoes get power off notifications */
            device.do_power_notify = true;

            /* If a new authentication is requested, start the auth
             * process.
             * The periodic handler will take care of sending out the
             * GetDevAuthenticationInfo packet
             *
             * If no authentication is requested, schedule the start of
             * GetAccessoryInfo
             */
            if (deviceid && (options & 0x03) && !DEVICE_AUTH_RUNNING) {
                device.auth.state = AUST_INIT;
            } else {
                device.accinfo = ACCST_INIT;
            }

            cmd_ok_mode0(cmd);


#if 0
            /* Bit 5: RF Transmitter lingo */
            if (lingoes & (1 << 5))
            {
                /* FM transmitter sends this: */
                /* FF 55 0E 00 13 00 00 00 35 00 00 00 04 00 00 00 00 A6 (??)*/

                /* GetAccessoryInfo */
                unsigned char data2[] = {0x00, 0x27, 0x00};
                iap_send_pkt(data2, sizeof(data2));
                /* RF Transmitter: Begin transmission */
                unsigned char data3[] = {0x05, 0x02};
                iap_send_pkt(data3, sizeof(data3));
            }

            /* Bit 7: RF Tuner lingo */
            if (lingoes & (1 << 7))
            {
                /* ipod fm remote sends this: */ 
                /* FF 55 0E 00 13 00 00 00 8D 00 00 00 0E 00 00 00 03 41 */
                radio_present = 1;
                /* GetDevAuthenticationInfo */    
                unsigned char data4[] = {0x00, 0x14};
                iap_send_pkt(data4, sizeof(data4));
            }
#endif
            break;
        }

        /* GetDevAuthenticationInfo (0x14)
         *
         * Sent from the iPod to the device
         */

        /* RetDevAuthenticationInfo (0x15)
         *
         * Send certificate information from the device to the iPod.
         * The certificate may come in multiple parts and has
         * to be reassembled.
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: General Lingo, always 0x00
         * 0x01: Command, always 0x15
         * 0x02: Authentication major version
         * 0x03: Authentication minor version
         * 0x04: Certificate current section index
         * 0x05: Certificate maximum section index
         * 0x06-0xNN: Certificate data
         *
         * Returns on success:
         * IAP_ACK_OK for intermediate sections
         * AckDevAuthenticationInfo for the last section
         *
         * Returns on failure:
         * IAP_ACK_BAD_PARAMETER
         * AckDevAuthenticationInfo for version mismatches
         *
         */
        case 0x15:
        {
            /* There are two formats of this packet. One with only
             * the version information bytes (for Auth version 1.0)
             * and the long form shown above
             */
            CHECKLEN0(4);

            if (!DEVICE_AUTH_RUNNING) {
                cmd_ack_mode0(cmd, IAP_ACK_BAD_PARAM);
                break;
            }

            /* We only support version 2.0 */
            if ((buf[2] != 2) || (buf[3] != 0)) {
                /* Version mismatches are signalled by AckDevAuthenticationInfo
                 * with the status set to Authentication Information unsupported
                 */
                reset_auth(&(device.auth));

                IAP_TX_INIT(0x00, 0x16);
                IAP_TX_PUT(0x08);

                iap_send_tx();
                break;
            }

            /* There must be at least one byte of certificate data
             * in the packet
             */
            CHECKLEN0(7);

            switch (device.auth.state)
            {
                /* This is the first packet. Note the maximum section number
                 * so we can check it later.
                 */
                case AUST_CERTREQ:
                {
                    device.auth.max_section = buf[5];
                    device.auth.state = AUST_CERTBEG;

                    /* Intentional fall-through */
                }
                /* All following packets */
                case AUST_CERTBEG:
                {
                    /* Check if this is the expected section */
                    if (buf[4] != device.auth.next_section) {
                        cmd_ack_mode0(cmd, IAP_ACK_BAD_PARAM);
                        break;
                    }

                    /* Is this the last section? */
                    if (device.auth.next_section == device.auth.max_section) {
                        /* If we could really do authentication we'd have to
                         * check the certificate here. Since we can't, just acknowledge
                         * the packet with an "everything OK" AckDevAuthenticationInfo
                         *
                         * Also, start GetAccessoryInfo process
                         */
                        IAP_TX_INIT(0x00, 0x16);
                        IAP_TX_PUT(0x00);

                        iap_send_tx();
                        device.auth.state = AUST_CERTDONE;
                        device.accinfo = ACCST_INIT;
                    } else {
                        device.auth.next_section++;
                        cmd_ok_mode0(cmd);
                    }
                    break;
                }

                default:
                {
                    cmd_ack_mode0(cmd, IAP_ACK_BAD_PARAM);
                    break;
                }
            }

            break;
        }

        /* AckDevAuthenticationInfo (0x16)
         *
         * Sent from the iPod to the device
         */

        /* GetDevAuthenticationSignature (0x17)
         *
         * Sent from the iPod to the device
         */

        /* RetDevAuthenticationSignature (0x18)
         *
         * Return a calculated signature based on the device certificate
         * and the challenge sent with GetDevAuthenticationSignature
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: General Lingo, always 0x00
         * 0x01: Command, always 0x17
         * 0x02-0xNN: Certificate data
         *
         * Returns on success:
         * AckDevAuthenticationStatus
         *
         * Packet format (offset in data[]: Description)
         * 0x00: Lingo ID: General Lingo, always 0x00
         * 0x01: Command, always 0x19
         * 0x02: Status (0x00: OK)
         *
         * Returns on failure:
         * IAP_ACK_BAD_PARAM
         *
         * TODO:
         * There is a timeout of 75 seconds between GetDevAuthenticationSignature
         * and RetDevAuthenticationSignature for Auth 2.0. This is currently not
         * checked.
         */
        case 0x18:
        {
            if (device.auth.state != AUST_CHASENT) {
                cmd_ack_mode0(cmd, IAP_ACK_BAD_PARAM);
                break;
            }

            /* Here we could check the signature. Since we can't, just
             * acknowledge and go to authenticated status
             */
            IAP_TX_INIT(0x00, 0x19);
            IAP_TX_PUT(0x00);

            iap_send_tx();
            device.auth.state = AUST_AUTH;
            break;
        }

        /* AckDevAuthenticationStatus (0x19)
         *
         * Sent from the iPod to the device
         */

        /* GetiPodAuthenticationInfo (0x1A)
         *
         * Obtain authentication information from the iPod.
         * This cannot be implemented without posessing an Apple signed
         * certificate and the corresponding private key.
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: General Lingo, always 0x00
         * 0x01: Command, always 0x1A
         *
         * This command requires authentication
         *
         * Returns:
         * IAP_ACK_CMD_FAILED
         */
        case 0x1A:
        {
            CHECKAUTH0;

            cmd_ack_mode0(cmd, IAP_ACK_CMD_FAILED);
            break;
        }

        /* RetiPodAuthenticationInfo (0x1B)
         *
         * Sent from the iPod to the device
         */

        /* AckiPodAuthenticationInfo (0x1C)
         *
         * Confirm authentication information from the iPod.
         * This cannot be implemented without posessing an Apple signed
         * certificate and the corresponding private key.
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: General Lingo, always 0x00
         * 0x01: Command, always 0x1C
         * 0x02: Authentication state (0x00: OK)
         *
         * This command requires authentication
         *
         * Returns: (none)
         */
        case 0x1C:
        {
            CHECKAUTH0;

            break;
        }

        /* GetiPodAuthenticationSignature (0x1D)
         *
         * Send challenge information to the iPod.
         * This cannot be implemented without posessing an Apple signed
         * certificate and the corresponding private key.
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: General Lingo, always 0x00
         * 0x01: Command, always 0x1D
         * 0x02-0x15: Challenge
         *
         * This command requires authentication
         *
         * Returns:
         * IAP_ACK_CMD_FAILED
         */
        case 0x1D:
        {
            CHECKAUTH0;

            cmd_ack_mode0(cmd, IAP_ACK_CMD_FAILED);
            break;
        }

        /* RetiPodAuthenticationSignature (0x1E)
         *
         * Sent from the iPod to the device
         */

        /* AckiPodAuthenticationStatus (0x1F)
         *
         * Confirm chellenge information from the iPod.
         * This cannot be implemented without posessing an Apple signed
         * certificate and the corresponding private key.
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: General Lingo, always 0x00
         * 0x01: Command, always 0x1C
         * 0x02: Challenge state (0x00: OK)
         *
         * This command requires authentication
         *
         * Returns: (none)
         */
        case 0x1F:
        {
            CHECKAUTH0;

            break;
        }

        /* NotifyiPodStateChange (0x23)
         *
         * Sent from the iPod to the device
         */

        /* GetIpodOptions (0x24)
         *
         * Request supported features of the iPod
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: General Lingo, always 0x00
         * 0x01: Command, always 0x24
         *
         * Retuns:
         * RetiPodOptions
         *
         * Packet format (offset in data[]: Description)
         * 0x00: Lingo ID: General Lingo, always 0x00
         * 0x01: Command, always 0x25
         * 0x02-0x09: Options as a bitfield
         */
        case 0x24:
        {
            /* There are only two features that can be communicated via this
             * function, video support and the ability to control line-out usage.
             * Rockbox supports neither
             */
            IAP_TX_INIT(0x00, 0x25);
            IAP_TX_PUT_U32(0x00);
            IAP_TX_PUT_U32(0x00);

            iap_send_tx();
            break;
        }

        /* RetiPodOptions (0x25)
         *
         * Sent from the iPod to the device
         */

        /* GetAccessoryInfo (0x27)
         *
         * Sent from the iPod to the device
         */

        /* RetAccessoryInfo (0x28)
         *
         * Send information about the device
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: General Lingo, always 0x00
         * 0x01: Command, always 0x28
         * 0x02: Accessory info type
         * 0x03-0xNN: Accessory information (depends on 0x02)
         *
         * Returns: (none)
         *
         * TODO: Actually do something with the information received here.
         * Some devices actually expect us to request the data they
         * offer, so completely ignoring this does not work, either.
         */
        case 0x28:
        {
            CHECKLEN0(3);

            switch (buf[0x02])
            {
                /* Info capabilities */
                case 0x00:
                {
                    CHECKLEN0(7);

                    device.capabilities = get_u32(&buf[0x03]);
                    /* Type 0x00 was already queried, that's where this information comes from */
                    device.capabilities_queried = 0x01;
                    device.capabilities &= ~0x01;
                    break;
                }

                /* For now, ignore all other information */
                default:
                {
                    break;
                }
            }

            /* If there are any unqueried capabilities left, do so */
            if (device.capabilities)
                device.accinfo = ACCST_DATA;

            break;
        }

        /* GetiPodPreferences (0x29)
         *
         * Retrieve information about the current state of the
         * iPod.
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: General Lingo, always 0x00
         * 0x01: Command, always 0x29
         * 0x02: Information class requested
         *
         * This command requires authentication
         *
         * Returns on success:
         * RetiPodPreferences
         *
         * Packet format (offset in data[]: Description)
         * 0x00: Lingo ID: General Lingo, always 0x00
         * 0x01: Command, always 0x2A
         * 0x02: Information class provided
         * 0x03: Information
         *
         * Returns on failure:
         * IAP_ACK_BAD_PARAM
         */
        case 0x29:
        {
            CHECKLEN0(3);
            CHECKAUTH0;

            IAP_TX_INIT(0x00, 0x2A);
            /* The only information really supported is 0x03, Line-out usage.
             * All others are video related
             */
            if (buf[2] == 0x03) {
                IAP_TX_PUT(0x03);
                IAP_TX_PUT(0x01);   /* Line-out enabled */

                iap_send_tx();
            } else {
                cmd_ack_mode0(cmd, IAP_ACK_BAD_PARAM);
            }

            break;
        }

        /* RetiPodPreference (0x2A)
         *
         * Sent from the iPod to the device
         */

        /* SetiPodPreferences (0x2B)
         *
         * Set preferences on the iPod
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: General Lingo, always 0x00
         * 0x01: Command, always 0x29
         * 0x02: Prefecence class requested
         * 0x03: Preference setting
         * 0x04: Restore on exit
         *
         * This command requires authentication
         *
         * Returns on success:
         * IAP_ACK_OK
         *
         * Returns on failure:
         * IAP_ACK_BAD_PARAM
         * IAP_ACK_CMD_FAILED
         */
        case 0x2B:
        {
            CHECKLEN0(5);
            CHECKAUTH0;

            /* The only information really supported is 0x03, Line-out usage.
             * All others are video related
             */
            if (buf[2] == 0x03) {
                /* If line-out disabled is requested, reply with IAP_ACK_CMD_FAILED,
                 * otherwise with IAP_ACK_CMD_OK
                 */
                if (buf[3] == 0x00) {
                    cmd_ack_mode0(cmd, IAP_ACK_CMD_FAILED);
                } else {
                    cmd_ok_mode0(cmd);
                }
            } else {
                cmd_ack_mode0(cmd, IAP_ACK_BAD_PARAM);
            }

            break;
        }
        
        /* The default response is IAP_ACK_BAD_PARAM */
        default:
        {
            cmd_ack_mode0(cmd, IAP_ACK_BAD_PARAM);
            break;
        }
    }
}

/* Lingo 0x02, Simple Remote Lingo
 *
 * TODO:
 * - Fix cmd 0x00 handling, there has to be a more elegant way of doing
 *   this
 */

static void cmd_ack_mode2(const unsigned char cmd, const unsigned char status)
{
    IAP_TX_INIT(0x02, 0x01);
    IAP_TX_PUT(status);
    IAP_TX_PUT(cmd);

    iap_send_tx();
}

#define cmd_ok_mode2(cmd) cmd_ack_mode2((cmd), IAP_ACK_OK)

static void iap_handlepkt_mode2(const unsigned int len, const unsigned char *buf)
{
    unsigned int cmd = buf[1];

    /* We expect at least three bytes in the buffer, one for the
     * lingo, one for the command, and one for the first button
     * state bits.
     */
    CHECKLEN2(3);

    /* Lingo 0x02 must have been negotiated */
    if (!DEVICE_LINGO_SUPPORTED(0x02)) {
        cmd_ack_mode2(cmd, IAP_ACK_BAD_PARAM);
        return;
    }

    switch (cmd)
    {
        /* ContextButtonStatus (0x00)
         *
         * Transmit button events from the device to the iPod
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: Simple Remote Lingo, always 0x02
         * 0x01: Command, always 0x00
         * 0x02: Button states 0:7
         * 0x03: Button states 8:15 (optional)
         * 0x04: Button states 16:23 (optional)
         * 0x05: Button states 24:31 (optional)
         *
         * Returns: (none)
         */
        case 0x00:
        {
            iap_remotebtn = BUTTON_NONE;
            iap_timeoutbtn = 0;
            
            if(buf[2] != 0)
            {
                if(buf[2] & 1)
                    REMOTE_BUTTON(BUTTON_RC_PLAY);
                if(buf[2] & 2)
                    REMOTE_BUTTON(BUTTON_RC_VOL_UP);
                if(buf[2] & 4)
                    REMOTE_BUTTON(BUTTON_RC_VOL_DOWN);
                if(buf[2] & 8)
                    REMOTE_BUTTON(BUTTON_RC_RIGHT);
                if(buf[2] & 16)
                    REMOTE_BUTTON(BUTTON_RC_LEFT);
            }
            else if(len >= 4 && buf[3] != 0)
            {
                if(buf[3] & 1) /* play */
                {
                    if (audio_status() != AUDIO_STATUS_PLAY)
                        REMOTE_BUTTON(BUTTON_RC_PLAY);
                }
                if(buf[3] & 2) /* pause */
                {
                    if (audio_status() == AUDIO_STATUS_PLAY)
                        REMOTE_BUTTON(BUTTON_RC_PLAY);
                }
                if(buf[3] & 128) /* Shuffle */
                {
                    if (!iap_btnshuffle)
                    {
                        iap_shuffle_state(!global_settings.playlist_shuffle);
                        iap_btnshuffle = true;
                    }
                }
            }
            else if(len >= 5 && buf[4] != 0)
            {
                if(buf[4] & 1) /* repeat */
                {
                    if (!iap_btnrepeat)
                    {
                        iap_repeat_next();
                        iap_btnrepeat = true;
                    }
                }

                /* Power off
                 * Not quite sure how to react to this, but stopping playback
                 * is a good start.
                 */
                if (buf[4] & 0x04)
                {
                    if (audio_status() == AUDIO_STATUS_PLAY)
                        REMOTE_BUTTON(BUTTON_RC_PLAY);
                }

                if(buf[4] & 16) /* ffwd */
                    REMOTE_BUTTON(BUTTON_RC_RIGHT);
                if(buf[4] & 32) /* frwd */
                    REMOTE_BUTTON(BUTTON_RC_LEFT);
            }

            break;
        }
        /* ACK (0x01)
         *
         * Sent from the iPod to the device
         */

        /* ImageButtonStatus (0x02)
         *
         * Transmit image button events from the device to the iPod
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: Simple Remote Lingo, always 0x02
         * 0x01: Command, always 0x02
         * 0x02: Button states 0:7
         * 0x03: Button states 8:15 (optional)
         * 0x04: Button states 16:23 (optional)
         * 0x05: Button states 24:31 (optional)
         *
         * This command requires authentication
         *
         * Returns on success:
         * IAP_ACK_OK
         *
         * Returns on failure:
         * IAP_ACK_*
         */
        case 0x02:
        {
            if (!DEVICE_AUTHENTICATED) {
                cmd_ack_mode2(cmd, IAP_ACK_NO_AUTHEN);
                break;
            }

            cmd_ack_mode2(cmd, IAP_ACK_CMD_FAILED);
            break;
        }

        /* VideoButtonStatus (0x03)
         *
         * Transmit video button events from the device to the iPod
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: Simple Remote Lingo, always 0x02
         * 0x01: Command, always 0x03
         * 0x02: Button states 0:7
         * 0x03: Button states 8:15 (optional)
         * 0x04: Button states 16:23 (optional)
         * 0x05: Button states 24:31 (optional)
         *
         * This command requires authentication
         *
         * Returns on success:
         * IAP_ACK_OK
         *
         * Returns on failure:
         * IAP_ACK_*
         */
        case 0x03:
        {
            if (!DEVICE_AUTHENTICATED) {
                cmd_ack_mode2(cmd, IAP_ACK_NO_AUTHEN);
                break;
            }

            cmd_ack_mode2(cmd, IAP_ACK_CMD_FAILED);
            break;
        }

        /* AudioButtonStatus (0x04)
         *
         * Transmit audio button events from the device to the iPod
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: Simple Remote Lingo, always 0x02
         * 0x01: Command, always 0x04
         * 0x02: Button states 0:7
         * 0x03: Button states 8:15 (optional)
         * 0x04: Button states 16:23 (optional)
         * 0x05: Button states 24:31 (optional)
         *
         * This command requires authentication
         *
         * Returns on success:
         * IAP_ACK_OK
         *
         * Returns on failure:
         * IAP_ACK_*
         */
        case 0x04:
        {
            unsigned char repeatbuf[6];

            if (!DEVICE_AUTHENTICATED) {
                cmd_ack_mode2(cmd, IAP_ACK_NO_AUTHEN);
                break;
            }

            /* This is basically the same command as ContextButtonStatus (0x00),
             * with the difference that it requires authentication and that
             * it returns an ACK packet to the device.
             * So just route it through the handler again, with 0x00 as the
             * command
             */
            memcpy(repeatbuf, buf, 6);
            repeatbuf[1] = 0x00;
            iap_handlepkt_mode2((len<6)?len:6, repeatbuf);

            cmd_ok_mode2(cmd);
            break;
        }
        
        /* The default response is IAP_ACK_BAD_PARAM */
        default:
        {
            cmd_ack_mode2(cmd, IAP_ACK_BAD_PARAM);
            break;
        }
    }
}

/* Lingo 0x03: Display Remote Lingo
 *
 * A bit of a hodgepogde of odds and ends.
 *
 * Used to control the equalizer in version 1.00 of the Lingo, but later
 * grew functions to control album art transfer and check the player
 * status.
 *
 * TODO:
 * - Actually support multiple equalizer profiles, currently only the
 *   profile 0 (equalizer disabled) is supported
 */

static void cmd_ack_mode3(const unsigned char cmd, const unsigned char status)
{
    IAP_TX_INIT(0x03, 0x00);
    IAP_TX_PUT(status);
    IAP_TX_PUT(cmd);

    iap_send_tx();
}

#define cmd_ok_mode3(cmd) cmd_ack_mode3((cmd), IAP_ACK_OK)

static void iap_handlepkt_mode3(const unsigned int len, const unsigned char *buf)
{
    unsigned int cmd = buf[1];

    /* We expect at least two bytes in the buffer, one for the
     * state bits.
     */
    CHECKLEN3(2);

    /* Lingo 0x03 must have been negotiated */
    if (!DEVICE_LINGO_SUPPORTED(0x03)) {
        cmd_ack_mode3(cmd, IAP_ACK_BAD_PARAM);
        return;
    }

    switch (cmd)
    {
        /* ACK (0x00)
         *
         * Sent from the iPod to the device 
         */

        /* GetCurrentEQProfileIndex (0x01)
         *
         * Return the index of the current equalizer profile.
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: Display Remote Lingo, always 0x03
         * 0x01: Command, always 0x01
         *
         * Returns:
         * RetCurrentEQProfileIndex
         *
         * Packet format (offset in data[]: Description)
         * 0x00: Lingo ID: Display Remote Lingo, always 0x03
         * 0x01: Command, always 0x02
         * 0x02-0x05: Index as an unsigned 32bit integer
         */
        case 0x01:
        {
            IAP_TX_INIT(0x03, 0x02);
            IAP_TX_PUT_U32(0x00);

            iap_send_tx();
            break;
        }

        /* RetCurrentEQProfileIndex (0x02)
         *
         * Sent from the iPod to the device
         */
        
        /* SetCurrentEQProfileIndex (0x03)
         *
         * Set the active equalizer profile
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: Display Remote Lingo, always 0x03
         * 0x01: Command, always 0x03
         * 0x02-0x05: Profile index to activate
         * 0x06: Whether to restore the previous profile on detach
         *
         * Returns on success:
         * IAP_ACK_OK
         *
         * Returns on failure:
         * IAP_ACK_CMD_FAILED
         *
         * TODO: Figure out return code for invalid index
         */
        case 0x03:
        {
            uint32_t index;

            CHECKLEN3(7);

            index = get_u32(&buf[2]);

            if (index > 0) {
                cmd_ack_mode3(cmd, IAP_ACK_BAD_PARAM);
                break;
            }

            /* Currently, we just ignore the command and acknowledge it */
            cmd_ok_mode3(cmd);
            break;
        }

        /* GetNumEQProfiles (0x04)
         *
         * Get the number of available equalizer profiles
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: Display Remote Lingo, always 0x03
         * 0x01: Command, always 0x04
         *
         * Returns:
         * RetNumEQProfiles
         *
         * Packet format (offset in data[]: Description)
         * 0x00: Lingo ID: Display Remote Lingo, always 0x03
         * 0x01: Command, always 0x05
         * 0x02-0x05: Number as an unsigned 32bit integer
         */
        case 0x04:
        {
            IAP_TX_INIT(0x03, 0x05);
            /* Return one profile (0, the disabled profile) */
            IAP_TX_PUT_U32(0x01);

            iap_send_tx();
            break;
        }

        /* RetNumEQProfiles (0x05)
         *
         * Sent from the iPod to the device
         */

        /* GetIndexedEQProfileName (0x06)
         *
         * Return the name of the indexed equalizer profile
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: Display Remote Lingo, always 0x03
         * 0x01: Command, always 0x06
         * 0x02-0x05: Profile index to get the name of
         *
         * Returns on success:
         * RetIndexedEQProfileName
         *
         * Packet format (offset in data[]: Description)
         * 0x00: Lingo ID: Display Remote Lingo, always 0x03
         * 0x01: Command, always 0x06
         * 0x02-0xNN: Name as an UTF-8 null terminated string
         *
         * Returns on failure:
         * IAP_ACK_BAD_PARAM
         *
         * TODO: Figure out return code for out of range index
         */
        case 0x06:
        {
            uint32_t index;

            CHECKLEN3(6);

            index = get_u32(&buf[2]);

            if (index > 0) {
                cmd_ack_mode3(cmd, IAP_ACK_BAD_PARAM);
                break;
            }
            IAP_TX_INIT(0x03, 0x07);
            IAP_TX_PUT_STRING("Default");

            iap_send_tx();
            break;
        }

        /* RetIndexedQUProfileName (0x07)
         *
         * Sent from the iPod to the device
         */

        /* SetRemoteEventNotification (0x08)
         *
         * Set events the device would like to be notified about
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: Display Remote Lingo, always 0x03
         * 0x01: Command, always 0x08
         * 0x02-0x05: Event bitmask
         *
         * Returns:
         * IAP_ACK_OK
         */
        case 0x08:
        {
            struct tm* tm;

            CHECKLEN3(6);
            CHECKAUTH3;

            /* Save the current state of the various attributes we track */
            device.trackpos_ms = iap_get_trackpos();
            device.track_index = iap_get_trackindex();
            device.chapter_index = 0;
            device.play_status = audio_status();
            /* TODO: Fix this */
            device.mute = false;
            device.volume = 0x80;
            device.power_state = charger_input_state;
            device.battery_level = battery_level();
            /* TODO: Fix this */
            device.equalizer_index = 0;
            device.shuffle = global_settings.playlist_shuffle;
            device.repeat = global_settings.repeat_mode;
            tm = get_time();
            memcpy(&(device.datetime), tm, sizeof(struct tm));
            device.alarm_state = 0;
            device.alarm_hour = 0;
            device.alarm_minute = 0;
            /* TODO: Fix this */
            device.backlight = 0;
            device.hold = button_hold();
            device.soundcheck = 0;
            device.audiobook = 0;
            device.trackpos_s = (device.trackpos_ms/1000) & 0xFFFF;

            /* Get the notification bits */
            device.do_notify = false;
            device.changed_notifications = 0;
            device.notifications = get_u32(&buf[0x02]);
            if (device.notifications)
                device.do_notify = true;

            cmd_ok_mode3(cmd);
            break;
        }

        /* RemoteEventNotification (0x09)
         *
         * Sent from the iPod to the device
         */

        /* GetRemoteEventStatus (0x0A)
         *
         * Request the events changed since the last call to GetREmoteEventStatus
         * or SetRemoteEventNotification
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: Display Remote Lingo, always 0x03
         * 0x01: Command, always 0x0A
         *
         * This command requires authentication
         *
         * Returns:
         * RetRemoteEventNotification
         *
         * Packet format (offset in data[]: Description)
         * 0x00: Lingo ID: Display Remote Lingo, always 0x03
         * 0x01: Command, always 0x0B
         * 0x02-0x05: Event status bits
         */
        case 0x0A:
        {
            CHECKAUTH3;
            IAP_TX_INIT(0x03, 0x0B);
            IAP_TX_PUT_U32(device.changed_notifications);

            iap_send_tx();

            device.changed_notifications = 0;
            break;
        }

        /* RetRemoteEventStatus (0x0B)
         *
         * Sent from the iPod to the device
         */

        /* GetiPodStateInfo (0x0C)
         *
         * Request state information from the iPod
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: Display Remote Lingo, always 0x03
         * 0x01: Command, always 0x0C
         * 0x02: Type information
         *
         * This command requires authentication
         *
         * Returns:
         * RetiPodStateInfo
         *
         * Packet format (offset in data[]: Description)
         * 0x00: Lingo ID: Display Remote Lingo, always 0x03
         * 0x01: Command, always 0x0D
         * 0x02: Type information
         * 0x03-0xNN: State information
         */
        case 0x0C:
        {
            struct mp3entry* id3;
            struct playlist_info* playlist;
            int play_status;
            struct tm* tm;

            CHECKLEN3(3);
            CHECKAUTH3;

            IAP_TX_INIT(0x03, 0x0D);
            IAP_TX_PUT(buf[0x02]);

            switch (buf[0x02])
            {
                /* 0x00: Track position
                 * Data length: 4
                 */
                case 0x00:
                {
                    id3 = audio_current_track();
                    IAP_TX_PUT_U32(id3->elapsed);

                    iap_send_tx();
                    break;
                }

                /* 0x01: Track index
                 * Data length: 4
                 */
                case 0x01:
                {
                    playlist = playlist_get_current();
                    IAP_TX_PUT_U32(playlist->index - playlist->first_index);

                    iap_send_tx();
                    break;
                }

                /* 0x02: Chapter information
                 * Data length: 8
                 */
                case 0x02:
                {
                    playlist = playlist_get_current();
                    IAP_TX_PUT_U32(playlist->index - playlist->first_index);
                    /* Indicate that track does not have chapters */
                    IAP_TX_PUT_U16(0x0000);
                    IAP_TX_PUT_U16(0xFFFF);

                    iap_send_tx();
                    break;
                }

                /* 0x03: Play status
                 * Data length: 1
                 */
                case 0x03:
                {
                    /* TODO: Handle FF/REW
                     */
                    play_status = audio_status();
                    if (play_status & AUDIO_STATUS_PLAY) {
                        if (play_status & AUDIO_STATUS_PAUSE) {
                            IAP_TX_PUT(0x02);
                        } else {
                            IAP_TX_PUT(0x01);
                        }
                    } else {
                        IAP_TX_PUT(0x00);
                    }

                    iap_send_tx();
                    break;
                }

                /* 0x04: Mute/UI/Volume
                 * Data length: 2
                 */
                case 0x04:
                {
                    /* Figuring out what the current volume is
                     * seems to be tricky.
                     * TODO: Fix.
                     */

                    /* Mute status */
                    IAP_TX_PUT(0x00);
                    /* Volume */
                    IAP_TX_PUT(0x80);

                    iap_send_tx();
                    break;
                }

                /* 0x05: Power/Battery
                 * Data length: 2
                 */
                case 0x05:
                {
                    iap_fill_power_state();

                    iap_send_tx();
                    break;
                }

                /* 0x06: Equalizer state
                 * Data length: 4
                 */
                case 0x06:
                {
                    /* Currently only one equalizer setting supported, 0 */
                    IAP_TX_PUT_U32(0x00);

                    iap_send_tx();
                    break;
                }

                /* 0x07: Shuffle
                 * Data length: 1
                 */
                case 0x07:
                {
                    IAP_TX_PUT(global_settings.playlist_shuffle?0x01:0x00);

                    iap_send_tx();
                    break;
                }

                /* 0x08: Repeat
                 * Data length: 1
                 */
                case 0x08:
                {
                    switch (global_settings.repeat_mode)
                    {
                        case REPEAT_OFF:
                        {
                            IAP_TX_PUT(0x00);
                            break;
                        }

                        case REPEAT_ONE:
                        {
                            IAP_TX_PUT(0x01);
                            break;
                        }

                        case REPEAT_ALL:
                        {
                            IAP_TX_PUT(0x02);
                            break;
                        }
                    }

                    iap_send_tx();
                    break;
                }

                /* 0x09: Data/Time
                 * Data length: 6
                 */
                case 0x09:
                {
                    tm = get_time();

                    /* Year */
                    IAP_TX_PUT_U16(tm->tm_year);

                    /* Month */
                    IAP_TX_PUT(tm->tm_mon+1);

                    /* Day */
                    IAP_TX_PUT(tm->tm_mday);

                    /* Hour */
                    IAP_TX_PUT(tm->tm_hour);

                    /* Minute */
                    IAP_TX_PUT(tm->tm_min);

                    iap_send_tx();
                    break;
                }

                /* 0x0A: Alarm
                 * Data length: 3
                 */
                case 0x0A:
                {
                    /* Alarm not supported, always off */
                    IAP_TX_PUT(0x00);
                    IAP_TX_PUT(0x00);
                    IAP_TX_PUT(0x00);

                    iap_send_tx();
                    break;
                }

                /* 0x0B: Backlight
                 * Data length: 1
                 */
                case 0x0B:
                {
                    /* TOOD: Find out how to do this */
                    IAP_TX_PUT(0x00);

                    iap_send_tx();
                    break;
                }

                /* 0x0C: Hold switch
                 * Data length: 1
                 */
                case 0x0C:
                {
                    IAP_TX_PUT(button_hold()?0x01:0x00);

                    iap_send_tx();
                    break;
                }

                /* 0x0D: Sound check
                 * Data length: 1
                 */
                case 0x0D:
                {
                    /* TODO: Find out what the hell this is. Default to off */
                    IAP_TX_PUT(0x00);

                    iap_send_tx();
                    break;
                }

                /* 0x0E: Audiobook
                 * Data length: 1
                 */
                case 0x0E:
                {
                    /* Default to normal */
                    IAP_TX_PUT(0x00);

                    iap_send_tx();
                    break;
                }

                /* 0x0F: Track position in seconds
                 * Data length: 2
                 */
                case 0x0F:
                {
                    unsigned int pos;

                    id3 = audio_current_track();
                    pos = id3->elapsed/1000;

                    IAP_TX_PUT_U16(pos);

                    iap_send_tx();
                    break;
                }

                /* 0x10: Mute/UI/Absolute volume
                 * Data length: 3
                 */
                case 0x10:
                {
                    /* TODO: See volume above */
                    IAP_TX_PUT(0x00);
                    IAP_TX_PUT(0x80);
                    IAP_TX_PUT(0x80);

                    iap_send_tx();
                    break;
                }

                default:
                {
                    cmd_ack_mode3(cmd, IAP_ACK_BAD_PARAM);
                    break;
                }
            }
            break;
        }

        /* RetiPodStateInfo (0x0D)
         *
         * Sent from the iPod to the device
         */

        /* SetiPodStateInfo (0x0E)
         *
         * Set status information to new values
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: Display Remote Lingo, always 0x03
         * 0x01: Command, always 0x0E
         * 0x02: Type of information to change
         * 0x03-0xNN: New information
         *
         * This command requires authentication
         *
         * Returns on success:
         * IAP_ACK_OK
         *
         * Returns on failure:
         * IAP_ACK_CMD_FAILED
         * IAP_ACK_BAD_PARAM
         */
        case 0x0E:
        {
            CHECKLEN3(3);
            CHECKAUTH3;
            switch (buf[0x02])
            {
                /* Track position (ms)
                 * Data length: 4
                 */
                case 0x00:
                {
                    uint32_t pos;

                    CHECKLEN3(7);
                    pos = get_u32(&buf[0x03]);
                    audio_ff_rewind(pos);

                    cmd_ok_mode3(cmd);
                    break;
                }

                /* Track index
                 * Data length: 4
                 */
                case 0x01:
                {
                    uint32_t index;

                    CHECKLEN3(7);
                    index = get_u32(&buf[0x03]);
                    audio_skip(index-iap_get_trackindex());

                    cmd_ok_mode3(cmd);
                    break;
                }

                /* Chapter index
                 * Data length: 2
                 */
                case 0x02:
                {
                    /* This is not supported */
                    cmd_ack_mode3(cmd, IAP_ACK_CMD_FAILED);
                    break;
                }

                /* Play status
                 * Data length: 1
                 */
                case 0x03:
                {
                    CHECKLEN3(4);
                    switch(buf[0x03])
                    {
                        case 0x00:
                        {
                            audio_stop();
                            cmd_ok_mode3(cmd);
                            break;
                        }

                        case 0x01:
                        {
                            audio_resume();
                            cmd_ok_mode3(cmd);
                            break;
                        }

                        case 0x02:
                        {
                            audio_pause();
                            cmd_ok_mode3(cmd);
                            break;
                        }

                        default:
                        {
                            cmd_ack_mode3(cmd, IAP_ACK_CMD_FAILED);
                            break;
                        }
                    }
                    break;
                }

                /* Volume/Mute
                 * Data length: 2
                 * TODO: Fix this
                 */
                case 0x04:
                {
                    CHECKLEN3(5);
                    cmd_ack_mode3(cmd, IAP_ACK_CMD_FAILED);
                    break;
                }

                /* Equalizer
                 * Data length: 5
                 */
                case 0x06:
                {
                    uint32_t index;

                    CHECKLEN3(8);
                    index = get_u32(&buf[0x03]);
                    if (index == 0) {
                        cmd_ok_mode3(cmd);
                    } else {
                        cmd_ack_mode3(cmd, IAP_ACK_BAD_PARAM);
                    }
                    break;
                }

                /* Shuffle
                 * Data length: 2
                 */
                case 0x07:
                {
                    CHECKLEN3(5);

                    switch(buf[0x03])
                    {
                        case 0x00:
                        {
                            iap_shuffle_state(false);
                            cmd_ok_mode3(cmd);
                            break;
                        }
                        case 0x01:
                        case 0x02:
                        {
                            iap_shuffle_state(true);
                            cmd_ok_mode3(cmd);
                            break;
                        }

                        default:
                        {
                            cmd_ack_mode3(cmd, IAP_ACK_BAD_PARAM);
                            break;
                        }
                    }
                    break;
                }

                /* Repeat
                 * Data length: 2
                 */
                case 0x08:
                {
                    CHECKLEN3(5);

                    switch(buf[0x03])
                    {
                        case 0x00:
                        {
                            iap_repeat_state(REPEAT_OFF);
                            cmd_ok_mode3(cmd);
                            break;
                        }
                        case 0x01:
                        {
                            iap_repeat_state(REPEAT_ONE);
                            cmd_ok_mode3(cmd);
                            break;
                        }
                        case 0x02:
                        {
                            iap_repeat_state(REPEAT_ALL);
                            cmd_ok_mode3(cmd);
                            break;
                        }
                        default:
                        {
                            cmd_ack_mode3(cmd, IAP_ACK_BAD_PARAM);
                            break;
                        }
                    }
                    break;
                }

                /* Date/Time
                 * Data length: 6
                 */
                case 0x09:
                {
                    CHECKLEN3(9);
                    
                    cmd_ack_mode3(cmd, IAP_ACK_CMD_FAILED);
                    break;
                }

                /* Alarm
                 * Data length: 4
                 */
                case 0x0A:
                {
                    CHECKLEN3(7);
                    
                    cmd_ack_mode3(cmd, IAP_ACK_CMD_FAILED);
                    break;
                }

                /* Backlight
                 * Data length: 2
                 */
                case 0x0B:
                {
                    CHECKLEN3(5);
                    
                    cmd_ack_mode3(cmd, IAP_ACK_CMD_FAILED);
                    break;
                }

                /* Sound check
                 * Data length: 2
                 */
                case 0x0D:
                {
                    CHECKLEN3(5);
                    
                    cmd_ack_mode3(cmd, IAP_ACK_CMD_FAILED);
                    break;
                }

                /* Audio book speed
                 * Data length: 1
                 */
                case 0x0E:
                {
                    CHECKLEN3(4);
                    
                    cmd_ack_mode3(cmd, IAP_ACK_CMD_FAILED);
                    break;
                }

                /* Track position (s)
                 * Data length: 2
                 */
                case 0x0F:
                {
                    uint16_t pos;

                    CHECKLEN3(5);
                    pos = get_u16(&buf[0x03]);
                    audio_ff_rewind(1000L * pos);

                    cmd_ok_mode3(cmd);
                    break;
                }

                /* Volume/Mute/Absolute
                 * Data length: 4
                 * TODO: Fix this
                 */
                case 0x10:
                {
                    CHECKLEN3(7);
                    cmd_ack_mode3(cmd, IAP_ACK_CMD_FAILED);
                    break;
                }

                default:
                {
                    cmd_ack_mode3(cmd, IAP_ACK_BAD_PARAM);
                    break;
                }
            }

            break;
        }

        /* GetPlayStatus (0x0F)
         *
         * Request the current play status information
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: Display Remote Lingo, always 0x03
         * 0x01: Command, always 0x0F
         *
         * This command requires authentication
         *
         * Returns:
         * RetPlayStatus
         *
         * Packet format (offset in data[]: Description)
         * 0x00: Lingo ID: Display Remote Lingo, always 0x03
         * 0x01: Command, always 0x10
         * 0x02: Play state
         * 0x03-0x06: Current track index
         * 0x07-0x0A: Current track length (ms)
         * 0x0B-0x0E: Current track position (ms)
         */
        case 0x0F:
        {
            int play_status;
            struct mp3entry* id3;
            struct playlist_info* playlist;

            CHECKAUTH3;

            IAP_TX_INIT(0x03, 0x10);

            play_status = audio_status();

            if (play_status & AUDIO_STATUS_PLAY) {
                /* Playing or paused */
                if (play_status & AUDIO_STATUS_PAUSE) {
                    /* Paused */
                    IAP_TX_PUT(0x02);
                } else {
                    /* Playing */
                    IAP_TX_PUT(0x01);
                }
                playlist = playlist_get_current();
                IAP_TX_PUT_U32(playlist->index - playlist->first_index);
                id3 = audio_current_track();
                IAP_TX_PUT_U32(id3->length);
                IAP_TX_PUT_U32(id3->elapsed);
            } else {
                /* Stopped, all values are 0x00 */
                IAP_TX_PUT(0x00);
                IAP_TX_PUT_U32(0x00);
                IAP_TX_PUT_U32(0x00);
                IAP_TX_PUT_U32(0x00);
            }

            iap_send_tx();
            break;
        }

        /* RetPlayStatus (0x10)
         *
         * Sent from the iPod to the device
         */

        /* SetCurrentPlayingTrack (0x11)
         *
         * Set the current playing track
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: Display Remote Lingo, always 0x03
         * 0x01: Command, always 0x11
         * 0x02-0x05: Index of track to play
         *
         * This command requires authentication
         *
         * Returns on success:
         * IAP_ACK_OK
         *
         * Returns on failure:
         * IAP_ACK_BAD_PARAM
         */
        case 0x11:
        {
            uint32_t index;
            uint32_t trackcount;

            CHECKAUTH3;
            CHECKLEN3(6);

            index = get_u32(&buf[0x02]);
            trackcount = playlist_amount();

            if (index >= trackcount)
            {
                cmd_ack_mode3(cmd, IAP_ACK_BAD_PARAM);
                break;
            }
            audio_skip(index-iap_get_trackindex());
            cmd_ok_mode3(cmd);

            break;
        }

        /* GetIndexedPlayingTrackInfo (0x12)
         *
         * Request information about a given track
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: Display Remote Lingo, always 0x03
         * 0x01: Command, always 0x12
         * 0x02: Type of information to retrieve
         * 0x03-0x06: Track index
         * 0x07-0x08: Chapter index
         *
         * This command requires authentication.
         *
         * Returns:
         * RetIndexedPlayingTrackInfo
         *
         * Packet format (offset in data[]: Description)
         * 0x00: Lingo ID: Display Remote Lingo, always 0x03
         * 0x01: Command, always 0x13
         * 0x02: Type of information returned
         * 0x03-0xNN: Information
         */
        case 0x12:
        {
            /* NOTE:
             *
             * Retrieving the track information from a track which is not
             * the currently playing track can take a seriously long time,
             * in the order of several seconds.
             *
             * This most certainly violates the IAP spec, but there's no way
             * around this for now.
             */
            uint32_t track_index;
            struct playlist_track_info track;
            struct mp3entry id3;

            CHECKLEN3(0x09);
            CHECKAUTH3;

            track_index = get_u32(&buf[0x03]);
            if (-1 == playlist_get_track_info(NULL, track_index, &track)) {
                cmd_ack_mode3(cmd, IAP_ACK_BAD_PARAM);
                break;
            }

            IAP_TX_INIT(0x03, 0x13);
            IAP_TX_PUT(buf[2]);
            switch (buf[2])
            {
                /* 0x00: Track caps/info
                 * Information length: 10 bytes
                 */
                case 0x00:
                {
                    iap_get_trackinfo(track_index, &id3);
                    /* Track capabilities. None of these are supported, yet */
                    IAP_TX_PUT_U32(0x00);

                    /* Track length in ms */
                    IAP_TX_PUT_U32(id3.length);

                    /* Chapter count, stays at 0 */
                    IAP_TX_PUT_U16(0x00);

                    iap_send_tx();
                    break;
                }

                /* 0x01: Chapter time/name
                 * Information length: 4+variable
                 */
                case 0x01:
                {
                    /* Chapter length, set at 0 (no chapters) */
                    IAP_TX_PUT_U32(0x00);
                    
                    /* Chapter name, empty */
                    IAP_TX_PUT_STRING("");

                    iap_send_tx();
                    break;
                }

                /* 0x02, Artist name
                 * Information length: variable
                 */
                case 0x02:
                {
                    /* Artist name */
                    iap_get_trackinfo(track_index, &id3);
                    IAP_TX_PUT_STRLCPY(id3.artist);

                    iap_send_tx();
                    break;
                }

                /* 0x03, Album name
                 * Information length: variable
                 */
                case 0x03:
                {
                    /* Album name */
                    iap_get_trackinfo(track_index, &id3);
                    IAP_TX_PUT_STRLCPY(id3.album);

                    iap_send_tx();
                    break;
                }

                /* 0x04, Genre name
                 * Information length: variable
                 */
                case 0x04:
                {
                    /* Genre name */
                    iap_get_trackinfo(track_index, &id3);
                    IAP_TX_PUT_STRLCPY(id3.genre_string);

                    iap_send_tx();
                    break;
                }

                /* 0x05, Track title
                 * Information length: variable
                 */
                case 0x05:
                {
                    /* Track title */
                    iap_get_trackinfo(track_index, &id3);
                    IAP_TX_PUT_STRLCPY(id3.title);

                    iap_send_tx();
                    break;
                }

                /* 0x06, Composer name
                 * Information length: variable
                 */
                case 0x06:
                {
                    /* Track Composer */
                    iap_get_trackinfo(track_index, &id3);
                    IAP_TX_PUT_STRLCPY(id3.composer);

                    iap_send_tx();
                    break;
                }

                /* 0x07, Lyrics
                 * Information length: variable
                 */
                case 0x07:
                {
                    /* Packet information bits. All 0 (single packet) */
                    IAP_TX_PUT(0x00);

                    /* Packet index */
                    IAP_TX_PUT_U16(0x00);

                    /* Lyrics */
                    IAP_TX_PUT_STRING("");

                    iap_send_tx();
                    break;
                }

                /* 0x08, Artwork count
                 * Information length: variable
                 */
                case 0x08:
                {
                    /* No artwork, return packet containing just the type byte */
                    iap_send_tx();
                    break;
                }

                default:
                {
                    cmd_ack_mode3(cmd, IAP_ACK_BAD_PARAM);
                    break;
                }
            }

            break;
        }
        
        /* RetIndexedPlayingTrackInfo (0x13)
         *
         * Sent from the iPod to the device
         */

        /* GetNumPlayingTracks (0x14)
         *
         * Request the number of tracks in the current playlist
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: Display Remote Lingo, always 0x03
         * 0x01: Command, always 0x14
         *
         * This command requires authentication.
         *
         * Returns:
         * RetNumPlayingTracks
         *
         * Packet format (offset in data[]: Description)
         * 0x00: Lingo ID: Display Remote Lingo, always 0x03
         * 0x01: Command, always 0x15
         * 0x02-0xNN: Number of tracks
         */
        case 0x14:
        {
            CHECKAUTH3;
            
            IAP_TX_INIT(0x03, 0x15);
            IAP_TX_PUT_U32(playlist_amount());

            iap_send_tx();
        }

        /* RetNumPlayingTracks (0x15)
         *
         * Sent from the iPod to the device
         */

        /* GetArtworkFormats (0x16)
         *
         * Request a list of supported artwork formats
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: Display Remote Lingo, always 0x03
         * 0x01: Command, always 0x16
         *
         * This command requires authentication.
         *
         * Returns:
         * RetArtworkFormats
         *
         * Packet format (offset in data[]: Description)
         * 0x00: Lingo ID: Display Remote Lingo, always 0x03
         * 0x01: Command, always 0x17
         * 0x02-0xNN: list of 7 byte format descriptors
         */
        case 0x16:
        {
            CHECKAUTH3;

            /* We return the empty list, meaning no artwork
             * TODO: Fix to return actual artwork formats
             */
            IAP_TX_INIT(0x03, 0x17);

            iap_send_tx();
            break;
        }

        /* RetArtworkFormats (0x17)
         *
         * Sent from the iPod to the device
         */

        /* GetTrackArtworkData (0x18)
         *
         * Request artwork for the given track
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: Display Remote Lingo, always 0x03
         * 0x01: Command, always 0x18
         * 0x02-0x05: Track index
         * 0x06-0x07: Format ID
         * 0x08-0x0B: Track offset in ms
         *
         * This command requires authentication.
         *
         * Returns:
         * RetTrackArtworkData
         *
         * Packet format (offset in data[]: Description)
         * 0x00: Lingo ID: Display Remote Lingo, always 0x03
         * 0x01: Command, always 0x19
         * 0x02-0x03: Descriptor index
         * 0x04: Pixel format code
         * 0x05-0x06: Image width in pixels
         * 0x07-0x08: Image height in pixels
         * 0x09-0x0A: Inset rectangle, top left x
         * 0x0B-0x0C: Inset rectangle, top left y
         * 0x0D-0x0E: Inset rectangle, bottom right x
         * 0x0F-0x10: Inset rectangle, bottom right y
         * 0x11-0x14: Row size in bytes
         * 0x15-0xNN: Image data
         *
         * If the image data does not fit in a single packet, subsequent
         * packets omit bytes 0x04-0x14.
         */
        case 0x18:
        {
            CHECKAUTH3;
            CHECKLEN3(0x0C);

            /* No artwork support currently */
            cmd_ack_mode3(cmd, IAP_ACK_BAD_PARAM);
            break;
        }

        /* RetTrackArtworkFormat (0x19)
         *
         * Sent from the iPod to the device
         */

        /* GetPowerBatteryState (0x1A)
         *
         * Request the current power state
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: Display Remote Lingo, always 0x03
         * 0x01: Command, always 0x1A
         *
         * This command requires authentication.
         *
         * Returns:
         * RetPowerBatteryState
         *
         * Packet format (offset in data[]: Description)
         * 0x00: Lingo ID: Display Remote Lingo, always 0x03
         * 0x01: Command, always 0x1B
         * 0x02: Power state
         * 0x03: Battery state
         */
        case 0x1A:
        {
            IAP_TX_INIT(0x03, 0x1B);
            
            iap_fill_power_state();
            iap_send_tx();
            break;
        }

        /* RetPowerBatteryState (0x1B)
         *
         * Sent from the iPod to the device
         */

        /* GetSoundCheckState (0x1C)
         *
         * Request the current sound check state
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: Display Remote Lingo, always 0x03
         * 0x01: Command, always 0x1C
         *
         * This command requires authentication.
         *
         * Returns:
         * RetSoundCheckState
         *
         * Packet format (offset in data[]: Description)
         * 0x00: Lingo ID: Display Remote Lingo, always 0x03
         * 0x01: Command, always 0x1D
         * 0x02: Sound check state
         */
        case 0x1C:
        {
            CHECKAUTH3;

            IAP_TX_INIT(0x03, 0x1D);
            IAP_TX_PUT(0x00);       /* Always off */

            iap_send_tx();
            break;
        }

        /* RetSoundCheckState (0x1D)
         *
         * Sent from the iPod to the device
         */

        /* SetSoundCheckState (0x1E)
         *
         * Set the sound check state
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: Display Remote Lingo, always 0x03
         * 0x01: Command, always 0x1E
         * 0x02: Sound check state
         * 0x03: Restore on exit
         *
         * This command requires authentication.
         *
         * Returns on success
         * IAP_ACK_OK
         *
         * Returns on failure
         * IAP_ACK_CMD_FAILED
         */
        case 0x1E:
        {
            CHECKAUTH3;
            CHECKLEN3(4);
            
            /* Sound check is not supported right now
             * TODO: Fix
             */

            cmd_ack_mode3(cmd, IAP_ACK_CMD_FAILED);
            break;
        }

        /* GetTrackArtworkTimes (0x1F)
         *
         * Request a list of timestamps at which artwork exists in a track
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: Display Remote Lingo, always 0x03
         * 0x01: Command, always 0x1F
         * 0x02-0x05: Track index
         * 0x06-0x07: Format ID
         * 0x08-0x09: Artwork Index
         * 0x0A-0x0B: Artwork count
         *
         * This command requires authentication.
         *
         * Returns:
         * RetTrackArtworkTimes
         *
         * Packet format (offset in data[]: Description)
         * 0x00: Lingo ID: Display Remote Lingo, always 0x03
         * 0x01: Command, always 0x20
         * 0x02-0x05: Offset in ms
         *
         * Bytes 0x02-0x05 can be repeated multiple times
         */
        case 0x1F:
        {
            uint32_t index;

            CHECKAUTH3;
            CHECKLEN3(0x0C);

            /* Artwork is currently unsuported, just check for a valid
             * track index
             */
            index = get_u32(&buf[0x02]);
            if (index >= playlist_amount())
            {
                cmd_ack_mode3(cmd, IAP_ACK_BAD_PARAM);
                break;
            }

            /* Send an empty list */
            IAP_TX_INIT(0x03, 0x20);

            iap_send_tx();
            break;
        }
        
        /* The default response is IAP_ACK_BAD_PARAM */
        default:
        {
            cmd_ack_mode3(cmd, IAP_ACK_BAD_PARAM);
            break;
        }
    }
}

static void cmd_ack_mode4(const unsigned int cmd, const unsigned char status)
{
    IAP_TX_INIT4(0x04, 0x0001);
    IAP_TX_PUT(status);
    IAP_TX_PUT_U16(cmd);

    iap_send_tx();
}

#define cmd_ok_mode4(cmd) cmd_ack_mode4((cmd), IAP_ACK_OK)

static void get_playlist_name(unsigned char *dest,
                              unsigned long item_offset,
                              size_t max_length)
{
    if (item_offset == 0) return;
    DIR* dp;
    struct dirent* playlist_file = NULL;

    dp = opendir(global_settings.playlist_catalog_dir);

    char *extension;
    unsigned long nbr = 0;
    while ((nbr < item_offset) && ((playlist_file = readdir(dp)) != NULL))
    {
        /*Increment only if there is a playlist extension*/
        if ((extension=strrchr(playlist_file->d_name, '.')) != NULL){
            if ((strcmp(extension, ".m3u") == 0 || 
                strcmp(extension, ".m3u8") == 0))
                nbr++;
        }
    }
    if (playlist_file != NULL) {
        strlcpy(dest, playlist_file->d_name, max_length);
    }
    closedir(dp);
}

static void iap_handlepkt_mode4(const unsigned int len, const unsigned char *buf)
{
    unsigned int cmd = (buf[1] << 8) | buf[2];

    /* Lingo 0x04 commands are at least 3 bytes in length */
    CHECKLEN4(3);

    /* Lingo 0x04 must have been negotiated */
    if (!DEVICE_LINGO_SUPPORTED(0x04)) {
        cmd_ack_mode4(cmd, IAP_ACK_BAD_PARAM);
        return;
    }

    /* All these commands require extended interface mode */
    if (interface_state != IST_EXTENDED) {
        cmd_ack_mode4(cmd, IAP_ACK_BAD_PARAM);
        return;
    }

    switch (cmd)
    {
        /* GetAudioBookSpeed */
        case 0x0009:
        {
            /* ReturnAudioBookSpeed */
            unsigned char data[] = {0x04, 0x00, 0x0A, 0x00};
            data[3] = iap_updateflag ? 0 : 1;
            iap_send_pkt(data, sizeof(data));
            break;
        }
        
        /* SetAudioBookSpeed */
        case 0x000B:
        {
            iap_updateflag = buf[3] ? 0 : 1;
            /* respond with cmd ok packet */
            cmd_ok_mode4(cmd);
            break;
        }
        
        /* RequestProtocolVersion (0x0012)
         *
         * This command is deprecated.
         *
         * Return the Extended Interface Protocol version
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: Extended Interface Protocol Lingo, always 0x04
         * 0x01-0x02: Command, always 0x0012
         *
         * Returns:
         * ReturnProtocolVersion
         *
         * Packet format (offset in data[]: Description)
         * 0x00: Lingo ID: Extended Interface Protocol Lingo, always 0x04
         * 0x01-0x02: Command, always 0x0013
         * 0x03: Major protocol version
         * 0x04: Minor protocol version
         */
        case 0x0012:
        {
            IAP_TX_INIT4(0x04, 0x0013);
            IAP_TX_PUT(LINGO_MAJOR(0x04));
            IAP_TX_PUT(LINGO_MINOR(0x04));

            iap_send_tx();
            break;
        }

        /* ReturnProtocolVersion (0x0013)
         *
         * This command is deprecated.
         *
         * Sent from the iPod to the device
         */

        /* RequestiPodName (0x0014)
         *
         * This command is deprecated.
         *
         * Retrieves the name of the iPod
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: Extended Interface Protocol Lingo, always 0x04
         * 0x01-0x02: Command, always 0x0014
         *
         * Returns:
         * ReturniPodName
         *
         * Packet format (offset in data[]: Description)
         * 0x00: Lingo ID: Extended Interface Protocol Lingo, always 0x04
         * 0x01-0x02: Command, always 0x0015
         * 0x03-0xNN: iPod name as NULL-terminated UTF8 string
         */
        case 0x0014:
        {
            IAP_TX_INIT4(0x04, 0x0015);
            IAP_TX_PUT_STRING("ROCKBOX");

            iap_send_tx();
            break;
        }

        /* ReturniPodName (0x0015)
         *
         * This command is deprecated.
         *
         * Sent from the iPod to the device
         */

        /* SelectDBRecord */
        case 0x0017:
        {
            memcpy(cur_dbrecord, buf + 3, 5);
            cmd_ok_mode4(cmd);
            break;
        }

        /* GetNumberCategorizedDBRecords */
        case 0x0018:
        {
            /* ReturnNumberCategorizedDBRecords */
            unsigned char data[] = {0x04, 0x00, 0x19, 0x00, 0x00, 0x00, 0x00};
            unsigned long num = 0;
            
            DIR* dp;
            unsigned long nbr_total_playlists = 0;
            struct dirent* playlist_file = NULL;
            char *extension;

            switch(buf[3]) /* type number */
            {
                case 0x01: /* total number of playlists */
                    dp = opendir(global_settings.playlist_catalog_dir);
                    while ((playlist_file = readdir(dp)) != NULL)
                    {
                    /*Increment only if there is a playlist extension*/
                        if ((extension=strrchr(playlist_file->d_name, '.'))
                        != NULL) {
                            if ((strcmp(extension, ".m3u") == 0 || 
                                strcmp(extension, ".m3u8") == 0))
                                nbr_total_playlists++;
                        }
                    }
                    closedir(dp);
                    /*Add 1 for the main playlist*/
                    num = nbr_total_playlists + 1;
                    break;
                case 0x05: /* total number of songs */
                    num = 1;
                    break;
            }
            put_u32(&data[3], num);
            iap_send_pkt(data, sizeof(data));
            break;
        }
        
        /* RetrieveCategorizedDatabaseRecords */
        case 0x001A:
        {
            /* ReturnCategorizedDatabaseRecord */
            unsigned char data[7 + MAX_PATH] = 
                            {0x04, 0x00, 0x1B, 0x00, 0x00, 0x00, 0x00,
                             'R', 'O', 'C', 'K', 'B', 'O', 'X', '\0'};
            
            unsigned long item_offset = get_u32(&buf[4]);

            get_playlist_name(data + 7, item_offset, MAX_PATH);
            /*Remove file extension*/
            char *dot=NULL;
            dot = (strrchr(data+7, '.'));
            if (dot != NULL)
                *dot = '\0';
            iap_send_pkt(data, 7 + strlen(data+7) + 1);
            break;
        }
        
        /* GetPlayStatus */
        case 0x001C:
        {
            /* ReturnPlayStatus */
            unsigned char data[] = {0x04, 0x00, 0x1D, 0x00, 0x00, 0x00, 
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
            struct mp3entry *id3 = audio_current_track();
            unsigned long time_total = id3->length;
            unsigned long time_elapsed = id3->elapsed;
            int status = audio_status();
            put_u32(&data[3], time_total);
            put_u32(&data[7], time_elapsed);
            if (status == AUDIO_STATUS_PLAY)
                data[11] = 0x01; /* play */
            else if (status & AUDIO_STATUS_PAUSE)
                data[11] = 0x02; /* pause */ 
            iap_send_pkt(data, sizeof(data));
            break;
        }
        
        /* GetCurrentPlayingTrackIndex */
        case 0x001E:
        {
            /* ReturnCurrentPlayingTrackIndex */
            unsigned char data[] = {0x04, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x00};
            long playlist_pos = playlist_next(0);
            playlist_pos -= playlist_get_first_index(NULL);
            if(playlist_pos < 0)
                playlist_pos += playlist_amount();
            put_u32(&data[3], playlist_pos);
            iap_send_pkt(data, sizeof(data));
            break;
        }
        
        /* GetIndexedPlayingTrackTitle */
        case 0x0020:
        /* GetIndexedPlayingTrackArtistName */
        case 0x0022:
        /* GetIndexedPlayingTrackAlbumName */
        case 0x0024:
        {
            unsigned char data[70] = {0x04, 0x00, 0xFF};
            struct mp3entry id3;
            int fd;
            size_t len;
            long tracknum = get_u32(&buf[3]);
            
            data[2] = cmd + 1;
            memcpy(&id3, audio_current_track(), sizeof(id3));
            tracknum += playlist_get_first_index(NULL);
            if(tracknum >= playlist_amount())
                tracknum -= playlist_amount();

            /* If the tracknumber is not the current one,
               read id3 from disk */
            if(playlist_next(0) != tracknum)
            {
                struct playlist_track_info info;
                playlist_get_track_info(NULL, tracknum, &info);
                fd = open(info.filename, O_RDONLY);
                memset(&id3, 0, sizeof(struct mp3entry));
                get_metadata(&id3, fd, info.filename);
                close(fd);
            }

            /* Return the requested track data */
            switch(cmd)
            {
                case 0x20:
                    len = strlcpy((char *)&data[3], id3.title, 64);
                    iap_send_pkt(data, 4+len);
                    break;
                case 0x22:
                    len = strlcpy((char *)&data[3], id3.artist, 64);
                    iap_send_pkt(data, 4+len);
                    break;
                case 0x24:
                    len = strlcpy((char *)&data[3], id3.album, 64);
                    iap_send_pkt(data, 4+len);
                    break;
            }
            break;
        }
        
        /* SetPlayStatusChangeNotification */
        case 0x0026:
        {
            device.do_notify = buf[3] ? true : false;
            /* respond with cmd ok packet */
            cmd_ok_mode4(cmd);
            break;
        }
        
        /* PlayCurrentSelection */
        case 0x0028:
        {
            switch (cur_dbrecord[0])
            {
                case 0x01:
                {/*Playlist*/
                    unsigned long item_offset = get_u32(&cur_dbrecord[1]);
                    
                    unsigned char selected_playlist
                    [sizeof(global_settings.playlist_catalog_dir)
                    + 1
                    + MAX_PATH] = {0};

                    strcpy(selected_playlist,
                            global_settings.playlist_catalog_dir);
                    int len = strlen(selected_playlist);
                    selected_playlist[len] = '/';
                    get_playlist_name (selected_playlist + len + 1,
                                       item_offset,
                                       MAX_PATH);
                    ft_play_playlist(selected_playlist,
                                     global_settings.playlist_catalog_dir,
                                     strrchr(selected_playlist, '/') + 1);
                    break;
                }
            }
            cmd_ok_mode4(cmd);
            break;
        }
        
        /* PlayControl */
        case 0x0029:
        {
            switch(buf[3])
            {
                case 0x01: /* play/pause */
                    iap_remotebtn = BUTTON_RC_PLAY;
                    iap_repeatbtn = 2;
                    break;
                case 0x02: /* stop */
                    iap_remotebtn = BUTTON_RC_PLAY|BUTTON_REPEAT;
                    iap_repeatbtn = 2;
                    break;
                case 0x03: /* skip++ */
                    iap_remotebtn = BUTTON_RC_RIGHT;
                    iap_repeatbtn = 2;
                    break;
                case 0x04: /* skip-- */
                    iap_remotebtn = BUTTON_RC_LEFT;
                    iap_repeatbtn = 2;
                    break;
                case 0x05: /* ffwd */
                    iap_remotebtn = BUTTON_RC_RIGHT;
                    break;
                case 0x06: /* frwd */
                    iap_remotebtn = BUTTON_RC_LEFT;
                    break;
                case 0x07: /* end ffwd/frwd */
                    iap_remotebtn = BUTTON_NONE;
                    break;
            }
            /* respond with cmd ok packet */
            cmd_ok_mode4(cmd);
            break;
        }
        
        /* GetShuffle */
        case 0x002C:
        {
            /* ReturnShuffle */
            unsigned char data[] = {0x04, 0x00, 0x2D, 0x00};
            data[3] = global_settings.playlist_shuffle ? 1 : 0;
            iap_send_pkt(data, sizeof(data));
            break;
        }
        
        /* SetShuffle */
        case 0x002E:
        {
            if(buf[3] && !global_settings.playlist_shuffle)
            {
                global_settings.playlist_shuffle = 1;
                settings_save();
                if (audio_status() & AUDIO_STATUS_PLAY)
                    playlist_randomise(NULL, current_tick, true);
            }
            else if(!buf[3] && global_settings.playlist_shuffle)
            {
                global_settings.playlist_shuffle = 0;
                settings_save();
                if (audio_status() & AUDIO_STATUS_PLAY)
                    playlist_sort(NULL, true);
            }

            /* respond with cmd ok packet */
            cmd_ok_mode4(cmd);
            break;
        }
        
        /* GetRepeat */
        case 0x002F:
        {
            /* ReturnRepeat */
            unsigned char data[] = {0x04, 0x00, 0x30, 0x00};
            if(global_settings.repeat_mode == REPEAT_OFF)
                data[3] = 0;
            else if(global_settings.repeat_mode == REPEAT_ONE)
                data[3] = 1;
            else
                data[3] = 2;
            iap_send_pkt(data, sizeof(data));
            break;
        }
        
        /* SetRepeat */
        case 0x0031:
        {
            int oldmode = global_settings.repeat_mode;
            if (buf[3] == 0)
                global_settings.repeat_mode = REPEAT_OFF;
            else if (buf[3] == 1)
                global_settings.repeat_mode = REPEAT_ONE;
            else if (buf[3] == 2)
                global_settings.repeat_mode = REPEAT_ALL;

            if (oldmode != global_settings.repeat_mode)
            {
                settings_save();
                if (audio_status() & AUDIO_STATUS_PLAY)
                    audio_flush_and_reload_tracks();
            }

            /* respond with cmd ok packet */
            cmd_ok_mode4(cmd);
            break;
        }
        
        /* GetMonoDisplayImageLimits */
        case 0x0033:
        {
            /* ReturnMonoDisplayImageLimits */
            unsigned char data[] = {0x04, 0x00, 0x34,
                                    LCD_WIDTH >> 8, LCD_WIDTH & 0xff,
                                    LCD_HEIGHT >> 8, LCD_HEIGHT & 0xff,
                                    0x01};
            iap_send_pkt(data, sizeof(data));
            break;
        }
        
        /* GetNumPlayingTracks */
        case 0x0035:
        {
            /* ReturnNumPlayingTracks */
            unsigned char data[] = {0x04, 0x00, 0x36, 0x00, 0x00, 0x00, 0x00};
            unsigned long playlist_amt = playlist_amount();
            put_u32(&data[3], playlist_amt);
            iap_send_pkt(data, sizeof(data));
            break;
        }
        
        /* SetCurrentPlayingTrack */
        case 0x0037:
        {
            int paused = (is_wps_fading() || (audio_status() & AUDIO_STATUS_PAUSE));
            long tracknum = get_u32(&buf[3]);
            
            audio_pause();
            audio_skip(tracknum - playlist_next(0));
            if (!paused)
                audio_resume();
            
            /* respond with cmd ok packet */
            cmd_ok_mode4(cmd);
            break;
        }
        
        default:
        {
            /* The default response is IAP_ACK_BAD_PARAM */
            cmd_ack_mode4(cmd, IAP_ACK_BAD_PARAM);
            break;
        }
    }
}

static void iap_handlepkt_mode7(const unsigned int len, const unsigned char *buf)
{
    unsigned int cmd = buf[1];
    switch (cmd)
    {
        /* RetTunerCaps */
        case 0x02:
        {
            /* do nothing */
            
            /* GetAccessoryInfo */
            unsigned char data[] = {0x00, 0x27, 0x00};
            iap_send_pkt(data, sizeof(data));
            break;
        }
        
        /* RetTunerFreq */
        case 0x0A:
            /* fall through */
        /* TunerSeekDone */
        case 0x13:
        {
            rmt_tuner_freq(len, buf);
            break;
        }
        
        /* RdsReadyNotify, RDS station name 0x21 1E 00 + ASCII text*/
        case 0x21:
        {
            rmt_tuner_rds_data(len, buf);
            break;
        }
    }
}

void iap_handlepkt(void)
{
    int level;
    int length;

    if(!iap_setupflag) return;

    /* if we are waiting for a remote button to go out,
       delay the handling of the new packet */
    if(iap_repeatbtn)
    {
        queue_post(&iap_queue, IAP_EV_MSG_RCVD, 0);
        sleep(1);
        return;
    }

    /* handle command by mode */
    length = get_u16(iap_rxstart);
    unsigned char mode = *(iap_rxstart+2);
    switch (mode) {
    case 0: iap_handlepkt_mode0(length, iap_rxstart+2); break;
    case 2: iap_handlepkt_mode2(length, iap_rxstart+2); break;
    case 3: iap_handlepkt_mode3(length, iap_rxstart+2); break;
    case 4: iap_handlepkt_mode4(length, iap_rxstart+2); break;
    case 7: iap_handlepkt_mode7(length, iap_rxstart+2); break;
    }

    /* Remove the handled packet from the RX buffer
     * This needs to be done with interrupts disabled, to make
     * sure the buffer and the pointers into it are handled
     * cleanly
     */
    level = disable_irq_save();
    memmove(iap_rxstart, iap_rxstart+(length+2), (RX_BUFLEN+2)-(length+2));
    iap_rxnext -= (length+2);
    iap_rxpayload -= (length+2);
    iap_rxlen += (length+2);
    restore_irq(level);

    /* poke the poweroff timer */
    reset_poweroff_timer();
}

int remote_control_rx(void)
{
    int btn = iap_remotebtn;
    if(iap_repeatbtn)
        iap_repeatbtn--;

    return btn;
}

const unsigned char *iap_get_serbuf(void)
{
    return iap_rxstart;
}

#ifdef IAP_MALLOC_DYNAMIC
static int iap_move_callback(int handle, void* current, void* new)
{
    (void) handle;
    (void) current;

    iap_txstart = new;
    iap_txpayload = iap_txstart+5;
    iap_txnext = iap_txpayload;
    iap_rxstart = iap_buffers+(TX_BUFLEN+6);

    return BUFLIB_CB_OK;
}
#endif

/* Change the shuffle state */
static void iap_shuffle_state(const bool state)
{
    /* Set shuffle to enabled */
    if(state && !global_settings.playlist_shuffle)
    {
        global_settings.playlist_shuffle = 1;
        settings_save();
        if (audio_status() & AUDIO_STATUS_PLAY)
            playlist_randomise(NULL, current_tick, true);
    }
    /* Set shuffle to disabled */
    else if(!state && global_settings.playlist_shuffle)
    {
        global_settings.playlist_shuffle = 0;
        settings_save();
        if (audio_status() & AUDIO_STATUS_PLAY)
            playlist_sort(NULL, true);
    }
}

/* Change the repeat state */
static void iap_repeat_state(const unsigned char state)
{
    if (state != global_settings.repeat_mode)
    {
        global_settings.repeat_mode = state;
        settings_save();
        if (audio_status() & AUDIO_STATUS_PLAY)
            audio_flush_and_reload_tracks();
    }
}

static void iap_repeat_next(void)
{
    switch (global_settings.repeat_mode)
    {
        case REPEAT_OFF:
        {
            iap_repeat_state(REPEAT_ALL);
            break;
        }
        case REPEAT_ALL:
        {
            iap_repeat_state(REPEAT_ONE);
            break;
        }
        case REPEAT_ONE:
        {
            iap_repeat_state(REPEAT_OFF);
            break;
        }
    }
}

/* This function puts the current power/battery state
 * into the TX buffer. The buffer is assumed to be initialized
 */
static void iap_fill_power_state(void)
{
    unsigned char power_state;
    unsigned char battery_l;

    power_state = charger_input_state;
    battery_l = battery_level();

    if (power_state == NO_CHARGER) {
        if (battery_l < 30) {
            IAP_TX_PUT(0x00);
        } else {
            IAP_TX_PUT(0x01);
        }
        IAP_TX_PUT((char)((battery_l * 255)/100));
    } else {
        IAP_TX_PUT(0x04);
        IAP_TX_PUT(0x00);
    }
}
