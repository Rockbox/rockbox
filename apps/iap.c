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

/* Add a button to the remote button bitfield. Also set iap_repeatbtn=2
 * if it's not already set
 */
#define REMOTE_BUTTON(x) do { \
        iap_remotebtn |= (x); \
        iap_timeoutbtn = 3; \
        iap_repeatbtn = 0; \
        } while(0)
        
/* Used to control the rate from calls to iap_periodic (roughly 10HZ) to
 * notifications sent to the device (normally 2HZ, higher for certain
 * events, for example volume changes). The maximum useful value is 5,
 * which sets the notification rate to 5HZ.
 * Don't set this to 0 to disable notifications, use device.do_notify for
 * that
 */
static volatile int iap_pollspeed = 0;
static bool iap_setupflag = false, iap_updateflag = false, iap_running = false;
/* This is set to true if a SYS_POWEROFF message is received,
 * signalling impending power off
 */
static bool iap_shutdown = false;
static int iap_changedctr = 0;
static struct timeout iap_task_tmo;

static unsigned long iap_remotebtn = 0;
/* Used for one-shot buttons (buttons that go out by themselves after a
 * short time, without the device having to send an explicit "button up"
 * event.
 * If !0, the button will go out on it's own
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

static long thread_stack[DEFAULT_STACK_SIZE/sizeof(long)];
static struct event_queue iap_queue;

static unsigned char serbuf[RX_BUFLEN];
static volatile bool serbuf_lock = false;

static unsigned char response[TX_BUFLEN];

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
    unsigned char *payload; /* payload data pointer */
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
    {1, 5},     /* General lingo, 0x00 */
    {0, 0},     /* Microphone lingo, 0x01, unsupported */
    {1, 2},     /* Simple remote lingo, 0x02 */
    {1, 0},     /* Display remote lingo, 0x03 */
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
    ACCST_INIT,     /* Send out GetAccessoryInfo */
    ACCST_SENT,     /* Wait for RetAccessoryInfo */
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
    bool do_notify;                 /* Notifications enabled */
    bool do_power_notify;           /* Whether to send power change notifications.
                                     * These are sent automatically to all devices
                                     * that used IdentifyDeviceLingoes to identify
                                     * themselves, independent of other notifications
                                     */
} device;
#define DEVICE_AUTHENTICATED (device.auth.state == AUST_AUTH)
#define DEVICE_AUTH_RUNNING (device.auth.state != AUST_NONE)
#define DEVICE_LINGO_SUPPORTED(x) (device.lingoes & BIT_N((x)&0x1f))

static void put_u32(unsigned char *buf, uint32_t data)
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
    device->do_notify = false;
    device->do_power_notify = false;
    device->accinfo = ACCST_NONE;
}

static int iap_task(struct timeout *tmo __attribute__ ((unused)))
{
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
    iap_changedctr = 1;
}

/* Do general setup of the needed infrastructure.
 *
 * Please note that a lot of additional work is done by iap_start()
 */
void iap_setup(int ratenum)
{
    iap_bitrate_set(ratenum);
    iap_pollspeed = 1;
    iap_updateflag = false;
    iap_changedctr = 0;
    iap_remotebtn = BUTTON_NONE;
    iap_setupflag = true;
}

/* Actually bring up the message queue, message handler thread and
 * notification timer
 */
static void iap_start(void)
{
    unsigned int tid;
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
    iap_running = true;
}

void iap_bitrate_set(int ratenum)
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

void iap_send_pkt(const unsigned char * data, int len)
{
    int i, chksum, responselen;
    
    if(len > TX_BUFLEN-4) len = TX_BUFLEN-4;
    responselen = len + 4;
    
    response[0] = 0xFF;
    response[1] = 0x55;
    
    chksum = response[2] = len;
    for(i = 0; i < len; i ++)
    {
        chksum += data[i];
        response[i+3] = data[i];
    }

    response[i+3] = 0x100 - (chksum & 0xFF);
    
    for(i = 0; i < responselen; i ++)
    {
        while (!tx_rdy()) ;
        tx_writec(response[i]);
    }
}    

bool iap_getc(unsigned char x)
{
    struct state_t *s = &frame_state;
    static long pkt_timeout;

    /* Check the time since the last packet arrived. */
    if ((s->state != ST_SYNC) && TIME_AFTER(current_tick, pkt_timeout)) {
        /* Packet timeouts only make sense while not waiting for the
         * sync byte */
         serbuf_lock = false;
         s->state = ST_SYNC;
         return iap_getc(x);
    }

    
    /* run state machine to detect and extract a valid frame */
    switch (s->state) {
    case ST_SYNC:
        if (x == 0xFF) {
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
        /* try to get a lock on serbuf */
        if (serbuf_lock) {
            s->state = ST_SYNC;
            break;
        }
        serbuf_lock = true;
    
        s->check = x;
        s->count = 0;
        s->payload = serbuf;
        if (x == 0) {
            /* large packet */
            s->state = ST_LENH;
        } else {
            /* small packet */
            s->len = x;
            s->state = ST_DATA;
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
        if ((s->len == 0) || (s->len > RX_BUFLEN)) {
            /* invalid length */
            serbuf_lock = false;
            s->state = ST_SYNC;
            return iap_getc(x);
        } else {
            s->state = ST_DATA;
        }
        break;
    case ST_DATA:
        s->check += x;
        s->payload[s->count++] = x;
        if (s->count == s->len) {
            s->state = ST_CHECK;
        }
        break;
    case ST_CHECK:
        s->check += x;
        if ((s->check & 0xFF) == 0) {
            /* done, received a valid frame */
            /* The IAP handler infrastructure may not actually be running yet,
             * it's started by the first successfully decoded packet
             */
            if (!iap_running)
                iap_start();
            queue_post(&iap_queue, IAP_EV_MSG_RCVD, 0);
        }
        s->state = ST_SYNC;
        serbuf_lock = false;
        break;
    default:
        panicf("Unhandled iap state %d", (int) s->state);
        break;
    }

    pkt_timeout = current_tick + IAP_PKT_TIMEOUT;
    
    /* return true while still hunting for the sync and start-of-frame byte */
    return (s->state == ST_SYNC) || (s->state == ST_SOF);
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
            unsigned char data[] = {0x00, 0x14};

            iap_send_pkt(data, sizeof(data));
            device.auth.state = AUST_CERTREQ;
            break;
        }

        case AUST_CERTDONE:
        {
            /* Send out GetDevAuthenticationSignature, with
             * 20 bytes of challenge and a retry counter of 1
             */
            unsigned char data[] = {0x00, 0x17, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x01};

            iap_send_pkt(data, sizeof(data));
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
    }

    /* Handle power down messages. */
    if (iap_shutdown && device.do_power_notify)
    {
        /* NotifyiPodStateChange */
        unsigned char data[] = {0x00, 0x23, 0x01};

        iap_send_pkt(data, sizeof(data));
        iap_shutdown = false;
    }

    /* Handle GetAccessoryInfo messages */
    if (device.accinfo == ACCST_INIT)
    {
        /* GetAccessoryInfo */
        unsigned char data[] = {0x00, 0x27, 0x00};

        iap_send_pkt(data, sizeof(data));
        device.accinfo = ACCST_SENT;
    }


    if (!device.do_notify) return;
    if (device.notifications == 0) return;

    /* Even if iap_periodic is called every 100ms, notifications
     * are sent on a longer timescale.
     */
    count += iap_pollspeed;
    if (count < 5) return;

    count = 0;
    
    /* PlayStatusChangeNotification */
    unsigned char data[] = {0x04, 0x00, 0x27, 0x04, 0x00, 0x00, 0x00, 0x00};
    unsigned long time_elapsed = audio_current_track()->elapsed;

    time_elapsed += wps_get_ff_rewind_count();

    data[3] = 0x04; /* playing */

    /* If info has changed, don't flag it right away */
    if(iap_changedctr && iap_changedctr++ >= iap_pollspeed * 2)
    {
        /* track info has changed */
        iap_changedctr = 0;
        data[3] = 0x01; /* 0x02 has same effect?  */
        iap_updateflag = true;
    }

    put_u32(&data[4], time_elapsed);
    iap_send_pkt(data, sizeof(data));
}

static void iap_set_remote_volume(void)
{
    unsigned char data[] = {0x03, 0x0D, 0x04, 0x00, 0x00};
    data[4] = (char)((global_settings.volume+58) * 4);
    iap_send_pkt(data, sizeof(data));
}

/* Change the current interface state.
 * On a change from IST_EXTENDED to IST_STANDARD, or from IST_STANDARD
 * to IST_EXTENDED, pause playback, if playing
 */
static void interface_state_change(enum interface_state new)
{
    if (((interface_state == IST_EXTENDED) && (new == IST_STANDARD)) ||
        ((interface_state == IST_STANDARD) && (new == IST_EXTENDED))) {
        if (audio_status() == AUDIO_STATUS_PLAY)
        {
            REMOTE_BUTTON(BUTTON_RC_PLAY);
            iap_repeatbtn = 2;
            iap_changedctr = 1;
        }
    }

    interface_state = new;
}

static void cmd_ack_mode0(unsigned char cmd, unsigned char status)
{
    unsigned char data[] = {0x00, 0x02, status, cmd};
    iap_send_pkt(data, sizeof(data));
}

static void cmd_ok_mode0(unsigned char cmd)
{
    cmd_ack_mode0(cmd, IAP_ACK_OK);
}

static void cmd_pending_mode0(unsigned char cmd, uint32_t msdelay) {
    unsigned char data[] = {0x00, 0x02, 0x06, cmd, 0x00, 0x00, 0x00, 0x00};

    put_u32(&data[4], msdelay);
    iap_send_pkt(data, sizeof(data));
}


static void iap_handlepkt_mode0(unsigned int len, const unsigned char *buf)
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
            reset_auth(&(device.auth));

            /* Devices using Identify do not get power off notifications */
            device.do_power_notify = false;
            
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
                    unsigned char data[] = {0x05, 0x02};
                    iap_send_pkt(data, sizeof(data));
                    break;
                }
            }

            if (lingo < 32) {
                device.lingoes = BIT_N(lingo);
                if (lingo == 0) {
                    /* For historical reasons, Identify with a Lingo of 0x00 also
                     * enables access to Lingo 0x02
                     */
                    device.lingoes = 0 | BIT_N(0x02);
                }
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
            unsigned char data[] = {0x00, 0x04, 0x00};

            if (!DEVICE_LINGO_SUPPORTED(0x04)) {
                cmd_ack_mode0(cmd, IAP_ACK_BAD_PARAM);
                break;
            }

            if (interface_state == IST_EXTENDED) {
                data[2] = 0x01;
            }
            iap_send_pkt(data, sizeof(data));
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
            unsigned char data[] = {0x00, 0x08, 'R', 'O', 'C', 'K', 'B', 'O', 'X', 0x00};

            iap_send_pkt(data, sizeof(data));
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
            /* ipod5G firmware version */
            unsigned char data[] = {0x00, 0x0A, 0x01, 0x02, 0x01};

            iap_send_pkt(data, sizeof(data));
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
            unsigned char data[] = {0x00, 0x0C, '0', '1', '2', '3', '4', '5', '6', '7', '8', 0x00};

            iap_send_pkt(data, sizeof(data));
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
            /* ipod is supposed to work only with 5G and nano 2G */
            /*{0x00, 0x0E, 0x00, 0x0B, 0x00, 0x05, 0x50, 0x41, 0x31, 0x34, 
                    0x37, 0x4C, 0x4C, 0x00};    PA147LL (IPOD 5G 60 GO) */
            /* ReturniPodModelNum */
            unsigned char data[] = {0x00, 0x0E, 0x00, 0x0B, 0x00, 0x10,
                                'R', 'O', 'C', 'K', 'B', 'O', 'X', 0x00};

            iap_send_pkt(data, sizeof(data));
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
            /* ReturnLingoProtocolVersion */
            unsigned char data[] = {0x00, 0x10, 0x00, 0x00, 0x00};
            unsigned char lingo = buf[2];

            CHECKLEN0(3);

            /* Supported lingos and versions are read from the lingo_versions
             * array
             */
            if (LINGO_SUPPORTED(lingo)) {
                data[2] = lingo;
                data[3] = LINGO_MAJOR(lingo);
                data[4] = LINGO_MINOR(lingo);
                iap_send_pkt(data, sizeof(data));
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
            } else {
                device.lingoes = lingoes;
                /* Seeing a valid IdentifyDeviceLingoes packet resets
                 * authentication
                 */
                reset_auth(&(device.auth));

                /* Devices using IdentifyDeviceLingoes get power off notifications */
                device.do_power_notify = true;
            }

            /* If a new authentication is requested, start the auth
             * process.
             * The periodic handler will take care of sending out the
             * GetDevAuthenticationInfo packet
             *
             * If no authentication is requested, schedule the start of
             * GetAccessoryInfo
             */
            if (deviceid && (options & 0x03) && !DEVICE_AUTH_RUNNING) {
                reset_auth(&(device.auth));
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
                unsigned char data[] = {0x00, 0x16, 0x08};

                reset_auth(&(device.auth));
                iap_send_pkt(data, sizeof(data));
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
                        unsigned char data[] = {0x00, 0x16, 0x00};

                        iap_send_pkt(data, sizeof(data));
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
            unsigned char data[] = {0x00, 0x19, 0x00};

            if (device.auth.state != AUST_CHASENT) {
                cmd_ack_mode0(cmd, IAP_ACK_BAD_PARAM);
                break;
            }

            /* Here we could check the signature. Since we can't, just
             * acknowledge and go to authenticated status
             */
            iap_send_pkt(data, sizeof(data));
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
            /* RetIpodOptions */
            unsigned char data[] = {0x00, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

            /* There are only two features that can be communicated via this
             * function, video support and the ability to control line-out usage.
             * Rockbox supports neither
             */
            iap_send_pkt(data, sizeof(data));
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
         * TODO: Actually do something with the information received here,
         * sending out additional requests for all the categories the
         * device supports
         */
        case 0x28:
        {
            /* Currently, nothing is done with the information */
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
            unsigned char data[] = {0x00, 0x2A, 0x00, 0x00};

            CHECKLEN0(3);
            CHECKAUTH0;

            /* The only information really supported is 0x03, Line-out usage.
             * All others are video related
             */
            if (buf[2] == 0x03) {
                data[2] = 0x03;
                data[3] = 0x01;     /* Line-out enabled */

                iap_send_pkt(data, sizeof(data));
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
 * - Implement at least cmd 0x04
 */

static void cmd_ack_mode2(unsigned char cmd, unsigned char status)
{
    unsigned char data[] = {0x02, 0x01, status, cmd};
    iap_send_pkt(data, sizeof(data));
}

static void cmd_ok_mode2(unsigned char cmd)
{
    cmd_ack_mode2(cmd, IAP_ACK_OK);
}

static void iap_handlepkt_mode2(unsigned int len, const unsigned char *buf)
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
                if((buf[3] & 128) && !iap_btnshuffle) /* shuffle */
                {
                    iap_btnshuffle = true;
                    if(!global_settings.playlist_shuffle)
                    {
                        global_settings.playlist_shuffle = 1;
                        settings_save();
                        if (audio_status() & AUDIO_STATUS_PLAY)
                            playlist_randomise(NULL, current_tick, true);
                    }
                    else if(global_settings.playlist_shuffle)
                    {
                        global_settings.playlist_shuffle = 0;
                        settings_save();
                        if (audio_status() & AUDIO_STATUS_PLAY)
                            playlist_sort(NULL, true);
                    }
                }
                else
                    iap_btnshuffle = false;
            }
            else if(len >= 5 && buf[4] != 0)
            {
                if((buf[4] & 1) && !iap_btnrepeat) /* repeat */
                {
                    int oldmode = global_settings.repeat_mode;
                    iap_btnrepeat = true;
                
                    if (oldmode == REPEAT_ONE)
                            global_settings.repeat_mode = REPEAT_OFF;
                    else if (oldmode == REPEAT_ALL)
                            global_settings.repeat_mode = REPEAT_ONE;
                    else if (oldmode == REPEAT_OFF)
                            global_settings.repeat_mode = REPEAT_ALL;

                    settings_save();
                    if (audio_status() & AUDIO_STATUS_PLAY)
                    audio_flush_and_reload_tracks();
                }
                else
                    iap_btnrepeat = false;

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

            cmd_ack_mode2(cmd, IAP_ACK_OK);
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

static void cmd_ack_mode3(unsigned char cmd, unsigned char status)
{
    unsigned char data[] = {0x03, 0x00, status, cmd};
    iap_send_pkt(data, sizeof(data));
}

static void cmd_ok_mode3(unsigned char cmd)
{
    cmd_ack_mode3(cmd, IAP_ACK_OK);
}

static void iap_handlepkt_mode3(unsigned int len, const unsigned char *buf)
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
            unsigned char data[] = {0x03, 0x02, 0x00, 0x00, 0x00, 0x00};

            iap_send_pkt(data, sizeof(data));
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
            /* Return one profile (0, the disabled profile) */
            unsigned char data[] = {0x03, 0x05, 0x00, 0x00, 0x00, 0x01};

            iap_send_pkt(data, sizeof(data));
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
            unsigned char data[] = {0x03, 0x07, 'D', 'e', 'f', 'a', 'u', 'l', 't', 0x00};

            index = get_u32(&buf[2]);

            if (index > 0) {
                cmd_ack_mode3(cmd, IAP_ACK_BAD_PARAM);
                break;
            }

            iap_send_pkt(data, sizeof(data));
            break;
        }


#if 0
        /* SetRemoteEventNotification */
        case 0x08:
        {
            /* ACK */
            unsigned char data[] = {0x03, 0x00, 0x00, 0x08};
            iap_send_pkt(data, sizeof(data));
            break;
        }
        
        /* GetiPodStateInfo */
        case 0x0C:
        {
            /* request ipod volume */
            if (buf[2] == 0x04)
            {
                iap_set_remote_volume();
            }
            break;
        }
        
        /* SetiPodStateInfo */
        case 0x0E:
        {
            if (buf[2] == 0x04)
                global_settings.volume = (-58)+((int)buf[4]+1)/4;
                sound_set_volume(global_settings.volume);   /* indent BUG? */
            break;
        }
#endif
        
        /* The default response is IAP_ACK_BAD_PARAM */
        default:
        {
            cmd_ack_mode3(cmd, IAP_ACK_BAD_PARAM);
            break;
        }
    }
}

static void cmd_ack_mode4(unsigned int cmd, unsigned char status)
{
    unsigned char data[] = {0x04, 0x00, 0x01, 0x00, 0x00, status};

    data[4] = (cmd >> 8) & 0xFF;
    data[5] = (cmd >> 0) & 0xFF;
    iap_send_pkt(data, sizeof(data));
}

static void cmd_ok_mode4(unsigned int cmd)
{
    cmd_ack_mode4(cmd, IAP_ACK_OK);
}

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

static void iap_handlepkt_mode4(unsigned int len, const unsigned char *buf)
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
            unsigned char data[] = {0x04, 0x00, 0x13, 0x00, 0x00};

            data[3] = LINGO_MAJOR(0x04);
            data[4] = LINGO_MINOR(0x04);

            iap_send_pkt(data, sizeof(data));
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
            unsigned char data[] = {0x04, 0x00, 0x015, 'R', 'O', 'C', 'K', 'B', 'O', 'X', 0x00};

            iap_send_pkt(data, sizeof(data));
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
                    iap_changedctr = 1;
                    break;
                case 0x02: /* stop */
                    iap_remotebtn = BUTTON_RC_PLAY|BUTTON_REPEAT;
                    iap_repeatbtn = 2;
                    iap_changedctr = 1;
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
                    if(iap_pollspeed) iap_pollspeed = 5;
                    break;
                case 0x06: /* frwd */
                    iap_remotebtn = BUTTON_RC_LEFT;
                    if(iap_pollspeed) iap_pollspeed = 5;
                    break;
                case 0x07: /* end ffwd/frwd */
                    iap_remotebtn = BUTTON_NONE;
                    if(iap_pollspeed) iap_pollspeed = 1;
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

static void iap_handlepkt_mode7(unsigned int len, const unsigned char *buf)
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
    struct state_t *s = &frame_state;

    if(!iap_setupflag) return;

    /* if we are waiting for a remote button to go out,
       delay the handling of the new packet */
    if(iap_repeatbtn)
    {
        queue_post(&iap_queue, IAP_EV_MSG_RCVD, 0);
        return;
    }

    /* handle command by mode */
    unsigned char mode = s->payload[0];
    switch (mode) {
    case 0: iap_handlepkt_mode0(s->len, s->payload); break;
    case 2: iap_handlepkt_mode2(s->len, s->payload); break;
    case 3: iap_handlepkt_mode3(s->len, s->payload); break;
    case 4: iap_handlepkt_mode4(s->len, s->payload); break;
    case 7: iap_handlepkt_mode7(s->len, s->payload); break;
    }
    
    /* release the lock on serbuf */
    serbuf_lock = false;

    /* poke the poweroff timer */
    reset_poweroff_timer();
}

int remote_control_rx(void)
{
    int btn = iap_remotebtn;
    if(iap_repeatbtn)
    {
        iap_repeatbtn--;
        if(!iap_repeatbtn)
        {
            iap_remotebtn = BUTTON_NONE;
        }
    }

    return btn;
}

const unsigned char *iap_get_serbuf(void)
{
    return serbuf;
}

