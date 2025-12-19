#pragma once
/* References:
 * [1]: MFi Accessory Firmware Specification R46
 * [2]: MFI Accessory Hardware Specification R9
 * [3]: MFi Accessory Interface Specification For Apple Devices R2
 */

#define IAP_SYNC_BYTE 0xFF
#define IAP_SOF_BYTE  0x55

/* [1] P.109 Table 2-10 iAP command packet format
 * | name | size | description |
 * | sync       | 0 or 1 | IAP_SYNC_BYTE, exists if UART transport is used |
 * | sof        | 1      | IAP_SOF_BYTE |
 * | length     | 1 or 3 | 0xNN or 0x00NNNN. sum of length of {lingo,command,trans}_id,payload |
 * | lingo_id   | 1      | IAP_LINGO_ID, lingo identifier |
 * | command_id | 1 or 2 | 2 bytes long if lingo == 4 |
 * | trans_id   | 0 or 2 | exists for some commands |
 * | payload    | N      | data |
 * | checksum   | 1      | crc checksum |
 */

/* [1] P.211 Table 4-1 Additional iAP lingoes */
enum IAPLingoID {
    IAPLingoID_General            = 0x00,
    IAPLingoID_Microphone         = 0x01,
    IAPLingoID_SimpleRemote       = 0x02,
    IAPLingoID_DisplayRemote      = 0x03,
    IAPLingoID_ExtendedInterface  = 0x04,
    IAPLingoID_AccessoryPower     = 0x05,
    IAPLingoID_USBHostMode        = 0x06,
    IAPLingoID_RFTuner            = 0x07,
    IAPLingoID_AccessoryEqualizer = 0x08,
    IAPLingoID_Sports             = 0x09,
    IAPLingoID_DigitalAudio       = 0x0A,
    IAPLingoID_Storage            = 0x0C,
    IAPLingoID_IPodOut            = 0x0D,
    IAPLingoID_Location           = 0x0E,
};

#include "lingoes/accessory-equalizer.h"
#include "lingoes/accessory-power.h"
#include "lingoes/digital-audio.h"
#include "lingoes/display-remote.h"
#include "lingoes/extended-interface.h"
#include "lingoes/general.h"
#include "lingoes/ipod-out.h"
#include "lingoes/location.h"
#include "lingoes/microphone.h"
#include "lingoes/rf-tuner.h"
#include "lingoes/simple-remote.h"
#include "lingoes/sports.h"
#include "lingoes/storage.h"
#include "lingoes/usb-host-mode.h"
