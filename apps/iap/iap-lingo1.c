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

/* Lingo 0x01: Microphone Lingo
 */

#include "iap-core.h"
#include "iap-lingo.h"

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

/* returns record status */
bool iap_record(bool onoff)
{
    if (!DEVICE_LINGO_SUPPORTED(0x01))
        return false;

    /* iPodModeChange */
    IAP_TX_INIT(0x01, 0x06);
    IAP_TX_PUT(onoff ? 0x00 : 0x01);
    iap_send_tx();

    return onoff;
}

void iap_handlepkt_mode1(const unsigned int len, const unsigned char *buf)
{
    unsigned int cmd = buf[1];

    /* Lingo 0x04 commands are at least 4 bytes in length */
    CHECKLEN(4);

    /* Lingo 0x01 must have been negotiated */
    if (!DEVICE_LINGO_SUPPORTED(0x01)) {
        cmd_ack(cmd, IAP_ACK_BAD_PARAM);
        return;
    }

    /* Authentication required for all commands */
    CHECKAUTH;

    switch (cmd)
    {
        /* BeginRecord (0x00) Deprecated
         *
         * Sent from the iPod to the device
         */

        /* EndRecord (0x01) Deprecated
         *
         * Sent from the iPod to the device
         */

        /* BeginPlayback (0x02) Deprecated
         *
         * Sent from the iPod to the device
         */

        /* EndPlayback (0x03) Deprecated
         *
         * Sent from the iPod to the device
         */

        /* ACK (0x04)
         *
         * The device sends an ACK response when a command
         * that does not return any data has completed.
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: Microphone Lingo, always 0x01
         * 0x01: Command, always 0x04
         * 0x02: The command result status
         * 0x03: The ID of the command for which the
         *       response is being sent
         *
         * Returns: (none)
         */
        case 0x04:
#ifdef LOGF_ENABLE
            if (buf[2] != 0x00)
                logf("iap: Mode1 Command ACK error: "
                            "0x%02x 0x%02x", buf[2], buf[3]);
#endif
            break;

        /* GetDevAck (0x05)
         *
         * Sent from the iPod to the device
         */

        /* iPodModeChange (0x06)
         *
         * Sent from the iPod to the device
         */

        /* GetDevCaps (0x07)
         *
         * Sent from the iPod to the device
         */

        /* RetDevCaps (0x08)
         *
         * The microphone device returns the payload
         * indicating which capabilities it supports.
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: Microphone Lingo, always 0x01
         * 0x01: Command, always 0x08
         * 0x02: Device capabilities (bits 31:24)
         * 0x03: Device capabilities (bits 23:16)
         * 0x04: Device capabilities (bits 15:8)
         * 0x05: Device capabilities (bits 7:0)
         *
         * Returns:
         * SetDevCtrl, sets stereo line input when supported
         */
        case 0x08:
            CHECKLEN(6);

            if ((buf[5] & 3) == 3) {
                /* SetDevCtrl, set stereo line-in */
                IAP_TX_INIT(0x01, 0x0B);
                IAP_TX_PUT(0x01);
                IAP_TX_PUT(0x01);

                iap_send_tx();
            }

            /* TODO?: configure recording level/limiter controls
               when supported by the device */

            break;

        /* GetDevCtrl (0x09)
         *
         * Sent from the iPod to the device
         */

        /* RetDevCaps (0x0A)
         *
         * The device returns the current control state
         * for the specified control type.
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: Microphone Lingo, always 0x01
         * 0x01: Command, always 0x0A
         * 0x02: The control type
         * 0x03: The control data
         */
        case 0x0A:
            switch (buf[2])
            {
                case 0x01:  /* stereo/mono line-in control */
                case 0x02:  /* recording level control */
                case 0x03:  /* recording level limiter control */
                    break;
            }
            break;

        /* SetDevCtrl (0x0B)
         *
         * Sent from the iPod to the device
         */

        /* The default response is IAP_ACK_BAD_PARAM */
        default:
        {
#ifdef LOGF_ENABLE
            logf("iap: Unsupported Mode1 Command");
#endif
            cmd_ack(cmd, IAP_ACK_BAD_PARAM);
            break;
        }
    }
}
