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
#include "iap-core.h"
#include "iap-lingo.h"
#include "kernel.h"
#include "system.h"
#include "tuner.h"
#if CONFIG_TUNER
#include "ipod_remote_tuner.h"
#endif

/*
 * This macro is meant to be used inside an IAP mode message handler.
 * It is passed the expected minimum length of the message inbufferfer.
 * If the inbufferfer does not have the required lenght an ACK
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
    IAP_TX_INIT(0x07, 0x00);
    IAP_TX_PUT(status);
    IAP_TX_PUT(cmd);

    iap_send_tx();
}

#define cmd_ok(cmd) cmd_ack((cmd), IAP_ACK_OK)

void iap_handlepkt_mode7(const unsigned int len, const unsigned char *inbuffer)
{
    /* Note that some of the Lingo Mode 7 commands are handled by
     * ../firmware/drivers/tuner/ipod_remote_tuner.c as some of the
     * commands are sourced with the remote as the master with the ipod acting
     * as the slave.
     */
    unsigned char cmd = inbuffer[1];
    unsigned char statusnotifymaskbyte = 0;

    /* We expect at least two bytes in the inbuffer, one for the
     * lingo and one for the command
     */
    CHECKLEN(2);

    /* Lingo 0x07 must have been negotiated */
    if (!DEVICE_LINGO_SUPPORTED(0x07)) {
        cmd_ack(cmd, IAP_ACK_BAD_PARAM);
        return;
    }

    switch (cmd)
    {
        /* case 00 ToIpod      Ack 2/6 bytes*/
        case 0x00:
        {
            /* 0x00 OK
             * 0x01 Unknown Track Category
             * 0x02 Command Failed. Command is valid but did not succeed
             * 0x03 Out Of Resources
             * 0x04 Bad Parameter
             * 0x05 Unknown Track ID
             * 0x06 Command Pending.
             * 0x07 Not Authenticated
             *
             * byte  1 is ID of command being acknowledged
             * bytes 2-5 only if status byte is pending. timeout in ms.
             */
             break;
        }

        /* case 0x01 ToAccessory GetTunerCaps
         * This is sent by iap-lingo0.c through case 0x15 after device
         * has been authenticated FF55020701F6
         */

        /* case 02 ToIpod      RetTunerCaps 8 bytes */
        case 0x02:
        {
            /* Capabilities are stored as bits in first 4 bytes,
             * inbuffer[2] byte is bits 31:24
             * inbuffer[3] byte is bits 23:16
             * inbuffer[4] byte is bits 15:08
             * inbuffer[5] byte is bits 07:00
             * inbuffer[6] and inbuffer[7] are all reserved bits
             * Bit 0 = AM Band 520-1710 Khz
             * Bit 1 = FM Europe/US 87.5 - 108.0 Mhz
             * Bit 2 = FM Japan 76.0 - 90.0 Mhz
             * Bit 3 = FM Wide 76.0 - 108.0 Mhz
             * Bit 4 = HD Radio Capable
             * Bit 5:7 Reserved
             * Bit 8 = Tuner Power On/Off Control Capable
             * Bit 9 = Status Change Notification Capable
             * Bit 10:15 Reserved
             * Bit 17:16 Minimum FM Resolution ID Bits
             *     00 = 200Khz, 01 = 100Khz, 10 = 50Khz, 11 = reserved
             * Bit 18 = Tuner Seek Up/Down Capable
             * Bit 19 = Tuner Seek RSSI Threshold. Only if 18=1
             * Bit 20 = Force Monophonic mode capable
             * Bit 21 = Stero Blend Capable
             * Bit 22 = FM Tuner deemphasis select capable
             * Bit 23 = AM Tuner Resolution 9Khz (0=10Khz Only) capable
             * Bit 24 = Radio Data System (RDS/RBDS) data capable
             * Bit 25 = Tuner Channel RSSI indicator capable
             * Bit 26 = Stero Source Indicator capable
             * Bit 27 = RDS/RBDS Raw mode capable
             * Bit 31:28 Reserved
             *
             * ipod Tuner returns 07 5E 07 0E 10 4B
             * Bytes 6,7 Reserved
             * ???????? ????????
             * ???????? ????????
             * 00010000 01001011
             *
             *  Byte 5 - 0E
             * 00000000
             * 76543210
             * 00001110
             *        AM
             *       FM Europe/US
             *      FM Japan
             *     FM Wide
             *
             * Byte 4 - 07
             * 11111100
             * 54321098
             * 00000111
             *        Tuner Power On/Off
             *       Status Change Notification
             *      ?? Should be reserved
             *
             * Byte 3 - 5E
             * 22221111
             * 32109876
             * 01011110
             *       Tuner Seek Up/Down
             *      Tuner Seek RSSI Threshold
             *     Force Mono Mode Capable
             *    Stereo Blend Capable
             *  FM Tuner deemphasis select capable
             *
             * Byte 2 - 07
             * 33222222
             * 10987654
             * 00000111
             *        RDS/RBDS Capable
             *       Tuner Channel RSSI Indicator
             *      Stereo Source
             *
             * Just need to see what we can use this data for
             * Make a selection for the tuner mode to select
             * Preference is
             * 1st - 76 to 108 FM
             * 2nd - 87.5 to 108 Fm
             * 3rd - 76 to 90 Fm
             * 4th - AM
             *
             */

            if ((inbuffer[4] & 0x03) >0) {
                statusnotifymaskbyte = 0;
                if ((inbuffer[4] >> 0) & 0x01) {
                    /* Supports Tuner Power On/Off, so set ON */
                    statusnotifymaskbyte = 1;
                }
                if ((inbuffer[4] >> 1) & 0x01) {
                    /* Supports Status Change Notification so set ON  */
                    /* Apple 5/6/7G firmware does NOT enable this bit */
                    /* statusnotifymaskbyte += 2;                     */
                }
                IAP_TX_INIT(0x07, 0x05);
                IAP_TX_PUT(statusnotifymaskbyte);
                iap_send_tx();
            }
            if ((inbuffer[5] >> 1) & 0x01) {
                /* Supports FM Europe/US Tuner 87.5 - 108.0 Mhz */
                /* Apple firmware sends this before setting region */
                IAP_TX_INIT(0x07, 0x0E);
                IAP_TX_PUT(0x00);
                iap_send_tx();
                /* Apple firmware then sends region */
                IAP_TX_INIT(0x07, 0x08);
                IAP_TX_PUT(0x02);
                iap_send_tx();
            } else if ((inbuffer[5] >> 3) & 0x01) {
                /* Supports FM Wide Tuner 76 - 108.0 Mhz */
                /* apple firmware send this before setting region */
                IAP_TX_INIT(0x07, 0x0E);
                IAP_TX_PUT(0x00);
                iap_send_tx();
                /* Apple firmware then send region */
                IAP_TX_INIT(0x07, 0x08);
                IAP_TX_PUT(0x08);
                iap_send_tx();
            } else if ((inbuffer[5] >> 2) & 0x01) {
                /* Supports FM Japan Tuner 76 - 90.0 Mhz */
                /* apple firmware send this before setting region */
                IAP_TX_INIT(0x07, 0x0E);
                IAP_TX_PUT(0x41);
                iap_send_tx();
                /* Apple firmware then send region */
                IAP_TX_INIT(0x07, 0x08);
                IAP_TX_PUT(0x04);
                iap_send_tx();
            } else if ((inbuffer[5] >> 0) & 0x01) {
                /* Supports AM Tuner */
                IAP_TX_INIT(0x07, 0x08);
                IAP_TX_PUT(0x01);
                iap_send_tx();
            }

            if ((inbuffer[2]  & 0x03) > 0) {
                statusnotifymaskbyte = 0;
                if ((inbuffer[2] >> 0) & 0x01) {
                   /* Supports RDS/RBDS Capable so set
                    *StatusChangeNotify for RDS/RBDS Data
                    */
                    statusnotifymaskbyte = 1;
                }
                if ((inbuffer[2] >> 1) & 0x01) {
                    /* Supports Tuner Channel RSSi Indicator Capable so set */
                    /* StatusChangeNotify for RSSI */
                    /* Apple 5G firmware does NOT enable this bit so we wont  */
                    /* statusnotifymaskbyte += 2;                   */
                }
                IAP_TX_INIT(0x07, 0x18);
                IAP_TX_PUT(statusnotifymaskbyte);
                iap_send_tx();
            }

            if ((inbuffer[4] >> 2) & 0x01) {
                /* Reserved */
            }
            if ((inbuffer[4] >> 3) & 0x01) {
                /* Reserved */
            }
            if ((inbuffer[3] >> 1) & 0x01) {
                /* Tuner Seek Up/Down` */
            }
            if ((inbuffer[3] >> 2) & 0x01) {
                /* Tuner Seek RSSI Threshold */
            }
            if ((inbuffer[3] >> 3) & 0x01) {
                /* Force Mono Mode */
            }
            if ((inbuffer[3] >> 4) & 0x01) {
                /* Stereo Blend */
            }
            if ((inbuffer[3] >> 6) & 0x01) {
                /* FM Tuner deemphasis */
            }
            if ((inbuffer[2] >> 2) & 0x01) {
                /* Stereo Source */
            }
            break;
        }
        /* case 03 ToAccessory GetTunerCtrl 2 bytes */

        /* case 04 ToIpod      RetTunerCtrl 3 bytes
         * Bit 0 power is on (1) or Off (0)
         * Bit 1 StatusChangeNotify is enabled (1) or disabled (0)
         * Bit 3 RDS/RBDS Raw mode enabled
         *
         * Should/Can we do something with these?
         */

        /* case 05 ToAccessory SetTunerCtrl 3 bytes
         *         Bits as per 0x04 above
         *         Bit 0/1 set through Lingo7 Cmd02 */

        /* case 06 ToAccessory GetTunerBand 2 bytes */

        /* case 07 ToIpod      RetTunerBand 3 bytes
         * Returns current band for Tuner. See 0x08 below
         *
         * Should/Can we do something with these?
         */

        /* case 08 ToAccessory SetTuneBand
         *         Set Bit 0 for AM
         *         Set Bit 1 for FM Europe/U S 87.5-108Mhz
         *         Set Bit 2 for FM JApan 76.0-90.0Mhz
         *         Set Bit 3 for FM Wide 76.0-108Mhz
         *         Currently we send this after receiving capabilities
         *         on 0x02 above
         */

        /* case 09 ToAccessory GetTunerFreq 2 bytes */

        /* case 0A ToIpod      RetTunerFreq 7 bytes */
        case 0x0A:
        {
            /* Returns Frequency set and RSSI Power Levels
             * These are sent as is to rmt_tuner_freq() in
             * ../firmware/drivers/tuner/ipod_remote_tuner.c */
            rmt_tuner_freq(len, inbuffer);
            break;
        }

        /* case 0B ToAccessory SetTunerFreq 6 bytes */

        /* case 0C ToAccessory GetTunerMode 2 bytes */

        /* case 0D ToIpod      RetTunerMode 3 bytes
         * Returns Tuner Mode Status in 8 bits as follows
         * Bit 1:0 - FM Tuner Resolution
         * Bit 2 Tuner is seeking up or down
         * Bit 3 Tuner is seeking with RSSI min theshold enabled
         * Bit 4 Force Mono Mode (1) or allow stereo (0)
         * Bit 5 Stereo Blend enabled. Valid only if Bit 4 is 0
         * Bit 6 FM Tuner Deemphasis 50uS (1) or 75uS (0)
         * Bit 7 Reserved 0
         */

        /* case 0E ToAccessory SetTunerMode 3 bytes
         *         See 0x0D for Bit Descriptions
         * Bits set by Cmd 02
         */

        /* case 0F ToAccessory GetTunerSeekRssi 2 bytes */

        /* case 10 ToIpod      RetTunerSeekRssi 3 bytes
         * Returns RSSI Value for seek operations
         * value is 0 (min) - 255 (max)
         */

        /* case 11 ToAccessory SetTunerSeekRssi 3 bytes */

        /* case 12 ToAccessory TunerSeekStart 3 bytes */

        /* case 13 ToIpod      TunerSeekDone 7 bytes */
        case 0x13:
        {
            rmt_tuner_freq(len, inbuffer);
            break;
        }

        /* case 14 ToAccessory GetTunerStatus 2 bytes */

        /* case 15 ToIpod      RetTunerStatus 3 bytes */

        /* case 16 ToAccessory GetStatusNotifyMask 2 bytes */

        /* case 17 ToIpod      RetStatusNotifyMask 3 bytes */

        /* case 18 ToAccessory SetStatusNotifyMask 3 bytes
         * This is set by Cmd 02
         */

        /* case 19 ToIpod      StatusChangeNotify 3 bytes */
        case 0x19:
        {
            /* Returns StatusChangeNotify bits to ipod.
             * Bit 0 set for RDS/RBDS data ready
             * Bit 1 set for Tuner RSSI level change
             * Bit 2 for Stereo Indicator changed
             * If any of these are set we will request the data
             * need to look at using these
             */
             break;
         }

        /* case 1A ToAccessory GetRdsReadyStatus 2 bytes */

        /* case 1B ToIpod      RetRdsReadyStatus 6 bytes */
        case 0x1B:
        {
            break;
        }
        /* case 1C ToAccessory GetRdsData 3 bytes */

        /* case 1D ToIpod      RetRdsData NN bytes */
        case 0x1D:
        {
            rmt_tuner_rds_data(len, inbuffer);
            break;
        }

        /* case 1E ToAccessory GetRdsNotifyMask 2 bytes*/

        /* case 1F ToIpod      RetRdsNotifyMask 6 Bytes*/
        case 0x1F:
        {
            break;
        }

        /* case 20 ToAccessory SetRdsNotifyMask 6 bytes */

        /* case 21 ToIpod      RdsReadyNotify NN bytes */
        case 0x21:
        {
            rmt_tuner_rds_data(len, inbuffer);
            break;
        }
        /* case 22 Reserved */

        /* case 23 Reserved */

        /* case 24 Reserved */

        /* case 25 ToAccessory GetHDProgramServiceCount 0 bytes */

        /* case 26 ToIpod      RetHDProgramServiceCount 1 bytes */
        case 0x26:
        {
            break;
        }

        /* case 27 ToAccessory GetHDProgramService 0  bytes */

        /* case 28 ToIpod      RetHDProgramService 1 bytes */
        case 0x28:
        {
            break;
        }

        /* case 29 ToAccessory SetHDProgramService 1 bytes */

        /* case 2A ToAccessory GetHDDataReadyStatus 0 bytes */

        /* case 2B ToIpod      RetHDDataReadyStatus 4 bytes */
        case 0x2B:
        {
            break;
        }

        /* case 2C ToAccessory GetHDData 1 bytes */

        /* case 2D ToIpod      RetHDData NN bytes */
        case 0x2D:
        {
            break;
        }

        /* case 2E ToAccessory GetHDDataNotifyMask 0 bytes */

        /* case 2F ToIpod      RetHDDataNotifyMask 4 bytes */
        case 0x2F:
        {
            break;
        }

        /* case 30 ToAccessory SetHDDataNotifyMask 4 bytes */

        /* case 31 ToIpod      HDDataReadyNotify NN bytes */
        case 0x31:
        {
            break;
        }

        /* The default response is IAP_ACK_BAD_PARAM */
        default:
        {
#ifdef LOGF_ENABLE
            logf("iap: Unsupported Mode07 Command");
#endif
            cmd_ack(cmd, IAP_ACK_BAD_PARAM);
            break;
        }
    }
}
