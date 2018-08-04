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
    if (cmd  != 0){
        IAP_TX_INIT(0x00, 0x02);
        IAP_TX_PUT(status);
        IAP_TX_PUT(cmd);

        iap_send_tx();
    }
}

#define cmd_ok(cmd) cmd_ack((cmd), IAP_ACK_OK)

static void cmd_pending(const unsigned char cmd, const uint32_t msdelay)
{
    IAP_TX_INIT(0x00, 0x02);
    IAP_TX_PUT(0x06);
    IAP_TX_PUT(cmd);
    IAP_TX_PUT_U32(msdelay);

    iap_send_tx();
}

void iap_handlepkt_mode0(const unsigned int len, const unsigned char *buf)
{
    unsigned int cmd = buf[1];

    /* We expect at least two bytes in the buffer, one for the
     * lingo, one for the command
     */
    CHECKLEN(2);

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
            CHECKLEN(3);

            /* Issuing this command exits any extended interface states
             * and resets authentication
             */
            iap_interface_state_change(IST_STANDARD);
            iap_reset_device(&device);

            switch (lingo) {
                case 0x04:
                {
                    /* A single lingo device negotiating the
                     * extended interface lingo. This causes an interface
                     * state change.
                     */
                    iap_interface_state_change(IST_EXTENDED);
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
                cmd_ack(cmd, IAP_ACK_BAD_PARAM);
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
                cmd_ack(cmd, IAP_ACK_BAD_PARAM);
                break;
            }

            cmd_pending(cmd, 1000);
            iap_interface_state_change(IST_EXTENDED);
            cmd_ok(cmd);
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
                cmd_ack(cmd, IAP_ACK_BAD_PARAM);
                break;
            }

            cmd_pending(cmd, 1000);
            iap_interface_state_change(IST_STANDARD);
            cmd_ok(cmd);
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
            IAP_TX_PUT_STRING(IAP_IPOD_VARIANT);

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

            CHECKLEN(3);

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
                cmd_ack(cmd, IAP_ACK_BAD_PARAM);
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

            CHECKLEN(14);

            /* Issuing this command exits any extended interface states */
            iap_interface_state_change(IST_STANDARD);

            /*
             * Actions by remote listed            Apple Firmware   Rockbox Firmware
             * Apple remote on Radio pause/play  - Mutes            Mutes
             *                       vol up/down - Vol Up/Dn        Vol Up/Dn
             *                       FF/FR       - Station Up/Dn    Station Up/Dn
             *                  iPod Pause/Play  - Mutes            Mutes
             *                       Vol up/down - Vol Up/Dn        Vol Up/Dn
             *                       FF/FR       - Station Up/Dn    Station Up/Dn
             *                 Remote pause/play - Pause/Play       Pause/Play
             *                       vol up/down - Vol Up/Dn        Vol Up/Dn
             *                       FF/FR       - Next/Prev Track  Next/Prev Track
             *                  iPod Pause/Play  - Pause/Play       Pause/Play
             *                       Vol up/down - Vol Up/Dn        Vol Up/Dn
             *                       FF/FR       - Next/Prev Track  Next/Prev Track
             *
             * The following bytes are returned by the accessories listed
             * FF 55 0E 00 13 00 00 00 3D 00 00 00 04 00 00 00 00 9E robi DAB Radio Remote
             * FF 55 0E 00 13 00 00 00 35 00 00 00 04 00 00 00 00 A6 (??) FM Transmitter
             * FF 55 0E 00 13 00 00 00 8D 00 00 00 0E 00 00 00 03 41 Apple Radio Remote
             *
             * Bytes 9-12 = Options         11111100 0000 00 00
             *                              54321098 7654 32 10
             * 00000004 = 00000000 00000000 00000000 0000 01 00 Bits 2
             * 00000004 = 00000000 00000000 00000000 0000 01 00 Bits 2
             * 0000000E = 00000000 00000000 00000000 0000 01 10 Bits 12
             *
             * Bit 0: Authentication 00 = No Authentication 
	     *                       01 = Defer Auth until required (V1)
             * Bit 1:                10 = Authenticate Immediately (V2) 
	     *                       11 = Reserved
             * Bit 2: Power Requirements 00 = Low Power Only 10 = Reserved
             * Bit 3:                    01 = Int High Power 11 = Reserved
             *
             * Bytes 13-16 = Device ID
             * 00000000
             * 00000000
             * 00000003
             *
             * Bytes 5-8 = lingoes spoken   11111100 00000000
             *                              54321098 76543210
             * 0000003D = 00000000 00000000 00000000 00111101 Bits 2345
             * 00000035 = 00000000 00000000 00000000 00110101 Bits 245
             * 0000008D = 00000000 00000000 00000000 10001101 Bits 237
             *
             *
             * Bit 0: Must be set by all devices. See above
             * Bit 1: Microphone Lingo
             * Bit 2: Simple Remote
             * Bit 3: Display Remote
             * Bit 4: Extended Remote
             * Bit 5: RF Transmitter lingo
             */

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
                cmd_ack(cmd, IAP_ACK_CMD_FAILED);
                break;
            }
            iap_reset_device(&device);
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

            cmd_ok(cmd);

            /* Bit 0: Must be set by all devices. See above*/
            /* Bit 1: Microphone Lingo */
            /* Bit 2: Simple Remote */
            /* Bit 3: Display Remote */
            /* Bit 4: Extended Remote */
            /* Bit 5: RF Transmitter lingo */
            if (lingoes & (1 << 5))
            {
                /* FM transmitter sends this: */
                /* FF 55 0E 00 13 00 00 00 35 00 00 00 04 00 00 00 00 A6 (??)*/
                /* 0x00000035 = 00000000 00000000 00000000 00110101 */
                /* 1<<5                                      1      */
                /* GetAccessoryInfo */
                IAP_TX_INIT(0x00, 0x27);
                IAP_TX_PUT(0x00);

                iap_send_tx();

                /* RF Transmitter: Begin transmission */
                IAP_TX_INIT(0x05, 0x02);

                iap_send_tx();
            }
            /* Bit 6: USB Host Control */
            /* Bit 7: RF Tuner lingo */
#if CONFIG_TUNER
            if (lingoes & (1 << 7))
            {
                /* ipod fm radio remote sends this: */
                /* FF 55 0E 00 13 00 00 00 8D 00 00 00 0E 00 00 00 03 */
                /* 0x0000008D = 00000000 00000000 00000000 00011101   */
                /* 1<<7                                               */
                radio_present = 1;
            }
#endif
            /* Bit 8: Accessory Equalizer Lingo */
            /* Bit 9: Reserved */
            /* Bit 10: Digial Audio Lingo */
            /* Bit 11: Reserved */
            /* Bit 12: Storage Lingo */
            /* Bit 13: Reserved */
            /* .................*/
            /* Bit 31: Reserved */
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
             * and the long form shown above but it must be at least 4
             * bytes long
             */
            CHECKLEN(4);

            if (!DEVICE_AUTH_RUNNING) {
                cmd_ack(cmd, IAP_ACK_BAD_PARAM);
                break;
            }

            device.auth.version = (buf[2] << 8) | buf[3];

            /* We only support authentication versions 1.0 and 2.0 */
            if ((device.auth.version != 0x100) && (device.auth.version != 0x200)) {
                /* Version mismatches are signalled by AckDevAuthenticationInfo
                 * with the status set to Authentication Information unsupported
                 */
                iap_reset_auth(&(device.auth));

                IAP_TX_INIT(0x00, 0x16);
                IAP_TX_PUT(0x08);

                iap_send_tx();
                break;
            }
            if (device.auth.version == 0x100) {
                /* If we could really do authentication we'd have to
                 * check the certificate here. Since we can't, just acknowledge
                 * the packet later with an "everything OK" AckDevAuthenticationInfo
                 * and change device.auth.state to AuthenticateState_CertificateDone
                 */
                device.auth.state = AUST_CERTALLRECEIVED;
            } else {
                /* Version 2.00 requires at least one byte of certificate data
                 * in the packet
                 */
                CHECKLEN(7);
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
                            cmd_ack(cmd, IAP_ACK_BAD_PARAM);
                            break;
                        }

                        /* Is this the last section? */
                        if (device.auth.next_section == device.auth.max_section) {
                            /* If we could really do authentication we'd have to
                             * check the certificate here. Since we can't, just acknowledge
                             * the packet later with an "everything OK" AckDevAuthenticationInfo
                             * and change device.auth.state to AuthenticateState_CertificateDone
                             */
                            device.auth.state = AUST_CERTALLRECEIVED;
                        } else {
                            device.auth.next_section++;
                            cmd_ok(cmd);
                        }
                        break;
                    }
                    default:
                    {
                        cmd_ack(cmd, IAP_ACK_BAD_PARAM);
                        break;
                    }
                }
            }
            if (device.auth.state == AUST_CERTALLRECEIVED) {
                /* We've received all the certificate data so just
                 *Acknowledge everything OK
                 */
                IAP_TX_INIT(0x00, 0x16);
                IAP_TX_PUT(0x00);

                iap_send_tx();

                /* GetAccessoryInfo*/
                IAP_TX_INIT(0x00, 0x27);
                IAP_TX_PUT(0x00);

                iap_send_tx();

                device.auth.state = AUST_CERTDONE;
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
                cmd_ack(cmd, IAP_ACK_BAD_PARAM);
                break;
            }

            /* Here we could check the signature. Since we can't, just
             * acknowledge and go to authenticated status
             */
            IAP_TX_INIT(0x00, 0x19);
            IAP_TX_PUT(0x00);

            iap_send_tx();
            device.auth.state = AUST_AUTH;
#if CONFIG_TUNER
            if (radio_present == 1)
            {
                /* GetTunerCaps */
                IAP_TX_INIT(0x07, 0x01);

                iap_send_tx();
            }
#endif
            iap_set_remote_volume();

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
            CHECKAUTH;

            cmd_ack(cmd, IAP_ACK_CMD_FAILED);
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
            CHECKAUTH;

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
            CHECKAUTH;

            cmd_ack(cmd, IAP_ACK_CMD_FAILED);
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
            CHECKAUTH;

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
            CHECKLEN(3);

            switch (buf[0x02])
            {
                /* Info capabilities */
                case 0x00:
                {
                    CHECKLEN(7);

                    device.capabilities = get_u32(&buf[0x03]);
                    /* Type 0x00 was already queried, that's where this 
		     * information comes from 
		     */
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
            CHECKLEN(3);
            CHECKAUTH;

            IAP_TX_INIT(0x00, 0x2A);
            /* The only information really supported is 0x03, Line-out usage.
             * All others are video related
             */
            if (buf[2] == 0x03) {
                IAP_TX_PUT(0x03);
                IAP_TX_PUT(0x01);   /* Line-out enabled */

                iap_send_tx();
            } else {
                cmd_ack(cmd, IAP_ACK_BAD_PARAM);
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
            CHECKLEN(5);
            CHECKAUTH;

            /* The only information really supported is 0x03, Line-out usage.
             * All others are video related
             */
            if (buf[2] == 0x03) {
                /* If line-out disabled is requested, reply with IAP_ACK_CMD_FAILED,
                 * otherwise with IAP_ACK_CMD_OK
                 */
                if (buf[3] == 0x00) {
                    cmd_ack(cmd, IAP_ACK_CMD_FAILED);
                } else {
                    cmd_ok(cmd);
                }
            } else {
                cmd_ack(cmd, IAP_ACK_BAD_PARAM);
            }

            break;
        }

        /* The default response is IAP_ACK_BAD_PARAM */
        default:
        {
#ifdef LOGF_ENABLE
            logf("iap: Unsupported Mode00 Command");
#endif
            cmd_ack(cmd, IAP_ACK_BAD_PARAM);
            break;
        }
    }
}
