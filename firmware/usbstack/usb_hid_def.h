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

#ifndef USB_HID_DEF_H
#define USB_HID_DEF_H

#include "usb_ch9.h"

/* Documents avaiable here: http://www.usb.org/developers/devclass_docs/ */

#define USB_HID_VER                     0x0110 /* 1.1 */
/* Subclass Codes (HID1_11.pdf, page 18) */
#define USB_HID_SUBCLASS_NONE           0
#define USB_HID_SUBCLASS_BOOT_INTERFACE 1
/* Protocol Codes (HID1_11.pdf, page 19) */
#define USB_HID_PROTOCOL_CODE_NONE      0
#define USB_HID_PROTOCOL_CODE_KEYBOARD  1
#define USB_HID_PROTOCOL_CODE_MOUSE     2
/* HID main items (HID1_11.pdf, page 38) */
#define USB_HID_INPUT                   0x80
#define USB_HID_OUTPUT                  0x90
#define USB_HID_FEATURE                 0xB0
#define USB_HID_COLLECTION              0xA0
#define USB_HID_COLLECTION_PHYSICAL     0x00
#define USB_HID_COLLECTION_APPLICATION  0x01
#define USB_HID_END_COLLECTION          0xC0
/* Parts (HID1_11.pdf, page 40) */
#define USB_HID_MAIN_ITEM_CONSTANT       BIT_N(0) /* 0x01 */
#define USB_HID_MAIN_ITEM_VARIABLE       BIT_N(1) /* 0x02 */
#define USB_HID_MAIN_ITEM_RELATIVE       BIT_N(2) /* 0x04 */
#define USB_HID_MAIN_ITEM_WRAP           BIT_N(3) /* 0x08 */
#define USB_HID_MAIN_ITEM_NONLINEAR      BIT_N(4) /* 0x10 */
#define USB_HID_MAIN_ITEM_NO_PREFERRED   BIT_N(5) /* 0x20 */
#define USB_HID_MAIN_ITEM_NULL_STATE     BIT_N(6) /* 0x40 */
#define USB_HID_MAIN_ITEM_VOLATILE       BIT_N(7) /* 0x80 */
#define USB_HID_MAIN_ITEM_BUFFERED_BYTES BIT_N(8) /* 0x0100 */
/* HID global items (HID1_11.pdf, page 45) */
#define USB_HID_USAGE_PAGE          0x04
#define USB_HID_LOGICAL_MINIMUM     0x14
#define USB_HID_LOGICAL_MAXIMUM     0x24
#define USB_HID_REPORT_SIZE         0x74
#define USB_HID_REPORT_ID           0x84
#define USB_HID_REPORT_COUNT        0x94
/* HID local items (HID1_11.pdf, page 50) */
#define USB_HID_USAGE_MINIMUM       0x18
#define USB_HID_USAGE_MAXIMUM       0x28
/* Types of class descriptors (HID1_11.pdf, page 59) */
#define USB_DT_HID                  0x21
#define USB_DT_REPORT               0x22

#define USB_HID_CONSUMER_USAGE      0x09

/* HID-only class specific requests (HID1_11.pdf, page 61) */
#define USB_HID_GET_REPORT              0x01
#define USB_HID_GET_IDLE                0x02
#define USB_HID_GET_PROTOCOL            0x03
#define USB_HID_SET_REPORT              0x09
#define USB_HID_SET_IDLE                0x0A
#define USB_HID_SET_PROTOCOL            0x0B

/* Get_Report and Set_Report Report Type field (HID1_11.pdf, page 61) */
#define USB_HID_REPORT_TYPE_INPUT       0x01
#define USB_HID_REPORT_TYPE_OUTPUT      0x02
#define USB_HID_REPORT_TYPE_FEATURE     0x03

struct usb_hid_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t wBcdHID;
    uint8_t  bCountryCode;
    uint8_t  bNumDescriptors;
    uint8_t  bDescriptorType0;
    uint16_t wDescriptorLength0;
} __attribute__ ((packed));

#endif
