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

#include "iap-core.h"
#include "iap-lingo.h"
#include "system.h"
#include "audio.h"
#include "powermgmt.h"
#include "settings.h"
#include "metadata.h"
#include "playback.h"
#if CONFIG_TUNER
#include "ipod_remote_tuner.h"
#endif

/*
 * This macro is meant to be used inside an IAP mode message handler.
 * It is passed the expected minimum length of the message buffer.
 * If the buffer does not have the required lenght an ACK
 * packet with a Bad Parameter error is generated.
 */
#define CHECKLEN(x) do { \
        if (len < (x)) { \
            cmd_ack(cmd, IAP_ACK_BAD_PARAM); \
            return; \
        }} while(0)

/* Check for authenticated state, and return an ACK Not
 * Authenticated on failure.
 */
#define CHECKAUTH do { \
        if (!DEVICE_AUTHENTICATED) { \
            cmd_ack(cmd, IAP_ACK_NO_AUTHEN); \
            return; \
        }} while(0)

static void cmd_ack(const unsigned char cmd, const unsigned char status)
{
    IAP_TX_INIT(0x03, 0x00);
    IAP_TX_PUT(status);
    IAP_TX_PUT(cmd);

    iap_send_tx();
}

#define cmd_ok(cmd) cmd_ack((cmd), IAP_ACK_OK)

void iap_handlepkt_mode3(const unsigned int len, const unsigned char *buf)
{
    unsigned int cmd = buf[1];

    /* We expect at least two bytes in the buffer, one for the
     * state bits.
     */
    CHECKLEN(2);

    /* Lingo 0x03 must have been negotiated */
    if (!DEVICE_LINGO_SUPPORTED(0x03)) {
        cmd_ack(cmd, IAP_ACK_BAD_PARAM);
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

            CHECKLEN(7);

            index = get_u32(&buf[2]);

            if (index > 0) {
                cmd_ack(cmd, IAP_ACK_BAD_PARAM);
                break;
            }

            /* Currently, we just ignore the command and acknowledge it */
            cmd_ok(cmd);
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

            CHECKLEN(6);

            index = get_u32(&buf[2]);

            if (index > 0) {
                cmd_ack(cmd, IAP_ACK_BAD_PARAM);
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

            CHECKLEN(6);
            CHECKAUTH;

            /* Save the current state of the various attributes we track */
            device.trackpos_ms = iap_get_trackpos();
            device.track_index = iap_get_trackindex();
            device.chapter_index = 0;
            device.play_status = audio_status();
            /* TODO: Fix this */
            device.mute = false;
            device.volume = global_settings.volume;
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

            cmd_ok(cmd);
            break;
        }

        /* RemoteEventNotification (0x09)
         *
         * Sent from the iPod to the device
         */

        /* GetRemoteEventStatus (0x0A)
         *
         * Request the events changed since the last call to
         * GetREmoteEventStatus or SetRemoteEventNotification
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
            CHECKAUTH;
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

            CHECKLEN(3);
            CHECKAUTH;

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
                    if (device.mute == false) {
                        /* Mute status False*/
                        IAP_TX_PUT(0x00);
                        /* Volume */
                        IAP_TX_PUT(0xFF & (int)((global_settings.volume + 90) * 2.65625));

                    } else {
                        /* Mute status True*/
                        IAP_TX_PUT(0x01);
                        /* Volume should be 0 if muted */
                        IAP_TX_PUT(0x00);
                    }

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
                    if (device.mute == false) {
                        /* Mute status False*/
                        IAP_TX_PUT(0x00);
                        /* Volume */
                        IAP_TX_PUT(0xFF & (int)((global_settings.volume + 90) * 2.65625));
                        IAP_TX_PUT(0xFF & (int)((global_settings.volume + 90) * 2.65625));

                    } else {
                        /* Mute status True*/
                        IAP_TX_PUT(0x01);
                        /* Volume should be 0 if muted */
                        IAP_TX_PUT(0x00);
                        IAP_TX_PUT(0x00);
                    }

                    iap_send_tx();
                    break;
                }
                default:
                {
                    cmd_ack(cmd, IAP_ACK_BAD_PARAM);
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
            CHECKLEN(3);
            CHECKAUTH;
            switch (buf[0x02])
            {
                /* Track position (ms)
                 * Data length: 4
                 */
                case 0x00:
                {
                    uint32_t pos;

                    CHECKLEN(7);
                    pos = get_u32(&buf[0x03]);
                    audio_ff_rewind(pos);

                    cmd_ok(cmd);
                    break;
                }

                /* Track index
                 * Data length: 4
                 */
                case 0x01:
                {
                    uint32_t index;

                    CHECKLEN(7);
                    index = get_u32(&buf[0x03]);
                    audio_skip(index-iap_get_trackindex());

                    cmd_ok(cmd);
                    break;
                }

                /* Chapter index
                 * Data length: 2
                 */
                case 0x02:
                {
                    /* This is not supported */
                    cmd_ack(cmd, IAP_ACK_CMD_FAILED);
                    break;
                }

                /* Play status
                 * Data length: 1
                 */
                case 0x03:
                {
                    CHECKLEN(4);
                    switch(buf[0x03])
                    {
                        case 0x00:
                        {
                            audio_stop();
                            cmd_ok(cmd);
                            break;
                        }

                        case 0x01:
                        {
                            audio_resume();
                            cmd_ok(cmd);
                            break;
                        }

                        case 0x02:
                        {
                            audio_pause();
                            cmd_ok(cmd);
                            break;
                        }

                        default:
                        {
                            cmd_ack(cmd, IAP_ACK_CMD_FAILED);
                            break;
                        }
                    }
                    break;
                }

                case 0x04:
                {
                    CHECKLEN(5);
                    if (buf[0x03]==0x00){
                        /* Not Muted */
                        global_settings.volume = (int) (buf[0x04]/2.65625)-90;
                        device.mute = false;
                    }
                    else {
                        device.mute = true;
                    }
                    cmd_ok(cmd);
                    break;
                }

                /* Equalizer
                 * Data length: 5
                 */
                case 0x06:
                {
                    uint32_t index;

                    CHECKLEN(8);
                    index = get_u32(&buf[0x03]);
                    if (index == 0) {
                        cmd_ok(cmd);
                    } else {
                        cmd_ack(cmd, IAP_ACK_BAD_PARAM);
                    }
                    break;
                }

                /* Shuffle
                 * Data length: 2
                 */
                case 0x07:
                {
                    CHECKLEN(5);

                    switch(buf[0x03])
                    {
                        case 0x00:
                        {
                            iap_shuffle_state(false);
                            cmd_ok(cmd);
                            break;
                        }
                        case 0x01:
                        case 0x02:
                        {
                            iap_shuffle_state(true);
                            cmd_ok(cmd);
                            break;
                        }

                        default:
                        {
                            cmd_ack(cmd, IAP_ACK_BAD_PARAM);
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
                    CHECKLEN(5);

                    switch(buf[0x03])
                    {
                        case 0x00:
                        {
                            iap_repeat_state(REPEAT_OFF);
                            cmd_ok(cmd);
                            break;
                        }
                        case 0x01:
                        {
                            iap_repeat_state(REPEAT_ONE);
                            cmd_ok(cmd);
                            break;
                        }
                        case 0x02:
                        {
                            iap_repeat_state(REPEAT_ALL);
                            cmd_ok(cmd);
                            break;
                        }
                        default:
                        {
                            cmd_ack(cmd, IAP_ACK_BAD_PARAM);
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
                    CHECKLEN(9);

                    cmd_ack(cmd, IAP_ACK_CMD_FAILED);
                    break;
                }

                /* Alarm
                 * Data length: 4
                 */
                case 0x0A:
                {
                    CHECKLEN(7);

                    cmd_ack(cmd, IAP_ACK_CMD_FAILED);
                    break;
                }

                /* Backlight
                 * Data length: 2
                 */
                case 0x0B:
                {
                    CHECKLEN(5);

                    cmd_ack(cmd, IAP_ACK_CMD_FAILED);
                    break;
                }

                /* Sound check
                 * Data length: 2
                 */
                case 0x0D:
                {
                    CHECKLEN(5);

                    cmd_ack(cmd, IAP_ACK_CMD_FAILED);
                    break;
                }

                /* Audio book speed
                 * Data length: 1
                 */
                case 0x0E:
                {
                    CHECKLEN(4);

                    cmd_ack(cmd, IAP_ACK_CMD_FAILED);
                    break;
                }

                /* Track position (s)
                 * Data length: 2
                 */
                case 0x0F:
                {
                    uint16_t pos;

                    CHECKLEN(5);
                    pos = get_u16(&buf[0x03]);
                    audio_ff_rewind(1000L * pos);

                    cmd_ok(cmd);
                    break;
                }

                /* Volume/Mute/Absolute
                 * Data length: 4
                 * TODO: Fix this
                 */
                case 0x10:
                {
                    CHECKLEN(7);
                    if (buf[0x03]==0x00){
                        /* Not Muted */
                        global_settings.volume = (int) (buf[0x04]/2.65625)-90;
                        device.mute = false;
                    }
                    else {
                        device.mute = true;
                    }

                    cmd_ok(cmd);
                    break;
                }

                default:
                {
                    cmd_ack(cmd, IAP_ACK_BAD_PARAM);
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

            CHECKAUTH;

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

            CHECKAUTH;
            CHECKLEN(6);

            index = get_u32(&buf[0x02]);
            trackcount = playlist_amount();

            if (index >= trackcount)
            {
                cmd_ack(cmd, IAP_ACK_BAD_PARAM);
                break;
            }
            audio_skip(index-iap_get_trackindex());
            cmd_ok(cmd);

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

            CHECKLEN(0x09);
            CHECKAUTH;

            track_index = get_u32(&buf[0x03]);
            if (-1 == playlist_get_track_info(NULL, track_index, &track)) {
                cmd_ack(cmd, IAP_ACK_BAD_PARAM);
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
                    cmd_ack(cmd, IAP_ACK_BAD_PARAM);
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
            CHECKAUTH;

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
            CHECKAUTH;

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
            CHECKAUTH;
            CHECKLEN(0x0C);

            /* No artwork support currently */
            cmd_ack(cmd, IAP_ACK_BAD_PARAM);
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
            CHECKAUTH;

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
            CHECKAUTH;
            CHECKLEN(4);

            /* Sound check is not supported right now
             * TODO: Fix
             */

            cmd_ack(cmd, IAP_ACK_CMD_FAILED);
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
            uint32_t trackcount;

            CHECKAUTH;
            CHECKLEN(0x0C);

            /* Artwork is currently unsuported, just check for a valid
             * track index
             */
            index = get_u32(&buf[0x02]);
            trackcount = playlist_amount();

            if (index >= trackcount)
            {
                cmd_ack(cmd, IAP_ACK_BAD_PARAM);
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
#ifdef LOGF_ENABLE
            logf("iap: Unsupported Mode03 Command");
#endif
            cmd_ack(cmd, IAP_ACK_BAD_PARAM);
            break;
        }
    }
}
