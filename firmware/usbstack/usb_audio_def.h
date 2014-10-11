/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:  $
 *
 * Copyright (C) 2010 by Amaury Pouly
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
/* Parts of this file are based on Frank Gevaerts work and on audio.h from linux */

#ifndef USB_AUDIO_DEF_H
#define USB_AUDIO_DEF_H

#include "usb_ch9.h"

#define USB_SUBCLASS_AUDIO_CONTROL      1
#define USB_SUBCLASS_AUDIO_STREAMING    2

#define USB_AC_HEADER           1
#define USB_AC_INPUT_TERMINAL   2
#define USB_AC_OUTPUT_TERMINAL  3
#define USB_AC_MIXER_UNIT       4
#define USB_AC_SELECTOR_UNIT    5
#define USB_AC_FEATURE_UNIT     6
#define USB_AC_PROCESSING_UNIT  8
#define USB_AC_EXTENSION_UNIT   9

#define USB_AS_GENERAL      1
#define USB_AS_FORMAT_TYPE  2

#define USB_AC_CHANNEL_LEFT_FRONT   0x1
#define USB_AC_CHANNEL_RIGHT_FRONT  0x2

#define USB_AC_CHANNELS_LEFT_RIGHT_FRONT    (USB_AC_CHANNEL_LEFT_FRONT | USB_AC_CHANNEL_RIGHT_FRONT)

/* usb audio data structures */
struct usb_ac_header {
    uint8_t  bLength;
    uint8_t  bDescriptorType; /* USB_DT_CS_INTERFACE */
    uint8_t  bDescriptorSubType; /* USB_AC_HEADER */
    uint16_t  bcdADC;
    uint16_t wTotalLength;
    uint8_t  bInCollection;
    uint8_t  baInterfaceNr[];
} __attribute__ ((packed));

#define USB_AC_SIZEOF_HEADER(n)         (8 + (n))

#define USB_AC_TERMINAL_UNDEFINED       0x100
#define USB_AC_TERMINAL_STREAMING       0x101
#define USB_AC_TERMINAL_VENDOR_SPEC     0x1ff

#define USB_AC_EXT_TERMINAL_UNDEFINED   0x600
#define USB_AC_EXT_TERMINAL_ANALOG      0x601
#define USB_AC_EXT_TERMINAL_DIGITAL     0x602
#define USB_AC_EXT_TERMINAL_LINE        0x603
#define USB_AC_EXT_TERMINAL_LEGACY      0x604
#define USB_AC_EXT_TERMINAL_SPDIF       0x605
#define USB_AC_EXT_TERMINAL_1394_DA     0x606
#define USB_AC_EXT_TERMINAL_1394_DV     0x607

#define USB_AC_EMB_MINIDISK     0x706
#define USB_AC_EMB_RADIO_RECV   0x710

#define USB_AC_INPUT_TERMINAL_UNDEFINED             0x200
#define USB_AC_INPUT_TERMINAL_MICROPHONE            0x201
#define USB_AC_INPUT_TERMINAL_DESKTOP_MICROPHONE    0x202
#define USB_AC_INPUT_TERMINAL_PERSONAL_MICROPHONE   0x203
#define USB_AC_INPUT_TERMINAL_OMNI_DIR_MICROPHONE   0x204
#define USB_AC_INPUT_TERMINAL_MICROPHONE_ARRAY      0x205
#define USB_AC_INPUT_TERMINAL_PROC_MICROPHONE_ARRAY 0x206

struct usb_ac_input_terminal {
    uint8_t  bLength;
    uint8_t  bDescriptorType; /* USB_DT_CS_INTERFACE */
    uint8_t  bDescriptorSubType; /* USB_AC_INPUT_TERMINAL */
    uint8_t  bTerminalId;
    uint16_t wTerminalType; /* USB_AC_INPUT_TERMINAL_* */
    uint8_t  bAssocTerminal;
    uint8_t  bNrChannels;
    uint16_t wChannelConfig;
    uint8_t  iChannelNames;
    uint8_t  iTerminal;
} __attribute__ ((packed));

#define USB_AC_OUTPUT_TERMINAL_UNDEFINED                    0x300
#define USB_AC_OUTPUT_TERMINAL_SPEAKER                      0x301
#define USB_AC_OUTPUT_TERMINAL_HEADPHONES                   0x302
#define USB_AC_OUTPUT_TERMINAL_HEAD_MOUNTED_DISPLAY_AUDIO   0x303
#define USB_AC_OUTPUT_TERMINAL_DESKTOP_SPEAKER              0x304
#define USB_AC_OUTPUT_TERMINAL_ROOM_SPEAKER                 0x305
#define USB_AC_OUTPUT_TERMINAL_COMMUNICATION_SPEAKER        0x306
#define USB_AC_OUTPUT_TERMINAL_LOW_FREQ_EFFECTS_SPEAKER     0x307

struct usb_ac_output_terminal {
    uint8_t  bLength;
    uint8_t  bDescriptorType; /* USB_DT_CS_INTERFACE */
    uint8_t  bDescriptorSubType; /* USB_AC_OUTPUT_TERMINAL */
    uint8_t  bTerminalId;
    uint16_t wTerminalType; /* USB_AC_OUTPUT_TERMINAL_* */
    uint8_t  bAssocTerminal;
    uint8_t  bSourceId;
    uint8_t  iTerminal;
} __attribute__ ((packed));

#define USB_AC_FU_CONTROL_UNDEFINED 0x00
#define USB_AC_MUTE_CONTROL         0x01
#define USB_AC_VOLUME_CONTROL       0x02
#define USB_AC_BASS_CONTROL         0x03
#define USB_AC_MID_CONTROL          0x04
#define USB_AC_TREBLE_CONTROL       0x05
#define USB_AC_EQUALIZER_CONTROL    0x06
#define USB_AC_AUTO_GAIN_CONTROL    0x07
#define USB_AC_DELAY_CONTROL        0x08
#define USB_AC_BASS_BOOST_CONTROL   0x09
#define USB_AC_LOUDNESS_CONTROL     0x0a

#define USB_AC_FU_MUTE          (1 << (USB_AC_MUTE_CONTROL - 1))
#define USB_AC_FU_VOLUME        (1 << (USB_AC_VOLUME_CONTROL - 1))
#define USB_AC_FU_BASS          (1 << (USB_AC_BASS_CONTROL - 1))
#define USB_AC_FU_MID           (1 << (USB_AC_MID_CONTROL - 1))
#define USB_AC_FU_TREBLE        (1 << (USB_AC_TREBLE_CONTROL - 1))
#define USB_AC_FU_EQUILIZER     (1 << (USB_AC_EQUALIZER_CONTROL - 1))
#define USB_AC_FU_AUTO_GAIN     (1 << (USB_AC_AUTO_GAIN_CONTROL - 1))
#define USB_AC_FU_DELAY         (1 << (USB_AC_DELAY_CONTROL - 1))
#define USB_AC_FU_BASS_BOOST    (1 << (USB_AC_BASS_BOOST_CONTROL - 1))
#define USB_AC_FU_LOUDNESS      (1 << (USB_AC_LOUDNESS_CONTROL - 1))

#define DEFINE_USB_AC_FEATURE_UNIT(n,ch) \
struct usb_ac_feature_unit_##n##_##ch { \
    uint8_t   bLength; \
    uint8_t   bDescriptorType; /* USB_DT_CS_INTERFACE */ \
    uint8_t   bDescriptorSubType; /* USB_AC_FEATURE_UNIT */ \
    uint8_t   bUnitId; \
    uint8_t   bSourceId; \
    uint8_t   bControlSize; \
    uint##n##_t bmaControls[ch + 1]; /* FU_* ORed*/ \
    uint8_t   iFeature; \
} __attribute__ ((packed));

#define USB_AC_SET_REQ  0x00
#define USB_AC_GET_REQ  0x80

#define USB_AC_CUR_REQ  0x1
#define USB_AC_MIN_REQ  0x2
#define USB_AC_MAX_REQ  0x3
#define USB_AC_RES_REQ  0x4
#define USB_AC_MEM_REQ  0x5

#define USB_AC_SET_CUR  (USB_AC_SET_REQ | USB_AC_CUR_REQ)
#define USB_AC_GET_CUR  (USB_AC_GET_REQ | USB_AC_CUR_REQ)
#define USB_AC_SET_MIN  (USB_AC_SET_REQ | USB_AC_MIN_REQ)
#define USB_AC_GET_MIN  (USB_AC_GET_REQ | USB_AC_MIN_REQ)
#define USB_AC_SET_MAX  (USB_AC_SET_REQ | USB_AC_MAX_REQ)
#define USB_AC_GET_MAX  (USB_AC_GET_REQ | USB_AC_MAX_REQ)
#define USB_AC_SET_RES  (USB_AC_SET_REQ | USB_AC_RES_REQ)
#define USB_AC_GET_RES  (USB_AC_GET_REQ | USB_AC_RES_REQ)
#define USB_AC_SET_MEM  (USB_AC_SET_REQ | USB_AC_MEM_REQ)
#define USB_AC_GET_MEM  (USB_AC_GET_REQ | USB_AC_MEM_REQ)
#define USB_AC_GET_STAT 0xff

#define USB_AS_FORMAT_TYPE_UNDEFINED    0x0
#define USB_AS_FORMAT_TYPE_I            0x1
#define USB_AS_FORMAT_TYPE_II           0x2
#define USB_AS_FORMAT_TYPE_III          0x3

struct usb_as_interface {
    uint8_t  bLength;
    uint8_t  bDescriptorType; /* USB_DT_CS_INTERFACE */
    uint8_t  bDescriptorSubType; /* USB_AS_GENERAL */
    uint8_t  bTerminalLink;
    uint8_t  bDelay;
    uint16_t wFormatTag;
} __attribute__ ((packed));

#define USB_AS_EP_GENERAL   0x01

#define USB_AS_EP_CS_SAMPLING_FREQ_CTL  0x01
#define USB_AS_EP_CS_PITCH_CTL          0x02

struct usb_iso_audio_endpoint_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;

    uint8_t  bEndpointAddress;
    uint8_t  bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t  bInterval;
    uint8_t  bRefresh;
    uint8_t  bSynchAddress;
} __attribute__ ((packed));

struct usb_as_iso_endpoint {
    uint8_t  bLength;
    uint8_t  bDescriptorType; /* USB_DT_CS_ENDPOINT */
    uint8_t  bDescriptorSubType; /* USB_AS_EP_GENERAL */
    uint8_t  bmAttributes;
    uint8_t  bLockDelayUnits;
    uint16_t wLockDelay;
} __attribute__ ((packed));

#define USB_AS_FORMAT_TYPE_I_UNDEFINED  0x0
#define USB_AS_FORMAT_TYPE_I_PCM        0x1
#define USB_AS_FORMAT_TYPE_I_PCM8       0x2
#define USB_AS_FORMAT_TYPE_I_IEEE_FLOAT 0x3
#define USB_AS_FORMAT_TYPE_I_ALAW       0x4
#define USB_AS_FORMAT_TYPE_I_MULAW      0x5

struct usb_as_format_type_i_discrete {
    uint8_t  bLength;
    uint8_t  bDescriptorType;       /* USB_DT_CS_INTERFACE */
    uint8_t  bDescriptorSubType;    /* USB_AS_FORMAT_TYPE */
    uint8_t  bFormatType;   /* USB_AS_FORMAT_TYPE_I */
    uint8_t  bNrChannels;
    uint8_t  bSubframeSize;
    uint8_t  bBitResolution;
    uint8_t  bSamFreqType; /* Number of discrete frequencies */
    uint8_t  tSamFreq[][3];
} __attribute__ ((packed));

#define USB_AS_SIZEOF_FORMAT_TYPE_I_DISCRETE(n) (8 + (n * 3))

#endif /* USB_AUDIO_DEF_H */
