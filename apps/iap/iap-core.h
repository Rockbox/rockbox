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
#ifndef _IAP_CORE_H
#define _IAP_CORE_H

#include <stdint.h>
#include <string.h>
#include "timefuncs.h"
#include "metadata.h"
#include "playlist.h"
#include "iap.h"

#define LOGF_ENABLE
/* #undef LOGF_ENABLE */
#ifdef LOGF_ENABLE
  #include "logf.h"
#endif

/* The Model ID of the iPod we emulate. Currently a 160GB classic */
#define IAP_IPOD_MODEL (0x00130200U)
#define IAP_IPOD_VARIANT "MC293"

/* The firmware version we emulate. Currently 2.0.3 */
#define IAP_IPOD_FIRMWARE_MAJOR (2)
#define IAP_IPOD_FIRMWARE_MINOR (0)
#define IAP_IPOD_FIRMWARE_REV   (3)

/* Status code for IAP ack messages */
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

/* Add a button to the remote button bitfield. Also set iap_repeatbtn=1
 * to ensure a button press is at least delivered once.
 */
#define REMOTE_BUTTON(x) do { \
        iap_remotebtn |= (x); \
        iap_timeoutbtn = 3; \
        iap_repeatbtn = 2; \
        } while(0)

/* States of the extended command support */
enum interface_state {
    IST_STANDARD,    /* General state, support lingo 0x00 commands */
    IST_EXTENDED,   /* Extended Interface lingo (0x04) negotiated */
};

/* States of the authentication state machine */
enum authen_state {
    AUST_NONE,            /* Initial state, no message sent */
    AUST_INIT,            /* Remote side has requested authentication */
    AUST_CERTREQ,         /* Remote certificate requested */
    AUST_CERTBEG,         /* Certificate is being received */
    AUST_CERTALLRECEIVED, /* Certificate all Received  */
    AUST_CERTDONE,        /* Certificate all Done */
    AUST_CHASENT,         /* Challenge sent */
    AUST_CHADONE,         /* Challenge response received */
    AUST_AUTH,            /* Authentication complete */
};

/* State of authentication */
struct auth_t {
    enum authen_state state;        /* Current state of authentication */
    unsigned char max_section;      /* The maximum number of certificate sections */
    unsigned char next_section;     /* The next expected section number */
    uint16_t version;               /* Authentication version */
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
struct device_t {
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
};

extern struct device_t device;
#define DEVICE_AUTHENTICATED (device.auth.state == AUST_AUTH)
#define DEVICE_AUTH_RUNNING ((device.auth.state != AUST_NONE) && (device.auth.state != AUST_AUTH))
#define DEVICE_LINGO_SUPPORTED(x) (device.lingoes & BIT_N((x)&0x1f))

extern unsigned long iap_remotebtn;
extern unsigned int iap_timeoutbtn;
extern int iap_repeatbtn;

extern unsigned char* iap_txpayload;
extern unsigned char* iap_txnext;

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

extern unsigned char lingo_versions[32][2];
#define LINGO_SUPPORTED(x) (LINGO_MAJOR((x)&0x1f) > 0)
#define LINGO_MAJOR(x) (lingo_versions[(x)&0x1f][0])
#define LINGO_MINOR(x) (lingo_versions[(x)&0x1f][1])

void put_u16(unsigned char *buf, const uint16_t data);
void put_u32(unsigned char *buf, const uint32_t data);
uint32_t get_u32(const unsigned char *buf);
uint16_t get_u16(const unsigned char *buf);
void iap_tx_strlcpy(const unsigned char *str);

void iap_reset_auth(struct auth_t* auth);
void iap_reset_device(struct device_t* device);

void iap_shuffle_state(bool state);
void iap_repeat_state(unsigned char state);
void iap_repeat_next(void);
void iap_fill_power_state(void);

void iap_send_tx(void);
void iap_set_remote_volume(void);

extern enum interface_state interface_state;
void iap_interface_state_change(const enum interface_state new);

extern bool iap_btnrepeat;
extern bool iap_btnshuffle;

uint32_t iap_get_trackpos(void);
uint32_t iap_get_trackindex(void);
void iap_get_trackinfo(const unsigned int track, struct mp3entry* id3);

#endif
