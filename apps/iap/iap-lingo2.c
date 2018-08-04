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

/* Lingo 0x02, Simple Remote Lingo
 *
 * TODO:
 * - Fix cmd 0x00 handling, there has to be a more elegant way of doing
 *   this
 */

#include "iap-core.h"
#include "iap-lingo.h"
#include "system.h"
#include "button.h"
#include "audio.h"
#include "settings.h"
#include "tuner.h"
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

static void cmd_ack(const unsigned char cmd, const unsigned char status)
{
    IAP_TX_INIT(0x02, 0x01);
    IAP_TX_PUT(status);
    IAP_TX_PUT(cmd);

    iap_send_tx();
}

#define cmd_ok(cmd) cmd_ack((cmd), IAP_ACK_OK)

void iap_handlepkt_mode2(const unsigned int len, const unsigned char *buf)
{
    static bool poweron_pressed = false;
#if CONFIG_TUNER
    static bool remote_mute = false;
#endif
    unsigned int cmd = buf[1];

    /* We expect at least three bytes in the buffer, one for the
     * lingo, one for the command, and one for the first button
     * state bits.
     */
    CHECKLEN(3);

    /* Lingo 0x02 must have been negotiated */
    if (!DEVICE_LINGO_SUPPORTED(0x02)) {
        cmd_ack(cmd, IAP_ACK_BAD_PARAM);
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
                {
                    REMOTE_BUTTON(BUTTON_RC_PLAY);
#if CONFIG_TUNER
                    if (radio_present == 1) {
                        if (remote_mute == 0) {
                            /* Not Muted so  radio on*/
                            tuner_set(RADIO_MUTE,0);
                        } else {
                            /* Muted so  radio off*/
                            tuner_set(RADIO_MUTE,1);
                        }
                        remote_mute = !remote_mute;
                    }
#endif
                }
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
#if CONFIG_TUNER
                    if (radio_present == 1) {
                        tuner_set(RADIO_MUTE,0);
                    }
#endif
                }
                if(buf[3] & 2) /* pause */
                {
                    if (audio_status() == AUDIO_STATUS_PLAY)
                        REMOTE_BUTTON(BUTTON_RC_PLAY);
#if CONFIG_TUNER
                    if (radio_present == 1) {
                        tuner_set(RADIO_MUTE,1);
                    }
#endif
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

                if (buf[4] & 2) /* power on */
                {
                    poweron_pressed = true;
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

            /* power on released */
            if (poweron_pressed && len >= 5 && !(buf[4] & 2))
            {
                poweron_pressed = false;
#ifdef HAVE_LINE_REC
                /* Belkin TuneTalk microphone sends power-on press+release
                 * events once authentication sequence is finished,
                 * GetDevCaps command is ignored by the device when it is
                 * sent before power-on release event is received.
                 * XXX: It is unknown if other microphone devices are
                 * sending the power-on events.
                 */
                if (DEVICE_LINGO_SUPPORTED(0x01)) {
                    /* GetDevCaps */
                    IAP_TX_INIT(0x01, 0x07);
                    iap_send_tx();
                }
#endif
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
                cmd_ack(cmd, IAP_ACK_NO_AUTHEN);
                break;
            }

            cmd_ack(cmd, IAP_ACK_CMD_FAILED);
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
                cmd_ack(cmd, IAP_ACK_NO_AUTHEN);
                break;
            }

            cmd_ack(cmd, IAP_ACK_CMD_FAILED);
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
                cmd_ack(cmd, IAP_ACK_NO_AUTHEN);
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

            cmd_ok(cmd);
            break;
        }

        /* The default response is IAP_ACK_BAD_PARAM */
        default:
        {
#ifdef LOGF_ENABLE
            logf("iap: Unsupported Mode02 Command");
#endif
            cmd_ack(cmd, IAP_ACK_BAD_PARAM);
            break;
        }
    }
}
