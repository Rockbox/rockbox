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
#include "iap-core.h"
#include "iap-lingo.h"
#include "button.h"
#include "config.h"
#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "thread.h"
#include "serial.h"
#include "appevents.h"
#include "core_alloc.h"

#include "playlist.h"
#include "playback.h"
#include "audio.h"
#include "settings.h"
#include "metadata.h"
#include "sound.h"
#include "action.h"
#include "powermgmt.h"
#include "usb.h"

#include "tuner.h"
#include "ipod_remote_tuner.h"


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

/* Events in the iap_queue */
#define IAP_EV_TICK         (1)     /* The regular task timeout */
#define IAP_EV_MSG_RCVD     (2)     /* A complete message has been received from the device */
#define IAP_EV_MALLOC       (3)     /* Allocate memory for the RX/TX buffers */

static bool iap_started = false;
static bool iap_setupflag = false, iap_running = false;
/* This is set to true if a SYS_POWEROFF message is received,
 * signalling impending power off
 */
static bool iap_shutdown = false;
static struct timeout iap_task_tmo;

unsigned long iap_remotebtn = 0;
/* Used to make sure a button press is delivered to the processing
 * backend. While this is !0, no new incoming messasges are processed.
 * Counted down by remote_control_rx()
 */
int iap_repeatbtn = 0;
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
unsigned int iap_timeoutbtn = 0;
bool iap_btnrepeat = false, iap_btnshuffle = false;

static long thread_stack[(DEFAULT_STACK_SIZE*6)/sizeof(long)];
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
unsigned char* iap_txpayload;
unsigned char* iap_txnext;

/* The versions of the various Lingoes we support. A major version
 * of 0 means unsupported
 */
unsigned char lingo_versions[32][2] = {
    {1, 9},     /* General lingo, 0x00 */
#ifdef HAVE_LINE_REC
    {1, 1},     /* Microphone lingo, 0x01 */
#else
    {0, 0},     /* Microphone lingo, 0x01, disabled */
#endif
    {1, 2},     /* Simple remote lingo, 0x02 */
    {1, 5},     /* Display remote lingo, 0x03 */
    {1, 12},    /* Extended Interface lingo, 0x04 */
    {1, 1},     /* RF/BT Transmitter lingo, 0x05 */
    {}          /* All others are unsupported */
};

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

enum interface_state interface_state = IST_STANDARD;

struct device_t device;

#ifdef IAP_MALLOC_DYNAMIC
static int iap_move_callback(int handle, void* current, void* new);

static struct buflib_callbacks iap_buflib_callbacks = {
    iap_move_callback,
    NULL
};
#endif

static void iap_malloc(void);

void put_u16(unsigned char *buf, const uint16_t data)
{
    buf[0] = (data >>  8) & 0xFF;
    buf[1] = (data >>  0) & 0xFF;
}

void put_u32(unsigned char *buf, const uint32_t data)
{
    buf[0] = (data >> 24) & 0xFF;
    buf[1] = (data >> 16) & 0xFF;
    buf[2] = (data >>  8) & 0xFF;
    buf[3] = (data >>  0) & 0xFF;
}

uint32_t get_u32(const unsigned char *buf)
{
    return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

uint16_t get_u16(const unsigned char *buf)
{
    return (buf[0] << 8) | buf[1];
}

#if defined(LOGF_ENABLE) && defined(ROCKBOX_HAS_LOGF)
/* Convert a buffer into a printable string, perl style
 * buf contains the data to be converted, len is the length
 * of the buffer.
 *
 * This will convert at most 1024 bytes from buf
 */
static char* hexstring(const unsigned char *buf, unsigned int len) {
    static char hexbuf[4097];
    unsigned int l;
    const unsigned char* p;
    unsigned char* out;
    unsigned char h[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                         '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    if (len > 1024) {
        l = 1024;
    } else {
        l = len;
    }
    p = buf;
    out = hexbuf;
    do {
            *out++ = h[(*p)>>4];
            *out++ = h[*p & 0x0F];
    } while(--l && p++);

    *out = 0x00;

    return hexbuf;
}
#endif


void iap_tx_strlcpy(const unsigned char *str)
{
    ptrdiff_t txfree;
    int r;

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

void iap_reset_auth(struct auth_t* auth)
{
    auth->state = AUST_NONE;
    auth->max_section = 0;
    auth->next_section = 0;
}

void iap_reset_device(struct device_t* device)
{
    iap_reset_auth(&(device->auth));
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

            /* Ack USB thread */
            case SYS_USB_CONNECTED:
            {
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                break;
            }
        }
    }
}

/* called by playback when the next track starts */
static void iap_track_changed(unsigned short id, void *ignored)
{
    (void)id;
    (void)ignored;
    if ((interface_state == IST_EXTENDED) && device.do_notify) {
        long playlist_pos = playlist_next(0);
        playlist_pos -= playlist_get_first_index(NULL);
        if(playlist_pos < 0)
            playlist_pos += playlist_amount();

        IAP_TX_INIT4(0x04, 0x0027);
        IAP_TX_PUT(0x01);
        IAP_TX_PUT_U32(playlist_pos);

        iap_send_tx();
        return;
    }
}

/* Do general setup of the needed infrastructure.
 *
 * Please note that a lot of additional work is done by iap_start()
 */
void iap_setup(const int ratenum)
{
    iap_bitrate_set(ratenum);
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

    iap_reset_device(&device);
    queue_init(&iap_queue, true);
    tid = create_thread(iap_thread, thread_stack, sizeof(thread_stack),
            0, "iap"
            IF_PRIO(, PRIORITY_SYSTEM)
            IF_COP(, CPU));
    if (!tid)
        panicf("Could not create iap thread");
    timeout_register(&iap_task_tmo, iap_task, MS_TO_TICKS(100), (intptr_t)NULL);
    add_event(PLAYBACK_EVENT_TRACK_CHANGE, iap_track_changed);

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
void iap_send_tx(void)
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

#if defined(LOGF_ENABLE) && defined(ROCKBOX_HAS_LOGF)
    logf("T: %s", hexstring(txstart+3, (iap_txnext - txstart)-3));
#endif
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
#ifdef LOGF_ENABLE
           logf("Unhandled iap state %d", (int) s->state);
#else
           panicf("Unhandled iap state %d", (int) s->state);
#endif
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

uint32_t iap_get_trackpos(void)
{
    struct mp3entry *id3 = audio_current_track();

    return id3->elapsed;
}

uint32_t iap_get_trackindex(void)
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
            IAP_TX_PUT_DATA(iap_rxstart,
                        (device.auth.version == 0x100) ? 16 : 20);
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
        iap_reset_device(&device);
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

    /* Mode 04 PlayStatusChangeNotification */
    /* Are we in Extended Mode */
    if (interface_state == IST_EXTENDED) {
        /* Return Track Position */
        struct mp3entry *id3 = audio_current_track();
        unsigned long time_elapsed = id3->elapsed;
        IAP_TX_INIT4(0x04, 0x0027);
        IAP_TX_PUT(0x04);
        IAP_TX_PUT_U32(time_elapsed);

        iap_send_tx();
    }

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
void iap_interface_state_change(const enum interface_state new)
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

static void iap_handlepkt_mode5(const unsigned int len, const unsigned char *buf)
{
    (void) len;
    unsigned int cmd = buf[1];
    switch (cmd)
    {
        /* Sent from iPod Begin Transmission */
        case 0x02:
        {
            /* RF Transmitter: Begin High Power transmission */
            unsigned char data0[] = {0x05, 0x02};
            iap_send_pkt(data0, sizeof(data0));
            break;
        }

        /* Sent from iPod End High Power Transmission */
        case 0x03:
        {
            /* RF Transmitter: End High Power transmission */
            unsigned char data1[] = {0x05, 0x03};
            iap_send_pkt(data1, sizeof(data1));
            break;
        }
        /* Return Version Number ?*/
        case 0x04:
        {
            /* do nothing */
            break;
        }
    }
}

#if 0
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
#endif

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
#if defined(LOGF_ENABLE) && defined(ROCKBOX_HAS_LOGF)
    logf("R: %s", hexstring(iap_rxstart+2, (length)));
#endif

    unsigned char mode = *(iap_rxstart+2);
    switch (mode) {
    case 0: iap_handlepkt_mode0(length, iap_rxstart+2); break;
#ifdef HAVE_LINE_REC
    case 1: iap_handlepkt_mode1(length, iap_rxstart+2); break;
#endif
    case 2: iap_handlepkt_mode2(length, iap_rxstart+2); break;
    case 3: iap_handlepkt_mode3(length, iap_rxstart+2); break;
    case 4: iap_handlepkt_mode4(length, iap_rxstart+2); break;
    case 5: iap_handlepkt_mode5(length, iap_rxstart+2); break;
    /* case 7: iap_handlepkt_mode7(length, iap_rxstart+2); break; */
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
void iap_shuffle_state(const bool state)
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
void iap_repeat_state(const unsigned char state)
{
    if (state != global_settings.repeat_mode)
    {
        global_settings.repeat_mode = state;
        settings_save();
        if (audio_status() & AUDIO_STATUS_PLAY)
            audio_flush_and_reload_tracks();
    }
}

void iap_repeat_next(void)
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
void iap_fill_power_state(void)
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
